/**
 * Copyright (c) 2016 - 2018, Nordic Semiconductor ASA
 * 
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 * 
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 * 
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 * 
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 * 
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */
#include "dfu_req_handling.h"

#include <stdint.h>
#include <stdbool.h>
#include "nrf_dfu.h"
#include "nrf_dfu_types.h"
#include "nrf_dfu_req_handler.h"
#include "nrf_dfu_handling_error.h"
#include "nrf_dfu_settings.h"
#include "nrf_dfu_transport.h"
#include "nrf_dfu_utils.h"
#include "nrf_dfu_flash.h"
#include "nrf_bootloader_info.h"
#include "nrf_delay.h"
#include "app_util.h"
#include "app_timer.h"
#include "pb.h"
#include "pb_common.h"
#include "pb_decode.h"
#include "dfu-cc.pb.h"
#include "crc32.h"
#ifdef SOFTDEVICE_PRESENT
#include "nrf_sdm.h"
#endif
#include "sdk_macros.h"
#include "nrf_crypto.h"

#define NRF_LOG_MODULE_NAME dfu_req_handling
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
NRF_LOG_MODULE_REGISTER();

#include "sr3_config.h"

STATIC_ASSERT(DFU_SIGNED_COMMAND_SIZE <= INIT_COMMAND_MAX_SIZE);

#if !defined(NRF_DFU_UPDATABLE_APPLICATION_ONLY) || (NRF_DFU_UPDATABLE_APPLICATION_ONLY == 0)
extern app_timer_id_t const nrf_dfu_post_sd_bl_timeout_timer_id;
#endif

extern app_timer_id_t const nrf_dfu_inactivity_timeout_timer_id;


/** @brief Buffers used to schedule store of received packets to FLASH. */
__ALIGN(4) static uint8_t  m_data_buf[FLASH_BUFFER_COUNT][FLASH_BUFFER_LENGTH];

STATIC_ASSERT(DATA_OBJECT_MAX_SIZE < sizeof(m_data_buf));

static size_t m_data_buf_pos;     /**< The number of bytes written in the current buffer. */
static size_t m_write_buffer_id;  /**< Index of the currently written data buffer. Must be between 0 and FLASH_BUFFER_COUNT - 1. */
static size_t m_read_buffer_id;   /**< Index of the last read data buffer. Must be between 0 and FLASH_BUFFER_COUNT - 1. */

/** @brief Contains a pointer to a callback function when object execute should wait for FLASH.
 *
 * If FLASH is not fast enough to store data as it is received from the connected transport,
 * there is a need to wait for it before object execute can proceed.
 * */
static nrf_dfu_req_callback m_objectexecute_callback_function;

/** @brief Contains a context passed to callback function. */
static void const * m_objectexecute_callback_context;

/** @brief If true perform postvalidate after FLASH operations are completed. */
static bool m_objectexecute_callback_postvalidate;


static uint32_t m_firmware_start_addr;  /**< Start address of the current firmware image. */
static uint32_t m_firmware_size_req;    /**< The size of the entire firmware image. Defined by the init command. */


static bool m_valid_init_packet_present;        /**< True if init command was received and validated. */
static bool m_is_dfu_complete_response_sent;    /**< True if the last DFU response was successfully sent. */
static bool m_dfu_last_settings_write_started;  /**< True if the last settings write was scheduled. */

/** @brief Contains decoded DFU packet received from the host.
 *
 * @note At the moment of writing only init command is supported.
 */
static dfu_packet_t dfu_init_packet;

/** @brief Holds a pointer to the raw init packet data. */
static nrf_value_length_t init_packet_data;

static nrf_crypto_hash_info_t const hash_info_sha256 =
{
    .hash_type   = NRF_CRYPTO_HASH_TYPE_SHA256,
    .endian_type = NRF_CRYPTO_ENDIAN_LE
};

static nrf_crypto_signature_info_t const sig_info_p256 =
{
    .curve_type  = NRF_CRYPTO_CURVE_SECP256R1,
    .hash_type   = NRF_CRYPTO_HASH_TYPE_SHA256,
    .endian_type = NRF_CRYPTO_ENDIAN_LE
};

/** @brief Value length structure holding the public key.
 *
 * @details The pk value pointed to is the public key present in dfu_public_key.c
 */
NRF_CRYPTO_ECC_PUBLIC_KEY_RAW_CREATE_FROM_ARRAY(crypto_key_pk, SECP256R1, pk);

/** @brief Value length structure to hold a signature
 */
NRF_CRYPTO_ECDSA_SIGNATURE_CREATE(crypto_sig, SECP256R1);

/** @brief Value length structure to hold the hash for the init packet
 */
NRF_CRYPTO_HASH_CREATE(init_packet_hash, SHA256);

/** @brief Value length structure to hold the hash for the firmware image
 */
NRF_CRYPTO_HASH_CREATE(fw_hash, SHA256);

/** @brief Timer used to reset the device when DFU is completed. */
APP_TIMER_DEF(m_reset_timer);


static void reset_device(void * p_context)
{
    NRF_LOG_DEBUG("Attempting to reset the device.");

    if (nrf_fstorage_is_busy(NULL))
    {
        NRF_LOG_INFO("Waiting until all flash operations are completed.");
        return;
    }

    if (!m_is_dfu_complete_response_sent)
    {
        NRF_LOG_INFO("Waiting until the response is sent.");
        return;
    }

#ifndef NRF_DFU_NO_TRANSPORT
    (void)nrf_dfu_transports_close();
#endif

#if NRF_LOG_ENABLED
    NRF_LOG_DEBUG("Reset.");
    NRF_LOG_FLUSH();
    nrf_delay_ms(100);
#endif

    NVIC_SystemReset();
}


static void nrf_dfu_completion_timer_start(void)
{
    static bool timer_started = false;

    // Start a timer to reset the device when all operations are completed.
    if (!timer_started)
    {
        ret_code_t err_code;

        NRF_LOG_DEBUG("Starting reset timer.");

        err_code = app_timer_create(&m_reset_timer, APP_TIMER_MODE_REPEATED, reset_device);
        APP_ERROR_CHECK(err_code);
        err_code = app_timer_start(m_reset_timer, APP_TIMER_TICKS(50), NULL);
        APP_ERROR_CHECK(err_code);

        timer_started = true;
    }
}


void nrf_dfu_req_handler_reset_if_dfu_complete(void)
{
    if (m_dfu_last_settings_write_started)
    {
        // Set a flag and wait for the on_dfu_complete callback.
        NRF_LOG_DEBUG("Response sent.");
        m_is_dfu_complete_response_sent = true;
    }
}


static void inactivity_timer_restart(void)
{
    APP_ERROR_CHECK(app_timer_stop(nrf_dfu_inactivity_timeout_timer_id));
    APP_ERROR_CHECK(app_timer_start(nrf_dfu_inactivity_timeout_timer_id,
                                    APP_TIMER_TICKS(NRF_DFU_INACTIVITY_TIMEOUT_MS),
                                    NULL));
}


static void pb_decoding_callback(pb_istream_t *str, uint32_t tag, pb_wire_type_t wire_type, void *iter)
{
    pb_field_iter_t *p_iter = (pb_field_iter_t *) iter;

    // match the beginning of the init command
    if (p_iter->pos->ptr == &dfu_init_command_fields[0])
    {
        uint8_t *ptr = (uint8_t *) str->state;
        uint32_t size = str->bytes_left;

        // remove tag byte
        ptr++;
        size--;

        // store the info in init_packet_data
        init_packet_data.p_value = ptr;
        init_packet_data.length = size;

        NRF_LOG_INFO("PB: Init packet data len: %d", size);
    }
}


static nrf_dfu_res_code_t dfu_handle_prevalidate(dfu_signed_command_t const * p_command, pb_istream_t * p_stream, uint8_t * p_init_cmd, uint32_t init_cmd_len)
{
    dfu_init_command_t const * p_init = &p_command->command.init;
    uint32_t                   err_code;
    uint32_t                   hw_version = CONFIG_DFU_HW_VERSION;
    uint32_t                   fw_version = 0;

    // check for init command found during decoding
    if (!p_init_cmd || !init_cmd_len)
    {
        return ext_error_set(NRF_DFU_EXT_ERROR_INIT_COMMAND_INVALID);
    }

#ifndef NRF_DFU_DEBUG_VERSION
    if ((p_init->has_is_debug != false) && (p_init->is_debug != false))
    {
        // Prevalidation should fail if debug is set in an init package for a non-debug Secure DFU bootloader.
        return NRF_DFU_RES_CODE_OPERATION_FAILED;
    }
#else
    if ((p_init->has_is_debug == false) || (p_init->is_debug == false))
    {
#endif
        if (p_init->has_hw_version == false)
        {
            NRF_LOG_ERROR("No HW version");
            return ext_error_set(NRF_DFU_EXT_ERROR_INIT_COMMAND_INVALID);
        }

        // Check of init command HW version
        if (p_init->hw_version != hw_version)
        {
            NRF_LOG_ERROR("Faulty HW version");
            return ext_error_set(NRF_DFU_EXT_ERROR_HW_VERSION_FAILURE);
        }

#ifdef SOFTDEVICE_PRESENT
        // Precheck the SoftDevice version
        bool found_sd_ver = false;
        for (size_t i = 0; i < p_init->sd_req_count; i++)
        {
            if (p_init->sd_req[i] == SD_FWID_GET(MBR_SIZE))
            {
                found_sd_ver = true;
                break;
            }
        }
        if (!found_sd_ver)
        {
            NRF_LOG_ERROR("SD req not met");
            return ext_error_set(NRF_DFU_EXT_ERROR_SD_VERSION_FAILURE);
        }
#endif

        // Get the fw version
        switch (p_init->type)
        {
            case DFU_FW_TYPE_APPLICATION:
                if (p_init->has_fw_version == false)
                {
                    return ext_error_set(NRF_DFU_EXT_ERROR_INIT_COMMAND_INVALID);
                }
                // Get the application FW version
                fw_version = s_dfu_settings.app_version;
                break;

#if !defined(NRF_DFU_UPDATABLE_APPLICATION_ONLY) || (NRF_DFU_UPDATABLE_APPLICATION_ONLY == 0)
            case DFU_FW_TYPE_SOFTDEVICE:
                // not loaded
                break;

            case DFU_FW_TYPE_BOOTLOADER: // fall through
            case DFU_FW_TYPE_SOFTDEVICE_BOOTLOADER:
                if (p_init->has_fw_version == false)
                {
                    return ext_error_set(NRF_DFU_EXT_ERROR_INIT_COMMAND_INVALID);
                }
                fw_version = s_dfu_settings.bootloader_version;
                break;
#endif

            default:
                NRF_LOG_INFO("Unknown FW update type");
                return ext_error_set(NRF_DFU_EXT_ERROR_INIT_COMMAND_INVALID);
        }

        NRF_LOG_INFO("Req version: %d, Expected: %d", p_init->fw_version, fw_version);

        // Check of init command FW version
        switch (p_init->type)
        {
            case DFU_FW_TYPE_APPLICATION:
#if !defined(NRF_DFU_UPDATABLE_APPLICATION_ONLY) || (NRF_DFU_UPDATABLE_APPLICATION_ONLY == 0)
            case DFU_FW_TYPE_BOOTLOADER:            // fall through
            case DFU_FW_TYPE_SOFTDEVICE_BOOTLOADER:
#endif
                if (p_init->fw_version < fw_version)
                {
                    NRF_LOG_ERROR("FW version too low");
                    return ext_error_set(NRF_DFU_EXT_ERROR_FW_VERSION_FAILURE);
                }
                break;

            default:
                // do not care about fw_version in the case of a softdevice transfer
                break;
        }

#ifdef NRF_DFU_DEBUG_VERSION
    }
#endif

    // Check the signature
    switch (p_command->signature_type)
    {
        case DFU_SIGNATURE_TYPE_ECDSA_P256_SHA256:
            {
                NRF_LOG_INFO("Calculating init packet hash");
                err_code = nrf_crypto_hash_compute(hash_info_sha256, p_init_cmd, init_cmd_len, &init_packet_hash);
                if (err_code != NRF_SUCCESS)
                {
                    NRF_LOG_WARNING("%s(): failed to compute hash", __func__);
                    return NRF_DFU_RES_CODE_OPERATION_FAILED;
                }

                if (crypto_sig.length != p_command->signature.size)
                {
                    NRF_LOG_WARNING("%s(): invalid signature size", __func__);
                    return NRF_DFU_RES_CODE_OPERATION_FAILED;
                }

                // Prepare the signature received over the air.
                memcpy(crypto_sig.p_value, p_command->signature.bytes, p_command->signature.size);

                // calculate the signature
                err_code = nrf_crypto_ecdsa_verify_hash(sig_info_p256, &crypto_key_pk, &init_packet_hash, &crypto_sig);
                if (err_code != NRF_SUCCESS)
                {
                    NRF_LOG_WARNING("%s(): signature check failed", __func__);
                    return NRF_DFU_RES_CODE_INVALID_OBJECT;
                }

                NRF_LOG_INFO("Image verified");
            }
            break;

        default:
            NRF_LOG_WARNING("%s(): invalid signature type", __func__);
            return ext_error_set(NRF_DFU_EXT_ERROR_WRONG_SIGNATURE_TYPE);
    }

    // Get the update size
    m_firmware_size_req = 0;

    switch (p_init->type)
    {
        case DFU_FW_TYPE_APPLICATION:
            if (p_init->has_app_size == false)
            {
                NRF_LOG_ERROR("No app image size");
                return ext_error_set(NRF_DFU_EXT_ERROR_INIT_COMMAND_INVALID);
            }
            m_firmware_size_req += p_init->app_size;
            break;

#if !defined(NRF_DFU_UPDATABLE_APPLICATION_ONLY) || (NRF_DFU_UPDATABLE_APPLICATION_ONLY == 0)
        case DFU_FW_TYPE_BOOTLOADER:
            if (p_init->has_bl_size == false)
            {
                NRF_LOG_ERROR("No bl image size");
                return ext_error_set(NRF_DFU_EXT_ERROR_INIT_COMMAND_INVALID);
            }
            m_firmware_size_req += p_init->bl_size;
            // check that the size of the bootloader is not larger than the present one.
#if defined ( NRF51 )
            if (p_init->bl_size > BOOTLOADER_SETTINGS_ADDRESS - BOOTLOADER_START_ADDR)
#elif defined( NRF52_SERIES )
            if (p_init->bl_size > NRF_MBR_PARAMS_PAGE_ADDRESS - BOOTLOADER_START_ADDR)
#endif
            {
                NRF_LOG_ERROR("BL too large");
                return NRF_DFU_RES_CODE_INSUFFICIENT_RESOURCES;
            }
            break;

        case DFU_FW_TYPE_SOFTDEVICE:
            if (p_init->has_sd_size == false)
            {
                NRF_LOG_ERROR("No SD image size");
                return ext_error_set(NRF_DFU_EXT_ERROR_INIT_COMMAND_INVALID);
            }
            m_firmware_size_req += p_init->sd_size;
            break;

        case DFU_FW_TYPE_SOFTDEVICE_BOOTLOADER:
            if (p_init->has_bl_size == false || p_init->has_sd_size == false)
            {
                NRF_LOG_ERROR("No BL/SD size");
                return ext_error_set(NRF_DFU_EXT_ERROR_INIT_COMMAND_INVALID);
            }
            m_firmware_size_req += p_init->sd_size + p_init->bl_size;
            if (p_init->sd_size == 0 || p_init->bl_size == 0)
            {
                NRF_LOG_ERROR("BL+SD size 0");
                return NRF_DFU_RES_CODE_INVALID_PARAMETER;
            }

            // check that the size of the bootloader is not larger than the present one.
#if defined ( NRF51 )
            if (p_init->bl_size > BOOTLOADER_SETTINGS_ADDRESS - BOOTLOADER_START_ADDR)
#elif defined ( NRF52_SERIES )
            if (p_init->bl_size > NRF_MBR_PARAMS_PAGE_ADDRESS - BOOTLOADER_START_ADDR)
#endif
            {
                NRF_LOG_ERROR("BL too large (SD+BL)");
                return NRF_DFU_RES_CODE_INSUFFICIENT_RESOURCES;
            }
            break;
#endif /* !defined(NRF_DFU_UPDATABLE_APPLICATION_ONLY) || (NRF_DFU_UPDATABLE_APPLICATION_ONLY == 0) */

        default:
            NRF_LOG_ERROR("Unknown FW update type");
            return ext_error_set(NRF_DFU_EXT_ERROR_INIT_COMMAND_INVALID);
    }

    NRF_LOG_INFO("Running hash check");
    // SHA256 is the only supported hash
    memcpy(fw_hash.p_value, &p_init->hash.hash.bytes[0], NRF_CRYPTO_HASH_SIZE_SHA256);

    // Instead of checking each type with has-check, check the result of the size_req to
    // Validate its content.
    if (m_firmware_size_req == 0)
    {
        NRF_LOG_ERROR("No FW size");
        return NRF_DFU_RES_CODE_INVALID_PARAMETER;
    }

    // Find the location to place the DFU updates
    err_code = nrf_dfu_find_cache(m_firmware_size_req, &m_firmware_start_addr);
    if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("Can't find room for update");
        return NRF_DFU_RES_CODE_INSUFFICIENT_RESOURCES;
    }

    NRF_LOG_INFO("Write address set to 0x%08x", m_firmware_start_addr);

    NRF_LOG_INFO("DFU prevalidate SUCCESSFUL!");

    return NRF_DFU_RES_CODE_SUCCESS;
}


/** @brief Function for validating the received image after all objects have been received and executed.
 */
static nrf_dfu_res_code_t nrf_dfu_postvalidate(dfu_init_command_t * p_init)
{
    uint32_t             err_code;
    nrf_dfu_res_code_t   res_code = NRF_DFU_RES_CODE_SUCCESS;
    nrf_dfu_bank_t     * p_bank;

    NRF_LOG_INFO("%s()", __func__);

    switch (p_init->hash.hash_type)
    {
        case DFU_HASH_TYPE_SHA256:
            err_code = nrf_crypto_hash_compute(hash_info_sha256, (uint8_t *)m_firmware_start_addr, m_firmware_size_req, &fw_hash);
            if (err_code != NRF_SUCCESS)
            {
                NRF_LOG_WARNING("%s(): failed to compute hash", __func__);
                res_code = NRF_DFU_RES_CODE_OPERATION_FAILED;
            }
            else if (memcmp(fw_hash.p_value, p_init->hash.hash.bytes, NRF_CRYPTO_HASH_SIZE_SHA256) != 0)
            {
                NRF_LOG_WARNING("%s(): invalid signature", __func__);

                res_code = ext_error_set(NRF_DFU_EXT_ERROR_VERIFICATION_FAILED);
            }
            break;

        default:
            NRF_LOG_WARNING("%s(): invalid signature type", __func__);
            res_code = ext_error_set(NRF_DFU_EXT_ERROR_WRONG_HASH_TYPE);
            break;
    }

    if (s_dfu_settings.bank_current == NRF_DFU_CURRENT_BANK_0)
    {
        NRF_LOG_INFO("Current bank is bank 0");
        p_bank = &s_dfu_settings.bank_0;
    }
#if defined(NRF_DFU_DUAL_BANK_SUPPORT) && (NRF_DFU_DUAL_BANK_SUPPORT == 1)
    else if (s_dfu_settings.bank_current == NRF_DFU_CURRENT_BANK_1)
    {
        NRF_LOG_INFO("Current bank is bank 1");
        p_bank = &s_dfu_settings.bank_1;
    }
#endif
    else
    {
        NRF_LOG_ERROR("Internal error, invalid current bank");
        res_code = NRF_DFU_RES_CODE_OPERATION_FAILED;
    }

    STATIC_ASSERT(DFU_FW_TYPE_MIN == 0);
    if (p_init->type > DFU_FW_TYPE_MAX)
    {
        NRF_LOG_WARNING("%s(): invalid image type 0x%x", __func__, p_init->type);
        res_code = ext_error_set(NRF_DFU_EXT_ERROR_INIT_COMMAND_INVALID);
    }

#if defined(SOFTDEVICE_PRESENT)
#if !defined(NRF_DFU_UPDATABLE_APPLICATION_ONLY) || (NRF_DFU_UPDATABLE_APPLICATION_ONLY == 0)
    if ((p_init->type == DFU_FW_TYPE_SOFTDEVICE) ||
        (p_init->type == DFU_FW_TYPE_SOFTDEVICE_BOOTLOADER))
    {
        // Verify the API version of potential new SoftDevices to prevent incompatibility.
        NRF_LOG_INFO("Running SD version check ============== ");

        uint32_t cur_SD_major_minor = SD_VERSION_GET(MBR_SIZE)/1000;
        uint32_t new_SD_major_minor = SD_VERSION_GET(m_firmware_start_addr)/1000;

        uint32_t cur_SD_size        = SD_SIZE_GET(MBR_SIZE);
        uint32_t new_SD_size        = SD_SIZE_GET(m_firmware_start_addr);

        if ((cur_SD_major_minor != new_SD_major_minor) ||
            (cur_SD_size        != new_SD_size))
        {
            NRF_LOG_INFO("New and current SD are incompatible")
            NRF_LOG_INFO("SD version: current %u, new %u", cur_SD_major_minor, new_SD_major_minor)
            NRF_LOG_INFO("SD size: current %u, new %u", cur_SD_size, new_SD_size)

            // A new SD API does not match the current API.
            if (p_init->type == DFU_FW_TYPE_SOFTDEVICE)
            {
                // Update cannot progress as BL may not work with new SD.
                NRF_LOG_WARNING("Bootloader mismatches SD - update invalid");
                res_code = NRF_DFU_RES_CODE_INVALID_OBJECT;
            }
            else if (s_dfu_settings.bank_current == NRF_DFU_CURRENT_BANK_1)
            {
                NRF_LOG_WARNING("Application mismatches SD and is invalidated");
                if (s_dfu_settings.bank_current == NRF_DFU_CURRENT_BANK_1)
                {
                    s_dfu_settings.bank_0.bank_code = NRF_DFU_BANK_INVALID;
                }
            }
        }
    }
#endif
#endif

    // Finalize postvalidation by updating DFU settings.
    if (res_code == NRF_DFU_RES_CODE_SUCCESS)
    {
        NRF_LOG_INFO("Successfully ran the postvalidation check!");

        switch (p_init->type)
        {
            case DFU_FW_TYPE_APPLICATION:
                p_bank->bank_code = NRF_DFU_BANK_VALID_APP;
#if defined(NRF_DFU_DUAL_BANK_SUPPORT) && (NRF_DFU_DUAL_BANK_SUPPORT == 1)
                // If this update is in bank 1, invalidate bank 0 before starting copy routine.
                if (s_dfu_settings.bank_current == NRF_DFU_CURRENT_BANK_1)
                {
                    NRF_LOG_INFO("Invalidating old application in bank 0");
                    s_dfu_settings.bank_0.bank_code = NRF_DFU_BANK_INVALID;
                }
#endif
                break;
#if !defined(NRF_DFU_UPDATABLE_APPLICATION_ONLY) || (NRF_DFU_UPDATABLE_APPLICATION_ONLY == 0)
            case DFU_FW_TYPE_SOFTDEVICE:
                p_bank->bank_code = NRF_DFU_BANK_VALID_SD;
                s_dfu_settings.sd_size = p_init->sd_size;
                break;
            case DFU_FW_TYPE_BOOTLOADER:
                p_bank->bank_code = NRF_DFU_BANK_VALID_BL;
                break;
            case DFU_FW_TYPE_SOFTDEVICE_BOOTLOADER:
                p_bank->bank_code = NRF_DFU_BANK_VALID_SD_BL;
                s_dfu_settings.sd_size = p_init->sd_size;
                break;
#endif
            default:
                ASSERT(false);
                break;
        }

#ifdef NRF_DFU_DEBUG_VERSION
        if (p_init->has_is_debug == false || p_init->is_debug == false)
        {
#endif
            switch (p_init->type)
            {
                case DFU_FW_TYPE_APPLICATION:
                    s_dfu_settings.app_version = p_init->fw_version;
                    break;
#if !defined(NRF_DFU_UPDATABLE_APPLICATION_ONLY) || (NRF_DFU_UPDATABLE_APPLICATION_ONLY == 0)
                case DFU_FW_TYPE_BOOTLOADER:
                case DFU_FW_TYPE_SOFTDEVICE_BOOTLOADER:
                    s_dfu_settings.bootloader_version = p_init->fw_version;
                    break;
#endif
                default:
                    // no implementation
                    break;
            }
#ifdef NRF_DFU_DEBUG_VERSION
        }
#endif
        // Calculate CRC32 for image
        p_bank->image_crc = s_dfu_settings.progress.firmware_image_crc;
        p_bank->image_size = m_firmware_size_req;
    }
    else
    {
        NRF_LOG_WARNING("Failed the postvalidation check!");

        p_bank->bank_code = NRF_DFU_BANK_INVALID;

        // Calculate CRC32 for image
        p_bank->image_crc = 0;
        p_bank->image_size = 0;
    }

    // Set the progress to zero and remove the last command
    memset(&s_dfu_settings.progress,     0x00, sizeof(s_dfu_settings.progress));
    memset( s_dfu_settings.init_command, 0xFF, sizeof(s_dfu_settings.init_command));

    s_dfu_settings.write_offset = 0;

    // Store the settings to flash and reset after that
    m_dfu_last_settings_write_started = true;
    if (nrf_dfu_settings_write() != NRF_SUCCESS)
    {
        res_code = NRF_DFU_RES_CODE_OPERATION_FAILED;
    }

    nrf_dfu_completion_timer_start();

    return res_code;
}


/** @brief Function to handle signed command
 */
static nrf_dfu_res_code_t dfu_handle_signed_command(dfu_signed_command_t const * p_command, pb_istream_t * p_stream)
{
    nrf_dfu_res_code_t ret_val = NRF_DFU_RES_CODE_SUCCESS;

    // Currently only init-packet is signed
    if (p_command->command.has_init == false)
    {
        return NRF_DFU_RES_CODE_INVALID_OBJECT;
    }

    ret_val = dfu_handle_prevalidate(p_command, p_stream, init_packet_data.p_value, init_packet_data.length);
    if (ret_val == NRF_DFU_RES_CODE_SUCCESS)
    {
        NRF_LOG_INFO("Prevalidate OK.");

        // This saves the init command to flash
        NRF_LOG_INFO("Saving init command...");
        if (nrf_dfu_settings_write() != NRF_SUCCESS)
        {
            return NRF_DFU_RES_CODE_OPERATION_FAILED;
        }
    }
    else
    {
        NRF_LOG_ERROR("Prevalidate failed!");
    }
    return ret_val;
}


static nrf_dfu_res_code_t dfu_handle_command(dfu_command_t const * p_command)
{
    return ext_error_set(NRF_DFU_EXT_ERROR_UNKNOWN_COMMAND);
}


static bool dfu_decode_commmand(pb_istream_t * p_stream)
{
    *p_stream = pb_istream_from_buffer(s_dfu_settings.init_command, s_dfu_settings.progress.command_size);

    // Attach our callback to follow the field decoding.
    p_stream->decoding_callback = pb_decoding_callback;

    // Reset the variable where the init pointer and length will be stored.
    init_packet_data.p_value = NULL;
    init_packet_data.length  = 0;

    if (!pb_decode(p_stream, dfu_packet_fields, &dfu_init_packet))
    {
        NRF_LOG_ERROR("Handler: Invalid protocol buffer stream");
        return false;
    }

    return true;
}


/** @brief Function handling command requests from the transport layer.
 */
static ret_code_t nrf_dfu_command_req(void                 const * p_context,
                                      nrf_dfu_req_t        const * p_req,
                                      nrf_dfu_req_callback         p_callback)
{
    nrf_dfu_res_code_t ret_val = NRF_DFU_RES_CODE_SUCCESS;
    nrf_dfu_res_t      dfu_res = {0};
    pb_istream_t       stream;

    switch (p_req->req_type)
    {
        case NRF_DFU_OBJECT_OP_CREATE:
            NRF_LOG_INFO("Before OP create command");

            if (p_req->object_size == 0)
            {
                ret_val = NRF_DFU_RES_CODE_INVALID_PARAMETER;
                break;
            }

            if (p_req->object_size > INIT_COMMAND_MAX_SIZE)
            {
                // It is impossible to handle the command because the size is too large
                ret_val = NRF_DFU_RES_CODE_INSUFFICIENT_RESOURCES;
                break;
            }

            NRF_LOG_INFO("Valid Command Create");

#if !defined(NRF_DFU_UPDATABLE_APPLICATION_ONLY) || (NRF_DFU_UPDATABLE_APPLICATION_ONLY == 0)
            // Stop post SD/BL timeout timer.
            (void)app_timer_stop(nrf_dfu_post_sd_bl_timeout_timer_id);
#endif

            // Setting DFU to uninitialized.
            m_valid_init_packet_present = false;

            // Reset all progress to zero.
            memset(&s_dfu_settings.progress, 0, sizeof(dfu_progress_t));

            // Set the init command size.
            s_dfu_settings.progress.command_size = p_req->object_size;
            break;

        case NRF_DFU_OBJECT_OP_CRC:
            NRF_LOG_INFO("Valid Command CRC");
            dfu_res.offset = s_dfu_settings.progress.command_offset;
            dfu_res.crc    = s_dfu_settings.progress.command_crc;
            break;

        case NRF_DFU_OBJECT_OP_WRITE:
            NRF_LOG_INFO("Before OP write command");

            if ((p_req->req_len + s_dfu_settings.progress.command_offset) > s_dfu_settings.progress.command_size)
            {
                NRF_LOG_ERROR("%s(): Init command larger than expected", __func__);

                dfu_res.offset = s_dfu_settings.progress.command_offset;
                dfu_res.crc    = s_dfu_settings.progress.command_crc;

                ret_val = NRF_DFU_RES_CODE_INVALID_PARAMETER;
            }
            else
            {
                // Copy the received data to RAM, updating offset and calculating CRC.
                memcpy(&s_dfu_settings.init_command[s_dfu_settings.progress.command_offset], p_req->p_req, p_req->req_len);

                s_dfu_settings.progress.command_offset += p_req->req_len;
                s_dfu_settings.progress.command_crc     = crc32_compute(p_req->p_req, p_req->req_len, &s_dfu_settings.progress.command_crc);

                dfu_res.offset = s_dfu_settings.progress.command_offset;
                dfu_res.crc    = s_dfu_settings.progress.command_crc;
            }
            break;

        case NRF_DFU_OBJECT_OP_EXECUTE:
            NRF_LOG_INFO("Before OP execute command");
            if (s_dfu_settings.progress.command_offset != s_dfu_settings.progress.command_size)
            {
                // The object wasn't the right (requested) size
                NRF_LOG_ERROR("Execute with faulty offset");
                ret_val = NRF_DFU_RES_CODE_OPERATION_NOT_PERMITTED;
                break;
            }

            NRF_LOG_INFO("Valid command execute");

            if (m_valid_init_packet_present)
            {
                // Init command already executed
                ret_val = NRF_DFU_RES_CODE_SUCCESS;
                break;
            }

            if (dfu_decode_commmand(&stream) == false)
            {
                ret_val = NRF_DFU_RES_CODE_INVALID_OBJECT;
                break;
            }

            // We have a valid DFU packet
            if (dfu_init_packet.has_signed_command)
            {
                NRF_LOG_INFO("Handling signed command");
                ret_val = dfu_handle_signed_command(&dfu_init_packet.signed_command, &stream);
            }
            else if (dfu_init_packet.has_command)
            {
                NRF_LOG_INFO("Handling unsigned command");
                ret_val = dfu_handle_command(&dfu_init_packet.command);
            }
            else
            {
                // We had no regular or signed command.
                NRF_LOG_ERROR("Decoded command but it has no content!!");
                ret_val = NRF_DFU_RES_CODE_INVALID_OBJECT;
                break;
            }

            if (ret_val == NRF_DFU_RES_CODE_SUCCESS)
            {
                // Setting DFU to initialized
                NRF_LOG_INFO("Setting DFU flag to initialized");
                m_valid_init_packet_present = true;
            }
            break;

        case NRF_DFU_OBJECT_OP_SELECT:
            NRF_LOG_INFO("Valid Command: NRF_DFU_OBJECT_OP_SELECT");
            dfu_res.offset   = s_dfu_settings.progress.command_offset;
            dfu_res.crc      = s_dfu_settings.progress.command_crc;
            dfu_res.max_size = INIT_COMMAND_MAX_SIZE;
            break;

        default:
            NRF_LOG_ERROR("Invalid Command Operation");
            ret_val = NRF_DFU_RES_CODE_OP_CODE_NOT_SUPPORTED;
            break;
    }

    p_callback(p_context, ret_val, &dfu_res);

    return NRF_SUCCESS;
}


static void nrf_dfu_flash_store_completed(nrf_fstorage_evt_t * p_evt)
{
    m_read_buffer_id = (m_read_buffer_id + 1) % FLASH_BUFFER_COUNT;

    if (m_objectexecute_callback_function != NULL)
    {
        size_t used_buffers = (FLASH_BUFFER_COUNT + m_write_buffer_id - m_read_buffer_id) % FLASH_BUFFER_COUNT;
        size_t available_buffers = FLASH_BUFFER_COUNT - used_buffers;

        // Make sure there are enough buffers to complete object execute command.
        // If this is last transfer, wait for all buffers to be stored and perform postvalidate.
        if (( m_objectexecute_callback_postvalidate && (m_read_buffer_id == m_write_buffer_id)) ||
            (!m_objectexecute_callback_postvalidate && (available_buffers * FLASH_BUFFER_LENGTH >= DATA_OBJECT_MAX_SIZE)))
        {
            nrf_dfu_res_code_t ret_code = NRF_DFU_RES_CODE_SUCCESS;
            if (m_objectexecute_callback_postvalidate)
            {
                ret_code = nrf_dfu_postvalidate(&dfu_init_packet.signed_command.command.init);
            }
            m_objectexecute_callback_function(m_objectexecute_callback_context, ret_code, NULL);
            m_objectexecute_callback_function = NULL;
        }
    }
}


static ret_code_t nrf_dfu_data_req_create(void                 const * p_context,
                                          nrf_dfu_req_t        const * p_req,
                                          nrf_dfu_req_callback         p_callback)
{
    // Validate state

    if (s_dfu_settings.progress.data_object_size != 0)
    {
        NRF_LOG_WARNING("%s(): previous object was not executed", __func__);
        p_callback(p_context, NRF_DFU_RES_CODE_OPERATION_NOT_PERMITTED, NULL);
        return NRF_SUCCESS;
    }

    // Validate object size

    if (p_req->object_size == 0)
    {
        NRF_LOG_WARNING("%s(): object size is zero", __func__);
        p_callback(p_context, NRF_DFU_RES_CODE_INVALID_PARAMETER, NULL);
        return NRF_SUCCESS;
    }

    if (p_req->object_size > DATA_OBJECT_MAX_SIZE)
    {
        NRF_LOG_WARNING("%s(): object size is too large (%u)", __func__, p_req->object_size);
        p_callback(p_context, NRF_DFU_RES_CODE_INSUFFICIENT_RESOURCES, NULL);
        return NRF_SUCCESS;
    }

    if (((p_req->object_size & (CODE_PAGE_SIZE - 1)) != 0) &&
        ((s_dfu_settings.progress.firmware_image_offset_last + p_req->object_size) != m_firmware_size_req) )
    {
        NRF_LOG_WARNING("%s(): object size must be page aligned", __func__);
        p_callback(p_context, NRF_DFU_RES_CODE_INVALID_PARAMETER, NULL);
        return NRF_SUCCESS;
    }

    if ((s_dfu_settings.progress.firmware_image_offset_last + p_req->object_size) > m_firmware_size_req)
    {
        NRF_LOG_WARNING("%s(): object goes beyond image (object_size is %u, offset is 0x%08x, firmware size is 0x%08x)",
                        __func__,
                        p_req->object_size,
                        s_dfu_settings.progress.firmware_image_offset_last,
                        m_firmware_size_req);
        p_callback(p_context, NRF_DFU_RES_CODE_OPERATION_NOT_PERMITTED, NULL);
        return NRF_SUCCESS;
    }

    NRF_LOG_INFO("Valid Data Create");

    s_dfu_settings.progress.data_object_size      = p_req->object_size;
    s_dfu_settings.progress.firmware_image_crc    = s_dfu_settings.progress.firmware_image_crc_last;
    s_dfu_settings.progress.firmware_image_offset = s_dfu_settings.progress.firmware_image_offset_last;

    s_dfu_settings.write_offset = m_firmware_start_addr + s_dfu_settings.progress.firmware_image_offset;

    m_data_buf_pos = 0;

    // Assert that modified bank was invalidated.
    if (s_dfu_settings.bank_current == NRF_DFU_CURRENT_BANK_0)
    {
        ASSERT(s_dfu_settings.bank_0.bank_code == NRF_DFU_BANK_INVALID);
    }
    else if (s_dfu_settings.bank_current == NRF_DFU_CURRENT_BANK_1)
    {
        ASSERT(s_dfu_settings.bank_1.bank_code == NRF_DFU_BANK_INVALID);
    }

    // Erase the FLASH region where object will be written.
    ret_code_t rcode = nrf_dfu_flash_erase(m_firmware_start_addr + s_dfu_settings.progress.firmware_image_offset,
                                           CEIL_DIV(p_req->object_size, CODE_PAGE_SIZE),
                                           NULL);
    if (rcode != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("%s(): erase operation failed", __func__);
    }
    else
    {
        NRF_LOG_INFO("Creating object with size: %d. Offset: 0x%08x, CRC: 0x%08x",
                     s_dfu_settings.progress.data_object_size,
                     s_dfu_settings.progress.firmware_image_offset,
                     s_dfu_settings.progress.firmware_image_crc);
        p_callback(p_context, NRF_DFU_RES_CODE_SUCCESS, NULL);
    }

    return rcode;
}


static ret_code_t nrf_dfu_data_req_write(void                 const * p_context,
                                         nrf_dfu_req_t        const * p_req,
                                         nrf_dfu_req_callback         p_callback)
{
    size_t const object_size_current = s_dfu_settings.progress.firmware_image_offset -
                                       s_dfu_settings.progress.firmware_image_offset_last;
    bool   const write_buffer_available = (m_read_buffer_id != ((m_write_buffer_id + 1) % FLASH_BUFFER_COUNT));
    bool   const object_completed = (p_req->req_len + object_size_current == s_dfu_settings.progress.data_object_size);

    nrf_dfu_res_t dfu_res;

    memset(&dfu_res, 0, sizeof(dfu_res));

    dfu_res.offset = s_dfu_settings.progress.firmware_image_offset;
    dfu_res.crc    = s_dfu_settings.progress.firmware_image_crc;

    // Validate request

    if (p_req->req_len > FLASH_BUFFER_LENGTH)
    {
        NRF_LOG_WARNING("%s(): to long request", __func__);
        p_callback(p_context, NRF_DFU_RES_CODE_INSUFFICIENT_RESOURCES, &dfu_res);
        return NRF_SUCCESS;
    }

    if (p_req->req_len + object_size_current > s_dfu_settings.progress.data_object_size)
    {
        NRF_LOG_WARNING("%s(): request goes boyond the image", __func__);
        p_callback(p_context, NRF_DFU_RES_CODE_INVALID_PARAMETER, &dfu_res);
        return NRF_SUCCESS;
    }

    if (write_buffer_available == false)
    {
        if ((FLASH_BUFFER_LENGTH - m_data_buf_pos < p_req->req_len) ||
            (object_completed != false))
        {
            NRF_LOG_WARNING("%s(): no write buffer available", __func__);

            // Flash operation failed. Revert CRC and offset.
            s_dfu_settings.progress.data_object_size      = 0;
            s_dfu_settings.progress.firmware_image_crc    = s_dfu_settings.progress.firmware_image_crc_last;
            s_dfu_settings.progress.firmware_image_offset = s_dfu_settings.progress.firmware_image_offset_last;

            s_dfu_settings.write_offset = m_firmware_start_addr + s_dfu_settings.progress.firmware_image_offset;

            // Update the return values
            dfu_res.crc    = s_dfu_settings.progress.firmware_image_crc;
            dfu_res.offset = s_dfu_settings.progress.firmware_image_offset;

            p_callback(p_context, NRF_DFU_RES_CODE_OPERATION_FAILED, &dfu_res);

            return NRF_SUCCESS;
        }
    }

    // Copy data


    // Update the CRC of the firmware image.
    s_dfu_settings.progress.firmware_image_crc     = crc32_compute(p_req->p_req, p_req->req_len, &s_dfu_settings.progress.firmware_image_crc);
    s_dfu_settings.progress.firmware_image_offset += p_req->req_len;

    // Update the return values
    dfu_res.offset = s_dfu_settings.progress.firmware_image_offset;
    dfu_res.crc    = s_dfu_settings.progress.firmware_image_crc;


    ret_code_t rcode = NRF_SUCCESS;
    size_t     copy_pos = 0;

    while ((copy_pos < p_req->req_len) && (rcode == NRF_SUCCESS))
    {
        size_t copy_len = p_req->req_len - copy_pos;
        if (m_data_buf_pos + copy_len > FLASH_BUFFER_LENGTH)
        {
            copy_len = FLASH_BUFFER_LENGTH - m_data_buf_pos;
        }

        memcpy(&m_data_buf[m_write_buffer_id][m_data_buf_pos], &p_req->p_req[copy_pos], copy_len);

        m_data_buf_pos += copy_len;
        copy_pos       += copy_len;

        if ((m_data_buf_pos == FLASH_BUFFER_LENGTH) ||
            ((object_completed != false) && (copy_pos == p_req->req_len)))
        {
            rcode = nrf_dfu_flash_store(s_dfu_settings.write_offset,
                                        &m_data_buf[m_write_buffer_id][0],
                                        m_data_buf_pos,
                                        nrf_dfu_flash_store_completed);
            if (rcode == NRF_SUCCESS)
            {
                NRF_LOG_INFO("Storing %d bytes at: 0x%08x", m_data_buf_pos, s_dfu_settings.write_offset);

                m_write_buffer_id = (m_write_buffer_id + 1) % FLASH_BUFFER_COUNT;

                s_dfu_settings.write_offset += m_data_buf_pos;
            }
            else
            {
                NRF_LOG_ERROR("%s(): failed storing %d B at address: 0x%08x",
                              __func__, m_data_buf_pos, s_dfu_settings.write_offset);

                // Flash operation failed. Revert CRC and offset.
                s_dfu_settings.progress.data_object_size      = 0;
                s_dfu_settings.progress.firmware_image_crc    = s_dfu_settings.progress.firmware_image_crc_last;
                s_dfu_settings.progress.firmware_image_offset = s_dfu_settings.progress.firmware_image_offset_last;

                s_dfu_settings.write_offset = m_firmware_start_addr + s_dfu_settings.progress.firmware_image_offset;

                // Update the return values
                dfu_res.crc    = s_dfu_settings.progress.firmware_image_crc;
                dfu_res.offset = s_dfu_settings.progress.firmware_image_offset;
            }

            m_data_buf_pos = 0;
        }
    }

    if (NRF_SUCCESS == rcode)
    {
        p_callback(p_context, NRF_DFU_RES_CODE_SUCCESS, &dfu_res);
    }

    return rcode;
}


static ret_code_t nrf_dfu_data_req_execute(void                const * p_context,
                                           nrf_dfu_req_t       const * p_req,
                                           nrf_dfu_req_callback        p_callback)
{
    // Validate state

    if (s_dfu_settings.progress.data_object_size !=
        s_dfu_settings.progress.firmware_image_offset -
        s_dfu_settings.progress.firmware_image_offset_last)
    {
        NRF_LOG_WARNING("%s:() object is incomplete, expected %d, got %d",
                        __func__,
                        s_dfu_settings.progress.data_object_size,
                        s_dfu_settings.progress.firmware_image_offset - s_dfu_settings.progress.firmware_image_offset_last);
        p_callback(p_context, NRF_DFU_RES_CODE_OPERATION_NOT_PERMITTED, NULL);
        return NRF_SUCCESS;
    }

    NRF_LOG_INFO("Valid Data Execute");

    s_dfu_settings.progress.data_object_size           = 0;
    s_dfu_settings.progress.firmware_image_offset_last = s_dfu_settings.progress.firmware_image_offset;
    s_dfu_settings.progress.firmware_image_crc_last    = s_dfu_settings.progress.firmware_image_crc;

    ret_code_t rcode = nrf_dfu_settings_write();
    if (rcode != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("%s(): cannot store settings", __func__);
        return rcode;
    }

    if (s_dfu_settings.progress.firmware_image_offset == m_firmware_size_req)
    {
        // Postpone postvalidate until the whole image is written to FLASH.
        // Note that as both FLASH and BLE calls are performed from the same
        // context (irq level) we don't need to use critical section.
        if (m_read_buffer_id == m_write_buffer_id)
        {
            nrf_dfu_res_code_t ret_code = nrf_dfu_postvalidate(&dfu_init_packet.signed_command.command.init);
            p_callback(p_context, ret_code, NULL);
        }
        else
        {
            m_objectexecute_callback_function     = p_callback;
            m_objectexecute_callback_context      = p_context;
            m_objectexecute_callback_postvalidate = true;
        }
    }
    else
    {
        // Postpone response until there is enough space to hold entire object.
        size_t used_buffers = (FLASH_BUFFER_COUNT + m_write_buffer_id - m_read_buffer_id) % FLASH_BUFFER_COUNT;
        size_t available_buffers = FLASH_BUFFER_COUNT - used_buffers;

        if (available_buffers * FLASH_BUFFER_LENGTH >= DATA_OBJECT_MAX_SIZE)
        {
            p_callback(p_context, NRF_DFU_RES_CODE_SUCCESS, NULL);
        }
        else
        {
            m_objectexecute_callback_function     = p_callback;
            m_objectexecute_callback_context      = p_context;
            m_objectexecute_callback_postvalidate = false;
        }
    }

    return rcode;
}


static uint32_t nrf_dfu_data_req_crc(void                 const * p_context,
                                     nrf_dfu_req_t        const * p_req,
                                     nrf_dfu_req_callback         p_callback)
{
    nrf_dfu_res_t dfu_res;

    memset(&dfu_res, 0, sizeof(dfu_res));

    dfu_res.crc    = s_dfu_settings.progress.firmware_image_crc;
    dfu_res.offset = s_dfu_settings.progress.firmware_image_offset;

    p_callback(p_context, NRF_DFU_RES_CODE_SUCCESS, &dfu_res);

    return NRF_SUCCESS;
}


static uint32_t nrf_dfu_data_req_select(void                 const * p_context,
                                        nrf_dfu_req_t        const * p_req,
                                        nrf_dfu_req_callback         p_callback)
{
    nrf_dfu_res_t dfu_res;

    memset(&dfu_res, 0, sizeof(dfu_res));

    dfu_res.crc      = s_dfu_settings.progress.firmware_image_crc;
    dfu_res.offset   = s_dfu_settings.progress.firmware_image_offset;
    dfu_res.max_size = DATA_OBJECT_MAX_SIZE;

    p_callback(p_context, NRF_DFU_RES_CODE_SUCCESS, &dfu_res);

    return NRF_SUCCESS;
}

static ret_code_t nrf_dfu_data_req(void                 const * p_context,
                                   nrf_dfu_req_t        const * p_req,
                                   nrf_dfu_req_callback         p_callback)
{
    NRF_LOG_INFO("%s(): op_code is %u", __func__, p_req->req_type);

    if (m_valid_init_packet_present == false)
    {
        NRF_LOG_WARNING("%s(): no valid init command", __func__);
        p_callback(p_context, NRF_DFU_RES_CODE_OPERATION_NOT_PERMITTED, NULL);
        return NRF_SUCCESS;
    }

    ret_code_t rcode;
    switch (p_req->req_type)
    {
        case NRF_DFU_OBJECT_OP_CREATE:
            rcode = nrf_dfu_data_req_create(p_context, p_req, p_callback);
            break;

        case NRF_DFU_OBJECT_OP_WRITE:
            rcode = nrf_dfu_data_req_write(p_context, p_req, p_callback);
            break;

        case NRF_DFU_OBJECT_OP_CRC:
            rcode = nrf_dfu_data_req_crc(p_context, p_req, p_callback);
            break;

        case NRF_DFU_OBJECT_OP_EXECUTE:
            rcode = nrf_dfu_data_req_execute(p_context, p_req, p_callback);
            break;

        case NRF_DFU_OBJECT_OP_SELECT:
            rcode = nrf_dfu_data_req_select(p_context, p_req, p_callback);
            break;

        default:
            NRF_LOG_WARNING("%s(): invalid operation", __func__);
            p_callback(p_context, NRF_DFU_RES_CODE_OP_CODE_NOT_SUPPORTED, NULL);
            rcode = NRF_SUCCESS;
            break;
    }

    return rcode;
}


ret_code_t nrf_dfu_req_handler_init(void)
{
#ifdef BLE_STACK_SUPPORT_REQD
    bool sd_irq_initialized = true;
#else
    bool sd_irq_initialized = false;
#endif
    uint32_t ret_val = nrf_dfu_flash_init(sd_irq_initialized);
    VERIFY_SUCCESS(ret_val);

    // If the command is stored to flash, init command was valid.
    pb_istream_t stream;
    if ((s_dfu_settings.progress.command_size != 0) && dfu_decode_commmand(&stream))
    {
        // Get the previously stored firmware size
        if ((s_dfu_settings.bank_0.bank_code == NRF_DFU_BANK_INVALID) &&
            (s_dfu_settings.bank_0.image_size != 0))
        {
            m_firmware_size_req = s_dfu_settings.bank_0.image_size;
        }
#if defined(NRF_DFU_DUAL_BANK_SUPPORT) && (NRF_DFU_DUAL_BANK_SUPPORT == 1)
        else if ((s_dfu_settings.bank_1.bank_code == NRF_DFU_BANK_INVALID) &&
                 (s_dfu_settings.bank_1.image_size != 0))
        {
            m_firmware_size_req = s_dfu_settings.bank_1.image_size;
        }
#endif
        else
        {
            return NRF_SUCCESS;
        }

        // Location should still be valid, expecting result of find-cache to be true
        (void)nrf_dfu_find_cache(m_firmware_size_req, &m_firmware_start_addr);

        // Setting valid init command to true to
        m_valid_init_packet_present = true;
    }

    // Initialize extended error handling with "No error" as the most recent error.
    (void) ext_error_set(NRF_DFU_EXT_ERROR_NO_ERROR);

    return NRF_SUCCESS;
}


ret_code_t nrf_dfu_req_handler_on_req(void                 const * p_context,
                                      nrf_dfu_req_t        const * p_req,
                                      nrf_dfu_req_callback         p_callback)
{
    static nrf_dfu_obj_type_t cur_obj_type = NRF_DFU_OBJ_TYPE_COMMAND;

    ASSERT(p_req != NULL);
    ASSERT(p_callback != NULL);

    // Reject requests until postvalidation is performed
    if (m_objectexecute_callback_function != NULL)
    {
        NRF_LOG_WARNING("%s(): postvalidate is pending", __func__);
        p_callback(p_context, NRF_DFU_RES_CODE_OPERATION_NOT_PERMITTED, NULL);
        return NRF_SUCCESS;
    }

    // Select right object type if needed
    switch (p_req->req_type)
    {
        case NRF_DFU_OBJECT_OP_CREATE:
        case NRF_DFU_OBJECT_OP_SELECT:

            // Validate object type
            switch((nrf_dfu_obj_type_t)p_req->obj_type)
            {
                case NRF_DFU_OBJ_TYPE_COMMAND:
                case NRF_DFU_OBJ_TYPE_DATA:
                    break;
                default:
                    NRF_LOG_ERROR("%s(): Invald object type", __func__);
                    p_callback(p_context, NRF_DFU_RES_CODE_INVALID_OBJECT, NULL);
                    return NRF_SUCCESS;
            }

            cur_obj_type = (nrf_dfu_obj_type_t)p_req->obj_type;
            break;

        default:
            break;
    }

    // Restart inactivity timer if needed
    switch (p_req->req_type)
    {
        case NRF_DFU_OBJECT_OP_CREATE:
        case NRF_DFU_OBJECT_OP_EXECUTE:
            inactivity_timer_restart();
            break;
        default:
            break;
    }

    // Call object specific function
    ret_code_t rcode;
    switch (cur_obj_type)
    {
        case NRF_DFU_OBJ_TYPE_COMMAND:
            rcode = nrf_dfu_command_req(p_context, p_req, p_callback);
            break;

        case NRF_DFU_OBJ_TYPE_DATA:
            rcode = nrf_dfu_data_req(p_context, p_req, p_callback);
            break;

        default:
            // Should not happen
            ASSERT(false);
            rcode = NRF_SUCCESS;
            break;
    }

    if (rcode != NRF_SUCCESS)
    {
        p_callback(p_context, NRF_DFU_RES_CODE_OPERATION_FAILED, NULL);
    }

    return NRF_SUCCESS;
}

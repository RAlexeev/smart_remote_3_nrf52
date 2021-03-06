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

#include <stddef.h>
#include <string.h>

#include "app_error.h"
#include "nrf_dfu_flash.h"
#include "nrf_soc.h"
#include "crc32.h"
#include "nrf_nvmc.h"

#include "nrf_dfu_settings.h"

#define NRF_LOG_MODULE_NAME nrf_dfu_settings
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

/**@brief Offset in the settings struct where the InitCommand is located. */
#define DFU_SETTINGS_INIT_COMMAND_OFFSET        offsetof(nrf_dfu_settings_t, init_command)

/**@brief Number of temporary data buffers used to store DFU settings.
 *
 * To avoid the situation when we override the buffer before pending operation
 * completes and it is actually stored, we use array of temporary buffers.
 * At the moment of writing fstorage can queue 44 transactions at a time
 * which should allow to store a bit more then 1 data object. Just in case
 * 16 temporary buffers are used here.
 */
#define DFU_SETTINGS_FLASH_BUFFER_COUNT 16


/**@brief   This variable reserves a page in flash for bootloader settings
 *          to ensure the linker doesn't place any code or variables at this location.
 */
#if defined (__CC_ARM )

    uint8_t m_dfu_settings_buffer[CODE_PAGE_SIZE]
        __attribute__((at(BOOTLOADER_SETTINGS_ADDRESS)))
        __attribute__((used));

#elif defined ( __GNUC__ ) || defined ( __SES_ARM )

    uint8_t m_dfu_settings_buffer[CODE_PAGE_SIZE]
        __attribute__((section(".bootloader_settings_page")))
        __attribute__((used));

#elif defined ( __ICCARM__ )

    __no_init __root uint8_t m_dfu_settings_buffer[CODE_PAGE_SIZE]
        @ BOOTLOADER_SETTINGS_ADDRESS;

#else

    #error Not a valid compiler/linker for m_dfu_settings placement.

#endif // Compiler specific

#if !defined(NRF_DFU_UPDATABLE_APPLICATION_ONLY) || (NRF_DFU_UPDATABLE_APPLICATION_ONLY == 0)
#ifndef BL_SETTINGS_ACCESS_ONLY
#if defined(NRF52_SERIES)

/**@brief   This variable reserves a page in flash for MBR parameters
 *          to ensure the linker doesn't place any code or variables at this location.
 */
#if defined ( __CC_ARM )

    uint8_t m_mbr_params_page[CODE_PAGE_SIZE]
        __attribute__((at(NRF_MBR_PARAMS_PAGE_ADDRESS)))
        __attribute__((used));

#elif defined ( __GNUC__ ) || defined ( __SES_ARM )

    uint8_t m_mbr_params_page[CODE_PAGE_SIZE]
        __attribute__((section(".mbr_params_page")))
        __attribute__((used));

#elif defined ( __ICCARM__ )

    __no_init uint8_t m_mbr_params_page[CODE_PAGE_SIZE]
        @ NRF_MBR_PARAMS_PAGE_ADDRESS;

#else

    #error Not a valid compiler/linker for m_mbr_params_page placement.

#endif // Compiler specific


/**@brief   This variable has the linker write the MBR parameters page address to the
 *          UICR register. This value will be written in the HEX file and thus to the
 *          UICR when the bootloader is flashed into the chip.
 */
#if defined ( __CC_ARM )

    uint32_t const m_uicr_mbr_params_page_address
        __attribute__((at(NRF_UICR_MBR_PARAMS_PAGE_ADDRESS)))
        __attribute__((used)) = NRF_MBR_PARAMS_PAGE_ADDRESS;

#elif defined ( __GNUC__ ) || defined ( __SES_ARM )

    uint32_t const m_uicr_mbr_params_page_address
        __attribute__((section(".uicr_mbr_params_page")))
        __attribute__((used)) = NRF_MBR_PARAMS_PAGE_ADDRESS;

#elif defined ( __ICCARM__ )

    __root uint32_t const m_uicr_mbr_params_page_address
        @ NRF_UICR_MBR_PARAMS_PAGE_ADDRESS = NRF_MBR_PARAMS_PAGE_ADDRESS;

#else

    #error Not a valid compiler/linker for m_mbr_params_page placement.

#endif // Compiler specific
#endif // #if defined( NRF52_SERIES )
#endif // #ifndef BL_SETTINGS_ACCESS_ONLY
#endif /* !defined(NRF_DFU_UPDATABLE_APPLICATION_ONLY) || (NRF_DFU_UPDATABLE_APPLICATION_ONLY == 0) */

STATIC_ASSERT(IS_POWER_OF_TWO(DFU_SETTINGS_FLASH_BUFFER_COUNT));

/** @brief Structure holding information about DFU settings write buffers. */
typedef struct
{
    nrf_dfu_settings_t buffer[DFU_SETTINGS_FLASH_BUFFER_COUNT];  /**< Buffer used for store operation. */
    size_t             write_id;                                 /**< Index of the currently written data buffer. */
    size_t             read_id;                                  /**< Index of the last read data buffer. */
} nrf_dfu_settings_store_buffers;

static nrf_dfu_settings_store_buffers m_store_buffers;

nrf_dfu_settings_t s_dfu_settings;


static size_t nrf_dfu_settings_buffer_index_inc(size_t id)
{
    // Buffer count is forced to be a power of two.
    return (id + 1) & (ARRAY_SIZE(m_store_buffers.buffer) - 1);
}


static void nrf_dfu_settings_store_completed(nrf_fstorage_evt_t * p_evt)
{
    m_store_buffers.read_id = nrf_dfu_settings_buffer_index_inc(m_store_buffers.read_id);
}


static uint32_t nrf_dfu_settings_crc_get(void)
{
    // The crc is calculated from the s_dfu_settings struct, except the crc itself and the init command
    return crc32_compute((uint8_t *)&s_dfu_settings + sizeof(s_dfu_settings.crc),
                         DFU_SETTINGS_INIT_COMMAND_OFFSET - sizeof(s_dfu_settings.crc),
                         NULL);
}


void nrf_dfu_settings_init(void)
{
    NRF_LOG_DEBUG("Enter %s", __func__);

    ret_code_t rc = nrf_dfu_flash_init(false);
    if (rc != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("%s() failed with error: %x", __func__, rc);
        APP_ERROR_HANDLER(rc);
    }

    // Copy the DFU settings out of flash and into a buffer in RAM.
    memcpy(&s_dfu_settings, m_dfu_settings_buffer, sizeof(nrf_dfu_settings_t));

    if (s_dfu_settings.settings_version == NRF_DFU_SETTINGS_VERSION)
    {
        if (s_dfu_settings.crc != 0xFFFFFFFF)
        {
            uint32_t crc = nrf_dfu_settings_crc_get();

            if (crc == s_dfu_settings.crc)
            {
                // Content is validated - return from function.
                return;
            }
            else
            {
                NRF_LOG_WARNING("%s(): settings CRC does not match", __func__);
            }
        }
    }
    else
    {
        NRF_LOG_WARNING("%s(): incorrect version of settings", __func__);
    }

    // Reached if the page is erased or CRC is wrong.
    NRF_LOG_DEBUG("Resetting bootloader settings.");

    memset(&s_dfu_settings,             0x00, sizeof(s_dfu_settings));
    memset(&s_dfu_settings.shared_data, 0xFF, sizeof(s_dfu_settings.shared_data));
    s_dfu_settings.settings_version = NRF_DFU_SETTINGS_VERSION;

    rc = nrf_dfu_settings_write();
    if (rc != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("%s(): settings cannot be stored, error=%x", __func__, rc);
        APP_ERROR_HANDLER(rc);
    }
}


ret_code_t nrf_dfu_settings_write(void)
{
    ret_code_t err_code;
    NRF_LOG_DEBUG("Writing settings...");
    NRF_LOG_DEBUG("Erasing old settings at: 0x%08x", (uint32_t)m_dfu_settings_buffer);

    // Only report completion on successful STORE.
    err_code = nrf_dfu_flash_erase((uint32_t)m_dfu_settings_buffer, 1, NULL);

    if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("Could not erase the settings page!");
        return NRF_ERROR_INTERNAL;
    }

    s_dfu_settings.crc = nrf_dfu_settings_crc_get();

    if (nrf_dfu_settings_buffer_index_inc(m_store_buffers.write_id) == m_store_buffers.read_id)
    {
        NRF_LOG_ERROR("Cannot schedule settings store - no buffer available!");
        return NRF_ERROR_INTERNAL;
    }

    memcpy(&m_store_buffers.buffer[m_store_buffers.write_id], &s_dfu_settings, sizeof(nrf_dfu_settings_t));

    err_code = nrf_dfu_flash_store((uint32_t)m_dfu_settings_buffer,
                                   &m_store_buffers.buffer[m_store_buffers.write_id],
                                   sizeof(nrf_dfu_settings_t),
                                   nrf_dfu_settings_store_completed);
    if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("Could not write the DFU settings page!");
        return NRF_ERROR_INTERNAL;
    }

    m_store_buffers.write_id = nrf_dfu_settings_buffer_index_inc(m_store_buffers.write_id);

    return NRF_SUCCESS;
}

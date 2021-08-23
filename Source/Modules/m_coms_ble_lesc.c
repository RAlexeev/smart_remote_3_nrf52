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

#include <stdbool.h>

#include "nrf_atomic.h"
#include "nrf_crypto.h"
#include "nrf_crypto_keys.h"
#include "nrf_drv_rng.h"
#include "nrf_sdh_ble.h"
#include "app_debug.h"
#include "app_isched.h"
#include "app_util.h"

#include "m_coms_ble_lesc.h"
#include "resources.h"
#include "rng_monitor.h"
#include "sr3_config.h"

#if CONFIG_SEC_LESC_PAIRING

#define NRF_LOG_MODULE_NAME m_coms_ble_lesc
#define NRF_LOG_LEVEL CONFIG_BLE_LESC_LOG_LEVEL
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

// Verify SDK configuration.
STATIC_ASSERT(RNG_ENABLED);
STATIC_ASSERT(RNG_CONFIG_POOL_SIZE >= NRF_CRYPTO_ECC_PRIVATE_KEY_SIZE_SECP256R1);
STATIC_ASSERT(BLE_GAP_LESC_P256_PK_LEN == NRF_CRYPTO_ECC_PUBLIC_KEY_SIZE_SECP256R1);

__ALIGN(4) static ble_gap_lesc_p256_pk_t m_coms_ble_lesc_public_key;      /**< LESC ECC Public Key. */
__ALIGN(4) static ble_gap_lesc_dhkey_t   m_coms_ble_lesc_dh_key;          /**< LESC ECC DH Key. */

/**@brief Allocated private key type to use for LESC DH generation.
 */
NRF_CRYPTO_ECC_PRIVATE_KEY_CREATE(m_private_key, SECP256R1);

/**@brief Allocated public key type to use for LESC DH generation.
 */
NRF_CRYPTO_ECC_PUBLIC_KEY_CREATE(m_public_key, SECP256R1);

/**@brief Allocated peer public key type to use for LESC DH generation.
 */
NRF_CRYPTO_ECC_PUBLIC_KEY_CREATE(m_peer_public_key, SECP256R1);

/**@brief Allocated raw public key to use for LESC DH.
 */
NRF_CRYPTO_ECC_PUBLIC_KEY_RAW_CREATE_FROM_ARRAY(m_public_key_raw, SECP256R1, m_coms_ble_lesc_public_key.pk);

/**@brief Allocated shared instance to use for LESC DH.
 */
NRF_CRYPTO_ECDH_SHARED_SECRET_CREATE_FROM_ARRAY(m_dh_key, SECP256R1, m_coms_ble_lesc_dh_key.key);

static bool                              m_coms_ble_lesc_keys_valid;    /**< True if keys are valid. */
static nrf_atomic_flag_t                 m_coms_ble_lesc_busy;          /**< Asserted if module is performing computations. */

static ble_gap_lesc_oob_data_t           m_coms_ble_lesc_oob_data;      /**< LESC OOB Data. */
static bool                              m_coms_ble_lesc_oob_data_valid;/**< True if OOB Data is valid. */

/**@brief Notify upper layers about LESC keys */
static void m_coms_ble_lesc_notify(void *p_context)
{
    m_coms_ble_lesc_key_event_handler_t evt_handler = (m_coms_ble_lesc_key_event_handler_t)(p_context);

    // Mark keys as valid.
    m_coms_ble_lesc_keys_valid = true;

    // Clear busy status.
    nrf_atomic_flag_clear(&m_coms_ble_lesc_busy);

    // Call event handler if it was requested.
    if (evt_handler)
    {
        evt_handler(&m_coms_ble_lesc_public_key);
    }
}

/**@brief Calculate LESC Keys */
static void m_coms_ble_lesc_calc_keys(void *p_context)
{
    // Verify our assumptions about module state.
    ASSERT(m_coms_ble_lesc_busy && (m_coms_ble_lesc_keys_valid == false));

    // Calculate LESC Keys.
    NRF_LOG_DEBUG("Starting SK/PK keys generation...");
    APP_ERROR_CHECK(nrf_crypto_ecc_key_pair_generate(NRF_CRYPTO_BLE_ECDH_CURVE_INFO, &m_private_key, &m_public_key));
    APP_ERROR_CHECK(nrf_crypto_ecc_public_key_to_raw(NRF_CRYPTO_BLE_ECDH_CURVE_INFO, &m_public_key, &m_public_key_raw));
    NRF_LOG_DEBUG("SK/PK keys ready.");

    NRF_LOG_DEBUG("SK[1/2]: %08X%08X%08X%08X",
               uint32_big_decode(&m_private_key.p_value[0x00]),
               uint32_big_decode(&m_private_key.p_value[0x04]),
               uint32_big_decode(&m_private_key.p_value[0x08]),
               uint32_big_decode(&m_private_key.p_value[0x0C]));
    NRF_LOG_DEBUG("SK[2/2]: %08X%08X%08X%08X",
               uint32_big_decode(&m_private_key.p_value[0x10]),
               uint32_big_decode(&m_private_key.p_value[0x14]),
               uint32_big_decode(&m_private_key.p_value[0x18]),
               uint32_big_decode(&m_private_key.p_value[0x1C]));

    NRF_LOG_DEBUG("PK[1/4]: %08X%08X%08X%08X",
               uint32_big_decode(&m_public_key.p_value[0x00]),
               uint32_big_decode(&m_public_key.p_value[0x04]),
               uint32_big_decode(&m_public_key.p_value[0x08]),
               uint32_big_decode(&m_public_key.p_value[0x0C]));
    NRF_LOG_DEBUG("PK[2/4]: %08X%08X%08X%08X",
               uint32_big_decode(&m_public_key.p_value[0x10]),
               uint32_big_decode(&m_public_key.p_value[0x14]),
               uint32_big_decode(&m_public_key.p_value[0x18]),
               uint32_big_decode(&m_public_key.p_value[0x1C]));
    NRF_LOG_DEBUG("PK[3/4]: %08X%08X%08X%08X",
               uint32_big_decode(&m_public_key.p_value[0x20]),
               uint32_big_decode(&m_public_key.p_value[0x24]),
               uint32_big_decode(&m_public_key.p_value[0x28]),
               uint32_big_decode(&m_public_key.p_value[0x2C]));
    NRF_LOG_DEBUG("PK[4/4]: %08X%08X%08X%08X",
               uint32_big_decode(&m_public_key.p_value[0x30]),
               uint32_big_decode(&m_public_key.p_value[0x34]),
               uint32_big_decode(&m_public_key.p_value[0x38]),
               uint32_big_decode(&m_public_key.p_value[0x3C]));

    // Get out of background scheduler context to update module state and notify upper layers.
    APP_ERROR_CHECK(app_isched_event_put(&g_fg_scheduler, m_coms_ble_lesc_notify, p_context));
}

/**@brief Calculate LESC DH Key. */
static void m_coms_ble_lesc_calc_dh_key(void *p_context)
{
    const uint32_t conn_handle = (uint32_t) p_context;

    // Verify our assumptions about module state.
    ASSERT(m_coms_ble_lesc_busy && (m_coms_ble_lesc_keys_valid == true));
    ASSERT(conn_handle != BLE_CONN_HANDLE_INVALID);


    // Calulate DH Key.
    NRF_LOG_DEBUG("Starting DH Key generation...");

    APP_ERROR_CHECK(nrf_crypto_ecdh_shared_secret_compute(NRF_CRYPTO_BLE_ECDH_CURVE_INFO,
                                                          &m_private_key,
                                                          &m_peer_public_key,
                                                          &m_dh_key));
    NRF_LOG_DEBUG("DH key ready.");

    NRF_LOG_DEBUG("DH[1/2]: %08X%08X%08X%08X",
               uint32_big_decode(&m_dh_key.p_value[0x00]),
               uint32_big_decode(&m_dh_key.p_value[0x04]),
               uint32_big_decode(&m_dh_key.p_value[0x08]),
               uint32_big_decode(&m_dh_key.p_value[0x0C]));
    NRF_LOG_DEBUG("DH[2/2]: %08X%08X%08X%08X",
               uint32_big_decode(&m_dh_key.p_value[0x10]),
               uint32_big_decode(&m_dh_key.p_value[0x14]),
               uint32_big_decode(&m_dh_key.p_value[0x18]),
               uint32_big_decode(&m_dh_key.p_value[0x1C]));

    // Clear busy status.
    nrf_atomic_flag_clear(&m_coms_ble_lesc_busy);

    // Pass DH Key to SD.
    APP_ERROR_CHECK(sd_ble_gap_lesc_dhkey_reply((uint16_t)conn_handle, &m_coms_ble_lesc_dh_key));
}

/**@brief Common procedure for LESC Keys and shared secret generation. */
static void m_coms_ble_lesc_gen_data(void *p_context)
{
    // Make sure that we are called only when the module is locked.
    ASSERT(m_coms_ble_lesc_busy);

    if (m_coms_ble_lesc_keys_valid == false)
    {
        /*
         * Put keys computation in background.
         * (Context holds event handler which should be called after keys generation)
         */
        APP_ERROR_CHECK(app_isched_event_put(&g_bg_scheduler, m_coms_ble_lesc_calc_keys, p_context));
    }
    else
    {
        /*
         * Put DH computation in background.
         * (Context holds pointer to the BLE event)
         */
        APP_ERROR_CHECK(app_isched_event_put(&g_bg_scheduler, m_coms_ble_lesc_calc_dh_key, p_context));
    }
}

ret_code_t m_coms_ble_lesc_generate_key(m_coms_ble_lesc_key_event_handler_t key_evt_handler)
{
    ret_code_t status;

    // Check if key event handler has been specified.
    if (key_evt_handler == NULL)
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    if (nrf_atomic_flag_set_fetch(&m_coms_ble_lesc_busy))
    {
        return NRF_ERROR_BUSY;
    }

    // Mark existing keys and OOB data as not valid;
    m_coms_ble_lesc_keys_valid       = false;
    m_coms_ble_lesc_oob_data_valid   = false;

    // Request LESC data generation.
    status = rng_monitor_request(NRF_CRYPTO_ECC_PRIVATE_KEY_SIZE_SECP256R1,
                                 m_coms_ble_lesc_gen_data,
                                 (void *)(key_evt_handler));
    if (status != NRF_SUCCESS)
    {
        nrf_atomic_flag_clear(&m_coms_ble_lesc_busy);
    }

    return status;
}

ret_code_t m_coms_ble_lesc_generate_oob_data(ble_gap_lesc_oob_data_t *p_oob_data)
{
    ret_code_t status;

    if (m_coms_ble_lesc_keys_valid)
    {
        // Get OOB data from the SoftDevice.
        status = sd_ble_gap_lesc_oob_data_get(BLE_CONN_HANDLE_INVALID,
                                              &m_coms_ble_lesc_public_key,
                                              &m_coms_ble_lesc_oob_data);
    }
    else
    {
        // Return error if key has not been generated yet.
        status = NRF_ERROR_INVALID_STATE;
    }

    if (status == NRF_SUCCESS)
    {
        // Mark OOB data as valid.
        m_coms_ble_lesc_oob_data_valid = true;

        // Copy OOB data to user-specifed buffer if it was requested.
        if (p_oob_data != NULL)
        {
            *p_oob_data = m_coms_ble_lesc_oob_data;
        }
    }

    return status;
}

ret_code_t m_coms_ble_lesc_get_oob_data(ble_gap_lesc_oob_data_t *p_oob_data)
{
    if (p_oob_data == NULL)
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    if (m_coms_ble_lesc_oob_data_valid == false)
    {
        return NRF_ERROR_INVALID_STATE;
    }

    *p_oob_data = m_coms_ble_lesc_oob_data;

    return NRF_SUCCESS;
}

ret_code_t m_coms_ble_lesc_init(void)
{
    ASSERT(m_private_key.length == NRF_CRYPTO_ECC_PRIVATE_KEY_SIZE_SECP256R1);
    ASSERT(m_public_key.length == NRF_CRYPTO_ECC_PUBLIC_KEY_SIZE_SECP256R1);

    nrf_atomic_flag_clear(&m_coms_ble_lesc_busy);
    m_coms_ble_lesc_keys_valid      = false;
    m_coms_ble_lesc_oob_data_valid  = false;

    return NRF_SUCCESS;
}

static void m_coms_ble_lesc_on_ble_evt(ble_evt_t const *p_ble_evt, void *p_context)
{
    bool busy;
    uint32_t conn_handle;

    ASSERT(p_ble_evt != NULL);

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_LESC_DHKEY_REQUEST:
            NRF_LOG_DEBUG("BLE_GAP_EVT_LESC_DHKEY_REQUEST");

            // Try to perform OOB authentication if OOB data is avaiable.
            if (p_ble_evt->evt.gap_evt.params.lesc_dhkey_request.oobd_req && m_coms_ble_lesc_oob_data_valid)
            {
                APP_ERROR_CHECK(sd_ble_gap_lesc_oob_data_set(p_ble_evt->evt.gap_evt.conn_handle, &m_coms_ble_lesc_oob_data, NULL));
            }

            // Mark module as busy.
            busy = nrf_atomic_flag_set_fetch(&m_coms_ble_lesc_busy);

            // Make sure that we are in proper state.
            APP_ERROR_CHECK_BOOL(!busy && m_coms_ble_lesc_keys_valid);

            static nrf_value_length_t peer_public_key_raw;

            peer_public_key_raw.p_value = &p_ble_evt->evt.gap_evt.params.lesc_dhkey_request.p_pk_peer->pk[0];
            peer_public_key_raw.length = BLE_GAP_LESC_P256_PK_LEN;

            APP_ERROR_CHECK(nrf_crypto_ecc_public_key_from_raw(NRF_CRYPTO_BLE_ECDH_CURVE_INFO,
                                                               &peer_public_key_raw,
                                                               &m_peer_public_key));

            conn_handle = (uint32_t) p_ble_evt->evt.gap_evt.conn_handle;

            // Request LESC data generation.
            APP_ERROR_CHECK(rng_monitor_request(NRF_CRYPTO_ECC_PRIVATE_KEY_SIZE_SECP256R1,
                                                m_coms_ble_lesc_gen_data,
                                                (void *)(conn_handle)));
            break;

        default:
            /* Ignore */
            break;
    }
}

NRF_SDH_BLE_OBSERVER(m_coms_ble_lesc_observer, BLE_OBSERVER_PRIORITY_HIGH, m_coms_ble_lesc_on_ble_evt, NULL);

#endif /* CONFIG_SEC_LESC_PAIRING */

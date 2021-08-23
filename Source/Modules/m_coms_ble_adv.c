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

#include <string.h>

#include "ble_advdata.h"
#include "ble_hci.h"
#include "nrf_fstorage.h"
#include "peer_manager.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"

#include "m_coms_ble_addr.h"
#include "m_coms_ble_adv.h"
#include "m_coms_ble_lesc.h"
#include "app_debug.h"

#include "resources.h"
#include "sr3_config.h"

#define NRF_LOG_MODULE_NAME m_coms_ble_adv
#define NRF_LOG_LEVEL       CONFIG_BLE_ADV_LOG_LEVEL
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

/**@brief Advertising mode. */
typedef enum
{
    ADV_MODE_NONE,                  /**< No advertising running. */
    ADV_MODE_DIRECTED_SINGLE,       /**< Single directed advertising. */
    ADV_MODE_DIRECTED_MULTIPLE,     /**< Multiple directed advertisings (in sequence), one targeted at each bonded Central. */
    ADV_MODE_UNDIRECTED_BONDABLE,   /**< Undirected advertising. */
    ADV_MODE_UNDIRECTED_WHITELIST,  /**< Undirected advertising with whitelist (when directed cannot be used, not bondable). */
} m_coms_ble_adv_mode_t;

/**@brief Struct to keep track of advertising type and dynamic parameters. */
typedef struct
{
    m_coms_ble_adv_mode_t   mode;           /**< Advertising mode */
    bool                    bond_initiate;  /**< Indicates whether a new bond should be made. */
    bool                    adv_active;     /**< Indicates whether advertising is active. */
    uint8_t                 adv_count;      /**< Count to keep track of directed and multi-master advertising sessions.
                                                 In whitelist advertising mode, each session targets at different peer.
                                                 In direct advertising mode, there are DIRECTED_ADV_COUNT sessions targetted at the same peer. */
    uint8_t                 adv_type;       /**< See BLE_GAP_ADV_TYPES. */
    uint8_t                 adv_flags;      /**< See BLE_GAP_DISC_MODES. */
} m_coms_ble_adv_state_t;

// Check if manufacturer data can with within advertising packet
#if defined(CONFIG_ADV_MANUF_ENABLED) && CONFIG_ADV_MANUF_ENABLED == 1
STATIC_ASSERT(sizeof(CONFIG_ADV_MANUF_DATA) / 2 <= 27);
#endif //defined(CONFIG_ADV_MANUF_ENABLED) && CONFIG_ADV_MANUF_ENABLED == 1

static const m_coms_ble_params_t *sp_ble_params;
static m_coms_ble_adv_state_t     s_state;
static bool                       s_wait_for_flash = false;
static pm_peer_id_t               s_wait_for_peer_removal = PM_PEER_ID_INVALID;

#if CONFIG_SEC_LESC_PAIRING
static bool                       s_lesc_key_set;
static bool                       s_wait_for_lesc_key = false;
#endif

static bool m_coms_ble_adv_determine(const ble_evt_t *p_ble_evt);
static ret_code_t m_coms_ble_adv_real_start(void);

/* Number of directed advertising bursts targetted at a single host */
#define DIRECTED_ADV_COUNT      ROUNDED_DIV(1000ul * CONFIG_ADV_TIMEOUT, 1280)

// Validate if DIRECTED_ADV_COUNT value is within range that can be handled by type used for adv_count.
STATIC_ASSERT(((DIRECTED_ADV_COUNT * CONFIG_MAX_BONDS) <= UINT8_MAX) &&
              (DIRECTED_ADV_COUNT != 0));

/**@brief Advertising preparation.
 *
 * @details Encodes the required advertising data and passes it to the stack.
 *          Also, builds a structure to be passed to the stack when starting advertising.
 *
 * @param[in]  adv_flags  Indicates which type of advertising to use,
 *                        see @ref BLE_GAP_DISC_MODES.
 */
static ret_code_t m_coms_ble_adv_prepare(uint8_t adv_flags)
{
    ble_uuid_t adv_uuids[] = { { BLE_UUID_HUMAN_INTERFACE_DEVICE_SERVICE, BLE_UUID_TYPE_BLE } };
    ble_advdata_t advdata;
#if defined(CONFIG_ADV_MANUF_ENABLED) && CONFIG_ADV_MANUF_ENABLED == 1
    ble_advdata_manuf_data_t manuf_data;
    uint8_t manuf_data_contents[sizeof(CONFIG_ADV_MANUF_DATA) / 2];
#endif //defined(CONFIG_ADV_MANUF_ENABLED) && CONFIG_ADV_MANUF_ENABLED == 1

    // Build and set advertising data.
    memset(&advdata, 0, sizeof(advdata));

    advdata.name_type               = BLE_ADVDATA_FULL_NAME;
    advdata.include_appearance      = true;
    advdata.flags                   = adv_flags;
    advdata.uuids_complete.uuid_cnt = sizeof(adv_uuids) / sizeof(adv_uuids[0]);
    advdata.uuids_complete.p_uuids  = adv_uuids;
#if defined(CONFIG_ADV_MANUF_ENABLED) && CONFIG_ADV_MANUF_ENABLED == 1
    {
        uint8_t  str[] = CONFIG_ADV_MANUF_DATA;
        uint32_t val;

        for (int i = 0; i < strlen((char*)str); i += 2)
        {
            int ret;

            ret = sscanf((char*) &str[i], "%02x", (unsigned int *)&val);

            APP_ERROR_CHECK_BOOL(ret == 1);
            APP_ERROR_CHECK_BOOL(i/2 < sizeof(manuf_data_contents));

            manuf_data_contents[i/2] = (val & 0xFF);
        }

        memset(&manuf_data, 0, sizeof(manuf_data));

        manuf_data.company_identifier = CONFIG_ADV_MANUF_ID;
        manuf_data.data.p_data        = manuf_data_contents;
        manuf_data.data.size          = sizeof(manuf_data_contents);

    }

    advdata.p_manuf_specific_data   = &manuf_data;
#endif //defined(CONFIG_ADV_MANUF_ENABLED) && CONFIG_ADV_MANUF_ENABLED == 1

    return ble_advdata_set(&advdata, NULL);
}

static void adv_start_handler(void * dat, uint16_t size)
{
    ret_code_t status = m_coms_ble_adv_real_start();

    if (status == NRF_ERROR_SOFTDEVICE_NOT_ENABLED)
    {
        // The SoftDevice has been disabled while we waited for flash or LESC Key.
        s_state.mode            = ADV_MODE_NONE;
        s_state.adv_active      = false;
        s_state.bond_initiate   = false;
    }
    else
    {
        APP_ERROR_CHECK(status);
    }
}

#if CONFIG_SEC_LESC_PAIRING
/**@brief Set LESC key. */
static void set_lesc_key(ble_gap_lesc_p256_pk_t *p_pk)
{
    APP_ERROR_CHECK(pm_lesc_public_key_set(p_pk));
    s_lesc_key_set = true;

    if (s_wait_for_lesc_key)
    {
        NRF_LOG_DEBUG("LESC Key Ready: Advertising...");
        s_wait_for_lesc_key = false;
        APP_ERROR_CHECK(app_sched_event_put(0, 0, adv_start_handler));
    }
}

/**@brief Clear the existing LESC key and request generation of a new one. */
static void reset_lesc_key(void)
{
    ret_code_t status = m_coms_ble_lesc_generate_key(set_lesc_key);

    switch (status)
    {
        case NRF_SUCCESS:
            // Generation of a new key has started. Revoke the existing key and wait for a new one.
            APP_ERROR_CHECK(pm_lesc_public_key_set(NULL));
            s_lesc_key_set = false;
            break;

        case NRF_ERROR_BUSY:
            // LESC module is busy. Ignore request as a new key is being generated.
            ASSERT(s_lesc_key_set == false);
            break;

        default:
            APP_ERROR_CHECK(status);
            break;
    }
}
#endif /* CONFIG_SEC_LESC_PAIRING */

static void m_coms_ble_adv_on_ble_evt(const ble_evt_t *p_ble_evt, void *p_context)
{
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            NRF_LOG_DEBUG("Mode Change: BLE_GAP_EVT_CONNECTED => ADV_MODE_NONE");
            s_state.mode            = ADV_MODE_NONE;
            s_state.adv_active      = false;
            s_state.bond_initiate   = false;
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            if (m_coms_ble_adv_determine(p_ble_evt))
            {
                // Disconnection was caused by the need to advertise for a bond.
                APP_ERROR_CHECK(app_sched_event_put(0, 0, adv_start_handler));
            }
            break;

        case BLE_GAP_EVT_TIMEOUT:
            if (p_ble_evt->evt.gap_evt.params.timeout.src == BLE_GAP_TIMEOUT_SRC_ADVERTISING)
            {
                s_state.adv_active = false;

                if (m_coms_ble_adv_determine(p_ble_evt))
                {
                    // Continue advertising.
                    APP_ERROR_CHECK(app_sched_event_put(0, 0, adv_start_handler));
                }
            }
            break;

        default:
            /* Ignore */
            break;
    }
}
NRF_SDH_BLE_OBSERVER(m_coms_ble_adv_observer, BLE_OBSERVER_PRIORITY_DEFAULT, m_coms_ble_adv_on_ble_evt, NULL);

static void m_coms_ble_adv_on_sys_evt(uint32_t sys_evt, void *p_context)
{
    switch (sys_evt)
    {
        case NRF_EVT_FLASH_OPERATION_SUCCESS:
        case NRF_EVT_FLASH_OPERATION_ERROR:
            if (s_wait_for_flash)
            {
                if (!nrf_fstorage_is_busy(NULL))
                {
                    NRF_LOG_DEBUG("Flash Ready: Advertising...");
                    s_wait_for_flash = false;
                    APP_ERROR_CHECK(app_sched_event_put(0, 0, adv_start_handler));
                }
            }
            break;

        default:
            /* Ignore */
            break;
    }
}
NRF_SDH_SOC_OBSERVER(m_coms_ble_adv_soc_observer, SOC_OBSERVER_PRIORITY_LOW, m_coms_ble_adv_on_sys_evt, NULL);

static void m_coms_ble_adv_pm_evt_handler(const pm_evt_t *p_evt)
{
    switch (p_evt->evt_id)
    {
        case PM_EVT_PEER_DELETE_SUCCEEDED:
            NRF_LOG_DEBUG("PM_EVT_PEER_DELETE_SUCCEEDED: %d", p_evt->peer_id);
            if ((s_wait_for_peer_removal != PM_PEER_ID_INVALID) &&
                (s_wait_for_peer_removal == p_evt->peer_id))
            {
                NRF_LOG_DEBUG("Peer %d Deleted: Advertising...", p_evt->peer_id);
                s_wait_for_peer_removal = PM_PEER_ID_INVALID;
                APP_ERROR_CHECK(app_sched_event_put(0, 0, adv_start_handler));
            }
            break;

#if (CONFIG_SEC_LESC_PAIRING && CONFIG_SEC_ALLOW_REPAIRING)
        case PM_EVT_CONN_SEC_SUCCEEDED:
        case PM_EVT_CONN_SEC_FAILED:
            reset_lesc_key();
            break;
#endif

        default:
            /* Ignore */
            break;
    }
}

ret_code_t m_coms_ble_adv_init(const m_coms_ble_params_t *p_ble_params)
{
    if (p_ble_params == NULL)
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    sp_ble_params   = p_ble_params;
#if CONFIG_SEC_LESC_PAIRING
    s_lesc_key_set  = false;
#endif

    memset(&s_state, 0, sizeof(s_state));
    s_state.mode = ADV_MODE_NONE;

    return pm_register(m_coms_ble_adv_pm_evt_handler);
}

/**@brief Determine the best advertising method based on the module state. */
static bool m_coms_ble_adv_determine(const ble_evt_t *p_ble_evt)
{
    pm_peer_id_t peer_id, next_peer_id;
    unsigned int peer_count = 0;

    // Check integrity of the peer database.
    for (peer_id = pm_next_peer_id_get(PM_PEER_ID_INVALID);
         peer_id != PM_PEER_ID_INVALID;
         peer_id = next_peer_id)
    {
        bool peer_data_corrupted = false;

        // Obtain the ID of the next peer.
        next_peer_id = pm_next_peer_id_get(peer_id);

#if CONFIG_BLE_PEER_RANK_ENABLED
        uint32_t rank;
        uint16_t rank_size = sizeof(rank);

        if (pm_peer_data_load(peer_id, PM_PEER_DATA_ID_PEER_RANK, &rank, &rank_size) != NRF_SUCCESS)
        {
            peer_data_corrupted = true;
        }
#endif /* CONFIG_BLE_PEER_RANK_ENABLED */

#if CONFIG_BLE_DYNAMIC_ADDR_ENABLED
        ble_gap_addr_t addr;

        if (m_coms_ble_addr_local_addr_get(peer_id, &addr) != NRF_SUCCESS)
        {
            peer_data_corrupted = true;
        }
#endif /* CONFIG_BLE_DYNAMIC_ADDR_ENABLED */

        if (peer_data_corrupted)
        {
            NRF_LOG_WARNING("Deleting Corrupted Peer %d ...", peer_id);
            APP_ERROR_CHECK(pm_peer_delete(peer_id));
            continue;
        }

        // Count peer as valid.
        peer_count += 1;
    }

    // Differentiate between bonding behavior and normal behavior.
    if (s_state.bond_initiate)
    {
        switch (s_state.mode)
        {
            case ADV_MODE_NONE:
                if ((p_ble_evt != NULL) &&
                    (p_ble_evt->header.evt_id == BLE_GAP_EVT_DISCONNECTED))
                {
                    // Peer disconnected. No advertising is running. Do nothing.
                }
                else if (sp_ble_params->bond_params.bond_reconnect_all && peer_count > 0)
                {
                    // Try advertising vs. all bonded masters.
                    // Use undirected advertising with whitelist if directed advertising is not to be used.
                    if (sp_ble_params->bond_params.directed_adv)
                    {
                        NRF_LOG_INFO("Mode Change: ADV_MODE_NONE => ADV_MODE_DIRECTED_MULTIPLE");
                        s_state.mode = ADV_MODE_DIRECTED_MULTIPLE;
                    }
                    else
                    {
                        NRF_LOG_INFO("Mode Change: ADV_MODE_NONE => ADV_MODE_UNDIRECTED_WHITELIST");
                        s_state.mode = ADV_MODE_UNDIRECTED_WHITELIST;
                    }
                }
                else
                {
                    // Go straight to undirected bondable.
                    NRF_LOG_INFO("Mode Change: ADV_MODE_NONE => ADV_MODE_UNDIRECTED_BONDABLE");
                    s_state.mode = ADV_MODE_UNDIRECTED_BONDABLE;
                }

                s_state.adv_count = 0;
                break;

            case ADV_MODE_UNDIRECTED_WHITELIST:
                if (p_ble_evt &&
                    p_ble_evt->header.evt_id == BLE_GAP_EVT_TIMEOUT)
                {
                    if (s_state.adv_count < peer_count)
                    {
                        // Continue the same advertising type.
                    }
                    else
                    {
                        // Reconnection attempt completed: start undirected bondable.
                        NRF_LOG_INFO("Mode Change: ADV_MODE_UNDIRECTED_WHITELIST => ADV_MODE_UNDIRECTED_BONDABLE");
                        s_state.mode = ADV_MODE_UNDIRECTED_BONDABLE;
                    }
                }
                break;

            case ADV_MODE_DIRECTED_MULTIPLE:
                if (p_ble_evt &&
                    p_ble_evt->header.evt_id == BLE_GAP_EVT_TIMEOUT)
                {
                    if (s_state.adv_count < (peer_count * DIRECTED_ADV_COUNT))
                    {
                        // Continue the same advertising type.
                    }
                    else
                    {
                        NRF_LOG_INFO("Mode Change: ADV_MODE_DIRECTED_MULTIPLE => ADV_MODE_UNDIRECTED_BONDABLE");
                        s_state.mode = ADV_MODE_UNDIRECTED_BONDABLE;
                    }
                }
                break;

            case ADV_MODE_UNDIRECTED_BONDABLE:
                if (p_ble_evt && p_ble_evt->header.evt_id == BLE_GAP_EVT_TIMEOUT)
                {
                    // Bondable advertising timed out.
                    NRF_LOG_INFO("Mode Change: ADV_MODE_UNDIRECTED_BONDABLE => ADV_MODE_NONE");
                    s_state.mode          = ADV_MODE_NONE;
                    s_state.bond_initiate = false;
                }
                break;

            default:
                APP_ERROR_CHECK_BOOL(false);
                break;
        }
    }
    else
    {
        switch (s_state.mode)
        {
            case ADV_MODE_NONE:
                if ((p_ble_evt != NULL) &&
                    (p_ble_evt->header.evt_id == BLE_GAP_EVT_DISCONNECTED))
                {
                    // Peer disconnected. No advertising is running. Do nothing.
                }
                else if (peer_count > 0)
                {
                    if (sp_ble_params->bond_params.reconnect_all)
                    {
                        // Try advertising vs. all bonded masters.
                        // Use undirected advertising with whitelist if directed advertising is not to be used.
                        if (sp_ble_params->bond_params.directed_adv)
                        {
                            NRF_LOG_INFO("Mode Change: ADV_MODE_NONE => ADV_MODE_DIRECTED_MULTIPLE");
                            s_state.mode = ADV_MODE_DIRECTED_MULTIPLE;
                        }
                        else
                        {
                            NRF_LOG_INFO("Mode Change: ADV_MODE_NONE => ADV_MODE_UNDIRECTED_WHITELIST");
                            s_state.mode = ADV_MODE_UNDIRECTED_WHITELIST;
                        }
                    }
                    else
                    {
                        // One or more bonded masters found. Target the latest used one for reconnection.
                        if (sp_ble_params->bond_params.directed_adv)
                        {
                            NRF_LOG_INFO("Mode Change: ADV_MODE_NONE => ADV_MODE_DIRECTED_SINGLE");
                            s_state.mode = ADV_MODE_DIRECTED_SINGLE;
                        }
                        else
                        {
#if CONFIG_ADV_WHITELIST
                            NRF_LOG_INFO("Mode Change: ADV_MODE_NONE => ADV_MODE_UNDIRECTED_WHITELIST");
                            s_state.mode = ADV_MODE_UNDIRECTED_WHITELIST;
#else
                            NRF_LOG_INFO("Mode Change: ADV_MODE_NONE => ADV_MODE_UNDIRECTED_BONDABLE");
                            s_state.mode = ADV_MODE_UNDIRECTED_BONDABLE;
#endif /* CONFIG_ADV_WHITELIST */
                        }
                    }
                }
                else
                {
                    // No bonds found: look for one.
                    NRF_LOG_INFO("Mode Change: ADV_MODE_NONE => ADV_MODE_UNDIRECTED_BONDABLE");
                    s_state.mode = ADV_MODE_UNDIRECTED_BONDABLE;
                }

                s_state.adv_count = 0;
                break;

            case ADV_MODE_DIRECTED_SINGLE:
                if (p_ble_evt &&
                    p_ble_evt->header.evt_id == BLE_GAP_EVT_TIMEOUT)
                {
                    if (s_state.adv_count < DIRECTED_ADV_COUNT)
                    {
                        // Continue the same type.
                    }
                    else
                    {
                        NRF_LOG_INFO("Mode Change: ADV_MODE_DIRECTED_SINGLE => ADV_MODE_NONE");
                        s_state.mode = ADV_MODE_NONE;
                    }
                }
                break;

            case ADV_MODE_UNDIRECTED_WHITELIST:
                if (p_ble_evt &&
                    p_ble_evt->header.evt_id == BLE_GAP_EVT_TIMEOUT)
                {
                    if (s_state.adv_count < peer_count)
                    {
                        // Continue the same type.
                    }
                    else
                    {
                        // Reconnection vs. all bonded masters attempted.
                        NRF_LOG_INFO("Mode Change: ADV_MODE_UNDIRECTED_WHITELIST => ADV_MODE_NONE");
                        s_state.mode = ADV_MODE_NONE;
                    }
                }
                break;

            case ADV_MODE_DIRECTED_MULTIPLE:
                if (p_ble_evt &&
                    p_ble_evt->header.evt_id == BLE_GAP_EVT_TIMEOUT)
                {
                    if (s_state.adv_count < (peer_count * DIRECTED_ADV_COUNT))
                    {
                        // Continue the same type.
                    }
                    else
                    {
                        // Reconnection vs. all bonded masters attempted.
                        NRF_LOG_INFO("Mode Change: ADV_MODE_DIRECTED_MULTIPLE => ADV_MODE_NONE");
                        s_state.mode = ADV_MODE_NONE;
                    }
                }
                break;

            case ADV_MODE_UNDIRECTED_BONDABLE:
                NRF_LOG_INFO("Mode Change: ADV_MODE_UNDIRECTED_BONDABLE => ADV_MODE_NONE");
                s_state.mode = ADV_MODE_NONE;
                break;

            default:
                APP_ERROR_CHECK_BOOL(false);
                break;
        }
    }

#if CONFIG_SEC_LESC_PAIRING
    // Reset the LESC key for each new advertising session.
    switch (s_state.mode)
    {
#if CONFIG_SEC_ALLOW_REPAIRING
        case ADV_MODE_DIRECTED_SINGLE:
        case ADV_MODE_DIRECTED_MULTIPLE:
            if ((s_state.adv_count % DIRECTED_ADV_COUNT) != 0)
            {
                // We are continuing advertising to the same peer. Skip LESC key renewal.
                break;
            }
            /* FALTHROUGH */

        case ADV_MODE_UNDIRECTED_WHITELIST:
            // Change LESC key when switching to a new peer.
            reset_lesc_key();
            break;
#else /* !CONFIG_SEC_ALLOW_REPAIRING */
        case ADV_MODE_DIRECTED_SINGLE:
        case ADV_MODE_DIRECTED_MULTIPLE:
        case ADV_MODE_UNDIRECTED_WHITELIST:
            break;
#endif /* CONFIG_SEC_ALLOW_REPAIRING */

        case ADV_MODE_UNDIRECTED_BONDABLE:
            // Generate fresh keys for the new bond.
            reset_lesc_key();
            break;

        case ADV_MODE_NONE:
            break;

        default:
            ASSERT(false);
            break;
    }
#endif

    return (s_state.mode != ADV_MODE_NONE);
}

/* Start advertising prepared by \ref m_coms_ble_adv_determine(). */
static ret_code_t m_coms_ble_adv_real_start(void)
{
    pm_peer_id_t peer_id;
    pm_peer_id_t peer_ids[CONFIG_MAX_BONDS];
    unsigned int peer_count;
    pm_peer_data_bonding_t peer_data;

    ble_gap_adv_params_t adv_params;
    bool use_whitelist;
    ret_code_t status;

    if (s_state.adv_active)
    {
        // Advertising is already running.
        return NRF_SUCCESS;
    }

    if (nrf_fstorage_is_busy(NULL))
    {
        NRF_LOG_DEBUG("Waiting for flash...");
        s_wait_for_flash = true;
        return NRF_SUCCESS;
    }

#if CONFIG_SEC_LESC_PAIRING
    // Check if the LESC key is ready.
    if (s_lesc_key_set == false &&
        ((s_state.mode == ADV_MODE_UNDIRECTED_BONDABLE) || (CONFIG_SEC_ALLOW_REPAIRING)))
    {
        NRF_LOG_DEBUG("Waiting for the LESC key ...");
        s_wait_for_lesc_key = true;
        return NRF_SUCCESS;
    }
#endif

    use_whitelist   = false;
    peer_id         = PM_PEER_ID_INVALID;
    memset(&adv_params, 0, sizeof(adv_params));

    // Getting the number of bonded masters.
    status = m_coms_ble_addr_peer_ids_get(peer_ids, &peer_count);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    switch (s_state.mode)
    {
        case ADV_MODE_DIRECTED_SINGLE:
            peer_id = peer_ids[peer_count - 1];

            s_state.adv_type    = BLE_GAP_ADV_TYPE_ADV_DIRECT_IND;
            s_state.adv_count   += 1;
            break;

        case ADV_MODE_DIRECTED_MULTIPLE:
            // Decide which bonded master to target at.
            if (s_state.bond_initiate)
            {
                // The least used one first.
                peer_id = peer_ids[s_state.adv_count / DIRECTED_ADV_COUNT];
            }
            else
            {
                // The most used one first.
                peer_id = peer_ids[peer_count - (s_state.adv_count / DIRECTED_ADV_COUNT) - 1];
            }

            s_state.adv_type    = BLE_GAP_ADV_TYPE_ADV_DIRECT_IND;
            s_state.adv_count   += 1;
            break;

        case ADV_MODE_UNDIRECTED_WHITELIST:
            // Decide which bonded master to target at.
            if (s_state.bond_initiate)
            {
                // The least used one first.
                peer_id = peer_ids[s_state.adv_count];
            }
            else
            {
                // The most used one first.
                peer_id = peer_ids[peer_count - s_state.adv_count - 1];
            }

            s_state.adv_type    = BLE_GAP_ADV_TYPE_ADV_IND;
            s_state.adv_count   += 1;
            use_whitelist       = true;
            break;

        case ADV_MODE_UNDIRECTED_BONDABLE:
            // Check whether we need to make room for the new bond.
            if (peer_count >= CONFIG_MAX_BONDS)
            {
                pm_peer_id_t peer_id;

                status = pm_peer_ranks_get(NULL, NULL, &peer_id, NULL);
                if (status != NRF_SUCCESS)
                {
                    return status;
                }

                status = pm_peer_delete(peer_id);
                if (status != NRF_SUCCESS)
                {
                    return status;
                }

                NRF_LOG_INFO("Deleting Peer %d ...", peer_id);
                s_wait_for_peer_removal = peer_id;
                return NRF_SUCCESS;
            }

            s_state.adv_type = BLE_GAP_ADV_TYPE_ADV_IND;
            break;

        default:
            APP_ERROR_CHECK_BOOL(false);
            break;
    }

    if (peer_id != PM_PEER_ID_INVALID)
    {
#if CONFIG_BLE_DYNAMIC_ADDR_ENABLED
        ble_gap_addr_t addr;

        // Get local address used to connect with the given peer.
        status = m_coms_ble_addr_local_addr_get(peer_id, &addr);
        if (status != NRF_SUCCESS)
        {
            return status;
        }

        // Set the local address.
        status = pm_id_addr_set(&addr);
        if (status != NRF_SUCCESS)
        {
            return status;
        }
#endif
        // Get the peer address.
        status = pm_peer_data_bonding_load(peer_id, &peer_data);
        if (status != NRF_SUCCESS)
        {
            return status;
        }

        // Set identities list.
        status = pm_device_identities_list_set(&peer_id, 1);
        if (status != NRF_SUCCESS)
        {
            return status;
        }
    }
    else
    {
        // Clear identities list.
        status = pm_device_identities_list_set(NULL, 0);
        if (status != NRF_SUCCESS)
        {
            return status;
        }
    }

    status = (use_whitelist) ? pm_whitelist_set(&peer_id, 1) :
                               pm_whitelist_set(NULL, 0);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

#if CONFIG_BLE_DYNAMIC_ADDR_ENABLED
    if (sp_ble_params->bond_params.change_address &&
       (s_state.mode == ADV_MODE_UNDIRECTED_BONDABLE))
    {
        m_coms_ble_evt_t evt;
        ble_gap_addr_t addr;
        ret_code_t status;

        status = m_coms_ble_addr_local_addr_new(&addr);
        switch (status)
        {
            case NRF_ERROR_INVALID_STATE:
                // No addresses stored in the database: do not change the address!
                break;

            case NRF_SUCCESS:
                // Set a new address.
                status = pm_id_addr_set(&addr);
                if (status != NRF_SUCCESS)
                {
                    return status;
                }

                // Notify the application that the address has changed.
                evt.type = M_COMS_BLE_EVT_ADDR_CHANGED;
                sp_ble_params->event_handler(&evt, sizeof(evt));
                break;

            default:
                return status;
        }
    }
#endif /* CONFIG_BLE_DYNAMIC_ADDR_ENABLED */

    // Configure advertising parameters.
    switch (s_state.adv_type)
    {
        case BLE_GAP_ADV_TYPE_ADV_IND:
            // Undirected advertising.
            adv_params.type = BLE_GAP_ADV_TYPE_ADV_IND;

            NRF_LOG_INFO("Advertising: BLE_GAP_ADV_TYPE_ADV_IND:");
            if (!use_whitelist)
            {
                // There is no whitelist set up: This is generic "I want to bond" advertising.
                NRF_LOG_DEBUG("\t- Whitelist: None");

                // Bondable.
                s_state.adv_flags   = BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE;

                // Anyone can connect.
                adv_params.fp       = BLE_GAP_ADV_FP_ANY;

                // There is no need for fast advertising because masters wanting to connect will do foreground scanning (intense scanning).
                adv_params.interval = sp_ble_params->adv_params.interval;

                // Time-out should always be 180 s when advertising as limited discoverable (according to Core spec).
                adv_params.timeout  = BLE_GAP_ADV_TIMEOUT_LIMITED_MAX;
            }
            else
            {
                // Whitelist has been configured for advertising targeted towards masters without fixed addresses.
#if NRF_LOG_ENABLED
                NRF_LOG_DEBUG("\t- Whitelist:");
                {
                    ble_gap_addr_t ble_addrs[8];
                    ble_gap_irk_t ble_irks[8];
                    uint32_t addr_cnt = ARRAY_SIZE(ble_addrs);
                    uint32_t irk_cnt = ARRAY_SIZE(ble_irks);

                    if (pm_whitelist_get(ble_addrs, &addr_cnt, ble_irks, &irk_cnt) == NRF_SUCCESS)
                    {
                        unsigned int i;

                        for (i = 0; i < addr_cnt; i++)
                        {
                            NRF_LOG_DEBUG("\t\tADDR: %02X:%02X:%02X:%02X:%02X:%02X",
                                          ble_addrs[i].addr[5],
                                          ble_addrs[i].addr[4],
                                          ble_addrs[i].addr[3],
                                          ble_addrs[i].addr[2],
                                          ble_addrs[i].addr[1],
                                          ble_addrs[i].addr[0]);
                        }

                        for (i = 0; i < irk_cnt; i++)
                        {
                            NRF_LOG_DEBUG("\t\tIRK:  %02X:%02X:%02X:%02X:%02X:%02X:...",
                                          ble_irks[i].irk[0],
                                          ble_irks[i].irk[1],
                                          ble_irks[i].irk[2],
                                          ble_irks[i].irk[3],
                                          ble_irks[i].irk[4],
                                          ble_irks[i].irk[5]);
                        }
                    }
                }
#endif /* NRF_LOG_ENABLED */

                // Not bondable.
                s_state.adv_flags   = BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED;

                // Only devices on the whitelist can connect.
                adv_params.fp       = BLE_GAP_ADV_FP_FILTER_BOTH;

                // Advertising with the shortest possible interval for a few seconds.
                adv_params.interval = sp_ble_params->adv_params.interval;
                adv_params.timeout  = sp_ble_params->adv_params.timeout;
            }
            break;

        case BLE_GAP_ADV_TYPE_ADV_DIRECT_IND:
            // Directed advertising.
            adv_params.type         = BLE_GAP_ADV_TYPE_ADV_DIRECT_IND;
            adv_params.p_peer_addr  = &peer_data.peer_ble_id.id_addr_info;

            // Not bondable.
            s_state.adv_flags       = BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED;

            // Directed advertising is always fixed at 1.28 seconds.
            adv_params.timeout      = 0;

            NRF_LOG_INFO("Advertising: BLE_GAP_ADV_TYPE_ADV_DIRECT_IND:");
            NRF_LOG_DEBUG("\t- Peer Address: %02X:%02X:%02X:%02X:%02X:%02X",
                          adv_params.p_peer_addr->addr[5],
                          adv_params.p_peer_addr->addr[4],
                          adv_params.p_peer_addr->addr[3],
                          adv_params.p_peer_addr->addr[2],
                          adv_params.p_peer_addr->addr[1],
                          adv_params.p_peer_addr->addr[0]);
            NRF_LOG_DEBUG("\t- Attempt: %u of %u",
                          1u + ((s_state.adv_count - 1) % DIRECTED_ADV_COUNT),
                          DIRECTED_ADV_COUNT);
            break;

        default:
            // Not dealing with these advertising types.
            return NRF_ERROR_INTERNAL;
    }

    status = m_coms_ble_adv_prepare(s_state.adv_flags);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    status = sd_ble_gap_adv_start(&adv_params, CONFIG_CONN_CFG_HIGH_BW_TAG);
    if (status == NRF_SUCCESS)
    {
        s_state.adv_active = true;

        // Notify upper layers each time we start advertising to a different peer.
        if ((s_state.adv_type != BLE_GAP_ADV_TYPE_ADV_DIRECT_IND) ||
            ((s_state.adv_type == BLE_GAP_ADV_TYPE_ADV_DIRECT_IND) && ((s_state.adv_count % DIRECTED_ADV_COUNT) == 1)))
        {
            m_coms_ble_evt_t evt;

            evt.type = (s_state.mode == ADV_MODE_UNDIRECTED_BONDABLE) ? M_COMS_BLE_EVT_ADV_BONDABLE :
                                                                        M_COMS_BLE_EVT_ADV;

            evt.data.adv_data.peer_id = (s_state.mode == ADV_MODE_UNDIRECTED_BONDABLE) ?
                                    PM_PEER_ID_INVALID : peer_id;

            sp_ble_params->event_handler(&evt, sizeof(evt));
        }
    }

    return status;
}

ret_code_t m_coms_ble_adv_stop(void)
{
    if (!s_state.adv_active)
    {
        return NRF_SUCCESS;
    }

    ret_code_t status = sd_ble_gap_adv_stop();
    if (status == NRF_SUCCESS)
    {
        NRF_LOG_INFO("Mode Change: %s() => ADV_MODE_NONE", __func__);
        s_state.mode        = ADV_MODE_NONE;
        s_state.adv_active  = false;
    }

    return status;
}

ret_code_t m_coms_ble_adv_start(bool bondable)
{
    ret_code_t status;

    s_state.bond_initiate   = bondable;

    status = m_coms_ble_adv_stop();
    if (status == NRF_SUCCESS)
    {
        if (!m_coms_ble_adv_determine(NULL))
        {
            // The m_coms_ble_adv_determine() should always return `true` in this case.
            return NRF_ERROR_INTERNAL;
        }

        status = m_coms_ble_adv_real_start();
    }

    return status;
}

bool m_coms_ble_adv_running(void)
{
    return (s_state.mode != ADV_MODE_NONE);
}

bool m_coms_ble_adv_bondable_running(void)
{
    return (m_coms_ble_adv_running() && s_state.bond_initiate);
}

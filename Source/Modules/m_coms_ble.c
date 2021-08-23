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

#include "m_coms_ble.h"
#include "nordic_common.h"
#include "app_error.h"
#include "app_debug.h"
#include "app_scheduler.h"
#include "app_timer.h"
#include "ble.h"
#include "ble_advdata.h"
#include "ble_bas.h"
#include "ble_conn_state.h"
#include "ble_conn_params.h"
#include "ble_dis.h"
#include "ble_flash.h"
#include "ble_ias.h"
#include "ble_hci.h"
#include "ble_hids.h"
#include "ble_srv_common.h"
#include "fds.h"
#include "peer_manager.h"
#include "m_coms_ble_addr.h"
#include "m_coms_ble_adv.h"
#include "m_coms_ble_atvv.h"
#include "m_coms_ble_dfu.h"
#include "m_coms_ble_hid.h"
#include "m_coms_ble_lesc.h"
#include "nrf.h"
#include "nrf_assert.h"
#include "nrf_atomic.h"
#include "nrf_cli.h"
#include "nrf_cli_ble_uart.h"
#include "nrf_drv_gpiote.h"
#include "nrf_drv_ppi.h"
#include "nrf_error.h"
#include "nrf_fstorage.h"
#include "nrf_log_ctrl.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"
#include "nrf_ble_gatt.h"
#include "crc16.h"
#include "compiler_abstraction.h"
#include "task_manager.h"

#include "resources.h"
#include "sr3_config.h"

#define NRF_LOG_MODULE_NAME m_coms_ble
#define NRF_LOG_LEVEL       CONFIG_BLE_MODULE_LOG_LEVEL
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

// Central role is not supported.
STATIC_ASSERT(NRF_SDH_BLE_CENTRAL_LINK_COUNT == 0);

// Check if we can cast FDS error codes to SDK error codes.
STATIC_ASSERT(FDS_SUCCESS == NRF_SUCCESS);

// Verify SDK configuration.
STATIC_ASSERT(FDS_ENABLED);
STATIC_ASSERT(NRF_FSTORAGE_ENABLED);
STATIC_ASSERT(PEER_MANAGER_ENABLED);
STATIC_ASSERT(BLE_DIS_ENABLED);
STATIC_ASSERT(BLE_HIDS_ENABLED);
STATIC_ASSERT(NRF_BLE_GATT_ENABLED);

STATIC_ASSERT(BLE_CONN_PARAMS_BLE_OBSERVER_PRIO == BLE_OBSERVER_PRIORITY_HIGH);
STATIC_ASSERT(BLE_CONN_STATE_BLE_OBSERVER_PRIO  == BLE_OBSERVER_PRIORITY_HIGH);
STATIC_ASSERT(NRF_BLE_GATT_BLE_OBSERVER_PRIO    == BLE_OBSERVER_PRIORITY_HIGH);
STATIC_ASSERT(BLE_HIDS_BLE_OBSERVER_PRIO        == BLE_OBSERVER_PRIORITY_DEFAULT);
STATIC_ASSERT(BLE_NUS_BLE_OBSERVER_PRIO         == BLE_OBSERVER_PRIORITY_DEFAULT);
STATIC_ASSERT(PM_BLE_OBSERVER_PRIO              == BLE_OBSERVER_PRIORITY_DEFAULT);

#if CONFIG_BATT_MEAS_ENABLED
STATIC_ASSERT(BLE_BAS_ENABLED);
STATIC_ASSERT(BLE_BAS_BLE_OBSERVER_PRIO == BLE_OBSERVER_PRIORITY_DEFAULT);
#endif

#if (CONFIG_LED_SIGNAL_IMMEDIATE_ALERT || CONFIG_BUZZER_ENABLED)
STATIC_ASSERT(BLE_IAS_ENABLED);
STATIC_ASSERT(BLE_IAS_BLE_OBSERVER_PRIO == BLE_OBSERVER_PRIORITY_DEFAULT);
#endif

// Verify Bluetooth Configuration
#if !(CONFIG_SEC_LEGACY_PAIRING || CONFIG_SEC_LESC_PAIRING)
#error "At least one pairing model (CONFIG_SEC_LEGACY_PAIRING or CONFIG_SEC_LESC_PAIRING) has to be enabled!"
#endif

#if !(CONFIG_PHY_TX_1MBPS || CONFIG_PHY_TX_2MBPS)
#error "At least one TX PHY (CONFIG_PHY_TX_1MBPS or CONFIG_PHY_TX_2MBPS) have to enabled!"
#endif

#if !(CONFIG_PHY_RX_1MBPS || CONFIG_PHY_RX_2MBPS)
#error "At least one RX PHY (CONFIG_PHY_RX_1MBPS or CONFIG_PHY_RX_2MBPS) have to enabled!"
#endif

//
// These defines are considered very unlikely to be changed between HID applications.
//

// Connection parameter handling
#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(CONFIG_FIRST_CONN_PARAMS_UPDATE_DELAY)          /**< Time from initiating event (connect or start of notification) to the first time a ble_gap_conn_param_update is called. */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS((1000u * CONFIG_NEXT_CONN_PARAMS_UPDATE_DELAY)) /**< Time between each call to ble_gap_conn_param_update after the first one. */
#define MAX_CONN_PARAMS_UPDATE_COUNT    CONFIG_MAX_CONN_PARAMS_UPDATE_COUNT                             /**< Number of attempts before giving up the connection parameter negotiation. */

APP_TIMER_DEF(s_encryption_timer);      /** Timer used to request encryption if Central does not enable it in a timely manner .*/

// Static variables
static const m_coms_ble_params_t    *sp_ble_params;                 /**< BLE parameters. */
static bool                          s_encrypted;                   /**< Link encryption status. */
static bool                          s_boot_mode_active;            /**< In Boot or Report mode? */
static uint16_t                      s_conn_handle;                 /**< Handle value of the current connection. */
static nrf_ble_gatt_t                s_gatt;                        /**< BLE GATT module handle. */
static uint16_t                      s_hid_cccd_handle;             /**< HID CCCD handle. */
static bool                          s_enable_sl_on_hid_cccd_write; /**< Waiting for the HID CCCD enable handler. */
static nrf_atomic_u32_t              s_sl_disabled_ctr;             /**< Number of slave latency disabling requests. */

#if CONFIG_CLI_ENABLED
static bool s_cli_sl_disabled;                                      /**< True, if slave latency has been disabled from the CLI. */
#endif

#if CONFIG_BATT_MEAS_ENABLED
static ble_bas_t                     s_bas;                     /**< Structure used to identify the battery service. */
static ble_srv_report_ref_t          s_bas_report_ref;          /**< Battery service report reference mapping to the HID service. */
#endif

#if (CONFIG_LED_SIGNAL_IMMEDIATE_ALERT || CONFIG_BUZZER_ENABLED)
static ble_ias_t                     s_ias;                     /**< Structure used to identify the IAS service. */
#endif

#define WAITING_REASON_SHUTDOWN     (1UL << 0)

static bool                         s_waiting_for_flash;
static bool                         s_waiting_for_disconnect;
static uint32_t                     s_waiting_reason;

#if CONFIG_CLI_BLE_ENABLED
NRF_CLI_BLE_UART_DEF(m_cli_ble_uart, &s_gatt, CONFIG_CLI_BLE_TX_BUFFER_SIZE, CONFIG_CLI_BLE_RX_BUFFER_SIZE);
NRF_CLI_DEF(m_ble_cli, "SR3-BT> ", &m_cli_ble_uart.transport, '\n', CONFIG_LOG_BLE_QSIZE);
#endif

/**@brief Convert PHY ID to human readable PHY speed. */
static const char *m_coms_ble_phy_speed(uint8_t phy_id)
{
    switch (phy_id)
    {
        case BLE_GAP_PHY_1MBPS:
            return "1 Mbit/s";

        case BLE_GAP_PHY_2MBPS:
            return "2 Mbit/s";

        case BLE_GAP_PHY_CODED:
            return "125 kbit/s";

        default:
            return "UNKNOWN";
    }
}

/**@brief BLE Stack event handler.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 */
static void m_coms_ble_on_ble_evt(ble_evt_t const * p_ble_evt, void *p_context)
{
    m_coms_ble_evt_t coms_evt;
    const ble_gatts_evt_write_t *p_evt_write;
    const ble_gap_phys_t supported_phys =
    {
        .tx_phys = ((CONFIG_PHY_TX_1MBPS) ? BLE_GAP_PHY_1MBPS : 0) |
                   ((CONFIG_PHY_TX_2MBPS) ? BLE_GAP_PHY_2MBPS : 0),
        .rx_phys = ((CONFIG_PHY_RX_1MBPS) ? BLE_GAP_PHY_1MBPS : 0) |
                   ((CONFIG_PHY_RX_2MBPS) ? BLE_GAP_PHY_2MBPS : 0),
    };

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
        {
#if CONFIG_CLI_BLE_ENABLED
            nrf_cli_ble_uart_config_t ble_uart_config = {
                    .conn_handle = p_ble_evt->evt.gap_evt.conn_handle
            };
#endif /* CONFIG_CLI_BLE_ENABLED */

            s_conn_handle   = p_ble_evt->evt.gap_evt.conn_handle;
            s_encrypted     = false;

            // Disable slave latency
            APP_ERROR_CHECK(m_coms_ble_sl_disable());
            s_enable_sl_on_hid_cccd_write = true;

#if CONFIG_PHY_SEND_UPDATE_REQUEST
            APP_ERROR_CHECK(sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle,
                                                  &supported_phys));
#endif

#if CONFIG_CLI_BLE_ENABLED
            // Create Bluetooth Console.
            APP_ERROR_CHECK(nrf_cli_init(&m_ble_cli,
                                         &ble_uart_config,
                                         true,
                                         true,
                                         (nrf_log_severity_t)(CONFIG_LOG_BLE_SEVERITY)));

            APP_ERROR_CHECK(nrf_cli_task_create(&m_ble_cli));
#endif /* CONFIG_CLI_BLE_ENABLED */

            // Notify m_coms of the event.
            coms_evt.type = M_COMS_BLE_EVT_CONNECTED;
            coms_evt.data.conn_data.conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            pm_peer_id_get(coms_evt.data.conn_data.conn_handle, &coms_evt.data.conn_data.peer_id);
            sp_ble_params->event_handler(&coms_evt, sizeof(coms_evt));
            break;
        }

        case BLE_GAP_EVT_DISCONNECTED:
            s_conn_handle = BLE_CONN_HANDLE_INVALID;

            // Stop Encryption timer.
            APP_ERROR_CHECK(app_timer_stop(s_encryption_timer));

            // Reset slave latency settings as they are no longer valid.
            nrf_atomic_u32_store(&s_sl_disabled_ctr, 0);
#if CONFIG_CLI_ENABLED
            s_cli_sl_disabled = false;
#endif

#if CONFIG_CLI_BLE_ENABLED
            // Destroy Bluetooth console.
            APP_ERROR_CHECK(nrf_cli_uninit(&m_ble_cli));
#endif

            if (s_waiting_for_disconnect)
            {
                NRF_LOG_DEBUG("Disconnected.");

                if (s_waiting_reason & WAITING_REASON_SHUTDOWN)
                {
                    s_waiting_reason &= ~(WAITING_REASON_SHUTDOWN);
                    APP_ERROR_CHECK(m_coms_ble_shutdown());
                }

                s_waiting_for_disconnect = false;
            }

            if (m_coms_ble_adv_running())
            {
                // Advertising has started again: do not do anything.
            }
            else
            {
                // Disconnection was not caused by the bonding procedure: notify m_coms of the event.
                coms_evt.type = M_COMS_BLE_EVT_DISCONNECTED;
                coms_evt.data.conn_data.conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
                pm_peer_id_get(coms_evt.data.conn_data.conn_handle, &coms_evt.data.conn_data.peer_id);
                sp_ble_params->event_handler(&coms_evt, sizeof(coms_evt));
            }
            break;

        case BLE_GAP_EVT_TIMEOUT:
            if (!m_coms_ble_adv_running() && (s_conn_handle == BLE_CONN_HANDLE_INVALID))
            {
                // Notify m_coms of the event.
                coms_evt.type = M_COMS_BLE_EVT_ADV_TIMEOUT;
                sp_ble_params->event_handler(&coms_evt, sizeof(coms_evt));
            }
            break;

        case BLE_GAP_EVT_AUTH_KEY_REQUEST:
            switch (p_ble_evt->evt.gap_evt.params.auth_key_request.key_type)
            {
                case BLE_GAP_AUTH_KEY_TYPE_PASSKEY:
                    // Notify m_coms of the event.
                    coms_evt.type = M_COMS_BLE_EVT_PASSKEY_REQ;
                    sp_ble_params->event_handler(&coms_evt, sizeof(coms_evt));
                    break;

                case BLE_GAP_AUTH_KEY_TYPE_OOB:
                    // Notify m_coms of the event.
                    coms_evt.type = M_COMS_BLE_EVT_OOBKEY_REQ;
                    sp_ble_params->event_handler(&coms_evt, sizeof(coms_evt));
                    break;

                default:
                    break;
            }

            // If you do not want encryption timer to run during passkey entry.
            APP_ERROR_CHECK(app_timer_stop(s_encryption_timer));
            break;

        case BLE_GAP_EVT_CONN_PARAM_UPDATE:
            coms_evt.type = M_COMS_BLE_EVT_CONN_UPDATE;
            coms_evt.data.conn_update.min_conn_interval = p_ble_evt->evt.gap_evt.params.conn_param_update.conn_params.min_conn_interval;
            coms_evt.data.conn_update.max_conn_interval = p_ble_evt->evt.gap_evt.params.conn_param_update.conn_params.max_conn_interval;
            coms_evt.data.conn_update.slave_latency = p_ble_evt->evt.gap_evt.params.conn_param_update.conn_params.slave_latency;
            coms_evt.data.conn_update.supervision_timeout = p_ble_evt->evt.gap_evt.params.conn_param_update.conn_params.conn_sup_timeout;
            sp_ble_params->event_handler(&coms_evt, sizeof(coms_evt));
            break;

        case BLE_GATTS_EVT_HVN_TX_COMPLETE:
            coms_evt.type = M_COMS_BLE_EVT_TX_COMPLETE;
            coms_evt.data.tx_complete.count = p_ble_evt->evt.gatts_evt.params.hvn_tx_complete.count;
            sp_ble_params->event_handler(&coms_evt, sizeof(coms_evt));
            break;

        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
            NRF_LOG_DEBUG("BLE_GAP_EVT_PHY_UPDATE_REQUEST (TX = 0x%02X, RX = 0x%02X)",
                          p_ble_evt->evt.gap_evt.params.phy_update_request.peer_preferred_phys.tx_phys,
                          p_ble_evt->evt.gap_evt.params.phy_update_request.peer_preferred_phys.rx_phys);

            APP_ERROR_CHECK(sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &supported_phys));
            break;

        case BLE_GAP_EVT_PHY_UPDATE:
            if (p_ble_evt->evt.gap_evt.params.phy_update.status == BLE_HCI_STATUS_CODE_SUCCESS)
            {
                NRF_LOG_INFO("PHY Update: TX = %s, RX = %s",
                             m_coms_ble_phy_speed(p_ble_evt->evt.gap_evt.params.phy_update.tx_phy),
                             m_coms_ble_phy_speed(p_ble_evt->evt.gap_evt.params.phy_update.rx_phy));
            }
            else
            {
                NRF_LOG_WARNING("PHY Update procedure failed! (HCI Error Code: 0x%02X)",
                                p_ble_evt->evt.gap_evt.params.phy_update.status);
            }
            break;

        case BLE_GATTS_EVT_WRITE:
            p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

            // Check whether this is the correct CCCD and if this is a 'start notification'
            if ((s_enable_sl_on_hid_cccd_write)                         &&
                (p_evt_write->handle == s_hid_cccd_handle)              &&
                (ble_srv_is_notification_enabled(p_evt_write->data)))
            {
                s_enable_sl_on_hid_cccd_write = false;
                APP_ERROR_CHECK(m_coms_ble_sl_enable());
            }
            break;

        default:
            break;
    }
}

NRF_SDH_BLE_OBSERVER(m_coms_ble_observer, BLE_OBSERVER_PRIORITY_LOW, m_coms_ble_on_ble_evt, NULL);

/**@brief Encrypting time-out handler.
 *
 * @param[in] p_context
 */
static void on_encrypt_timeout(void* p_context)
{
    if ((s_conn_handle != BLE_CONN_HANDLE_INVALID) && !s_encrypted)
    {
        NRF_LOG_DEBUG("Securing Connection...");
        APP_ERROR_CHECK(pm_conn_secure(s_conn_handle, false));
    }
}

/**@brief Function for handling the Peer Manager events.
 *
 * @param[in]   p_event Pointer to event data structure.
 */
static void peer_manager_evt_handler(pm_evt_t const *p_evt)
{
    ble_gap_sec_params_t sec_params;
    m_coms_ble_evt_t coms_evt;

    pm_conn_sec_config_t pm_conn_sec_config =
    {
        .allow_repairing = ((CONFIG_SEC_ALLOW_REPAIRING) != 0),
    };

    switch (p_evt->evt_id)
    {
        case PM_EVT_BONDED_PEER_CONNECTED:
            NRF_LOG_INFO("PM_EVT_BONDED_PEER_CONNECTED: %d", p_evt->peer_id);

            // Start Encryption Timer.
            APP_ERROR_CHECK(app_timer_start(s_encryption_timer, APP_TIMER_TICKS(CONFIG_ENC_REQ_DELAY), 0));

#if CONFIG_BLE_PEER_RANK_ENABLED
            // Bump peer rank.
            APP_ERROR_CHECK(pm_peer_rank_highest(p_evt->peer_id));
#endif
            break;

        case PM_EVT_CONN_SEC_START:
            NRF_LOG_INFO("PM_EVT_CONN_SEC_START: %d", p_evt->peer_id);

            // Stop Encryption timer.
            APP_ERROR_CHECK(app_timer_stop(s_encryption_timer));
            break;

        case PM_EVT_CONN_SEC_SUCCEEDED:
            NRF_LOG_INFO("PM_EVT_CONN_SEC_SUCCEEDED: %d", p_evt->peer_id);

            // Stop Encryption timer.
            APP_ERROR_CHECK(app_timer_stop(s_encryption_timer));

            // Attach application data to bond if it was created.
            if (p_evt->params.conn_sec_succeeded.procedure == PM_LINK_SECURED_PROCEDURE_BONDING)
            {
#if CONFIG_BLE_PEER_RANK_ENABLED
                // Bump peer rank.
                APP_ERROR_CHECK(pm_peer_rank_highest(p_evt->peer_id));
#endif
#if CONFIG_BLE_DYNAMIC_ADDR_ENABLED
                ble_gap_addr_t ble_addr;

                // Save our local address.
                APP_ERROR_CHECK(pm_id_addr_get(&ble_addr));
                APP_ERROR_CHECK(m_coms_ble_addr_local_addr_set(p_evt->peer_id, &ble_addr));
#endif
            }

            // If this is first pairing/bonding during this connection:
            if (!s_encrypted)
            {
                // Mark connection as encrypted.
                s_encrypted = true;

                // Enable slave latency
                if (p_evt->params.conn_sec_succeeded.procedure == PM_LINK_SECURED_PROCEDURE_ENCRYPTION)
                {
                    s_enable_sl_on_hid_cccd_write = false;
                    APP_ERROR_CHECK(m_coms_ble_sl_enable());
                }

                // Notify upper layers.
                coms_evt.type = M_COMS_BLE_EVT_CONN_SECURED;
                coms_evt.data.conn_data.conn_handle = p_evt->conn_handle;
                pm_peer_id_get(coms_evt.data.conn_data.conn_handle, &coms_evt.data.conn_data.peer_id);
                sp_ble_params->event_handler(&coms_evt, sizeof(coms_evt));
            }
            break;

        case PM_EVT_CONN_SEC_FAILED:
            NRF_LOG_WARNING("PM_EVT_CONN_SEC_FAILED: %d", p_evt->peer_id);

            /*
             * If connection was encrypted, it is still encrypted.
             * If connection was not encrypted, disconnect.
             */
            if (!s_encrypted)
            {
                m_coms_ble_disconnect();
            }
            break;

        case PM_EVT_CONN_SEC_CONFIG_REQ:
            NRF_LOG_DEBUG("PM_EVT_CONN_SEC_CONFIG_REQ: %d", p_evt->peer_id);

            /*
             * A connected peer (central) is trying to pair, but the Peer Manager already has a bond
             * for that peer. Setting allow_repairing to false rejects the pairing request.
             */
            pm_conn_sec_config_reply(p_evt->conn_handle, &pm_conn_sec_config);
            break;

        case PM_EVT_CONN_SEC_PARAMS_REQ:
            NRF_LOG_DEBUG("PM_EVT_CONN_SEC_PARAMS_REQ: %d (lesc=%d, mitm=%d, oob=%d, iocaps=%d)",
                          p_evt->peer_id,
                          p_evt->params.conn_sec_params_req.p_peer_params->lesc,
                          p_evt->params.conn_sec_params_req.p_peer_params->mitm,
                          p_evt->params.conn_sec_params_req.p_peer_params->oob,
                          p_evt->params.conn_sec_params_req.p_peer_params->io_caps);

            // Configure our security basing on host capabilities.
            memset(&sec_params, 0, sizeof(sec_params));

            sec_params.bond           = CONFIG_SEC_BOND;
            sec_params.io_caps        = sp_ble_params->sec_params.io_capabilities;
            sec_params.min_key_size   = CONFIG_SEC_MIN_KEY_SIZE;
            sec_params.max_key_size   = CONFIG_SEC_MAX_KEY_SIZE;
            sec_params.kdist_own.enc  = CONFIG_SEC_BOND;
            sec_params.kdist_own.id   = CONFIG_SEC_BOND;
            sec_params.kdist_peer.enc = CONFIG_SEC_BOND;
            sec_params.kdist_peer.id  = CONFIG_SEC_BOND;

#if CONFIG_SEC_LESC_PAIRING
            if (p_evt->params.conn_sec_params_req.p_peer_params->lesc)
            {
                /*
                 * LESC Paring:
                 *  - OOB flag has to be cleared since we cannot fetch data from the host.
                 */
                sec_params.lesc = 1;
                sec_params.oob  = 0;
            }
            else
#endif
#if CONFIG_SEC_LEGACY_PAIRING
            {
                // Legacy Paring: OOB flag has to be set if we are providing TK over NFC.
                sec_params.lesc = 0;
                sec_params.oob  = (CONFIG_NFC_ENABLED && CONFIG_NFC_PAIRING_TAG);
            }
#else /* !CONFIG_SEC_LEGACY_PAIRING */
            {
                // Reject Legacy pairing if it is not allowed.
                APP_ERROR_CHECK(pm_conn_sec_params_reply(p_evt->conn_handle,
                                                         NULL,
                                                         p_evt->params.conn_sec_params_req.p_context));
                break;
            }
#endif /* CONFIG_SEC_LEGACY_PAIRING */

            // Enable MITM protection if it could be achieved through OOB or passkey.
            sec_params.mitm = sec_params.oob || (sec_params.io_caps != BLE_GAP_IO_CAPS_NONE);

            NRF_LOG_DEBUG("pm_conn_sec_params_reply(lesc=%d, mitm=%d, oob=%d, iocaps=%d)",
                          sec_params.lesc,
                          sec_params.mitm,
                          sec_params.oob,
                          sec_params.io_caps);

            APP_ERROR_CHECK(pm_conn_sec_params_reply(p_evt->conn_handle,
                                                     &sec_params,
                                                     p_evt->params.conn_sec_params_req.p_context));
            break;

        case PM_EVT_STORAGE_FULL:
            NRF_LOG_WARNING("PM_EVT_STORAGE_FULL: %d", p_evt->peer_id);

            // Run garbage collection on the flash.
            switch(fds_gc())
            {
                case FDS_SUCCESS:
                case FDS_ERR_BUSY:
                case FDS_ERR_NO_SPACE_IN_QUEUES:
                    /* Ignore */
                    break;

                default:
                    APP_ERROR_CHECK_BOOL(false);
                    break;
            }
            break;

        case PM_EVT_ERROR_UNEXPECTED:
            NRF_LOG_ERROR("PM_EVT_ERROR_UNEXPECTED: %d", p_evt->peer_id);
            APP_ERROR_CHECK(p_evt->params.error_unexpected.error);
            break;

        case PM_EVT_PEER_DATA_UPDATE_SUCCEEDED:
            NRF_LOG_DEBUG("PM_EVT_PEER_DATA_UPDATE_SUCCEEDED: %d (Data ID: %d)",
                      p_evt->peer_id,
                      p_evt->params.peer_data_update_succeeded.data_id);
            break;

        case PM_EVT_PEER_DATA_UPDATE_FAILED:
            NRF_LOG_ERROR("PM_EVT_PEER_DATA_UPDATE_FAILED: %d (Data ID: %d)",
                p_evt->peer_id,
                p_evt->params.peer_data_update_failed.data_id);
            APP_ERROR_CHECK(p_evt->params.peer_data_update_failed.error);
            break;

        case PM_EVT_PEER_DELETE_SUCCEEDED:
            NRF_LOG_DEBUG("PM_EVT_PEER_DELETE_SUCCEEDED: %d", p_evt->peer_id);
            break;

        case PM_EVT_PEER_DELETE_FAILED:
            NRF_LOG_ERROR("PM_EVT_PEER_DELETE_FAILED: %d", p_evt->peer_id);
            APP_ERROR_CHECK(p_evt->params.peer_delete_failed.error);
            break;

        case PM_EVT_PEERS_DELETE_SUCCEEDED:
            NRF_LOG_DEBUG("PM_EVT_PEERS_DELETE_SUCCEEDED: %d", p_evt->peer_id);
            break;

        case PM_EVT_PEERS_DELETE_FAILED:
            NRF_LOG_ERROR("PM_EVT_PEERS_DELETE_FAILED: %d", p_evt->peer_id);
            APP_ERROR_CHECK(p_evt->params.peers_delete_failed_evt.error);
            break;

        case PM_EVT_LOCAL_DB_CACHE_APPLIED:
            NRF_LOG_DEBUG("PM_EVT_LOCAL_DB_CACHE_APPLIED: %d", p_evt->peer_id);
            break;

        case PM_EVT_LOCAL_DB_CACHE_APPLY_FAILED:
            NRF_LOG_WARNING("PM_EVT_LOCAL_DB_CACHE_APPLY_FAILED: %d", p_evt->peer_id);

            // The local database has likely changed, send service changed indications.
            pm_local_database_has_changed();
            break;

        case PM_EVT_SERVICE_CHANGED_IND_SENT:
            NRF_LOG_INFO("PM_EVT_SERVICE_CHANGED_IND_SENT: %d", p_evt->peer_id);
            break;

        case PM_EVT_SERVICE_CHANGED_IND_CONFIRMED:
            NRF_LOG_INFO("PM_EVT_SERVICE_CHANGED_IND_CONFIRMED: %d", p_evt->peer_id);
            break;

        case PM_EVT_FLASH_GARBAGE_COLLECTED:
            NRF_LOG_INFO("PM_EVT_FLASH_GARBAGE_COLLECTED: %d", p_evt->peer_id);
            break;

        default:
            NRF_LOG_WARNING("UNKNOWN Peer Manager Event %d", p_evt->evt_id);
            break;
    }
}

/**@brief Function for handling the HID events.
 *
 * @param[in]   p_evt     HID event structure.
 */
static void ble_hids_evt_handler(m_coms_ble_hid_evt_t * p_evt)
{
    m_coms_ble_evt_t coms_evt;

    switch (p_evt->hids_evt->evt_type)
    {
        case BLE_HIDS_EVT_HOST_SUSP:
        case BLE_HIDS_EVT_HOST_EXIT_SUSP:
        case BLE_HIDS_EVT_NOTIF_ENABLED:
        case BLE_HIDS_EVT_NOTIF_DISABLED:
            /* Ignore */
            break;

        case BLE_HIDS_EVT_REP_CHAR_WRITE:
            APP_ERROR_CHECK_BOOL(p_evt->data != 0);

            coms_evt.type                             = M_COMS_BLE_EVT_DATA_RECEIVED;
            coms_evt.data.data_received.interface_idx = p_evt->interface_idx;
            coms_evt.data.data_received.report_type   = p_evt->report_type;
            coms_evt.data.data_received.report_idx    = p_evt->report_idx;
            coms_evt.data.data_received.len           = p_evt->len;
            coms_evt.data.data_received.data          = p_evt->data;
            sp_ble_params->event_handler(&coms_evt, sizeof(coms_evt));
            break;

        case BLE_HIDS_EVT_BOOT_MODE_ENTERED:
            s_boot_mode_active = true;
            break;

        case BLE_HIDS_EVT_REPORT_MODE_ENTERED:
            s_boot_mode_active = false;
            break;

        case BLE_HIDS_EVT_REPORT_READ:
            coms_evt.type                        = M_COMS_BLE_EVT_READ_REQ;
            coms_evt.data.read_req.interface_idx = p_evt->interface_idx;
            coms_evt.data.read_req.report_idx    = p_evt->report_idx;
            sp_ble_params->event_handler(&coms_evt, sizeof(coms_evt));
            break;

        default:
            // Unknown Event.
            APP_ERROR_CHECK_BOOL(false);
            break;
    }
}

/**@brief Function for handling the HID errors.
 *
 * @param[in]   nrf_error Error code.
 */
static void ble_hids_error_handler(uint32_t nrf_error)
{
    if (nrf_error != NRF_ERROR_INVALID_PARAM)
    {
        APP_ERROR_CHECK(nrf_error);
    }
    else
    {
        // Ignore.
    }
}

/**@brief Function for handling the Application's system events.
 *
 * @param[in]   sys_evt   system event.
 */
static void m_coms_ble_on_sys_evt(uint32_t sys_evt, void *p_context)
{
    switch(sys_evt)
    {
        case NRF_EVT_FLASH_OPERATION_ERROR:
        case NRF_EVT_FLASH_OPERATION_SUCCESS:
            if (s_waiting_for_flash)
            {
                if (!nrf_fstorage_is_busy(NULL))
                {
                    NRF_LOG_DEBUG("Flash Ready.");

                    if (s_waiting_reason & WAITING_REASON_SHUTDOWN)
                    {
                        s_waiting_reason &= ~(WAITING_REASON_SHUTDOWN);
                        APP_ERROR_CHECK(m_coms_ble_shutdown());
                    }

                    s_waiting_for_flash = false;
                }
            }
            break;

        default:
            /* Ignore */
            break;
    }
}

NRF_SDH_SOC_OBSERVER(m_coms_ble_soc_observer, SOC_OBSERVER_PRIORITY_LOW, m_coms_ble_on_sys_evt, NULL);

/**@brief Connection Parameters module error handler.
 *
 * @param[in] nrf_error Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_CHECK(nrf_error);
}

/**@brief Function for initializing the Peer Manager.
 */
static ret_code_t peer_manager_init(const m_coms_ble_params_t *p_ble_params, bool delete_bonds)
{
    ret_code_t status;

    ble_gap_sec_params_t sec_params =
    {
        .bond           = CONFIG_SEC_BOND,
        .lesc           = CONFIG_SEC_LESC_PAIRING,
        .io_caps        = p_ble_params->sec_params.io_capabilities,
        .mitm           = (p_ble_params->sec_params.io_capabilities != BLE_GAP_IO_CAPS_NONE),
        .min_key_size   = CONFIG_SEC_MIN_KEY_SIZE,
        .max_key_size   = CONFIG_SEC_MAX_KEY_SIZE,
        .kdist_own.enc  = CONFIG_SEC_BOND,
        .kdist_own.id   = CONFIG_SEC_BOND,
        .kdist_peer.enc = CONFIG_SEC_BOND,
        .kdist_peer.id  = CONFIG_SEC_BOND,
    };

    // Initialize peer manager.
    status = pm_init();
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    // Register event handler.
    status = pm_register(peer_manager_evt_handler);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    // Set default security parameters.
    status = pm_sec_params_set(&sec_params);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    // Clear all bonded Centrals if the "delete all bonds" button is pushed.
    if (delete_bonds)
    {
        NRF_LOG_DEBUG("Deleting all bonds!");

        status = m_coms_ble_bond_clear();
        if (status != NRF_SUCCESS)
        {
            return status;
        }
    }

    return NRF_SUCCESS;
}

/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static uint32_t ble_stack_init(void)
{
    ble_cfg_t ble_cfg;
    ble_opt_t ble_opt;
    ret_code_t err_code;
    uint32_t ram_start;

    // Enable the SoftDevice.
    err_code = nrf_sdh_enable_request();
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Fetch the start address of the application RAM.
    err_code = nrf_sdh_ble_app_ram_start_get(&ram_start);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Prepare space for custom UUIDs.
    memset(&ble_cfg, 0, sizeof(ble_cfg));
    ble_cfg.common_cfg.vs_uuid_cfg.vs_uuid_count = NRF_SDH_BLE_VS_UUID_COUNT;

    err_code = sd_ble_cfg_set(BLE_COMMON_CFG_VS_UUID, &ble_cfg, ram_start);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Configure the ATT database size.
    memset(&ble_cfg, 0, sizeof(ble_cfg));
    ble_cfg.gatts_cfg.attr_tab_size.attr_tab_size = NRF_SDH_BLE_GATTS_ATTR_TAB_SIZE;

    err_code = sd_ble_cfg_set(BLE_GATTS_CFG_ATTR_TAB_SIZE, &ble_cfg, ram_start);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Configure Service Changed Characteristic
    memset(&ble_cfg, 0, sizeof(ble_cfg));
    ble_cfg.gatts_cfg.service_changed.service_changed = NRF_SDH_BLE_SERVICE_CHANGED;

    err_code = sd_ble_cfg_set(BLE_GATTS_CFG_SERVICE_CHANGED, &ble_cfg, ram_start);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Configure the maximum number of connections.
    memset(&ble_cfg, 0, sizeof(ble_cfg));
    ble_cfg.gap_cfg.role_count_cfg.periph_role_count  = NRF_SDH_BLE_PERIPHERAL_LINK_COUNT;
#if !defined(S112)
    ble_cfg.gap_cfg.role_count_cfg.central_role_count = NRF_SDH_BLE_CENTRAL_LINK_COUNT;
    ble_cfg.gap_cfg.role_count_cfg.central_sec_count  = 0;
#endif

    err_code = sd_ble_cfg_set(BLE_GAP_CFG_ROLE_COUNT, &ble_cfg, ram_start);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Configure the maximum MTU.
    memset(&ble_cfg, 0x00, sizeof(ble_cfg));
    ble_cfg.conn_cfg.conn_cfg_tag                   = CONFIG_CONN_CFG_HIGH_BW_TAG;
    ble_cfg.conn_cfg.params.gatt_conn_cfg.att_mtu   = NRF_SDH_BLE_GATT_MAX_MTU_SIZE;

    err_code = sd_ble_cfg_set(BLE_CONN_CFG_GATT, &ble_cfg, ram_start);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Configure the Notification queue length.
    memset(&ble_cfg, 0x00, sizeof(ble_cfg));
    ble_cfg.conn_cfg.conn_cfg_tag                               = CONFIG_CONN_CFG_HIGH_BW_TAG;
    ble_cfg.conn_cfg.params.gatts_conn_cfg.hvn_tx_queue_size    = CONFIG_GATTS_CONN_HVN_TX_QUEUE_SIZE;

    err_code = sd_ble_cfg_set(BLE_CONN_CFG_GATTS, &ble_cfg, ram_start);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Configure the Write Command queue length.
    memset(&ble_cfg, 0x00, sizeof(ble_cfg));
    ble_cfg.conn_cfg.conn_cfg_tag                                   = CONFIG_CONN_CFG_HIGH_BW_TAG;
    ble_cfg.conn_cfg.params.gattc_conn_cfg.write_cmd_tx_queue_size  = BLE_GATTC_WRITE_CMD_TX_QUEUE_SIZE_DEFAULT;

    err_code = sd_ble_cfg_set(BLE_CONN_CFG_GATTC, &ble_cfg, ram_start);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Configure the maximum event length.
    memset(&ble_cfg, 0x00, sizeof(ble_cfg));
    ble_cfg.conn_cfg.conn_cfg_tag                       = CONFIG_CONN_CFG_HIGH_BW_TAG;
    ble_cfg.conn_cfg.params.gap_conn_cfg.conn_count     = NRF_SDH_BLE_PERIPHERAL_LINK_COUNT;
    ble_cfg.conn_cfg.params.gap_conn_cfg.event_length   = (CONFIG_MIN_CONN_INTERVAL / NRF_SDH_BLE_PERIPHERAL_LINK_COUNT);

    err_code = sd_ble_cfg_set(BLE_CONN_CFG_GAP, &ble_cfg, ram_start);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Enable connection event length extension.
    memset(&ble_opt, 0x00, sizeof(ble_opt));
    ble_opt.common_opt.conn_evt_ext.enable = true;

    err_code = sd_ble_opt_set(BLE_COMMON_OPT_CONN_EVT_EXT, &ble_opt);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

#if CONFIG_PA_LNA_ENABLED
# if !IS_IO_VALID(CONFIG_IO_PA) && !IS_IO_VALID(CONFIG_IO_LNA)
#  error The PA/LNA interface is enabled but no PA or LNA pin isconfigured!
# endif
    {
        ble_opt_t                   ble_opt_pa_lna;
        nrf_ppi_channel_t           ppi_chn1, ppi_chn2;
        uint8_t                     gpiote_chn;
        nrf_drv_gpiote_out_config_t gpiote_config =
        {
            .action     = NRF_GPIOTE_POLARITY_TOGGLE,
            .init_state = NRF_GPIOTE_INITIAL_VALUE_LOW,
            .task_pin   = IS_IO_VALID(CONFIG_IO_PA) ? CONFIG_IO_PA : CONFIG_IO_LNA
        };

        // Allocate GPIOTE channel
        // Lacking a dedicated channel allocation function: do full config as a workaround
        err_code = nrf_drv_gpiote_out_init(gpiote_config.task_pin, &gpiote_config);
        if (err_code != NRF_SUCCESS)
        {
            return err_code;
        }

        nrf_drv_gpiote_out_task_disable(gpiote_config.task_pin);
        gpiote_chn = (nrf_drv_gpiote_out_task_addr_get(gpiote_config.task_pin) - NRF_GPIOTE_TASKS_OUT_0) / sizeof(uint32_t);

        // Allocate PPI channels
        err_code = nrf_drv_ppi_channel_alloc(&ppi_chn1);
        if (err_code != NRF_SUCCESS)
        {
            return err_code;
        }

        err_code = nrf_drv_ppi_channel_alloc(&ppi_chn2);
        if (err_code != NRF_SUCCESS)
        {
            return err_code;
        }

        memset(&ble_opt_pa_lna, 0, sizeof(ble_opt_pa_lna));
        ble_opt_pa_lna.common_opt.pa_lna.gpiote_ch_id       = gpiote_chn;
        ble_opt_pa_lna.common_opt.pa_lna.ppi_ch_id_clr      = (uint8_t)ppi_chn1;
        ble_opt_pa_lna.common_opt.pa_lna.ppi_ch_id_set      = (uint8_t)ppi_chn2;

# if IS_IO_VALID(CONFIG_IO_PA)
        ble_opt_pa_lna.common_opt.pa_lna.pa_cfg.enable      = 1;
        ble_opt_pa_lna.common_opt.pa_lna.pa_cfg.gpio_pin    = CONFIG_IO_PA;
        ble_opt_pa_lna.common_opt.pa_lna.pa_cfg.active_high = !CONFIG_PA_ACTIVE_LOW;
# endif
# if IS_IO_VALID(CONFIG_IO_LNA)
        ble_opt_pa_lna.common_opt.pa_lna.lna_cfg.enable      = 1;
        ble_opt_pa_lna.common_opt.pa_lna.lna_cfg.gpio_pin    = CONFIG_IO_LNA;
        ble_opt_pa_lna.common_opt.pa_lna.lna_cfg.active_high = !CONFIG_LNA_ACTIVE_LOW;
# endif

        err_code = sd_ble_opt_set(BLE_COMMON_OPT_PA_LNA, &ble_opt_pa_lna);
        if (err_code != NRF_SUCCESS)
        {
            return err_code;
        }
    }
#endif /* CONFIG_PA_LNA_ENABLED */

    // Set transmit power.
    return sd_power_dcdc_mode_set((CONFIG_DCDC_ENABLED) ? NRF_POWER_DCDC_ENABLE : NRF_POWER_DCDC_DISABLE);
}

/**@brief Disable the BLE stack.
 *
 * @details Disables the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_disable(void * p_evt, uint16_t evt_size)
{
    APP_ERROR_CHECK(nrf_sdh_disable_request());

    if (evt_size == sizeof(m_coms_ble_evt_t))
    {
        sp_ble_params->event_handler(p_evt, evt_size);
    }
}

/**@brief Initialize GAP.
 *
 * @details This function shall be used to set up all the necessary GAP (Generic Access Profile)
 *          parameters of the device. It also sets the permissions and appearance.
 */
static ret_code_t gap_params_init(const m_coms_ble_params_t * p_params)
{
    ble_gap_conn_sec_mode_t sec_mode;
    ret_code_t              status;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    status = sd_ble_gap_tx_power_set(CONFIG_RADIO_TX_POWER);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    status = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *) p_params->device_info.device_name,
                                          strlen(p_params->device_info.device_name));
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    return sd_ble_gap_appearance_set(p_params->appearance);
}

/**@brief GATT event handler.
*/
static void gatt_evt_handler(nrf_ble_gatt_t *p_gatt, nrf_ble_gatt_evt_t const *p_evt)
{
    m_coms_ble_evt_t coms_evt;

    switch (p_evt->evt_id)
    {
        case NRF_BLE_GATT_EVT_ATT_MTU_UPDATED:
            NRF_LOG_INFO("Effective ATT MTU: %u bytes", p_evt->params.att_mtu_effective);

            // Send event to lower layers
            coms_evt.type = M_COMS_BLE_EVT_MTU_CHANGED;
            coms_evt.data.mtu_changed.effective_mtu = p_evt->params.att_mtu_effective;
            sp_ble_params->event_handler(&coms_evt, sizeof(coms_evt));
            break;

#if !defined(S112)
        case NRF_BLE_GATT_EVT_DATA_LENGTH_UPDATED:
            NRF_LOG_INFO("Data Length: %u bytes", p_evt->params.data_length);
            break;
#endif

        default:
            NRF_LOG_WARNING("Unknown NRF_BLE_GATT event!");
            break;
    }
}

static ret_code_t gatt_init()
{
    ret_code_t status;

    status = nrf_ble_gatt_init(&s_gatt, gatt_evt_handler);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    return nrf_ble_gatt_att_mtu_periph_set(&s_gatt, NRF_SDH_BLE_GATT_MAX_MTU_SIZE);
}

NRF_SDH_BLE_OBSERVER(m_coms_ble_gatt_observer, NRF_BLE_GATT_BLE_OBSERVER_PRIO, nrf_ble_gatt_on_ble_evt, &s_gatt);

/**@brief Initialize the Connection Parameters module.
 */
static ret_code_t conn_params_init(const m_coms_ble_params_t *p_params)
{
    ble_gap_conn_params_t  conn_params = p_params->conn_params;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = &conn_params;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = NULL;
    cp_init.error_handler                  = conn_params_error_handler;

    return ble_conn_params_init(&cp_init);
}

/**@brief Initialize Device Information Service.
 */
static ret_code_t dis_init(const m_coms_ble_params_t * p_params)
{
    ble_dis_init_t   dis_init_obj;
    ble_dis_pnp_id_t pnp_id;

    pnp_id.vendor_id_source = p_params->device_info.pnp_vendor_id_src;
    pnp_id.vendor_id        = p_params->device_info.pnp_vendor_id;
    pnp_id.product_id       = p_params->device_info.pnp_product_id;
    pnp_id.product_version  = p_params->device_info.pnp_product_version;

    memset(&dis_init_obj, 0, sizeof(dis_init_obj));

    // Converting null-terminated strings to structs
    if (p_params->device_info.manufacturer_name != 0)
    {
        ble_srv_ascii_to_utf8(&dis_init_obj.manufact_name_str, (char *) p_params->device_info.manufacturer_name);
    }
    if (p_params->device_info.hw_revision != 0)
    {
        ble_srv_ascii_to_utf8(&dis_init_obj.hw_rev_str, (char *) p_params->device_info.hw_revision);
    }
    if (p_params->device_info.fw_revision != 0)
    {
        ble_srv_ascii_to_utf8(&dis_init_obj.fw_rev_str, (char *) p_params->device_info.fw_revision);
    }
    if (p_params->device_info.serial_number != 0)
    {
        ble_srv_ascii_to_utf8(&dis_init_obj.serial_num_str, (char *) p_params->device_info.serial_number);
    }

    dis_init_obj.p_pnp_id = &pnp_id;

    // Device Information Service can be read without encryption.
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&dis_init_obj.dis_attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&dis_init_obj.dis_attr_md.write_perm);

    return ble_dis_init(&dis_init_obj);
}

#if CONFIG_BATT_MEAS_ENABLED
/**@brief Initialize the Battery Service.
 */
static ret_code_t bas_init(const m_coms_ble_params_t * p_params)
{
    ble_bas_init_t bas_init_obj;

    memset(&bas_init_obj, 0, sizeof(bas_init_obj));

    bas_init_obj.evt_handler          = 0;
    bas_init_obj.support_notification = true;
    bas_init_obj.p_report_ref         = s_bas_report_ref.report_type == 0 ? 0 : &s_bas_report_ref;
    bas_init_obj.initial_batt_level   = 100;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&bas_init_obj.battery_level_char_attr_md.cccd_write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&bas_init_obj.battery_level_char_attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&bas_init_obj.battery_level_char_attr_md.write_perm);

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&bas_init_obj.battery_level_report_read_perm);

    return ble_bas_init(&s_bas, &bas_init_obj);
}

NRF_SDH_BLE_OBSERVER(m_coms_ble_bas_observer, BLE_OBSERVER_PRIORITY_DEFAULT, ble_bas_on_ble_evt, &s_bas);
#endif /* CONFIG_BATT_MEAS_ENABLED */

#if (CONFIG_LED_SIGNAL_IMMEDIATE_ALERT || CONFIG_BUZZER_ENABLED)
/**@brief Immediate Alert Find Me Service Event Handler.
 */
static void ias_evt_handler(ble_ias_t *p_ias, ble_ias_evt_t *p_evt)
{
    m_coms_ble_evt_t coms_evt;

    switch(p_evt->evt_type)
    {
        case BLE_IAS_EVT_ALERT_LEVEL_UPDATED:
            coms_evt.type = M_COMS_BLE_EVT_ALERT;
            coms_evt.data.alert.level = p_evt->params.alert_level;
            coms_evt.data.alert.conn_handle = p_ias->conn_handle;
            sp_ble_params->event_handler(&coms_evt, sizeof(coms_evt));
            break;

        default:
            return;
    }
}

/**@brief Initialize the Immediate Alert Service.
 */
static ret_code_t ias_init(void)
{
    ble_ias_init_t ias_init_params;

    ias_init_params.evt_handler = ias_evt_handler;

    return ble_ias_init(&s_ias, &ias_init_params);
}

NRF_SDH_BLE_OBSERVER(m_coms_ble_ias_observer, BLE_OBSERVER_PRIORITY_DEFAULT, ble_ias_on_ble_evt, &s_ias);
#endif /* CONFIG_LED_SIGNAL_IMMEDIATE_ALERT || CONFIG_BUZZER_ENABLED */

ret_code_t m_coms_ble_init(const m_coms_ble_params_t * p_ble_params,
                           const struct ble_hid_db_s * p_hid_db,
                           bool delete_bonds)
{
    m_coms_ble_hid_init_t   hid_params;
    ret_code_t              status;

    if (p_ble_params == NULL || p_hid_db == NULL)
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    // Initializing static variables
    sp_ble_params        = p_ble_params;
    s_encrypted          = false;
    s_boot_mode_active   = false;
    s_conn_handle        = BLE_CONN_HANDLE_INVALID;

#if CONFIG_BATT_MEAS_ENABLED
    memset(&s_bas_report_ref, 0, sizeof(s_bas_report_ref));
#endif

    s_waiting_for_disconnect = false;
    s_waiting_for_flash      = false;
    s_waiting_reason         = 0;

    status = app_timer_create(&s_encryption_timer, APP_TIMER_MODE_SINGLE_SHOT, on_encrypt_timeout);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    // Create HID configuration.
    hid_params.base_hid_version = sp_ble_params->base_hid_version;
    hid_params.b_country_code   = sp_ble_params->hid_country_code;
    hid_params.flags            = sp_ble_params->hids_flags;
    hid_params.io_capabilities  = sp_ble_params->sec_params.io_capabilities;
    hid_params.db_loc           = p_hid_db;
    hid_params.evt_handler      = ble_hids_evt_handler;
    hid_params.error_handler    = ble_hids_error_handler;

    // Initializing the SoftDevice
    status = ble_stack_init();
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    // Initializing Peer Manager
    status = peer_manager_init(p_ble_params, delete_bonds);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    // Initializing address management module
    status = m_coms_ble_addr_init();
    if (status != NRF_SUCCESS)
    {
        return status;
    }

#if CONFIG_SEC_LESC_PAIRING
    // Initializing LESC Key generation module.
    status = m_coms_ble_lesc_init();
    if (status != NRF_SUCCESS)
    {
        return status;
    }
#endif

    // Initializing GAP parameters
    status = gap_params_init(sp_ble_params);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    // Initializing GATT module.
    status = gatt_init();
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    // Initializing the Device Information Service
    status = dis_init(sp_ble_params);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

#if CONFIG_AUDIO_ATVV_ENABLED
    status = m_coms_ble_atvv_init();
    if (status != NRF_SUCCESS)
    {
        return status;
    }
#endif

#if CONFIG_BATT_MEAS_ENABLED
    // Initializing the Battery Service
    status = bas_init(sp_ble_params);
    if (status != NRF_SUCCESS)
    {
        return status;
    }
#endif

#if CONFIG_CLI_BLE_ENABLED
    status = nrf_cli_ble_uart_service_init();
    if (status != NRF_SUCCESS)
    {
        return status;
    }
#endif /* CONFIG_CLI_BLE_ENABLED */

#if CONFIG_DFU_ENABLED
    // Initializing the DFU Service
    status = m_coms_ble_dfu_init(sp_ble_params);
    if (status != NRF_SUCCESS)
    {
        return status;
    }
#endif

#if (CONFIG_LED_SIGNAL_IMMEDIATE_ALERT || CONFIG_BUZZER_ENABLED)
    // Initializing the Immediate Alert Service
    status = ias_init();
    if (status != NRF_SUCCESS)
    {
        return status;
    }
#endif

    // Initializing HID service
    status = m_coms_ble_hid_init(&hid_params, &s_hid_cccd_handle);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    // Initializing Connection parameter handling
    status = conn_params_init(sp_ble_params);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    // Initializing Advertising.
    status = m_coms_ble_adv_init(sp_ble_params);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    // Start advertising.
    status = m_coms_ble_adv_start(false);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    return NRF_SUCCESS;
}

ret_code_t m_coms_ble_oobkey_set(uint8_t * p_key)
{
    return sd_ble_gap_auth_key_reply(s_conn_handle,
                                     (p_key != NULL) ? BLE_GAP_AUTH_KEY_TYPE_OOB : BLE_GAP_AUTH_KEY_TYPE_NONE,
                                     p_key);
}

ret_code_t m_coms_ble_hid_report_send(uint8_t * p_data,
                                      uint8_t   p_len,
                                      uint8_t   p_hid_interface,
                                      uint8_t   p_report_idx)
{
    ret_code_t status;

    if (!s_encrypted)
    {
        // HoG Profile requires encryption.
        return NRF_ERROR_INVALID_STATE;
    }

    if (s_boot_mode_active)
    {
        static m_coms_hid_boot_pkt_t boot_report;
        m_coms_ble_hid_boot_type_t   boot_report_type = ble_boot_pkt_none;

        // If boot mode is active: use boot callback to allow main application to restructure the report to fit the boot format.
        sp_ble_params->boot_mode_callback(&boot_report, &boot_report_type, p_data, p_len, p_hid_interface, p_report_idx);

        status = NRF_ERROR_INVALID_STATE;

        if (boot_report_type & ble_boot_pkt_keyboard)
        {
            status = m_coms_ble_hid_keyboard_boot_report_send(&boot_report);
        }
        if (boot_report_type & ble_boot_pkt_mouse)
        {
            status = m_coms_ble_hid_mouse_boot_report_send(&boot_report);
        }
    }
    else
    {
        status = m_coms_ble_hid_input_report_send(p_hid_interface, p_report_idx, p_data, p_len);
    }

    return status;
}

ret_code_t m_coms_ble_read_respond(uint8_t * p_data, uint8_t p_len)
{
    ble_gatts_rw_authorize_reply_params_t reply_params;

    if (p_data == 0 || p_len == 0)
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    reply_params.params.read.gatt_status = BLE_GATT_STATUS_SUCCESS;
    reply_params.params.read.len         = p_len;
    reply_params.params.read.offset      = 0;
    reply_params.params.read.p_data      = p_data;
    reply_params.params.read.update      = 1;
    reply_params.type                    = BLE_GATTS_AUTHORIZE_TYPE_READ;

    return sd_ble_gatts_rw_authorize_reply(s_conn_handle, &reply_params);
}

ret_code_t m_coms_ble_battery_level_update(uint8_t p_batt_level)
{
#if CONFIG_BATT_MEAS_ENABLED
    return ble_bas_battery_level_update(&s_bas, p_batt_level);
#else
    return NRF_ERROR_NOT_SUPPORTED;
#endif
}

ret_code_t m_coms_ble_advertise(bool bondable)
{
    if (sp_ble_params == NULL)
    {
        return NRF_ERROR_INVALID_STATE;
    }

    return m_coms_ble_adv_start(bondable);
}

ret_code_t m_coms_ble_disconnect(void)
{
    ret_code_t status;

    status = sd_ble_gap_disconnect(s_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    s_conn_handle = BLE_CONN_HANDLE_INVALID;

    return NRF_SUCCESS;
}

ret_code_t m_coms_ble_shutdown(void)
{
    ret_code_t status;

    // Stop advertising if there is any.
    status = m_coms_ble_adv_stop();
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    // Connected?
    if (s_conn_handle != BLE_CONN_HANDLE_INVALID)
    {
        // Disconnect.
        status = sd_ble_gap_disconnect(s_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        if (status != NRF_SUCCESS)
        {
            return status;
        }

        s_conn_handle = BLE_CONN_HANDLE_INVALID;

        NRF_LOG_DEBUG("Waiting for Disconnect...");
        s_waiting_for_disconnect = true;
        s_waiting_reason |= WAITING_REASON_SHUTDOWN;

        return NRF_SUCCESS;
    }

    if (nrf_fstorage_is_busy(NULL))
    {
        NRF_LOG_DEBUG("Waiting for Flash...");
        s_waiting_for_flash = true;
        s_waiting_reason |= WAITING_REASON_SHUTDOWN;

        return NRF_SUCCESS;
    }

    m_coms_ble_evt_t coms_evt;
    coms_evt.type = M_COMS_BLE_EVT_SHUTDOWN;
    return app_sched_event_put(&coms_evt, sizeof(coms_evt), ble_stack_disable);
}

bool m_coms_ble_bond_stored(void)
{
    return (pm_peer_count() != 0);
}

ret_code_t m_coms_ble_bond_clear(void)
{
    return pm_peers_delete();
}

ret_code_t m_coms_ble_sl_disable(void)
{
    ret_code_t  status;
    ble_opt_t   ble_opt;

    if (nrf_atomic_u32_fetch_add(&s_sl_disabled_ctr, 1) == 0)
    {
        // Slave latency was enabled, turn it off
        ble_opt.gap_opt.slave_latency_disable.disable = 1;
        ble_opt.gap_opt.slave_latency_disable.conn_handle = s_conn_handle;
        status = sd_ble_opt_set(BLE_GAP_OPT_SLAVE_LATENCY_DISABLE, &ble_opt);
    }
    else
    {
        return NRF_SUCCESS;
    }

    if (status != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("Unable to set slave latency, Error: %u", status);
    }
    else
    {
        NRF_LOG_INFO("Slave latency disabled");
    }

    return status;
}

ret_code_t m_coms_ble_sl_enable(void)
{
    ret_code_t  status;
    ble_opt_t   ble_opt;
    uint32_t    sl_disabled_ctr_tmp;

    sl_disabled_ctr_tmp = nrf_atomic_u32_sub(&s_sl_disabled_ctr, 1);
    switch(sl_disabled_ctr_tmp)
    {
        case 0:
            // Slave latency was disabled, turn it on
            ble_opt.gap_opt.slave_latency_disable.disable = 0;
            ble_opt.gap_opt.slave_latency_disable.conn_handle = s_conn_handle;
            status = sd_ble_opt_set(BLE_GAP_OPT_SLAVE_LATENCY_DISABLE, &ble_opt);
            break;

        case ~0:
            // Slave latency was already enabled
            NRF_LOG_ERROR("Slave latency already enabled");
            APP_ERROR_CHECK_BOOL(false);
            break;

        default:
            return NRF_SUCCESS;
    }

    if (status != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("Unable to set slave latency, Error: %u", status);
    }
    else
    {
        NRF_LOG_INFO("Slave latency enabled");
    }

    return status;
}

#if CONFIG_CLI_ENABLED
static void m_coms_ble_sl_enable_cmd(nrf_cli_t const * p_cli, size_t argc, char **argv)
{
    if (nrf_cli_help_requested(p_cli))
    {
        nrf_cli_help_print(p_cli, NULL, 0);
        nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Usage:\r\n  %s\r\n", argv[0]);
        return;
    }

    if (s_cli_sl_disabled)
    {
        ret_code_t status;

        status = m_coms_ble_sl_enable();
        if (status != NRF_SUCCESS)
        {
            nrf_cli_fprintf(p_cli,
                            NRF_CLI_ERROR,
                            "Unable to enable slave latency. Error: %u\r\n",
                            status);
        }
        else
        {
            s_cli_sl_disabled = false;
        }
    }
    else
    {
        nrf_cli_fprintf(p_cli,
                        NRF_CLI_ERROR,
                        "Slave latency already enabled!\r\n");
    }
}

static void m_coms_ble_sl_disable_cmd(nrf_cli_t const * p_cli, size_t argc, char **argv)
{
    if (nrf_cli_help_requested(p_cli))
    {
        nrf_cli_help_print(p_cli, NULL, 0);
        nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Usage:\r\n  %s\r\n", argv[0]);
        return;
    }

    if (!s_cli_sl_disabled)
    {
        ret_code_t status;

        status = m_coms_ble_sl_disable();
        if (status != NRF_SUCCESS)
        {
            nrf_cli_fprintf(p_cli,
                            NRF_CLI_ERROR,
                            "Unable to disable slave latency. Error: %u\r\n",
                            status);
        }
        else
        {
            s_cli_sl_disabled = true;
        }
    }
    else
    {
        nrf_cli_fprintf(p_cli,
                        NRF_CLI_ERROR,
                        "Slave latency already disabled!\r\n");
    }
}

static void m_coms_ble_cmd(nrf_cli_t const * p_cli, size_t argc, char **argv)
{
    if (nrf_cli_help_requested(p_cli))
    {
        nrf_cli_help_print(p_cli, NULL, 0);
        nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Usage:\r\n  %s <subcommand>\r\n", argv[0]);
        return;
    }

    if (argc >= 2)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Unknown subcommand '%s'!\r\n", argv[1]);
    }
    else
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Please specify subcommand!\r\n");
    }

}

NRF_CLI_CREATE_STATIC_SUBCMD_SET(m_coms_ble_sl_subcmds)
{
    NRF_CLI_CMD(enable, NULL, "enable slave latency", m_coms_ble_sl_enable_cmd),
    NRF_CLI_CMD(disable, NULL, "disable slave latency", m_coms_ble_sl_disable_cmd),
    { NULL }
};

NRF_CLI_CREATE_STATIC_SUBCMD_SET(m_coms_ble_subcmds)
{
    NRF_CLI_CMD(sl, &m_coms_ble_sl_subcmds, "configure slave latency", m_coms_ble_cmd),
    { NULL }
};

NRF_CLI_CMD_REGISTER(ble,
                     &m_coms_ble_subcmds,
                     "configure ble connection parameters",
                     m_coms_ble_cmd);
#endif /* CONFIG_CLI_ENABLED */

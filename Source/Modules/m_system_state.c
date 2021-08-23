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

#include <stdlib.h>
#include "nrf_assert.h"
#include "nrf_pwr_mgmt.h"
#include "app_debug.h"
#include "app_timer.h"
#include "key_combo_util.h"

#include "m_acc.h"
#include "m_audio.h"
#include "m_gyro.h"
#include "m_system_state.h"
#include "m_protocol_hid.h"
#include "m_coms.h"
#include "m_coms_ble_atvv.h"

#include "sr3_config.h"

#if defined(CONFIG_BOARD_NRF52832_PCA20023) || \
    defined(CONFIG_BOARD_NRF52832_PCA63519) || \
    defined(CONFIG_BOARD_NRF52832_CUSTOM)   || \
    defined(CONFIG_BOARD_NRF52810_PCA20031) || \
    defined(CONFIG_BOARD_NRF52810_CUSTOM)

#define NRF_LOG_MODULE_NAME m_system_state
#define NRF_LOG_LEVEL NRF_LOG_DEFAULT_LEVEL
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();


#if CONFIG_AUDIO_ENABLED
static m_coms_audio_service_t m_audio_services;
# if CONFIG_AUDIO_ATVV_ENABLED
static uint16_t m_atvv_peer_conn_handle;
static bool     m_atvv_has_disabled_sl;
#  if CONFIG_KBD_ATVV_KEY_CHORDS_ENABLED
static bool     m_atvv_triggered_disconnect_to_pair;
#  endif /* CONFIG_KBD_ATVV_KEY_CHORDS_ENABLED */
# endif /* CONFIG_AUDIO_ATVV_ENABLED */
#endif /* CONFIG_AUDIO_ENABLED */

#if CONFIG_GYRO_ENABLED
static bool     m_gyro_active;                      /**< Gyroscope module state. */
static uint8_t  m_gyro_idle_time;                   /**< Gyroscope idle time. */
static uint8_t  m_gyro_mouse_btn_state;             /**< Mouse button (translated keys) state. */
static uint32_t m_gyro_mouse_btn_timestamp[2];      /**< Timestamp of mouse button presses. */
APP_TIMER_DEF   (m_gyro_timer);                     /**< Gyroscope idle timer. */

static uint8_t  m_keys_pressed;                     /**< Number of currently pressed keys. */
#endif /* CONFIG_GYRO_ENABLED */

static int8_t   m_conn_count;                       /**< Number of currently connected hosts. */

#if (CONFIG_KBD_KEY_COMBO_ENABLED && CONFIG_GYRO_ENABLED)
static void gyro_calibrate(void * p_context)
{
    ret_code_t rcode = m_gyro_calibrate();

    if (rcode == NRF_ERROR_INVALID_STATE)
    {
        NRF_LOG_WARNING("%s(): cannot calibrate gyro now", __func__);
    }
    else
    {
        APP_ERROR_CHECK(rcode);
    }
}

// Register key combinations
KEY_COMBO_MEMBER_REGISTER(CONFIG_GYRO_MODE_KEY_ID);
KEY_COMBO_ONE_KEY_REGISTER(CONFIG_GYRO_MODE_KEY_ID, 3000, gyro_calibrate);
#endif /* (CONFIG_KBD_KEY_COMBO_ENABLED && CONFIG_GYRO_ENABLED) */

#if CONFIG_AUDIO_ENABLED && CONFIG_AUDIO_ATVV_ENABLED && CONFIG_KBD_ATVV_KEY_CHORDS_ENABLED

static void atvv_chord_enter_pairing_mode(void * p_context)
{
    NRF_LOG_DEBUG("atvv_chord_enter_pairing_mode()");
    if (m_conn_count > 0)
    {
        APP_ERROR_CHECK(m_coms_ble_disconnect());
        m_atvv_triggered_disconnect_to_pair = true;
    }
    else
    {
        APP_ERROR_CHECK(m_coms_ble_advertise(true));
    }
}

static void atvv_chord_erase_pairing(void * p_context)
{
    NRF_LOG_DEBUG("atvv_chord_erase_pairing()");
    APP_ERROR_CHECK(m_coms_ble_bond_clear());
}
KEY_COMBO_MEMBER_REGISTER(CONFIG_KBD_ATVV_HOME_KEY_ID);
KEY_COMBO_MEMBER_REGISTER(CONFIG_KBD_ATVV_BACK_KEY_ID);
KEY_COMBO_MEMBER_REGISTER(CONFIG_KBD_ATVV_DPAD_CENTER_KEY_ID);
KEY_COMBO_TWO_KEY_REGISTER(CONFIG_KBD_ATVV_HOME_KEY_ID, CONFIG_KBD_ATVV_BACK_KEY_ID, CONFIG_AUDIO_ATVV_KEY_CHORD_DURATION, atvv_chord_enter_pairing_mode);
KEY_COMBO_THREE_KEY_REGISTER(CONFIG_KBD_ATVV_DPAD_CENTER_KEY_ID, CONFIG_KBD_ATVV_BACK_KEY_ID, CONFIG_KBD_ATVV_HOME_KEY_ID, CONFIG_AUDIO_ATVV_KEY_CHORD_DURATION, atvv_chord_erase_pairing);
#endif /* CONFIG_AUDIO_ENABLED && CONFIG_AUDIO_ATVV_ENABLED && CONFIG_KBD_KEY_COMBO_ENABLED && CONFIG_KBD_ATVV_KEY_CHORDS_ENABLED */

#if CONFIG_AUDIO_ENABLED
/**@brief Enable relevant BLE service and audio module. */
static void m_system_state_audio_service_enable(m_coms_audio_service_t service)
{
    if (m_audio_services & service)
    {
        return; // Already enabled
    }
    m_coms_audio_service_enable(service);

    if (m_audio_services == 0)
    {
        APP_ERROR_CHECK(m_audio_enable());
    }

    m_audio_services |= service;
}

/**@brief Disable relevant BLE service and audio module. */
static void m_system_state_audio_service_disable(m_coms_audio_service_t service)
{
    if ((m_audio_services & service) == 0)
    {
        return; // Already disabled
    }

    m_coms_audio_service_disable(service);

    m_audio_services &= ~service;

    if (m_audio_services == 0)
    {
        APP_ERROR_CHECK(m_audio_disable());
    }
}
#endif /* CONFIG_AUDIO_ENABLED */

#if CONFIG_AUDIO_ENABLED && CONFIG_AUDIO_HID_ENABLED
/**@brief Handle Audio Control Commands. */
static void m_system_state_audio_ctrl(uint8_t command)
{
    switch (command)
    {
        case 0x00:
            m_system_state_audio_service_disable(M_COMS_AUDIO_SERVICE_HID);
            break;

        case 0x01:
            m_system_state_audio_service_enable(M_COMS_AUDIO_SERVICE_HID);
            break;

        default:
            /* Unknown Command: Ignore */
            break;
    }
}
#endif /* CONFIG_AUDIO_ENABLED && CONFIG_AUDIO_HID_ENABLED */

#if CONFIG_AUDIO_ENABLED && CONFIG_AUDIO_ATVV_ENABLED
static void m_system_state_atvv_event_handle(const event_t * p_event)
{
    switch(p_event->atvv.type)
    {
        case EVT_ATVV_STATE_MIC_OPEN:
            if (m_atvv_has_disabled_sl)
            {
                // Slave latency was disabled to be more responsive to MIC_OPEN command
                APP_ERROR_CHECK(m_coms_ble_sl_enable());
                m_atvv_has_disabled_sl = false;
            }
            m_system_state_audio_service_enable(M_COMS_AUDIO_SERVICE_ATVV);
            break;

        case EVT_ATVV_STATE_MIC_CLOSE:
            m_system_state_audio_service_disable(M_COMS_AUDIO_SERVICE_ATVV);
            break;

        case EVT_ATVV_STATE_SEARCH_TIMEOUT:
            if (m_atvv_has_disabled_sl)
            {
                // Slave latency was disabled to be more responsive to MIC_OPEN command
                // But this command was not received within timeout
                APP_ERROR_CHECK(m_coms_ble_sl_enable());
                m_atvv_has_disabled_sl = false;
            }
            break;

        case EVT_ATVV_STATE_ENABLED:
            m_atvv_peer_conn_handle = p_event->atvv.conn_id;
            break;

        case EVT_ATVV_STATE_DISABLED:
            m_atvv_peer_conn_handle = BLE_CONN_HANDLE_INVALID;
            m_system_state_audio_service_disable(M_COMS_AUDIO_SERVICE_ATVV);
            break;

        default:
            break;
    }
}
#endif /* CONFIG_AUDIO_ENABLED && CONFIG_AUDIO_ATVV_ENABLED */

#if CONFIG_GYRO_ENABLED
/**@brief Handle Gyroscope On/Off Button Press. */
static void m_system_state_gyro_toggle(void)
{
    uint32_t timestamp = app_timer_cnt_get();

    switch (m_gyro_active)
    {
        case false:
            m_gyro_idle_time    = 0;
            m_gyro_active       = true;

            APP_ERROR_CHECK(m_gyro_enable());
            APP_ERROR_CHECK(app_timer_start(m_gyro_timer,
                                            APP_TIMER_TICKS(1000),
                                            NULL));
#if CONFIG_ACC_USE_CLICK_DETECTION
            APP_ERROR_CHECK(m_acc_click_detection_enable());
#endif

            if (m_gyro_mouse_btn_state & (1 << 0))
            {
                event_send(EVT_KEY_UP, CONFIG_GYRO_LEFT_CLICK_KEY_ID, m_gyro_mouse_btn_timestamp[0]);
                m_gyro_mouse_btn_timestamp[0] = timestamp;
                event_send(EVT_KEY_DOWN, MOUSE_KEY_ID(0), m_gyro_mouse_btn_timestamp[0]);
            }

            if (m_gyro_mouse_btn_state & (1 << 1))
            {
                event_send(EVT_KEY_UP, CONFIG_GYRO_RIGHT_CLICK_KEY_ID, m_gyro_mouse_btn_timestamp[1]);
                m_gyro_mouse_btn_timestamp[1] = timestamp;
                event_send(EVT_KEY_DOWN, MOUSE_KEY_ID(1), m_gyro_mouse_btn_timestamp[1]);
            }
            break;

        case true:
            m_gyro_active   = false;

#if CONFIG_ACC_USE_CLICK_DETECTION
            APP_ERROR_CHECK(m_acc_click_detection_disable());
#endif
            APP_ERROR_CHECK(app_timer_stop(m_gyro_timer));
            APP_ERROR_CHECK(m_gyro_disable());

            if (m_gyro_mouse_btn_state & (1 << 0))
            {
                event_send(EVT_KEY_UP, MOUSE_KEY_ID(0),  m_gyro_mouse_btn_timestamp[0]);
                m_gyro_mouse_btn_timestamp[0] = timestamp;
                event_send(EVT_KEY_DOWN, CONFIG_GYRO_LEFT_CLICK_KEY_ID, m_gyro_mouse_btn_timestamp[0]);
            }

            if (m_gyro_mouse_btn_state & (1 << 1))
            {
                event_send(EVT_KEY_UP, MOUSE_KEY_ID(1), m_gyro_mouse_btn_timestamp[1]);
                m_gyro_mouse_btn_timestamp[1] = timestamp;
                event_send(EVT_KEY_DOWN, CONFIG_GYRO_RIGHT_CLICK_KEY_ID, m_gyro_mouse_btn_timestamp[1]);
            }
            break;

        default:
            APP_ERROR_CHECK_BOOL(false);
            break;
    }

    APP_ERROR_CHECK(event_send(EVT_SYSTEM_GYRO_STATE, m_gyro_active));
}

/**@brief Gyroscope timer handler. */
static void m_system_state_gyro_timeout(void *p_context)
{
    if (m_gyro_active == false)
    {
        return;
    }

    if (m_gyro_idle_time++ >= CONFIG_GRYO_IDLE_TIMEOUT)
    {
        m_system_state_gyro_toggle();
    }
}
#endif /* CONFIG_GYRO_ENABLED */

ret_code_t m_system_state_init(void)
{
    ret_code_t status = NRF_SUCCESS;

#if CONFIG_GYRO_ENABLED
    m_gyro_active           = false;
    m_gyro_mouse_btn_state  = 0;
    m_keys_pressed          = 0;

    status = app_timer_create(&m_gyro_timer,
                              APP_TIMER_MODE_REPEATED,
                              m_system_state_gyro_timeout);
#endif

#if CONFIG_AUDIO_ENABLED && CONFIG_AUDIO_ATVV_ENABLED
    m_atvv_peer_conn_handle = BLE_CONN_HANDLE_INVALID;
# if CONFIG_KBD_ATVV_KEY_CHORDS_ENABLED
    m_atvv_triggered_disconnect_to_pair = false;
# endif /* CONFIG_KBD_ATVV_KEY_CHORDS_ENABLED */
#endif /* CONFIG_AUDIO_ENABLED && CONFIG_AUDIO_ATVV_ENABLED */

    return status;
}

bool m_system_state_event_handler(const event_t *p_event)
{
    bool retval = false;

    ASSERT(p_event != NULL);

#if CONFIG_PWR_MGMT_ENABLED
    /* Notify power module about user activity */
    switch (EVENT_GROUP(p_event->type))
    {
        case EVT_GROUP_KEY:
        case EVT_GROUP_REL:
            nrf_pwr_mgmt_feed();
            break;

        default:
            // Dispatch particular events.
            switch (p_event->type)
            {
                case EVT_BT_ALERT_LEVEL:
                     nrf_pwr_mgmt_feed();
                     break;

                default:
                    /* Do nothing */
                    break;
            }
            break;
    }
#endif /* CONFIG_PWR_MGMT_ENABLED */

    /* Handle individual events */
    switch (p_event->type)
    {
#if CONFIG_AUDIO_ENABLED && CONFIG_AUDIO_HID_ENABLED
        /* Handle HID Commands */
        case EVT_HID_REPORT_OUTPUT:
            if (p_event->hid.usage == HID_USAGE(0xFF00, 0x00))
            {
                /* Handle Audio Control commands */
                m_system_state_audio_ctrl(p_event->hid.report);
            }
            break;
#endif /* CONFIG_AUDIO_ENABLED && CONFIG_AUDIO_HID_ENABLED*/

        /* Handle User Input */
        case EVT_KEY_DOWN:
            switch (p_event->key.id)
            {
#if CONFIG_GYRO_ENABLED
                case CONFIG_GYRO_MODE_KEY_ID:
                    if (m_keys_pressed == 0)
                    {
                        m_system_state_gyro_toggle();
                    }
                    break;

                case CONFIG_GYRO_LEFT_CLICK_KEY_ID:
                    m_gyro_mouse_btn_state          |= (1 << 0);
                    m_gyro_mouse_btn_timestamp[0]   = p_event->key.down_timestamp;

                    if (m_gyro_active)
                    {
                        /* Reset gyro idle timer */
                        m_gyro_idle_time = 0;

                        /* Replace this event by mouse button press */
                        event_send(EVT_KEY_DOWN, MOUSE_KEY_ID(0), m_gyro_mouse_btn_timestamp[0]);
                        retval = true;
                    }
                    break;

                case CONFIG_GYRO_RIGHT_CLICK_KEY_ID:
                    m_gyro_mouse_btn_state          |= (1 << 1);
                    m_gyro_mouse_btn_timestamp[1]   = p_event->key.down_timestamp;

                    if (m_gyro_active)
                    {
                        /* Reset gyro idle timer */
                        m_gyro_idle_time = 0;

                        /* Replace this event by mouse button press */
                        event_send(EVT_KEY_DOWN, MOUSE_KEY_ID(1), m_gyro_mouse_btn_timestamp[1]);
                        retval = true;
                    }
                    break;
#endif /* CONFIG_GYRO_ENABLED */

#if CONFIG_AUDIO_ENABLED && CONFIG_AUDIO_ATVV_ENABLED
                case CONFIG_KBD_ATVV_START_SEARCH_KEY_ID:
                    if ((m_atvv_peer_conn_handle                               != BLE_CONN_HANDLE_INVALID) && 
                        (m_coms_ble_atvv_start_search(m_atvv_peer_conn_handle) == NRF_SUCCESS))
                    {
                        // Disable slave latency to be more responsive to a MIC_OPEN command from the host
                        APP_ERROR_CHECK(m_coms_ble_sl_disable());
                        m_atvv_has_disabled_sl = true;
                    }
                    break;
#endif /* CONFIG_AUDIO_ENABLED && CONFIG_AUDIO_ATVV_ENABLED */

                default:
                    /* Do nothing */
                    break;
            }

#if CONFIG_GYRO_ENABLED
            if (retval == false)
            {
                m_keys_pressed += 1;
            }
#endif /* CONFIG_GYRO_ENABLED */
            break;

#if CONFIG_GYRO_ENABLED
#if (CONFIG_KBD_HELD_EVENT_INTERVAL_MS != 0)
        case EVT_KEY_HELD:
            switch (p_event->key.id)
            {
                case CONFIG_GYRO_LEFT_CLICK_KEY_ID:
                    if (m_gyro_active)
                    {
                        // Do not propagate a KEY_HELD event in gyro mode.
                        retval = true;
                    }
                    else if (p_event->key.down_timestamp != m_gyro_mouse_btn_timestamp[0])
                    {
                        // Replace event in order to correct the timestamp.
                        event_send(p_event->type, p_event->key.id, m_gyro_mouse_btn_timestamp[0]);
                        retval = true;
                    }
                    break;

                case CONFIG_GYRO_RIGHT_CLICK_KEY_ID:
                    if (m_gyro_active)
                    {
                        // Do not propagate the KEY_HELD event in gyro mode.
                        retval = true;
                    }
                    else if (p_event->key.down_timestamp != m_gyro_mouse_btn_timestamp[1])
                    {
                        // Replace event in order to correct the timestamp.
                        event_send(p_event->type, p_event->key.id, m_gyro_mouse_btn_timestamp[1]);
                        retval = true;
                    }
                    break;

                default:
                    /* Do nothing */
                    break;
            }
            break;
#endif /* CONFIG_KBD_HELD_EVENT_INTERVAL_MS != 0) */

        case EVT_KEY_UP:
            switch (p_event->key.id)
            {
                case CONFIG_GYRO_LEFT_CLICK_KEY_ID:
                    m_gyro_mouse_btn_state &= ~(1 << 0);
                    if (m_gyro_active)
                    {
                        /* Reset gyro idle timer */
                        m_gyro_idle_time = 0;

                        /* Replace this event by mouse button release */
                        event_send(EVT_KEY_UP, MOUSE_KEY_ID(0), m_gyro_mouse_btn_timestamp[0]);
                        retval = true;
                    }
                    else if (p_event->key.down_timestamp != m_gyro_mouse_btn_timestamp[0])
                    {
                        // Replace event in order to correct timestamp.
                        event_send(p_event->type, p_event->key.id, m_gyro_mouse_btn_timestamp[0]);
                        retval = true;
                    }
                    break;

                case CONFIG_GYRO_RIGHT_CLICK_KEY_ID:
                    m_gyro_mouse_btn_state &= ~(1 << 1);
                    if (m_gyro_active)
                    {
                        /* Reset gyro idle timer */
                        m_gyro_idle_time = 0;

                        /* Replace this event by mouse button release */
                        event_send(EVT_KEY_UP, MOUSE_KEY_ID(1), m_gyro_mouse_btn_timestamp[1]);
                        retval = true;
                    }
                    else if (p_event->key.down_timestamp != m_gyro_mouse_btn_timestamp[1])
                    {
                        // Replace event in order to correct the timestamp.
                        event_send(p_event->type, p_event->key.id, m_gyro_mouse_btn_timestamp[1]);
                        retval = true;
                    }
                    break;

                default:
                    /* Do nothing */
                    break;
            }

            if (retval == false)
            {
               m_keys_pressed -= 1;
            }
            break;

        case EVT_REL_X:
        case EVT_REL_Y:
            m_gyro_idle_time = 0;
            break;
#endif /* CONFIG_GYRO_ENABLED */

#if (CONFIG_BATT_MEAS_ENABLED && CONFIG_PWR_MGMT_ENABLED)
        /* Handle low battery level */
        case EVT_SYSTEM_BATTERY_LEVEL:
            if (p_event->system.data == 0)
            {
                nrf_pwr_mgmt_shutdown(NRF_PWR_MGMT_SHUTDOWN_STAY_IN_SYSOFF);
            }
            break;
#endif /* (CONFIG_BATT_MEAS_ENABLED && CONFIG_PWR_MGMT_ENABLED) */

        /* Handle connection loss */
        case EVT_BT_CONN_STATE:
            if (p_event->bt.data == BT_CONN_STATE_DISCONNECTED)
            {
                m_conn_count -= 1;

                if (m_conn_count == 0)
                {
#if CONFIG_AUDIO_ATVV_ENABLED && CONFIG_AUDIO_ENABLED && CONFIG_KBD_ATVV_KEY_CHORDS_ENABLED
                    if (m_coms_ble_bond_stored() && !m_atvv_triggered_disconnect_to_pair)
#else
                    if (m_coms_ble_bond_stored())
#endif /* CONFIG_AUDIO_ATVV_ENABLED && CONFIG_AUDIO_ENABLED && CONFIG_KBD_ATVV_KEY_CHORDS_ENABLED */
                    {
#if CONFIG_PWR_MGMT_ENABLED
                        nrf_pwr_mgmt_shutdown(NRF_PWR_MGMT_SHUTDOWN_GOTO_SYSOFF);
#else
                        APP_ERROR_CHECK(m_coms_ble_advertise(false));
#endif
                    }
                    else
                    {
                        APP_ERROR_CHECK(m_coms_ble_advertise(true));
                    }
                }
            }
            else if (p_event->bt.data == BT_CONN_STATE_CONNECTED)
            {
                m_conn_count += 1;
#if CONFIG_AUDIO_ATVV_ENABLED && CONFIG_AUDIO_ENABLED && CONFIG_KBD_ATVV_KEY_CHORDS_ENABLED
                m_atvv_triggered_disconnect_to_pair = false;
#endif /* CONFIG_AUDIO_ATVV_ENABLED && CONFIG_AUDIO_ENABLED && CONFIG_KBD_ATVV_KEY_CHORDS_ENABLED */
            }

            APP_ERROR_CHECK_BOOL(m_conn_count >= 0);
            break;

        case EVT_BT_ADV_STATE:
            if ((p_event->bt.data == BT_ADV_STATE_IDLE) && (m_conn_count == 0))
            {
#if CONFIG_PWR_MGMT_ENABLED
                nrf_pwr_mgmt_shutdown(NRF_PWR_MGMT_SHUTDOWN_GOTO_SYSOFF);
#else
                APP_ERROR_CHECK(m_coms_ble_advertise((m_coms_ble_bond_stored() == false)));
#endif
            }
            break;

#if CONFIG_AUDIO_ATVV_ENABLED && CONFIG_AUDIO_ENABLED
        case EVT_ATVV_STATE:
            m_system_state_atvv_event_handle(p_event);
            break;
#endif /* CONFIG_AUDIO_ATVV_ENABLED && CONFIG_AUDIO_ENABLED */

        default:
            /* Ignore */
            break;
    }

    return retval;
}
#endif

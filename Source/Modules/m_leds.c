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
#include <stdint.h>
#include <stdbool.h>

#include "nrf_delay.h"
#include "nrf_error.h"
#include "nrf_pwr_mgmt.h"
#include "app_timer.h"

#include "drv_leds.h"
#include "m_leds.h"

#include "resources.h"
#include "sr3_config.h"

#if CONFIG_LED_ENABLED

#define NRF_LOG_MODULE_NAME m_leds
#define NRF_LOG_LEVEL CONFIG_LED_MODULE_LOG_LEVEL
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

#define MAX_LEDS        4

typedef enum
{
    LED_STATE_OFF,
    LED_STATE_BLINKING,
    LED_STATE_BLINKING_FOREVER,
    LED_STATE_ON,
} led_state_t;

typedef struct
{
    led_state_t     state;
    uint8_t         state_data;

    app_timer_id_t  timer;
    app_timer_t     timer_data;
} led_entry_t;

static led_entry_t      m_leds[MAX_LEDS];
#if CONFIG_PWR_MGMT_ENABLED
static bool             m_leds_going_down;
#endif

static void m_leds_state_machine(void* p_context)
{
    unsigned int led    = (unsigned int)(p_context);
    led_entry_t *p_led  = &m_leds[led];

    switch (p_led->state)
    {
        case LED_STATE_OFF:
            APP_ERROR_CHECK(drv_leds_clr(1 << led));
            break;

        case LED_STATE_ON:
            APP_ERROR_CHECK(drv_leds_set(1 << led));
            break;

        case LED_STATE_BLINKING_FOREVER:
            p_led->state_data |= ~1;
            /* FALLTHROUGH */

        case LED_STATE_BLINKING:
            if (--(p_led->state_data) & 1)
            {
                APP_ERROR_CHECK(drv_leds_set(1 << led));
            }
            else
            {
                APP_ERROR_CHECK(drv_leds_clr(1 << led));
            }

            if (p_led->state_data != 0)
            {
                APP_ERROR_CHECK(app_timer_start(p_led->timer, APP_TIMER_TICKS(CONFIG_LED_FLIP_INTERVAL), (void *)(led)));
            }
            else
            {
                p_led->state = LED_STATE_OFF;

#if CONFIG_PWR_MGMT_ENABLED
                if (m_leds_going_down)
                {
                    nrf_pwr_mgmt_shutdown(NRF_PWR_MGMT_SHUTDOWN_CONTINUE);
                }
#endif /* CONFIG_PWR_MGMT_ENABLED */
            }

            break;
    }
}

ret_code_t m_leds_init(void)
{
    ret_code_t status;
    unsigned int i;

    // Initialize the LED driver
    status = drv_leds_init();
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    // Clear all LEDs
    status = drv_leds_clr(drv_leds_all());
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    // Initialize module state
    memset(m_leds, 0, sizeof(m_leds));
    for (i = 0; i < MAX_LEDS; i++)
    {
        led_entry_t *p_led = &m_leds[i];

        p_led->state = LED_STATE_OFF;
        p_led->timer = &(p_led->timer_data);

        status = app_timer_create(&(p_led->timer), APP_TIMER_MODE_SINGLE_SHOT, m_leds_state_machine);
        if (status != NRF_SUCCESS)
        {
            return status;
        }
    }

#if CONFIG_PWR_MGMT_ENABLED
    m_leds_going_down = false;
#endif

    return NRF_SUCCESS;
}

/**@brief Turn on LEDs.
 *
 * @param[in] leds_mask Indicates which LEDs to turn on.
 *
 * @return    NRF_SUCCESS on success, otherwise an error code.
 */
static ret_code_t m_leds_set(uint8_t leds_mask)
{
    unsigned int i;

    // Check if all requested LEDs are suppoted.
    if (leds_mask & ~drv_leds_all())
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    for (i = 0; (leds_mask != 0) && (i < MAX_LEDS); i++)
    {
        led_entry_t *p_led = &m_leds[i];

        if (leds_mask & (1 << i))
        {
            p_led->state = LED_STATE_ON;
            m_leds_state_machine((void *)(i));
        }
    }

    return NRF_SUCCESS;
}

/**@brief Turn off LEDs.
 *
 * @param[in] leds_mask Indicates which LEDs to turn off.
 *
 * @return    NRF_SUCCESS on success, otherwise an error code.
 */
static ret_code_t m_leds_clr(uint8_t leds_mask)
{
    unsigned int i;

    // Check if all requested LEDs are suppoted.
    if (leds_mask & ~drv_leds_all())
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    for (i = 0; (leds_mask != 0) && (i < MAX_LEDS); i++)
    {
        led_entry_t *p_led = &m_leds[i];

        if (leds_mask & (1 << i))
        {
            p_led->state = LED_STATE_OFF;
            m_leds_state_machine((void *)(i));
        }
    }

    return NRF_SUCCESS;
}

/**@brief Turn flashing on an LED on or off.
 *
 * @param[in] leds_mask Indicates which LEDs should flash.
 * @param[in] times Indicates how many times to blink. Set to 0 in order to blink forever.
 *
 * @return    NRF_SUCCESS on success, otherwise an error code.
 */
static ret_code_t m_leds_flash(uint8_t leds_mask, uint8_t times)
{
    unsigned int i;

    /*
     * Check if all requested LEDs are suppoted.
     * Check if the 'times' parameter could be safely doubled.
     */
    if ((leds_mask & ~drv_leds_all()) || (times & 0x80))
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    for (i = 0; (leds_mask != 0) && (i < MAX_LEDS); i++)
    {
        led_entry_t *p_led = &m_leds[i];

        if (leds_mask & (1 << i))
        {
            p_led->state      = (times == 0) ? LED_STATE_BLINKING_FOREVER : LED_STATE_BLINKING;
            p_led->state_data = (times << 1);
            m_leds_state_machine((void *)(i));
        }
    }

    return NRF_SUCCESS;
}

bool m_leds_event_handler(const event_t *p_event)
{
    switch (p_event->type)
    {
#if CONFIG_LED_SIGNAL_LOW_BATTERY
        case EVT_SYSTEM_BATTERY_LEVEL:
            if (p_event->system.data <= CONFIG_LED_LOW_BATTERY_THRESHOLD)
            {
                APP_ERROR_CHECK(m_leds_clr(CONFIG_LED_LOW_BATTERY_LEDS_CLEAR));
                APP_ERROR_CHECK(m_leds_set(CONFIG_LED_LOW_BATTERY_LEDS_SET));
                APP_ERROR_CHECK(m_leds_flash(CONFIG_LED_LOW_BATTERY_LEDS_FLASH,
                                             CONFIG_LED_LOW_BATTERY_FLASHES));
            }
            break;
#endif /* CONFIG_LED_SIGNAL_LOW_BATTERY */

        case EVT_BT_CONN_STATE:
            switch (p_event->bt.data)
            {
#if CONFIG_LED_SIGNAL_CONNECTION_ERROR
                case BT_CONN_STATE_DISCONNECTED:
                    APP_ERROR_CHECK(m_leds_clr(CONFIG_LED_CONNECTION_ERROR_LEDS_CLEAR));
                    APP_ERROR_CHECK(m_leds_set(CONFIG_LED_CONNECTION_ERROR_LEDS_SET));
                    APP_ERROR_CHECK(m_leds_flash(CONFIG_LED_CONNECTION_ERROR_LEDS_FLASH,
                                                 CONFIG_LED_CONNECTION_ERROR_FLASHES));
                    break;
#endif /* CONFIG_LED_SIGNAL_CONNECTION_ERROR */
#if CONFIG_LED_SIGNAL_CONNECTION
                case BT_CONN_STATE_CONNECTED:
                    APP_ERROR_CHECK(m_leds_clr(CONFIG_LED_CONNECTION_LEDS_CLEAR));
                    APP_ERROR_CHECK(m_leds_set(CONFIG_LED_CONNECTION_LEDS_SET));
                    APP_ERROR_CHECK(m_leds_flash(CONFIG_LED_CONNECTION_LEDS_FLASH,
                                                 CONFIG_LED_CONNECTION_FLASHES));
                break;
#endif /* CONFIG_LED_SIGNAL_CONNECTION */
            }
            break;

            case EVT_BT_ADV_STATE:
                switch (p_event->bt.data)
                {
#if CONFIG_LED_SIGNAL_ADVERTISING
                    case BT_ADV_STATE_ACTIVE:
                    case BT_ADV_STATE_ACTIVE_BONDABLE:
                        APP_ERROR_CHECK(m_leds_clr(CONFIG_LED_ADVERTISING_LEDS_CLEAR));
                        APP_ERROR_CHECK(m_leds_set(CONFIG_LED_ADVERTISING_LEDS_SET));
                        APP_ERROR_CHECK(m_leds_flash(CONFIG_LED_ADVERTISING_LEDS_FLASH,
                                                     CONFIG_LED_ADVERTISING_FLASHES));
                        break;
                    case BT_ADV_STATE_IDLE:
                        APP_ERROR_CHECK(m_leds_clr(CONFIG_LED_ADVERTISING_LEDS_CLEAR));
                        break;
#endif /* CONFIG_LED_SIGNAL_ADVERTISING */
                }
                break;

#if CONFIG_LED_SIGNAL_IMMEDIATE_ALERT
        case EVT_BT_ALERT_LEVEL:
            switch (p_event->bt.data)
            {
                case 0:     // Level: None
                    APP_ERROR_CHECK(m_leds_clr(CONFIG_LED_IMMEDIATE_ALERT_0_CLEAR));
                    APP_ERROR_CHECK(m_leds_set(CONFIG_LED_IMMEDIATE_ALERT_0_SET));
                    APP_ERROR_CHECK(m_leds_flash(CONFIG_LED_IMMEDIATE_ALERT_0_FLASH,
                                                 CONFIG_LED_IMMEDIATE_ALERT_0_FLASHES));
                    break;

                case 1:     // Level: Mild
                    APP_ERROR_CHECK(m_leds_clr(CONFIG_LED_IMMEDIATE_ALERT_1_CLEAR));
                    APP_ERROR_CHECK(m_leds_set(CONFIG_LED_IMMEDIATE_ALERT_1_SET));
                    APP_ERROR_CHECK(m_leds_flash(CONFIG_LED_IMMEDIATE_ALERT_1_FLASH,
                                                 CONFIG_LED_IMMEDIATE_ALERT_1_FLASHES));
                    break;

                default:    // Level: High
                    APP_ERROR_CHECK(m_leds_clr(CONFIG_LED_IMMEDIATE_ALERT_2_CLEAR));
                    APP_ERROR_CHECK(m_leds_set(CONFIG_LED_IMMEDIATE_ALERT_2_SET));
                    APP_ERROR_CHECK(m_leds_flash(CONFIG_LED_IMMEDIATE_ALERT_2_FLASH,
                                                 CONFIG_LED_IMMEDIATE_ALERT_2_FLASHES));
                    break;
            }
            break;
#endif /* CONFIG_LED_SIGNAL_IMMEDIATE_ALERT */

        default:
            /* Ignore */
            break;
    }

    return false;
}

#if CONFIG_PWR_MGMT_ENABLED
static bool m_leds_shutdown(nrf_pwr_mgmt_evt_t event)
{
    bool retval = true;
    unsigned int i;

    m_leds_going_down = true;

    // Blink up to 3 times (6 cycles) before going to sleep.
    if (event == NRF_PWR_MGMT_EVT_PREPARE_WAKEUP)
    {
        for (i = 0; i < MAX_LEDS; i++)
        {
            led_entry_t *p_led = &m_leds[i];

            if (p_led->state == LED_STATE_BLINKING_FOREVER)
            {
                p_led->state = LED_STATE_BLINKING;
            }

            if (p_led->state == LED_STATE_BLINKING)
            {
                if ((p_led->state_data >> 1) > 3)
                {
                   p_led->state_data = (3 << 1) | (p_led->state_data & 0x01);
                }

                retval = false;
            }

        }
    }

    // Clear all LEDs if we are ready for shutdown.
    if (retval == true)
    {
        APP_ERROR_CHECK(drv_leds_clr(drv_leds_all()));
    }

    return retval;
}
NRF_PWR_MGMT_HANDLER_REGISTER(m_leds_shutdown, SHUTDOWN_PRIORITY_LATE);
#endif /* CONFIG_PWR_MGMT_ENABLED */
#endif /* CONFIG_LED_ENABLED */

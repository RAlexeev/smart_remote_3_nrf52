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

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "nrf_assert.h"
#include "nrf_error.h"
#include "nrf_gpio.h"
#include "nrf_pwr_mgmt.h"
#include "app_debug.h"
#include "app_error.h"
#include "app_scheduler.h"
#include "app_timer.h"

#include "drv_touchpad.h"
#include "m_touchpad.h"

#include "resources.h"
#include "sr3_config.h"

#if CONFIG_TOUCHPAD_ENABLED

#define NRF_LOG_MODULE_NAME m_touchpad
#define NRF_LOG_LEVEL CONFIG_TOUCHPAD_MODULE_LOG_LEVEL
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();


APP_TIMER_DEF                    (s_timer);
static drv_touchpad_data_t       s_touchpad_data;
static bool                      s_touchpad_enabled;
static bool                      s_touchpad_tap_state;
static bool                      s_gyro_enabled;

static void m_touchpad_read_handler(ret_code_t status, drv_touchpad_data_t *p_data)
{
    static uint32_t timestamp;
    static uint8_t errors = 0;

    /*
     * Sometimes the touchpad is not ready to handle our request.
     * Do not fail if the error is not persistent.
     */
    if (status == NRF_ERROR_INTERNAL)
    {
        if (errors++ < 3)
            return;
    }
    else if (status == NRF_SUCCESS)
    {
        errors = 0;
    }
    else
    {
        APP_ERROR_CHECK_BOOL(false);
    }

    /* When gyroscope is enabled, the touchpad sends only wheel/pan motion. */
    if (s_gyro_enabled)
    {
        p_data->pan     += p_data->x;
        p_data->scroll  += p_data->y;

        p_data->x       = 0;
        p_data->y       = 0;
    }

    /* Send events */
    if (p_data->x)
    {
        event_send(EVT_REL_X, p_data->x);
    }

    if (p_data->y)
    {
        event_send(EVT_REL_Y, p_data->y);
    }

    if (p_data->scroll)
    {
        event_send(EVT_REL_WHEEL, p_data->scroll);
    }

    if (p_data->pan)
    {
        event_send(EVT_REL_PAN, p_data->pan);
    }

    if (p_data->tap != s_touchpad_tap_state)
    {
        if (p_data->tap)
        {
            timestamp = app_timer_cnt_get();
            event_send(EVT_KEY_DOWN, MOUSE_KEY_ID(0), timestamp);
        }
        else
        {
            event_send(EVT_KEY_UP, MOUSE_KEY_ID(0), timestamp);
        }

        s_touchpad_tap_state = p_data->tap;
    }
}

static void m_touchpad_timer_handler(void * p_context)
{
    ret_code_t status;

    status = drv_touchpad_schedule_read(&s_touchpad_data);
    if (status == NRF_ERROR_BUSY)
    {
        NRF_LOG_WARNING("%s(): WARNING: Cannot schedule a touchpad read: Driver is busy!", (uint32_t)__func__);
    }
    else
    {
        APP_ERROR_CHECK(status);
    }
}

ret_code_t m_touchpad_init(void)
{
    ret_code_t status;

    status = app_timer_create(&s_timer, APP_TIMER_MODE_REPEATED, m_touchpad_timer_handler);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    // Touchpad init
    status = drv_touchpad_init(m_touchpad_read_handler);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    // Touchpad default off
    s_touchpad_enabled      = false;
    s_touchpad_tap_state    = false;
    s_gyro_enabled          = false;

    return drv_touchpad_disable(); // Touchpad is enabled by connection event m_coms_btle_evt_handler.
}

ret_code_t m_touchpad_enable(void)
{
    ret_code_t status;

    ASSERT(s_touchpad_enabled == false);
    NRF_LOG_INFO("Enabled");

    status = app_timer_start(s_timer, APP_TIMER_TICKS(CONFIG_TOUCHPAD_POLL_INTERVAL), NULL);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    s_touchpad_enabled = true;

    return drv_touchpad_enable();
}

ret_code_t m_touchpad_disable(void)
{
    ret_code_t status;

    ASSERT(s_touchpad_enabled == true);
    NRF_LOG_INFO("Disabled");

    status = app_timer_stop(s_timer);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    s_touchpad_enabled = false;

    return drv_touchpad_disable();
}

bool m_touchpad_event_handler(const event_t *p_event)
{
    switch (p_event->type)
    {
        case EVT_SYSTEM_GYRO_STATE:
            s_gyro_enabled = p_event->system.data;
            break;

        default:
            /* Ignore */
            break;
    }

    return false;
}

#if CONFIG_PWR_MGMT_ENABLED
static bool m_touchpad_shutdown(nrf_pwr_mgmt_evt_t event)
{
    if (s_touchpad_enabled)
    {
        m_touchpad_disable();
    }

    return true;
}
NRF_PWR_MGMT_HANDLER_REGISTER(m_touchpad_shutdown, SHUTDOWN_PRIORITY_DEFAULT);
#endif /* CONFIG_PWR_MGMT_ENABLED */
#endif /* CONFIG_TOUCHPAD_ENABLED */

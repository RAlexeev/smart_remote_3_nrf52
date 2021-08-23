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

#include "nrf_assert.h"
#include "nrf_pwr_mgmt.h"
#include "app_debug.h"
#include "app_timer.h"
#include "app_isched.h"

#include "drv_buzzer.h"
#include "m_buzzer.h"

#include "resources.h"
#include "sr3_config.h"

#if CONFIG_BUZZER_ENABLED

#define NRF_LOG_MODULE_NAME m_buzzer
#define NRF_LOG_LEVEL CONFIG_BUZZER_MODULE_LOG_LEVEL
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

typedef enum {
    M_BUZZER_STATE_IDLE,        // Buzzer is idle.
    M_BUZZER_STATE_TRANSITION,  // Buzzer is in transition state.
    M_BUZZER_STATE_ACTIVE,      // Buzzer is active.
} m_buzzer_module_state_t;

static m_buzzer_module_state_t  m_buzzer_module_state;
static const sr3_buzzer_sound_t *mp_buzzer_sound;
static bool                     m_buzzer_sound_changed;

#if CONFIG_PWR_MGMT_ENABLED
static bool                     m_buzzer_going_down;
#endif

/**@brief Buzzer module state machine: Request handler. */
static ret_code_t m_buzzer_play(const sr3_buzzer_sound_t *p_sound)
{
    if (mp_buzzer_sound != p_sound)
    {
        NRF_LOG_INFO("New sound requested: 0x%08X", (uint32_t)p_sound);

        mp_buzzer_sound         = p_sound;
        m_buzzer_sound_changed  = true;

        if (m_buzzer_module_state == M_BUZZER_STATE_IDLE)
        {
            NRF_LOG_INFO("Enabling the buzzer (sound: 0x%08X)...", (uint32_t)mp_buzzer_sound);

            m_buzzer_module_state   = M_BUZZER_STATE_TRANSITION;
            m_buzzer_sound_changed  = false;

            return drv_buzzer_enable(mp_buzzer_sound);
        }
        else
        {
            if (m_buzzer_module_state == M_BUZZER_STATE_ACTIVE)
            {
                NRF_LOG_INFO("Disabling the buzzer...");

                m_buzzer_module_state = M_BUZZER_STATE_TRANSITION;

                return drv_buzzer_disable();
            }
        }
    }

    return NRF_SUCCESS;
}

/**@brief Buzzer state machine: Acknowledge handler. */
static void m_buzzer_ack(void *p_context)
{
    drv_buzzer_evt_t event = (drv_buzzer_evt_t)(unsigned long)(p_context);

    ASSERT(m_buzzer_module_state == M_BUZZER_STATE_TRANSITION);

    switch (event)
    {
        case DRV_BUZZER_EVT_STARTED:
            if (m_buzzer_sound_changed)
            {
                NRF_LOG_INFO("Disabling the buzzer...");
                APP_ERROR_CHECK(drv_buzzer_disable());
            }
            else
            {
                m_buzzer_module_state = M_BUZZER_STATE_ACTIVE;
                NRF_LOG_INFO("Enabled");
            }
            break;

        case DRV_BUZZER_EVT_STOPPED:
            if (mp_buzzer_sound != NULL)
            {
                NRF_LOG_INFO("Enabling the buzzer (sound: 0x%08X)...", (uint32_t)mp_buzzer_sound);

                m_buzzer_sound_changed = false;
                APP_ERROR_CHECK(drv_buzzer_enable(mp_buzzer_sound));
            }
            else
            {
                m_buzzer_module_state = M_BUZZER_STATE_IDLE;
                NRF_LOG_INFO("Disabled");

#if CONFIG_PWR_MGMT_ENABLED
                if (m_buzzer_going_down)
                {
                    nrf_pwr_mgmt_shutdown(NRF_PWR_MGMT_SHUTDOWN_CONTINUE);
                }
#endif /* CONFIG_PWR_MGMT_ENABLED */
            }
            break;

        default:
            APP_ERROR_CHECK_BOOL(false);
            break;
    }
}

/**@brief Driver acknowledge handler. */
static void m_buzzer_ack_handler(drv_buzzer_evt_t event)
{
    NRF_LOG_INFO("Event: %u", event);
    APP_ERROR_CHECK(app_isched_event_put(&g_fg_scheduler, m_buzzer_ack, (void *)event));
}

ret_code_t m_buzzer_init(void)
{

    m_buzzer_module_state   = M_BUZZER_STATE_IDLE;
    mp_buzzer_sound         = NULL;
    m_buzzer_sound_changed  = false;

#if CONFIG_PWR_MGMT_ENABLED
    m_buzzer_going_down     = false;
#endif

    return drv_buzzer_init(m_buzzer_ack_handler);
}

bool m_buzzer_event_handler(const event_t *p_event)
{
    const sr3_buzzer_sound_t *p_sound;

    ASSERT(p_event != NULL);

    switch (p_event->type)
    {
        case EVT_BT_ALERT_LEVEL:
            switch(p_event->bt.data)
            {
                case 0:     /* Alarm: None */
                    p_sound = NULL;
                    break;

                case 1:     /* Alarm: Mild */
                    p_sound = g_sr3_buzzer_sound_mild;
                    break;

                default:    /* Alarm: High */
                    p_sound = g_sr3_buzzer_sound_high;
                    break;
            }

            APP_ERROR_CHECK(m_buzzer_play(p_sound));
            break;

        default:
            /* Ignore */
            break;
    }

    return false;
}

#if CONFIG_PWR_MGMT_ENABLED
static bool m_buzzer_shutdown(nrf_pwr_mgmt_evt_t event)
{
    if (m_buzzer_module_state != M_BUZZER_STATE_IDLE)
    {
        if (!m_buzzer_going_down)
        {
            m_buzzer_going_down = true;
            APP_ERROR_CHECK(m_buzzer_play(NULL));
        }

        return false;
    }
    else
    {
        return true;
    }
}
NRF_PWR_MGMT_HANDLER_REGISTER(m_buzzer_shutdown, SHUTDOWN_PRIORITY_DEFAULT);
#endif /* CONFIG_PWR_MGMT_ENABLED */
#endif /* CONFIG_BUZZER_ENABLED */

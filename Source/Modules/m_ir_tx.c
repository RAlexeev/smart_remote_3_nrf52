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

#include "drv_ir.h"
#include "m_ir_tx.h"

#include "resources.h"
#include "sr3_config.h"

#if CONFIG_IR_TX_ENABLED

#define NRF_LOG_MODULE_NAME m_ir_rx
#define NRF_LOG_LEVEL CONFIG_IR_MODULE_LOG_LEVEL
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

typedef enum {
    M_IR_STATE_IDLE,        // IR transmitter is idle.
    M_IR_STATE_TRANSITION,  // IR transmitter is in transition state.
    M_IR_STATE_ACTIVE,      // IR transmitter is active.
} m_ir_module_state_t;

APP_TIMER_DEF                   (m_gap_timer);
static m_ir_module_state_t      m_ir_module_state;
static const sr3_ir_symbol_t    *mp_current_symbol;

#if CONFIG_PWR_MGMT_ENABLED
static bool                     m_ir_going_down;
#endif

/**@brief IR Module State Machine: Request handler. */
static ret_code_t m_ir_tx_send_symbol(const sr3_ir_symbol_t *p_symbol)
{
    if (mp_current_symbol != p_symbol)
    {
        NRF_LOG_DEBUG("New symbol requested: 0x%08X", (uint32_t)p_symbol);
        mp_current_symbol = p_symbol;

        if (m_ir_module_state == M_IR_STATE_IDLE)
        {
            m_ir_module_state = M_IR_STATE_TRANSITION;

            APP_ERROR_CHECK(drv_ir_enable());
            NRF_LOG_INFO("IR Transmitter: Enabled");

            return drv_ir_send_symbol(p_symbol);
        }
        else
        {
            if (m_ir_module_state == M_IR_STATE_ACTIVE)
            {
                NRF_LOG_DEBUG("Symbol Change: Requesting 0x%08X", (uint32_t)NULL);
                m_ir_module_state = M_IR_STATE_TRANSITION;
                return drv_ir_send_symbol(NULL);
            }
        }
    }

    return NRF_SUCCESS;
}

/**@brief IR Module State Machine: Acknowledgement handler. */
static void m_ir_tx_ack_symbol(void *p_context)
{
    sr3_ir_symbol_t *p_symbol = p_context;

    ASSERT(m_ir_module_state == M_IR_STATE_TRANSITION);

    if (mp_current_symbol == p_symbol)
    {
        m_ir_module_state = (mp_current_symbol != NULL) ? M_IR_STATE_ACTIVE : M_IR_STATE_IDLE;

        if (m_ir_module_state == M_IR_STATE_IDLE)
        {
            APP_ERROR_CHECK(drv_ir_disable());
            NRF_LOG_INFO("IR Transmitter: Disabled");

#if CONFIG_PWR_MGMT_ENABLED
            if (m_ir_going_down)
            {
                nrf_pwr_mgmt_shutdown(NRF_PWR_MGMT_SHUTDOWN_CONTINUE);
            }
#endif /* CONFIG_PWR_MGMT_ENABLED */
        }
    }
    else
    {
        if (p_symbol == NULL)
        {
            NRF_LOG_DEBUG("Symbol Change: Requesting 0x%08X", (uint32_t)mp_current_symbol);
            APP_ERROR_CHECK(drv_ir_send_symbol(mp_current_symbol));
        }
        else
        {
            NRF_LOG_DEBUG("Symbol Change: Requesting 0x%08X", (uint32_t)NULL);
            APP_ERROR_CHECK(drv_ir_send_symbol(NULL));
        }
    }
}

/**@brief Driver acknowledgement handler. */
static void m_ir_tx_drv_ack_handler(const sr3_ir_symbol_t *p_symbol)
{
    NRF_LOG_DEBUG("Symbol 0x%08X acknowledged.", (uint32_t)p_symbol);

    if (p_symbol)
    {
        APP_ERROR_CHECK(app_isched_event_put(&g_fg_scheduler, m_ir_tx_ack_symbol, (void *)p_symbol));
    }
    else
    {
        // Insert inter-symbol gap after stopping transmission.
        NRF_LOG_DEBUG("Inserting inter-symbol gap...");
        APP_ERROR_CHECK(app_timer_start(m_gap_timer, APP_TIMER_TICKS(CONFIG_IR_TX_INTER_SYMBOL_GAP), (void *)p_symbol));
    }
}

ret_code_t m_ir_tx_init(void)
{
    ret_code_t status;

    status = app_timer_create(&m_gap_timer, APP_TIMER_MODE_SINGLE_SHOT, m_ir_tx_ack_symbol);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    mp_current_symbol = NULL;
    m_ir_module_state = M_IR_STATE_IDLE;

#if CONFIG_PWR_MGMT_ENABLED
    m_ir_going_down   = false;
#endif

    return drv_ir_init(m_ir_tx_drv_ack_handler);
}

bool m_ir_tx_event_handler(const event_t *p_event)
{
    ASSERT(p_event != NULL);

    switch (p_event->type)
    {
        case EVT_IR_SYMBOL:
            APP_ERROR_CHECK(m_ir_tx_send_symbol(p_event->ir.p_symbol));
            break;

        default:
            /* Ignore */
            break;
    }

    return false;
}

#if CONFIG_PWR_MGMT_ENABLED
static bool m_ir_tx_shutdown(nrf_pwr_mgmt_evt_t event)
{
    if (m_ir_module_state != M_IR_STATE_IDLE)
    {
        if (!m_ir_going_down)
        {
            m_ir_going_down = true;
            APP_ERROR_CHECK(m_ir_tx_send_symbol(NULL));
        }

        return false;
    }
    else
    {
        return true;
    }
}
NRF_PWR_MGMT_HANDLER_REGISTER(m_ir_tx_shutdown, SHUTDOWN_PRIORITY_DEFAULT);
#endif /* CONFIG_PWR_MGMT_ENABLED */
#endif /* CONFIG_IR_TX_ENABLED */

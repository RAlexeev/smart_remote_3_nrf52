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

#include "nrf_pwr_mgmt.h"
#include "app_error.h"
#include "app_timer.h"

#include "drv_acc.h"
#include "m_acc.h"
#include "event_bus.h"
#include "resources.h"
#include "sr3_config.h"

#if CONFIG_ACC_ENABLED
#define NRF_LOG_MODULE_NAME m_acc
#define NRF_LOG_LEVEL CONFIG_ACC_MODULE_LOG_LEVEL
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

/**@brief Accelerometer click handler. */
static void m_acc_click_handler(void)
{
    uint32_t timestamp = app_timer_cnt_get();

    event_send(EVT_KEY_DOWN, MOUSE_KEY_ID(0), timestamp);
    event_send(EVT_KEY_UP,   MOUSE_KEY_ID(0), timestamp);
}

ret_code_t m_acc_init(void)
{
    return drv_acc_init(m_acc_click_handler);
}

#if CONFIG_ACC_USE_CLICK_DETECTION
ret_code_t m_acc_click_detection_enable(void)
{
    return drv_acc_mode_set(DRV_ACC_MODE_CLICK_DETECT);
}

ret_code_t m_acc_click_detection_disable(void)
{
    return drv_acc_mode_set(DRV_ACC_MODE_IDLE);
}
#endif /* CONFIG_ACC_USE_CLICK_DETECTION */

#if CONFIG_PWR_MGMT_ENABLED
static bool m_acc_shutdown(nrf_pwr_mgmt_evt_t event)
{
#if CONFIG_ACC_WAKEUP_SOURCE
    if (event == NRF_PWR_MGMT_EVT_PREPARE_WAKEUP)
    {
        APP_ERROR_CHECK(drv_acc_mode_set(DRV_ACC_MODE_WAKE_UP));
    }
    else
#endif /* CONFIG_ACC_WAKEUP_SOURCE */
    {
        APP_ERROR_CHECK(drv_acc_mode_set(DRV_ACC_MODE_IDLE));
    }

    return true;
}

NRF_PWR_MGMT_HANDLER_REGISTER(m_acc_shutdown, SHUTDOWN_PRIORITY_LATE);
#endif /* CONFIG_PWR_MGMT_ENABLED */
#endif /* CONFIG_ACC_ENABLED */

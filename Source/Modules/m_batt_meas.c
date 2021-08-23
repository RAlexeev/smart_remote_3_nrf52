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

#include "nrf_drv_saadc.h"
#include "nrf_pwr_mgmt.h"
#include "app_debug.h"
#include "app_timer.h"

#include "event_bus.h"
#include "m_batt_meas.h"

#include "resources.h"
#include "sr3_config.h"

#if CONFIG_BATT_MEAS_ENABLED

#define NRF_LOG_MODULE_NAME m_batt_meas
#define NRF_LOG_LEVEL CONFIG_BATT_MEAS_MODULE_LOG_LEVEL
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

// Verify SDK configuration.
STATIC_ASSERT(SAADC_ENABLED);

#define ADC_CHANNEL     0
#define ADC_DIVIDER     6
#define ADC_REFERENCE   600 /* mV */
#define ADC_MAX_CONV    ((1 << 14) - 1)

APP_TIMER_DEF           (m_batt_timer);                 /**< Battery measurement timer. */
static uint8_t          m_batt_meas_prev_level = 255;   /**< Previous notified battery level. */
#if CONFIG_PWR_MGMT_ENABLED
static bool             m_batt_meas_going_down;         /**< True if module shutdown was requested. */
#endif

/**@brief Process ADC data. */
static void m_batt_meas_process(void *p_context)
{
    nrf_saadc_value_t measurement = *(nrf_saadc_value_t *)(p_context);
    uint32_t voltage;
    uint8_t level;

    // Calculate battery voltage.
    voltage = ((uint32_t)(measurement) * ADC_DIVIDER * ADC_REFERENCE) / ADC_MAX_CONV;

    // A simple linear approximation is sufficient when the power consumption is fairly low (< 100 mW).
    if (voltage >= CONFIG_BATT_MEAS_MAX_LEVEL)
    {
        level = 100;
    }
    else if (voltage <= CONFIG_BATT_MEAS_MIN_LEVEL)
    {
        level = 0;
    }
    else
    {
        level = 100 * (voltage - CONFIG_BATT_MEAS_MIN_LEVEL) /
                (CONFIG_BATT_MEAS_MAX_LEVEL - CONFIG_BATT_MEAS_MIN_LEVEL);
    }

    NRF_LOG_INFO("Battery level: %u%% (%u mV)", level, voltage);

    /*
     * Only notify the application about the battery level if:
     * - change in the battery level since the last notification has exceeded the threshold,
     * - or if the battery level is 0.
     */
    if ((level <= ((int)(m_batt_meas_prev_level) - CONFIG_BATT_NOTIFICATION_THRESHOLD)) ||
        (level >= ((int)(m_batt_meas_prev_level) + CONFIG_BATT_NOTIFICATION_THRESHOLD)) ||
        (level == 0))
    {
        event_send(EVT_SYSTEM_BATTERY_LEVEL, level);
        m_batt_meas_prev_level = level;
    }
}

static void m_batt_meas_timeout_handler(void* p_context)
{
    static nrf_saadc_value_t buffer;

#if CONFIG_PWR_MGMT_ENABLED
    /*
     * Timeout handler might be called after shutdown, when the SAADC driver is already uninitialized.
     * In such case, the measurement cannot be perfomed.
     */
    if (m_batt_meas_going_down)
    {
        return;
    }
#endif

    APP_ERROR_CHECK(nrf_drv_saadc_buffer_convert(&buffer, 1));
    APP_ERROR_CHECK(nrf_drv_saadc_sample());
}


void m_batt_meas_saadc_event_handler(const nrf_drv_saadc_evt_t *p_event)
{
    switch (p_event->type)
    {
        case NRF_DRV_SAADC_EVT_CALIBRATEDONE:
            // Perform first measurement just after calibration.
            APP_ERROR_CHECK(app_isched_event_put(&g_fg_scheduler,
                                                 m_batt_meas_timeout_handler,
                                                 NULL));

            // The following measurements will be done at regular intervals.
            APP_ERROR_CHECK(app_timer_start(m_batt_timer,
                                            APP_TIMER_TICKS(1000u * CONFIG_BATT_MEAS_POLL_INTERVAL),
                                            NULL));
            break;

        case NRF_DRV_SAADC_EVT_DONE:
            ASSERT(p_event->data.done.size == 1);
            APP_ERROR_CHECK(app_isched_event_put(&g_fg_scheduler,
                                                 m_batt_meas_process,
                                                 p_event->data.done.p_buffer));
            break;

        default:
            /* Ignore */
            break;
    }
}

ret_code_t m_batt_meas_init(void)
{
    nrf_saadc_channel_config_t adc_channel_config = NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_VDD);
    ret_code_t status;

    // Verify the SAADC driver configuration.
    ASSERT(SAADC_CONFIG_RESOLUTION      == NRF_SAADC_RESOLUTION_14BIT);
    ASSERT(adc_channel_config.reference == NRF_SAADC_REFERENCE_INTERNAL);
    ASSERT(adc_channel_config.gain      == NRF_SAADC_GAIN1_6);

    // Enable Burst Mode if oversampling is enabled.
    adc_channel_config.burst = (SAADC_CONFIG_OVERSAMPLE != 0) ? NRF_SAADC_BURST_ENABLED :
                                                                NRF_SAADC_BURST_DISABLED;

    status = nrf_drv_saadc_init(NULL, m_batt_meas_saadc_event_handler);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    status = nrf_drv_saadc_channel_init(ADC_CHANNEL, &adc_channel_config);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    status = app_timer_create(&m_batt_timer, APP_TIMER_MODE_REPEATED, m_batt_meas_timeout_handler);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    // Trigger calibration procedure.
    return nrf_drv_saadc_calibrate_offset();
}

#if CONFIG_PWR_MGMT_ENABLED
static bool m_batt_meas_shutdown(nrf_pwr_mgmt_evt_t event)
{
    m_batt_meas_going_down = true;

    app_timer_stop(m_batt_timer);
    nrf_drv_saadc_channel_uninit(ADC_CHANNEL);
    nrf_drv_saadc_uninit();

    return true;
}
NRF_PWR_MGMT_HANDLER_REGISTER(m_batt_meas_shutdown, SHUTDOWN_PRIORITY_DEFAULT);
#endif /* CONFIG_PWR_MGMT_ENABLED */
#endif /* CONFIG_BATT_MEAS_ENABLED */

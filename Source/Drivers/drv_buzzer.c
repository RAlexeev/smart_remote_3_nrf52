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
#include "nrf_assert.h"
#include "nrf_drv_pwm.h"
#include "app_util_platform.h"

#include "drv_buzzer.h"

#if CONFIG_BUZZER_ENABLED

#define NRF_LOG_MODULE_NAME drv_buzzer
#define NRF_LOG_LEVEL       CONFIG_BUZZER_DRV_LOG_LEVEL
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

#define BUZZER_PWM_BASE_FREQUENCY   1000000
#define BUZZER_PWM_FREQUENCY        4000
#define BUZZER_PWM_TOP_VALUE        (BUZZER_PWM_BASE_FREQUENCY / BUZZER_PWM_FREQUENCY)
#define BUZZER_PWM_SYMBOL_REPEAT    0
#define BUZZER_PWM_BUFFER_SIZE      64
#define BUZZER_PWM_NON_STOP_LOOPS   (uint16_t)(-1)

#define BUZZER_PWM_POLARITY_BIT     0x8000
#define BUZZER_PWM_ON_SYMBOL        (BUZZER_PWM_POLARITY_BIT | (BUZZER_PWM_TOP_VALUE / 2))
#define BUZZER_PWM_OFF_SYMBOL       (BUZZER_PWM_POLARITY_BIT)

static nrf_drv_pwm_t                m_pwm = CONFIG_BUZZER_PWM_INSTANCE;
static drv_buzzer_event_handler_t   m_drv_buzzer_event_handler;
static const sr3_buzzer_sound_t    *mp_buzzer_sound;
static const sr3_buzzer_sound_t    *mp_buzzer_sound_segment;
static uint32_t                     m_period_index;
static uint16_t                     m_seq0_samples[BUZZER_PWM_BUFFER_SIZE];
static uint16_t                     m_seq1_samples[BUZZER_PWM_BUFFER_SIZE];

static nrf_pwm_sequence_t const     m_seq0 =
{
    .values.p_common = m_seq0_samples,
    .length          = NRF_PWM_VALUES_LENGTH(m_seq0_samples),
    .repeats         = BUZZER_PWM_SYMBOL_REPEAT,
    .end_delay       = 0
};

static nrf_pwm_sequence_t const     m_seq1 =
{
    .values.p_common = m_seq1_samples,
    .length          = NRF_PWM_VALUES_LENGTH(m_seq1_samples),
    .repeats         = BUZZER_PWM_SYMBOL_REPEAT,
    .end_delay       = 0
};

static void drv_buzzer_update_buff(uint16_t *p_buffer, unsigned int buffer_size)
{
    while (buffer_size--)
    {
        *(p_buffer++) = (m_period_index++ < mp_buzzer_sound_segment->on_time_count) ?
                        BUZZER_PWM_ON_SYMBOL : BUZZER_PWM_OFF_SYMBOL;

        if (m_period_index >= (mp_buzzer_sound_segment->on_time_count + mp_buzzer_sound_segment->off_time_count))
        {
            m_period_index           = 0;
            mp_buzzer_sound_segment += 1;

            if ((mp_buzzer_sound_segment->on_time_count | mp_buzzer_sound_segment->off_time_count) == 0)
            {
                mp_buzzer_sound_segment = mp_buzzer_sound;
            }
        }
    }
}

static void drv_buzzer_pwm_event_handler(nrf_drv_pwm_evt_type_t event_type)
{
    switch (event_type)
    {
        case NRF_DRV_PWM_EVT_END_SEQ0:
            drv_buzzer_update_buff(m_seq0_samples, ARRAY_SIZE(m_seq0_samples));
            break;

        case NRF_DRV_PWM_EVT_END_SEQ1:
            drv_buzzer_update_buff(m_seq1_samples, ARRAY_SIZE(m_seq1_samples));
            break;

        case NRF_DRV_PWM_EVT_FINISHED:
        case NRF_DRV_PWM_EVT_STOPPED:
            nrf_pwm_disable(m_pwm.p_registers);
            if (m_drv_buzzer_event_handler != NULL)
            {
                m_drv_buzzer_event_handler(DRV_BUZZER_EVT_STOPPED);
            }
            break;

        default:
            /* Ignore */
            break;
    }
}

ret_code_t drv_buzzer_enable(const sr3_buzzer_sound_t *p_sound)
{
    if (p_sound == NULL)
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    ASSERT((p_sound->on_time_count | p_sound->off_time_count) != 0);

    m_period_index          = 0;
    mp_buzzer_sound         = p_sound;
    mp_buzzer_sound_segment = p_sound;


    drv_buzzer_update_buff(m_seq0_samples, ARRAY_SIZE(m_seq0_samples));
    drv_buzzer_update_buff(m_seq1_samples, ARRAY_SIZE(m_seq1_samples));

    nrf_pwm_enable(m_pwm.p_registers);
    nrf_drv_pwm_complex_playback(&m_pwm, &m_seq0, &m_seq1, BUZZER_PWM_NON_STOP_LOOPS,
        (NRF_DRV_PWM_FLAG_SIGNAL_END_SEQ0 | NRF_DRV_PWM_FLAG_SIGNAL_END_SEQ1 | NRF_DRV_PWM_FLAG_STOP));

    if (m_drv_buzzer_event_handler != NULL)
    {
        m_drv_buzzer_event_handler(DRV_BUZZER_EVT_STARTED);
    }

    return NRF_SUCCESS;
}

ret_code_t drv_buzzer_disable(void)
{
    nrf_drv_pwm_stop(&m_pwm, false);
    return NRF_SUCCESS;
}

ret_code_t drv_buzzer_init(drv_buzzer_event_handler_t buzzer_event_handler)
{
    static const nrf_drv_pwm_config_t config =
    {
        .output_pins =
        {
            IS_IO_VALID(CONFIG_IO_BUZZER) ? CONFIG_IO_BUZZER : NRF_DRV_PWM_PIN_NOT_USED,
            NRF_DRV_PWM_PIN_NOT_USED,
            NRF_DRV_PWM_PIN_NOT_USED,
            NRF_DRV_PWM_PIN_NOT_USED,
        },

        .irq_priority   = PDM_CONFIG_IRQ_PRIORITY,
        .base_clock     = NRF_PWM_CLK_1MHz,
        .count_mode     = NRF_PWM_MODE_UP,
        .top_value      = BUZZER_PWM_TOP_VALUE,
        .load_mode      = NRF_PWM_LOAD_COMMON,
        .step_mode      = NRF_PWM_STEP_AUTO
    };

    ret_code_t status = nrf_drv_pwm_init(&m_pwm, &config, drv_buzzer_pwm_event_handler);
    if (status == NRF_SUCCESS)
    {
        nrf_pwm_disable(m_pwm.p_registers);
        m_drv_buzzer_event_handler = buzzer_event_handler;
    }

    return status;
}

#endif /* CONFIG_BUZZER_ENABLED */

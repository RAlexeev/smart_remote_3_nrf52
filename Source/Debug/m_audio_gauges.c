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
#include <string.h>

#include "nrf_assert.h"
#include "app_timer.h"

#include "m_audio_gauges.h"

#if CONFIG_AUDIO_GAUGES_ENABLED

#define NRF_LOG_MODULE_NAME m_audio_gauges
#define NRF_LOG_LEVEL CONFIG_AUDIO_MODULE_LOG_LEVEL
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

#define FRAME_LENGTH_IN_TICKS   \
        (CONFIG_AUDIO_FRAME_SIZE_SAMPLES * (APP_TIMER_CLOCK_FREQ / (APP_TIMER_PRESCALER + 1)) \
        / CONFIG_AUDIO_REAL_SAMPLING_FREQUENCY)

void m_audio_cpu_gauge_reset(m_audio_cpu_gauge_t *p_gauge)
{
    ASSERT(p_gauge != NULL);

    memset(p_gauge, 0, sizeof(*p_gauge));
    p_gauge->min_cpu_usage = ~0;
}

void m_audio_cpu_gauge_log(const m_audio_cpu_gauge_t *p_gauge, const char *p_prefix)
{
    ASSERT(p_gauge != NULL);

    NRF_LOG_INFO("%s CPU usage (min/avg/max): %u%%/%u%%/%u%%",
                 p_prefix,
                 m_audio_gauge_get_min_cpu_usage(p_gauge),
                 m_audio_gauge_get_avg_cpu_usage(p_gauge),
                 m_audio_gauge_get_max_cpu_usage(p_gauge));
}

void m_audio_measure_cpu_usage_start(m_audio_cpu_gauge_t *p_gauge)
{
    ASSERT(p_gauge != NULL);

    p_gauge->timestamp = app_timer_cnt_get();
}

void m_audio_measure_cpu_usage_end(m_audio_cpu_gauge_t *p_gauge)
{
    uint32_t delta;

    delta = app_timer_cnt_diff_compute(app_timer_cnt_get(), p_gauge->timestamp);

    p_gauge->cur_cpu_usage = 100ul * delta / FRAME_LENGTH_IN_TICKS;
    p_gauge->total_time   += FRAME_LENGTH_IN_TICKS;
    p_gauge->cpu_time     += delta;

    if (p_gauge->max_cpu_usage < p_gauge->cur_cpu_usage)
    {
         p_gauge->max_cpu_usage = p_gauge->cur_cpu_usage;
    }

    if (p_gauge->min_cpu_usage > p_gauge->cur_cpu_usage)
    {
        p_gauge->min_cpu_usage = p_gauge->cur_cpu_usage;
    }
}

void m_audio_bitrate_gauge_reset(m_audio_bitrate_gauge_t *p_gauge)
{
    ASSERT(p_gauge != NULL);

    memset(p_gauge, 0, sizeof(*p_gauge));
    p_gauge->min_bitrate = ~0;
}

void m_audio_bitrate_gauge_log(const m_audio_bitrate_gauge_t *p_gauge, const char *p_prefix)
{
    ASSERT(p_gauge != NULL);

    NRF_LOG_INFO("%s Bitrate (min/avg/max): %u/%u/%u kbit/s",
                 p_prefix,
                 m_audio_gauge_get_min_bitrate(p_gauge),
                 m_audio_gauge_get_avg_bitrate(p_gauge),
                 m_audio_gauge_get_max_bitrate(p_gauge));
}

void m_audio_measure_bitrate(m_audio_bitrate_gauge_t *p_gauge, unsigned int bytes)
{
    ASSERT(p_gauge != NULL);

    p_gauge->cur_bitrate    = 8ul * bytes * CONFIG_AUDIO_REAL_SAMPLING_FREQUENCY / CONFIG_AUDIO_FRAME_SIZE_SAMPLES / 1000;
    p_gauge->frames         += 1;
    p_gauge->bytes          += bytes;

    if (p_gauge->max_bitrate < p_gauge->cur_bitrate)
    {
        p_gauge->max_bitrate = p_gauge->cur_bitrate;
    }

    if (p_gauge->min_bitrate > p_gauge->cur_bitrate)
    {
        p_gauge->min_bitrate = p_gauge->cur_bitrate;
    }
}

void m_audio_loss_gauge_reset(m_audio_loss_gauge_t *p_gauge)
{
    ASSERT(p_gauge != NULL);

    memset(p_gauge, 0, sizeof(*p_gauge));
}

void m_audio_loss_gauge_log(const m_audio_loss_gauge_t *p_gauge, const char *p_prefix)
{
    uint32_t total, lost, discarded;

    ASSERT(p_gauge != NULL);

    total     = m_audio_gauge_get_total_count(p_gauge);
    lost      = m_audio_gauge_get_lost_count(p_gauge);
    discarded = m_audio_gauge_get_discarded_count(p_gauge);

    NRF_LOG_INFO("%s processed: %u, lost: %u, discarded: %u (loss ratio: %u%%, discard ratio: %u%%)",
                 p_prefix,
                 total,
                 lost,
                 discarded,
                 (total != 0) ? (100ul * lost / total) : 0,
                 (total != 0) ? (100ul * discarded / total) : 0);
}

void m_audio_count_total(m_audio_loss_gauge_t *p_gauge)
{
    nrf_atomic_u32_add(&(p_gauge->total), 1);
}

void m_audio_count_lost(m_audio_loss_gauge_t *p_gauge)
{
    nrf_atomic_u32_add(&(p_gauge->lost), 1);
}

void m_audio_count_discarded(m_audio_loss_gauge_t *p_gauge)
{
    nrf_atomic_u32_add(&(p_gauge->discarded), 1);
}

#endif /* CONFIG_AUDIO_GAUGES_ENABLED */

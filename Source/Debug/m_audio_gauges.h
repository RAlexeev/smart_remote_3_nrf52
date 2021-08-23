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

#ifndef __M_AUDIO_GAUGES_H__
#define __M_AUDIO_GAUGES_H__

#include <stdint.h>
#include "nrf.h"
#include "nrf_atomic.h"
#include "app_util_platform.h"
#include "sr3_config.h"

typedef struct
{
    uint32_t    total_time;
    uint32_t    cpu_time;
    uint32_t    timestamp;
    uint8_t     min_cpu_usage;
    uint8_t     cur_cpu_usage;
    uint8_t     max_cpu_usage;
} m_audio_cpu_gauge_t;

typedef struct
{
    uint32_t    frames;
    uint32_t    bytes;
    uint8_t     min_bitrate;
    uint8_t     cur_bitrate;
    uint8_t     max_bitrate;
} m_audio_bitrate_gauge_t;

typedef struct
{
    nrf_atomic_u32_t    total;
    nrf_atomic_u32_t    lost;
    nrf_atomic_u32_t    discarded;
} m_audio_loss_gauge_t;

__STATIC_INLINE uint8_t m_audio_gauge_get_cur_cpu_usage(const m_audio_cpu_gauge_t *p_gauge)
{
    return p_gauge->cur_cpu_usage;
}

__STATIC_INLINE uint8_t m_audio_gauge_get_min_cpu_usage(const m_audio_cpu_gauge_t *p_gauge)
{
    return p_gauge->min_cpu_usage;
}

__STATIC_INLINE uint8_t m_audio_gauge_get_avg_cpu_usage(const m_audio_cpu_gauge_t *p_gauge)
{
    uint32_t total_time;
    uint32_t cpu_time;

    CRITICAL_REGION_ENTER();
    total_time  = p_gauge->total_time;
    cpu_time    = p_gauge->cpu_time;
    CRITICAL_REGION_EXIT();

    if (total_time == 0)
    {
        return 0;
    }

    return 100ull * cpu_time / total_time;
}

__STATIC_INLINE uint8_t m_audio_gauge_get_max_cpu_usage(const m_audio_cpu_gauge_t *p_gauge)
{
    return p_gauge->max_cpu_usage;
}

__STATIC_INLINE uint8_t m_audio_gauge_get_cur_bitrate(const m_audio_bitrate_gauge_t *p_gauge)
{
    return p_gauge->cur_bitrate;
}

__STATIC_INLINE uint8_t m_audio_gauge_get_min_bitrate(const m_audio_bitrate_gauge_t *p_gauge)
{
    return p_gauge->min_bitrate;
}

__STATIC_INLINE uint8_t m_audio_gauge_get_avg_bitrate(const m_audio_bitrate_gauge_t *p_gauge)
{
    uint32_t frames;
    uint32_t bytes;

    CRITICAL_REGION_ENTER();
    frames  = p_gauge->frames;
    bytes   = p_gauge->bytes;
    CRITICAL_REGION_EXIT();

    if (frames == 0)
    {
        return 0;
    }

    return (8ull * bytes * CONFIG_AUDIO_REAL_SAMPLING_FREQUENCY) / (1000ull * frames * CONFIG_AUDIO_FRAME_SIZE_SAMPLES);
}

__STATIC_INLINE uint8_t m_audio_gauge_get_max_bitrate(const m_audio_bitrate_gauge_t *p_gauge)
{
    return p_gauge->max_bitrate;
}

__STATIC_INLINE uint32_t m_audio_gauge_get_total_count(const m_audio_loss_gauge_t *p_gauge)
{
    return p_gauge->total;
}

__STATIC_INLINE uint32_t m_audio_gauge_get_lost_count(const m_audio_loss_gauge_t *p_gauge)
{
    return p_gauge->lost;
}

__STATIC_INLINE uint32_t m_audio_gauge_get_discarded_count(const m_audio_loss_gauge_t *p_gauge)
{
    return p_gauge->discarded;
}

#if CONFIG_AUDIO_GAUGES_ENABLED

void m_audio_cpu_gauge_reset(m_audio_cpu_gauge_t *p_gauge);
void m_audio_cpu_gauge_log(const m_audio_cpu_gauge_t *p_gauge, const char *p_prefix);
void m_audio_measure_cpu_usage_start(m_audio_cpu_gauge_t *p_gauge);
void m_audio_measure_cpu_usage_end(m_audio_cpu_gauge_t *p_gauge);

void m_audio_bitrate_gauge_reset(m_audio_bitrate_gauge_t *p_gauge);
void m_audio_bitrate_gauge_log(const m_audio_bitrate_gauge_t *p_gauge, const char *p_prefix);
void m_audio_measure_bitrate(m_audio_bitrate_gauge_t *p_gauge, unsigned int bytes);

void m_audio_loss_gauge_reset(m_audio_loss_gauge_t *p_gauge);
void m_audio_loss_gauge_log(const m_audio_loss_gauge_t *p_gauge, const char *p_prefix);
void m_audio_count_total(m_audio_loss_gauge_t *p_gauge);
void m_audio_count_lost(m_audio_loss_gauge_t *p_gauge);
void m_audio_count_discarded(m_audio_loss_gauge_t *p_gauge);

#else /* !CONFIG_AUDIO_GAUGES_ENABLED */

#define m_audio_cpu_gauge_reset(p_gauge)            do { } while (0)
#define m_audio_cpu_gauge_log(p_gauge, prefix)      do { } while (0)
#define m_audio_measure_cpu_usage_start(p_gauge)    do { } while (0)
#define m_audio_measure_cpu_usage_end(p_gauge)      do { } while (0)

#define m_audio_bitrate_gauge_reset(p_gauge)        do { } while (0)
#define m_audio_bitrate_gauge_log(p_gauge, prefix)  do { } while (0)
#define m_audio_measure_bitrate(p_gauge, bytes)     do { } while (0)

#define m_audio_loss_gauge_reset(p_gauge)           do { } while (0)
#define m_audio_loss_gauge_log(p_gauge, prefix)     do { } while (0)
#define m_audio_count_total(p_gauge)                do { } while (0)
#define m_audio_count_lost(p_gauge)                 do { } while (0)
#define m_audio_count_discarded(p_gauge)            do { } while (0)

#endif /* CONFIG_AUDIO_GAUGES_ENABLED */
#endif /* __M_AUDIO_GAUGES_H__ */

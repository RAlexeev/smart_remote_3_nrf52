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

#include "app_debug.h"
#include "nrf_balloc.h"

#include "m_audio_frame.h"
#include "resources.h"

#if CONFIG_AUDIO_ENABLED

#define NRF_LOG_MODULE_NAME m_audio_frame
#define NRF_LOG_LEVEL CONFIG_AUDIO_MODULE_LOG_LEVEL
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

NRF_BALLOC_DEF(m_audio_frame_pool, sizeof(m_audio_frame_t), CONFIG_AUDIO_FRAME_POOL_SIZE);

ret_code_t m_audio_frame_init(void)
{
    return nrf_balloc_init(&m_audio_frame_pool);
}

m_audio_frame_t *m_audio_frame_get(m_audio_frame_t *p_frame)
{
    if (p_frame == NULL)
    {
        p_frame = nrf_balloc_alloc(&m_audio_frame_pool);
        if (p_frame != NULL)
        {
            p_frame->reference_count = 1;
        }
    }
    else
    {
        p_frame->reference_count += 1;
        ASSERT(p_frame->reference_count != 0);
    }

    return p_frame;
}

void m_audio_frame_put(m_audio_frame_t *p_frame)
{
    ASSERT(p_frame && p_frame->reference_count != 0);

    --p_frame->reference_count;
    if (p_frame->reference_count == 0)
    {
        nrf_balloc_free(&m_audio_frame_pool, p_frame);
    }
}

uint8_t m_audio_frame_pool_current_utilization_get(void)
{
    return m_audio_frame_pool.p_stack_limit - m_audio_frame_pool.p_cb->p_stack_pointer;
}

uint8_t m_audio_frame_pool_max_utilization_get(void)
{
    return nrf_balloc_max_utilization_get(&m_audio_frame_pool);
}

#endif /* CONFIG_AUDIO_ENABLED */

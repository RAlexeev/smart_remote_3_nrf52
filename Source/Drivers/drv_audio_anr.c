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
#include <stdlib.h>
#include "nrf_assert.h"
#include "drv_audio_anr.h"
#include "sr3_config.h"

#if (CONFIG_AUDIO_ENABLED && CONFIG_AUDIO_ANR_ENABLED)

#define NRF_LOG_MODULE_NAME drv_audio_anr
#define NRF_LOG_LEVEL CONFIG_AUDIO_DRV_ANR_LOG_LEVEL
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

typedef int16_t sint15;
#include "vocal_anr.h"

void drv_audio_anr_init(void)
{
    anr_config_data_t anr_cfg =
    {
        .anr_length             = CONFIG_AUDIO_ANR_LENGTH,
        .anr_delay_length       = CONFIG_AUDIO_ANR_DELAY_LENGTH,
        .anr_mode               = 1,

        .debug_path_search      = 0,
        .debug_generate_noise   = 0,
        .debug_dump_stats       = 0,
        .debug_channel          = 0,
    };

    global_mdm_pre_reset();
    vocal_application_startup();
    anr__initialize(&anr_cfg);
}

void drv_audio_anr_perfrom(int16_t *p_samples, unsigned int buffer_size)
{
    const int16_t *p_in = p_samples;
    int16_t *p_out = p_samples;

    ASSERT(p_samples != NULL);

    while (buffer_size--)
    {
        int16_t signal, noise;

        signal = *p_in++;
        noise  = *p_in++;

        anr__execute_tx(noise);
        *p_out++ = anr__execute_rx(signal);
    }
}

#endif /* (CONFIG_AUDIO_ENABLED && CONFIG_AUDIO_ANR_ENABLED) */

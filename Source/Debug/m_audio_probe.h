/**
 * Copyright (c) 2017 - 2018, Nordic Semiconductor ASA
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
#ifndef __M_AUDIO_TEST_H__
#define __M_AUDIO_TEST_H__

#include <stdint.h>
#include <stddef.h>

#include "nrf_cli.h"
#include "sdk_config.h"

/**@brief CLI handler function top level module command ("audio probe") */
void m_audio_probe_cmd(nrf_cli_t const * p_cli, size_t argc, char **argv);

/**@brief CLI subcommands of the module */
extern const nrf_cli_cmd_entry_t m_audio_probe_subcmds;

/**@brief Type representing probe points in Smart Remote audio chain.
 *
 * Points have to be kept in alphabetical order for console autocomplete.
 */
typedef enum
{
    M_AUDIO_PROBE_POINT_ANR_IN,      /**< Inject audio before ANR */
    M_AUDIO_PROBE_POINT_ANR_OUT,     /**< Tap audio after ANR */
    M_AUDIO_PROBE_POINT_CODEC_IN,    /**< Inject audio before codec */
    M_AUDIO_PROBE_POINT_EQ_IN,       /**< Inject audio before equalizer */
    M_AUDIO_PROBE_POINT_EQ_OUT,      /**< Tap audio after equalizer */
    M_AUDIO_PROBE_POINT_GAIN_IN,     /**< Inject audio before gain control */
    M_AUDIO_PROBE_POINT_GAIN_OUT,    /**< Tap audio after gain control */
    M_AUDIO_PROBE_INFO_SUBCOMMAND,   /**< Dummy entry to simplify autocompletion of audio probe commands in CLI. */
    M_AUDIO_PROBE_POINT_PDM_OUT,     /**< Tap audio after PDM microphone */
    M_AUDIO_PROBE_POINTS_NUM
} m_audio_probe_point_enum_t;

/**@brief Initialize audio instrumentation subsystem. */
void m_audio_probe_init(void);

/**@brief Insert a probe point into the audio chain. */
#if CONFIG_AUDIO_PROBE_ENABLED
void m_audio_probe_point(m_audio_probe_point_enum_t point, int16_t *buffer, size_t buffer_size);
#else
__STATIC_INLINE void m_audio_probe_point(m_audio_probe_point_enum_t point, int16_t *buffer, size_t buffer_size)
{
    // empty function
}
#endif /* CONFIG_AUDIO_PROBE_ENABLED */

#endif /* __M_AUDIO_TEST_H__ */

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

#include "nrf.h"
#include "nrf_assert.h"
#include "app_debug.h"

#include "drv_audio.h"
#include "drv_audio_codec.h"
#include "sr3_config.h"

#if (CONFIG_AUDIO_ENABLED && (CONFIG_AUDIO_CODEC == CONFIG_AUDIO_CODEC_OPUS))

#define NRF_LOG_MODULE_NAME drv_audio_codec
#define NRF_LOG_LEVEL CONFIG_AUDIO_DRV_CODEC_LOG_LEVEL
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

#include "opus.h"
#define OPUS_MAX_FRAME_SIZE 3840

#if   (CONFIG_OPUS_MODE == CONFIG_OPUS_MODE_CELT)
# define OPUS_APPLICATION    OPUS_APPLICATION_RESTRICTED_LOWDELAY
# define OPUS_ENCODER_SIZE   7180
# define OPUS_MODE           "CELT"
# if (CONFIG_AUDIO_SAMPLING_FREQUENCY != 8000) && \
     (CONFIG_AUDIO_SAMPLING_FREQUENCY != 16000) && \
     (CONFIG_AUDIO_SAMPLING_FREQUENCY != 24000)
#  error "Selected sampling frequency is not supported by CELT codec."
# endif
# if (CONFIG_AUDIO_FRAME_SIZE_MS != 5) && \
     (CONFIG_AUDIO_FRAME_SIZE_MS != 10) && \
     (CONFIG_AUDIO_FRAME_SIZE_MS != 20)
#  error "Selected audio frame size is not supported by CELT codec."
# endif
#elif (CONFIG_OPUS_MODE == CONFIG_OPUS_MODE_SILK)
# define OPUS_APPLICATION    OPUS_APPLICATION_VOIP
# define OPUS_ENCODER_SIZE   10916
# define OPUS_MODE           "SILK"
# if (CONFIG_AUDIO_SAMPLING_FREQUENCY != 8000) && \
     (CONFIG_AUDIO_SAMPLING_FREQUENCY != 16000)
#  error "Selected sampling frequency is not supported by SILK codec."
# endif
# if (CONFIG_AUDIO_FRAME_SIZE_MS != 10) && \
     (CONFIG_AUDIO_FRAME_SIZE_MS != 20) && \
     (CONFIG_AUDIO_FRAME_SIZE_MS != 40) && \
     (CONFIG_AUDIO_FRAME_SIZE_MS != 60)
#  error "Selected audio frame size is not supported by SILK codec"
# endif
#else
# error "Unsupported OPUS Mode"
#endif

__ALIGN(4) static uint8_t           m_opus_encoder[OPUS_ENCODER_SIZE];
static OpusEncoder * const          m_opus_state = (OpusEncoder *)m_opus_encoder;

#if CONFIG_CLI_ENABLED
static bool                         m_opus_encoder_initialized;
static uint8_t                      m_opus_complexity = CONFIG_OPUS_COMPLEXITY;
static int32_t                      m_opus_bitrate    = ((CONFIG_OPUS_BITRATE != 0) ? CONFIG_OPUS_BITRATE : OPUS_AUTO);
static bool                         m_opus_vbr        = ((CONFIG_OPUS_BITRATE == 0) || (CONFIG_OPUS_VBR_ENABLED != 0));
#else /* !CONFIG_CLI_ENABLED */
# define                            m_opus_complexity   CONFIG_OPUS_COMPLEXITY
# define                            m_opus_bitrate      ((CONFIG_OPUS_BITRATE != 0) ? CONFIG_OPUS_BITRATE : OPUS_AUTO)
# define                            m_opus_vbr          ((CONFIG_OPUS_BITRATE == 0) || (CONFIG_OPUS_VBR_ENABLED != 0))
#endif /* CONFIG_CLI_ENABLED */

static void drv_audio_codec_log_config(const char *action)
{
    if (m_opus_bitrate == OPUS_AUTO)
    {
        NRF_LOG_INFO("OPUS/" OPUS_MODE " Codec %s (mode: VBR, complexity: %u, frame: %u ms).",
                     action,
                     m_opus_complexity,
                     CONFIG_AUDIO_FRAME_SIZE_MS);
    }
    else
    {
       NRF_LOG_INFO("OPUS/" OPUS_MODE " Codec %s (mode: %s %u kbit/s, complexity: %u, frame: %u ms)",
                    action,
                    (m_opus_vbr) ? "CVBR" : "CBR",
                    m_opus_bitrate / 1000,
                    m_opus_complexity,
                    CONFIG_AUDIO_FRAME_SIZE_MS);
    }
}

void drv_audio_codec_init(void)
{
    ASSERT(opus_encoder_get_size(1) == sizeof(m_opus_encoder));

    APP_ERROR_CHECK_BOOL(opus_encoder_init(m_opus_state, CONFIG_AUDIO_SAMPLING_FREQUENCY, 1, OPUS_APPLICATION) == OPUS_OK);

    APP_ERROR_CHECK_BOOL(opus_encoder_ctl(m_opus_state, OPUS_SET_BITRATE(m_opus_bitrate))                      == OPUS_OK);
    APP_ERROR_CHECK_BOOL(opus_encoder_ctl(m_opus_state, OPUS_SET_VBR(m_opus_vbr))                              == OPUS_OK);
    APP_ERROR_CHECK_BOOL(opus_encoder_ctl(m_opus_state, OPUS_SET_VBR_CONSTRAINT((m_opus_bitrate != OPUS_AUTO)))== OPUS_OK);

    APP_ERROR_CHECK_BOOL(opus_encoder_ctl(m_opus_state, OPUS_SET_COMPLEXITY(m_opus_complexity))                == OPUS_OK);
    APP_ERROR_CHECK_BOOL(opus_encoder_ctl(m_opus_state, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE))                    == OPUS_OK);
    APP_ERROR_CHECK_BOOL(opus_encoder_ctl(m_opus_state, OPUS_SET_LSB_DEPTH(16))                                == OPUS_OK);
    APP_ERROR_CHECK_BOOL(opus_encoder_ctl(m_opus_state, OPUS_SET_DTX(0))                                       == OPUS_OK);
    APP_ERROR_CHECK_BOOL(opus_encoder_ctl(m_opus_state, OPUS_SET_INBAND_FEC(0))                                == OPUS_OK);
    APP_ERROR_CHECK_BOOL(opus_encoder_ctl(m_opus_state, OPUS_SET_PACKET_LOSS_PERC(0))                          == OPUS_OK);

#if CONFIG_CLI_ENABLED
    m_opus_encoder_initialized = true;
#endif

    drv_audio_codec_log_config("initialized");
}

void drv_audio_codec_encode(int16_t *input_samples, m_audio_frame_t *p_frame)
{
    int frame_size;

    frame_size = opus_encode(m_opus_state,
                             input_samples,
                             CONFIG_AUDIO_FRAME_SIZE_SAMPLES,
                             p_frame->data + ((CONFIG_OPUS_HEADER_ENABLED) ? 2 : 0),
                             sizeof(p_frame->data) - ((CONFIG_OPUS_HEADER_ENABLED) ? 2 : 0));

    APP_ERROR_CHECK_BOOL((frame_size >= 0) && (frame_size <= OPUS_MAX_FRAME_SIZE));

#if CONFIG_OPUS_HEADER_ENABLED
    p_frame->data[0] = frame_size >> 8;
    p_frame->data[1] = frame_size >> 0;
#endif

    p_frame->data_size = frame_size + 2;
}

#if CONFIG_CLI_ENABLED
static void drv_audio_codec_set_cmd(nrf_cli_t const * p_cli, size_t argc, char **argv)
{
    if (nrf_cli_help_requested(p_cli))
    {
        nrf_cli_help_print(p_cli, NULL, 0);
        nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Usage:\r\n  %s <parameter> <value>\r\n", argv[0]);
        return;
    }

    if (argc >= 2)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Unknown parameter '%s'!\r\n", argv[1]);
    }
    else
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Please specify parameter!\r\n");
    }
}

static void drv_audio_codec_info_cmd(nrf_cli_t const * p_cli, size_t argc, char **argv)
{
    if (nrf_cli_help_requested(p_cli))
    {
        nrf_cli_help_print(p_cli, NULL, 0);
        nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Usage:\r\n  %s\r\n", argv[0]);
        return;
    }

    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Codec: OPUS/" OPUS_MODE "\r\n");
    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "\tFrame Length:\t%u ms\r\n", CONFIG_AUDIO_FRAME_SIZE_MS);

    if (m_opus_bitrate == OPUS_AUTO)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "\tBitrate:\tVBR\r\n");
    }
    else
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "\tBitrate:\t%s %u kbit/s\r\n",
                        (m_opus_vbr) ? "CVBR" : "CBR",
                        m_opus_bitrate / 1000);
    }

    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "\tComplexity:\t%u\r\n", m_opus_complexity);
}

static void drv_audio_codec_set_bitrate_cmd(nrf_cli_t const * p_cli, size_t argc, char **argv)
{
    bool vbr = m_opus_vbr;
    char *p_str_end;
    long bitrate;

    if (nrf_cli_help_requested(p_cli))
    {
        nrf_cli_help_print(p_cli, NULL, 0);
        nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Usage:\r\n  %s <bitrate> [vbr|cbr]\r\n", argv[0]);
        return;
    }

    if ((argc < 2) || (argc > 3))
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Please specify bitrate and optional bitrate control mode!\r\n");
        return;
    }

    // Parse value string.
    if (!strcmp("auto", argv[1]))
    {
        bitrate = OPUS_AUTO;
    }
    else
    {
        p_str_end = argv[1];
        bitrate = strtol(argv[1], &p_str_end, 10) * 1000;

        if ((*p_str_end != '\0') || (bitrate < 500) || (bitrate > 512000))
        {
            nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Invalid bitrate!\r\n");
            return;
        }
    }

    // Parse optional mode.
    if (argc >= 3)
    {
        if (!strcmp("vbr", argv[2]))
        {
            vbr = true;
        }
        else if (!strcmp("cbr", argv[2]))
        {
            vbr = false;
        }
        else
        {
            nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Invalid bitrate mode!\r\n");
            return;
        }
    }

    if (bitrate == OPUS_AUTO)
    {
        // Select VBR mode if automatic bitrate management is selected.
        vbr = true;
    }

    // Set new paramaters.
    m_opus_bitrate = bitrate;
    m_opus_vbr     = vbr;

    // If codec is initialized, update its settings.
    if (m_opus_encoder_initialized)
    {
        int retval;

        // Use critical region to avoid race condition between parameter update and codec operation.
        CRITICAL_REGION_ENTER();
        retval = opus_encoder_ctl(m_opus_state, OPUS_SET_BITRATE(m_opus_bitrate));
        if (retval == OPUS_OK)
        {
            retval = opus_encoder_ctl(m_opus_state, OPUS_SET_VBR(m_opus_vbr));
            if (retval == OPUS_OK)
            {
                retval = opus_encoder_ctl(m_opus_state,
                                          OPUS_SET_VBR_CONSTRAINT((m_opus_bitrate != OPUS_AUTO)));
            }
        }
        CRITICAL_REGION_EXIT();

        if (retval != OPUS_OK)
        {
            nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Error setting bitrate!\r\n");
            return;
        }

        drv_audio_codec_log_config("reconfigured");
    }
}

static void drv_audio_codec_set_complexity_cmd(nrf_cli_t const * p_cli, size_t argc, char **argv)
{
    long complexity;
    char *p_str_end;

    // Check input parameters.
    if (nrf_cli_help_requested(p_cli))
    {
        nrf_cli_help_print(p_cli, NULL, 0);
        nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Usage:\r\n  %s <0-10>\r\n", argv[0]);
        return;
    }

    if (argc != 2)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Please specify complexity!\r\n");
        return;
    }

    // Parse value string.
    p_str_end = argv[1];
    complexity = strtol(argv[1], &p_str_end, 10);
    if ((*p_str_end != '\0') || (complexity < 0) || (complexity > 10))
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Invalid complexity!\r\n");
        return;
    }

    // Set new complexity.
    m_opus_complexity = complexity;

    // If codec is initialized, update it's settings.
    if (m_opus_encoder_initialized)
    {
        int retval;

        // Use critical region to avoid race condition between parameter update and codec operation.
        CRITICAL_REGION_ENTER();
        retval = opus_encoder_ctl(m_opus_state, OPUS_SET_COMPLEXITY(m_opus_complexity));
        CRITICAL_REGION_EXIT();

        if (retval != OPUS_OK)
        {
            nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Error setting complexity!\r\n");
            return;
        }

        drv_audio_codec_log_config("reconfigured");
    }
}

NRF_CLI_CREATE_STATIC_SUBCMD_SET(drv_audio_codec_set_subcmds)
{
    NRF_CLI_CMD(bitrate,    NULL,   "set codec bitrate",    drv_audio_codec_set_bitrate_cmd),
    NRF_CLI_CMD(complexity, NULL,   "set codec complexity", drv_audio_codec_set_complexity_cmd),
    { NULL }
};

static const nrf_cli_static_entry_t drv_audio_codec_subcmds_table[] =
{
    NRF_CLI_CMD(info,   NULL,                           "print information about codec",    drv_audio_codec_info_cmd),
    NRF_CLI_CMD(set,    &drv_audio_codec_set_subcmds,   "set codec parameter",              drv_audio_codec_set_cmd),
    { NULL }
};

const nrf_cli_cmd_entry_t drv_audio_codec_subcmds =
{
    .is_dynamic = false,
    .u.p_static = drv_audio_codec_subcmds_table,
};
#endif /* CONFIG_CLI_ENABLED */
#endif /* (CONFIG_AUDIO_ENABLED && (CONFIG_AUDIO_CODEC == CONFIG_AUDIO_CODEC_OPUS)) */

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

#if (CONFIG_AUDIO_ENABLED && (CONFIG_AUDIO_CODEC == CONFIG_AUDIO_CODEC_SBC))
#define NRF_LOG_MODULE_NAME drv_audio_codec
#define NRF_LOG_LEVEL CONFIG_AUDIO_DRV_CODEC_LOG_LEVEL
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

#include "sbc_encoder.h"

#if   (CONFIG_AUDIO_SAMPLING_FREQUENCY == 16000)
# define SBC_SAMPLING_FREQUENCY SBC_sf16000
#elif (CONFIG_AUDIO_SAMPLING_FREQUENCY == 32000)
# define SBC_SAMPLING_FREQUENCY SBC_sf32000
#else
# error "Selected sampling frequency is not supported by the SBC codec!"
#endif

#if   (CONFIG_SBC_ALLOCATION == 0)
# define SBC_ALLOCATION_METHOD  SBC_LOUDNESS
#elif (CONFIG_SBC_ALLOCATION == 1)
# define SBC_ALLOCATION_METHOD  SBC_SNR
#else
# error "Unsupported allocation method selected for SBC codec"
#endif

static SBC_ENC_PARAMS m_sbc_enc_params;

void drv_audio_codec_init(void)
{
    m_sbc_enc_params.s16ChannelMode         = SBC_MONO;
    m_sbc_enc_params.s16NumOfChannels       = 1;
    m_sbc_enc_params.s16SamplingFreq        = SBC_SAMPLING_FREQUENCY;
    m_sbc_enc_params.s16NumOfBlocks         = CONFIG_SBC_BLOCKS;
    m_sbc_enc_params.s16NumOfSubBands       = CONFIG_SBC_SUBBANDS;
    m_sbc_enc_params.s16BitPool             = CONFIG_SBC_BITPOOL;
    m_sbc_enc_params.s16AllocationMethod    = SBC_ALLOCATION_METHOD;
    m_sbc_enc_params.mSBCEnabled            = (CONFIG_SBC_MODE == CONFIG_SBC_MODE_MSBC) ? 1 : 0;

    SBC_Encoder_Init(&m_sbc_enc_params);

#if   (CONFIG_SBC_MODE == CONFIG_SBC_MODE_MSBC)
    NRF_LOG_INFO("SBC Codec selected (mSBC mode, frame: %u ms)",
                 CONFIG_AUDIO_FRAME_SIZE_MS);
#elif (CONFIG_SBC_MODE == CONFIG_SBC_MODE_CUSTOM)
    NRF_LOG_INFO("SBC Codec selected (blocks: %u, subbands: %u, bitpool: %u, frame: %u ms)",
                 m_sbc_enc_params.s16NumOfBlocks,
                 m_sbc_enc_params.s16NumOfSubBands,
                 m_sbc_enc_params.s16BitPool,
                 CONFIG_AUDIO_FRAME_SIZE_MS);
#else
# error "Invalid SBC Mode!"
#endif
}

void drv_audio_codec_encode(int16_t *input_samples, m_audio_frame_t *p_frame)
{
    m_sbc_enc_params.ps16PcmBuffer = input_samples;
    m_sbc_enc_params.pu8Packet     = p_frame->data;

    SBC_Encoder(&m_sbc_enc_params);

    p_frame->data_size = m_sbc_enc_params.u16PacketLength;
}

#if CONFIG_CLI_ENABLED
static void drv_audio_codec_info_cmd(nrf_cli_t const * p_cli, size_t argc, char **argv)
{
    if (nrf_cli_help_requested(p_cli))
    {
        nrf_cli_help_print(p_cli, NULL, 0);
        nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Usage:\r\n  %s\r\n", argv[0]);
        return;
    }

    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Codec: %s\r\n",
                    (CONFIG_SBC_MODE == CONFIG_SBC_MODE_MSBC) ? "mSBC" : "SBC");

    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "\tBlocks:\t\t%u\r\n", CONFIG_SBC_BLOCKS);
    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "\tSubbands:\t%u\r\n", CONFIG_SBC_SUBBANDS);
    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "\tBit Pool:\t%u\r\n", CONFIG_SBC_BITPOOL);
    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "\tAllocation:\t%s\r\n",
                    (SBC_ALLOCATION_METHOD == SBC_LOUDNESS) ? "Loudness" :
                    (SBC_ALLOCATION_METHOD == SBC_SNR)      ? "SNR" :
                    "Unknown");
}

static const nrf_cli_static_entry_t drv_audio_codec_subcmds_table[] =
{
    NRF_CLI_CMD(info,   NULL,   "print information about codec",    drv_audio_codec_info_cmd),
    { NULL }
};

const nrf_cli_cmd_entry_t drv_audio_codec_subcmds =
{
    .is_dynamic = false,
    .u.p_static = drv_audio_codec_subcmds_table,
};
#endif /* CONFIG_CLI_ENABLED */
#endif /* (CONFIG_AUDIO_ENABLED && (CONFIG_AUDIO_CODEC == CONFIG_AUDIO_CODEC_SBC)) */

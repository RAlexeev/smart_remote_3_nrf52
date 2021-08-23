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

#ifndef _SR3_CONFIG_AUDIO_H
#define _SR3_CONFIG_AUDIO_H

// Calculate audio parameters.
#if (CONFIG_AUDIO_CODEC == CONFIG_AUDIO_CODEC_ADPCM)
# define CONFIG_AUDIO_FRAME_SIZE_BYTES      ((CONFIG_AUDIO_FRAME_SIZE_SAMPLES / 2) + 3)
#elif (CONFIG_AUDIO_CODEC == CONFIG_AUDIO_CODEC_BV32FP)
# define CONFIG_AUDIO_FRAME_SIZE_SAMPLES    80
# define CONFIG_AUDIO_FRAME_SIZE_BYTES      20
#elif (CONFIG_AUDIO_CODEC == CONFIG_AUDIO_CODEC_OPUS)
# define CONFIG_AUDIO_FRAME_SIZE_BYTES      ((CONFIG_OPUS_BITRATE_LIMIT * CONFIG_AUDIO_FRAME_SIZE_SAMPLES / (8 * CONFIG_AUDIO_SAMPLING_FREQUENCY)) + ((CONFIG_OPUS_HEADER_ENABLED) ? 2 : 0))
# define CONFIG_OPUS_VBR_ENABLED            ((CONFIG_OPUS_BITRATE_CFG & 0x01) == 0x00)
# define CONFIG_OPUS_BITRATE                (CONFIG_OPUS_BITRATE_CFG & ~0x0F)
# if (CONFIG_OPUS_BITRATE > CONFIG_OPUS_BITRATE_LIMIT)
#  error "Bitrate limit cannot be lower than selected codec bitrate"
# endif
#elif (CONFIG_AUDIO_CODEC == CONFIG_AUDIO_CODEC_SBC)
/*
 * See A2DP specification v1.3.1, page 70, 12.9 Calculation of Bit Rate and Frame Length.
 * Since MONO is the only mode used here, the size is calculated for one channel.
 *
 * Fixed settings used for mSBC are taken from "Hands-Free Profile 1.7",
 * page 115, section 5.7.4 "mSBC coding".
 */
# define CONFIG_AUDIO_FRAME_SIZE_SAMPLES    (CONFIG_SBC_BLOCKS * CONFIG_SBC_SUBBANDS)
# define CONFIG_AUDIO_FRAME_SIZE_BYTES      (4 + ((4 * CONFIG_SBC_SUBBANDS) / 8) + CEIL_DIV(CONFIG_SBC_BLOCKS * CONFIG_SBC_BITPOOL, 8))
# if (CONFIG_SBC_MODE == CONFIG_SBC_MODE_MSBC)
#  define CONFIG_SBC_BLOCKS      15
#  define CONFIG_SBC_SUBBANDS    8
#  define CONFIG_SBC_ALLOCATION  SBC_LOUDNESS
#  define CONFIG_SBC_BITPOOL     26
# endif
#else
# error "Unsupported Compression"
#endif

#if (defined(CONFIG_AUDIO_FRAME_SIZE_SAMPLES) && !defined(CONFIG_AUDIO_FRAME_SIZE_MS))
# define CONFIG_AUDIO_FRAME_SIZE_MS (1000 * CONFIG_AUDIO_FRAME_SIZE_SAMPLES / CONFIG_AUDIO_SAMPLING_FREQUENCY)
#elif (defined(CONFIG_AUDIO_FRAME_SIZE_MS) && !defined(CONFIG_AUDIO_FRAME_SIZE_SAMPLES))
# define CONFIG_AUDIO_FRAME_SIZE_SAMPLES (CONFIG_AUDIO_FRAME_SIZE_MS * CONFIG_AUDIO_SAMPLING_FREQUENCY / 1000)
#else
# error "Either CONFIG_AUDIO_FRAME_SIZE_SAMPLES or CONFIG_AUDIO_FRAME_SIZE_MS has to be defined!"
#endif

// Create PDM configuration basing on audio settings.
#if (CONFIG_AUDIO_SAMPLING_FREQUENCY == 8000)
# define CONFIG_PDM_MCLKFREQ                    0x04100000
# define CONFIG_AUDIO_REAL_SAMPLING_FREQUENCY   7936
#elif (CONFIG_AUDIO_SAMPLING_FREQUENCY == 16000)
# define CONFIG_PDM_MCLKFREQ                    0x08400000
# define CONFIG_AUDIO_REAL_SAMPLING_FREQUENCY   16125
#elif (CONFIG_AUDIO_SAMPLING_FREQUENCY == 24000)
# define CONFIG_PDM_MCLKFREQ                    0x0C000000
# define CONFIG_AUDIO_REAL_SAMPLING_FREQUENCY   23819
#elif (CONFIG_AUDIO_SAMPLING_FREQUENCY == 32000)
# define CONFIG_PDM_MCLKFREQ                    0x10000000
# define CONFIG_AUDIO_REAL_SAMPLING_FREQUENCY   31250
#else
# error "Unsuppored CONFIG_AUDIO_SAMPLING_FREQUENCY value!"
#endif

#if CONFIG_AUDIO_ANR_ENABLED
# define CONFIG_PDM_BUFFER_SIZE_SAMPLES (2 * CONFIG_AUDIO_FRAME_SIZE_SAMPLES)
#else /* !CONFIG_AUDIO_ANR_ENABLED */
# define CONFIG_PDM_BUFFER_SIZE_SAMPLES (1 * CONFIG_AUDIO_FRAME_SIZE_SAMPLES)
#endif /* CONFIG_AUDIO_ANR_ENABLED */

// Calculate stack size.
#if CONFIG_AUDIO_ENABLED
# if (CONFIG_AUDIO_CODEC == CONFIG_AUDIO_CODEC_BV32FP)
#  define CONFIG_STACK_SIZE_AUDIO   CONFIG_STACK_SIZE_AUDIO_BV32FP
# elif ((CONFIG_AUDIO_CODEC == CONFIG_AUDIO_CODEC_OPUS) && (CONFIG_OPUS_MODE == CONFIG_OPUS_MODE_CELT))
#  define CONFIG_STACK_SIZE_AUDIO   CONFIG_STACK_SIZE_AUDIO_OPUS_CELT
# elif ((CONFIG_AUDIO_CODEC == CONFIG_AUDIO_CODEC_OPUS) && (CONFIG_OPUS_MODE == CONFIG_OPUS_MODE_SILK))
#  define CONFIG_STACK_SIZE_AUDIO   CONFIG_STACK_SIZE_AUDIO_OPUS_SILK
# else
#  define CONFIG_STACK_SIZE_AUDIO   0
# endif
#else /* !CONFIG_AUDIO_ENABLED */
# define CONFIG_STACK_SIZE_AUDIO    0
#endif /* CONFIG_AUDIO_ENABLED */

#if defined(CONFIG_BOARD_NRF52832_PCA20023) || defined(CONFIG_BOARD_NRF52810_PCA20031)
# if !CONFIG_PDM_MIC_PWR_CTRL_ENABLED
#  error "Product Example devices require control over microphone power line to be enabled."
# endif
#endif

#endif /* _SR3_CONFIG_AUDIO_H */

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

/**
 * @defgroup MOD_AUDIO_FRAME Audio frame management module.
 * @ingroup input
 * @{
 * @brief Functions for managing audio frames.
 */
#ifndef __M_AUDIO_FRAME_H__
#define __M_AUDIO_FRAME_H__

#include <stdint.h>
#include "sdk_errors.h"
#include "sr3_config.h"

/**@brief Compressed audio frame representation. */
typedef struct
{
    uint8_t     data[CONFIG_AUDIO_FRAME_SIZE_BYTES];
    uint8_t     reference_count;
    uint16_t    data_size;
} m_audio_frame_t;

/**@brief Function for initializing the audio frame management module.
 *
 * @return  NRF_SUCCESS on success, otherwise error code.
 */
ret_code_t m_audio_frame_init(void);

/**@brief Function for getting an audio frame.
 *
 * @details     If an argument is given, the same frame is returned and only the reference count is increased. \
 *              If the argument is NULL, a new frame is allocated and the reference count is set to 1.
 *
 * @param[in]   p_frame Pointer to an audio frame or NULL.
 *
 * @return      Pointer to an audio frame or NULL if allocation failed.
 */
m_audio_frame_t *m_audio_frame_get(m_audio_frame_t *p_frame);

/**@brief Function for putting an audio frame.
 *
 * @details     Reference count of the given audio frame is decreased. If the reference count reaches 0, the frame is freed.
 *
 * @param[in]   p_frame Pointer to an audio frame.
 */
void m_audio_frame_put(m_audio_frame_t *p_frame);

/**@brief Function for getting current utilization statistics.
 *
 * @return  Current utilization [entries].
 */
uint8_t m_audio_frame_pool_current_utilization_get(void);

/**@brief Function for getting maximum utilization statistics.
 *
 * @return  Maximum utilization [entries].
 */
uint8_t m_audio_frame_pool_max_utilization_get(void);

#endif /* __M_AUDIO_FRAME_H__ */
/** @} */


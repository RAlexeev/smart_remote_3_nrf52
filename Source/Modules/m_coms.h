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

/** @file
 *
 * @defgroup MOD_COMS Communications module
 * @ingroup output
 * @{
 * @brief Functions for configuring the communications module.
 */
#ifndef __M_COMS_H__
#define __M_COMS_H__

#include <stdint.h>
#include "m_audio.h"
#include "m_coms_ble.h"
#include "event_bus.h"

/**@brief Module-specifc status codes
 */
typedef enum
{
    M_COMS_STATUS_SUCCESS        = NRF_SUCCESS, /**< Data successfully processed */
    M_COMS_STATUS_SD_BUFFER_FULL,               /**< SoftDevice buffer is full */
    M_COMS_STATUS_QUEUE_EMPTY,                  /**< Data queue is empty */
    M_COMS_STATUS_CANNOT_SEND,                  /**< Data cannot be sent at this time */
} m_coms_data_process_status_t;

/**@brief Services that can transmit audio data.*/
typedef enum
{
    M_COMS_AUDIO_SERVICE_HID  = (1 << 0),
    M_COMS_AUDIO_SERVICE_ATVV = (1 << 1)
} m_coms_audio_service_t;

/**@brief Initialize the communications module.
 *
 * @param[in] delete_bonds          Set to true to remove all bonds during initialization.
 *
 * @return NRF_SUCCESS on success, otherwise an error code.
 */
ret_code_t m_coms_init(bool delete_bonds);

/**@brief Free function that handles send buffers. */
typedef void (*m_coms_free_func_t)(void *);

/**@brief Send audio data.
 *
 * @param[in] p_audio_frame Audio frame to transmit.
 *
 * @return NRF_SUCCESS on success, otherwise an error code.
 */
ret_code_t m_coms_send_audio(m_audio_frame_t *p_audio_frame);

/**@brief Event Bus event handler.
 *
 * @param[in]   p_event Pointer to the event structure.
 *
 * @return      true if further processing of the given event should be abandoned.
 */
bool m_coms_event_handler(const event_t *p_event);

/**@brief Enable sending of audio data via given Service.
 *
 * @note If no audio service is enabled, @ref m_coms_send_audio will have no effect
 *
 * @param[in] service Audio Service to enable
 *
 * @return NRF_SUCCESS
 * @return NRF_ERROR_NOT_SUPPORTED
 */
void m_coms_audio_service_enable(m_coms_audio_service_t service);

/**@brief Disable sending of audio data via given Service.
 *
 * @param[in] service Audio Service to disable
 *
 * @return NRF_SUCCESS
 * @return NRF_ERROR_NOT_SUPPORTED
 */
void m_coms_audio_service_disable(m_coms_audio_service_t service);

#endif /* __M_COMS_H__ */
/** @} */

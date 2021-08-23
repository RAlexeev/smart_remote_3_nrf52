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
 * @defgroup MOD_COMS_BLE_ATVV Voice over BLE for Android implementation
 * @ingroup ble
 * @{
 * @brief
 *
 *
 */
#ifndef __M_COMS_BLE_ATVV_H__
#define __M_COMS_BLE_ATVV_H__

#include <stdint.h>
#include <stdbool.h>

#include "app_util.h"
#include "drv_ir.h"
#include "event_bus.h"
#include "m_coms.h"
#include "sdk_errors.h"

/**@brief Function for initializing Android TV Voice (ATVV) functionality.
 *
 * @retval NRF_SUCCESS or error code.
 */
ret_code_t m_coms_ble_atvv_init(void);

/**@brief Function for indicating to a peer that a user has pressed the voice search button and that audio transfer should be started.
 *
 * @details Once the voice search start message has been sent, a separate "open mic" command is expected from the peer.
 *          Audio streaming can be started once this message has been received (@ref EVT_ATVV_STATE_MIC_OPEN).
 *
 * @param[in] conn_handle Connection handle to use for the search message.
 *
 * @retval NRF_SUCCESS
 * @retval NRF_ERROR_BUSY          Already running a voice search.
 * @retval NRF_ERROR_INVALID_STATE Peer(s) has not enabled the ATVV Service.
 */
ret_code_t m_coms_ble_atvv_start_search(uint16_t conn_handle);

/**@brief Function for sending audio data.
 *
 * @note The data pointer must be updated according to how many bytes are transmitted per call to this function.
 *
 * @details If audio frame length is longer than the MTU size or ATVV-permitted transmit size, the frame will be sent
            in multiple chunks. The @p p_bytes_transmitted is set to the number of bytes of the frame sent.
 *          The ATVV frame header is automatically inserted.
 *
 * @param[in]  p_frame_buf         Pointer to the frame buffer.
 * @param[in]  frame_buf_len       Length of the frame buffer.
 * @param[out] p_bytes_transmitted Pointer to memory where number of bytes transmitted will be stored.
 * @param[out] p_status            Pointer to memory where status information will be stored.
 *
 * @retval NRF_SUCCESS No errors occured.
 */
ret_code_t m_coms_ble_atvv_audio_send(uint8_t const                * p_frame_buf,
                                      uint16_t                       frame_buf_len,
                                      uint16_t                     * p_bytes_transmitted,
                                      m_coms_data_process_status_t * p_status);

/**@brief Process control message queue.
 *
 * @details Control messages that are not sent immediately are put into a queue awaiting transmission.
 *          Call this function regularly to send control messages in the queue.
 *
 * @retval NRF_SUCCESS         Control message successfully sent.
 * @retval NRF_ERROR_NOT_FOUND No messages left in the queue.
 * @retval NRF_ERROR_BUSY      Could not send message.
 */
ret_code_t m_coms_ble_atvv_ctl_pkt_queue_process(void);

#endif /*  __M_COMS_BLE_ATVV_H__ */

/** @} */

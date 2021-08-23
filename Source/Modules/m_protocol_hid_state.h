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

#ifndef M_PROTOCOL_HID_STATE_H__
#define M_PROTOCOL_HID_STATE_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup MOD_PROTOCOL_HID_STATE HID state module
 * @ingroup system_protocol
 * @{
 * @brief Module for managing the HID protocol state.
 */

/**
 * @brief Protocol HID state item.
 * */
typedef struct
{
    uint32_t usage; /**< HID usage. */
    int16_t  value; /**< HID value. */
} m_protocol_hid_state_item_t;


/**
 * @brief Function for initializing the states of all items.
 *
 * @return NRF_SUCCESS on success, otherwise an error code.
 * */
ret_code_t m_protocol_hid_state_init(void);

/**
 * @brief Function for updating the value linked to the HID usage.
 *
 * The function updates the HID state and triggers a report generation event if BLE
 * is connected. If a connection was not made yet, information about usage change
 * may be stored in a queue if usage is queueable.
 *
 * @param[in] usage     Usage to update.
 * @param[in] report    Value linked with the usage.
 * @param[in] absolute  If true, this change event will be enqueued until a connection
 *                      is established.
 * */
void m_protocol_hid_state_update(uint32_t usage, int16_t report, bool absolute);

/**@def m_protocol_hid_state_update_rel
 * @brief Function for updating the value linked to the HID usage where value change is relative.
 * @param[in] usage      Usage to update.
 * @param[in] report     Value linked with the usage.
 *
 * @note Relative changes are used for events associated with motion and will
 *       not be enqueued in case a connection is not established.
 *       For more information, look for usage types defined in HID Usage Tables.
 * */
#define m_protocol_hid_state_update_rel(usage, report) m_protocol_hid_state_update(usage, report, false)

/**@def m_protocol_hid_state_update_abs
 * @brief Function for the value linked to the HID usage where value change is absolute.
 * @param[in] usage      Usage to update.
 * @param[in] report     Value linked with the usage.
 *
 * @note Absolute changes are used for events associated with keys and will
 *       be enqueued in case a connection is not established.
 *       For more information, look for usage types defined in HID Usage Tables.
 * */
#define m_protocol_hid_state_update_abs(usage, report) m_protocol_hid_state_update(usage, report, true)

/**
 * @brief Function for switching the HID state to connected state.
 *
 * @return NRF_SUCCESS on success, otherwise an error code.
 * */
ret_code_t m_protocol_hid_state_connect(void);

/**
 * @brief Function for switching the HID state to disconnected state.
 *
 * @return NRF_SUCCESS on success, otherwise an error code.
 * */
ret_code_t m_protocol_hid_state_disconnect(void);

/**
 * @brief Function for getting the state of the given usage.
 *
 * @param[in] usage  HID usage to find.
 *
 * @return Protocol HID state item or NULL if not found.
 * */
m_protocol_hid_state_item_t *m_protocol_hid_state_get(uint32_t usage);

/**
 * @brief Function for starting the iterator over a HID page.
 *
 * @param[in] page  Page to iterate.
 *
 * @return First item with the given page or NULL if not found.
 * */
m_protocol_hid_state_item_t const *m_protocol_hid_state_page_it_init(uint16_t page);

/**
 * @brief Function for moving the state iterator to the next item.
 *
 * @param[in] p_item  Current item.
 *
 * @return Next protocol HID item or NULL if not found.
 * */
m_protocol_hid_state_item_t const *m_protocol_hid_state_page_it_next(m_protocol_hid_state_item_t const *p_item);

#ifdef __cplusplus
}
#endif

#endif /* M_PROTOCOL_HID_STATE_H__ */
/** @} */


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
 * @defgroup KEY_COMBO_UTIL Key combo utility
 * @ingroup other
 * @{
 * @brief Functions for detecting hardware-specific key combinations.
 */
#ifndef __KEY_COMBO_UTIL_H__
#define __KEY_COMBO_UTIL_H__

#include "app_util.h"
#include "nrf.h"
#include "nrf_section.h"
#include "event_bus.h"

/**@brief Definition for key combination triggered callback*/
typedef void (*key_combo_handler_t)(void * p_context);

/**@brief Key combo description. Used to describe key combinations. */
typedef struct
{
    uint8_t             combo_keys[4];     
    uint32_t            combo_duration_ms : 30;
    uint32_t            combo_num_keys : 2;
    key_combo_handler_t handler;           
} key_combo_desc_t;

/**@brief Member key in a key combo. Used to register keys that will be used in one or more key combinations */
typedef union
{
    uint8_t  key_id; 
    uint32_t padding; // Must be Word-sized
} key_combo_member_t;

// Verify that structs are Word sized
STATIC_ASSERT((sizeof(key_combo_desc_t) % sizeof(uint32_t)) == 0);
STATIC_ASSERT((sizeof(key_combo_member_t) % sizeof(uint32_t)) == 0);

/**@brief Macro for registering key to use in subsequent combo definitions 
 * @details Key ID follows format = 0xRowColumn. E.g. 0x14 = key on row 1, column 4.
 *
 * @note Maximum 32 keys can be registered for use in key combos
 *
 * @param[in] _key_id   ID of key that will be used in one or more key combos. 
 */
#define KEY_COMBO_MEMBER_REGISTER(_key_id)                                                                         \
    NRF_SECTION_ITEM_REGISTER(combo_member_keys, static const key_combo_member_t CONCAT_2(_COMBO_MEMBER, __LINE__)) = \
        {.key_id = _key_id}

/**@brief Macro for registering one-key combo handler. 
 * @param[in] key_id_1    First key ID in key combo
 * @param[in] duration_ms Duration in milliseconds that the combo keys must be pressed to trigger the combo
 * @param[in] evt_handler Event handler to call when combo is triggered
 */
#define KEY_COMBO_ONE_KEY_REGISTER(key_id_1, duration_ms, evt_handler)                                              \
    NRF_SECTION_ITEM_REGISTER(combo_descriptions, static const key_combo_desc_t CONCAT_2(evt_handler, _combo_cb)) = \
        {                                                                                                           \
            .handler           = evt_handler,                                                                       \
            .combo_keys        = {key_id_1, 0xFF, 0xFF, 0xFF},                                                      \
            .combo_num_keys    = 1,                                                                                 \
            .combo_duration_ms = duration_ms                                                                        \
        }
   
/**@brief Macro for registering two-key combo handler. 
 * @param[in] key_id_1    First key ID in key combo
 * @param[in] key_id_2    Second key ID in key combo.
 * @param[in] duration_ms Duration in milliseconds that the combo keys must be pressed to trigger the combo
 * @param[in] evt_handler Event handler to call when combo is triggered
 */
#define KEY_COMBO_TWO_KEY_REGISTER(key_id_1, key_id_2, duration_ms, evt_handler)                                    \
    NRF_SECTION_ITEM_REGISTER(combo_descriptions, static const key_combo_desc_t CONCAT_2(evt_handler, _combo_cb)) = \
        {                                                                                                           \
            .handler           = evt_handler,                                                                       \
            .combo_keys        = {key_id_1, key_id_2, 0xFF, 0xFF},                                                  \
            .combo_num_keys    = 2,                                                                                 \
            .combo_duration_ms = duration_ms                                                                        \
        }

/**@brief Macro for registering three-key combo handler. 
 * @param[in] key_id_1    First key ID in key combo
 * @param[in] key_id_2    Second key ID in key combo
 * @param[in] key_id_3    Third key ID in key combo
 * @param[in] duration_ms Duration in milliseconds that the combo keys must be pressed to trigger the combo
 * @param[in] evt_handler Event handler to call when combo is triggered
 */
#define KEY_COMBO_THREE_KEY_REGISTER(key_id_1, key_id_2, key_id_3, duration_ms, evt_handler)                        \
    NRF_SECTION_ITEM_REGISTER(combo_descriptions, static const key_combo_desc_t CONCAT_2(evt_handler, _combo_cb)) = \
        {                                                                                                           \
            .handler           = evt_handler,                                                                       \
            .combo_keys        = {key_id_1, key_id_2, key_id_3, 0xFF},                                              \
            .combo_num_keys    = 3,                                                                                 \
            .combo_duration_ms = duration_ms                                                                        \
        }
        
/**@brief Key combo detection utility initialization
 *
 * @return NRF_SUCCESS on success, otherwise an error code.
 */
ret_code_t key_combo_util_init(void);

/**@brief Event bus event handler.
 *
 * @param[in]   p_event Pointer to the event structure.
 *
 * @return      True if further processing of the given event should be abandoned.
 */
bool key_combo_util_key_process(const event_t * p_event);

#endif /* __KEY_COMBO_UTIL_H__ */
/** @} */


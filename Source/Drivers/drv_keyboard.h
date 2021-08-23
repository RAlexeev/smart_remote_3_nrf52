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
 * @defgroup DRV_KEYBOARD Matrix keyboard driver
 * @ingroup MOD_KEYBOARD
 * @{
 * @brief This header file defines the prototypes for keyboard matrix driver functions.
 */
#ifndef __DRV_KEYBOARD_H__
#define __DRV_KEYBOARD_H__

#include <stdbool.h>
#include <stdint.h>

#include "sdk_errors.h"
#include "m_keyboard.h"

// The Keyboard key ID must fit in 1 byte.
STATIC_ASSERT((KEYBOARD_KEY_ID(-1, -1) & ~0xFF) == 0);

/**@brief Maximum number of simultaneously held keys. */
#define DRV_KEYBOARD_MAX_KEYS   6

typedef void (*drv_keyboard_event_handler_t)(uint8_t *p_pressed_keys, uint8_t num_of_pressed_keys, bool keys_blocked);

/**@brief Keyboard driver initialization.
 *
 * @param[in]   keyboard_event_handler    Handler for data read from the keyboard.
 *
 * @return  NRF_SUCCESS on success, otherwise an error code.
 */
ret_code_t drv_keyboard_init(drv_keyboard_event_handler_t keyboard_event_handler);

/**@brief Get the state of keyboard matrix.
 *
 * One scan performed as a blocking function.
 * Functional only when pulling is disabled. Returns an error otherwise.
 *
 * @param[out] p_pressed_keys             Pointer to the pressed_keys array. The array should have at least KEY_VECTOR_SIZE elements.
 * @param[out] p_number_of_pressed_keys   Number of pressed keys.
 * @param[out] p_keys_blocked             True, if some keys are blocked and cannot be read.
 *
 * @return  NRF_SUCCESS on success, otherwise an error code.
 */
ret_code_t drv_keyboard_keys_get(uint8_t *p_pressed_keys, uint8_t *p_number_of_pressed_keys, bool *p_keys_blocked);

/**@brief Enable keyboard scanning.
 *
 * @return  NRF_SUCCESS on success, otherwise an error code.
 */
ret_code_t drv_keyboard_enable(void);

/**@brief Disable keyboard scanning.
 *
 * @return  NRF_SUCCESS on success, otherwise an error code.
 */
ret_code_t drv_keyboard_disable(void);

/**@brief Shutdown keyboard and prepare it for sleeping.
 *
 * @param[in] wakeup If false, go to deepest sleep and do not prepare to wake up the system.
 *
 * @return True if the module is ready for shutdown, false otherwise.
 */
bool drv_keyboard_shutdown(bool wakeup);

#endif /* __DRV_KEYBOARD_H__ */

/** @} */

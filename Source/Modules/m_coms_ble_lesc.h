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
 * @defgroup MOD_COMS_BLE_LESC BLE LESC submodule
 * @ingroup ble
 * @{
 * @brief This module implements BLE Low Energy Secure Connections key generation.
 *
 *
 */
#ifndef __M_COMS_BLE_LESC_H__
#define __M_COMS_BLE_LESC_H__

#include "ble.h"
#include "ble_gap.h"
#include "sdk_errors.h"

/**@brief Key generation event handler. */
typedef void (*m_coms_ble_lesc_key_event_handler_t)(ble_gap_lesc_p256_pk_t *p_pk);

/**@brief Function for requesting LESC SK/PK key generation.
 *
 * @param[in]   key_evt_handler Event handler which is called when key generation has been completed.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
ret_code_t m_coms_ble_lesc_generate_key(m_coms_ble_lesc_key_event_handler_t key_evt_handler);

/**@brief Function for generating LESC OOB data.
 *
 * @param[out]  p_oob_data  Pointer to the buffer which is filled with OOB data or NULL.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
ret_code_t m_coms_ble_lesc_generate_oob_data(ble_gap_lesc_oob_data_t *p_oob_data);

/**@brief Function for getting the generated LESC OOB data.
 *
 * @param[out]  p_oob_data  Pointer to the buffer which is filled with OOB data.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
ret_code_t m_coms_ble_lesc_get_oob_data(ble_gap_lesc_oob_data_t *p_oob_data);

/**@brief Function for initializing the BLE LESC module.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
ret_code_t m_coms_ble_lesc_init(void);

#endif /*  __M_COMS_BLE_LESC_H__ */

/** @} */

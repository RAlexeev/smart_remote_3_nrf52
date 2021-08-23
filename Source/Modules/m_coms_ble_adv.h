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
 * @defgroup MOD_COMS_BLE_ADV BLE advertising submodule
 * @ingroup ble
 * @{
 * @brief This submodule includes all advertising logic: when to start advertising, and of which type.
 *
 */
#ifndef __M_COMS_BLE_ADV_H__
#define __M_COMS_BLE_ADV_H__

#include <stdbool.h>
#include <stdint.h>

#include "ble.h"
#include "m_coms_ble.h"
#include "peer_manager.h"

/**@brief Function for initializing advertising logic.
 *
 * @note Memory holding bonding parameters (p_bond_params) is expected to be persistent, and is not copied locally.
 *
 * @param[in] p_ble_params  Pointer to bonding parameters.
 *
 * @return NRF_SUCCESS on success, otherwise error code.
 */
ret_code_t m_coms_ble_adv_init(const m_coms_ble_params_t *p_ble_params);

/**@brief Function for starting advertising.
 *
 * @param[in] bondable  True, if bondable advertising should be started.
 *
 * @return NRF_SUCCESS on success, otherwise error code.
 */
ret_code_t m_coms_ble_adv_start(bool bondable);

/**@brief Function for stopping advertising.
 *
 * @return NRF_SUCCESS on success, otherwise error code.
 */
ret_code_t m_coms_ble_adv_stop(void);

/**@brief Function for checking whether any type of advertising is currently running.
 *
 * @return 'true' when any advertising is running.
 */
bool m_coms_ble_adv_running(void);

/**@brief Function for checking whether bondable advertising is currently running.
 *
 * @return 'true' when bondable advertising is running.
 */
bool m_coms_ble_adv_bondable_running(void);

#endif /* __M_COMS_BLE_ADV_H__ */

/** @} */

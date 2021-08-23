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
 * @defgroup DRV_IR IR driver
 * @ingroup MOD_IR_TX
 * @{
 * @brief IR SIRC driver.
 */
#ifndef __DRV_IR_SIRC__
#define __DRV_IR_SIRC__

#include <stdint.h>
#include "sdk_errors.h"

/**@brief Callback type. */
typedef void (*drv_ir_callback_t)(const sr3_ir_symbol_t *p_ir_symbol);

/**@brief Send an IR symbol.
 *
 * @param[in]   p_ir_symbol Pointer to an IR symbol to start or NULL to stop.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
ret_code_t drv_ir_send_symbol(const sr3_ir_symbol_t *p_ir_symbol);

/**@brief Enable the IR blaster.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
ret_code_t drv_ir_enable(void);

/**@brief Disable the IR blaster.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
ret_code_t drv_ir_disable(void);

/**@brief IR blaster initialization.
 *
 * @param[in]   m_acknowledge_handler Callback used to report a start and end of IR symbol generation.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
ret_code_t drv_ir_init(drv_ir_callback_t m_acknowledge_handler);

#endif /* __DRV_IR_SIRC__ */
/** @} */

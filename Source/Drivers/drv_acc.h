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
 * @defgroup DRV_ACC Accelerometer driver
 * @{
 * @ingroup MOD_ACC
 * @brief Accelerometer driver API.
 */
#ifndef __DRV_ACC_H__
#define __DRV_ACC_H__

#include <stdbool.h>
#include <stdint.h>

#include "sdk_errors.h"

typedef enum {
  DRV_ACC_MODE_UNDEFINED,
  DRV_ACC_MODE_WAKE_UP,
  DRV_ACC_MODE_CLICK_DETECT,
  DRV_ACC_MODE_IDLE,
} drv_acc_mode_t;

#define DRV_ACC_MODE_DEFAULT    DRV_ACC_MODE_IDLE

/**@brief Callback type. */
typedef void (*drv_acc_callback_t)(void);

/**@brief Accelerometer driver initialization.
 *
 * @note This function uses unscheduled I2C transactions - must be called only in MAIN CONTEXT.
 *
 * @param[in]   click_handler Callback used to report click events.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
ret_code_t drv_acc_init(drv_acc_callback_t click_handler);

/**@brief Set the mode of operation.
 *
 * @note This function uses unscheduled I2C transactions - must be called in MAIN CONTEXT only.
 *
 * @param[in]   mode    Mode of operation.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
ret_code_t drv_acc_mode_set(drv_acc_mode_t mode);

#endif /* __DRV_ACC_H__ */

/** @} */

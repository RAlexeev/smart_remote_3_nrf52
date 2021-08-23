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
 * @defgroup DRV_TOUCHPAD Touchpad driver
 * @{
 * @ingroup MOD_TOUCHPAD
 * @brief Touchpad driver API.
 */
#ifndef __DRV_TOUCHPAD_H__
#define __DRV_TOUCHPAD_H__

#include <stdbool.h>
#include <stdint.h>

#include "sdk_errors.h"

/**@brief Touchpad output data.  */
typedef struct
{
    int16_t x;
    int16_t y;
    int8_t  scroll;
    int8_t  pan;

    bool    tap;
} drv_touchpad_data_t;

/**@brief Touchpad read data handler. */
typedef void (*drv_touchpad_read_handler_t)(ret_code_t status, drv_touchpad_data_t *p_data);

/**@brief Touchpad driver initialization.
 * @param[in] read_handler Callback function for a data ready event.
 * @return
 * @retval NRF_SUCCESS
 * @retval NRF_ERROR_INTERNAL Bus error.
 */
ret_code_t drv_touchpad_init(drv_touchpad_read_handler_t read_handler);

/**@brief Enable touchpad to manage power consumption.
 *
 * @return NRF_SUCCESS If the enable request was scheduled, otherwise an error code.
 */
ret_code_t drv_touchpad_enable(void);

/**@brief Disable touchpad to manage power consumption.
 *
 * @return NRF_SUCCESS If the disable request was scheduled, otherwise an error code.
 */
ret_code_t drv_touchpad_disable(void);

/**@brief Schedule a touchpad read.
 *
 * @param[in] p_data Pointer to the touchpad output data.
 * @return NRF_SUCCESS If the read operation was scheduled, otherwise an error code.
 */
ret_code_t drv_touchpad_schedule_read(drv_touchpad_data_t *p_data);

#endif /* __DRV_TOUCHPAD_H__ */

/** @} */

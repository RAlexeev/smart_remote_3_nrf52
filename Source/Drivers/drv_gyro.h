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
 * @defgroup DRV_GYRO Gyro driver
 * @ingroup MOD_GYRO
 * @{
 * @brief Gyro driver API.
 */
#ifndef __DRV_GYRO_H__
#define __DRV_GYRO_H__

#include <stdbool.h>
#include <stdint.h>

#include "sdk_errors.h"
#include "AIR_MOTION_Lib.h"

/**@brief Gyroscope enabled handler */
typedef void (*drv_gyro_ready_handler_t)(void);

/**@brief Gyroscope read data handler */
typedef void (*drv_gyro_read_handler_t)(ret_code_t status,
                                        t_struct_AIR_MOTION_ProcessDeltaSamples *p_samples);

/**@brief Initializes gyroscope chip and driver.
 *
 * @param[in] ready_handler  Handler triggered when driver is ready for operation.
 * @param[in] read_handler   Handler for data read from the gyroscope.
 *
 * @return NRF_SUCCESS on success, otherwise an error code.
 */
ret_code_t drv_gyro_init(drv_gyro_ready_handler_t ready_handler,
		                 drv_gyro_read_handler_t  read_handler);

/**@brief Turns the gyroscope on.
 *
 * @return NRF_SUCCESS if the switch-on request was successfully registered. Otherwise an error code.
 */
ret_code_t drv_gyro_enable(void);

/**@brief Turns the gyroscope off.
 *
 * @return NRF_SUCCESS if the switch-off request was successfully registered. Otherwise an error code.
 */
ret_code_t drv_gyro_disable(void);

/**@brief Schedules a gyroscope read.
 *
 * @param[out]  p_samples   Pointer to the structure that will be filled up by data.
 *
 * @return NRF_SUCCESS if the read request was successfully registered. Otherwise an error code.
 */
ret_code_t drv_gyro_schedule_read(t_struct_AIR_MOTION_ProcessDeltaSamples* p_samples);

#endif /* __DRV_GYRO_H__ */

/** @} */

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
 * @defgroup MOD_GYRO Gyroscope module
 * @ingroup input
 * @{
 * @brief Functions for configuring the gyro module.
 */
#ifndef __M_GYRO__
#define __M_GYRO__

#include <stdint.h>
#include "event_bus.h"

/**@brief Function for initializing the gyro module.
 *
 * @param[in] force_calibration If true, the gyroscope calibration will be performed even if valid calibration data is present in the flash.
 *
 * @return NRF_SUCCESS on success, otherwise an error code.
 */
ret_code_t m_gyro_init(bool force_calibration);

/**@brief Function for enabling the gyro module.
 *
 * @return NRF_SUCCESS on success, otherwise an error code.
 */
ret_code_t m_gyro_enable(void);

/**@brief Function for disabling the gyro module.
 *
 * @return NRF_SUCCESS on success, otherwise an error code.
 */
ret_code_t m_gyro_disable(void);

/**@brief Function for initiating gyro calibration.
 *
 * @note Gyro will be automatically turned on if not already enabled.
 *
 * @return NRF_SUCCESS on success, otherwise an error code.
 */
ret_code_t m_gyro_calibrate(void);

/**@brief Event bus event handler.
 *
 * @param[in]   p_event Pointer to the event structure.
 *
 * @return      True if further processing of the given event should be abandoned.
 */
bool m_gyro_event_handler(const event_t *p_event);

#endif /* __M_GYRO__ */
/** @} */


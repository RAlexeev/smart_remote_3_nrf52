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
 * @defgroup DRV_LEDS LED drivers
 * @{
 * @ingroup MOD_LEDS
 * @brief Development Kit LED drivers.
 */
#ifndef __DRV_LEDS_H__
#define __DRV_LEDS_H__

#include <stdint.h>
#include "sdk_errors.h"
#include "sr3_config.h"

/**@brief LED driver initialization.
 *
 * @return NRF_SUCCESS on success, otherwise an error code.
 */
ret_code_t drv_leds_init(void);

/**@brief Set LEDs.
 *
 * @param[in] leds_mask Bit mask of the LEDs to set.
 * @return    NRF_SUCCESS on success, otherwise an error code.
 */
ret_code_t drv_leds_set(uint8_t leds_mask);

/**@brief Clear LEDs.
 *
 * @param[in] leds_mask Bit mask of the LEDs to clear.
 * @return    NRF_SUCCESS on success, otherwise an error code.
 */
ret_code_t drv_leds_clr(uint8_t leds_mask);

/**@brief Get the mask of all LEDs.
 *
 * @return  Mask covering all supported LEDs.
 */
uint8_t drv_leds_all(void);

#endif /* __DRV_LEDS_H__ */

/** @} */

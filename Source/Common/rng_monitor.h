/**
 * Copyright (c) 2017 - 2018, Nordic Semiconductor ASA
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

#ifndef __RNG_MONITOR_H__
#define __RNG_MONITOR_H__

#include <stdbool.h>
#include <stdint.h>

#include "nrf.h"
#include "sdk_errors.h"

/**
* @defgroup RNG_MONITOR Random Number Generator Monitor.
* @{
* @ingroup other
* @brief RNG Monitor periodically checks RNG pool and notifies components waiting for entropy when it is avaiable.
*/

/**@brief Function handling RNG Monitor notifications
 *
 * @param[in]   p_context               Pointer user defined context
 */
typedef void (*rng_monitor_notification_handler_t)(void *p_context);

/**@brief Initializes RNG Monitor.
 *
 * @note The nrf_drv_rng driver has to be initialized before calling this function.
 *
 * @return NRF_SUCCESS on success, otherwise an error code.
 */
ret_code_t rng_monitor_init(void);

/**@brief Request radnom data read.
 *
 * @param[in] random_bytes_requested    Number of random bytes requested.
 * @param[in] notification_handler      Function which will be called when there will be enough random bytes in RNG pool.
 * @param[in] p_notification_context    Context passed to notification handler.
 *
 * @return NRF_SUCCESS on success, otherwise an error code.
 */
ret_code_t rng_monitor_request(uint8_t random_bytes_requested,
                               rng_monitor_notification_handler_t notification_handler,
                               void *p_notification_context);
/**
 *@}
 **/

#endif /* __RNG_MONITOR_H__ */

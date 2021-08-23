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
/**@file
 *
 * @defgroup nrf_dfu_settings DFU settings
 * @{
 * @ingroup  nrf_dfu
 */

#ifndef NRF_DFU_SETTINGS_H__
#define NRF_DFU_SETTINGS_H__

#include <stdint.h>
#include "nrf_dfu_types.h"
#include "nrf_dfu_flash.h"

#ifdef __cplusplus
extern "C" {
#endif


/**@brief   Global settings.
 *
 * @note Using this variable is not thread-safe.
 *
 */
extern nrf_dfu_settings_t s_dfu_settings;


/**@brief   Function for writing DFU settings to flash.
 *
 * @retval  NRF_SUCCESS         If the write process was successfully initiated.
 * @retval  NRF_ERROR_INTERNAL  If a flash error occurred.
 */
ret_code_t nrf_dfu_settings_write(void);


/**@brief   Function for initializing the DFU settings module.
 *
 * @note FLASH will be initialized assuming SD disabled configuration. NVM driver
 *       will be used directly.
 */
void nrf_dfu_settings_init(void);


/** @brief Function for copying peer data from DFU settings to RAM.
 *
 * @param[in,out] p_data    Structure to copy peer data to.
 *
 * @retval  NRF_SUCCESS     Peer data was successfully copied.
 * @retval  NRF_ERROR_NULL  p_data was NULL.
 */
ret_code_t nrf_dfu_settings_peer_data_copy(nrf_dfu_peer_data_t * p_data);


/** @brief Function for copying the advertisement name from DFU settings to RAM.
 *
 * @param[in,out]   p_adv_name  Structure to copy the new advertisement name to.
 *
 * @retval  NRF_SUCCESS     Advertisement name was successfully copied.
 * @retval  NRF_ERROR_NULL  p_adv_name was NULL.
 */
ret_code_t nrf_dfu_settings_adv_name_copy(nrf_dfu_adv_name_t * p_adv_name);


/** @brief Function for validating data shared by application in DFU settings.
 *
 * @return  True if shared data is validated by CRC, false if not.
 */
bool nrf_dfu_settings_shared_data_is_valid(void);


/** @brief Function for erasing shared data in DFU settings.
 *
 * @return NRF_SUCCESS is shared data was successfully erased.
 */
ret_code_t nrf_dfu_settings_shared_data_erase(void);


#ifdef __cplusplus
}
#endif

#endif // NRF_DFU_SETTINGS_H__

/**@} */

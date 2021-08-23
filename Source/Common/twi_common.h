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

#ifndef __TWI_COMMON_H__
#define __TWI_COMMON_H__

#include <stdbool.h>
#include <stdint.h>

#include "nrf.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_twi_mngr.h"
#include "sr3_config.h"
#include "resources.h"

/**
* @defgroup TWI_COMMON TWI common procedures
* @{
* @ingroup other
* @brief Common TWI functions.
*/


/**@brief Initialize TWI.
 *
 * Function fills in bus configuration with correct data and initializes TWI
 * manager.
 *
 * @return NRF_SUCCESS on success, otherwise an error code.
 */
ret_code_t twi_init(void);


/**@brief Perform a single synchronous TWI transaction.
 *
 * @param[in] p_bus_config    Required TWI bus configuration.
 * @param[in] p_transfers     Pointer to an array of transfers to be performed.
 * @param     transfer_count  Number of transfers to be performed.
 *
 * @return NRF_SUCCESS on success, otherwise an error code.
 */
ret_code_t twi_perform(nrf_drv_twi_config_t const    * p_bus_config,
                       nrf_twi_mngr_transfer_t const * p_transfers,
                       uint8_t                         transfer_count);


/**@brief Perform a single synchronous TWI transaction.
 *
 * @param[in] p_transaction  Transaction to be scheduled by TWI master.
 *
 * @return NRF_SUCCESS on success, otherwise an error code.
 */
ret_code_t twi_schedule(nrf_twi_mngr_transaction_t const * p_transaction);


/**@brief Reads from the register of the TWI device.
 *
 * @param[in]  p_bus_config  Required TWI bus configuration.
 * @param[in]  device_addr   Device address.
 * @param[in]  register_addr Register address.
 * @param[out] p_value       Pointer to the variable to which the register value will be written.
 *
 * @return NRF_SUCCESS on success, otherwise an error code.
 */
ret_code_t twi_register_read(nrf_drv_twi_config_t const * p_bus_config,
                             uint8_t                      device_addr,
                             uint8_t                      register_addr,
                             uint8_t                    * p_value);


/**@brief Writes to the register of the TWI device.
 *
 * @param[in] p_bus_config   Required TWI bus configuration.
 * @param[in] device_addr    Device address.
 * @param[in] register_addr  Register address.
 * @param[in] value          Data to write.
 *
 * @return NRF_SUCCESS on success, otherwise an error code.
 */
ret_code_t twi_register_write(nrf_drv_twi_config_t const * p_bus_config,
                              uint8_t                      device_addr,
                              uint8_t                      register_addr,
                              uint8_t                      value);


/**@brief Writes to multiple registers of the TWI device.
 *
 * @param[in] p_bus_config          Required TWI bus configuration.
 * @param[in] device_addr           Device address.
 * @param[in] p_reg_val_array       Array of the { register address, value } pairs.
 * @param[in] reg_val_array_size    Number of the { register address, value } pairs to write.
 * @param[in] perform_verification  If true, then the register content will be verified after write.
 *
 * @return NRF_SUCCESS on success, otherwise an error code.
 */
ret_code_t twi_register_bulk_write(nrf_drv_twi_config_t const * p_bus_config,
                                   uint8_t                      device_addr,
                                   const uint8_t                p_reg_val_array[][2],
                                   unsigned int                 reg_val_array_size,
                                   bool                         perform_verification);

/**
 *@}
 **/

#endif /* __TWI_COMMON_H__ */

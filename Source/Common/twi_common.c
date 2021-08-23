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


#include <stdbool.h>
#include <stdint.h>

#include "nrf_delay.h"
#include "nrf_twi_mngr.h"

#include "twi_common.h"
#include "resources.h"
#include "sr3_config.h"

#if (CONFIG_TWI0_ENABLED) || (CONFIG_TWI1_ENABLED)

#if CONFIG_TWI0_ENABLED
    #define TWI_COMMON_DEFAULT_BUS 0
#elif CONFIG_TWI1_ENABLED
    #define TWI_COMMON_DEFAULT_BUS 1
#else
    #error "No TWI bus is available"
#endif


/* Perform sanity checks. */
STATIC_ASSERT(NRF_TWI_MNGR_ENABLED);
STATIC_ASSERT(TWI_ENABLED);
STATIC_ASSERT(TWI0_ENABLED);

#if CONFIG_TWI0_ENABLED
STATIC_ASSERT(IS_IO_VALID(CONFIG_IO_TWI0_SCL) && IS_IO_VALID(CONFIG_IO_TWI0_SDA));
#endif
#if CONFIG_TWI1_ENABLED
STATIC_ASSERT(IS_IO_VALID(CONFIG_IO_TWI1_SCL) && IS_IO_VALID(CONFIG_IO_TWI1_SDA));
#endif

STATIC_ASSERT(((CONFIG_TWI_INIT_DELAY) >= 0) && ((CONFIG_TWI_INIT_DELAY) <= 1000));

/**@brief TWI manager instance. */
NRF_TWI_MNGR_DEF(twi_mngr, CONFIG_TWI_QSIZE, 0);

/**@brief Configuration of TWI buses. */
const nrf_drv_twi_config_t g_twi_bus_config[] = {
#if CONFIG_TWI0_ENABLED
    /* TWI bus 0. */
    [0] = {
        .frequency          = (nrf_twi_frequency_t)TWI_DEFAULT_CONFIG_FREQUENCY,
        .interrupt_priority = TWI_DEFAULT_CONFIG_IRQ_PRIORITY,
        .clear_bus_init     = TWI_DEFAULT_CONFIG_CLR_BUS_INIT,
        .hold_bus_uninit    = CONFIG_TWI0_UP_IN_SYSOFF_STATE,
        .scl                = CONFIG_IO_TWI0_SCL,
        .sda                = CONFIG_IO_TWI0_SDA,
    },
#endif
#if CONFIG_TWI1_ENABLED
    /* TWI bus 1. */
    [1] = {
        .frequency          = (nrf_twi_frequency_t)TWI_DEFAULT_CONFIG_FREQUENCY,
        .interrupt_priority = TWI_DEFAULT_CONFIG_IRQ_PRIORITY,
        .clear_bus_init     = TWI_DEFAULT_CONFIG_CLR_BUS_INIT,
        .hold_bus_uninit    = CONFIG_TWI1_UP_IN_SYSOFF_STATE,
        .scl                = CONFIG_IO_TWI1_SCL,
        .sda                = CONFIG_IO_TWI1_SDA,
    }
#endif
};

/**@brief Pointer to TWI manager instance. */
nrf_twi_mngr_t const * const g_twi_mngr = &twi_mngr;


ret_code_t twi_init(void)
{
    /* Initialize TWI manager. */
    ret_code_t status = nrf_twi_mngr_init(g_twi_mngr, &g_twi_bus_config[TWI_COMMON_DEFAULT_BUS]);

#if CONFIG_TWI_INIT_DELAY
    if (status == NRF_SUCCESS)
    {
        /* Wait for a moment. Some TWI devices do not respond
         * if they are accessed right after bus initialization. */
        nrf_delay_ms(CONFIG_TWI_INIT_DELAY);
    }
#endif

    return status;
}


ret_code_t twi_perform(nrf_drv_twi_config_t const    * p_bus_config,
                       nrf_twi_mngr_transfer_t const * p_transfers,
                       uint8_t                         transfer_count)
{
    ASSERT((p_bus_config >= &g_twi_bus_config[0]) &&
           (p_bus_config <= &g_twi_bus_config[ARRAY_SIZE(g_twi_bus_config) - 1]));

#if NRF_PWR_MGMT_ENABLED
    void (*idle_func)(void) = nrf_pwr_mgmt_run;
#else
    void (*idle_func)(void) = NULL;
#endif

    return nrf_twi_mngr_perform(g_twi_mngr, p_bus_config, p_transfers, transfer_count, idle_func);
}


ret_code_t twi_schedule(nrf_twi_mngr_transaction_t const * p_transaction)
{
    ASSERT((p_transaction->p_required_twi_cfg >= &g_twi_bus_config[0]) &&
           (p_transaction->p_required_twi_cfg <= &g_twi_bus_config[ARRAY_SIZE(g_twi_bus_config) - 1]));

    return nrf_twi_mngr_schedule(g_twi_mngr, p_transaction);
}


ret_code_t twi_register_read(nrf_drv_twi_config_t const * p_bus_config,
                             uint8_t                      device_addr,
                             uint8_t                      register_addr,
                             uint8_t                    * p_value)
{
    nrf_twi_mngr_transfer_t transfers[2];

    transfers[0].p_data    = &register_addr;
    transfers[0].length    = sizeof(register_addr);
    transfers[0].operation = NRF_TWI_MNGR_WRITE_OP(device_addr);
    transfers[0].flags     = NRF_TWI_MNGR_NO_STOP;

    transfers[1].p_data    = p_value;
    transfers[1].length    = sizeof(*p_value);
    transfers[1].operation = NRF_TWI_MNGR_READ_OP(device_addr);
    transfers[1].flags     = 0;

    return twi_perform(p_bus_config, transfers, ARRAY_SIZE(transfers));
}


ret_code_t twi_register_write(nrf_drv_twi_config_t const * p_bus_config,
                              uint8_t                      device_addr,
                              uint8_t                      register_addr,
                              uint8_t                      value)
{
    nrf_twi_mngr_transfer_t transfers[1];
    uint8_t data_to_write[2];

    data_to_write[0] = register_addr;
    data_to_write[1] = value;

    transfers[0].p_data    = data_to_write;
    transfers[0].length    = sizeof(data_to_write);
    transfers[0].operation = NRF_TWI_MNGR_WRITE_OP(device_addr);
    transfers[0].flags     = 0;

    return twi_perform(p_bus_config, transfers, ARRAY_SIZE(transfers));
}


ret_code_t twi_register_bulk_write(nrf_drv_twi_config_t const * p_bus_config,
                                   uint8_t                      device_addr,
                                   const uint8_t                p_reg_val_array[][2],
                                   unsigned int                 reg_val_array_size,
                                   bool                         perform_verification)
{
    for (unsigned int i = 0; i < reg_val_array_size; i++)
    {
        ret_code_t status = twi_register_write(p_bus_config, device_addr, p_reg_val_array[i][0], p_reg_val_array[i][1]);
        if (status != NRF_SUCCESS)
        {
            return status;
        }

        if (perform_verification)
        {
            uint8_t read;

            status = twi_register_read(p_bus_config, device_addr, p_reg_val_array[i][0], &read);
            if (status != NRF_SUCCESS)
            {
                return status;
            }

            if (p_reg_val_array[i][1] != read)
            {
                return NRF_ERROR_INTERNAL;
            }
        }
    }

    return NRF_SUCCESS;
}


#if CONFIG_PWR_MGMT_ENABLED
/**@brief Flush TWI transactions and shutdown TWI modules before going to sleep.
 *
 * @param[in] wakeup If false, go to deepest sleep and do not prepare to wake up the system.
 *
 * @return True if the TWI subsystem is ready for shutdown, false otherwise.
 */
static bool twi_shutdown(nrf_pwr_mgmt_evt_t event)
{
    while (!nrf_twi_mngr_is_idle(g_twi_mngr))
    {
        nrf_pwr_mgmt_run();
    }
    nrf_twi_mngr_uninit(g_twi_mngr);

    return true;
}
NRF_PWR_MGMT_HANDLER_REGISTER(twi_shutdown, SHUTDOWN_PRIORITY_STATISTICS);
#endif /* CONFIG_PWR_MGMT_ENABLED */

#endif /* CONFIG_TWI0_ENABLED || CONFIG_TWI1_ENABLED */

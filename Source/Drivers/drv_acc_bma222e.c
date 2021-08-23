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

#include <string.h>
#ifndef __ICCARM__
#include <alloca.h>
#endif

#include "nordic_common.h"
#include "nrf.h"
#include "nrf_assert.h"
#include "nrf_error.h"
#include "nrf_delay.h"

#include "app_debug.h"
#include "app_error.h"
#include "app_gpiote.h"

#include "drv_acc_bma222e.h"
#include "drv_acc.h"

#include "resources.h"
#include "twi_common.h"
#include "sr3_config.h"

#if CONFIG_ACC_ENABLED && (CONFIG_ACC_DRIVER == CONFIG_ACC_DRIVER_BMA222E)

#if CONFIG_ACC_CLICK_THRESHOLD > 31
#error Driver only supports CONFIG_ACC_CLICK_THRESHOLD value in range [0,31]
#endif

#define NRF_LOG_MODULE_NAME drv_acc
#define NRF_LOG_LEVEL CONFIG_ACC_DRV_LOG_LEVEL
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

// Check pin configuration.
STATIC_ASSERT(IS_IO_VALID(CONFIG_IO_ACC_IRQ));

static drv_acc_mode_t           m_acc_mode = DRV_ACC_MODE_UNDEFINED;
static app_gpiote_user_id_t     m_acc_gpiote;

#if CONFIG_ACC_USE_CLICK_DETECTION
#error Click detection not yet supported by this driver
#endif

/**@brief Reads one or more consecutive registers from the device.
 *
 * @note This function uses unscheduled TWI transactions - must be called only in MAIN CONTEXT.
 *
 * @param[in]  reg      Register address.
 * @param[out] contents Pointer to data buffer.
 * @param[in]  num_regs Number of registers to be read.
 *
 * @return NRF_SUCCESS on success, otherwise an error code.
 */
static ret_code_t bma222e_read_regs(uint8_t reg, uint8_t* contents, uint8_t num_regs)
{
    nrf_twi_mngr_transfer_t transfers[2];

    if (num_regs > 1)
    {
        // Setting auto increment
        reg |= BIT_7;
    }

    transfers[0].p_data     = &reg;
    transfers[0].length     = 1;
    transfers[0].operation  = NRF_TWI_MNGR_WRITE_OP(BMA222E_TWI_ADDRESS);
    transfers[0].flags      = NRF_TWI_MNGR_NO_STOP;

    transfers[1].p_data     = contents;
    transfers[1].length     = num_regs;
    transfers[1].operation  = NRF_TWI_MNGR_READ_OP(BMA222E_TWI_ADDRESS);
    transfers[1].flags      = 0;

    return twi_perform(&g_twi_bus_config[CONFIG_ACC_TWI_BUS], transfers, ARRAY_SIZE(transfers));
}

/**@brief Writes one or more consecutive registers to the device.
 * This function uses unscheduled TWI transactions - must be called only in MAIN CONTEXT.
 *
 * @param[in] reg      Register address.
 * @param[in] contents Data to write.
 * @param[in] num_regs Length of data/number of registers that should be written.
 *
 * @return NRF_SUCCESS on success, otherwise error code.
 */
static ret_code_t bma222e_write_regs(uint8_t reg, uint8_t* contents, uint8_t num_regs)
{
    nrf_twi_mngr_transfer_t transfers[1];

#ifndef __ICCARM__
    uint8_t *write_buffer = alloca(num_regs + 1);
#else
    uint8_t write_buffer[6];

    ASSERT(sizeof(write_buffer) >= (num_regs + 1));
#endif

    if (num_regs > 1)
    {
        // Setting auto increment
        reg |= BIT_7;
    }

    write_buffer[0] = reg;
    memcpy(&write_buffer[1], contents, num_regs);

    transfers[0].p_data     = write_buffer;
    transfers[0].length     = num_regs + 1;
    transfers[0].operation  = NRF_TWI_MNGR_WRITE_OP(BMA222E_TWI_ADDRESS);
    transfers[0].flags      = 0;

    return twi_perform(&g_twi_bus_config[CONFIG_ACC_TWI_BUS], transfers, ARRAY_SIZE(transfers));
}

/**@brief Resets the memory contents to its default state.
 *
 * @return NRF_SUCCESS on success, otherwise an error code.
 */
static ret_code_t bma222e_reset(void)
{
    ret_code_t status;
    uint8_t reg_val;

    // Reboot
    reg_val = BMA2x2_ENABLE_SOFT_RESET_VALUE;
    status = bma222e_write_regs(BMA222E_BGW_SOFTRESET, &reg_val, 1);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    // Waits longer than Wakeup Time(1.8ms).
    nrf_delay_ms(2);

    return NRF_SUCCESS;
}

/**@brief Read and verify ID.
 *
 * @return NRF_SUCCESS if bma222e chip was found, otherwise an error code.
 */
static ret_code_t bma222e_verify_id(void)
{
    ret_code_t status;
    uint8_t reg_val;

    status = bma222e_read_regs(BMA222E_BGW_CHIPID, &reg_val, sizeof(reg_val));
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    return (reg_val == BMA222E_I_AM_BMA222E) ? NRF_SUCCESS : NRF_ERROR_NOT_FOUND;
}

/**@brief Reads interrupt source registers, which clears interrupt flag.
 *
 * @return NRF_SUCCESS on success, otherwise an error code.
 */
static ret_code_t bma222e_int_clear(void)
{
    ret_code_t status;
    uint8_t reg_val = BMA2x2_RESET_INTR_MSK;

    status = bma222e_write_regs(BMA222E_INT_RST_LATCH, &reg_val, 1);
    if (status != NRF_SUCCESS)
    {
        return status;
    }
    return NRF_SUCCESS;
}

/**@brief Enables selected interrupts.
 *
 * @param[in] int_en_0  Contents of the INT_EN_0 register.
 * @param[in] int_en_1  Contents of the INT_EN_1 register.
 * @param[in] int_en_2  Contents of the INT_EN_2 register.
 *
 * @return NRF_SUCCESS on success, otherwise an error code.
 */
static ret_code_t bma222e_int_enable(uint8_t int_en_0, uint8_t int_en_1, uint8_t int_en_2)
{
    ret_code_t status;
    uint8_t    reg_val;

    reg_val = int_en_0;
    status = bma222e_write_regs(BMA222E_INT_EN_0, &reg_val, 1);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    reg_val = int_en_1;
    status = bma222e_write_regs(BMA222E_INT_EN_1, &reg_val, 1);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

	  reg_val = int_en_2;
    status = bma222e_write_regs(BMA222E_INT_EN_2, &reg_val, 1);
    return status;
}

/**@brief Sets the accelerometer in idle mode.
 *
 * @return NRF_SUCCESS on success, otherwise an error code.
 */
static ret_code_t bma222e_idle_mode_set(void)
{
    ret_code_t status;
    uint8_t reg_val;

    NRF_LOG_INFO("DRV_ACC_MODE_IDLE");

    // Disable all interrupts.
    status = bma222e_int_enable(0x00, 0x00, 0x00);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    // Update configuration.
    reg_val = ((0U << BMA2x2_MODE_CTRL_POS)  & BMA2x2_MODE_CTRL_MSK) |
              ((0U << BMA2x2_SLEEP_DURN_POS) & BMA2x2_SLEEP_DURN_MSK); 
    status = bma222e_write_regs(BMA222E_PMU_LPW, &reg_val, 1);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    reg_val = BMA2x2_ENABLE_INTR1_PAD_SLOPE_MSK;
    status = bma222e_write_regs(BMA222E_INT_MAP_0, &reg_val, 1);
    if (status != NRF_SUCCESS)
    {
        return status;
    }
    reg_val = BMA2x2_INTR1_PAD_ACTIVE_LEVEL_MSK;
    status = bma222e_write_regs(BMA222E_INT_OUT_CTRL, &reg_val, 1);
    if (status != NRF_SUCCESS)
    {
        return status;
    }
    reg_val = 0x00;
    status = bma222e_write_regs(BMA222E_INT_RST_LATCH, &reg_val, 1);
    if (status != NRF_SUCCESS)
    {
        return status;
    }
    reg_val = CONFIG_ACC_WAKEUP_THRESHOLD & 0xFF; 
    status = bma222e_write_regs(BMA222E_INT_6, &reg_val, 1);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    // Clear interrupts.
    status = bma222e_int_clear();
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    // Set Power Down mode.
    reg_val = ((2U      << BMA2x2_MODE_CTRL_POS) & BMA2x2_MODE_CTRL_MSK) |
              ((0x0B << BMA2x2_SLEEP_DURN_POS) & BMA2x2_SLEEP_DURN_MSK);
    return bma222e_write_regs(BMA222E_PMU_LPW, &reg_val, 1);
}

/**@brief Sets the accelerometer in wakeup mode.
 *
 * @return NRF_SUCCESS on success, otherwise an error code.
 */
static ret_code_t bma222e_wakeup_mode_set(void)
{
    ret_code_t status;
    uint8_t reg_val;

    NRF_LOG_INFO("DRV_ACC_MODE_WAKE_UP");

    // Disable all interrupts.
    status = bma222e_int_enable(0x00, 0x00, 0x00);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    // Update configuration.
    reg_val = CONFIG_ACC_WAKEUP_THRESHOLD & 0xFF;
    status = bma222e_write_regs(BMA222E_INT_6, &reg_val, 1);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    // A safe way to change parameters of an enabled interrupt is to keep the following sequence:
    // disable the desired interrupt, change parameters, wait for at least 10ms, and then re-enable the desired interrupt.
    nrf_delay_ms(10);

    // Clear interrupts.
    status = bma222e_int_clear();
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    // Enable selected interrupts.
    return bma222e_int_enable(BMA2x2_ENABLE_SLOPE_Z_INTR_MSK |
                              BMA2x2_ENABLE_SLOPE_Y_INTR_MSK |
                              BMA2x2_ENABLE_SLOPE_X_INTR_MSK,
                              0x00, 0x00);
}

/**@brief Sets accelerometer in the given mode.
 *
 * @param[in]   mode    Mode of accelerometer operation.
 *
 * @return NRF_SUCCESS on success, otherwise an error code.
 */
static ret_code_t bma222e_mode_set(drv_acc_mode_t mode)
{
    switch (mode)
    {
        case DRV_ACC_MODE_WAKE_UP:
            return bma222e_wakeup_mode_set();
        case DRV_ACC_MODE_IDLE:
            return bma222e_idle_mode_set();
        default:
            return NRF_ERROR_NOT_SUPPORTED;
    }

    /* Not reached */
}

/**@brief Enables PIO interrupt. */
static ret_code_t bma222e_enable_pio_interrupts(void)
{
    return app_gpiote_user_enable(m_acc_gpiote);
}

/**@brief Disables PIO interrupt. */
static ret_code_t bma222e_disable_pio_interrupts(void)
{
    return app_gpiote_user_disable(m_acc_gpiote);
}


/**@brief Accelerometer Interrupt handler.
 */
static void m_acc_interrupt_handler(uint32_t const *p_event_pins_low_to_high,
                                   uint32_t const *p_event_pins_high_to_low)
{
    NRF_LOG_DEBUG("<Accelerometer Interrupt>");
}

/*--- drv_acc.h API ---*/

ret_code_t drv_acc_mode_set(drv_acc_mode_t mode)
{
    ret_code_t status = NRF_SUCCESS;

    if (m_acc_mode != mode)
    {
        status = bma222e_disable_pio_interrupts();
        if (status != NRF_SUCCESS)
        {
            return status;
        }

        status = bma222e_mode_set(mode);
        if (status != NRF_SUCCESS)
        {
            return status;
        }

        switch (mode)
        {
            case DRV_ACC_MODE_WAKE_UP:
                status = bma222e_enable_pio_interrupts();
                if (status != NRF_SUCCESS)
                {
                    return status;
                }
                break;

            default:
                /* Do nothing */
                break;
        }

        m_acc_mode = mode;
    }

    return status;
}

ret_code_t drv_acc_init(drv_acc_callback_t click_handler)
{
    uint32_t low_to_high_mask = (1 << CONFIG_IO_ACC_IRQ);
    uint32_t high_to_low_mask = (0 << CONFIG_IO_ACC_IRQ);
    ret_code_t status;

    UNUSED_PARAMETER(click_handler);

    status = app_gpiote_user_register(&m_acc_gpiote,
                                      &low_to_high_mask,
                                      &high_to_low_mask,
                                      m_acc_interrupt_handler);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    status = bma222e_verify_id();
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    status = bma222e_reset();
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    return drv_acc_mode_set(DRV_ACC_MODE_DEFAULT);
}

#endif /* CONFIG_ACC_ENABLED && (CONFIG_ACC_DRIVER == CONFIG_ACC_DRIVER_BMA222E) */

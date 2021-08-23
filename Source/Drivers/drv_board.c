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

#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "drv_board.h"
#include "sr3_config.h"

#define ASSERT_SIGNAL(pin, act_low, configure)          \
do {                                                    \
    if (IS_IO_VALID((pin)))                             \
    {                                                   \
        if ((act_low))                                  \
        {                                               \
            nrf_gpio_pin_clear((pin));                  \
        }                                               \
        else                                            \
        {                                               \
            nrf_gpio_pin_set((pin));                    \
        }                                               \
                                                        \
        if ((configure))                                \
        {                                               \
             nrf_gpio_cfg_output((pin));                \
        }                                               \
    }                                                   \
} while (0)

#define DEASSERT_SIGNAL(pin, act_low, configure)        \
do {                                                    \
    if (IS_IO_VALID((pin)))                             \
    {                                                   \
        if ((act_low))                                  \
        {                                               \
            nrf_gpio_pin_set((pin));                    \
        }                                               \
        else                                            \
        {                                               \
            nrf_gpio_pin_clear((pin));                  \
        }                                               \
                                                        \
        if ((configure))                                \
        {                                               \
             nrf_gpio_cfg_output((pin));                \
        }                                               \
    }                                                   \
} while (0)

#define PIN_MASK(pin)   (IS_IO_VALID(pin) ? (1u << (pin)) : 0)

#if CONFIG_PWR_MGMT_ENABLED
/* Pins which should retain configuration during System OFF state. */
static const uint32_t m_protected_pins = 0
#if (CONFIG_ACC_ENABLED && CONFIG_ACC_WAKEUP_SOURCE)
    | PIN_MASK(CONFIG_IO_ACC_IRQ)
#endif /* (CONFIG_ACC_ENABLED && CONFIG_ACC_WAKEUP_SOURCE) */
    | PIN_MASK(CONFIG_IO_BOARD_PWR_CTRL)
    | PIN_MASK(CONFIG_IO_BUZZER)
    | PIN_MASK(CONFIG_IO_IR_RX_ENABLE)
    | PIN_MASK(CONFIG_IO_IR_TX_LED)
#if CONFIG_KBD_ENABLED && (CONFIG_KBD_DRIVER == CONFIG_KBD_DRIVER_GPIO) && CONFIG_KBD_WAKEUP_SOURCE
    | PIN_MASK(CONFIG_IO_KEY_COL_0)
    | PIN_MASK(CONFIG_IO_KEY_COL_1)
    | PIN_MASK(CONFIG_IO_KEY_COL_2)
    | PIN_MASK(CONFIG_IO_KEY_COL_3)
    | PIN_MASK(CONFIG_IO_KEY_COL_4)
    | PIN_MASK(CONFIG_IO_KEY_COL_5)
    | PIN_MASK(CONFIG_IO_KEY_COL_6)
    | PIN_MASK(CONFIG_IO_KEY_COL_7)
    | PIN_MASK(CONFIG_IO_KEY_ROW_0)
    | PIN_MASK(CONFIG_IO_KEY_ROW_1)
    | PIN_MASK(CONFIG_IO_KEY_ROW_2)
    | PIN_MASK(CONFIG_IO_KEY_ROW_3)
    | PIN_MASK(CONFIG_IO_KEY_ROW_4)
    | PIN_MASK(CONFIG_IO_KEY_ROW_5)
    | PIN_MASK(CONFIG_IO_KEY_ROW_6)
    | PIN_MASK(CONFIG_IO_KEY_ROW_7)
#endif /* (CONFIG_KBD_ENABLED && (CONFIG_KBD_DRIVER == CONFIG_KBD_DRIVER_GPIO) && CONFIG_KBD_WAKEUP_SOURCE) */
    | PIN_MASK(CONFIG_IO_LED_1)
    | PIN_MASK(CONFIG_IO_LED_2)
    | PIN_MASK(CONFIG_IO_LED_3)
    | PIN_MASK(CONFIG_IO_LED_4)
    | PIN_MASK(CONFIG_IO_LED_PWR_CTRL)
    | PIN_MASK(CONFIG_IO_PDM_MIC_PWR_CTRL)
#if CONFIG_TWI0_UP_IN_SYSOFF_STATE
    | PIN_MASK(CONFIG_IO_TWI0_SCL)
    | PIN_MASK(CONFIG_IO_TWI0_SDA)
#endif /* CONFIG_TWI0_UP_IN_SYSOFF_STATE */
#if CONFIG_TWI1_UP_IN_SYSOFF_STATE
    | PIN_MASK(CONFIG_IO_TWI1_SCL)
    | PIN_MASK(CONFIG_IO_TWI1_SDA)
#endif /* CONFIG_TWI1_UP_IN_SYSOFF_STATE */
    | PIN_MASK(CONFIG_IO_TP_DCDC_DISCHARGE)
    | PIN_MASK(CONFIG_IO_TP_DCDC_SHUTDOWN)
#if (CONFIG_DFU_ENABLED && (CONFIG_DFU_IMAGE_DEST==2))
    | PIN_MASK(CONFIG_IO_SER_FLASH_CS)
#endif /* (CONFIG_DFU_ENABLED && (CONFIG_DFU_IMAGE_DEST==2)) */
    | PIN_MASK(CONFIG_IO_PA)
    | PIN_MASK(CONFIG_IO_LNA)
    | 0;

bool drv_board_shutdown(bool wakeup)
{
    unsigned int i;

    /* Stop Touchpad DC/DC Converter */
    ASSERT_SIGNAL(CONFIG_IO_TP_DCDC_SHUTDOWN,   CONFIG_TP_DCDC_SHUTDOWN_ACT_LOW,    false);

    /* Discharge Touchpad DC/DC output capacitors */
#if IS_IO_VALID(CONFIG_IO_TP_DCDC_DISCHARGE)
    ASSERT_SIGNAL(CONFIG_IO_TP_DCDC_DISCHARGE,  CONFIG_TP_DCDC_DISCHARGE_ACT_LOW,   false);
    nrf_delay_ms(10);
    DEASSERT_SIGNAL(CONFIG_IO_TP_DCDC_DISCHARGE,CONFIG_TP_DCDC_DISCHARGE_ACT_LOW,   false);
#endif

    /* Make sure that all peripherals with dynamic power management are off. */
    DEASSERT_SIGNAL(CONFIG_IO_BUZZER,           false,                              false);
    DEASSERT_SIGNAL(CONFIG_IO_IR_RX_ENABLE,     CONFIG_IR_RX_ENABLE_ACT_LOW,        false);
    DEASSERT_SIGNAL(CONFIG_IO_IR_TX_LED,        false,                              false);
    DEASSERT_SIGNAL(CONFIG_IO_LED_1,            CONFIG_LED_ACT_LOW_MASK & (1 << 0), false);
    DEASSERT_SIGNAL(CONFIG_IO_LED_2,            CONFIG_LED_ACT_LOW_MASK & (1 << 1), false);
    DEASSERT_SIGNAL(CONFIG_IO_LED_3,            CONFIG_LED_ACT_LOW_MASK & (1 << 2), false);
    DEASSERT_SIGNAL(CONFIG_IO_LED_4,            CONFIG_LED_ACT_LOW_MASK & (1 << 3), false);
    DEASSERT_SIGNAL(CONFIG_IO_LED_PWR_CTRL,     CONFIG_LED_PWR_CTRL_ACT_LOW,        false);
    DEASSERT_SIGNAL(CONFIG_IO_PDM_MIC_PWR_CTRL, CONFIG_PDM_MIC_PWR_CTRL_ACT_LOW,    false);
    DEASSERT_SIGNAL(CONFIG_IO_PA,               CONFIG_PA_ACTIVE_LOW,               false);
    DEASSERT_SIGNAL(CONFIG_IO_LNA,              CONFIG_LNA_ACTIVE_LOW,              false);

    /* Power down core peripherals. */
    DEASSERT_SIGNAL(CONFIG_IO_BOARD_PWR_CTRL,   CONFIG_BOARD_PWR_CTRL_ACT_LOW,      false);

    /* Disable all unused pins. */
    for (i = 0; i < 32; i++)
    {
        if (m_protected_pins & (1ul << i))
        {
            continue;
        }

        NRF_GPIO->PIN_CNF[i] = ((GPIO_PIN_CNF_SENSE_Disabled    << GPIO_PIN_CNF_SENSE_Pos) \
                             | (GPIO_PIN_CNF_DRIVE_S0S1         << GPIO_PIN_CNF_DRIVE_Pos) \
                             | (GPIO_PIN_CNF_PULL_Disabled      << GPIO_PIN_CNF_PULL_Pos)  \
                             | (GPIO_PIN_CNF_INPUT_Disconnect   << GPIO_PIN_CNF_INPUT_Pos) \
                             | (GPIO_PIN_CNF_DIR_Input          << GPIO_PIN_CNF_DIR_Pos));
    }

    return true;
}
#endif /* CONFIG_PWR_MGMT_ENABLED */

ret_code_t drv_board_init(void)
{
#if !defined(NRF_BOOTLOADER)
    /* Application: Power up core peripherals. */
    ASSERT_SIGNAL(CONFIG_IO_BOARD_PWR_CTRL,     CONFIG_BOARD_PWR_CTRL_ACT_LOW,      true);
#else
    /* Bootloader: Power down core peripherals. */
    DEASSERT_SIGNAL(CONFIG_IO_BOARD_PWR_CTRL,   CONFIG_BOARD_PWR_CTRL_ACT_LOW,      true);
#endif

    /* Make sure that all peripherals with dynamic power management are off. */
    DEASSERT_SIGNAL(CONFIG_IO_BUZZER,           false,                              true);
    DEASSERT_SIGNAL(CONFIG_IO_IR_RX_ENABLE,     CONFIG_IR_RX_ENABLE_ACT_LOW,        true);
    DEASSERT_SIGNAL(CONFIG_IO_IR_TX_LED,        false,                              true);
    DEASSERT_SIGNAL(CONFIG_IO_LED_1,            CONFIG_LED_ACT_LOW_MASK & (1 << 0), true);
    DEASSERT_SIGNAL(CONFIG_IO_LED_2,            CONFIG_LED_ACT_LOW_MASK & (1 << 1), true);
    DEASSERT_SIGNAL(CONFIG_IO_LED_3,            CONFIG_LED_ACT_LOW_MASK & (1 << 2), true);
    DEASSERT_SIGNAL(CONFIG_IO_LED_4,            CONFIG_LED_ACT_LOW_MASK & (1 << 3), true);
    DEASSERT_SIGNAL(CONFIG_IO_LED_PWR_CTRL,     CONFIG_LED_PWR_CTRL_ACT_LOW,        true);
    DEASSERT_SIGNAL(CONFIG_IO_PDM_MIC_PWR_CTRL, CONFIG_PDM_MIC_PWR_CTRL_ACT_LOW,    true);
    DEASSERT_SIGNAL(CONFIG_IO_PA,               CONFIG_PA_ACTIVE_LOW,               true);
    DEASSERT_SIGNAL(CONFIG_IO_LNA,              CONFIG_LNA_ACTIVE_LOW,              true);

#if !defined(NRF_BOOTLOADER)
    /* Application: Start Touchpad DC/DC Converter */
    DEASSERT_SIGNAL(CONFIG_IO_TP_DCDC_DISCHARGE, CONFIG_TP_DCDC_DISCHARGE_ACT_LOW,  true);
    DEASSERT_SIGNAL(CONFIG_IO_TP_DCDC_SHUTDOWN, CONFIG_TP_DCDC_SHUTDOWN_ACT_LOW,    true);

    /* Application: Wait for power supply stabilization. */
    nrf_delay_ms(1);
#else
    /* Bootloader: Stop Touchpad DC/DC Converter and discharge output capacitors */
    ASSERT_SIGNAL(CONFIG_IO_TP_DCDC_SHUTDOWN,   CONFIG_TP_DCDC_SHUTDOWN_ACT_LOW,    true);
    ASSERT_SIGNAL(CONFIG_IO_TP_DCDC_DISCHARGE,  CONFIG_TP_DCDC_DISCHARGE_ACT_LOW,   true);
#endif

    return NRF_SUCCESS;
}

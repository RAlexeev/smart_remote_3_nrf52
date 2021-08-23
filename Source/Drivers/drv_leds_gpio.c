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

#include <stdint.h>
#include <stdbool.h>

#include "nrf.h"
#include "nrf_gpio.h"
#include "nrf_error.h"

#include "drv_leds.h"
#include "sr3_config.h"

#if CONFIG_LED_ENABLED

#if CONFIG_LED_PWR_CTRL_ENABLED
STATIC_ASSERT(IS_IO_VALID(CONFIG_IO_LED_PWR_CTRL));
#endif

#define NRF_LOG_MODULE_NAME drv_leds
#define NRF_LOG_LEVEL CONFIG_LED_DRV_LOG_LEVEL
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

#define MAX_LEDS    4

static const uint8_t m_led_to_pin_map[MAX_LEDS] = {
    (uint8_t)(CONFIG_IO_LED_1),
    (uint8_t)(CONFIG_IO_LED_2),
    (uint8_t)(CONFIG_IO_LED_3),
    (uint8_t)(CONFIG_IO_LED_4),
};

#if CONFIG_LED_PWR_CTRL_ENABLED
/**@brief Check if one or more LEDs are on */
static bool drv_leds_is_on(void)
{
    for (unsigned int i = 0; i < ARRAY_SIZE(m_led_to_pin_map); ++i)
    {
        uint32_t pin;
        uint32_t pin_state;
        bool     pin_active_low;

        pin = m_led_to_pin_map[i];

        if (!IS_IO_VALID(pin))
        {
            continue;
        }

        pin_state      = nrf_gpio_pin_read(pin);
        pin_active_low = (CONFIG_LED_ACT_LOW_MASK & (1 << i)) == 0 ? false : true;

        if ((pin_state == 0 &&  pin_active_low) ||
            (pin_state != 0 && !pin_active_low))
        {
            return true;
        }
    }

    return false;
}

/**@brief Turn LED power pin on or off */
static void drv_leds_power_configure(bool power_on)
{
    if ((CONFIG_LED_PWR_CTRL_ACT_LOW &&  power_on) ||
       (!CONFIG_LED_PWR_CTRL_ACT_LOW && !power_on))
    {
        nrf_gpio_pin_clear(CONFIG_IO_LED_PWR_CTRL);
    }
    else
    {
        nrf_gpio_pin_set(CONFIG_IO_LED_PWR_CTRL);
    }
}
#endif /* CONFIG_LED_PWR_CTRL_ENABLED */

ret_code_t drv_leds_init(void)
{
    unsigned int i;

#if CONFIG_LED_PWR_CTRL_ENABLED
    nrf_gpio_cfg_output(CONFIG_IO_LED_PWR_CTRL);
    drv_leds_power_configure(false);
#endif /* CONFIG_LED_PWR_CTRL_ENABLED */

    for (i = 0; i < MAX_LEDS; i++)
    {
        uint8_t pin = m_led_to_pin_map[i];

        if (!IS_IO_VALID(pin))
        {
            continue;
        }

        nrf_gpio_cfg_output(pin);

        if (CONFIG_LED_ACT_LOW_MASK & (1  << i))
        {
            nrf_gpio_pin_set(pin);
        }
        else
        {
            nrf_gpio_pin_clear(pin);
        }
    }

    return NRF_SUCCESS;
}

ret_code_t drv_leds_set(uint8_t leds_mask)
{
    unsigned int i;

    // Check if all requested LEDs are suppoted.
    if (leds_mask & ~((1 << MAX_LEDS) - 1))
    {
        return NRF_ERROR_INVALID_PARAM;
    }

#if CONFIG_LED_PWR_CTRL_ENABLED
    drv_leds_power_configure(true);
#endif /* CONFIG_LED_PWR_CTRL_ENABLED */

    for (i = 0; i < MAX_LEDS; i++)
    {
        uint8_t pin;

        if ((leds_mask & (1 << i)) == 0)
        {
            continue;
        }

        pin = m_led_to_pin_map[i];
        if (!IS_IO_VALID(pin))
        {
            return NRF_ERROR_INVALID_PARAM;
        }

        if (CONFIG_LED_ACT_LOW_MASK & (1 << i))
        {
            nrf_gpio_pin_clear(pin);
        }
        else
        {
            nrf_gpio_pin_set(pin);
        }
    }

    return NRF_SUCCESS;
}

ret_code_t drv_leds_clr(uint8_t leds_mask)
{
    ret_code_t   err_code;
    unsigned int i;

    // Check if all requested LEDs are suppoted.
    if (leds_mask & ~((1 << MAX_LEDS) - 1))
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    err_code = NRF_SUCCESS;

    for (i = 0; i < MAX_LEDS; i++)
    {
        uint8_t pin;

        if ((leds_mask & (1 << i)) == 0)
        {
            continue;
        }

        pin = m_led_to_pin_map[i];
        if (!IS_IO_VALID(pin))
        {
            err_code = NRF_ERROR_INVALID_PARAM;
            break;
        }

        if (CONFIG_LED_ACT_LOW_MASK & (1  << i))
        {
            nrf_gpio_pin_set(pin);
        }
        else
        {
            nrf_gpio_pin_clear(pin);
        }
    }

#if CONFIG_LED_PWR_CTRL_ENABLED
    if (!drv_leds_is_on())
    {
        drv_leds_power_configure(false);
    }
#endif /* CONFIG_LED_PWR_CTRL_ENABLED */

    return err_code;
}

uint8_t drv_leds_all(void)
{
    uint8_t mask = 0;
    unsigned int i;

    for (i = 0; i < MAX_LEDS; i++)
    {
        uint8_t pin = m_led_to_pin_map[i];

        if (IS_IO_VALID(pin))
        {
            mask |= (1 << i);
        }
    }

    return mask;
}
#endif /* CONFIG_LED_ENABLED */

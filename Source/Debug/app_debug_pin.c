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

#include <stdlib.h>
#include <string.h>

#include "nrf.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "app_debug_pin.h"

#if CONFIG_DEBUG_PIN_ENABLED

#define PIO_INIT(pin)               \
do {                                \
    if (IS_IO_VALID((pin)))         \
    {                               \
        nrf_gpio_cfg_output((pin)); \
        nrf_gpio_pin_clear((pin));  \
    }                               \
} while (0)

void debug_pin_init(void)
{
    PIO_INIT(CONFIG_IO_DBG_MARK);
    PIO_INIT(CONFIG_IO_DBG_CLOCK);
    PIO_INIT(CONFIG_IO_DBG_DATA);

    PIO_INIT(CONFIG_IO_DBG_AUDIO_CAPTURE);
    PIO_INIT(CONFIG_IO_DBG_AUDIO_PROCESS);

    PIO_INIT(CONFIG_IO_DBG_SLEEP);

    PIO_INIT(CONFIG_IO_DBG_IR_TX_PWM_INT);
    PIO_INIT(CONFIG_IO_DBG_IR_TX_SACK);
    PIO_INIT(CONFIG_IO_DBG_IR_TX_EACK);
}

void debug_pin_set(const unsigned int pio)
{
    if (IS_IO_VALID(pio))
    {
        nrf_gpio_pin_set(pio);
    }
}

void debug_pin_clear(const unsigned int pio)
{
    if (IS_IO_VALID(pio))
    {
        nrf_gpio_pin_clear(pio);
    }
}

void debug_pin_pulse(const unsigned int pio)
{
    if (IS_IO_VALID(pio))
    {
        nrf_gpio_pin_set(pio);
        nrf_delay_us(1);
        nrf_gpio_pin_clear(pio);
    }
}

void debug_pin_flag(const unsigned int pio, bool flag)
{
    if (IS_IO_VALID(pio))
    {
        if (flag)
        {
            nrf_gpio_pin_set(pio);
        }
        else
        {
            nrf_gpio_pin_clear(pio);
        }
    }
}

void debug_pin_toggle(const unsigned int pio)
{
    if (IS_IO_VALID(pio))
    {
        nrf_gpio_pin_toggle(pio);
    }
}

void debug_pin_send(const uint32_t word, const uint8_t bits)
{
#if IS_IO_VALID(CONFIG_IO_DBG_MARK) && \
    IS_IO_VALID(CONFIG_IO_DBG_CLOCK) && \
    IS_IO_VALID(CONFIG_IO_DBG_DATA)

    nrf_gpio_pin_set(CONFIG_IO_DBG_MARK);
    for (unsigned int bit = bits; bit > 0; )
    {
        if ((word & (1<<(--bit))) != 0)
        {
            nrf_gpio_pin_set(CONFIG_IO_DBG_DATA);
        }

        nrf_gpio_pin_set(CONFIG_IO_DBG_CLOCK);
        nrf_gpio_pin_clear(CONFIG_IO_DBG_CLOCK);
        nrf_gpio_pin_clear(CONFIG_IO_DBG_DATA);
    }
    nrf_gpio_pin_clear(CONFIG_IO_DBG_MARK);
#endif
}

void debug_pin_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t *p_file_name)
{
    debug_pin_pulse(CONFIG_IO_DBG_MARK);
    debug_pin_send(error_code, 32);
    debug_pin_send(line_num, 32);

    if (p_file_name)
    {
        while (*p_file_name)
        {
            debug_pin_send(*(p_file_name++), 8);
        }
    }
}
#endif /* CONFIG_DEBUG_PIO_ENABLED */

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
 *
 * @defgroup APP_DEBUG_PIN  Pin-based debug utilities
 * @ingroup APP_DEBUG
 * @{
 * @brief Functions that control pin-based debug utilities.
 *
 */

#ifndef __APP_DEBUG_PIN_H__
#define __APP_DEBUG_PIN_H__

#include <stdint.h>

#include "nrf.h"
#include "sr3_config.h"

#if CONFIG_DEBUG_PIN_ENABLED

#define DBG_PIN_PULSE(pin)          debug_pin_pulse(pin)
#define DBG_PIN_SET(pin)            debug_pin_set(pin)
#define DBG_PIN_CLEAR(pin)          debug_pin_clear(pin)
#define DBG_PIN_TOGGLE(pin)         debug_pin_toggle(pin)
#define DBG_PIN_FLAG(pin, flag)     debug_pin_flag(pin, flag)
#define DBG_PIN_SEND(word, bits)    debug_pin_send(word, bits)

/**
 * @brief Initialize the debug lines and startup pulse.
 */
void debug_pin_init(void);

/**
 * @brief Signal 1 µs pulse on the selected line.
 * @param[in] pin   Line to be pulsed.
 */
void debug_pin_pulse(const unsigned int pin);

/**
 * @brief Set the selected line.
 * @param[in] pin   Line to be set.
 */
void debug_pin_set(const unsigned int pin);

/**
 * @brief Clear the selected line.
 * @param[in] pin   Line to be cleared.
 */
void debug_pin_clear(const unsigned int pin);

/**
 * @brief Toggle the selected line.
 * @param[in] pin   Line to be toggled.
 */
void debug_pin_toggle(const unsigned int pin);

/**
 * @brief Set a line according to a flag.
 * @param[in] pin   Line to be set.
 * @param[in] flag  Flag to be set.
 */
void debug_pin_flag(const unsigned int pin, bool flag);

/**
 * @brief Function for displaying an error code on the scope.
 *
 * @warning Bits are sent MSB first with data sampling on falling clock and positive frame signal.
 *
 * @param[in] word  Word to be sent.
 * @param[in] bits  Word length in bits (1-32).
 */
void debug_pin_send(const uint32_t word, const uint8_t bits);

/**
 * @brief Function for error handling, which is called when an error has occurred.
 *
 * @param[in] error_code  Error code supplied to the handler.
 * @param[in] line_num    Line number where the handler is called.
 * @param[in] p_file_name Pointer to the file name.
 */
void debug_pin_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t *p_file_name);

#else /* !CONFIG_DEBUG_PIN_ENABLED */

#define DBG_PIN_PULSE(pin)          do { } while (0)
#define DBG_PIN_SET(pin)            do { } while (0)
#define DBG_PIN_CLEAR(pin)          do { } while (0)
#define DBG_PIN_TOGGLE(pin)         do { } while (0)
#define DBG_PIN_FLAG(pin, flag)     do { } while (0)
#define DBG_PIN_SEND(word, bits)    do { } while (0)

__STATIC_INLINE void debug_pin_init(void)
{
    // Do nothing.
}

__STATIC_INLINE void debug_pin_pulse(const unsigned int pin)
{
    // Do nothing.
}

__STATIC_INLINE void debug_pin_set(const unsigned int pin)
{
    // Do nothing.
}

__STATIC_INLINE void debug_pin_clear(const unsigned int pin)
{
    // Do nothing.
}

__STATIC_INLINE void debug_pin_toggle(const unsigned int pin)
{
    // Do nothing.
}

__STATIC_INLINE void debug_pin_flag(const unsigned int pin, bool flag)
{
    // Do nothing.
}

__STATIC_INLINE void debug_pin_send(const uint32_t word, const uint8_t bits)
{
    // Do nothing.
}

__STATIC_INLINE void debug_pin_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t *p_file_name)
{
    // Do nothing.
}

#endif /* CONFIG_DEBUG_PIN_ENABLED */
#endif /* __APP_DEBUG_PIN_H__ */
/** @} */


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
#include <string.h>

#include "nrf.h"
#include "nrf_assert.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "app_debug.h"
#include "app_error.h"
#include "app_gpiote.h"
#include "app_timer.h"

#include "drv_keyboard.h"

#include "resources.h"
#include "sr3_config.h"

#if (CONFIG_KBD_ENABLED && (CONFIG_KBD_DRIVER == CONFIG_KBD_DRIVER_GPIO))

#define NRF_LOG_MODULE_NAME drv_keyboard
#define NRF_LOG_LEVEL CONFIG_KBD_DRV_LOG_LEVEL
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

#define KEYBOARD_MODE_TIMER             1
#define KEYBOARD_MODE_GPIOTE            2

#define KEYBOARD_MODE                   KEYBOARD_MODE_GPIOTE

#define KEYBOARD_NUM_OF_COLUMNS         8  //!< Number of columns in the keyboard matrix.
#define KEYBOARD_NUM_OF_ROWS            8  //!< Number of rows in the keyboard matrix.
#define KEYBOARD_IDLE_SCAN_CNT_MAX      3  //!< Idle scans before switching into interrupt mode
#define KEYBOARD_SELECT_TIME            3  //!< Time required to select given column and charge parasitic capacitances through switch resistance [us].

#if (KEYBOARD_MODE == KEYBOARD_MODE_TIMER)
#define KEYBOARD_APP_TIMER_MODE         APP_TIMER_MODE_REPEATED
#else
#define KEYBOARD_APP_TIMER_MODE         APP_TIMER_MODE_SINGLE_SHOT
#endif

#define KEYBOARD_MASK_COL_0  (IS_IO_VALID(CONFIG_IO_KEY_COL_0) ? (1ul << CONFIG_IO_KEY_COL_0) : 0)
#define KEYBOARD_MASK_COL_1  (IS_IO_VALID(CONFIG_IO_KEY_COL_1) ? (1ul << CONFIG_IO_KEY_COL_1) : 0)
#define KEYBOARD_MASK_COL_2  (IS_IO_VALID(CONFIG_IO_KEY_COL_2) ? (1ul << CONFIG_IO_KEY_COL_2) : 0)
#define KEYBOARD_MASK_COL_3  (IS_IO_VALID(CONFIG_IO_KEY_COL_3) ? (1ul << CONFIG_IO_KEY_COL_3) : 0)
#define KEYBOARD_MASK_COL_4  (IS_IO_VALID(CONFIG_IO_KEY_COL_4) ? (1ul << CONFIG_IO_KEY_COL_4) : 0)
#define KEYBOARD_MASK_COL_5  (IS_IO_VALID(CONFIG_IO_KEY_COL_5) ? (1ul << CONFIG_IO_KEY_COL_5) : 0)
#define KEYBOARD_MASK_COL_6  (IS_IO_VALID(CONFIG_IO_KEY_COL_6) ? (1ul << CONFIG_IO_KEY_COL_6) : 0)
#define KEYBOARD_MASK_COL_7  (IS_IO_VALID(CONFIG_IO_KEY_COL_7) ? (1ul << CONFIG_IO_KEY_COL_7) : 0)

#define KEYBOARD_MASK_COL_ALL ( KEYBOARD_MASK_COL_0 \
                              | KEYBOARD_MASK_COL_1 \
                              | KEYBOARD_MASK_COL_2 \
                              | KEYBOARD_MASK_COL_3 \
                              | KEYBOARD_MASK_COL_4 \
                              | KEYBOARD_MASK_COL_5 \
                              | KEYBOARD_MASK_COL_6 \
                              | KEYBOARD_MASK_COL_7 )

#define KEYBOARD_MASK_ROW_0  (IS_IO_VALID(CONFIG_IO_KEY_ROW_0) ? (1ul << CONFIG_IO_KEY_ROW_0) : 0)
#define KEYBOARD_MASK_ROW_1  (IS_IO_VALID(CONFIG_IO_KEY_ROW_1) ? (1ul << CONFIG_IO_KEY_ROW_1) : 0)
#define KEYBOARD_MASK_ROW_2  (IS_IO_VALID(CONFIG_IO_KEY_ROW_2) ? (1ul << CONFIG_IO_KEY_ROW_2) : 0)
#define KEYBOARD_MASK_ROW_3  (IS_IO_VALID(CONFIG_IO_KEY_ROW_3) ? (1ul << CONFIG_IO_KEY_ROW_3) : 0)
#define KEYBOARD_MASK_ROW_4  (IS_IO_VALID(CONFIG_IO_KEY_ROW_4) ? (1ul << CONFIG_IO_KEY_ROW_4) : 0)
#define KEYBOARD_MASK_ROW_5  (IS_IO_VALID(CONFIG_IO_KEY_ROW_5) ? (1ul << CONFIG_IO_KEY_ROW_5) : 0)
#define KEYBOARD_MASK_ROW_6  (IS_IO_VALID(CONFIG_IO_KEY_ROW_6) ? (1ul << CONFIG_IO_KEY_ROW_6) : 0)
#define KEYBOARD_MASK_ROW_7  (IS_IO_VALID(CONFIG_IO_KEY_ROW_7) ? (1ul << CONFIG_IO_KEY_ROW_7) : 0)

#define KEYBOARD_MASK_ROW_ALL ( KEYBOARD_MASK_ROW_0 \
                              | KEYBOARD_MASK_ROW_1 \
                              | KEYBOARD_MASK_ROW_2 \
                              | KEYBOARD_MASK_ROW_3 \
                              | KEYBOARD_MASK_ROW_4 \
                              | KEYBOARD_MASK_ROW_5 \
                              | KEYBOARD_MASK_ROW_6 \
                              | KEYBOARD_MASK_ROW_7 )

static drv_keyboard_event_handler_t   m_keyboard_event_handler;             //!< Keyboard event handler
static uint8_t                        m_key_vector[DRV_KEYBOARD_MAX_KEYS];  //!< vector holding currently pressed keys. Filled up from index 0.
static uint8_t                        m_key_vector_size;                    //!< Size of the currently pressed keys vector.
static bool                           m_keys_blocked;                       //!< True if some of keys are blocked and cannot be read.
static bool                           m_keyboard_scan_enabled;              //!< Variable protecting driver state.
APP_TIMER_DEF                         (m_keyboard_timer);                   //!< Keyboard scan timer.

#if  (KEYBOARD_MODE == KEYBOARD_MODE_GPIOTE)
static app_gpiote_user_id_t           m_keyboard_gpiote;                    //!< GPIOTE Handle.
static uint8_t                        m_idle_scan_counter;                  //!< Idle Scan Counter.
#endif

/** Table describing relationship between column number and column pin */
static const uint8_t m_column_to_pin_map[KEYBOARD_NUM_OF_COLUMNS] =
{
    (uint8_t)(CONFIG_IO_KEY_COL_0),
    (uint8_t)(CONFIG_IO_KEY_COL_1),
    (uint8_t)(CONFIG_IO_KEY_COL_2),
    (uint8_t)(CONFIG_IO_KEY_COL_3),
    (uint8_t)(CONFIG_IO_KEY_COL_4),
    (uint8_t)(CONFIG_IO_KEY_COL_5),
    (uint8_t)(CONFIG_IO_KEY_COL_6),
    (uint8_t)(CONFIG_IO_KEY_COL_7),
};

static const uint8_t m_row_to_pin_map[KEYBOARD_NUM_OF_ROWS] =
{
    (uint8_t)(CONFIG_IO_KEY_ROW_0),
    (uint8_t)(CONFIG_IO_KEY_ROW_1),
    (uint8_t)(CONFIG_IO_KEY_ROW_2),
    (uint8_t)(CONFIG_IO_KEY_ROW_3),
    (uint8_t)(CONFIG_IO_KEY_ROW_4),
    (uint8_t)(CONFIG_IO_KEY_ROW_5),
    (uint8_t)(CONFIG_IO_KEY_ROW_6),
    (uint8_t)(CONFIG_IO_KEY_ROW_7),
};

/**@brief Set desired configuration on given GPIOs. */
static void drv_keyboard_gpio_cfg(uint32_t mask, uint32_t pin_cnf)
{
    unsigned int i = 8 * sizeof(mask);

    while ((i--, mask != 0))
    {
        unsigned int n = __CLZ(mask);

        i     -= n;
        mask <<= n + 1;

        NRF_GPIO->PIN_CNF[i] = pin_cnf;
    }

    // Make sure that new confiuration is active.
    __DSB();
}

/**@brief Initialize Keyboard GPIOs. */
static void drv_keyboard_gpio_init(void)
{
    // Make sure that columns will drive 0 when they will be configured as outputs.
    NRF_GPIO->OUTCLR = KEYBOARD_MASK_COL_ALL;

    // Configure Columns.
    drv_keyboard_gpio_cfg(KEYBOARD_MASK_COL_ALL,
                          (GPIO_PIN_CNF_SENSE_Disabled   << GPIO_PIN_CNF_SENSE_Pos) |
                          (GPIO_PIN_CNF_DRIVE_D0S1       << GPIO_PIN_CNF_DRIVE_Pos) |
                          (GPIO_PIN_CNF_PULL_Disabled    << GPIO_PIN_CNF_PULL_Pos)  |
                          (GPIO_PIN_CNF_DIR_Output       << GPIO_PIN_CNF_DIR_Pos)   |
                          (GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos) );

    // Configure Rows.
    drv_keyboard_gpio_cfg(KEYBOARD_MASK_ROW_ALL,
                          (GPIO_PIN_CNF_SENSE_Disabled   << GPIO_PIN_CNF_SENSE_Pos) |
                          (GPIO_PIN_CNF_DRIVE_S0S1       << GPIO_PIN_CNF_DRIVE_Pos) |
                          (GPIO_PIN_CNF_PULL_Pulldown    << GPIO_PIN_CNF_PULL_Pos)  |
                          (GPIO_PIN_CNF_DIR_Input        << GPIO_PIN_CNF_DIR_Pos)   |
                          (GPIO_PIN_CNF_INPUT_Connect    << GPIO_PIN_CNF_INPUT_Pos) );
}

/**@brief Extract rows state form gpio lines state. */
static uint8_t drv_keyboard_get_rows_state(uint32_t gpio_state)
{
    uint8_t rows_state = 0;
    unsigned int i;

    for (i = 0; i < KEYBOARD_NUM_OF_ROWS; i++)
    {
        uint8_t pin = m_row_to_pin_map[i];

        if (!IS_IO_VALID(pin))
        {
            continue;
        }

        rows_state |= ((gpio_state >> pin) & 1) << i;
    }

    return rows_state;
}

#if (KEYBOARD_MODE == KEYBOARD_MODE_GPIOTE)
/**@brief Enable interrupt-based key press sensing. */
static ret_code_t drv_keyboard_sense_enable(void)
{
    // Drive "1" on all columns.
    NRF_GPIO->OUTSET = KEYBOARD_MASK_COL_ALL;
    __DSB();

    // Enable GPIOTE sensing.
    return app_gpiote_user_enable(m_keyboard_gpiote);
}

/**@brief Disable interrupt-based key press sensing. */
static ret_code_t drv_keyboard_sense_disable(void)
{
    ret_code_t status;

    // Disable GPIOTE sensing.
    status = app_gpiote_user_disable(m_keyboard_gpiote);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    // Drive "0" on all columns.
    NRF_GPIO->OUTCLR = KEYBOARD_MASK_COL_ALL;
    __DSB();

    return NRF_SUCCESS;
}

/**@brief Handler processing scan results */
static void drv_keyboard_scan_handler(uint8_t *p_pressed_keys, uint8_t num_of_pressed_keys, bool keys_blocked)
{
    // Pass data to upper layers.
    m_keyboard_event_handler(p_pressed_keys, num_of_pressed_keys, keys_blocked);

    if (num_of_pressed_keys)
    {
        // Reset idle scan counter if at least one key is pressed.
        m_idle_scan_counter = 0;
    }
    else
    {
        // Count number of idle scans
        m_idle_scan_counter += 1;
    }

    if (m_keyboard_scan_enabled)
    {
        if (m_idle_scan_counter < KEYBOARD_IDLE_SCAN_CNT_MAX)
        {
            // Keep actively polling keyboard if it is in use.
            NRF_LOG_DEBUG("%s(): Timer Enabled", __func__);
            APP_ERROR_CHECK(app_timer_start(m_keyboard_timer,
                                        APP_TIMER_TICKS(CONFIG_KBD_POLL_INTERVAL),
                                        (void *)(drv_keyboard_scan_handler)));
        }
        else
        {
            // Fall back to sensing if keyboard is idle.
            NRF_LOG_DEBUG("%s(): Sense Enabled", __func__);
            APP_ERROR_CHECK(drv_keyboard_sense_enable());
        }
    }
}
#endif /* (KEYBOARD_MODE == KEYBOARD_MODE_GPIOTE) */

/**@brief Perform keyboard scan. */
static void drv_keyboard_scan(void* p_context)
{
    drv_keyboard_event_handler_t callback = (drv_keyboard_event_handler_t)(p_context);
    uint8_t blocking_mask, rows_state[KEYBOARD_NUM_OF_COLUMNS], detected_keypresses_on_column;
    unsigned int column, row;

    // Perform scan.
    for (column = 0; column < KEYBOARD_NUM_OF_COLUMNS; column++)
    {
        uint8_t pin = m_column_to_pin_map[column];
        uint32_t gpio_state;

        if (!IS_IO_VALID(pin))
        {
            rows_state[column] = 0;
            continue;
        }

        // Drive "1" on the selected column.
        NRF_GPIO->OUTSET = (1ul << pin);
        __DSB();

        // Wait for column signal propagation.
        nrf_delay_us(KEYBOARD_SELECT_TIME);

        // Read GPIOs state.
        gpio_state = NRF_GPIO->IN;

        // Restore "0" on the selected column.
        NRF_GPIO->OUTCLR = (1ul << pin);
        __DSB();

        // Save rows state for given column.
        rows_state[column] = drv_keyboard_get_rows_state(gpio_state);
    }

    // Process results.
    blocking_mask       = 0;
    m_key_vector_size   = 0;
    m_keys_blocked      = false;

    for (column = 0; column < KEYBOARD_NUM_OF_COLUMNS; column++)
    {
        if (rows_state[column] == 0)
        {
            continue;
        }

        detected_keypresses_on_column = 0;
        
        for (row = 0; row < KEYBOARD_NUM_OF_ROWS; row++)
        {
            if ((rows_state[column] & (1ul << row)) != 0)
            {
                detected_keypresses_on_column += 1;
                
                if (m_key_vector_size < DRV_KEYBOARD_MAX_KEYS)
                {
                    m_key_vector[m_key_vector_size++] = KEYBOARD_KEY_ID(row, column);
                }
                
            }
        }

        if (((blocking_mask & rows_state[column]) != 0) && (detected_keypresses_on_column > 1))
        {
            // Blocking/ghosting occurs when three or more keys on two rows and columns are pressed at the same time
            m_keys_blocked = true;
            break;
        }

        blocking_mask |= rows_state[column];
    }

    if (callback != NULL)
    {
        callback(m_key_vector, m_key_vector_size, m_keys_blocked);
    }
}

#if (KEYBOARD_MODE == KEYBOARD_MODE_GPIOTE)
/**@brief GPIOTE PORT interrupt handler.*/
static void drv_keyboard_interrupt_handler(uint32_t const *p_event_pins_low_to_high,
                                           uint32_t const *p_event_pins_high_to_low)
{
    if (m_keyboard_scan_enabled)
    {
        NRF_LOG_DEBUG("%s(): Interrupt", __func__);

        // Disable sensing.
        drv_keyboard_sense_disable();

        // Reset idle scan counter.
        m_idle_scan_counter = 0;

        // Schedule keyboard_scan
        APP_ERROR_CHECK(app_isched_event_put(&g_fg_scheduler,
                                             drv_keyboard_scan,
                                             (void *)(drv_keyboard_scan_handler)));
    }
}
#endif /* (KEYBOARD_MODE == KEYBOARD_MODE_GPIOTE) */

ret_code_t drv_keyboard_init(drv_keyboard_event_handler_t keyboard_event_handler)
{
    ret_code_t status;

    if (!keyboard_event_handler)
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    m_keyboard_event_handler    = keyboard_event_handler;
    m_keyboard_scan_enabled     = false;

    status = app_timer_create(&m_keyboard_timer, KEYBOARD_APP_TIMER_MODE, drv_keyboard_scan);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

#if (KEYBOARD_MODE == KEYBOARD_MODE_GPIOTE)
    uint32_t low_to_high_mask = KEYBOARD_MASK_ROW_ALL;
    uint32_t high_to_low_mask = 0;

    status = app_gpiote_user_register(&m_keyboard_gpiote,
                                      &low_to_high_mask,
                                      &high_to_low_mask,
                                      drv_keyboard_interrupt_handler);
    if (status != NRF_SUCCESS)
    {
        return status;
    }
#endif /* (KEYBOARD_MODE == KEYBOARD_MODE_GPIOTE) */

    drv_keyboard_gpio_init();

    return NRF_SUCCESS;
}

ret_code_t drv_keyboard_keys_get(uint8_t *p_pressed_keys,
                                 uint8_t *p_number_of_pressed_keys,
                                 bool *p_keys_blocked)
{
    if (m_keyboard_scan_enabled)
    {
        return NRF_ERROR_INVALID_STATE;
    }

    drv_keyboard_scan(NULL);

    memcpy(p_pressed_keys, m_key_vector, sizeof(m_key_vector));
    *p_number_of_pressed_keys = m_key_vector_size;
    *p_keys_blocked = m_keys_blocked;

    return NRF_SUCCESS;
}

ret_code_t drv_keyboard_enable(void)
{
    ASSERT(m_keyboard_scan_enabled == false);
    m_keyboard_scan_enabled = true;

#if (KEYBOARD_MODE == KEYBOARD_MODE_GPIOTE)
    m_idle_scan_counter = 0;
#endif

#if (KEYBOARD_MODE == KEYBOARD_MODE_TIMER)
    NRF_LOG_DEBUG("%s(): Timer Enabled", __func__);
    return app_timer_start(m_keyboard_timer,
                           APP_TIMER_TICKS(CONFIG_KBD_POLL_INTERVAL),
                           (void *)m_keyboard_event_handler);
#else
    NRF_LOG_DEBUG("%s(): Timer Enabled", __func__);
    return app_timer_start(m_keyboard_timer,
                           APP_TIMER_TICKS(CONFIG_KBD_POLL_INTERVAL),
                           (void *)drv_keyboard_scan_handler);
#endif
}

ret_code_t drv_keyboard_disable(void)
{
    ASSERT(m_keyboard_scan_enabled == true);
    m_keyboard_scan_enabled = false;

#if (KEYBOARD_MODE == KEYBOARD_MODE_GPIOTE)
    NRF_LOG_DEBUG("%s(): Sense Disabled", __func__);
    drv_keyboard_sense_disable();
#endif

    NRF_LOG_DEBUG("%s(): Timer Disabled", __func__);
    return app_timer_stop(m_keyboard_timer);
}

#if NRF_PWR_MGMT_ENABLED
bool drv_keyboard_shutdown(bool wakeup)
{
    if (m_keyboard_scan_enabled)
    {
        drv_keyboard_disable();
    }

#if (CONFIG_KBD_WAKEUP_SOURCE && (KEYBOARD_MODE == KEYBOARD_MODE_GPIOTE))
    if (wakeup)
    {
        NRF_LOG_DEBUG("%s(): Sense Enabled", __func__);
        drv_keyboard_sense_enable();
    }
    else
    {
        NRF_LOG_DEBUG("%s(): Sense Disabled", __func__);
        drv_keyboard_sense_disable();
    }
#endif

    return true;
}
#endif /* NRF_PWR_MGMT_ENABLED */
#endif /* (CONFIG_KBD_ENABLED && (CONFIG_KBD_DRIVER == CONFIG_KBD_DRIVER_GPIO)) */

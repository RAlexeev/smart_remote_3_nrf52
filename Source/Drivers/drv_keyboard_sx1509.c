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
#include "nrf_atomic.h"
#include "nrf_assert.h"
#include "nrf_gpio.h"
#include "nrf_pwr_mgmt.h"

#include "app_debug.h"
#include "app_error.h"
#include "app_timer.h"

#include "drv_keyboard.h"
#include "drv_keyboard_sx1509.h"

#include "resources.h"
#include "twi_common.h"
#include "sr3_config.h"

#if (CONFIG_KBD_ENABLED && (CONFIG_KBD_DRIVER == CONFIG_KBD_DRIVER_SX1509))

#define NRF_LOG_MODULE_NAME drv_keyboard
#define NRF_LOG_LEVEL CONFIG_KBD_DRV_LOG_LEVEL
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

#define KEYBOARD_NUM_OF_COLUMNS 8   //!< Number of columns in the keyboard matrix.
#define KEYBOARD_NUM_OF_ROWS    8   //!< Number of rows in the keyboard matrix.

/**@brief SX1509 configuration */
static const uint8_t m_register_config[][2] =
{
    { RegReset,            0x12 }, // Software Reset - Part 1.
    { RegReset,            0x34 }, // Software Reset - Part 2.
    { RegPullUpA,          0xFF }, // Enable pull-ups on Port A (rows).
    { RegPolarityA,        0xFF }, // Enable polarity inversion on Port A (rows).
    { RegPolarityB,        0xFF }, // Enable polarity inversion on Port B (columns).
    { RegDataB,            0x00 }, // Do not drive Port B (columns).
    { RegOpenDrainB,       0xFF }, // Set Port B (columns) pins as open-drain.
    { RegDirB,             0x00 }, // Set Port B (columns) pins as outputs.
    { RegInputDisableB,    0xFF }, // Disable inputs on Port B (columns).
};

/**@brief Address of RegDataA register */
static uint8_t m_RegDataA = RegDataA;

/**@brief Column switch commands */
static uint8_t m_col_switch[][2] =
{
    {RegDataB, 0x01},
    {RegDataB, 0x02},
    {RegDataB, 0x04},
    {RegDataB, 0x08},
    {RegDataB, 0x10},
    {RegDataB, 0x20},
    {RegDataB, 0x40},
    {RegDataB, 0x80},
    {RegDataB, 0x00},
};

APP_TIMER_DEF                         (m_keyboard_timer);
static uint8_t                        m_key_vector[DRV_KEYBOARD_MAX_KEYS];    //!< Vector holding currently pressed keys. Filled up from index 0.
static uint8_t                        m_key_vector_size;                      //!< Size of the currently pressed keys vector.
static bool                           m_keys_blocked;                         //!< True if some of keys are blocked and cannot be read.
static drv_keyboard_event_handler_t   m_keyboard_event_handler;               //!< Keyboard event handler.
static nrf_atomic_flag_t              m_read_operation_active;                //!< Flag protecting shared data used in read operation.
static uint8_t                        m_row_state[KEYBOARD_NUM_OF_COLUMNS];   //!< Buffer for row state.

/**@brief TWI transfers performing keyboard scan */
static const nrf_twi_mngr_transfer_t s_scan_transfers[] =
{
    NRF_TWI_MNGR_WRITE  (SX1509_TWI_ADDRESS, &(m_col_switch[0][0]), 2, 0),
    NRF_TWI_MNGR_WRITE  (SX1509_TWI_ADDRESS, &m_RegDataA,           1, NRF_TWI_MNGR_NO_STOP),
    NRF_TWI_MNGR_READ   (SX1509_TWI_ADDRESS, &(m_row_state[0]),     1, 0),

    NRF_TWI_MNGR_WRITE  (SX1509_TWI_ADDRESS, &(m_col_switch[1][0]), 2, 0),
    NRF_TWI_MNGR_WRITE  (SX1509_TWI_ADDRESS, &m_RegDataA,           1, NRF_TWI_MNGR_NO_STOP),
    NRF_TWI_MNGR_READ   (SX1509_TWI_ADDRESS, &(m_row_state[1]),     1, 0),

    NRF_TWI_MNGR_WRITE  (SX1509_TWI_ADDRESS, &(m_col_switch[2][0]), 2, 0),
    NRF_TWI_MNGR_WRITE  (SX1509_TWI_ADDRESS, &m_RegDataA,           1, NRF_TWI_MNGR_NO_STOP),
    NRF_TWI_MNGR_READ   (SX1509_TWI_ADDRESS, &(m_row_state[2]),     1, 0),

    NRF_TWI_MNGR_WRITE  (SX1509_TWI_ADDRESS, &(m_col_switch[3][0]), 2, 0),
    NRF_TWI_MNGR_WRITE  (SX1509_TWI_ADDRESS, &m_RegDataA,           1, NRF_TWI_MNGR_NO_STOP),
    NRF_TWI_MNGR_READ   (SX1509_TWI_ADDRESS, &(m_row_state[3]),     1, 0),

    NRF_TWI_MNGR_WRITE  (SX1509_TWI_ADDRESS, &(m_col_switch[4][0]), 2, 0),
    NRF_TWI_MNGR_WRITE  (SX1509_TWI_ADDRESS, &m_RegDataA,           1, NRF_TWI_MNGR_NO_STOP),
    NRF_TWI_MNGR_READ   (SX1509_TWI_ADDRESS, &(m_row_state[4]),     1, 0),

    NRF_TWI_MNGR_WRITE  (SX1509_TWI_ADDRESS, &(m_col_switch[5][0]), 2, 0),
    NRF_TWI_MNGR_WRITE  (SX1509_TWI_ADDRESS, &m_RegDataA,           1, NRF_TWI_MNGR_NO_STOP),
    NRF_TWI_MNGR_READ   (SX1509_TWI_ADDRESS, &(m_row_state[5]),     1, 0),

    NRF_TWI_MNGR_WRITE  (SX1509_TWI_ADDRESS, &(m_col_switch[6][0]), 2, 0),
    NRF_TWI_MNGR_WRITE  (SX1509_TWI_ADDRESS, &m_RegDataA,           1, NRF_TWI_MNGR_NO_STOP),
    NRF_TWI_MNGR_READ   (SX1509_TWI_ADDRESS, &(m_row_state[6]),     1, 0),

    NRF_TWI_MNGR_WRITE  (SX1509_TWI_ADDRESS, &(m_col_switch[7][0]), 2, 0),
    NRF_TWI_MNGR_WRITE  (SX1509_TWI_ADDRESS, &m_RegDataA,           1, NRF_TWI_MNGR_NO_STOP),
    NRF_TWI_MNGR_READ   (SX1509_TWI_ADDRESS, &(m_row_state[7]),     1, 0),

    NRF_TWI_MNGR_WRITE  (SX1509_TWI_ADDRESS, &(m_col_switch[8][0]), 2, 0),
};

/**@brief HAL TWI callback processing keyboard scan result */
static void drv_keyboard_process_scan_result(ret_code_t status, void *p_user_data)
{
    drv_keyboard_event_handler_t callback = (drv_keyboard_event_handler_t)p_user_data;
    unsigned int column, row;
    uint8_t blocking_mask, detected_keypresses_on_column;

    APP_ERROR_CHECK(status);

    m_key_vector_size   = 0;
    m_keys_blocked      = false;
    blocking_mask       = 0;

    for (column = 0; column < KEYBOARD_NUM_OF_COLUMNS; column++)
    {
        if (m_row_state[column] == 0)
        {
            continue;
        }

        detected_keypresses_on_column = 0;

        for (row = 0; row < KEYBOARD_NUM_OF_ROWS; row++)
        {
            if ((m_row_state[column] & (1ul << row)) != 0)
            {
                detected_keypresses_on_column += 1;

                if (m_key_vector_size < DRV_KEYBOARD_MAX_KEYS)
                {
                    m_key_vector[m_key_vector_size++] = KEYBOARD_KEY_ID(row, column);
                }
            }
        }

        if (((blocking_mask & m_row_state[column]) != 0) && (detected_keypresses_on_column > 1))
        {
            m_keys_blocked = true;
            break;
        }

        blocking_mask |= m_row_state[column];
    }

    nrf_atomic_flag_clear(&m_read_operation_active);
    if (callback != NULL)
    {
        callback(m_key_vector, m_key_vector_size, m_keys_blocked);
    }
}

static ret_code_t drv_keyboard_schedule_scan(drv_keyboard_event_handler_t callback)
{
    static nrf_twi_mngr_transaction_t scan_transaction;
    ret_code_t status;

    if (nrf_atomic_flag_set_fetch(&m_read_operation_active))
    {
        return NRF_ERROR_BUSY;
    }

    scan_transaction.callback            = drv_keyboard_process_scan_result;
    scan_transaction.p_user_data         = (void *)(callback);
    scan_transaction.p_transfers         = s_scan_transfers;
    scan_transaction.number_of_transfers = ARRAY_SIZE(s_scan_transfers);
    scan_transaction.p_required_twi_cfg  = &g_twi_bus_config[CONFIG_KBD_TWI_BUS];

    status = twi_schedule(&scan_transaction);
    if (status != NRF_SUCCESS)
    {
        nrf_atomic_flag_clear(&m_read_operation_active);
    }

    return status;
}

static void drv_keyboard_timer_handler(void *p_context)
{
    ret_code_t status;

    status = drv_keyboard_schedule_scan((drv_keyboard_event_handler_t)(p_context));
    if (status == NRF_ERROR_BUSY)
    {
        NRF_LOG_WARNING("%s(): WARNING: Cannot schedule keyboard scan: Driver is busy!", (uint32_t)__func__);
    }
    else
    {
        APP_ERROR_CHECK(status);
    }
}

ret_code_t drv_keyboard_init(drv_keyboard_event_handler_t keyboard_event_handler)
{
    uint8_t device_check;
    ret_code_t status;

    if (!keyboard_event_handler)
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    status = twi_register_read(&g_twi_bus_config[CONFIG_KBD_TWI_BUS], SX1509_TWI_ADDRESS, RegReset, &device_check);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    if (device_check != 0x00)
    {
       return NRF_ERROR_INTERNAL;
    }

    /* Setting device configuration */
    status = twi_register_bulk_write(&g_twi_bus_config[CONFIG_KBD_TWI_BUS],
                                     SX1509_TWI_ADDRESS,
                                     m_register_config,
                                     ARRAY_SIZE(m_register_config),
                                     false);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    nrf_atomic_flag_clear(&m_read_operation_active);
    m_keyboard_event_handler = keyboard_event_handler;

    return app_timer_create(&m_keyboard_timer,
                            APP_TIMER_MODE_REPEATED,
                            drv_keyboard_timer_handler);

}

ret_code_t drv_keyboard_keys_get(uint8_t *p_pressed_keys,
                                 uint8_t *p_number_of_pressed_keys,
                                 bool *p_keys_blocked)
{
    ret_code_t status;

    status = drv_keyboard_schedule_scan(NULL);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    while (m_read_operation_active)
    {
#if CONFIG_PWR_MGMT_ENABLED
        nrf_pwr_mgmt_run();
#endif
    }

    memcpy(p_pressed_keys, m_key_vector, sizeof(m_key_vector));
    *p_number_of_pressed_keys = m_key_vector_size;
    *p_keys_blocked = m_keys_blocked;

    return NRF_SUCCESS;
}

ret_code_t drv_keyboard_enable(void)
{
    return app_timer_start(m_keyboard_timer,
                           APP_TIMER_TICKS(CONFIG_KBD_POLL_INTERVAL),
                           (void *)m_keyboard_event_handler);
}

ret_code_t drv_keyboard_disable(void)
{
    return app_timer_stop(m_keyboard_timer);
}

#if CONFIG_PWR_MGMT_ENABLED
bool drv_keyboard_shutdown(bool wakeup)
{
    drv_keyboard_disable();
    return true;
}
#endif /* CONFIG_PWR_MGMT_ENABLED */
#endif /* (CONFIG_KBD_ENABLED && (CONFIG_KBD_DRIVER == CONFIG_KBD_DRIVER_SX1509)) */

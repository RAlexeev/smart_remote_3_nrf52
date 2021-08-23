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

#include "nrf.h"
#include "arm_math.h"
#include "nordic_common.h"
#include "nrf_atomic.h"
#include "nrf_assert.h"
#include "app_debug.h"
#include "app_error.h"

#include "drv_touchpad.h"

#include "twi_common.h"
#include "resources.h"

#if CONFIG_TOUCHPAD_ENABLED

#define NRF_LOG_MODULE_NAME drv_touchpad
#define NRF_LOG_LEVEL CONFIG_TOUCHPAD_DRV_LOG_LEVEL
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

#define PRODUCT_ID_BYTES       10U  //!< Number of bytes to be expected to be in the product ID.
#define DEVICE_ADDRESS         0x20 //!< Device address on Two-wire bus.

/**
 * Touchpad register addresses
 */
#define TOUCHPAD_INT_STATUS    0x14 //!< Interrupt status register.
#define TOUCHPAD_BUTTON_STATUS 0x41 //!< Button status register.
#define TOUCHPAD_FINGER0_REL   0x30 //!< First register in finger delta block.
#define TOUCHPAD_GESTURE_FLAGS 0x3A //!< Gesture flags 0.
#define TOUCHPAD_SCROLL        0x3F //!< Scroll zone X / horizontal multifinger scroll.
#define TOUCHPAD_CONTROL       0x42 //!< Device control register.

#define TOUCHPAD_PRODUCT_ID    0xA2 //!< Address of the product ID string.

/**@brief Operational states */
typedef enum
{
  SleepmodeNormal        = 0x00, /**< Normal operation. */
  SleepmodeSensorSleep   = 0x01, /**< Low power operation. */
} TouchpadSleepMode_t;

/**@brief Tap types detected by the touchpad */
enum ToucphadTapType
{
  Press      = BIT_5,
  EarlyTap   = BIT_3,
  DoubleTap  = BIT_2,
  TapAndHold = BIT_1,
  SingleTap  = BIT_0,
  NoTap      = 0
};

static ret_code_t touchpad_product_id_verify(void);
static ret_code_t touchpad_product_id_read(uint8_t *product_id, uint8_t product_id_bytes);

static void touchpad_read_callback(ret_code_t status, void *p_user_data);
static void touchpad_enable_disable_callback(ret_code_t status, void *p_user_data);

static drv_touchpad_read_handler_t m_read_handler;
static uint8_t m_finger_state[3];
static uint8_t m_buffer[7];

/**@brief Flag protecting shared data used in read operation */
static nrf_atomic_flag_t m_read_operation_active;

static const uint8_t expected_product_id[PRODUCT_ID_BYTES] = {'T', 'M', '1', '9', '4', '4', '-', '0', '0', '2'};  //!< Product ID expected to be received from product ID query.

static uint8_t m_int_status_reg_addr    = TOUCHPAD_INT_STATUS;
static uint8_t m_finger0_rel_reg_addr   = TOUCHPAD_FINGER0_REL;
static uint8_t m_button_status_reg_addr = TOUCHPAD_BUTTON_STATUS;
static uint8_t m_gesture_flags_reg_addr = TOUCHPAD_GESTURE_FLAGS;
static uint8_t m_scroll_reg_addr        = TOUCHPAD_SCROLL;

static const nrf_twi_mngr_transfer_t m_touchpad_read_transfers[] =
{
    NRF_TWI_MNGR_WRITE  (DEVICE_ADDRESS, &m_int_status_reg_addr,     1, NRF_TWI_MNGR_NO_STOP),
    NRF_TWI_MNGR_READ   (DEVICE_ADDRESS, &m_finger_state[0],         3, 0),
    NRF_TWI_MNGR_WRITE  (DEVICE_ADDRESS, &m_finger0_rel_reg_addr,    1, NRF_TWI_MNGR_NO_STOP),
    NRF_TWI_MNGR_READ   (DEVICE_ADDRESS, &m_buffer[0],               2, 0),
    NRF_TWI_MNGR_WRITE  (DEVICE_ADDRESS, &m_button_status_reg_addr,  1, NRF_TWI_MNGR_NO_STOP),
    NRF_TWI_MNGR_READ   (DEVICE_ADDRESS, &m_buffer[2],               1, 0),
    NRF_TWI_MNGR_WRITE  (DEVICE_ADDRESS, &m_gesture_flags_reg_addr,  1, NRF_TWI_MNGR_NO_STOP),
    NRF_TWI_MNGR_READ   (DEVICE_ADDRESS, &m_buffer[3],               2, 0),
    NRF_TWI_MNGR_WRITE  (DEVICE_ADDRESS, &m_scroll_reg_addr,         1, NRF_TWI_MNGR_NO_STOP),
    NRF_TWI_MNGR_READ   (DEVICE_ADDRESS, &m_buffer[5],               2, 0),
};

static uint8_t m_enable_data[]  = { TOUCHPAD_CONTROL, SleepmodeNormal };
static uint8_t m_disable_data[] = { TOUCHPAD_CONTROL, SleepmodeSensorSleep };

static const nrf_twi_mngr_transfer_t m_enable_transfers[] =
{
    NRF_TWI_MNGR_WRITE   (DEVICE_ADDRESS, m_enable_data,  sizeof(m_enable_data),  0),
};

static const nrf_twi_mngr_transfer_t m_disable_transfers[] =
{
    NRF_TWI_MNGR_WRITE   (DEVICE_ADDRESS, m_disable_data, sizeof(m_disable_data), 0),
};

static const nrf_twi_mngr_transaction_t m_enable_transaction =
{
    .callback            = touchpad_enable_disable_callback,
    .p_user_data         = NULL,
    .p_transfers         = m_enable_transfers,
    .number_of_transfers = ARRAY_SIZE(m_enable_transfers),
    .p_required_twi_cfg  = &g_twi_bus_config[CONFIG_TOUCHPAD_TWI_BUS]
};

static const nrf_twi_mngr_transaction_t m_disable_transaction =
{
    .callback            = touchpad_enable_disable_callback,
    .p_user_data         = NULL,
    .p_transfers         = m_disable_transfers,
    .number_of_transfers = ARRAY_SIZE(m_disable_transfers),
    .p_required_twi_cfg  = &g_twi_bus_config[CONFIG_TOUCHPAD_TWI_BUS]
};

static void touchpad_read_callback(ret_code_t status, void *p_user_data)
{
    drv_touchpad_data_t *p_data = p_user_data;
    static bool tap_is_held;
    float       x32f;
    float       y32f;
    float       sqrtf;

    if (status != NRF_SUCCESS)
    {
        nrf_atomic_flag_clear(&m_read_operation_active);
        m_read_handler(status, p_data);
        return;
    }

    // Check for scroll/pan gesture.
    if (m_buffer[4] & BIT_4) // Two-finger scroll detected
    {
        p_data->x       = 0;
        p_data->y       = 0;
        p_data->scroll  = (int8_t) m_buffer[6] / 2;
        p_data->pan     = (int8_t) m_buffer[5] / 2;
    }
    else
    {
        // Exponential increase of cursor movement speed:
        // m_x = x*sqrt(abs(x)); m_y = y*sqrt(abs(y)
        x32f = (float)((int8_t)m_buffer[0]);
        arm_sqrt_f32(x32f * (x32f < 0 ? -1 : 1), &sqrtf);
        p_data->x = (int16_t)(x32f * sqrtf);

        y32f = (float)((int8_t)m_buffer[1]);
        arm_sqrt_f32(y32f * (y32f < 0 ? -1 : 1), &sqrtf);
        p_data->y = -(int16_t)(y32f * sqrtf);

        p_data->scroll  = 0;
        p_data->pan     = 0;
    }

    // Process taps.
    if (m_buffer[3] & EarlyTap)
    {
        p_data->tap         = true;
    }
    else if (m_buffer[3] & SingleTap)
    {
        p_data->tap         = false;
    }
    else if (m_buffer[3] & TapAndHold)
    {
        p_data->tap         = true;
        tap_is_held         = true;
    }
    else if (m_buffer[3] & DoubleTap)
    {
        p_data->tap         = true;
    }
    else if ((m_finger_state[1] || (m_finger_state[2] & 0x03)) == 0)
    {
        p_data->tap         = false;
        tap_is_held         = false;
    }
    else
    {
        p_data->tap         = tap_is_held;
    }

    nrf_atomic_flag_clear(&m_read_operation_active);
    m_read_handler(status, p_data);
}

static void touchpad_enable_disable_callback(ret_code_t status, void *p_user_data)
{
    APP_ERROR_CHECK(status);
}

ret_code_t drv_touchpad_init(drv_touchpad_read_handler_t read_handler)
{
    ret_code_t status;

    ASSERT(read_handler != NULL);

    // Read and verify product ID.
    status = touchpad_product_id_verify();
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    nrf_atomic_flag_clear(&m_read_operation_active);
    m_read_handler = read_handler;

    return drv_touchpad_disable();
}

ret_code_t drv_touchpad_enable(void)
{
    return twi_schedule(&m_enable_transaction);
}

ret_code_t drv_touchpad_disable(void)
{
    return twi_schedule(&m_disable_transaction);
}

ret_code_t drv_touchpad_schedule_read(drv_touchpad_data_t *p_data)
{
    static nrf_twi_mngr_transaction_t read_transaction;
    ret_code_t status;

    ASSERT(p_data != NULL);

    if (nrf_atomic_flag_set_fetch(&m_read_operation_active))
    {
        return NRF_ERROR_BUSY;
    }

    read_transaction.callback            = touchpad_read_callback;
    read_transaction.p_user_data         = p_data;
    read_transaction.p_transfers         = m_touchpad_read_transfers;
    read_transaction.number_of_transfers = ARRAY_SIZE(m_touchpad_read_transfers);
    read_transaction.p_required_twi_cfg  = &g_twi_bus_config[CONFIG_TOUCHPAD_TWI_BUS];

    status = twi_schedule(&read_transaction);
    if (status != NRF_SUCCESS)
    {
        nrf_atomic_flag_clear(&m_read_operation_active);
    }

    return status;
}

static ret_code_t touchpad_product_id_verify(void)
{
    uint8_t product_id[PRODUCT_ID_BYTES];
    ret_code_t status;

    status = touchpad_product_id_read(product_id, PRODUCT_ID_BYTES);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    if (memcmp(expected_product_id, product_id, PRODUCT_ID_BYTES) != 0)
    {
        return NRF_ERROR_NOT_FOUND;
    }

    return NRF_SUCCESS;
}

static ret_code_t touchpad_product_id_read(uint8_t *product_id, uint8_t product_id_bytes)
{
    uint8_t register_address = TOUCHPAD_PRODUCT_ID;
    nrf_twi_mngr_transfer_t transfers[2];

    transfers[0].p_data     = &register_address;
    transfers[0].length     = sizeof(register_address);
    transfers[0].operation  = NRF_TWI_MNGR_WRITE_OP(DEVICE_ADDRESS);
    transfers[0].flags      = NRF_TWI_MNGR_NO_STOP;

    transfers[1].p_data     = product_id;
    transfers[1].length     = product_id_bytes;
    transfers[1].operation  = NRF_TWI_MNGR_READ_OP(DEVICE_ADDRESS);
    transfers[1].flags      = 0;

    return twi_perform(&g_twi_bus_config[CONFIG_TOUCHPAD_TWI_BUS], transfers, ARRAY_SIZE(transfers));
}

#endif /* CONFIG_TOUCHPAD_ENABLED */

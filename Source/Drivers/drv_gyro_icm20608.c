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

#include "nordic_common.h"
#include "nrf_atomic.h"
#include "nrf_assert.h"
#include "nrf_error.h"
#include "app_error.h"
#include "app_timer.h"
#include "app_debug.h"
#include "app_scheduler.h"

#include "resources.h"
#include "twi_common.h"
#include "sr3_config.h"

#if CONFIG_GYRO_ENABLED

#define NRF_LOG_MODULE_NAME drv_gyro
#define NRF_LOG_LEVEL CONFIG_GYRO_DRV_LOG_LEVEL
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

#include "drv_gyro.h"

#define SLEEP_TO_ON_TIMEOUT_MSEC      50

#define ICM20608_WHOAMI               0xAF

//-----------------------------------------------------------------------------
// Register map
//-----------------------------------------------------------------------------
#define ICM20608_REG_SMPLRT_DIV       0x19
#define ICM20608_REG_CONFIG           0x1A
#define ICM20608_REG_GYRO_CONFIG      0x1B
#define ICM20608_REG_ACCEL_CONFIG     0x1C
#define ICM20608_REG_ACCEL_CONFIG2    0x1D
#define ICM20608_REG_LP_MODE_CFG      0x1E
#define ICM20608_REG_ACCEL_WOM_THR    0x1F

#define ICM20608_REG_FIFO_ENABLE      0x23

#define ICM20608_REG_INT_PIN_CFG      0x37
#define ICM20608_REG_INT_ENABLE       0x38

#define ICM20608_REG_INT_STATUS       0x3A
#define ICM20608_REG_ACCEL_XOUT_H     0x3B

#define ICM20608_REG_GYRO_XOUT_H      0x43

#define ICM20608_REG_ACCEL_INTEL_CTRL 0x69

#define ICM20608_REG_USER_CTRL        0x6A

#define ICM20608_REG_PWR_MGMT_1       0x6B
#define ICM20608_REG_PWR_MGMT_2       0x6C

#define ICM20608_REG_FIFO_COUNT_H     0x72
#define ICM20608_REG_FIFO_RW          0x74

#define ICM20608_REG_WHO_AM_I         0x75

// Config
#define ICM20628_DLPF_CFG_6           0x06

// Gyro config
#define ICM20608_GYRO_FS_1000DPS      0x10

// Accel config
#define ICM20628_ACCEL_FS_2G          0x00

// Accel config 2
#define ICM20628_ACCEL_DLPF_CFG_6     0x06

// Low Power Mode configuration
#define ICM20628_LPOSC_CLKSEL_9       0x09

// FIFO enable
#define ICM20608_XG_FIFO_EN           0x40
#define ICM20608_YG_FIFO_EN           0x20
#define ICM20608_ZG_FIFO_EN           0x10
#define ICM20608_ACCEL_FIFO_EN        0x08

// INT/DRDY pin / Bypass Enable configuration
#define ICM20608_LATCH_INT_EN         0x20

// Interrupt enable
#define ICM20628_WOM_INT_EN           0xE0

// Accelerometer Intelligence control
#define ICM20628_ACCEL_INTEL_EN       0x80
#define ICM20628_ACCEL_INTEL_MODE     0x40

// User control
#define ICM20608_FIFO_EN              0x40
#define ICM20608_FIFO_RST             0x04

// Power management 1
#define ICM20608_ACCEL_CYCLE          0x20
#define ICM20608_CLKSEL_AUTO          0x01
#define ICM20608_SLEEP                0x40

// Power management 2
#define ICM20608_PWR_MGMT_2_ACCEL_OFF 0x38
#define ICM20608_PWR_MGMT_2_GYRO_OFF  0x07

// Who Am I value
#define ICM20608_WHOAMI               0xAF

static void drv_gyro_state_change_callback(ret_code_t status, void *p_user_data);

/**@brief State of gyro driver  */
typedef enum
{
    DRV_GYRO_STATE_OFF,            /**< Stable. Wait for wake up event. */
    DRV_GYRO_STATE_WAKING_UP1,     /**< Transient. Wait for waking up TWI transaction end */
    DRV_GYRO_STATE_WAKING_UP2,     /**< Transient. Wait Boot up timeout */
    DRV_GYRO_STATE_SHUTTING_DOWN,  /**< Transient. Wait for disabling TWI transaction end event */
    DRV_GYRO_STATE_ON,             /**< Stable. Wait for shut down event */
} drv_gyro_power_state_t;

/**@brief Internal events handled by gyro */
typedef enum
{
    DRV_GYRO_EVENT_WAKE_UP,    /**< Generated by drv_gyro_enable() function */
    DRV_GYRO_EVENT_WOKEN_UP,   /**< Generated by waking up TWI transaction callback */
    DRV_GYRO_EVENT_ENABLED,    /**< Generated by timer set for boot up timeout */
    DRV_GYRO_EVENT_SHUT_DOWN,  /**< Generated by drv_gyro_disable() function */
    DRV_GYRO_EVENT_DISABLED    /**< Generated by disbling TWI transaction callback */
} drv_gyro_control_event_t;

/**@brief Gyroscope ready handler */
static drv_gyro_ready_handler_t s_ready_handler;

/**@brief Gyroscope read data handler */
static drv_gyro_read_handler_t s_read_handler;

/**@brief ICM20608 configuration */
static const uint8_t s_register_config[][2] =
{
    { ICM20608_REG_PWR_MGMT_1,        ICM20608_CLKSEL_AUTO                                         },
    { ICM20608_REG_SMPLRT_DIV,        0x09                                                         },
    { ICM20608_REG_GYRO_CONFIG,       ICM20608_GYRO_FS_1000DPS                                     },
    { ICM20608_REG_CONFIG,            ICM20628_DLPF_CFG_6,                                         },
    { ICM20608_REG_ACCEL_CONFIG,      ICM20628_ACCEL_FS_2G,                                        },
    { ICM20608_REG_ACCEL_CONFIG2,     ICM20628_ACCEL_DLPF_CFG_6                                    },
    { ICM20608_REG_PWR_MGMT_2,        ICM20608_PWR_MGMT_2_ACCEL_OFF | ICM20608_PWR_MGMT_2_GYRO_OFF },
    { ICM20608_REG_PWR_MGMT_1,        ICM20608_CLKSEL_AUTO | ICM20608_SLEEP                        },
};

/**@brief ICM20608 configuration update for on wake up - SLEEP bit OFF */
static int8_t s_wake_up_config[][2] =
{
    { ICM20608_REG_PWR_MGMT_1,  ICM20608_CLKSEL_AUTO                                               },
    { ICM20608_REG_PWR_MGMT_2,  0x00                                                               },
};

/**@brief ICM20608 configuration update for sleep state */
static uint8_t s_sleep_config[][2] =
{
    { ICM20608_REG_PWR_MGMT_2,  ICM20608_PWR_MGMT_2_ACCEL_OFF | ICM20608_PWR_MGMT_2_GYRO_OFF       },
    { ICM20608_REG_PWR_MGMT_1,  ICM20608_CLKSEL_AUTO | ICM20608_SLEEP                              },
};

/**@brief Transfers for gyroscope waking up */
static const nrf_twi_mngr_transfer_t s_wake_up_transfers[] =
{
    NRF_TWI_MNGR_WRITE  (ICM20608_TWI_ADDRESS, s_wake_up_config[0], 2, 0),
    NRF_TWI_MNGR_WRITE  (ICM20608_TWI_ADDRESS, s_wake_up_config[1], 2, 0),
};

/**@brief Transfers for putting gyroscope in sleep mode */
static const nrf_twi_mngr_transfer_t s_sleep_transfers[] =
{
    NRF_TWI_MNGR_WRITE  (ICM20608_TWI_ADDRESS, s_sleep_config[0], 2, 0),
    NRF_TWI_MNGR_WRITE  (ICM20608_TWI_ADDRESS, s_sleep_config[1], 2, 0),
};

/**@brief Variable holding INT_STATUS register address */
static uint8_t s_accel_xout_h_reg_addr = ICM20608_REG_ACCEL_XOUT_H;

/**@brief Buffer used in gyroscope read operation */
static uint8_t s_read_buffer[14];

/**@brief Transfers for reading data */
static const nrf_twi_mngr_transfer_t s_read_transfers[] =
{
    NRF_TWI_MNGR_WRITE  (ICM20608_TWI_ADDRESS, &s_accel_xout_h_reg_addr, 1, NRF_TWI_MNGR_NO_STOP),
    NRF_TWI_MNGR_READ   (ICM20608_TWI_ADDRESS, &s_read_buffer[0], sizeof(s_read_buffer), 0),
};

/**@brief Transaction for gyroscope waking up  */
static const nrf_twi_mngr_transaction_t s_wake_up_transaction =
{
    .callback               = drv_gyro_state_change_callback,
    .p_user_data            = (void *)(DRV_GYRO_EVENT_WOKEN_UP),
    .p_transfers            = s_wake_up_transfers,
    .number_of_transfers    = ARRAY_SIZE(s_wake_up_transfers),
    .p_required_twi_cfg     = &g_twi_bus_config[CONFIG_GYRO_TWI_BUS],
};

/**@brief Transaction for putting gyroscope in sleep mode  */
static nrf_twi_mngr_transaction_t s_sleep_transaction =
{
    .callback               = drv_gyro_state_change_callback,
    .p_user_data            = (void *)(DRV_GYRO_EVENT_DISABLED),
    .p_transfers            = s_sleep_transfers,
    .number_of_transfers    = ARRAY_SIZE(s_sleep_transfers),
    .p_required_twi_cfg     = &g_twi_bus_config[CONFIG_GYRO_TWI_BUS],
};

/**@brief TWI transaction structure used in read operation */
static nrf_twi_mngr_transaction_t s_read_transaction;

/**@brief Variable protecting shared data used in read operation */
static nrf_atomic_flag_t s_read_operation_active;

/**@brief Current state gyro */
static drv_gyro_power_state_t s_drv_gyro_cur_state = DRV_GYRO_STATE_OFF;

/**@brief Required stable state gyro */
static drv_gyro_power_state_t s_drv_gyro_req_state = DRV_GYRO_STATE_OFF;

/**@brief gyro power power mode timer */
APP_TIMER_DEF(s_timer);


/**@brief Get state name */
static const char *drv_gyro_state_name(drv_gyro_power_state_t state)
{
    char *name = NULL;

    switch (state)
    {
        case DRV_GYRO_STATE_OFF:
            name = STRINGIFY(DRV_GYRO_STATE_OFF);
            break;

        case DRV_GYRO_STATE_WAKING_UP1:
            name = STRINGIFY(DRV_GYRO_STATE_WAKING_UP1);
            break;

        case DRV_GYRO_STATE_WAKING_UP2:
            name = STRINGIFY(DRV_GYRO_STATE_WAKING_UP2);
            break;

        case DRV_GYRO_STATE_SHUTTING_DOWN:
            name = STRINGIFY(DRV_GYRO_STATE_SHUTTING_DOWN);
            break;

        case DRV_GYRO_STATE_ON:
            name = STRINGIFY(DRV_GYRO_STATE_ON);
            break;

        default:
            ASSERT(false);
            break;
    }

    return name;
}

/**@brief Verifying ICM20608 product ID
 *
 * @return NRF_SUCCESS if ICM20608 chip has been detected, otherwise an error code.
 */
static ret_code_t icm20608_verify_product_id(void)
{
    ret_code_t status;
    uint8_t whoami;

    status = twi_register_read(&g_twi_bus_config[CONFIG_GYRO_TWI_BUS],
                               ICM20608_TWI_ADDRESS,
                               ICM20608_REG_WHO_AM_I,
                               &whoami);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    if (whoami != ICM20608_WHOAMI)
    {
        return NRF_ERROR_NOT_FOUND;
    }

    return NRF_SUCCESS;
}

/**@brief HAL TWI callback processing gyroscope FIFO data */
static void drv_gyro_read_callback(ret_code_t status, void *p_user_data)
{
    t_struct_AIR_MOTION_ProcessDeltaSamples *p_samples = p_user_data;

    if (status == NRF_SUCCESS)
    {
        p_samples->AccSamples.X  =  ((int16_t)((s_read_buffer[0+0] << 8) | s_read_buffer[0+1]));
        p_samples->AccSamples.Y  = -((int16_t)((s_read_buffer[0+2] << 8) | s_read_buffer[0+3]));
        p_samples->AccSamples.Z  = -((int16_t)((s_read_buffer[0+4] << 8) | s_read_buffer[0+5]));

        p_samples->GyroSamples.X =  ((int16_t)((s_read_buffer[8+0] << 8) | s_read_buffer[8+1]));
        p_samples->GyroSamples.Y =  ((int16_t)((s_read_buffer[8+2] << 8) | s_read_buffer[8+3]));
        p_samples->GyroSamples.Z =  ((int16_t)((s_read_buffer[8+4] << 8) | s_read_buffer[8+5]));

        NRF_LOG_DEBUG("%s(): Acc X = %d", (uint32_t)__func__, p_samples->AccSamples.X);
        NRF_LOG_DEBUG("%s(): Acc Y = %d", (uint32_t)__func__, p_samples->AccSamples.Y);
        NRF_LOG_DEBUG("%s(): Acc Z = %d", (uint32_t)__func__, p_samples->AccSamples.Z);

        NRF_LOG_DEBUG("%s(): Gyro X = %d", (uint32_t)__func__, p_samples->GyroSamples.X);
        NRF_LOG_DEBUG("%s(): Gyro Y = %d", (uint32_t)__func__, p_samples->GyroSamples.Y);
        NRF_LOG_DEBUG("%s(): Gyro Z = %d", (uint32_t)__func__, p_samples->GyroSamples.Z);
    }

    nrf_atomic_flag_clear(&s_read_operation_active);
    s_read_handler(status, p_user_data);
}

/**@brief Gyro state handler. Executed only from forward scheduler context.
 */
static void drv_gyro_power_mode_event_handler(void *p_context)
{
    drv_gyro_control_event_t event = (drv_gyro_control_event_t)((unsigned long)(p_context));

    STATIC_ASSERT(sizeof(event) <= sizeof(p_context));

    // Appease some compilers claiming that this variable is not used if
    // debug mode is disabled.
    UNUSED_VARIABLE(event);

    ASSERT((s_drv_gyro_req_state == DRV_GYRO_STATE_ON) ||
           (s_drv_gyro_req_state == DRV_GYRO_STATE_OFF));

    if (s_drv_gyro_cur_state == s_drv_gyro_req_state)
    {
        return;
    }

    drv_gyro_power_state_t const prev_state = s_drv_gyro_cur_state;

    switch (s_drv_gyro_cur_state)
    {
        case DRV_GYRO_STATE_OFF:
            ASSERT(event == DRV_GYRO_EVENT_WAKE_UP);

            s_drv_gyro_cur_state = DRV_GYRO_STATE_WAKING_UP1;
            APP_ERROR_CHECK(twi_schedule(&s_wake_up_transaction));
            break;

        case DRV_GYRO_STATE_WAKING_UP1:
            ASSERT(event == DRV_GYRO_EVENT_WOKEN_UP);

            s_drv_gyro_cur_state = DRV_GYRO_STATE_WAKING_UP2;
            APP_ERROR_CHECK(app_timer_start(s_timer,
                            APP_TIMER_TICKS(SLEEP_TO_ON_TIMEOUT_MSEC),
                            NULL));
            break;

        case DRV_GYRO_STATE_WAKING_UP2:
            ASSERT(event == DRV_GYRO_EVENT_ENABLED);

            if (s_drv_gyro_req_state == DRV_GYRO_STATE_ON)
            {
                s_drv_gyro_cur_state = DRV_GYRO_STATE_ON;
                s_ready_handler();
            }
            else
            {
                s_drv_gyro_cur_state = DRV_GYRO_STATE_SHUTTING_DOWN;
                APP_ERROR_CHECK(twi_schedule(&s_sleep_transaction));
            }
            break;

        case DRV_GYRO_STATE_ON:
            ASSERT(event == DRV_GYRO_EVENT_SHUT_DOWN);

            s_drv_gyro_cur_state = DRV_GYRO_STATE_SHUTTING_DOWN;
            APP_ERROR_CHECK(twi_schedule(&s_sleep_transaction));
            break;

        case DRV_GYRO_STATE_SHUTTING_DOWN:
            ASSERT(event == DRV_GYRO_EVENT_DISABLED);

            if (s_drv_gyro_req_state == DRV_GYRO_STATE_OFF)
            {
                s_drv_gyro_cur_state = DRV_GYRO_STATE_OFF;
            }
            else
            {
                s_drv_gyro_cur_state = DRV_GYRO_STATE_WAKING_UP1;
                APP_ERROR_CHECK(twi_schedule(&s_wake_up_transaction));
            }
            break;

        default:
            // Assert error for unsupported state.
            ASSERT(false);
            break;
    }

    NRF_LOG_INFO("gyro state change %s -> %s (%s required)",
                 drv_gyro_state_name(prev_state),
                 drv_gyro_state_name(s_drv_gyro_cur_state),
                 drv_gyro_state_name(s_drv_gyro_req_state));
}

/**@brief NRF_TWI_MNGR gyroscope state change transaction callback */
static void drv_gyro_state_change_callback(ret_code_t status, void *p_user_data)
{
    APP_ERROR_CHECK(status);
    APP_ERROR_CHECK(app_isched_event_put(&g_fg_scheduler,
                                         drv_gyro_power_mode_event_handler,
                                         p_user_data));
}

/**@brief APP_timer callback for SLEEP to ACTIVE timeout  */
static void drv_gyro_timer_handler(void * p_context)
{
    APP_ERROR_CHECK(app_isched_event_put(&g_fg_scheduler,
                                         drv_gyro_power_mode_event_handler,
                                         (void *)(DRV_GYRO_EVENT_ENABLED)));
}

/**@brief schedules gyroscope read.
 *
 * @param[out]  p_samples   Pointer to structure which will be filled up by data.
 *
 * @return NRF_SUCCESS if the read request was successfully registered. Otherwise an error code.
 */
static ret_code_t drv_gyro_schedule_read_transaction(t_struct_AIR_MOTION_ProcessDeltaSamples *p_samples)
{
    ret_code_t status;

    if (p_samples == NULL)
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    if (nrf_atomic_flag_set_fetch(&s_read_operation_active))
    {
        return NRF_ERROR_BUSY;
    }

    s_read_transaction.callback             = drv_gyro_read_callback;
    s_read_transaction.p_user_data          = p_samples;
    s_read_transaction.p_transfers          = s_read_transfers;
    s_read_transaction.number_of_transfers  = ARRAY_SIZE(s_read_transfers);
    s_read_transaction.p_required_twi_cfg   = &g_twi_bus_config[CONFIG_GYRO_TWI_BUS],

    status = twi_schedule(&s_read_transaction);
    if (status != NRF_SUCCESS)
    {
        nrf_atomic_flag_clear(&s_read_operation_active);
    }

    return status;
}

ret_code_t drv_gyro_init(drv_gyro_ready_handler_t ready_handler, drv_gyro_read_handler_t read_handler)
{
    ret_code_t status;

    ASSERT(read_handler  != NULL);
    ASSERT(ready_handler != NULL);

    // Read and verify product ID.
    status = icm20608_verify_product_id();
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    status = app_timer_create(&s_timer,
                              APP_TIMER_MODE_SINGLE_SHOT,
                              drv_gyro_timer_handler);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    // Set device configuration.
    status = twi_register_bulk_write(&g_twi_bus_config[CONFIG_GYRO_TWI_BUS],
                                     ICM20608_TWI_ADDRESS,
                                     s_register_config,
                                     ARRAY_SIZE(s_register_config),
                                     true);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    // Initialize driver state.
    nrf_atomic_flag_clear(&s_read_operation_active);

    s_drv_gyro_cur_state = DRV_GYRO_STATE_OFF;
    s_drv_gyro_req_state = DRV_GYRO_STATE_OFF;

    s_ready_handler = ready_handler;
    s_read_handler  = read_handler;

    return NRF_SUCCESS;
}

ret_code_t drv_gyro_schedule_read(t_struct_AIR_MOTION_ProcessDeltaSamples *p_samples)
{
    ASSERT(p_samples != NULL);

    if (s_drv_gyro_cur_state != DRV_GYRO_STATE_ON)
    {
        return NRF_ERROR_INVALID_STATE;
    }

    return drv_gyro_schedule_read_transaction(p_samples);
}

ret_code_t drv_gyro_enable(void)
{
    NRF_LOG_INFO("%s()", __func__);

    s_drv_gyro_req_state = DRV_GYRO_STATE_ON;

    if (s_drv_gyro_cur_state == DRV_GYRO_STATE_OFF)
    {
        // Gyro is in stable off state - trigger state change.
        return app_isched_event_put(&g_fg_scheduler,
                                    drv_gyro_power_mode_event_handler,
                                    (void *)(DRV_GYRO_EVENT_WAKE_UP));
    }

    return NRF_SUCCESS;
}

ret_code_t drv_gyro_disable(void)
{
    NRF_LOG_INFO("%s()", __func__);

    s_drv_gyro_req_state = DRV_GYRO_STATE_OFF;

    if (s_drv_gyro_cur_state == DRV_GYRO_STATE_ON)
    {
        // Gyro is in stable on state - trigger state change.
        return app_isched_event_put(&g_fg_scheduler,
                                    drv_gyro_power_mode_event_handler,
                                    (void *)(DRV_GYRO_EVENT_SHUT_DOWN));
    }

    return NRF_SUCCESS;
}
#endif /* CONFIG_GYRO_ENABLED */

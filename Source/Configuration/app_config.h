/**
 * Copyright (c) 2017 - 2018, Nordic Semiconductor ASA
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

#ifndef APP_CONFIG_H
#define APP_CONFIG_H
#include "sr3_config.h"

// nRF_BLE_Services
//==========================================================

// Battery service
#define BLE_BAS_ENABLED     (CONFIG_BATT_MEAS_ENABLED)

// Nordic UART Service
#define BLE_NUS_ENABLED     (CONFIG_CLI_BLE_ENABLED)

// Immediate Alert Service
#define BLE_IAS_ENABLED     (CONFIG_LED_SIGNAL_IMMEDIATE_ALERT || CONFIG_BUZZER_ENABLED)

// nRF_Core
//==========================================================

// Enable CLI commands for MPU module
#define NRF_MPU_CLI_CMDS            (CONFIG_CLI_ENABLED)

// Module for Protecting Stack
#define NRF_STACK_GUARD_ENABLED     (CONFIG_STACK_GUARD_ENABLED)

// Size of stack guard
#define NRF_STACK_GUARD_CONFIG_SIZE (CONFIG_STACK_GUARD_CONFIG_SIZE)

// nRF_Crypto
//==========================================================

// Cryptography library
#define NRF_CRYPTO_ENABLED  (CONFIG_SEC_LESC_PAIRING)

// nRF_Drivers
//==========================================================

// PDM peripheral driver
#define PDM_ENABLED                  (CONFIG_AUDIO_ENABLED)

// PPI peripheral driver
#define PPI_ENABLED                  (CONFIG_PA_LNA_ENABLED)

// PWM peripheral driver
#define PWM_ENABLED                  (CONFIG_PWM0_ENABLED || CONFIG_PWM1_ENABLED)
// Enable PWM0 instance
#define PWM0_ENABLED                 (CONFIG_PWM0_ENABLED)
// Enable PWM1 instance
#define PWM1_ENABLED                 (CONFIG_PWM1_ENABLED)

// SAADC peripheral driver
#define SAADC_ENABLED                (CONFIG_BATT_MEAS_ENABLED)

// TWI/TWIM peripheral driver
#define TWI_ENABLED                  (CONFIG_TWI0_ENABLED || CONFIG_TWI1_ENABLED)
// Enable TWI0 instance
#define TWI0_ENABLE                  (CONFIG_TWI0_ENABLED)
// Enable TWI1 instance
#define TWI1_ENABLE                  (CONFIG_TWI1_ENABLED)

// Enable UART
#define UART_ENABLED                 (CONFIG_LOG_UART_ENABLED || CONFIG_CLI_UART_ENABLED)
// Enable hardware flow control
#define UART_DEFAULT_CONFIG_HWFC     (CONFIG_CL_UART_HWFC)
// Set parity
#define UART_DEFAULT_CONFIG_PARITY   (CONFIG_CL_UART_PARITY)
//Set baudrate
#define UART_DEFAULT_CONFIG_BAUDRATE (CONFIG_CL_UART_BAUDRATE)
// <o> Default Baudrate

// WDT peripheral driver
#define WDT_ENABLED                  (CONFIG_WATCHDOG_ENABLED)

// RNG driver
#define RNG_ENABLED                  (CONFIG_NFC_ENABLED || CONFIG_SEC_LESC_PAIRING)

// nRF_Libraries
//==========================================================

// TWI transaction manager
#define NRF_TWI_MNGR_ENABLED                            (TWI_ENABLED)

// Bypass the GDB problem with multiple stack pointers backtrace
#define HARDFAULT_HANDLER_GDB_PSP_BACKTRACE             (CONFIG_DEBUG_ENABLED)

// Dynamic memory allocator
#define MEM_MANAGER_ENABLED                             (NRF_CRYPTO_ENABLED)

// Block allocator module
#define NRF_BALLOC_CONFIG_DEBUG_ENABLED                 (CONFIG_DEBUG_ENABLED)

// ble_uart command line interface transport
#define NRF_CLI_BLE_UART_ENABLED                        (CONFIG_CLI_BLE_ENABLED)

// uart command line interface transport
#define NRF_CLI_UART_ENABLED                            (CONFIG_CLI_UART_ENABLED)

// fprintf function
#define NRF_FPRINTF_ENABLED                             (CONFIG_LOG_ENABLED || CONFIG_CLI_ENABLED)

// Linked memory allocator module
#define NRF_MEMOBJ_ENABLED                              (CONFIG_LOG_ENABLED)

// Power management module
#define NRF_PWR_MGMT_ENABLED                            (CONFIG_PWR_MGMT_ENABLED)

// Standby timeout (in seconds)
#define NRF_PWR_MGMT_CONFIG_STANDBY_TIMEOUT_S           (CONFIG_PWR_MGMT_CONFIG_STANDBY_TIMEOUT_S)

// CPU usage monitor
#define NRF_PWR_MGMT_CONFIG_CPU_USAGE_MONITOR_ENABLED   (CONFIG_PWR_MGMT_CPU_USAGE_MONITOR_ENABLED)

// Selected pin will be set when CPU is in sleep mode.
#define NRF_PWR_MGMT_CONFIG_DEBUG_PIN_ENABLED           ((CONFIG_DEBUG_PIN_ENABLED) && (IS_IO_VALID(CONFIG_IO_DBG_SLEEP)))

// Pin number
#define NRF_PWR_MGMT_SLEEP_DEBUG_PIN                    (CONFIG_IO_DBG_SLEEP)

// Power management module log level
#define NRF_PWR_MGMT_CONFIG_LOG_LEVEL                   (CONFIG_PWR_MGMT_MODULE_LOG_LEVEL)

// Enable CLI commands for Task Manager module
#define TASK_MANAGER_CLI_CMDS                           (CONFIG_CLI_ENABLED)

// Maximum number of tasks which can be created
#define TASK_MANAGER_CONFIG_MAX_TASKS                   (CONFIG_CLI_BLE_ENABLED + CONFIG_CLI_RTT_ENABLED + CONFIG_CLI_UART_ENABLED)
// Enable task manager
#define TASK_MANAGER_ENABLED                            (TASK_MANAGER_CONFIG_MAX_TASKS > 0)
// Stack size for every task
#define TASK_MANAGER_CONFIG_STACK_SIZE                  (CONFIG_TASK_MANAGER_CONFIG_STACK_SIZE)
// Configures stack guard
#define TASK_MANAGER_CONFIG_STACK_GUARD                 (CONFIG_TASK_MANAGER_CONFIG_STACK_GUARD)

// Command line interface.
#define NRF_CLI_ENABLED                                 (CONFIG_CLI_ENABLED)
// Enable logger backend interface
#define NRF_CLI_LOG_BACKEND                             (CONFIG_LOG_CLI_ENABLED)
// Enable/disable CLI RTT module.
#define NRF_CLI_RTT_ENABLED                             (CONFIG_CLI_RTT_ENABLED)

// nRF_Log
//==========================================================

// Enable CLI commands for Logger module
#define NRF_LOG_CLI_CMDS                (CONFIG_CLI_ENABLED)
// Logging module for nRF5 SDK
#define NRF_LOG_ENABLED                 (CONFIG_LOG_ENABLED)
// Default Severity level
#define NRF_LOG_DEFAULT_LEVEL           (CONFIG_LOG_COMPILED_IN_LEVEL)

// Enable RTT as logger backend
#define NRF_LOG_BACKEND_RTT_ENABLED     (CONFIG_LOG_RTT_ENABLED)
// Enable UART as logger backend
#define NRF_LOG_BACKEND_UART_ENABLED    (CONFIG_LOG_UART_ENABLED)
// Set UART TX pin
#define NRF_LOG_BACKEND_UART_TX_PIN     (CONFIG_UART_TX_PIN)

// nRF_NFC
//==========================================================

// Encoding the advertising data and/or scan response data which is specific for OOB pairing
#define NFC_BLE_OOB_ADVDATA_ENABLED     (CONFIG_NFC_ENABLED)
// Hardware Abstraction Layer for NFC library
#define NFC_HAL_ENABLED                 (CONFIG_NFC_ENABLED)
// NFC NDEF Message generator module
#define NFC_NDEF_MSG_ENABLED            (CONFIG_NFC_ENABLED)
// Encoding message in NFC NDEF format for Bluetooth LE pairing over NFC
#define NFC_BLE_PAIR_MSG_ENABLED        (CONFIG_NFC_ENABLED)
// Common data for Connection Handover and Connection Handover Parse modules
#define NFC_CH_COMMON_ENABLED           (CONFIG_NFC_ENABLED)
// Declaring record in NFC NDEF format for Bluetooth EP pairing over NFC
#define NFC_EP_OOB_REC_ENABLED          (CONFIG_NFC_ENABLED)
// Declaring record in NFC NDEF format for Bluetooth LE pairing over NFC
#define NFC_LE_OOB_REC_ENABLED          (CONFIG_NFC_ENABLED)
// Declaration of NFC NDEF Handover Select record
#define NFC_HS_REC_ENABLED              (CONFIG_NFC_ENABLED)

// <h> nRF_Segger_RTT
//==========================================================
// <h> segger_rtt - SEGGER RTT

// <o> SEGGER_RTT_CONFIG_MAX_NUM_UP_BUFFERS - Size of upstream buffer.
#if CONFIG_AUDIO_PROBE_ENABLED
#define SEGGER_RTT_CONFIG_MAX_NUM_UP_BUFFERS    (CONFIG_AUDIO_PROBE_RTT_CHANNEL_FIRST + CONFIG_AUDIO_PROBE_RTT_CHANNELS_UP)
#endif

// <o> SEGGER_RTT_CONFIG_MAX_NUM_DOWN_BUFFERS - Size of downstream buffer.
#if CONFIG_AUDIO_PROBE_ENABLED
#define SEGGER_RTT_CONFIG_MAX_NUM_DOWN_BUFFERS  (CONFIG_AUDIO_PROBE_RTT_CHANNEL_FIRST + CONFIG_AUDIO_PROBE_RTT_CHANNELS_DOWN)
#endif

// <h> nRF_SoftDevice
//==========================================================
// Static maximum MTU size.
#define NRF_SDH_BLE_GATT_MAX_MTU_SIZE   (CONFIG_BLE_GATT_MAX_MTU_SIZE)
// Attribute Table size in bytes. The size must be a multiple of 4.
#define NRF_SDH_BLE_GATTS_ATTR_TAB_SIZE (CONFIG_BLE_GATTS_ATTR_TAB_SIZE)
// Include the Service Changed characteristic in the Attribute Table.
#define NRF_SDH_BLE_SERVICE_CHANGED     (CONFIG_DFU_ENABLED)

#endif /* APP_CONFIG_H */

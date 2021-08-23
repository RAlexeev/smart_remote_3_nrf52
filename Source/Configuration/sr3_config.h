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

#ifndef _SR3_CONFIG_H
#define _SR3_CONFIG_H

#include "nordic_common.h"

// SoC configuration:
#define APP_TIMER_PRESCALER                     0
#define IS_IO_VALID(io)                         (((io) & ~0x1F) == 0)

// Audio codecs:
#define CONFIG_AUDIO_CODEC_ADPCM                1
#define CONFIG_AUDIO_CODEC_BV32FP               2
#define CONFIG_AUDIO_CODEC_OPUS                 3
#define CONFIG_AUDIO_CODEC_SBC                  4

// OPUS modes:
#define CONFIG_OPUS_MODE_CELT                   (1 << 0)
#define CONFIG_OPUS_MODE_SILK                   (1 << 1)
#define CONFIG_OPUS_MODE_HYBRID                 (CONFIG_OPUS_MODE_CELT | CONFIG_OPUS_MODE_SILK)

// SBC modes:
#define CONFIG_SBC_MODE_MSBC                    0
#define CONFIG_SBC_MODE_CUSTOM                  1

// PDM microphones:
#define CONFIG_PDM_MIC_LEFT                     1
#define CONFIG_PDM_MIC_RIGHT                    2

// Keyboard drivers:
#define CONFIG_KBD_DRIVER_GPIO                  1
#define CONFIG_KBD_DRIVER_SX1509                2

// Accelerometer drivers:
#define CONFIG_ACC_DRIVER_LIS3DH                1
#define CONFIG_ACC_DRIVER_BMA222E               2

// IR TX protocols:
#define CONFIG_IR_TX_PROTOCOL_SIRC              1
#define CONFIG_IR_TX_PROTOCOL_NEC               2

// CLI Logger interface:
#define CONFIG_CL_INTERFACE_LOGGER              1
#define CONFIG_CL_INTERFACE_CLI                 2

// Connection bandwidth tag:
#define CONFIG_CONN_CFG_HIGH_BW_TAG             1

// Fetch board configuration:
#if defined(CONFIG_BOARD_NRF52832_PCA20023)
# define CONFIG_BOARD "PCA20023 (Smart Remote 3 nRF52832 Product Example)"
# include "sr3_config_nrf52832_pca20023.h"
#elif defined(CONFIG_BOARD_NRF52832_PCA63519)
# define CONFIG_BOARD "PCA63519 (Smart Remote 3 nRF52832 Shield)"
# include "sr3_config_nrf52832_pca63519.h"
#elif defined(CONFIG_BOARD_NRF52832_CUSTOM)
# define CONFIG_BOARD "CUSTOM (Smart Remote 3 nRF52832 Custom version)"
# include "sr3_config_nrf52832_custom.h"
#elif defined(CONFIG_BOARD_NRF52810_PCA20031)
# define CONFIG_BOARD "PCA20031 (Smart Remote 3 nRF52810 Product Example)"
# include "sr3_config_nrf52810_pca20031.h"
#elif defined(CONFIG_BOARD_NRF52810_CUSTOM)
# define CONFIG_BOARD "CUSTOM (Smart Remote 3 nRF52810 Custom version)"
# include "sr3_config_nrf52810_custom.h"
#else
# define CONFIG_BOARD "UNKNOWN"
# error "CONFIG_BOARD_xxx is not defined!"
#endif

// Fetch audio configuration:
#include "sr3_config_audio.h"

// Obtain Logger configuration.
#define CONFIG_LOG_RTT_ENABLED  (CONFIG_CL_RTT_ENABLED && \
                                 ((CONFIG_CL_RTT_INTERFACE == CONFIG_CL_INTERFACE_LOGGER) || \
                                  ((CONFIG_CL_RTT_INTERFACE == CONFIG_CL_INTERFACE_CLI) && CONFIG_CLI_RTT_LOG_BACKEND_ENABLED)))

#define CONFIG_LOG_UART_ENABLED (CONFIG_CL_UART_ENABLED && \
                                 ((CONFIG_CL_UART_INTERFACE == CONFIG_CL_INTERFACE_LOGGER) || \
                                  ((CONFIG_CL_UART_INTERFACE == CONFIG_CL_INTERFACE_CLI) && CONFIG_CLI_UART_LOG_BACKEND_ENABLED)))

#define CONFIG_LOG_CLI_ENABLED  ((((CONFIG_CL_RTT_INTERFACE == CONFIG_CL_INTERFACE_CLI)  && CONFIG_CLI_RTT_LOG_BACKEND_ENABLED)  && CONFIG_CL_RTT_ENABLED)  || \
                                 (((CONFIG_CL_UART_INTERFACE == CONFIG_CL_INTERFACE_CLI) && CONFIG_CLI_UART_LOG_BACKEND_ENABLED) && CONFIG_CL_UART_ENABLED) || \
                                 (CONFIG_CLI_BLE_LOG_BACKEND_ENABLED && CONFIG_CLI_BLE_ENABLED))

#define CONFIG_LOG_ENABLED      (CONFIG_LOG_RTT_ENABLED || CONFIG_LOG_UART_ENABLED || CONFIG_LOG_CLI_ENABLED)

#if CONFIG_LOG_CLI_ENABLED
# define CONFIG_LOG_COMPILED_IN_LEVEL   CONFIG_LOG_LEVEL_CAP
#elif (CONFIG_LOG_RTT_ENABLED && CONFIG_LOG_UART_ENABLED)
# define CONFIG_LOG_COMPILED_IN_LEVEL   MIN(CONFIG_LOG_LEVEL_CAP, MAX(CONFIG_LOG_RTT_SEVERITY, CONFIG_LOG_UART_SEVERITY))
#elif CONFIG_LOG_RTT_ENABLED
# define CONFIG_LOG_COMPILED_IN_LEVEL   MIN(CONFIG_LOG_LEVEL_CAP, CONFIG_LOG_RTT_SEVERITY)
#elif CONFIG_LOG_UART_ENABLED
# define CONFIG_LOG_COMPILED_IN_LEVEL   MIN(CONFIG_LOG_LEVEL_CAP, CONFIG_LOG_UART_SEVERITY)
#else
# define CONFIG_LOG_COMPILED_IN_LEVEL   0
#endif

// Obtain CLI configuration.
#define CONFIG_CLI_RTT_ENABLED  ((CONFIG_CL_RTT_INTERFACE == CONFIG_CL_INTERFACE_CLI) && CONFIG_CL_RTT_ENABLED)
#define CONFIG_CLI_UART_ENABLED ((CONFIG_CL_UART_INTERFACE == CONFIG_CL_INTERFACE_CLI) && CONFIG_CL_UART_ENABLED)

#define CONFIG_CLI_ENABLED      (CONFIG_CLI_BLE_ENABLED || CONFIG_CLI_RTT_ENABLED || CONFIG_CLI_UART_ENABLED)

// Obtain PWM configuration.
#define CONFIG_PWM_INSTANCES_NEEDED (CONFIG_IR_TX_ENABLED + CONFIG_BUZZER_ENABLED)
#define CONFIG_PWM0_ENABLED         (CONFIG_PWM_INSTANCES_NEEDED > 0)
#define CONFIG_PWM1_ENABLED         (CONFIG_PWM_INSTANCES_NEEDED > 1)

#if CONFIG_IR_TX_ENABLED && CONFIG_BUZZER_ENABLED
# ifdef NRF52810_XXAA
#  error "The nRF52810 does not have enough PWM instances for the selected configuration!"
# endif
# define CONFIG_IR_TX_PWM_INSTANCE  NRF_DRV_PWM_INSTANCE(0)
# define CONFIG_BUZZER_PWM_INSTANCE NRF_DRV_PWM_INSTANCE(1)
#elif CONFIG_BUZZER_ENABLED
# define CONFIG_BUZZER_PWM_INSTANCE NRF_DRV_PWM_INSTANCE(0)
#else
# define CONFIG_IR_TX_PWM_INSTANCE  NRF_DRV_PWM_INSTANCE(0)
#endif

// Obtain TWI configuration.
#define CONFIG_TWI0_ENABLED ((CONFIG_ACC_ENABLED                                                    && (CONFIG_ACC_TWI_BUS      == 0)) || \
                             (CONFIG_GYRO_ENABLED                                                   && (CONFIG_GYRO_TWI_BUS     == 0)) || \
                             (CONFIG_KBD_ENABLED && (CONFIG_KBD_DRIVER == CONFIG_KBD_DRIVER_SX1509) && (CONFIG_KBD_TWI_BUS      == 0)) || \
                             (CONFIG_TOUCHPAD_ENABLED                                               && (CONFIG_TOUCHPAD_TWI_BUS == 0)))

#define CONFIG_TWI1_ENABLED ((CONFIG_ACC_ENABLED                                                    && (CONFIG_ACC_TWI_BUS      == 1))  || \
                             (CONFIG_GYRO_ENABLED                                                   && (CONFIG_GYRO_TWI_BUS     == 1))  || \
                             (CONFIG_KBD_ENABLED && (CONFIG_KBD_DRIVER == CONFIG_KBD_DRIVER_SX1509) && (CONFIG_KBD_TWI_BUS      == 1))  || \
                             (CONFIG_TOUCHPAD_ENABLED                                               && (CONFIG_TOUCHPAD_TWI_BUS == 1)))

// Obtain stack guard configuration.
#if CONFIG_STACK_GUARD_ENABLED
# define CONFIG_STACK_SIZE_GUARD    (1 << CONFIG_STACK_GUARD_CONFIG_SIZE)
#else /* !NRF_STACK_GUARD_ENABLED */
# define CONFIG_STACK_SIZE_GUARD    0
#endif /* NRF_STACK_GUARD_ENABLED */

#define CONFIG_STACK_SIZE   (CONFIG_STACK_SIZE_BASE + CONFIG_STACK_SIZE_AUDIO + CONFIG_STACK_SIZE_GUARD)

// Check if dynamic BLE address management is needed.
#define CONFIG_BLE_DYNAMIC_ADDR_ENABLED (CONFIG_SEC_BOND && (CONFIG_MAX_BONDS > 1) && CONFIG_CHANGE_ADDRESS)

// Check if peer rank management is needed.
#define CONFIG_BLE_PEER_RANK_ENABLED    (CONFIG_SEC_BOND && (CONFIG_MAX_BONDS > 1))

// Check if RNG monitor is needed.
#define CONFIG_RNG_MONITOR_ENABLED      (CONFIG_SEC_LESC_PAIRING || CONFIG_NFC_ENABLED)

// Fetch HID configuration:
#include "sr3_config_hid.h"

// Fetch IR configuration:
#include "sr3_config_ir.h"

// Fetch buzzer configuration:
#include "sr3_config_buzzer.h"

#endif /* _SR3_CONFIG_H */

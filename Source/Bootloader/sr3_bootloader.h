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
/**@file
 *
 * @brief Main Smart Remote bootloader file.
 *
 * @defgroup smart_remote_3_nrf52_nrf_dfu Smart Remote Bootloader
 * @ingroup app_common
 * @brief Groups together other bootloader related modules.
 *
 * @defgroup smart_remote_3_nrf52_nrf_ble_dfu BLE DFU module
 * @ingroup smart_remote_3_nrf52_nrf_dfu
 *
 * @defgroup sr3_bootloader_main Bootloader main
 * @{
 * @ingroup smart_remote_3_nrf52_nrf_dfu
 * @brief Contains bootloader main function which calls other modules.
 */

#ifndef SR3_BOOTLOADER_H__
#define SR3_BOOTLOADER_H__

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Initialize the keep alive timer.
 *
 * @details Keep alive timer is started if watchdog is enabled.
 *          It periodically wakes up the CPU to make sure watchdog timer will
 *          be constantly updated.
 */
void sr3_bootloader_keep_alive_timer_init(void);

/** @brief Update watchdog timer.
 *
 * @details Refresh all enabled watchdog channels.
 */
void sr3_bootloader_watchdog_feed(void);

/** @brief Function for initializing the board.
 *
 * @details Perform board initialization when DFU is initialized.
 */
void sr3_bootloader_board_init(void);

#ifdef __cplusplus
}
#endif

#endif /* SR3_BOOTLOADER_H__ */
/** @} */

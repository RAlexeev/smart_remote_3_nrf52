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

#include "nrf_dfu.h"

#include "nrf_power.h"
#include "nrf_soc.h"
#include "nrf_clock.h"
#include "nrf_delay.h"
#include "nrf_drv_clock.h"

#include "nrf_bootloader_app_start.h"
#include "nrf_bootloader_info.h"
#include "nrf_dfu_settings.h"
#include "nrf_dfu_utils.h"
#include "nrf_dfu_transport.h"
#include "nrf_dfu_req_handler.h"
#include "app_scheduler.h"
#include "app_timer.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"

#include "sr3_bootloader.h"
#include "sr3_bootloader_commands.h"


#define SCHED_QUEUE_SIZE 16  /**< Maximum number of events in the scheduler queue. */

// Inactivity timer to resets the device.
// It can be stopped by transports when they are active and restarted as necessary.
static app_timer_t    nrf_dfu_inactivity_timeout_timer    = {{0}};
const  app_timer_id_t nrf_dfu_inactivity_timeout_timer_id = &nrf_dfu_inactivity_timeout_timer;


/**@brief Check if we should enter DFU mode. */
static bool nrf_dfu_enter_check(void)
{
    if (nrf_power_gpregret_get() == BOOTLOADER_DFU_START)
    {
        nrf_power_gpregret_set(0);
        return true;
    }

    return false;
}


/**@brief Initialize clock. */
static void clock_init(void)
{
    ret_code_t err_code;

    // Delay clock initialization (see FTPAN-132)
    nrf_delay_us(140);

    NRF_LOG_DEBUG("Initializing the clock.");
    err_code = nrf_drv_clock_init();
    APP_ERROR_CHECK(err_code);

    nrf_drv_clock_lfclk_request(NULL);

    // Wait for the clock to be ready.
    while (!nrf_clock_lf_is_running()) {;}
}


/**@brief Function for initializing the timer module. */
static void timers_init(void)
{
    ret_code_t err_code;

    err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);

    // Start a single shot timer that will reset the DFU on timeout.
    err_code = app_timer_create(&nrf_dfu_inactivity_timeout_timer_id,
                                APP_TIMER_MODE_SINGLE_SHOT,
                                nrf_dfu_reset_timeout_handler);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the event scheduler. */
static void scheduler_init(void)
{
    APP_SCHED_INIT(APP_TIMER_SCHED_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);
}


/**@brief Event handling loop.
 * @note This function does not return.
 */
static void wait_for_event()
{
    // Keep alive timer will wake CPU so it could refresh the watchdog counter.
    sr3_bootloader_keep_alive_timer_init();

    while (true)
    {
        // Feed watchdog to avoid a sudden reboot.
        sr3_bootloader_watchdog_feed();

        app_sched_execute();
        if (!NRF_LOG_PROCESS())
        {
        #ifdef BLE_STACK_SUPPORT_REQD
            (void)sd_app_evt_wait();
        #else
            __WFE();
        #endif
        }
    }
}


uint32_t nrf_dfu_init()
{
    bool enter_bootloader_mode = false;
    uint32_t ret_val;

    NRF_LOG_DEBUG("In real %s", __func__);

    nrf_dfu_settings_init();

#if ( defined(NRF_DFU_DUAL_BANK_SUPPORT) && (NRF_DFU_DUAL_BANK_SUPPORT == 1)) || \
    (!defined(NRF_DFU_UPDATABLE_APPLICATION_ONLY) || (NRF_DFU_UPDATABLE_APPLICATION_ONLY == 0))
    // Continue ongoing DFU operations.
    // Note that this part does not rely on SoftDevice interaction.
    ret_val = nrf_dfu_continue(&enter_bootloader_mode);
    if (ret_val != NRF_SUCCESS)
    {
        NRF_LOG_DEBUG("Could not continue DFU operation: 0x%08x", ret_val);
        enter_bootloader_mode = true;
    }
#endif

    // Check if there is a reason to enter DFU mode
    // besides the effect of the continuation.
    if (nrf_dfu_enter_check())
    {
        NRF_LOG_DEBUG("Application sent bootloader request");
        enter_bootloader_mode = true;
    }

    // Skip CRC check if system is resuming from power down mode.
    bool is_resume = ((NRF_POWER->RESETREAS & POWER_RESETREAS_OFF_Msk) != 0);

    bool app_is_valid = nrf_dfu_app_is_valid(is_resume);

#if defined(NRF_DFU_BLE_REQUIRES_BONDS) && (NRF_DFU_BLE_REQUIRES_BONDS == 1)
    if ((enter_bootloader_mode != false) &&
        (app_is_valid != false) &&
        (nrf_dfu_settings_shared_data_is_valid() == false))
    {
        // Do not enter the DFU if bonds are corrupted and application is valid.
        // Go back to the application to allow the user to fix bond information.
        enter_bootloader_mode = false;
    }
#endif

    if ((enter_bootloader_mode != false) || (app_is_valid == false))
    {
        // Initialize the clock and timers.
        clock_init();
        timers_init();

        sr3_bootloader_board_init();

        scheduler_init();

        // Start the inactivity timer.
        ret_val = app_timer_start(nrf_dfu_inactivity_timeout_timer_id,
                                  APP_TIMER_TICKS(NRF_DFU_INACTIVITY_TIMEOUT_MS),
                                  NULL);

        if (ret_val != NRF_SUCCESS)
        {
            NRF_LOG_ERROR("Could not initialize inactivity timer: 0x%08x", ret_val);
            return ret_val;
        }

        // Initializing transports
        ret_val = nrf_dfu_transports_init(false);
        if (ret_val != NRF_SUCCESS)
        {
            NRF_LOG_ERROR("Could not initialize DFU transport: 0x%08x", ret_val);
            return ret_val;
        }

        (void)nrf_dfu_req_handler_init();

        // Transport has been activated. Mark this fact in DFU settings page.
        s_dfu_settings.transport_activated = 1;
        ret_val = nrf_dfu_settings_write();
        if (ret_val != NRF_SUCCESS)
        {
            NRF_LOG_ERROR("Could not save transport state: 0x%08x", ret_val);
            return ret_val;
        }

        // This function will never return.
        NRF_LOG_DEBUG("Waiting for events");
        wait_for_event();

        // Should never get here.
        APP_ERROR_CHECK_BOOL(false);
    }

    // Erase shared data.
    ret_val = nrf_dfu_settings_shared_data_erase();
    if (ret_val != NRF_SUCCESS)
    {
        return ret_val;
    }

    NRF_LOG_DEBUG("Jumping to: 0x%08x", MAIN_APPLICATION_START_ADDR);
    nrf_bootloader_app_start(MAIN_APPLICATION_START_ADDR);

    // Should not be reached!
    NRF_LOG_INFO("After real %s", __func__);
    return NRF_SUCCESS;
}

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
 * @brief Smart Remote 3 bootloader.
 */
#include "nrf_bootloader.h"
#include "nrf_bootloader_app_start.h"
#include "nrf_bootloader_info.h"
#include "nrf_delay.h"
#include "nrf_log_backend_rtt.h"
#include "nrf_log_ctrl.h"
#include "nrf_wdt.h"

#include "app_timer.h"
#include "drv_board.h"

#include "sr3_bootloader.h"

#define NRF_LOG_MODULE_NAME sr3_bootloader
#define NRF_LOG_LEVEL NRF_LOG_DEFAULT_LEVEL
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();


/**@brief The interval at which inactivity timer is being triggered.
 * @note Must be often enough to make sure watchdog is feed properly timely.
 */
#define KEEP_ALIVE_TIMER_INTERVAL 500 /* ms */

/**@brief Keep alive timer. Periodically wakes up CPU to keep watchdog fed. */
APP_TIMER_DEF(sr3_bootloader_keep_alive_timer);

#if (NRF_LOG_ENABLED && NRF_LOG_BACKEND_RTT_ENABLED)
/**@brief Logger over RTT instance. */
NRF_LOG_BACKEND_RTT_DEF(rtt_log_backend);

/**@brief Initialized logging. */
static void sr3_bootloader_init_log(void)
{
    // Initialize logging subsystem.
    APP_ERROR_CHECK(NRF_LOG_INIT(NULL));

    // Start RTT console if debugger is connected. RTT console may be one of the logger backends
    // so it needs to be attached to logger as soon as possible.
    if (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk)
    {
        int32_t rtt_backend_id = -1;
        nrf_log_backend_rtt_init();
        rtt_backend_id = nrf_log_backend_add(&rtt_log_backend.backend, (nrf_log_severity_t)NRF_LOG_DEFAULT_LEVEL);
        ASSERT(rtt_backend_id >= 0);
        nrf_log_backend_enable(&rtt_log_backend.backend);
    }
}
#endif /* (NRF_LOG_ENABLED && NRF_LOG_BACKEND_RTT_ENABLED) */

/**@brief Handler function of keep alive timer. */
static void sr3_bootloader_keep_alive_handler(void * p_context)
{
    UNUSED_VARIABLE(p_context);
    /* We don't need to do anything. CPU is already woken up. */
}

void sr3_bootloader_keep_alive_timer_init(void)
{
    if (nrf_wdt_started())
    {
        ret_code_t err_code = app_timer_create(&sr3_bootloader_keep_alive_timer,
                                               APP_TIMER_MODE_REPEATED,
                                               sr3_bootloader_keep_alive_handler);
        APP_ERROR_CHECK(err_code);
        err_code = app_timer_start(sr3_bootloader_keep_alive_timer,
                                   APP_TIMER_TICKS(KEEP_ALIVE_TIMER_INTERVAL),
                                   NULL);
        APP_ERROR_CHECK(err_code);
    }
}

void sr3_bootloader_watchdog_feed(void)
{
    STATIC_ASSERT(NRF_WDT_CHANNEL_NUMBER == NRF_WDT_RR7 + 1);

    if (nrf_wdt_started())
    {
        for (nrf_wdt_rr_register_t i = NRF_WDT_RR0; i < NRF_WDT_RR7; i++)
        {
            if (nrf_wdt_reload_request_is_enabled(i))
            {
                nrf_wdt_reload_request_set(i);
            }
        }
    }
}

void sr3_bootloader_board_init(void)
{
    drv_board_init();
}

void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info)
{
#if NRF_LOG_ENABLED
    NRF_LOG_ERROR("FAULT: 0x%08x (PC: 0x%08x)", id, pc);
    NRF_LOG_FLUSH();
    nrf_delay_ms(100);
#endif
    NVIC_SystemReset();
}

void app_error_handler_bare(uint32_t error_code)
{
#if NRF_LOG_ENABLED
    NRF_LOG_ERROR("ERROR: 0x%08x", error_code);
    NRF_LOG_FLUSH();
    nrf_delay_ms(100);
#endif
    NVIC_SystemReset();
}

/**@brief Bootloader main function. */
int main(void)
{
    // Feed watchdog to assure we won't reset just after reboot.
    sr3_bootloader_watchdog_feed();

#if (NRF_LOG_ENABLED && NRF_LOG_BACKEND_RTT_ENABLED)
    // Start logging if enabled.
    sr3_bootloader_init_log();
#endif

    // Launch DFU Bootloader.
    nrf_bootloader_init();

    // If everything failed, assert error.
    APP_ERROR_CHECK_BOOL(false);
}


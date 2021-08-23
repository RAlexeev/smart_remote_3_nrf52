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

/*
 * @brief Smart Remote 3 application main file.
 */
#include "nrf.h"
#include "nrf_cli.h"
#include "nrf_cli_rtt.h"
#include "nrf_cli_ble_uart.h"
#include "nrf_crypto.h"
#include "nrf_delay.h"
#include "nrf_drv_clock.h"
#include "nrf_drv_rng.h"
#include "nrf_drv_wdt.h"
#include "nrf_log_backend_rtt.h"
#include "nrf_log_backend_uart.h"
#include "nrf_log_ctrl.h"
#include "nrf_mbr.h"
#include "nrf_mpu.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_stack_guard.h"

#if CONFIG_CL_UART_ENABLED
#include "nrf_cli_uart.h"
#endif

#include "app_debug.h"
#include "app_error.h"
#include "app_gpiote.h"
#include "app_scheduler.h"
#include "app_timer.h"
#include "task_manager.h"

#include "m_init.h"

#include "resources.h"
#include "rng_monitor.h"
#include "stack_profiler.h"

#include "nrf_log.h"

/**@brief Foreground scheduler queue size.
 *
 * Maximum observed value:              5
 * Safety Multiplier:                   1.5 (as SoftDevice might block application execution for a while)
 *
 * RESULT (rounded up):                 8
 */
#define APP_ISCHED_QUEUE_SIZE_FG        8

/**@brief Background scheduler queue size.
 *
 * Background scheduler is currently used for audio compression, audio gauges and generating LESC keys.
 *
 * Maximum observed value:              2 (3 with audio gauges enabled)
 * Safety Multiplier:                   1.5 (as SoftDevice might block application execution for a while)
 *
 * RESULT (rounded up):                 3 (5 with audio gauges enabled)
 */
#define APP_ISCHED_QUEUE_SIZE_BG        (3 + ((CONFIG_AUDIO_GAUGES_ENABLED) ? 2 : 0))

/**@brief SDK app_scheduler emmulation event pool size */
#define APP_SCHED_EVENT_POOL_SIZE       APP_ISCHED_QUEUE_SIZE_FG

/**@brief SDK app_scheduler emmulatiom maximum event data size */
#define APP_SCHED_MAX_EVENT_DATA_SIZE   (3 * sizeof(uint32_t))

/**@brief Number of modules using app_gpiote.
 *
 * drv_acc_lis3dh:                      1
 * drv_keyboard_matrix:                 1
 * ------------------------------------
 * TOTAL:                               2
 */
#define APP_GPIOTE_MAX_USERS            2

// Verify SDK configuration.
STATIC_ASSERT(APP_TIMER_ENABLED);
STATIC_ASSERT(CLOCK_ENABLED);
STATIC_ASSERT(GPIOTE_ENABLED);


#if CONFIG_CL_RTT_ENABLED
#if CONFIG_CL_RTT_INTERFACE == CONFIG_CL_INTERFACE_LOGGER
/**@brief Logger over RTT instance. */
NRF_LOG_BACKEND_RTT_DEF(m_rtt_log_backend);

#elif CONFIG_CL_RTT_INTERFACE == CONFIG_CL_INTERFACE_CLI
/**@brief Console over RTT instance. */
NRF_CLI_RTT_DEF(m_cli_rtt);
NRF_CLI_DEF(s_rtt_cli, "SR3-RTT> ", &m_cli_rtt.transport, '\n', CONFIG_LOG_RTT_QSIZE);
#else
#error "Unsupported RTT mode, please configure RTT as CLI or Logger backend"
#endif
#endif

#if CONFIG_CL_UART_ENABLED
#if CONFIG_CL_UART_INTERFACE == CONFIG_CL_INTERFACE_LOGGER
/**@brief Logger over UART instance. */
NRF_LOG_BACKEND_UART_DEF(m_uart_log_backend);

#elif CONFIG_CL_UART_INTERFACE == CONFIG_CL_INTERFACE_CLI
/**@brief Console over UART instance. */
NRF_CLI_UART_DEF(m_cli_uart, UART0_INSTANCE_INDEX, CONFIG_CLI_UART_TX_BUFFER_SIZE, CONFIG_CLI_UART_RX_BUFFER_SIZE);
NRF_CLI_DEF(s_uart_cli, "SR3-UART> ", &m_cli_uart.transport, '\r', CONFIG_LOG_UART_QSIZE);

#else
#error "Unsupported UART mode, please configure UART as CLI or Logger backend"
#endif
#endif

/**@brief Foregroud Scheduler. */
app_isched_t g_fg_scheduler;

/**@brief Backgroud Scheduler. */
app_isched_t g_bg_scheduler;

/**@brief SWI1 IRQ Handler used to execute foregroud scheduler tasks. */
void SWI1_EGU1_IRQHandler(void)
{
    app_isched_events_execute(&g_fg_scheduler);
}

/**@brief Foreground scheduler post-put hook */
static void fg_scheduler_post_put_hook(const app_isched_t *p_isched,
                                       const app_isched_event_t *p_evt,
                                       void *p_hook_context)
{
    NVIC_SetPendingIRQ(SWI1_IRQn);
}

/**@brief SWI3 IRQ Handler used to execute background scheduler tasks. */
void SWI3_EGU3_IRQHandler(void)
{
    app_isched_events_execute(&g_bg_scheduler);
}

/**@brief Background scheduler post-put hook */
static void bg_scheduler_post_put_hook(const app_isched_t *p_isched,
                                       const app_isched_event_t *p_evt,
                                       void *p_hook_context)
{
    NVIC_SetPendingIRQ(SWI3_IRQn);
}

/**@brief Enable foreground scheduler tasks execution */
static void schedulers_enable(void)
{
    NVIC_EnableIRQ(SWI1_IRQn);  // Enable foreground tasks execution.
    NVIC_EnableIRQ(SWI3_IRQn);  // Enable background tasks execution.
}

/**@brief Disable foreground scheduler tasks execution */
static void schedulers_disable(void)
{
    NVIC_DisableIRQ(SWI3_IRQn);  // Disable background tasks execution.
    NVIC_DisableIRQ(SWI1_IRQn);  // Disable foreground tasks execution.
}

#if CONFIG_WATCHDOG_ENABLED
static nrf_drv_wdt_channel_id s_watchdog_channel;

static void watchdog_event_handler(void)
{
    NVIC_SystemReset();
}
#endif

#if CONFIG_JLINK_MONITOR_ENABLED
STATIC_ASSERT(APP_TIMER_KEEPS_RTC_ACTIVE);

void JLINK_MONITOR_OnEnter(void)
{
    app_timer_pause();
}

void JLINK_MONITOR_OnExit(void)
{
    app_timer_resume();
}

void JLINK_MONITOR_OnPoll(void)
{
#if CONFIG_WATCHDOG_ENABLED
    nrf_drv_wdt_channel_feed(s_watchdog_channel);
#endif
}
#endif


/** @brief Idle task
 */
static void idle_task(void *p_context)
{
    // Enter main loop.
    for (;;)
    {
        /* Handle background and debug events one by one, go to sleep when done. */
        do
        {
#if CONFIG_WATCHDOG_ENABLED
            nrf_drv_wdt_channel_feed(s_watchdog_channel);
#endif
#if TASK_MANAGER_ENABLED
            task_yield();
#endif
        }
        while (NRF_LOG_PROCESS());

#if CONFIG_PWR_MGMT_ENABLED
        nrf_pwr_mgmt_run();
#endif
    }
}

/**@brief Application main function.
 */
int main(void)
{
    bool is_resume;

    // Enable traps in order to catch faulty code.
    SCB->CCR    |= SCB_CCR_DIV_0_TRP_Msk /* | SCB_CCR_UNALIGN_TRP_Msk */;

#if defined(FLOAT_ABI_HARD)
    // Enable automatic FPU state preservation and lazy stacking.
    FPU->FPCCR  |= FPU_FPCCR_ASPEN_Msk | FPU_FPCCR_LSPEN_Msk;
#endif

#if defined(NRF52832_XXAA)
    // Enable I-Cache.
    NRF_NVMC->ICACHECNF |= NVMC_ICACHECNF_CACHEEN_Msk;
#endif

    // Apply register changes listed above.
    __DSB();

#if CONFIG_WATCHDOG_ENABLED
    STATIC_ASSERT(WDT_ENABLED);
    {
        nrf_drv_wdt_config_t wdt_config = NRF_DRV_WDT_DEAFULT_CONFIG;

        wdt_config.reload_value = 1000u * CONFIG_WATCHDOG_TIMEOUT;

        APP_ERROR_CHECK(nrf_drv_wdt_init(&wdt_config, watchdog_event_handler));
        APP_ERROR_CHECK(nrf_drv_wdt_channel_alloc(&s_watchdog_channel));
        nrf_drv_wdt_enable();
    }
#endif

    // Initialize basic debugging subsystems.
    debug_pin_init();
#if CONFIG_JLINK_MONITOR_ENABLED
    NVIC_SetPriority(DebugMonitor_IRQn, _PRIO_SD_LOW);
#endif

    // Delay clock initialization (see FTPAN-132)
    nrf_delay_us(140);

    // Initialize Clock Management Driver.
    APP_ERROR_CHECK(nrf_drv_clock_init());

    // Start LFCLK.
    nrf_drv_clock_lfclk_request(NULL);

#if CONFIG_STACK_PROFILER_ENABLED
    // Initialize stack profiler.
    stack_profiler_init();
#endif

    // Initialize schedulers.
    APP_ISCHED_INIT_NAMED(fg_scheduler, &g_fg_scheduler, APP_ISCHED_QUEUE_SIZE_FG);
    APP_ISCHED_INIT_NAMED(bg_scheduler, &g_bg_scheduler, APP_ISCHED_QUEUE_SIZE_BG);

    // Use app_isched hooks to execute tasks at predefined priorities.
    APP_ERROR_CHECK(app_isched_install_hook(&g_fg_scheduler, APP_ISCHED_HOOK_POST_PUT,
                                            fg_scheduler_post_put_hook, NULL));
    APP_ERROR_CHECK(app_isched_install_hook(&g_bg_scheduler, APP_ISCHED_HOOK_POST_PUT,
                                            bg_scheduler_post_put_hook, NULL));

    // Use foreground scheduler to execute events enqueued to SDK scheduler.
    APP_SCHED_INIT(&g_fg_scheduler, APP_SCHED_MAX_EVENT_DATA_SIZE, APP_SCHED_EVENT_POOL_SIZE);

    // Initialize system timer.
    app_timer_init();

#if CONFIG_LOG_ENABLED
    // Initialize SDK Logger.
    APP_ERROR_CHECK(NRF_LOG_INIT(app_timer_cnt_get));
#endif

#if CONFIG_CL_RTT_ENABLED
    /*
     * Start RTT console if debugger is connected. RTT console may be one of the logger backends
     * so it needs to be attached to logger as soon as possible.
     */
    if (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk)
    {
#if CONFIG_CL_RTT_INTERFACE == CONFIG_CL_INTERFACE_LOGGER
        nrf_log_backend_rtt_init();
        int32_t rtt_backend_id = nrf_log_backend_add(&m_rtt_log_backend.backend,
                                                     (nrf_log_severity_t)CONFIG_LOG_RTT_SEVERITY);
        if (rtt_backend_id < 0)
        {
            ASSERT(false);
        }
        nrf_log_backend_enable(&m_rtt_log_backend.backend);
#endif

#if CONFIG_CL_RTT_INTERFACE == CONFIG_CL_INTERFACE_CLI
        APP_ERROR_CHECK(nrf_cli_init(&s_rtt_cli,
                                     NULL,
                                     true,
                                     CONFIG_CLI_RTT_LOG_BACKEND_ENABLED,
                                     (nrf_log_severity_t)(CONFIG_LOG_RTT_SEVERITY)));
#endif
    }
#endif

#if CONFIG_CL_UART_ENABLED
#if CONFIG_CL_UART_INTERFACE == CONFIG_CL_INTERFACE_LOGGER
    nrf_log_backend_uart_init();
    int32_t uart_backend_id = nrf_log_backend_add(&m_uart_log_backend.backend,
                                                  (nrf_log_severity_t)CONFIG_LOG_UART_SEVERITY);
    if (uart_backend_id < 0)
    {
        ASSERT(false);
    }
    nrf_log_backend_enable(&m_uart_log_backend.backend);

#elif CONFIG_CL_UART_INTERFACE == CONFIG_CL_INTERFACE_CLI
    nrf_drv_uart_config_t m_uart_config = NRF_DRV_UART_DEFAULT_CONFIG;
    m_uart_config.pseltxd = CONFIG_UART_TX_PIN;
    m_uart_config.pselrxd = CONFIG_UART_RX_PIN;
    m_uart_config.hwfc    = (nrf_uart_hwfc_t)CONFIG_CL_UART_HWFC;

    APP_ERROR_CHECK(nrf_cli_init(&s_uart_cli,
                                 &m_uart_config,
                                 true,
                                 CONFIG_CLI_UART_LOG_BACKEND_ENABLED,
                                 (nrf_log_severity_t)(CONFIG_LOG_UART_SEVERITY)));
#endif
#endif

    NRF_LOG_INFO("System ON");
    NRF_LOG_INFO("Board: %s", (uint32_t)CONFIG_BOARD);

    // Check if we are resuming operation after sleeping in System OFF state.
    is_resume = ((NRF_POWER->RESETREAS & POWER_RESETREAS_OFF_Msk) != 0);
    NRF_LOG_INFO("Boot Mode: %s", (uint32_t)((is_resume) ? "Resume" : "Power On"));

    // Clear RESETREAS register.
    NRF_POWER->RESETREAS = NRF_POWER->RESETREAS;

    // Initialize MPU driver.
    APP_ERROR_CHECK(nrf_mpu_init());

    // Create Guard Pages.
    APP_ERROR_CHECK(NRF_STACK_GUARD_INIT());

    // Initialize crypto library
#if CONFIG_SEC_LESC_PAIRING
    APP_ERROR_CHECK(nrf_crypto_init());
#elif CONFIG_NFC_ENABLED
    APP_ERROR_CHECK(nrf_drv_rng_init(NULL));
#endif

#if CONFIG_RNG_MONITOR_ENABLED
    // Initialize Random Number Generator monitor.
    APP_ERROR_CHECK(rng_monitor_init());
#endif

#if CONFIG_PWR_MGMT_ENABLED
    APP_ERROR_CHECK(nrf_pwr_mgmt_init());
#endif

    // Initialize GPIOTE manager.
    APP_GPIOTE_INIT(APP_GPIOTE_MAX_USERS);

    // Disable schedulers.
    schedulers_disable();

    // Initialize Firmware Modules.
    m_init(is_resume);

    // Fix interrupt priorities altered by SoftDevice.
    NVIC_SetPriority(SD_EVT_IRQn, 5);
    NVIC_SetPriority(SWI1_IRQn, APP_IRQ_PRIORITY_LOW);
    NVIC_SetPriority(SWI3_IRQn, APP_IRQ_PRIORITY_LOWEST);

    // Enable schedulers.
    schedulers_enable();

#if CONFIG_CLI_RTT_ENABLED
    // Create RTT console task if debugger is connected.
    if (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk)
    {
        APP_ERROR_CHECK(nrf_cli_task_create(&s_rtt_cli));
    }
#endif

#if CONFIG_CLI_UART_ENABLED
    APP_ERROR_CHECK(nrf_cli_task_create(&s_uart_cli));
#endif

#if TASK_MANAGER_ENABLED
    // Start task manager.
    task_manager_start(idle_task, NULL);
#else
    idle_task((void *)0);
#endif

    // Should never reach that point.
    APP_ERROR_CHECK_BOOL(false);
}

#if CONFIG_PWR_MGMT_ENABLED
/**@brief Log usage stattistics of most important firmware components. */
static bool sr3_core_log_statistics(nrf_pwr_mgmt_evt_t event)
{
    NRF_LOG_INFO("Maximum app_timer operations queue usage: %u entries",
                 app_timer_op_queue_utilization_get());

    NRF_LOG_INFO("Maximum app_scheduler event pool usage: %u entries",
                 app_sched_get_max_utilization());

    NRF_LOG_INFO("Maximum foreground scheduler queue usage: %u entries",
                 app_isched_get_max_utilization(&g_fg_scheduler));

    NRF_LOG_INFO("Maximum background scheduler queue usage: %u entries",
                 app_isched_get_max_utilization(&g_bg_scheduler));

#if CONFIG_STACK_PROFILER_ENABLED
    NRF_LOG_INFO("Maximum stack usage: %u%% (%u out of %u bytes)",
              100ul * stack_profiler_get_max_stack_usage() / REAL_STACK_SIZE,
              stack_profiler_get_max_stack_usage(),
              REAL_STACK_SIZE);
#endif

    return true;
}

NRF_PWR_MGMT_HANDLER_REGISTER(sr3_core_log_statistics, SHUTDOWN_PRIORITY_STATISTICS);
#endif /* CONFIG_PWR_MGMT_ENABLED */

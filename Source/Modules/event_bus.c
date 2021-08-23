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

#include <stdarg.h>

#include "nrf_balloc.h"
#include "nrf_pwr_mgmt.h"
#include "app_debug.h"
#include "app_timer.h"

#include "event_bus.h"
#include "resources.h"
#include "sr3_config.h"

#define NRF_LOG_MODULE_NAME event_bus
#define NRF_LOG_LEVEL       CONFIG_EVENT_BUS_LOG_LEVEL
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

static const event_handler_t   *sp_event_handlers;  /**< Pointer to the event handlers table. */

// Define event allocator.
NRF_BALLOC_DEF(s_event_pool, sizeof(event_t), CONFIG_EVENT_POOL_SIZE);

#if CONFIG_EVENT_MONITOR_ENABLED
/**@brief Translate event type to string. */
static const char *event_type_str(const event_t *p_event)
{
    ASSERT(p_event != NULL);

    switch (p_event->type)
    {
#define EVT2STR(_event_type)  case _event_type: return #_event_type

        EVT2STR(EVT_SYSTEM_BATTERY_LEVEL);
        EVT2STR(EVT_SYSTEM_GYRO_STATE);

        EVT2STR(EVT_KEY_UP);
        EVT2STR(EVT_KEY_DOWN);
        EVT2STR(EVT_KEY_HELD);

        EVT2STR(EVT_REL_X);
        EVT2STR(EVT_REL_Y);
        EVT2STR(EVT_REL_WHEEL);
        EVT2STR(EVT_REL_PAN);

        EVT2STR(EVT_HID_REPORT_INPUT);
        EVT2STR(EVT_HID_REPORT_OUTPUT);
        EVT2STR(EVT_HID_REPORT_FEATURE);

        EVT2STR(EVT_IR_SYMBOL);

        EVT2STR(EVT_BT_CONN_STATE);
        EVT2STR(EVT_BT_ADV_STATE);
        EVT2STR(EVT_BT_ALERT_LEVEL);
        EVT2STR(EVT_ATVV_STATE);

#undef EVT2STR
        default:
            return "UNKNOWN";
    }
}

/**@brief Dump event in human-readable format. */
static void event_dump(const event_t *p_event)
{
    uint32_t hold_time;

    switch (EVENT_GROUP(p_event->type))
    {
        case EVT_GROUP_SYSTEM:
            NRF_LOG_INFO("< %s: %u >", (uint32_t)event_type_str(p_event), p_event->system.data);
            break;

        case EVT_GROUP_KEY:
            hold_time = app_timer_cnt_diff_compute(app_timer_cnt_get(), p_event->key.down_timestamp);
            NRF_LOG_INFO("< %s: 0x%04X %u (%ums ago) >",
                      (uint32_t)event_type_str(p_event),
                      p_event->key.id,
                      p_event->key.down_timestamp,
                      ROUNDED_DIV((uint64_t)(hold_time) * ((APP_TIMER_PRESCALER) + 1) * 1000, (uint64_t)APP_TIMER_CLOCK_FREQ));
            break;

        case EVT_GROUP_REL:
            NRF_LOG_INFO("< %s: %d >", (uint32_t)event_type_str(p_event), p_event->rel.shift);
            break;

        case EVT_GROUP_HID:
            NRF_LOG_INFO("< %s: 0x%04X:0x%04X %d >", (uint32_t)event_type_str(p_event),
                         HID_USAGE_PAGE(p_event->hid.usage), HID_USAGE_ID(p_event->hid.usage),
                         p_event->hid.report);
            break;

        case EVT_GROUP_IR:
            NRF_LOG_INFO("< %s: 0x%08X >", (uint32_t)event_type_str(p_event), (uint32_t)p_event->ir.p_symbol);
            break;

        case EVT_GROUP_BT:
            NRF_LOG_INFO("< %s: %u 0x%04X 0x%04X >",
                      (uint32_t)event_type_str(p_event),
                      p_event->bt.data,
                      p_event->bt.conn_id,
                      p_event->bt.peer_id);
            break;

        case EVT_GROUP_ATVV:
            NRF_LOG_INFO("< %s: 0x%02X %u 0x%04X 0x%04X >",
                      (uint32_t)event_type_str(p_event),
                      p_event->atvv.type,
                      p_event->atvv.rate_khz,
                      p_event->atvv.conn_id,
                      p_event->atvv.peer_id);
            break;

        default:
            NRF_LOG_INFO("< UNKNOWN EVENT: 0x%02X >", p_event->type);
            break;
    }
}
#endif /* CONFIG_EVENT_MONITOR_ENABLED */

/**@brief Process the event. */
static void event_process(void *p_context)
{
    const event_handler_t *p_handler = sp_event_handlers;
    event_t *p_event = p_context;

    ASSERT((p_event != NULL) && (p_handler != NULL));

#if CONFIG_EVENT_MONITOR_ENABLED
    if (CONFIG_EVENT_MONITOR_TYPES & (1ul << EVENT_GROUP(p_event->type)))
    {
        event_dump(p_event);
    }
#endif

    // Pass the event to handlers.
    while (*p_handler != NULL)
    {
        if ((*p_handler++)(p_event))
        {
            // Stop event processing if handler returned true.
#if CONFIG_EVENT_MONITOR_ENABLED
            if (CONFIG_EVENT_MONITOR_TYPES & (1ul << EVENT_GROUP(p_event->type)))
            {
                NRF_LOG_INFO("< ^^^^ EVENT CONSUMED ^^^^ >");
            }
#endif
            break;
        }
    }

    nrf_balloc_free(&s_event_pool, p_event);
}

ret_code_t event_send(event_type_t event_type, ...)
{
    ret_code_t status;
    event_t *p_event;
    va_list args;

    p_event = nrf_balloc_alloc(&s_event_pool);
    if (p_event == NULL)
    {
        NRF_LOG_WARNING("%s(): WARNING: Allocation error: Event lost!", (uint32_t)__func__);
        APP_ERROR_CHECK_BOOL(CONFIG_EVENT_FORCE_ERROR_CHECKING);
        return NRF_ERROR_NO_MEM;
    }

    va_start(args, event_type);

    p_event->type = event_type;

    switch (EVENT_GROUP(p_event->type))
    {
        case EVT_GROUP_SYSTEM:
            p_event->system.data = va_arg(args, uint32_t);
            break;

        case EVT_GROUP_KEY:
            p_event->key.id = va_arg(args, uint32_t);
            p_event->key.down_timestamp = va_arg(args, uint32_t);
            break;

        case EVT_GROUP_REL:
            p_event->rel.shift = va_arg(args, uint32_t);
            break;

        case EVT_GROUP_HID:
            p_event->hid.usage = va_arg(args, uint32_t);
            p_event->hid.report = (int16_t)va_arg(args, int);
            break;

        case EVT_GROUP_IR:
            p_event->ir.p_symbol = va_arg(args, void *);
            break;

        case EVT_GROUP_BT:
            p_event->bt.data = va_arg(args, uint32_t);
            p_event->bt.conn_id = va_arg(args, uint32_t);
            p_event->bt.peer_id = va_arg(args, uint32_t);
            break;

        case EVT_GROUP_ATVV:
            p_event->atvv.type     = va_arg(args, uint32_t);
            p_event->atvv.rate_khz = va_arg(args, uint32_t);
            p_event->atvv.conn_id  = va_arg(args, uint32_t);
            p_event->atvv.peer_id  = va_arg(args, uint32_t);
            break;

        default:
            APP_ERROR_CHECK_BOOL(false);
            break;
    }

    va_end(args);

    status = app_isched_event_put(&g_fg_scheduler, event_process, p_event);
    if (status != NRF_SUCCESS)
    {
        nrf_balloc_free(&s_event_pool, p_event);

        NRF_LOG_WARNING("%s(): WARNING: Scheduling error: Event lost!", (uint32_t)__func__);
        APP_ERROR_CHECK_BOOL(CONFIG_EVENT_FORCE_ERROR_CHECKING);
    }

    return status;
}

ret_code_t event_bus_init(const event_handler_t *event_handlers_table)
{
    if (event_handlers_table == NULL)
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    sp_event_handlers = event_handlers_table;

    return nrf_balloc_init(&s_event_pool);
}

#if CONFIG_PWR_MGMT_ENABLED
static bool event_bus_log_statistics(nrf_pwr_mgmt_evt_t event)
{
    NRF_LOG_INFO("event_bus: Maximum Event Pool usage: %d entries",
            nrf_balloc_max_utilization_get(&s_event_pool));

    return true;
}

NRF_PWR_MGMT_HANDLER_REGISTER(event_bus_log_statistics, SHUTDOWN_PRIORITY_STATISTICS);
#endif /* CONFIG_PWR_MGMT_ENABLED */

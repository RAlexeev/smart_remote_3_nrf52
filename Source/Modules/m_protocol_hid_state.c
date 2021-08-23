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

/**@file
 *
 * @brief HID state module.
 */

#include <stdlib.h>

#include "nrf_assert.h"
#include "sdk_common.h"
#include "app_debug.h"
#include "app_isched.h"
#include "app_timer.h"
#include "event_bus.h"
#include "sr3_config.h"
#include "resources.h"

#include "m_protocol_hid_state.h"

#define NRF_LOG_MODULE_NAME m_protocol_hid_state
#define NRF_LOG_LEVEL CONFIG_PROTOCOL_HID_MODULE_LOG_LEVEL
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

/**@brief Maximum value of the RTC counter.
 *
 * @note Ensure it matches SoC specification.
 */
#define MAX_RTC_COUNTER_VAL 0x00FFFFFF

STATIC_ASSERT(UINT32_MAX - APP_TIMER_TICKS(CONFIG_HID_REPORT_EXPIRATION) > MAX_RTC_COUNTER_VAL);


/**@brief Module state. */
typedef enum
{
    M_PROTOCOL_HID_STATE_STATE_DISCONNECTED,   /**< Module is not connected. */
    M_PROTOCOL_HID_STATE_STATE_CONNECTED_IDLE, /**< Module is connected, no data exchange. */
    M_PROTOCOL_HID_STATE_STATE_CONNECTED_BUSY  /**< Module is connected, report is generated. */
} m_protocol_hid_state_state_t;

/**@brief Module structure. */
typedef struct
{
    void                         *token;
    m_protocol_hid_state_state_t  state;
    uint8_t                       item_count;
    uint8_t                       eventq_head;
    uint8_t                       eventq_tail;
    m_protocol_hid_state_item_t   item[CONFIG_PROTOCOL_HID_STATE_ITEM_COUNT];
    m_protocol_hid_state_item_t   eventq[CONFIG_PROTOCOL_HID_EVENT_QUEUE_SIZE];
    uint32_t                      eventq_timestamp[CONFIG_PROTOCOL_HID_EVENT_QUEUE_SIZE];
#if CONFIG_PWR_MGMT_ENABLED
    uint8_t                       item_count_max;
    uint8_t                       eventq_len_max;
#endif
} m_protocol_hid_state_t;

/**@brief Instance of HID state. */
static m_protocol_hid_state_t m_state;


static void m_protocol_hid_state_issue_report(uint32_t usage, int16_t report);


/**@brief Get index of the next element in the event queue. */
static uint8_t m_protocol_hid_state_eventq_next(uint8_t idx)
{
    size_t size = ARRAY_SIZE(m_state.eventq);
    return (idx + 1) % size;
}

/**@brief Get distance between two elements in the event queue. */
static unsigned int m_protocol_hid_state_eventq_diff(uint8_t idx1, uint8_t idx2)
{
    size_t size = ARRAY_SIZE(m_state.eventq);
    return (size + idx1 - idx2) % size;
}

/**@brief Get number of elements in the event queue. */
static unsigned int m_protocol_hid_state_eventq_len(void)
{
    return m_protocol_hid_state_eventq_diff(m_state.eventq_head, m_state.eventq_tail);
}

/**@brief Check if the event queue is full. */
static bool m_protocol_hid_state_eventq_full(void)
{
    return m_protocol_hid_state_eventq_next(m_state.eventq_head) == m_state.eventq_tail;
}

/**@brief Remove stale events from the event queue. */
static void m_protocol_hid_state_eventq_cleanup(uint32_t timestamp)
{
    /* Find timed out events. */
    uint8_t first_valid;
    for (first_valid = m_state.eventq_tail;
         first_valid != m_state.eventq_head;
         first_valid = m_protocol_hid_state_eventq_next(first_valid))
    {
        uint32_t diff = app_timer_cnt_diff_compute(timestamp, m_state.eventq_timestamp[first_valid]);
        if (diff < APP_TIMER_TICKS(CONFIG_HID_REPORT_EXPIRATION))
        {
            break;
        }
    }

    /* Remove events but only if key up was generated for each removed key down. */
    uint8_t maxfound_idx = m_state.eventq_tail;
    for (uint8_t i = m_state.eventq_tail; i != first_valid; i = m_protocol_hid_state_eventq_next(i))
    {
        if (m_state.eventq[i].value > 0)
        {
            /* Every key down must be paired with key up.
             * Set hit count to value as we just detected first key down for this usage. */
            unsigned int hit_count = m_state.eventq[i].value;

            uint8_t j;
            for (j  = m_protocol_hid_state_eventq_next(i);
                 j != first_valid;
                 j  = m_protocol_hid_state_eventq_next(j))
            {
                if (m_state.eventq[i].usage == m_state.eventq[j].usage)
                {
                    hit_count += m_state.eventq[j].value;
                    if (hit_count == 0)
                    {
                        /* When the hit count reaches zero, all events with this usage
                         * are paired. */
                        break;
                    }
                }
            }

            if (j == first_valid)
            {
                /* Pair not found. */
                break;
            }

            if (m_protocol_hid_state_eventq_diff(j, m_state.eventq_tail) >
                m_protocol_hid_state_eventq_diff(maxfound_idx, m_state.eventq_tail))
            {
                maxfound_idx = j;
            }
        }

        if (i == maxfound_idx)
        {
            /* All events up to this point have pairs and can be deleted. */
            NRF_LOG_WARNING("%s(): WARNING: %u stale events removed from the queue!",
                            __func__,
                            m_protocol_hid_state_eventq_diff(i, m_state.eventq_tail) + 1);
            m_state.eventq_tail = m_protocol_hid_state_eventq_next(i);
            maxfound_idx = m_state.eventq_tail;
        }
    }
}

/**@brief Compare two usage values. */
static int m_protocol_hid_state_usage_compare(const void *a, const void *b)
{
    const uint32_t * p_a = a;
    const uint32_t * p_b = b;

    return *p_a - *p_b;
}

/**@brief Compare two page values. */
static int m_protocol_hid_state_usage_page_compare(const void *a, const void *b)
{
    const uint32_t * p_a = a;
    const uint32_t * p_b = b;

    return HID_USAGE_PAGE(*p_a) - HID_USAGE_PAGE(*p_b);
}

/**@brief Update value linked with given usage. */
static bool m_protocol_hid_state_set_value(uint32_t usage, int16_t report, bool absolute)
{
    const uint8_t prev_item_count = m_state.item_count;
    bool update_needed = false;

    ASSERT(usage != 0);

    /* Report equal to zero brings no change. This should never happen. */
    ASSERT(report != 0);

    m_protocol_hid_state_item_t *p_item;
    p_item = bsearch(&usage,
                     m_state.item,
                     ARRAY_SIZE(m_state.item),
                     sizeof(m_state.item[0]),
                     m_protocol_hid_state_usage_compare);

    if (p_item != NULL)
    {
        /* Item is present in the array - update its value. */
        p_item->value += report;
        if (p_item->value == 0)
        {
            ASSERT(m_state.item_count != 0);
            m_state.item_count -= 1;
            p_item->usage = 0;
        }

        update_needed = true;
    }
    else if ((report < 0) && (absolute != false))
    {
        /* For items with absolute value, the value is used as a reference counter
         * and must not fall below zero. This could happen if a key up event is
         * lost and the state receives an unpaired key down event. */
    }
    else if (prev_item_count >= CONFIG_PROTOCOL_HID_STATE_ITEM_COUNT)
    {
        /* Configuration should allow the HID module to hold data about the maximum number
         * of simultaneously pressed keys. Generate a warning if an item cannot
         * be recorded. */
        NRF_LOG_WARNING("%s(): WARNING: no place on the list to store HID item!", __func__);
    }
    else
    {
        size_t const idx = ARRAY_SIZE(m_state.item) - prev_item_count - 1;

        ASSERT(m_state.item[idx].usage == 0);

        /* Record this value change. */
        m_state.item[idx].usage = usage;
        m_state.item[idx].value = report;
        m_state.item_count += 1;

#if CONFIG_PWR_MGMT_ENABLED
        /* Update statistics. */
        if (m_state.item_count > m_state.item_count_max)
        {
            m_state.item_count_max = m_state.item_count;
        }
#endif

        update_needed = true;
    }

    if (prev_item_count != m_state.item_count)
    {
        /* Sort elements on the list. Use simple algorithm with small footprint. */
        for (size_t k = 0; k < ARRAY_SIZE(m_state.item); k++)
        {
            size_t id = k;
            for (size_t l = k + 1; l < ARRAY_SIZE(m_state.item); l++)
            {
                if (m_state.item[l].usage < m_state.item[id].usage)
                {
                    id = l;
                }
            }
            if (id != k)
            {
                m_protocol_hid_state_item_t tmp = m_state.item[k];
                m_state.item[k] = m_state.item[id];
                m_state.item[id] = tmp;
            }
        }
    }

    return update_needed;
}

/**@brief Enqueue event that updates of given usage. */
static void m_protocol_hid_state_enqueue(uint32_t usage, int16_t report)
{
    m_protocol_hid_state_eventq_cleanup(app_timer_cnt_get());

    if (m_protocol_hid_state_eventq_full() != false)
    {
        if (m_state.state == M_PROTOCOL_HID_STATE_STATE_DISCONNECTED)
        {
            /* In disconnected state no items are recorded yet.
             * Try to remove queued items starting from the oldest one. */
            for (uint8_t i  = m_state.eventq_tail;
                         i != m_state.eventq_head;
                         i  = m_protocol_hid_state_eventq_next(i))
            {

                /* Initial cleanup was done above. Queue will not contain events
                 * with expired timestamp. */
                uint32_t timestamp = (m_state.eventq_timestamp[i] + APP_TIMER_TICKS(CONFIG_HID_REPORT_EXPIRATION)) &
                                     MAX_RTC_COUNTER_VAL;

                m_protocol_hid_state_eventq_cleanup(timestamp);
                if (m_protocol_hid_state_eventq_full() == false)
                {
                    /* At least one element was removed from the queue.
                     * Do not continue list traverse as content was modified! */
                    break;
                }
            }
        }

        if (m_protocol_hid_state_eventq_full() != false)
        {
            /* To maintain the sanity of HID state, clear all recorded events and items. */
            NRF_LOG_WARNING("%s(): WARNING: Queue is full, all events are dropped!", __func__);
            memset(m_state.item, 0, sizeof(m_state.item));
            m_state.item_count = 0;
            m_state.eventq_head = 0;
            m_state.eventq_tail = 0;
        }
    }

    /* Add a new event to the queue. */
    m_state.eventq[m_state.eventq_head].usage = usage;
    m_state.eventq[m_state.eventq_head].value = report;
    m_state.eventq_timestamp[m_state.eventq_head] = app_timer_cnt_get();
    m_state.eventq_head = m_protocol_hid_state_eventq_next(m_state.eventq_head);


#if CONFIG_PWR_MGMT_ENABLED
    uint8_t eventq_len = m_protocol_hid_state_eventq_len();
    if (eventq_len > m_state.eventq_len_max)
    {
        m_state.eventq_len_max = eventq_len;
    }
#endif
}

/**@brief Callback used when report was generated. */
static void m_protocol_hid_state_report_issued(void *p_context)
{
    if (m_state.token != p_context)
    {
        /* On disconnection event state and queue are reset and HID event collection
         * for new peer starts. Any in-flight callback that was issued before the 
         * disconnection happened is invalid. To detect this properly, a token value is
         * passed along with a callback and is compared to the one saved in the state structure.
         * New token is generated on disconnection event causing stale callbacks to be
         * ignored.
         */
        return;
    }

    ASSERT(m_state.state == M_PROTOCOL_HID_STATE_STATE_CONNECTED_BUSY);

    while (true)
    {
        if (m_protocol_hid_state_eventq_len() == 0)
        {
            /* Module is connected but there are no events to dequeue.
             * Switch to idle state. */
            m_state.state = M_PROTOCOL_HID_STATE_STATE_CONNECTED_IDLE;
            break;
        }

        /* There are enqueued events to handle. */
        m_protocol_hid_state_item_t *event = &m_state.eventq[m_state.eventq_tail];
        m_state.eventq_tail = m_protocol_hid_state_eventq_next(m_state.eventq_tail);
        if (m_protocol_hid_state_set_value(event->usage, event->value, true))
        {
            /* Some item was updated. Report must be issued. */
            m_protocol_hid_state_issue_report(event->usage, event->value);
            break;
        }

        /* No item was changed. Try next event. */
    }
}

/**@brief Request report generation. */
static void m_protocol_hid_state_issue_report(uint32_t usage, int16_t report)
{
    event_send(EVT_HID_REPORT_INPUT, usage, report);
    m_state.state = M_PROTOCOL_HID_STATE_STATE_CONNECTED_BUSY;

    /* As events are executed by g_fg_scheduler, next dequeue will be called
     * after a report is generated.
     */
    APP_ERROR_CHECK(app_isched_event_put(&g_fg_scheduler,
                                         m_protocol_hid_state_report_issued,
                                         m_state.token));
}


void m_protocol_hid_state_update(uint32_t usage, int16_t report, bool absolute)
{
    switch (m_state.state)
    {
        case M_PROTOCOL_HID_STATE_STATE_DISCONNECTED:
            if (absolute == false)
            {
                /* Do not store relative changes when disconnected. */
            }
            else
            {
                /* Report cannot be sent yet - enqueue this HID event. */
                m_protocol_hid_state_enqueue(usage, report);
            }
            break;

        case M_PROTOCOL_HID_STATE_STATE_CONNECTED_IDLE:
            /* Update state and issue report generation event. */
            if (m_protocol_hid_state_set_value(usage, report, absolute))
            {
                m_protocol_hid_state_issue_report(usage, report);
            }
            break;

        case M_PROTOCOL_HID_STATE_STATE_CONNECTED_BUSY:
            if (absolute == false)
            {
                /* Sequence does not matter - update value. */
                bool unused = m_protocol_hid_state_set_value(usage, report, absolute);
                UNUSED_VARIABLE(unused);
            }
            else
            {
                /* Sequence is important - enqueue this HID event. */
                m_protocol_hid_state_enqueue(usage, report);
            }
            break;

        default:
            ASSERT(false);
            break;
    }
}

ret_code_t m_protocol_hid_state_connect(void)
{
    ASSERT(m_state.state == M_PROTOCOL_HID_STATE_STATE_DISCONNECTED);

    if (m_protocol_hid_state_eventq_len() > 0)
    {
        /* Remove all stale events from the queue. */
        m_protocol_hid_state_eventq_cleanup(app_timer_cnt_get());
    }

    if (m_protocol_hid_state_eventq_len() == 0)
    {
        /* No events left on the queue - connect but stay idle. */
        m_state.state = M_PROTOCOL_HID_STATE_STATE_CONNECTED_IDLE;
    }
    else
    {
        /* There are some collected events, start event draining procedure. */
        m_state.state = M_PROTOCOL_HID_STATE_STATE_CONNECTED_BUSY;
        APP_ERROR_CHECK(app_isched_event_put(&g_fg_scheduler,
                                             m_protocol_hid_state_report_issued,
                                             m_state.token));
    }

    return NRF_SUCCESS;
}

ret_code_t m_protocol_hid_state_disconnect(void)
{
    /* Check if module is connected. Disconnect request can happen if Bluetooth connection
     * attempt failed. */
    if (m_state.state != M_PROTOCOL_HID_STATE_STATE_DISCONNECTED)
    {
        m_state.state = M_PROTOCOL_HID_STATE_STATE_DISCONNECTED;

        /* Clear state and queue. */
        memset(m_state.item, 0, sizeof(m_state.item));
        m_state.item_count = 0;
        m_state.eventq_head = 0;
        m_state.eventq_tail = 0;

        /* Disconnection starts a new state session. Queue is cleared and event collection
         * is started. When a connection happens, the same queue is used until all collected
         * events are drained. */
        m_state.token = (void *)app_timer_cnt_get();
    }

    return NRF_SUCCESS;
}

m_protocol_hid_state_item_t *m_protocol_hid_state_get(uint32_t usage)
{
    if (m_state.state == M_PROTOCOL_HID_STATE_STATE_DISCONNECTED)
    {
        return NULL;
    }

    return bsearch(&usage,
                   m_state.item,
                   ARRAY_SIZE(m_state.item),
                   sizeof(m_state.item[0]),
                   m_protocol_hid_state_usage_compare);
}

m_protocol_hid_state_item_t const *m_protocol_hid_state_page_it_init(uint16_t page)
{
    if (m_state.state == M_PROTOCOL_HID_STATE_STATE_DISCONNECTED)
    {
        return NULL;
    }

    uint32_t usage = HID_USAGE(page, 0);
    m_protocol_hid_state_item_t *p_item;
    p_item = bsearch(&usage,
                     m_state.item,
                     ARRAY_SIZE(m_state.item),
                     sizeof(m_state.item[0]),
                     m_protocol_hid_state_usage_page_compare);
    if (!p_item)
    {
        return NULL;
    }

    while (HID_USAGE_PAGE(p_item->usage) == page)
    {
        p_item -= 1;
        if (p_item < &m_state.item[0])
        {
            return &m_state.item[0];
        }
    }

    return p_item + 1;
}

m_protocol_hid_state_item_t const *m_protocol_hid_state_page_it_next(m_protocol_hid_state_item_t const *p_item)
{
    ASSERT(m_state.state != M_PROTOCOL_HID_STATE_STATE_DISCONNECTED);

    p_item += 1;
    if (p_item >= &m_state.item[CONFIG_PROTOCOL_HID_STATE_ITEM_COUNT])
    {
        return NULL;
    }

    return p_item;
}

ret_code_t m_protocol_hid_state_init(void)
{
    memset(&m_state, 0, sizeof(m_state));

    return NRF_SUCCESS;
}

#if CONFIG_PWR_MGMT_ENABLED
static bool m_protocol_hid_state_log_statistics(nrf_pwr_mgmt_evt_t event)
{
    NRF_LOG_INFO("Maximum HID State item count: %u items", m_state.item_count_max);
    NRF_LOG_INFO("Maximum HID State event queue length: %u items", m_state.eventq_len_max);

    return true;
}

NRF_PWR_MGMT_HANDLER_REGISTER(m_protocol_hid_state_log_statistics, SHUTDOWN_PRIORITY_STATISTICS);
#endif /* CONFIG_PWR_MGMT_ENABLED */

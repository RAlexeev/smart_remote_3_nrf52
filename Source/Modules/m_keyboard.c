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
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "nrf_assert.h"
#include "nrf_error.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_queue.h"
#include "app_debug.h"
#include "app_scheduler.h"
#include "app_timer.h"
#include "app_util.h"

#include "event_bus.h"
#include "drv_keyboard.h"
#include "m_keyboard.h"

#include "resources.h"
#include "sr3_config.h"

#if CONFIG_KBD_ENABLED

#define NRF_LOG_MODULE_NAME m_keyboard
#define NRF_LOG_LEVEL CONFIG_KBD_MODULE_LOG_LEVEL
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

/**@brief Keyboard held event interval.*/
#define KBD_HELD_INTERVAL_TICKS APP_TIMER_TICKS(CONFIG_KBD_HELD_EVENT_INTERVAL_MS)

typedef struct
{
    uint8_t     key_id;
    uint32_t    down_timestamp;
} keyboard_state_t;

static keyboard_state_t m_keyboard_state[DRV_KEYBOARD_MAX_KEYS];          //!< Array holding the keys that have already been transmitted.
static uint8_t          m_keyboard_state_len;                             //!< Number of keys in m_keyboard_state.
static bool             m_keyboard_enabled;                               //!< Keyboard module state.

#if (CONFIG_KBD_HELD_EVENT_INTERVAL_MS != 0)
typedef struct
{
    uint8_t key_id;
    uint8_t timer_idx;
} key_timer_rel_t;

typedef union
{
    key_timer_rel_t context_data;
    uint32_t        context_raw;
} key_held_context_t;

static app_timer_t     m_key_held_timer_id[DRV_KEYBOARD_MAX_KEYS];          //!< Array of timers used to keep track of individual key held durations.
static key_timer_rel_t m_key_held_timer_alloc_table[DRV_KEYBOARD_MAX_KEYS]; //!< Array keeping track of which timers are allocated to which keys.

// Queue holds index to timer array
NRF_QUEUE_DEF(uint8_t, m_timer_idx_queue, DRV_KEYBOARD_MAX_KEYS, NRF_QUEUE_MODE_NO_OVERFLOW);
NRF_QUEUE_INTERFACE_DEC(uint8_t, key_timer_idx_queue);
NRF_QUEUE_INTERFACE_DEF(uint8_t, key_timer_idx_queue, &m_timer_idx_queue);

STATIC_ASSERT(sizeof(key_held_context_t) == sizeof(void*));
STATIC_ASSERT(ARRAY_SIZE(m_key_held_timer_id) < 255);

static ret_code_t key_held_timer_alloc(uint8_t * timer_idx, uint8_t key_id)
{
    for (unsigned int i = 0; i < ARRAY_SIZE(m_key_held_timer_alloc_table); ++i)
    {
        if (m_key_held_timer_alloc_table[i].timer_idx >= ARRAY_SIZE(m_key_held_timer_id))
        {
            APP_ERROR_CHECK(key_timer_idx_queue_pop(timer_idx));
            m_key_held_timer_alloc_table[i].timer_idx = *timer_idx;
            m_key_held_timer_alloc_table[i].key_id    = key_id;

            return NRF_SUCCESS;
        }
    }

    return NRF_ERROR_NO_MEM;
}

static ret_code_t key_held_timer_stop_and_dealloc(uint8_t key_id)
{
    for (unsigned int i = 0; i < ARRAY_SIZE(m_key_held_timer_alloc_table); ++i)
    {
        if (m_key_held_timer_alloc_table[i].key_id == key_id &&
            m_key_held_timer_alloc_table[i].timer_idx < ARRAY_SIZE(m_key_held_timer_id))
        {
            APP_ERROR_CHECK(app_timer_stop(&m_key_held_timer_id[m_key_held_timer_alloc_table[i].timer_idx]));
            APP_ERROR_CHECK(key_timer_idx_queue_push(&m_key_held_timer_alloc_table[i].timer_idx));
            m_key_held_timer_alloc_table[i].timer_idx = (uint8_t) -1;

            return NRF_SUCCESS;
        }
    }

    return NRF_ERROR_NOT_FOUND;
}

static void m_keyboard_held_timer_timeout_handler(void * p_context)
{
    key_held_context_t key_context;
    bool key_held = false;
    int idx;

    key_context.context_raw = (uint32_t) p_context;

    // Check if the key is still pressed
    for (idx = 0; idx < m_keyboard_state_len; ++idx)
    {
        if (m_keyboard_state[idx].key_id == key_context.context_data.key_id)
        {
            key_held = true;
            break;
        }
    }

    if (key_held)
    {
        event_send(EVT_KEY_HELD, m_keyboard_state[idx], m_keyboard_state[idx].down_timestamp);
    }
}

static void m_keyboard_held_start(uint8_t key)
{
    uint8_t timer_idx;
    key_held_context_t context;

    // Allocate timer
    APP_ERROR_CHECK(key_held_timer_alloc(&timer_idx, key));

    context.context_data.key_id    = key;
    context.context_data.timer_idx = timer_idx;

    APP_ERROR_CHECK(app_timer_start(&m_key_held_timer_id[timer_idx],
                                    KBD_HELD_INTERVAL_TICKS,
                                    (void*)context.context_raw));
}
#endif /* (CONFIG_KBD_HELD_EVENT_INTERVAL_MS != 0) */

/**@brief Update state struct with new keys, maintaining key_id and down_timestamp relationships. */
static void m_keyboard_state_update(uint8_t * p_new_keys, uint16_t num_new_keys, uint32_t timestamp)
{
    keyboard_state_t keyboard_state[DRV_KEYBOARD_MAX_KEYS];

    memcpy(keyboard_state, m_keyboard_state, sizeof(m_keyboard_state));

    for (unsigned int i = 0, j = 0; i < num_new_keys; ++i)
    {
        uint8_t key_id;

        key_id = p_new_keys[i];
        m_keyboard_state[i].key_id = key_id;

        while (j < m_keyboard_state_len && keyboard_state[j].key_id < key_id)
        {
            ++j;
        }

        if (j < m_keyboard_state_len && keyboard_state[j].key_id == key_id)
        {
            // Key already pressed: use old timestamp
            m_keyboard_state[i].down_timestamp = keyboard_state[j].down_timestamp;
            ++j;
        }
        else
        {
            // Key not pressed yet: use new timestamp
            m_keyboard_state[i].down_timestamp = timestamp;
        }
    }

    m_keyboard_state_len = num_new_keys;
}

/**@brief Process keyboard scan result. */
static void m_keyboard_process(void *p_event_data, uint16_t event_size)
{
    uint8_t *p_pressed_keys;
    uint8_t num_of_pressed_keys;
    unsigned int i, j;
    uint32_t timestamp;
    bool keys_changed;
#if (CONFIG_KBD_HELD_EVENT_INTERVAL_MS != 0)
    uint8_t held_keys_to_start[DRV_KEYBOARD_MAX_KEYS];
    uint32_t num_held_keys_to_start = 0;
#endif

    p_pressed_keys      = p_event_data;
    num_of_pressed_keys = event_size;

    /*
     * Check for differences between saved and new keyboard state.
     * Send an event if a difference is found.
     */

    /* Sort elements. Only few elements are on the list - use a simple algorithm
     * with small footprint. */
    for (size_t k = 0; k < num_of_pressed_keys; k++)
    {
        size_t id = k;
        for (size_t l = k + 1; l < num_of_pressed_keys; l++)
        {
            if (p_pressed_keys[l] < p_pressed_keys[id])
            {
                id = l;
            }
        }
        if (id != k)
        {
            uint8_t tmp = p_pressed_keys[k];
            p_pressed_keys[k] = p_pressed_keys[id];
            p_pressed_keys[id] = tmp;
        }
    }
    i = 0;
    j = 0;

    keys_changed = (num_of_pressed_keys != m_keyboard_state_len);

    timestamp = app_timer_cnt_get();

    while ((i < num_of_pressed_keys) && (j < m_keyboard_state_len))
    {
        if (p_pressed_keys[i] == m_keyboard_state[j].key_id)
        {
            i += 1;
            j += 1;
            continue;
        }

        keys_changed = true;

        if (p_pressed_keys[i] > m_keyboard_state[j].key_id)
        {
#if (CONFIG_KBD_HELD_EVENT_INTERVAL_MS != 0)
            APP_ERROR_CHECK(key_held_timer_stop_and_dealloc(m_keyboard_state[j].key_id));
#endif
            event_send(EVT_KEY_UP, m_keyboard_state[j], m_keyboard_state[j].key_id);
            ++j;
        }

        if (p_pressed_keys[i] < m_keyboard_state[j].key_id)
        {
#if (CONFIG_KBD_HELD_EVENT_INTERVAL_MS != 0)
            held_keys_to_start[num_held_keys_to_start] = p_pressed_keys[i];
            ++num_held_keys_to_start;
#endif
            event_send(EVT_KEY_DOWN, p_pressed_keys[i], timestamp);
            ++i;
        }
    }

    while (j < m_keyboard_state_len)
    {
#if (CONFIG_KBD_HELD_EVENT_INTERVAL_MS != 0)
        APP_ERROR_CHECK(key_held_timer_stop_and_dealloc(m_keyboard_state[j].key_id));
#endif
        event_send(EVT_KEY_UP, m_keyboard_state[j], timestamp);
        ++j;
    }

    while (i < num_of_pressed_keys)
    {
#if (CONFIG_KBD_HELD_EVENT_INTERVAL_MS != 0)
        held_keys_to_start[num_held_keys_to_start] = p_pressed_keys[i];
        ++num_held_keys_to_start;
#endif
        event_send(EVT_KEY_DOWN, p_pressed_keys[i], timestamp);
        ++i;
    }

#if (CONFIG_KBD_HELD_EVENT_INTERVAL_MS != 0)
    for (i = 0; i < num_held_keys_to_start; ++i)
    {
        m_keyboard_held_start(held_keys_to_start[i]);
    }
#endif


    if (keys_changed)
    {
        // Save keyboard state.
        m_keyboard_state_update(p_pressed_keys, num_of_pressed_keys, timestamp);
    }
}

/**@brief Handle a keyboard driver event. */
static void m_keyboard_event_handler(uint8_t *p_pressed_keys, uint8_t num_of_pressed_keys, bool keys_blocked)
{
    if (!keys_blocked)
    {
        APP_ERROR_CHECK(app_sched_event_put(p_pressed_keys, num_of_pressed_keys, m_keyboard_process));
    }
}

ret_code_t m_keyboard_init(bool *p_delete_bonds)
{
    uint8_t pressed_keys[DRV_KEYBOARD_MAX_KEYS];
    uint8_t number_of_pressed_keys;
    bool keys_blocked;
    ret_code_t status;

    m_keyboard_enabled              = false;
    m_keyboard_state_len            = 0;
    memset(m_keyboard_state, 0, sizeof(m_keyboard_state));

    /* Initialize the keyboard driver. */
    status = drv_keyboard_init(m_keyboard_event_handler);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    /* Perform initial keyboard scan. */
    status = drv_keyboard_keys_get(pressed_keys, &number_of_pressed_keys, &keys_blocked);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    m_keyboard_event_handler(pressed_keys, number_of_pressed_keys, keys_blocked);

    /* Check the DELETE_BONDS key. */
    if (p_delete_bonds)
    {
        /* Check if the user requested bonds removal. */
        *p_delete_bonds = (keys_blocked == false) &&
                          (number_of_pressed_keys == 1) &&
                          (pressed_keys[0] == CONFIG_KBD_DELETE_BONDS_KEY_ID);
    }

#if (CONFIG_KBD_HELD_EVENT_INTERVAL_MS != 0)
    for (size_t i = 0;  i < ARRAY_SIZE(m_key_held_timer_id); ++ i)
    {
        app_timer_id_t tid = &m_key_held_timer_id[i];
        uint8_t timer_idx;

        memset(m_key_held_timer_alloc_table, (uint8_t) -1, sizeof(m_key_held_timer_alloc_table));

        status = app_timer_create(&tid,
                                  APP_TIMER_MODE_REPEATED,
                                  m_keyboard_held_timer_timeout_handler);
        if (status != NRF_SUCCESS)
        {
            return status;
        }

        // Push timer indexes to the queue to indicate that they are available.
        timer_idx = i;
        status = key_timer_idx_queue_push(&timer_idx);
        if (status != NRF_SUCCESS)
        {
            return status;
        }
    }
#endif

    return NRF_SUCCESS;
}

ret_code_t m_keyboard_enable(void)
{
    NRF_LOG_INFO("Enabled");
    ASSERT(m_keyboard_enabled == false);

    m_keyboard_enabled = true;
    return drv_keyboard_enable();
}

ret_code_t m_keyboard_disable(void)
{
    NRF_LOG_INFO("Disabled");
    ASSERT(m_keyboard_enabled == true);

    m_keyboard_enabled = false;
    return drv_keyboard_disable();
}

#if CONFIG_PWR_MGMT_ENABLED
static bool m_keyboard_shutdown(nrf_pwr_mgmt_evt_t event)
{
    bool wakeup;

    if (m_keyboard_enabled)
    {
        m_keyboard_disable();
    }

    wakeup = (event == NRF_PWR_MGMT_EVT_PREPARE_WAKEUP) ? true : false;

    return drv_keyboard_shutdown(wakeup);
}
NRF_PWR_MGMT_HANDLER_REGISTER(m_keyboard_shutdown, SHUTDOWN_PRIORITY_DEFAULT);
#endif /* CONFIG_PWR_MGMT_ENABLED */
#endif /* CONFIG_KBD_ENABLED */

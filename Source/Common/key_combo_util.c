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

#include <stdlib.h>
#include "nrf_assert.h"
#include "app_debug.h"
#include "app_timer.h"

#include "key_combo_util.h"

#include "sr3_config.h"

#if CONFIG_KBD_KEY_COMBO_ENABLED

#define NRF_LOG_MODULE_NAME key_combo_util
#define NRF_LOG_LEVEL CONFIG_KBD_COMBO_LOG_LEVEL
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

#if (CONFIG_KBD_HELD_EVENT_INTERVAL_MS == 0)
#error "Combo keys requires non zero CONFIG_KBD_HELD_EVENT_INTERVAL_MS."
#endif

NRF_SECTION_DEF(combo_descriptions, key_combo_desc_t);
NRF_SECTION_DEF(combo_member_keys, key_combo_member_t);

static uint32_t           m_combo_bitmask = 0; // Combo bitmask of currently pressed keys
static uint8_t            m_triggering_key_id; // Key which triggered combo timing
static int32_t            m_combo_duration_ms; // Duration of current bitmask in milliseconds
static key_combo_desc_t * m_active_combo = 0;  // When valid: pointer to combo description struct. Otherwise NULL

static uint32_t key_id_to_bit_msk(uint8_t key_id)
{
    for (unsigned int i = 0; i < NRF_SECTION_ITEM_COUNT(combo_member_keys, key_combo_member_t); ++i)
    {
        if (NRF_SECTION_ITEM_GET(combo_member_keys, key_combo_member_t, i)->key_id == key_id)
        {
            return (1ul << i);
        }
    }

    return 0;
}

static uint32_t combo_desc_to_bitmask(key_combo_desc_t * combo_desc)
{
    uint32_t bitmask = 0;

    for (unsigned int i = 0; i < combo_desc->combo_num_keys; ++i)
    {
        bitmask |= key_id_to_bit_msk(combo_desc->combo_keys[i]);
    }

    return bitmask;
}

static void process_combo_key_change(uint8_t key_id, bool key_press)
{
    uint32_t key_bitmask;

    key_bitmask = key_id_to_bit_msk(key_id);

    if (key_bitmask == 0)
    {
        // Irrelevant key: invalidate combo
        m_active_combo = NULL;
        return;
    }

    if (key_press && ((key_bitmask & m_combo_bitmask) != 0))
    {
        // Unexpected: key already set
        NRF_LOG_WARNING("Key already set: 0x%02X", key_id);
        return;
    }
    if (!key_press && ((key_bitmask & m_combo_bitmask) == 0))
    {
        // Unexpected: key already released
        NRF_LOG_WARNING("Key already released: 0x%02X", key_id);
        return;
    }

    NRF_LOG_DEBUG("Combo key id 0x%02X %s", key_id, (key_press ? "pressed" : "released"));


    if (key_press)
    {
        m_combo_bitmask |= key_bitmask; // Add key to bitmask
    }
    else
    {
        m_combo_bitmask ^= key_bitmask; // Remove key from bitmask
    }
    // Reset state
    m_active_combo      = NULL;
    m_combo_duration_ms = 0;

    // Find matching combos
    for (int i = 0; i < NRF_SECTION_ITEM_COUNT(combo_descriptions, key_combo_desc_t); ++i)
    {
        if (m_combo_bitmask == combo_desc_to_bitmask(NRF_SECTION_ITEM_GET(combo_descriptions, key_combo_desc_t, i)))
        {
            m_active_combo      = NRF_SECTION_ITEM_GET(combo_descriptions, key_combo_desc_t, i);
            // When a key release triggers combo, choose first key in combo as trigger key, since key_id is no longer held
            m_triggering_key_id = key_press ? key_id : m_active_combo->combo_keys[0];
            NRF_LOG_DEBUG("Valid combo detected. key_combo_desc_t* = 0x%08x", m_active_combo);
            break;
        }
    }
}

static void process_combo_key_held(uint8_t key_id)
{
    if (m_active_combo == NULL        ||
        key_id != m_triggering_key_id ||
        m_combo_duration_ms == (int32_t) -1)
    {
        return;
    }

    m_combo_duration_ms += CONFIG_KBD_HELD_EVENT_INTERVAL_MS;

    if (m_combo_duration_ms >= m_active_combo->combo_duration_ms)
    {
        NRF_LOG_DEBUG("Key combo triggered. key_combo_desc_t* = 0x%08x", m_active_combo);

        m_active_combo->handler(0);

        // Set to -1 to indicate that action has been triggered
        m_combo_duration_ms = (int32_t) -1;
    }
    else
    {
        NRF_LOG_DEBUG("Key combo time remaining: %d ms", m_active_combo->combo_duration_ms - m_combo_duration_ms);
    }
}

ret_code_t key_combo_util_init(void)
{

    // Verify that total number of keys used in key combos does not exceed allotted data width
    if (NRF_SECTION_ITEM_COUNT(combo_member_keys, key_combo_member_t) > 32)
    {
        NRF_LOG_ERROR("Too many combo keys");
        return NRF_ERROR_INVALID_DATA;
    }

    // Verify that keys are only used once in the same combo,
    // and that all keys are registered as combo members
    for (int i = 0; i < NRF_SECTION_ITEM_COUNT(combo_descriptions, key_combo_desc_t); ++i)
    {
        key_combo_desc_t * combo = NRF_SECTION_ITEM_GET(combo_descriptions, key_combo_desc_t, i);
        uint32_t combo_bitmask = 0;

        for (int j = 0; j < combo->combo_num_keys; ++j)
        {
            uint32_t key_bitmask = key_id_to_bit_msk(combo->combo_keys[j]);

            if (key_bitmask == 0)
            {
                NRF_LOG_ERROR("Combo key not registered: 0x%02X", combo->combo_keys[j]);
                return NRF_ERROR_INVALID_DATA;
            }

            if ((key_bitmask & combo_bitmask) != 0)
            {
                NRF_LOG_ERROR("Same key used more than once in key combo: 0x%02X", combo->combo_keys[j]);
                return NRF_ERROR_INVALID_DATA;
            }
            combo_bitmask |= key_bitmask;
        }
    }

    return NRF_SUCCESS;
}

bool key_combo_util_key_process(const event_t * p_event)
{
    switch (p_event->type)
    {
        case EVT_KEY_DOWN:
            process_combo_key_change(p_event->key.id, true);
            break;

        case EVT_KEY_UP:
            process_combo_key_change(p_event->key.id, false);
            break;

        case EVT_KEY_HELD:
            process_combo_key_held(p_event->key.id);
            break;

        default:
            break;
    }

    return false;
}
#endif /* CONFIG_KBD_KEY_COMBO_ENABLED */

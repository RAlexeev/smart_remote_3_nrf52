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
#include "m_protocol_hid.h"
#include "m_protocol_hid_state.h"
#include "sr3_config.h"

#define NRF_LOG_MODULE_NAME m_protocol_hid
#define NRF_LOG_LEVEL CONFIG_PROTOCOL_HID_MODULE_LOG_LEVEL
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();


/**@brief Compare Key ID in HID Keymap entries. */
static int m_protocol_hid_keymap_compare(const void *a, const void *b)
{
    const sr3_hid_keymap_t *p_a = a;
    const sr3_hid_keymap_t *p_b = b;

    return (p_a->key_id - p_b->key_id);
}

/**@brief Translate Key ID to HID Usage. */
static uint32_t m_protocol_hid_key2usage(uint16_t key_id)
{
    sr3_hid_keymap_t key =
    {
        .key_id = key_id
    };
    sr3_hid_keymap_t *p_map = bsearch(&key,
                                      g_sr3_hid_keymap,
                                      g_sr3_hid_keymap_size,
                                      sizeof(key),
                                      m_protocol_hid_keymap_compare);
    if (p_map == NULL)
    {
        return 0;
    }

    return p_map->hid_usage;
}

/**@brief Handle Bluetooth connection events. */
void m_protocol_hid_bt_event_handler(unsigned int event_id)
{
    switch (event_id)
    {
        case BT_CONN_STATE_SECURED:
            APP_ERROR_CHECK(m_protocol_hid_state_connect());
            break;

        case BT_CONN_STATE_DISCONNECTED:
            APP_ERROR_CHECK(m_protocol_hid_state_disconnect());
            break;

        case BT_CONN_STATE_CONNECTED:
            /* Ignore. */
            break;

        default:
            APP_ERROR_CHECK_BOOL(false);
            break;
    }
}

/**@brief Handle key events. */
void  m_protocol_hid_key_event_handler(uint16_t key_id, bool key_down)
{
    /* Key down event increases key ref counter, key up decreases it. */
    int16_t report = (key_down != false) ? (1) : (-1);

    /* Do not translate gyro on/off key. */
    if (key_id == CONFIG_GYRO_MODE_KEY_ID)
    {
        return;
    }

#if CONFIG_GOOGLE_VOICE_SHORTCUT_ENABLED
    if (key_id == CONFIG_GOOGLE_VOICE_SHORTCUT_KEY_ID)
    {
        if (key_down != false)
        {
            m_protocol_hid_state_update_abs(HID_USAGE(0x07, 0xE0), report); /* Left Ctrl */
            m_protocol_hid_state_update_abs(HID_USAGE(0x07, 0xE1), report); /* Left Shift */
            m_protocol_hid_state_update_abs(HID_USAGE(0x07, 0x37), report); /* . and > */
        }
        else
        {

            m_protocol_hid_state_update_abs(HID_USAGE(0x07, 0x37), report); /* . and > */
            m_protocol_hid_state_update_abs(HID_USAGE(0x07, 0xE1), report); /* Left Shift */
            m_protocol_hid_state_update_abs(HID_USAGE(0x07, 0xE0), report); /* Left Ctrl */
        }

        return;
    }
#endif /* CONFIG_GOOGLE_VOICE_SHORTCUT_ENABLED */

    /* For generic key, look for translation in HID Keymap.
     * If no translation found, send Space key. */
    uint32_t usage = m_protocol_hid_key2usage(key_id);
    if (usage == 0)
    {
        usage = HID_USAGE(0x07, 0x2C);
    }
    m_protocol_hid_state_update_abs(usage, report);
}


ret_code_t m_protocol_hid_init(void)
{
#if CONFIG_DEBUG_ENABLED
    /* Validate the order of key IDs on the key map array. */
    for (unsigned int i = 1; i < g_sr3_hid_keymap_size; i++)
    {
        if (g_sr3_hid_keymap[i - 1].key_id >= g_sr3_hid_keymap[i].key_id)
        {
            NRF_LOG_ERROR("The g_sr3_hid_keymap array must be sorted by key_id!");
            APP_ERROR_CHECK_BOOL(false);
        }
    }
#endif

    /* Initialize protocol HID module. */
    APP_ERROR_CHECK(m_protocol_hid_state_init());

    return NRF_SUCCESS;
}

bool m_protocol_hid_event_handler(const event_t *p_event)
{
    ASSERT(p_event != NULL);

    switch (p_event->type)
    {
        /* Key events. */
        case EVT_KEY_DOWN:
            m_protocol_hid_key_event_handler(p_event->key.id, true);
            break;

        case EVT_KEY_UP:
            m_protocol_hid_key_event_handler(p_event->key.id, false);
            break;

        /* Motion events. */
        case EVT_REL_X:
            m_protocol_hid_state_update_rel(HID_USAGE(0x01, 0x30), p_event->rel.shift);
            break;

        case EVT_REL_Y:
            m_protocol_hid_state_update_rel(HID_USAGE(0x01, 0x31), p_event->rel.shift);
            break;

        case EVT_REL_WHEEL:
            m_protocol_hid_state_update_rel(HID_USAGE(0x01, 0x38), p_event->rel.shift);
            break;

        case EVT_REL_PAN:
            m_protocol_hid_state_update_rel(HID_USAGE(0x0C, 0x238), p_event->rel.shift);
            break;

        /* Bluetooth connection events. */
        case EVT_BT_CONN_STATE:
            m_protocol_hid_bt_event_handler(p_event->bt.data);
            break;

        default:
            /* Ignore. */
            break;
    }

    return false;
}

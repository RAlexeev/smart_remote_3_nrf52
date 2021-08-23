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
#include "m_protocol_ir.h"
#include "sr3_config.h"

#if CONFIG_IR_TX_ENABLED

#define NRF_LOG_MODULE_NAME m_protocol_ir
#define NRF_LOG_LEVEL CONFIG_IR_MODULE_LOG_LEVEL
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

/**@brief Compare Key ID in IR Keymap entries. */
static int m_protocol_ir_keymap_compare(const void *a, const void *b)
{
    const sr3_ir_keymap_t *p_a = a;
    const sr3_ir_keymap_t *p_b = b;

    return (p_a->key_id - p_b->key_id);
}

/**@brief Translate Key ID to IR Symbol. */
static const sr3_ir_symbol_t *m_protocol_ir_key2symbol(uint16_t key_id)
{
    sr3_ir_keymap_t *p_map;
    sr3_ir_keymap_t key =
    {
        .key_id = key_id
    };

    p_map = bsearch(&key,
                    g_sr3_ir_keymap,
                    g_sr3_ir_keymap_size,
                    sizeof(key),
                    m_protocol_ir_keymap_compare);

    if (p_map == NULL)
    {
        return NULL;
    }

    return &(p_map->symbol);
}

ret_code_t m_protocol_ir_init(void)
{
#if CONFIG_DEBUG_ENABLED
    /* Verify that IR keymap is sorted as expected. */
    const sr3_ir_keymap_t *p_map = g_sr3_ir_keymap;
    unsigned int i = g_sr3_ir_keymap_size;
    int v = -1;

    while (i--)
    {
        if (v >= p_map->key_id)
        {
            NRF_LOG_ERROR("The g_sr3_ir_keymap array must be sorted by key_id!");
            APP_ERROR_CHECK_BOOL(false);
        }

        v = (p_map++)->key_id;
    }
#endif /* CONFIG_DEBUG_ENABLED */

    return NRF_SUCCESS;
}

bool m_protocol_ir_event_handler(const event_t *p_event)
{
    static uint16_t transmitted_key_id = KEY_ID_INVALID;
    static uint16_t next_key_id = KEY_ID_INVALID;
    const sr3_ir_symbol_t *p_symbol;

    ASSERT(p_event != NULL);

    switch (p_event->type)
    {
        case EVT_KEY_DOWN:
            ASSERT(p_event->key.id != KEY_ID_INVALID);

            p_symbol = m_protocol_ir_key2symbol(p_event->key.id);
            if (p_symbol)
            {
                if (transmitted_key_id == KEY_ID_INVALID)
                {
                    transmitted_key_id = p_event->key.id;
                    event_send(EVT_IR_SYMBOL, p_symbol);
                }
                else
                {
                    next_key_id = p_event->key.id;
                }
            }
            break;

        case EVT_KEY_UP:
            ASSERT(p_event->key.id != KEY_ID_INVALID);

            if (transmitted_key_id == p_event->key.id)
            {
                transmitted_key_id  = next_key_id;
                next_key_id         = KEY_ID_INVALID;

                if (transmitted_key_id != KEY_ID_INVALID)
                {
                    p_symbol = m_protocol_ir_key2symbol(transmitted_key_id);
                }
                else
                {
                    p_symbol = NULL;
                }

                event_send(EVT_IR_SYMBOL, p_symbol);
            }

            if (next_key_id == p_event->key.id)
            {
                next_key_id = KEY_ID_INVALID;
            }
            break;

        default:
            /* Ignore */
            break;
    }

    return false;
}

#endif /* CONFIG_IR_TX_ENABLED */

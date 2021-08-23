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
#include <stdlib.h>

#include "ble.h"
#include "ble_advdata.h"
#include "nrf.h"
#include "nrf_assert.h"
#include "nrf_error.h"

#include "m_coms_ble_addr.h"
#include "sr3_config.h"

#define NRF_LOG_MODULE_NAME m_coms_ble_addr
#define NRF_LOG_LEVEL CONFIG_BLE_ADDR_LOG_LEVEL
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

#define MAX_SIMULTANEOUS_CONTEXT_WRITES 1

#if CONFIG_BOND_PUBLIC_ADDR_ENABLED && CONFIG_CHANGE_ADDRESS
#error Cannot both use public address (static) and randomly change address
#endif

/**@brief Peer representation. */
typedef struct
{
    pm_peer_id_t    id;
    uint32_t        rank;
} m_coms_ble_addr_peer_t;

/**@brief Application data attached to peer. */
typedef struct
{
    ble_gap_addr_t      ble_addr;
    uint8_t             __padding[1];
} m_coms_ble_addr_app_data_t;

// Application data size must be multiple of word size.
STATIC_ASSERT((sizeof(m_coms_ble_addr_app_data_t) % sizeof(uint32_t)) == 0);

typedef struct
{
    pm_store_token_t            token;
    m_coms_ble_addr_app_data_t  app_data;
} m_coms_ble_addr_app_data_buffer_t;

#if CONFIG_BLE_DYNAMIC_ADDR_ENABLED

/**@brief Data buffer holding application data during flash operations. */
static m_coms_ble_addr_app_data_buffer_t    s_app_data_buffer[MAX_SIMULTANEOUS_CONTEXT_WRITES];

ret_code_t m_coms_ble_addr_local_addr_set(pm_peer_id_t peer_id, ble_gap_addr_t *p_ble_addr)
{
    m_coms_ble_addr_app_data_buffer_t *p_app_data_buffer = NULL;
    ret_code_t status;

    if (!p_ble_addr)
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    // Allocate space for application data.
    for (size_t i = 0; i < ARRAY_SIZE(s_app_data_buffer); i++)
    {
        if (s_app_data_buffer[i].token == PM_STORE_TOKEN_INVALID)
        {
            p_app_data_buffer = &s_app_data_buffer[i];
            break;
        }
    }

    if (!p_app_data_buffer)
    {
        return NRF_ERROR_NO_MEM;
    }

    memcpy(&(p_app_data_buffer->app_data.ble_addr), p_ble_addr, sizeof(ble_gap_addr_t));

    status = pm_peer_data_app_data_store(peer_id,
                                       (const uint8_t *)&(p_app_data_buffer->app_data),
                                       sizeof(p_app_data_buffer->app_data),
                                       &(p_app_data_buffer->token));

    if (status == NRF_SUCCESS)
    {
        NRF_LOG_DEBUG("Application data save: %d [0x%08X]",
                  peer_id, p_app_data_buffer->token);
        NRF_LOG_DEBUG("\t - Local Address: %02X:%02X:%02X:%02X:%02X:%02X",
                   p_app_data_buffer->app_data.ble_addr.addr[5],
                   p_app_data_buffer->app_data.ble_addr.addr[4],
                   p_app_data_buffer->app_data.ble_addr.addr[3],
                   p_app_data_buffer->app_data.ble_addr.addr[2],
                   p_app_data_buffer->app_data.ble_addr.addr[1],
                   p_app_data_buffer->app_data.ble_addr.addr[0]);
    }

    return status;
}

ret_code_t m_coms_ble_addr_local_addr_get(pm_peer_id_t peer_id, ble_gap_addr_t *p_ble_addr)
{
    m_coms_ble_addr_app_data_t app_data;
    uint16_t app_data_len;
    ret_code_t status;

    if (!p_ble_addr)
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    app_data_len = sizeof(app_data);
    status = pm_peer_data_app_data_load(peer_id, (uint8_t *)&app_data, &app_data_len);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    ASSERT(app_data_len == sizeof(app_data));

    NRF_LOG_DEBUG("Application data load: %d", peer_id);
    NRF_LOG_DEBUG("\t- Local Address: %02X:%02X:%02X:%02X:%02X:%02X",
               app_data.ble_addr.addr[5],
               app_data.ble_addr.addr[4],
               app_data.ble_addr.addr[3],
               app_data.ble_addr.addr[2],
               app_data.ble_addr.addr[1],
               app_data.ble_addr.addr[0]);

    *p_ble_addr = app_data.ble_addr;

    return NRF_SUCCESS;
}

ret_code_t m_coms_ble_addr_local_addr_new(ble_gap_addr_t *addr)
{
    uint64_t largest_addr_val;
    pm_peer_id_t peer_id;

    largest_addr_val = 0;
    peer_id = PM_PEER_ID_INVALID;
    while ((peer_id = pm_next_peer_id_get(peer_id)) != PM_PEER_ID_INVALID)
    {
        m_coms_ble_addr_app_data_t app_data;
        uint16_t app_data_len;
        uint64_t addr_val;
        ret_code_t status;

        app_data_len = sizeof(app_data);
        status = pm_peer_data_app_data_load(peer_id, (uint8_t *)&app_data, &app_data_len);
        if (status != NRF_SUCCESS)
        {
            return status;
        }

        ASSERT(app_data_len == sizeof(app_data));

        addr_val =  (uint64_t)(app_data.ble_addr.addr[0]) << 0;
        addr_val |= (uint64_t)(app_data.ble_addr.addr[1]) << 8;
        addr_val |= (uint64_t)(app_data.ble_addr.addr[2]) << 16;
        addr_val |= (uint64_t)(app_data.ble_addr.addr[3]) << 24;
        addr_val |= (uint64_t)(app_data.ble_addr.addr[4]) << 32;
        addr_val |= (uint64_t)(app_data.ble_addr.addr[5]) << 40;

        if (addr_val > largest_addr_val)
        {
            largest_addr_val = addr_val;
        }
    }

    if (largest_addr_val == 0ULL)
    {
        return NRF_ERROR_INVALID_STATE;
    }

    largest_addr_val += 1;

    addr->addr_type = BLE_GAP_ADDR_TYPE_RANDOM_STATIC;
    addr->addr[0]   = (largest_addr_val >> 0);
    addr->addr[1]   = (largest_addr_val >> 8);
    addr->addr[2]   = (largest_addr_val >> 16);
    addr->addr[3]   = (largest_addr_val >> 24);
    addr->addr[4]   = (largest_addr_val >> 32);
    addr->addr[5]   = (largest_addr_val >> 40) | 0xC0;  // Two upper bits must be "1"

    NRF_LOG_DEBUG("New Local Address: %02X:%02X:%02X:%02X:%02X:%02X",
               addr->addr[5],
               addr->addr[4],
               addr->addr[3],
               addr->addr[2],
               addr->addr[1],
               addr->addr[0]);


    return NRF_SUCCESS;
}

static void m_coms_ble_addr_pm_evt_handler(const pm_evt_t *p_evt)
{
    size_t i;

    switch (p_evt->evt_id)
    {
        case PM_EVT_PEER_DATA_UPDATE_SUCCEEDED:
            if (p_evt->params.peer_data_update_succeeded.data_id == PM_PEER_DATA_ID_APPLICATION)
            {
                NRF_LOG_DEBUG("PM_EVT_PEER_DATA_UPDATE_SUCCEEDED: %d [0x%08X]",
                          p_evt->peer_id,
                          (p_evt->params.peer_data_update_succeeded.action != PM_PEER_DATA_OP_DELETE) ? p_evt->params.peer_data_update_succeeded.token : PM_STORE_TOKEN_INVALID);

                if (p_evt->params.peer_data_update_succeeded.action != PM_PEER_DATA_OP_DELETE)
                {
                    // Free application data buffer since it is no longer needed.
                    for (i = 0; i < ARRAY_SIZE(s_app_data_buffer); i++)
                    {
                        if (s_app_data_buffer[i].token == p_evt->params.peer_data_update_succeeded.token)
                        {
                            s_app_data_buffer[i].token = PM_STORE_TOKEN_INVALID;
                            break;
                        }
                    }

                    if (i == ARRAY_SIZE(s_app_data_buffer))
                    {
                        NRF_LOG_WARNING("%s(): WARNING: No application data buffer found for PM store token 0x%08X!",
                                  (uint32_t)__func__, p_evt->params.peer_data_update_succeeded.token);
                    }
                }
            }
            break;

        default:
            /* Ignore */
            break;
    }
}
#endif /* CONFIG_BLE_DYNAMIC_ADDR_ENABLED */

ret_code_t m_coms_ble_addr_peer_ids_get(pm_peer_id_t *p_peer_ids, unsigned int *p_peers_num)
{
    m_coms_ble_addr_peer_t peers[CONFIG_MAX_BONDS];
    pm_peer_id_t peer_id = PM_PEER_ID_INVALID;
    unsigned int n = 0;

    while ((peer_id = pm_next_peer_id_get(peer_id)) != PM_PEER_ID_INVALID)
    {
        if (n >= CONFIG_MAX_BONDS)
        {
            return NRF_ERROR_NO_MEM;
        }

#if CONFIG_BLE_PEER_RANK_ENABLED
        uint16_t   rank_len = sizeof(peers[n].rank);
        ret_code_t status;

        status = pm_peer_data_load(peer_id, PM_PEER_DATA_ID_PEER_RANK, &peers[n].rank, &rank_len);
        if (status != NRF_SUCCESS)
        {
            return status;
        }

        ASSERT(rank_len == sizeof(peers[n].rank));
#endif /* CONFIG_BLE_PEER_RANK_ENABLED */

        peers[n++].id = peer_id;
    }

#if CONFIG_BLE_PEER_RANK_ENABLED
    /*
     * Sort peers by rank. There are only a few elements on the list,
     * so use a simple algorithm with small footprint.
     */
    for (size_t k = 0; k < n; k++)
    {
        size_t id = k;
        for (size_t l = k + 1; l < n; l++)
        {
            if (peers[l].rank < peers[id].rank)
            {
                id = l;
            }
        }
        if (id != k)
        {
            m_coms_ble_addr_peer_t tmp = peers[k];
            peers[k]  = peers[id];
            peers[id] = tmp;
        }
    }

    NRF_LOG_DEBUG("Peer Ranks:");
    for (size_t i = 0; i < n; i++)
    {
        NRF_LOG_DEBUG("\t- Peer %d Rank: %d", peers[i].id, peers[i].rank);
    }
#endif /* CONFIG_BLE_PEER_RANK_ENABLED */

    *p_peers_num = n;
    while (n > 0)
    {
        n -= 1;
        p_peer_ids[n] = peers[n].id;
    }

    return NRF_SUCCESS;
}

ret_code_t m_coms_ble_addr_init(void)
{
#if CONFIG_BOND_PUBLIC_ADDR_ENABLED
#if (CONFIG_BOND_PUBLIC_ADDR_LOC > 0x100010F8) || \
    (CONFIG_BOND_PUBLIC_ADDR_LOC < 0x10001080)
#error Invalid register selection for public Bluetooth address
#endif
    {
        uint32_t       ret_code;
        ble_gap_addr_t bt_addr;
        uint8_t        empty_flash_mask[BLE_GAP_ADDR_LEN];

        bt_addr.addr_type = BLE_GAP_ADDR_TYPE_PUBLIC;
        memcpy(bt_addr.addr, (uint32_t*) CONFIG_BOND_PUBLIC_ADDR_LOC, BLE_GAP_ADDR_LEN);

        memset(empty_flash_mask, 0xFF, sizeof(empty_flash_mask));
        if (memcmp(empty_flash_mask, bt_addr.addr, BLE_GAP_ADDR_LEN) == 0)
        {
                NRF_LOG_WARNING("Public Bluetooth address not found. Using default address instead.");
        }
        else
        {
            ret_code = sd_ble_gap_addr_set(&bt_addr);
            if (ret_code != NRF_SUCCESS)
            {
                NRF_LOG_ERROR("Could not set public Bluetooth address. Using default address instead.");
            }
        }
    }

#endif /* CONFIG_BOND_PUBLIC_ADDR_ENABLED */

#if CONFIG_BLE_DYNAMIC_ADDR_ENABLED
    {
        ret_code_t status;

        // Initialize application data buffers.
        for (size_t i = 0; i < ARRAY_SIZE(s_app_data_buffer); i++)
        {
            s_app_data_buffer[i].token = PM_STORE_TOKEN_INVALID;
        }

        // Register the module in Peer Manager and return.
        status = pm_register(m_coms_ble_addr_pm_evt_handler);
        if (status != NRF_SUCCESS)
        {
            return status;
        }
    }
#endif

    return NRF_SUCCESS;
}

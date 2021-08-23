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
#include "nrf_assert.h"
#include "app_scheduler.h"

static app_isched_t        *mp_isched;
static const nrf_balloc_t  *mp_pool;
static unsigned int         m_max_event_data_size;

// Define own allocator.
NRF_BALLOC_INTERFACE_LOCAL_DEF(app_sched_event_t, app_sched_event, mp_pool);

uint32_t app_sched_init(app_isched_t *p_isched,
                        const nrf_balloc_t *p_pool,
                        unsigned int max_event_data_size)
{
    if ((p_isched == NULL) || (p_pool == NULL))
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    mp_isched               = p_isched;
    mp_pool                 = p_pool;
    m_max_event_data_size   = max_event_data_size;

    return NRF_SUCCESS;
}

static void app_sched_event_handler(void *p_context)
{
    app_sched_event_t *p_evt = p_context;

    ASSERT(p_evt != NULL);
    ASSERT(p_evt->handler != NULL);

    if (p_evt->event_data_size > 0)
    {
        p_evt->handler(p_evt->event_data, p_evt->event_data_size);
    }
    else
    {
        p_evt->handler(NULL, 0);
    }

    app_sched_event_free(p_evt);
}

uint32_t app_sched_event_put(void *                    p_event_data,
                             uint16_t                  event_size,
                             app_sched_event_handler_t handler)
{
    app_sched_event_t *p_evt;
    ret_code_t status;

    if (handler == NULL)
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    if (event_size > m_max_event_data_size)
    {
        return NRF_ERROR_DATA_SIZE;
    }

    p_evt = app_sched_event_alloc();
    if (p_evt == NULL)
    {
        return NRF_ERROR_NO_MEM;
    }

    p_evt->handler = handler;
    if ((p_event_data != NULL) && (event_size > 0))
    {
        memcpy(p_evt->event_data, p_event_data, event_size);
        p_evt->event_data_size = event_size;
    }
    else
    {
        p_evt->event_data_size = 0;
    }

    status = app_isched_event_put(mp_isched, app_sched_event_handler, p_evt);
    if (status != NRF_SUCCESS)
    {
        app_sched_event_free(p_evt);
    }

    return status;
}

uint8_t app_sched_get_max_utilization(void)
{
    return nrf_balloc_max_utilization_get(mp_pool);
}

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

#include "app_debug.h"
#include "app_timer.h"
#include "nrf_atomic.h"
#include "nrf_balloc.h"
#include "nrf_queue.h"
#include "nrf_drv_rng.h"

#include "resources.h"
#include "sr3_config.h"
#include "rng_monitor.h"

#if CONFIG_RNG_MONITOR_ENABLED

#define NRF_LOG_MODULE_NAME rng_monitor
#define NRF_LOG_LEVEL CONFIG_RNG_MONITOR_LOG_LEVEL
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

// Verify SDK configuration.
STATIC_ASSERT(RNG_ENABLED);

/**@brief RNG Monitor request structure */
typedef struct
{
    rng_monitor_notification_handler_t  notification_handler;
    void                                *p_notification_context;
    uint8_t                             request_size;
} rng_monitor_request_t;

/**@brief RNG Monitor requests pool */
NRF_BALLOC_DEF(s_rng_monitor_request_pool,
               sizeof(rng_monitor_request_t),
               CONFIG_RNG_MONITOR_MAX_REQUESTS);

/**@brief RNG Monitor requests queue */
NRF_QUEUE_DEF(rng_monitor_request_t *,
              s_rng_monitor_request_queue,
              CONFIG_RNG_MONITOR_MAX_REQUESTS,
              NRF_QUEUE_MODE_NO_OVERFLOW);

static nrf_atomic_flag_t    s_rng_monitor_active;   //!< Flag set if RNG is actively polled.
APP_TIMER_DEF               (s_rng_monitor_timer);  //!< Timer used to poll RNG.

/**@brief Function processing requests. */
static void rng_monitor_process_requests(void *p_context)
{
    uint8_t random_bytes_available;

    // Check how many random bytes are avaiable in RNG.
    nrf_drv_rng_bytes_available(&random_bytes_available);

    while (true)
    {
        rng_monitor_request_t *p_req;

        // Peek first request in queue
        if (nrf_queue_peek(&s_rng_monitor_request_queue, &p_req) != NRF_SUCCESS)
        {
            // Queue is empty.
            nrf_atomic_flag_clear(&s_rng_monitor_active);
            NRF_LOG_INFO("Idle");
            return;
        }

        NRF_LOG_INFO("RNG Pool: %u bytes", random_bytes_available);
        if (random_bytes_available < p_req->request_size)
        {
            // There is not enough random data, try again later.
            break;
        }

        // Request is satisfied: Call notification handler.
        NRF_LOG_DEBUG("Request 0x%08X satisfied (size %u bytes)", (uint32_t)p_req, p_req->request_size);
        ASSERT(p_req->notification_handler != NULL);
        p_req->notification_handler(p_req->p_notification_context);

        // Do not count consumed random bytes in the budget.
        random_bytes_available -= p_req->request_size;

        // Remove satisfied request from queue and free it.
        APP_ERROR_CHECK(nrf_queue_pop(&s_rng_monitor_request_queue, &p_req));
        nrf_balloc_free(&s_rng_monitor_request_pool, p_req);
    }

    // Schedule next try.
    APP_ERROR_CHECK(app_timer_start(s_rng_monitor_timer,
                                    APP_TIMER_TICKS(CONFIG_RNG_MONITOR_POLL_INTERVAL),
                                    NULL));
}

ret_code_t rng_monitor_init(void)
{
    ret_code_t status;

    nrf_atomic_flag_clear(&s_rng_monitor_active);

    status = nrf_balloc_init(&s_rng_monitor_request_pool);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    return app_timer_create(&s_rng_monitor_timer,
                            APP_TIMER_MODE_SINGLE_SHOT,
                            rng_monitor_process_requests);
}

ret_code_t rng_monitor_request(uint8_t random_bytes_requested,
                               rng_monitor_notification_handler_t notification_handler,
                               void *p_notification_context)
{
    rng_monitor_request_t *p_req;
    ret_code_t status;

    if ((random_bytes_requested > RNG_CONFIG_POOL_SIZE) || (notification_handler == NULL))
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    // Allocate and fill new request.
    p_req = nrf_balloc_alloc(&s_rng_monitor_request_pool);
    if (p_req == NULL)
    {
        return NRF_ERROR_NO_MEM;
    }

    p_req->notification_handler     = notification_handler;
    p_req->p_notification_context   = p_notification_context;
    p_req->request_size             = random_bytes_requested;

    // Enqueue request
    status = nrf_queue_push(&s_rng_monitor_request_queue, &p_req);
    if (status != NRF_SUCCESS)
    {
        nrf_balloc_free(&s_rng_monitor_request_pool, p_req);
        return status;
    }

    NRF_LOG_DEBUG("Request 0x%08X enqueued (size %u bytes)", (uint32_t)p_req, p_req->request_size);

    // Activate polling if module is idle.
    if (!nrf_atomic_flag_set_fetch(&s_rng_monitor_active))
    {
        NRF_LOG_DEBUG("Active");
        return app_isched_event_put(&g_fg_scheduler, rng_monitor_process_requests, NULL);
    }

    return NRF_SUCCESS;
}

#endif /* CONFIG_RNG_MONITOR_ENABLED */


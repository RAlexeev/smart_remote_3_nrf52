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
#include "app_isched.h"
#include "app_util_platform.h"

ret_code_t app_isched_install_hook(app_isched_t *p_isched,
                                   app_isched_hook_type_t hook_type,
                                   app_isched_hook_t hook,
                                   void *p_hook_context)
{
    if ((p_isched == NULL) || (hook_type >= APP_ISCHED_HOOKS_COUNT) || (hook == NULL))
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    // Atomically remove old hook.
    p_isched->hooks[hook_type] = NULL;
    __DMB();

    // Update hook context before installing new hook.
    p_isched->p_hook_contexts[hook_type] = p_hook_context;
    __DMB();

    // Atomically install new hook.
    p_isched->hooks[hook_type] = hook;
    __DMB();

    return NRF_SUCCESS;
}

static void app_isched_hook_exec(const app_isched_t *p_isched,
                                 const app_isched_hook_type_t hook_type,
                                 const app_isched_event_t *p_evt)
{
    app_isched_hook_t hook;
    void *p_hook_context;

    ASSERT(p_isched != NULL);
    ASSERT(hook_type < APP_ISCHED_HOOKS_COUNT);

    // This check is atomic and provides early exit if there is no hook installed.
    if (p_isched->hooks[hook_type] == NULL)
    {
        return;
    }

    CRITICAL_REGION_ENTER();
    hook            = p_isched->hooks[hook_type];
    p_hook_context  = p_isched->p_hook_contexts[hook_type];
    CRITICAL_REGION_EXIT();

    if (hook != NULL)
    {
        hook(p_isched, p_evt, p_hook_context);
    }
}
ret_code_t app_isched_event_put(app_isched_t *p_isched,
                                app_isched_event_handler_t handler,
                                void *p_context)
{
    app_isched_event_t evt;
    ret_code_t status;

    if ((p_isched == NULL) || (handler == NULL))
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    evt.handler     = handler;
    evt.p_context   = p_context;

    app_isched_hook_exec(p_isched, APP_ISCHED_HOOK_PRE_PUT, &evt);

    status = nrf_queue_push(p_isched->p_event_queue, &evt);
    if (status == NRF_SUCCESS)
    {
        // Do not pass event to post-put hook, as it might be already processed and freed.
        app_isched_hook_exec(p_isched, APP_ISCHED_HOOK_POST_PUT, NULL);
    }

    return status;
}

ret_code_t app_isched_events_execute(app_isched_t *p_isched)
{
    app_isched_event_t evt;

    if (p_isched == NULL)
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    while (nrf_queue_pop(p_isched->p_event_queue, &evt) == NRF_SUCCESS)
    {
        ASSERT(evt.handler != NULL);

        app_isched_hook_exec(p_isched, APP_ISCHED_HOOK_PRE_EXEC, &evt);
        evt.handler(evt.p_context);
        app_isched_hook_exec(p_isched, APP_ISCHED_HOOK_POST_EXEC, &evt);
    }

    return NRF_SUCCESS;
}

size_t app_isched_get_max_utilization(const app_isched_t *p_isched)
{
    return nrf_queue_max_utilization_get(p_isched->p_event_queue);
}

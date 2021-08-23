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

#ifndef APP_ISCHED_H__
#define APP_ISCHED_H__

#include "nrf_queue.h"

/**
* @defgroup isched Instantiable scheduler
* @{
* @ingroup other
* @brief Instantiable scheduler functions.
*/

// Forward declaration of __app_isched_struct.
struct __app_isched_struct;

/**@brief Instantiable scheduler instance.
 *  You can run several instances of this scheduler at the same time.
 *  Note that the instantiable scheduler stores only pointers, not actual objects.
 */
typedef struct __app_isched_struct app_isched_t;

/**@brief Instantiable scheduler event handler. */
typedef void (*app_isched_event_handler_t)(void *p_context);

/**@brief Structure representing an event. */
typedef struct
{
    app_isched_event_handler_t  handler;    /**< Event handler. */
    void                       *p_context;  /**< Pointer to the event context. */
} app_isched_event_t;

/**@brief Instantiable scheduler hook. */
typedef void (*app_isched_hook_t)(const app_isched_t *p_isched,
                                  const app_isched_event_t *p_evt,
                                  void *p_hook_context);

/**@brief Instantiable scheduler hook types. */
typedef enum
{
    APP_ISCHED_HOOK_PRE_PUT,        /**< This hook will be called before putting an event into queue. */
    APP_ISCHED_HOOK_POST_PUT,       /**< This hook will be called after putting an event into queue. */
    APP_ISCHED_HOOK_PRE_EXEC,       /**< This hook will be called before event execution. */
    APP_ISCHED_HOOK_POST_EXEC,      /**< This hook will be called after event execution. */

    // This must be the last value.
    APP_ISCHED_HOOKS_COUNT,
} app_isched_hook_type_t;

/**@brief app_isched_t data structure */
struct __app_isched_struct
{
    const nrf_queue_t   *p_event_queue;                             /**< Event queue. */
    app_isched_hook_t   hooks[APP_ISCHED_HOOKS_COUNT];              /**< Hooks. */
    void                *p_hook_contexts[APP_ISCHED_HOOKS_COUNT];   /**< Hook contexts. */
};

/**@brief Initialize a named scheduler instance.
 *
 * @note    This macro reserves memory for the given pool instance.
 *          Use it only once per each app_balloc_pool instance.
 *
 * @param[in]   _name           Name of the scheduler storage.
 * @param[in]   _isched         Pointer to the app_isched instance that is to be initialized.
 * @param[in]   _queue_size     Maximum number of events in the execution queue.
 */
#define APP_ISCHED_INIT_NAMED(_name, _isched, _queue_size)                                          \
    do {                                                                                            \
        NRF_QUEUE_DEF(app_isched_event_t, _name##_queue, _queue_size, NRF_QUEUE_MODE_NO_OVERFLOW);  \
        (_isched)->p_event_queue = &_name##_queue;                                                  \
                                                                                                    \
        for (unsigned int i = 0; i < APP_ISCHED_HOOKS_COUNT; i++)                                   \
        {                                                                                           \
            (_isched)->hooks[i] = NULL;                                                             \
        }                                                                                           \
    } while (0)

/**@brief Initialize a scheduler instance.
 *
 * @note    This macro reserves memory for the given pool instance.
 *          Use it only once per each app_balloc_pool instance.
 *
 * @param[in]   _isched         Pointer to the app_isched instance that is to be initialized.
 * @param[in]   _queue_size     Maximum number of events in the execution queue.
 */
#define APP_ISCHED_INIT(_isched, _queue_size)                                                   \
        APP_ISCHED_INIT_NAMED(/* empty */, _isched, _queue_size)

/**@brief Put an event in the scheduler instance.
 *
 * @param[in]   p_isched    Pointer to the scheduler instance.
 * @param[in]   handler     Event handler that will receive the event.
 * @param[in]   p_context   Pointer to the event context which will be passed to the event handler.
 *
 * @return  NRF_SUCCESS on success, otherwise error code.
 */
ret_code_t app_isched_event_put(app_isched_t *p_isched,
                                app_isched_event_handler_t handler,
                                void *p_context);

/**@brief Execute events queued in the given scheduler.
 *
 * @param[in]   p_isched    Pointer to the scheduler instance.
 *
 * @return  NRF_SUCCESS on success, otherwise error code.
 */
ret_code_t app_isched_events_execute(app_isched_t *p_isched);

/**@brief Install app_isched hook.
 *
 * @param[in]   p_isched        Pointer to the scheduler instance.
 * @param[in]   hook_type       Hook type.
 * @param[in]   hook            Hook or NULL if previously set hook should be removed.
 * @param[in]   p_hook_context  Context, which will be passed to hook.
 *
 * @return  NRF_SUCCESS on success, otherwise error code.
 */
ret_code_t app_isched_install_hook(app_isched_t *p_isched,
                                   app_isched_hook_type_t hook_type,
                                   app_isched_hook_t hook,
                                   void *p_hook_context);

/**@brief Get maximum scheduler utilization.
 *
 * @param[in]   p_isched    Pointer to the scheduler instance.
 *
 * @return Maximum number of events stored in the scheduler.
 */
size_t app_isched_get_max_utilization(const app_isched_t *p_isched);

#endif // APP_ISCHED_H__

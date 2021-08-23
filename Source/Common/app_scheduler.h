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

#ifndef APP_SCHEDULER_H__
#define APP_SCHEDULER_H__

#include <stdint.h>
#include "nrf_balloc.h"
#include "app_error.h"
#include "app_isched.h"

/**
 * @defgroup app_scheduler Custom implementation of app_scheduler
 * @{
 * @ingroup other
 * @brief A custom implementation of the app_scheduler that replaces the SDK app_scheduler.
 */

/**@brief Scheduler event handler type. */
typedef void (*app_sched_event_handler_t)(void *p_event_data, uint16_t event_data_size);

/**@brief Scheduler event. */
typedef struct
{
    app_sched_event_handler_t   handler;                /**< Event handler. */
    unsigned int                event_data_size;        /**< Size of data attached to event. */

    // This field must be the last one in the struture.
    uint32_t                    event_data[1];          /**< Buffer for the event data. */
} app_sched_event_t;

/**@brief Macro that calculates the size of event structure for the given amount of attached data. */
#define APP_SCHED_EVT_SIZE(_event_data_size)                                                    \
    ALIGN_NUM(sizeof(uint32_t), (offsetof(app_sched_event_t, event_data) + (_event_data_size)))

/**@brief Initialize SDK scheduler emulation.
 *
 * @param[in]   _isched                 Instance of app_isched used to manage app_scheduler events.
 * @param[in]   _max_event_data_size    Maximum size of data attached to event.
 * @param[in]   _pool_size              Event pool size.
 */
#define APP_SCHED_INIT(_isched, _max_event_data_size, _pool_size)                               \
    do {                                                                                        \
        NRF_BALLOC_DEF(__app_scheduler_event_pool,                                              \
                       APP_SCHED_EVT_SIZE(_max_event_data_size),                                \
                       _pool_size);                                                             \
                                                                                                \
        APP_ERROR_CHECK(nrf_balloc_init(&__app_scheduler_event_pool));                          \
        APP_ERROR_CHECK(app_sched_init((_isched),                                               \
                                       &__app_scheduler_event_pool,                             \
                                       (_max_event_data_size)));                                \
    } while (0)

/**@brief Initialize SDK scheduler emmulation.
 *
 * @note    This is internal function. Use of APP_SCHED_INIT() macro is recommended.
 *
 * @param[in]   p_isched                Instance of app_isched used to manage app_scheduler events.
 * @param[in]   p_pool                  Instance of app_balloc_pool used to store events.
 * @param[in]   max_event_data_size     Maximum size of data attached to event.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
uint32_t app_sched_init(app_isched_t *p_isched,
                        const nrf_balloc_t *p_pool,
                        unsigned int max_event_data_size);

/**@brief Function for scheduling an event.
 *
  * @details Puts an event into the event queue.
 *
 * @param[in]   p_event_data   Pointer to event data that is to be scheduled.
 * @param[in]   event_size   Size of event data that is to be scheduled.
 * @param[in]   handler        Event handler that will receive the event.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
uint32_t app_sched_event_put(void *                    p_event_data,
                             uint16_t                  event_size,
                             app_sched_event_handler_t handler);

/**@brief Get maximum scheduler utilization.
 *
 * @return Maximum number of events stored in the scheduler.
 */
uint8_t app_sched_get_max_utilization(void);

#endif // APP_SCHEDULER_H__

/** @} */

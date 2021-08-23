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

/**
 * @defgroup EVENT_BUS Event bus
 * @ingroup other
 * @{
 * @brief Details of the event bus mechanism and its functions.
 */
#ifndef __EVENT_BUS_H__
#define __EVENT_BUS_H__

#include <stdint.h>
#include <stdbool.h>
#include "sdk_errors.h"

// Enable anonymous unions.
#if defined(__CC_ARM)
    #pragma push
    #pragma anon_unions
#elif defined(__ICCAR_)
    #pragma language=save
    #pragma language=extended
#elif defined(__GNUC__)
    // Anonymous unions are supported by default.
#endif

/**@brief Translate event type to event group. */
#define EVENT_GROUP(_event_type)     (event_group_t)((unsigned long)(_event_type) >> 8)

/**@brief Event groups. */
typedef enum
{
    EVT_GROUP_SYSTEM,       /**< System state events. */
    EVT_GROUP_KEY,          /**< Keyboard input events. */
    EVT_GROUP_REL,          /**< Relative motion events. */
    EVT_GROUP_HID,          /**< HID report events. */
    EVT_GROUP_IR,           /**< IR symbol events. */
    EVT_GROUP_BT,           /**< Bluetooth events. */
    EVT_GROUP_ATVV,         /**< ATVV events. */
} event_group_t;

/**@brief Event types. */
typedef enum
{
    /* System Events */
    EVT_SYSTEM_BATTERY_LEVEL    = (EVT_GROUP_SYSTEM << 8),
    EVT_SYSTEM_GYRO_STATE,

    /* Keyboard input events */
    EVT_KEY_UP                  = (EVT_GROUP_KEY << 8),
    EVT_KEY_DOWN,
    EVT_KEY_HELD,

    /* Relative motion events */
    EVT_REL_X                   = (EVT_GROUP_REL << 8),
    EVT_REL_Y,
    EVT_REL_WHEEL,
    EVT_REL_PAN,

    /* HID report events */
    EVT_HID_REPORT_INPUT        = (EVT_GROUP_HID << 8),
    EVT_HID_REPORT_OUTPUT,
    EVT_HID_REPORT_FEATURE,

    /* IR symbol */
    EVT_IR_SYMBOL               = (EVT_GROUP_IR << 8),

    /* Bluetooth events */
    EVT_BT_CONN_STATE           = (EVT_GROUP_BT << 8),
    EVT_BT_ADV_STATE,
    EVT_BT_ALERT_LEVEL,
    EVT_BT_DFU_STATE,

    /* ATVV events */
    EVT_ATVV_STATE              = (EVT_GROUP_ATVV << 8),
} event_type_t;

typedef struct
{
    event_type_t type;

    union {
        // Valid for EVT_GROUP_SYSTEM.
        struct {
            uint8_t     data;
        } system;

        // Valid for EVT_GROUP_KEY.
        struct {
            uint16_t    id;
            uint32_t    down_timestamp;
        } key;

        // Valid for EVT_GROUP_REL.
        struct {
            int16_t     shift;
        } rel;

        // Valid for EVT_GROUP_HID,
        struct {
            uint32_t    usage;
            int16_t     report;
        } hid;

        // Valid for EVT_GROUP_IR,
        struct {
            const sr3_ir_symbol_t   *p_symbol;
        } ir;

        // Valid for EVT_GROUP_BT,
        struct {
            uint8_t     data;
            uint16_t    conn_id;
            uint16_t    peer_id;
        } bt;

        // Valid for EVT_GROUP_ATVV
        struct {
            uint8_t     type;
            uint8_t     rate_khz;
            uint16_t    conn_id;
            uint16_t    peer_id;
        } atvv;
    };
} event_t;

#if defined(__CC_ARM)
    #pragma pop
#elif defined(__ICCAR_)
    #pragma language=restore
#elif defined(__GNUC__)
    // Anonymous unions are supported by default.
#endif

// EVT_BT_CONN_STATE data
#define BT_CONN_STATE_DISCONNECTED      0
#define BT_CONN_STATE_CONNECTED         1
#define BT_CONN_STATE_SECURED           2

// EVT_BT_ADV_STATE data
#define BT_ADV_STATE_IDLE               0
#define BT_ADV_STATE_ACTIVE             1
#define BT_ADV_STATE_ACTIVE_BONDABLE    2

// EVT_BT_DFU_STATE data
#define BT_DFU_STATE_STARTED            0
#define BT_DFU_STATE_FINISHED           1
#define BT_DFU_STATE_ERROR              2

// EVT_ATVV_STATE type
#define EVT_ATVV_STATE_ENABLED          0 /**< ATVV state: open */
#define EVT_ATVV_STATE_DISABLED         1 /**< ATVV state: disabled */
#define EVT_ATVV_STATE_MIC_OPEN         2 /**< ATVV state: microphone open */
#define EVT_ATVV_STATE_MIC_CLOSE        3 /**< ATVV state: microphone closed */
#define EVT_ATVV_STATE_SEARCH_TIMEOUT   4 /**< ATVV state: search timeout */

/**@brief Event handler.
 *
 * @param[in]   p_event Pointer to the event structure.
 *
 * @return      True if further processing of the given event should be abandoned.
 */
typedef bool (*event_handler_t)(const event_t *p_event);

/**@brief Function for initializing the event bus.
 *
 * @param[in]   event_handlers_table    Table with event handlers.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
ret_code_t event_bus_init(const event_handler_t *event_handlers_table);

/**@brief Function for sending an event.
 *
 * @param[in]   event_type  Event type.
 * @param[in]   ...         Event data.
 *
 * @note                    Event data elements are mapped one to one to data fields in the event structure.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
ret_code_t event_send(event_type_t event_type, ...);

#endif /** __EVENT_BUS_H__ */

/**
  @}
*/

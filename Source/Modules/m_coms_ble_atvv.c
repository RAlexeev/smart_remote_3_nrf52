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

#include "m_coms_ble_atvv.h"

#include <stdarg.h>

#include "app_timer.h"
#include "ble_conn_state.h"
#include "m_coms_ble_atvv_srv.h"
#include "nrf_queue.h"
#include "peer_manager.h"
#include "resources.h"
#include "sr3_config.h"

#if CONFIG_AUDIO_ATVV_ENABLED

#if (CONFIG_AUDIO_CODEC != CONFIG_AUDIO_CODEC_ADPCM) || \
    (CONFIG_AUDIO_SAMPLING_FREQUENCY != 8000)        || \
    (CONFIG_AUDIO_FRAME_SIZE_SAMPLES != 256)
#error ATVV v0.4 only supports 8 kHz ADPCM with 256 sample frame size
#endif

#if CONFIG_AUDIO_ATVV_FRAME_FRAG_LEN != 20
#error ATVV v0.4 only 20 bytes frame fragment size
#endif

#if (CONFIG_KBD_KEY_COMBO_ENABLED != 1) && (CONFIG_KBD_ATVV_KEY_CHORDS_ENABLED == 1)
#error Detection of key combinations must be enabled for Android TV Voice pairing key chords to work
#endif

#define NRF_LOG_MODULE_NAME m_coms_ble_atvv
#define NRF_LOG_LEVEL       CONFIG_BLE_ATVV_LOG_LEVEL
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

#define ATVV_SEARCH_OPEN_TIMEOUT APP_TIMER_TICKS(CONFIG_AUDIO_ATVV_SEARCH_TIMEOUT) /* Timeout for 'start search' message. Used to e.g. re-enable slave latency */
#define ATVV_MIC_OPEN_TIMEOUT1   APP_TIMER_TICKS(ATVV_TIMEOUT_1_MIC_OPEN) /* Triggers when audio data has not been generated after mic open */
#define ATVV_MIC_OPEN_TIMEOUT2   APP_TIMER_TICKS(ATVV_TIMEOUT_2_STREAMING_DURATION - ATVV_TIMEOUT_1_MIC_OPEN) /* Used to limit maximum streaming duration. */

BLE_ATVV_DEF(m_atvv);
APP_TIMER_DEF(m_mic_timer);

/**@brief ADPCM frame header specified in ATVV specification. */
typedef PACKED_STRUCT
{
    uint8_t sequence_number[2];       /**< Frame sequence number. */
    uint8_t id;                       /**< Remote control ID. Not used in the current spec version. */
    int16_t prev_pred;                /**< Previously predicted ADPCM value. */
    uint8_t index;                    /**< Index in ADPCM step size table. */
} atvv_adpcm_frame_header;

/**@brief ATVV connection states. */
typedef enum
{
    MIC_STATE_CLOSED,                 /**< Peer has not requested audio data. */
    MIC_STATE_CLOSING,                /**< Peer has requested microphone close while a frame is in transit. */
    MIC_STATE_OPEN_IDLE,              /**< Peer has requested audio data, but the AUDIO_START message has not yet been sent. */
    MIC_STATE_OPEN_WAITING,           /**< Waiting for the AUDIO_START message to be transmitted. */
    MIC_STATE_OPEN_ACTIVE,            /**< Audio actively transmitted. */
    MIC_STATE_INVALID,                /**< Invalid state marker. */
} atvv_mic_state_t;

/**@brief ATVV connection instance. */
typedef struct
{
    uint16_t         conn_handle;     /**< Reference to an applicable connection. */
    uint16_t         frame_counter;   /**< Counter used for sync messages. */
    bool             frame_in_transit;/**< True when a frame is currently partially transmitted. */
    uint8_t          sampling_rate;   /**< Audio sampling rate [kHz]. */
    atvv_mic_state_t state;           /**< State of an instance. */
} atvv_instance_t;

/**@brief Queued ATVV command. */
typedef struct
{
    struct
    {
        ble_atvv_ctl_type_t type;     /**< Control message type. */
        void *              p_param;  /**< Applicable control message parameter. */
    } ctl_msg;
    atvv_instance_t * p_atvv_instance;/**< Pointer to an applicable instance. */
    atvv_mic_state_t  new_state;      /**< New state that should be applied to the instance. @ref MIC_STATE_INVALID is not applied.  */
} atvv_ctl_queue_item_t;

static ret_code_t m_coms_ble_atvv_state_update(atvv_instance_t * p_instance,
                                               atvv_mic_state_t  new_state);

static atvv_instance_t m_instances[CONFIG_AUDIO_ATVV_PEER_NUM];

NRF_QUEUE_DEF(atvv_ctl_queue_item_t,
              atvv_ctl_queue,
              CONFIG_AUDIO_ATVV_CTL_MSG_QUEUE_LEN,
              NRF_QUEUE_MODE_NO_OVERFLOW);

/**@brief Send a control message to the peer, either directly or through a queue. */
static ret_code_t m_coms_ble_atvv_ctl_msg_send(
    atvv_instance_t   * p_instance,
    atvv_mic_state_t    new_state,
    bool                enqueue,
    ble_atvv_ctl_type_t type, ...)
{
    va_list args;
    void  * p_param;

    va_start(args, type);

    switch (type)
    {
        case BLE_ATVV_CTL_MIC_OPEN_ERROR:
            p_param = (void*) va_arg(args, int);
            break;

        case BLE_ATVV_CTL_AUDIO_SYNC:
            p_param = (void*) va_arg(args, uint32_t);
            break;

        default:
            // No parameters for other control message types
            break;
    }

    va_end(args);

    if (!enqueue)
    {
        ret_code_t err_code;

        err_code = ble_atvv_ctl_send(&m_atvv, p_instance->conn_handle, type, p_param);
        if ((err_code == NRF_SUCCESS) && (new_state != MIC_STATE_INVALID))
        {
            APP_ERROR_CHECK(m_coms_ble_atvv_state_update(p_instance, new_state));
        }

        return err_code;
    }

    atvv_ctl_queue_item_t queue_item =
    {
        .ctl_msg.type    = type,
        .ctl_msg.p_param = p_param,
        .p_atvv_instance = p_instance,
        .new_state       = new_state
    };

    return nrf_queue_push(&atvv_ctl_queue, &queue_item);
}

/**@brief Find an instance that corresponds to the connection handle. */
static ret_code_t m_coms_ble_atvv_instance_find(uint16_t conn_handle, atvv_instance_t ** pp_instance)
{
    for (size_t i = 0; i < ARRAY_SIZE(m_instances); ++i)
    {
        if (m_instances[i].conn_handle == conn_handle)
        {
            *pp_instance = &m_instances[i];

            return NRF_SUCCESS;
        }
    }

    return NRF_ERROR_NOT_FOUND;
}

/**@brief Find and allocate a free instance. */
static ret_code_t m_coms_ble_atvv_instance_allocate(uint16_t conn_handle)
{
    bool already_allocated = false;
    int  instance_idx      = -1;

    for (size_t i = 0; i < ARRAY_SIZE(m_instances); ++i)
    {
        if (m_instances[i].conn_handle == conn_handle)
        {
            already_allocated = true;
            instance_idx      = i;
            break;
        }

        if (m_instances[i].conn_handle == BLE_CONN_HANDLE_INVALID)
        {
            instance_idx = i;
        }
    }

    if (already_allocated)
    {
        if (m_instances[instance_idx].state != MIC_STATE_CLOSED)
        {
            NRF_LOG_WARNING("Unexpected allocation by conn_handle %d", conn_handle);
        }

        return NRF_SUCCESS;
    }

    if (instance_idx != -1)
    {
        m_instances[instance_idx].conn_handle      = conn_handle;
        m_instances[instance_idx].state            = MIC_STATE_CLOSED;
        m_instances[instance_idx].frame_in_transit = false;
        m_instances[instance_idx].frame_counter    = 0;

        return NRF_SUCCESS;
    }

    return NRF_ERROR_NO_MEM;
}

/**@brief Find and free/deallocate an instance. */
static void m_coms_ble_atvv_instance_free(uint16_t conn_handle)
{
    for (size_t i = 0; i < ARRAY_SIZE(m_instances); ++i)
    {
        if (m_instances[i].conn_handle == conn_handle)
        {
            m_instances[i].conn_handle = BLE_CONN_HANDLE_INVALID;
        }
    }
}

/**@brief Utility function used to trigger a queue transmission.
 *        Useful if there is no outside activity to drive the queue processing.
 *
 * @note  Upon successful transmission, queued control messages can update the instance state.
 */
static void m_coms_ble_atvv_ctl_queue_ping(atvv_instance_t * p_instance)
{
    if ((p_instance->state == MIC_STATE_CLOSED) ||
        (p_instance->state == MIC_STATE_OPEN_IDLE))
    {
        // No audio activity to drive m_coms data queues
        // Sending the control message manually, which triggers further processing of the queue

        (void) m_coms_ble_atvv_ctl_pkt_queue_process();
    }
}

/**@brief Send an ATVV event to the event bus. */
static void m_coms_ble_atvv_send_event(uint8_t  type,
                                       uint16_t conn_handle,
                                       uint8_t  sampling_rate_khz)
{
    pm_peer_id_t peer_id;

    if (pm_peer_id_get(conn_handle, &peer_id) != NRF_SUCCESS)
    {
        peer_id = PM_PEER_ID_INVALID;
    }

    APP_ERROR_CHECK(event_send(EVT_ATVV_STATE, type, sampling_rate_khz, conn_handle, peer_id));
}

/**@brief ATVV state processing function. */
static ret_code_t m_coms_ble_atvv_state_update(atvv_instance_t * p_instance,
                                               atvv_mic_state_t  new_state)
{
    ret_code_t err_code;
    uint8_t    evt;

    if (p_instance->state == new_state)
    {
        return NRF_SUCCESS;
    }

    err_code = NRF_SUCCESS;
    evt      = (uint8_t) -1;

    switch (p_instance->state)
    {
        case MIC_STATE_CLOSED:
            switch (new_state)
            {
                case MIC_STATE_OPEN_IDLE:
                    p_instance->state            = MIC_STATE_OPEN_IDLE;
                    p_instance->frame_counter    = 0;
                    p_instance->frame_in_transit = false;
                    evt                          = EVT_ATVV_STATE_MIC_OPEN;

                    err_code = app_timer_stop(m_mic_timer);
                    if (err_code != NRF_SUCCESS)
                    {
                        return err_code;
                    }
                    err_code = app_timer_start(m_mic_timer, ATVV_MIC_OPEN_TIMEOUT1, (void*) p_instance);

                    break;

                default:
                    err_code = NRF_ERROR_INVALID_STATE;
                    break;
            }
            break;

        case MIC_STATE_CLOSING:
            switch (new_state)
            {
                case MIC_STATE_CLOSED:
                    p_instance->state = MIC_STATE_CLOSED;
                    evt               = EVT_ATVV_STATE_MIC_CLOSE;

                    err_code = m_coms_ble_atvv_ctl_msg_send(p_instance,
                                                            MIC_STATE_INVALID,
                                                            true,
                                                            BLE_ATVV_CTL_AUDIO_STOP);
                    break;

                default:
                    err_code = NRF_ERROR_INVALID_STATE;
                    break;
            }
            break;

        case MIC_STATE_OPEN_IDLE:
            switch (new_state)
            {
                case MIC_STATE_OPEN_ACTIVE:
                    /* Fall through */
                case MIC_STATE_OPEN_WAITING:
                    p_instance->state = MIC_STATE_OPEN_WAITING;
                    err_code = m_coms_ble_atvv_ctl_msg_send(p_instance,
                                                            MIC_STATE_OPEN_ACTIVE,
                                                            true,
                                                            BLE_ATVV_CTL_AUDIO_START);
                    break;

                case MIC_STATE_CLOSED:
                    /* Fall through */
                case MIC_STATE_CLOSING:
                    p_instance->state = MIC_STATE_CLOSED;
                    evt               = EVT_ATVV_STATE_MIC_CLOSE;
                    err_code = app_timer_stop(m_mic_timer);
                    break;

                default:
                    err_code = NRF_ERROR_INVALID_STATE;
                    break;
            }
            break;

        case MIC_STATE_OPEN_WAITING:
            switch (new_state)
            {
                case MIC_STATE_CLOSED:
                    /* Fall through */
                case MIC_STATE_CLOSING:
                    p_instance->state = MIC_STATE_CLOSING;
                    err_code = m_coms_ble_atvv_ctl_msg_send(p_instance,
                                                            MIC_STATE_CLOSED,
                                                            true,
                                                            BLE_ATVV_CTL_AUDIO_STOP);
                    if (err_code != NRF_SUCCESS)
                    {
                        return err_code;
                    }
                    err_code = app_timer_stop(m_mic_timer);
                    break;

                case MIC_STATE_OPEN_ACTIVE:
                    p_instance->state = MIC_STATE_OPEN_ACTIVE;

                    err_code = app_timer_stop(m_mic_timer);
                    if (err_code != NRF_SUCCESS)
                    {
                        return err_code;
                    }
                    err_code = app_timer_start(m_mic_timer, ATVV_MIC_OPEN_TIMEOUT2, (void*) p_instance);
                    break;

                default:
                    err_code = NRF_ERROR_INVALID_STATE;
                    break;
            }
            break;

        case MIC_STATE_OPEN_ACTIVE:
            switch (new_state)
            {
                case MIC_STATE_CLOSED:
                    /* Fall through */
                case MIC_STATE_CLOSING:
                    if (!p_instance->frame_in_transit)
                    {
                        p_instance->state = MIC_STATE_CLOSED;
                        evt               = EVT_ATVV_STATE_MIC_CLOSE;
                        err_code = m_coms_ble_atvv_ctl_msg_send(p_instance,
                                                                MIC_STATE_INVALID,
                                                                true,
                                                                BLE_ATVV_CTL_AUDIO_STOP);
                        if (err_code != NRF_SUCCESS)
                        {
                            return err_code;
                        }

                    }
                    else
                    {
                        p_instance->state = MIC_STATE_CLOSING;
                    }
                    err_code = app_timer_stop(m_mic_timer);
                    break;

                default:
                    err_code = NRF_ERROR_INVALID_STATE;
                    break;
            }
            break;

        default:
            err_code = NRF_ERROR_INVALID_STATE;
            break;
    }

    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    if (evt != (uint8_t) -1)
    {
        m_coms_ble_atvv_send_event(evt, p_instance->conn_handle, p_instance->sampling_rate);
    }

    m_coms_ble_atvv_ctl_queue_ping(p_instance);

    return NRF_SUCCESS;
}

/**@brief Handle events from the ATVV Service. */
static void m_coms_atvv_srv_event_handler(ble_atvv_evt_t const * p_evt)
{
    atvv_instance_t          * p_instance;
    ret_code_t                 err_code;
    uint16_t                   sampling_rate = 0;

    if (p_evt->type != BLE_ATVV_EVT_ENABLED)
    {
        err_code = m_coms_ble_atvv_instance_find(p_evt->conn_handle, &p_instance);
        if (err_code != NRF_SUCCESS)
        {
            NRF_LOG_DEBUG("No ATVV instance for this connection");
            return;
        }
    }

    switch (p_evt->type)
    {
        case BLE_ATVV_EVT_ENABLED:
            NRF_LOG_DEBUG("BLE_ATVV_EVT_ENABLED");
            err_code = m_coms_ble_atvv_instance_allocate(p_evt->conn_handle);
            if (err_code == NRF_ERROR_NO_MEM)
            {
                NRF_LOG_WARNING("Insufficient memory for a new ATVV instance");
                return;
            }
            else
            {
                APP_ERROR_CHECK(err_code);

                m_coms_ble_atvv_send_event(EVT_ATVV_STATE_ENABLED, p_evt->conn_handle, sampling_rate);
            }
            break;

        case BLE_ATVV_EVT_DISABLED:
            NRF_LOG_DEBUG("BLE_ATVV_EVT_DISABLED");

            m_coms_ble_atvv_instance_free(p_evt->conn_handle);

            m_coms_ble_atvv_send_event(EVT_ATVV_STATE_DISABLED, p_evt->conn_handle, sampling_rate);
            break;

        case BLE_ATVV_EVT_MIC_OPEN:
            NRF_LOG_DEBUG("BLE_ATVV_EVT_MIC_OPEN");
            NRF_LOG_DEBUG("Codec used: 0x%04x", p_evt->params.mic_open.codec);

            if (p_instance->state != MIC_STATE_CLOSED)
            {
                // This or another instance already opened
                NRF_LOG_DEBUG("Mic already open");
                APP_ERROR_CHECK(m_coms_ble_atvv_ctl_msg_send(p_instance,
                                                             MIC_STATE_INVALID,
                                                             true,
                                                             BLE_ATVV_CTL_MIC_OPEN_ERROR,
                                                             (int)BLE_ATVV_ERROR_INVALID_STATE));
                return;
            }

            if (p_evt->params.mic_open.codec != BLE_ATVV_USED_CODEC_ADPCM_8KHZ)
            {
                APP_ERROR_CHECK(m_coms_ble_atvv_ctl_msg_send(p_instance,
                                                             MIC_STATE_INVALID,
                                                             true,
                                                             BLE_ATVV_CTL_MIC_OPEN_ERROR,
                                                             (int)BLE_ATVV_ERROR_INVALID_CODEC));
                m_coms_ble_atvv_ctl_queue_ping(p_instance);
                return;
            }
            else
            {
                p_instance->sampling_rate = 8;
            }

            APP_ERROR_CHECK(m_coms_ble_atvv_state_update(p_instance, MIC_STATE_OPEN_IDLE));
            break;

        case BLE_ATVV_EVT_MIC_CLOSE:
            NRF_LOG_DEBUG("BLE_ATVV_EVT_MIC_CLOSE");

            APP_ERROR_CHECK(m_coms_ble_atvv_state_update(p_instance, MIC_STATE_CLOSED));
            break;

        case BLE_ATVV_EVT_GET_CAPS:
            NRF_LOG_DEBUG("BLE_ATVV_EVT_GET_CAPS");
            NRF_LOG_DEBUG("Peer ATVV version: 0x%04x", p_evt->params.get_caps.peer_version);
            NRF_LOG_DEBUG("Peer ATVV codec support: 0x%04x", p_evt->params.get_caps.peer_codec_support);

            APP_ERROR_CHECK(m_coms_ble_atvv_ctl_msg_send(p_instance, MIC_STATE_INVALID, true, BLE_ATVV_CTL_GET_CAPS_RESP));
            m_coms_ble_atvv_ctl_queue_ping(p_instance);
            return;

        default:
            NRF_LOG_WARNING("Unknown ATVV event");
            return;
    }
}

/**@brief app_timer time-out handler for ATVV time-outs. */
static void m_coms_atvv_mic_timeout_handler(void * p_context)
{
    atvv_instance_t * p_instance;

    p_instance = (atvv_instance_t *) p_context;

    switch (p_instance->state)
    {
        case MIC_STATE_CLOSED:
            // Peer has not started streaming a reasonable time after a start search message
            NRF_LOG_DEBUG("ATVV search time-out");
            m_coms_ble_atvv_send_event(EVT_ATVV_STATE_SEARCH_TIMEOUT, p_instance->conn_handle, 0);
            break;

        case MIC_STATE_OPEN_IDLE:
            // Time-out 1 = We have not started streaming audio fast enough after the micopen command was received
            NRF_LOG_DEBUG("ATVV time-out1");
            APP_ERROR_CHECK(m_coms_ble_atvv_ctl_msg_send(p_instance,
                                                         MIC_STATE_CLOSED,
                                                         true,
                                                         BLE_ATVV_CTL_MIC_OPEN_ERROR,
                                                         (int)BLE_ATVV_ERROR_MIC_OPEN_TIMEOUT));
            break;

        case MIC_STATE_OPEN_ACTIVE:
            // Time-out 2 = Peer has not sent a close mic command within the specified time limit
            NRF_LOG_DEBUG("ATVV time-out2");
            APP_ERROR_CHECK(m_coms_ble_atvv_state_update(p_instance, MIC_STATE_CLOSED));
            break;

        default:
            // Corner-case: mic time-out event is already in the scheduler queue when application logic stops the timer
            break;
    }
}

ret_code_t m_coms_ble_atvv_init(void)
{
    ret_code_t err_code;

    for (size_t i = 0; i < ARRAY_SIZE(m_instances); ++i)
    {
        m_instances[i].conn_handle = BLE_CONN_HANDLE_INVALID;
    }

    err_code = app_timer_create(&m_mic_timer,
                                APP_TIMER_MODE_SINGLE_SHOT,
                                m_coms_atvv_mic_timeout_handler);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    };

    return ble_atvv_init(&m_atvv, m_coms_atvv_srv_event_handler);
}

ret_code_t m_coms_ble_atvv_start_search(uint16_t conn_handle)
{
    atvv_instance_t * p_instance;
    ret_code_t        err_code;

    if (conn_handle == BLE_CONN_HANDLE_INVALID)
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    err_code = m_coms_ble_atvv_instance_find(conn_handle, &p_instance);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    };

    if (p_instance->state != MIC_STATE_CLOSED)
    {
        return NRF_ERROR_INVALID_STATE;
    }

    err_code = m_coms_ble_atvv_ctl_msg_send(p_instance, MIC_STATE_INVALID, true, BLE_ATVV_CTL_START_SEARCH);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    };

    err_code = app_timer_stop(m_mic_timer);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    };

    err_code = app_timer_start(m_mic_timer, ATVV_SEARCH_OPEN_TIMEOUT, (void*) p_instance);

    m_coms_ble_atvv_ctl_queue_ping(p_instance);

    return err_code;
}

ret_code_t m_coms_ble_atvv_audio_send(uint8_t const                * p_frame_buf,
                                      uint16_t                       frame_buf_len,
                                      uint16_t                     * p_bytes_transmitted,
                                      m_coms_data_process_status_t * p_status)
{
    atvv_instance_t * p_instance;
    ret_code_t        err_code;
    uint16_t          frame_bytes_sent;

    ASSERT(p_frame_buf != 0);
    ASSERT(frame_buf_len <= ATVV_CAPS_FRAME_SIZE_DEFAULT_134);

    // Find the active instance (only one active instance is currently supported)
    // NOTE: This should eventually be replaced by explicitly providing conn_handle as the function argument
    p_instance = 0;
    for (size_t i = 0; i < ARRAY_SIZE(m_instances); ++i)
    {
        if ((m_instances[i].conn_handle != BLE_CONN_HANDLE_INVALID) &&
            (m_instances[i].state       != MIC_STATE_CLOSED))
        {
            p_instance = &m_instances[i];
            break;
        }
    }

    if (p_instance == 0)
    {
        *p_status = M_COMS_STATUS_CANNOT_SEND;
        return NRF_SUCCESS;
    };
    if (p_instance->state == MIC_STATE_CLOSED)
    {
        *p_status = M_COMS_STATUS_CANNOT_SEND;
        return NRF_SUCCESS;
    };

    if (p_instance->state == MIC_STATE_OPEN_IDLE)
    {
        // The "audio start" message must be sent before audio data
        err_code = m_coms_ble_atvv_state_update(p_instance, MIC_STATE_OPEN_ACTIVE);
        if (err_code != NRF_SUCCESS)
        {
            return err_code;
        }
        else
        {
            *p_status = M_COMS_STATUS_CANNOT_SEND;
            return NRF_SUCCESS;
        }
    }

    if (!p_instance->frame_in_transit)
    {
        atvv_adpcm_frame_header * header;
        uint8_t                   frame_packet[CONFIG_AUDIO_ATVV_FRAME_FRAG_LEN];

        // First packet of frame: Header needs to be populated
        // ADPCM frame already comes with state information from the codec driver. Only must add "seq number" and "Id"
        // ADPCM state information is 3 bytes: 16-bit prediction, and 8-bit index

        header            = (atvv_adpcm_frame_header *) frame_packet;
        header->id        = CONFIG_AUDIO_ATVV_REMOTE_CONTROL_ID;
        header->prev_pred = *(int16_t*) &p_frame_buf[0];
        header->index     = *(uint8_t*) &p_frame_buf[2];
        uint16_big_encode(p_instance->frame_counter, header->sequence_number);

        frame_bytes_sent = (CONFIG_AUDIO_ATVV_FRAME_FRAG_LEN - sizeof(atvv_adpcm_frame_header) + 3);

        // Fill the rest of the packet with frame contents
        memcpy(&frame_packet[sizeof(atvv_adpcm_frame_header)],
               &p_frame_buf[3],
               CONFIG_AUDIO_ATVV_FRAME_FRAG_LEN - sizeof(atvv_adpcm_frame_header));

        err_code = ble_atvv_frame_data_transmit(&m_atvv,
                                                frame_packet,
                                                CONFIG_AUDIO_ATVV_FRAME_FRAG_LEN,
                                                p_instance->conn_handle);

    }
    else
    {
        // Partial frame remaining
        frame_bytes_sent = (frame_buf_len >= CONFIG_AUDIO_ATVV_FRAME_FRAG_LEN) ?
            CONFIG_AUDIO_ATVV_FRAME_FRAG_LEN : frame_buf_len;

        err_code = ble_atvv_frame_data_transmit(&m_atvv,
                                                p_frame_buf,
                                                frame_bytes_sent,
                                                p_instance->conn_handle);
    }

    switch (err_code)
    {
        case NRF_SUCCESS:
            *p_status                    = M_COMS_STATUS_SUCCESS;
            *p_bytes_transmitted         = frame_bytes_sent;
            p_instance->frame_in_transit = (frame_buf_len > frame_bytes_sent);
            break;

        case BLE_ERROR_GATTS_SYS_ATTR_MISSING:
            /* Fall through */
        case NRF_ERROR_INVALID_STATE:
            /* Fall through */
        case NRF_ERROR_BUSY:
            /* Fall through */
        case NRF_ERROR_FORBIDDEN:
            *p_status = M_COMS_STATUS_CANNOT_SEND;
            return NRF_SUCCESS;

        case NRF_ERROR_RESOURCES:
            *p_status = M_COMS_STATUS_SD_BUFFER_FULL;
            return NRF_SUCCESS;

        default:
            return err_code;
    }

    if (p_instance->frame_in_transit)
    {
        return NRF_SUCCESS;
    }

    if (p_instance->state == MIC_STATE_CLOSING)
    {
        err_code = m_coms_ble_atvv_state_update(p_instance, MIC_STATE_CLOSED);
    }

    p_instance->frame_counter += 1;

    if ((p_instance->frame_counter % CONFIG_AUDIO_ATVV_SYNC_INTERVAL) == 0)
    {
        // Won't enqueue or retry audio sync message because it will get stale quickly
        (void) m_coms_ble_atvv_ctl_msg_send(p_instance, MIC_STATE_INVALID, false, BLE_ATVV_CTL_AUDIO_SYNC, p_instance->frame_counter);
    }

    return err_code;
}

ret_code_t m_coms_ble_atvv_ctl_pkt_queue_process(void)
{
    ret_code_t            err_code;
    atvv_ctl_queue_item_t queue_item;

    err_code = nrf_queue_peek(&atvv_ctl_queue, &queue_item);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    };

    err_code = ble_atvv_ctl_send(&m_atvv,
                                 queue_item.p_atvv_instance->conn_handle,
                                 queue_item.ctl_msg.type,
                                 queue_item.ctl_msg.p_param);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    };

    err_code = nrf_queue_pop(&atvv_ctl_queue, &queue_item);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    };

    if (queue_item.new_state != MIC_STATE_INVALID)
    {
        err_code = m_coms_ble_atvv_state_update(queue_item.p_atvv_instance, queue_item.new_state);
    }

    return err_code;
}

#endif /* CONFIG_AUDIO_ATVV_ENABLED */

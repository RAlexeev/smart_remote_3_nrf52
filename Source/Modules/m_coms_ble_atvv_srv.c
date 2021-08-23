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

#include "m_coms_ble_atvv_srv.h"

#include "ble.h"
#include "ble_srv_common.h"
#include "sdk_common.h"

#if CONFIG_AUDIO_ATVV_ENABLED

#define NRF_LOG_MODULE_NAME m_coms_ble_atvv_srv
#define NRF_LOG_LEVEL       CONFIG_BLE_ATVV_LOG_LEVEL
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

#if CONFIG_AUDIO_CODEC == CONFIG_AUDIO_CODEC_ADPCM
#define ATVV_FRAME_HEADER_SIZE 6
#define ATVV_FRAME_SIZE        (ATVV_FRAME_HEADER_SIZE + \
                               ((CONFIG_AUDIO_FRAME_SIZE_SAMPLES * sizeof(int16_t)) / 4))
#define ATVV_CODECS_SUPPORTED  ATVV_CAPS_SUPP_CODEC_ADPCM_8KHZ
#else
#error ATVV only supports ADPCM codec
#endif /* CONFIG_AUDIO_CODEC == CONFIG_AUDIO_CODEC_ADPCM */

#define ATVV_NOTIF_CHAR_COUNT 2 /* Number of characteristics in ATVV Service that supports Notifications */

#define ATVV_CMD_LEN_GET_CAPS 5
#define ATVV_CMD_LEN_OPEN     3
#define ATVV_CMD_LEN_CLOSE    1

/**@brief Format of the AUDIO_STOP control message. */
typedef PACKED_STRUCT
{
    uint8_t ctl_type;            /**< Control type value. */
} ble_atvv_ctl_audio_stop_t;

/**@brief Format of the AUDIO_SART control message. */
typedef PACKED_STRUCT
{
    uint8_t ctl_type;            /**< Control type value. */
} ble_atvv_ctl_audio_start_t;

/**@brief Format of the START_SEARCH control message. */
typedef PACKED_STRUCT
{
    uint8_t ctl_type;            /**< Control type value. */
} ble_atvv_ctl_start_search_t;

/**@brief Format of the AUDIO_SYNC control message. */
typedef PACKED_STRUCT
{
    uint8_t ctl_type;            /**< Control type value. */
    uint8_t frame_num[2];        /**< 16-bit frame number (big endian). */
} ble_atvv_ctl_audio_sync_t;

/**@brief Format of the GET_CAPS_RESP control message. */
typedef PACKED_STRUCT
{
    uint8_t ctl_type;            /**< Control type value. */
    uint8_t version[2];          /**< 16-bit version value ([major, minor], big endian). */
    uint8_t codecs_supported[2]; /**< 16-bit codec support bitmap (big endian). */
    uint8_t bytes_per_frame[2];  /**< 16-bit frame size (big endian). */
    uint8_t bytes_per_char[2];   /**< 16-bit characteristic/chunk size (big endian). */
} ble_atvv_ctl_caps_resp_t;

/**@brief Format of the MIC_OPEN_ERROR control message. */
typedef PACKED_STRUCT
{
    uint8_t ctl_type;            /**< Control type value. */
    uint8_t error_code[2];       /**< 16-bit error code (big endian). */
} ble_atvv_ctl_mic_open_error_t;

/**@brief Convenience union to keep track of control message sizes. */
typedef union
{
    ble_atvv_ctl_audio_stop_t     audio_stop;
    ble_atvv_ctl_audio_start_t    audio_start;
    ble_atvv_ctl_start_search_t   start_search;
    ble_atvv_ctl_audio_sync_t     audio_sync;
    ble_atvv_ctl_caps_resp_t      get_caps_resp;
    ble_atvv_ctl_mic_open_error_t mic_open_error;
} ble_atvv_ctl_t;

/**@brief Function for parsing a command value written to ATVV_CHAR_TX. */
static ret_code_t ble_atvv_cmd_val_parse(uint8_t const * p_cmd, uint16_t len, ble_atvv_evt_t * p_evt)
{
    if (len == 0)
    {
        return NRF_ERROR_DATA_SIZE;
    }

    switch (p_cmd[0])
    {
        case ATVV_CMD_GET_CAPS:
            if (len != ATVV_CMD_LEN_GET_CAPS)
            {
                return NRF_ERROR_DATA_SIZE;
            }
            p_evt->type = BLE_ATVV_EVT_GET_CAPS;
            p_evt->params.get_caps.peer_version       = uint16_big_decode(&p_cmd[1]);
            p_evt->params.get_caps.peer_codec_support = (ble_atvv_supported_codecs_t) uint16_big_decode(&p_cmd[3]);
            break;

        case ATVV_CMD_MIC_OPEN:
            p_evt->type = BLE_ATVV_EVT_MIC_OPEN;
            if (len == ATVV_CMD_LEN_OPEN)
            {
                p_evt->params.mic_open.codec = (ble_atvv_used_codec_t) uint16_big_decode(&p_cmd[1]);
            }
            else
            {
                NRF_LOG_WARNING("ATVV_CMD_MIC_OPEN format error. Assuming ADPCM 8 kHz.");
                p_evt->params.mic_open.codec = BLE_ATVV_USED_CODEC_ADPCM_8KHZ;
            }

            break;

        case ATVV_CMD_MIC_CLOSE:
            if (len != ATVV_CMD_LEN_CLOSE)
            {
                return NRF_ERROR_DATA_SIZE;
            }
            p_evt->type = BLE_ATVV_EVT_MIC_CLOSE;
            break;

        default:
            return NRF_ERROR_DATA_SIZE;
    }

    return NRF_SUCCESS;
}

/**@brief Function for sending notifications via RX or CTL characteristics. */
static ret_code_t ble_atvv_send_notification(uint8_t const * p_data, uint16_t len, uint16_t value_handle, uint16_t conn_handle)
{
    ret_code_t             err_code;
    uint16_t               data_len = len;
    ble_gatts_hvx_params_t hvx =
    {
        .handle = value_handle,
        .type   = BLE_GATT_HVX_NOTIFICATION,
        .offset = 0,
        .p_len  = &data_len,
        .p_data = p_data
    };

    err_code = sd_ble_gatts_hvx(conn_handle, &hvx);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    };

    return data_len == len ? NRF_SUCCESS : NRF_ERROR_INVALID_LENGTH;
}

/**@brief Utility function for checking whether RX and CTL characteristics have Notifications enabled. */
static ret_code_t ble_atvv_check_cccds_enabled(ble_atvv_t const * p_atvv,
                                      uint8_t          * p_enabled_cccd_count,
                                      uint16_t           conn_handle)
{
    ret_code_t        err_code;
    uint16_t          cccd_value;
    ble_gatts_value_t cccd =
    {
        .len     = sizeof(cccd_value),
        .p_value = (uint8_t*)&cccd_value,
        .offset  = 0
    };

    *p_enabled_cccd_count = 0;

    err_code = sd_ble_gatts_value_get(conn_handle,
                                      p_atvv->rx_handles.cccd_handle,
                                      &cccd);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    };

    *p_enabled_cccd_count += (cccd_value == 0x0001);

    err_code = sd_ble_gatts_value_get(conn_handle,
                                      p_atvv->ctl_handles.cccd_handle,
                                      &cccd);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    };

    *p_enabled_cccd_count += (cccd_value == 0x0001);

    return NRF_SUCCESS;
}

/**@brief Function for handling the @ref BLE_GAP_EVT_CONNECTED event from the SoftDevice.
 *
 * @param[in] p_atvv    ATVV Service structure.
 * @param[in] p_ble_evt Pointer to the event received from the BLE stack.
 */
static void ble_atvv_on_connected(ble_atvv_t const * p_atvv, ble_evt_t const * p_ble_evt)
{
    ret_code_t err_code;
    uint8_t    cccds_enabled_count;

    err_code = ble_atvv_check_cccds_enabled(p_atvv, &cccds_enabled_count, p_ble_evt->evt.gap_evt.conn_handle);
    if (err_code != NRF_SUCCESS)
    {
        return;
    }

    if (cccds_enabled_count == ATVV_NOTIF_CHAR_COUNT)
    {
        ble_atvv_evt_t evt;

        // Connected to a paired host and CCCDs are already enabled
        evt.type        = BLE_ATVV_EVT_ENABLED;
        evt.p_atvv      = p_atvv;
        evt.conn_handle = p_ble_evt->evt.gap_evt.conn_handle;

        p_atvv->event_handler(&evt);
    }
}

/**@brief Function for handling the @ref BLE_GAP_EVT_DISCONNECTED event from the SoftDevice.
 *
 * @param[in] p_atvv    ATVV Service structure.
 * @param[in] p_ble_evt Pointer to the event received from the BLE stack.
 */
static void ble_atvv_on_disconnected(ble_atvv_t const * p_atvv, ble_evt_t const * p_ble_evt)
{
    ret_code_t err_code;
    uint8_t    cccds_enabled_count;

    err_code = ble_atvv_check_cccds_enabled(p_atvv, &cccds_enabled_count, p_ble_evt->evt.gap_evt.conn_handle);
    if (err_code != NRF_SUCCESS)
    {
        return;
    }

    if (cccds_enabled_count == ATVV_NOTIF_CHAR_COUNT)
    {
        ble_atvv_evt_t evt;

        // Previously enabled -> disable
        evt.type        = BLE_ATVV_EVT_DISABLED;
        evt.p_atvv      = p_atvv;
        evt.conn_handle = p_ble_evt->evt.gap_evt.conn_handle;

        p_atvv->event_handler(&evt);
    }
}

/**@brief Function for handling the @ref BLE_GATTS_EVT_WRITE event from the SoftDevice.
 *
 * @param[in] p_atvv    ATVV Service structure.
 * @param[in] p_ble_evt Pointer to the event received from the BLE stack.
 */
static void ble_atvv_on_write(ble_atvv_t const * p_atvv, ble_evt_t const * p_ble_evt)
{
    ble_gatts_evt_write_t const * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;
    uint16_t                      conn_handle = p_ble_evt->evt.gatts_evt.conn_handle;
    ble_atvv_evt_t                evt =
    {
        .p_atvv      = p_atvv,
        .conn_handle = conn_handle
    };

    if (p_evt_write->handle == p_atvv->tx_handles.value_handle)
    {
        if (ble_atvv_cmd_val_parse(p_evt_write->data, p_evt_write->len, &evt) == NRF_SUCCESS)
        {
            p_atvv->event_handler(&evt);
        }
        else
        {
            NRF_LOG_WARNING("Invalid command 0x%02x (length %d)", p_evt_write->data[0], p_evt_write->len);
        }
    }
    else if ((p_evt_write->handle == p_atvv->rx_handles.cccd_handle) ||
             (p_evt_write->handle == p_atvv->ctl_handles.cccd_handle))
    {
        ret_code_t err_code;
        uint16_t   write_value;
        uint8_t    cccds_enabled_count;

        APP_ERROR_CHECK_BOOL(p_evt_write->len == 2); /* 2 = length of CCCD */
        memcpy(&write_value, p_evt_write->data, p_evt_write->len);

        err_code = ble_atvv_check_cccds_enabled(p_atvv, &cccds_enabled_count, conn_handle);
        if (err_code != NRF_SUCCESS)
        {
            return;
        }

        if (cccds_enabled_count == ATVV_NOTIF_CHAR_COUNT)
        {
            // All CCCDs enabled
            evt.type = BLE_ATVV_EVT_ENABLED;

            p_atvv->event_handler(&evt);
        }
        else if ((write_value == 0x0000) && (cccds_enabled_count == (ATVV_NOTIF_CHAR_COUNT - 1)))
        {
            // All CCCDs no longer enabled
            evt.type = BLE_ATVV_EVT_DISABLED;

            p_atvv->event_handler(&evt);
        }
    }
}

/**@brief Function for adding Characteristics with the Notify property.
 *
 * @param[in] p_atvv  ATVV Service structure.
 * @param[in] uuid    16-bit Characteristic UUID.
 * @param[in] max_len Length of the Characteristic.
 *
 * @return NRF_SUCCESS on success, otherwise an error code.
 */
static ret_code_t ble_atvv_notify_char_add(ble_atvv_t * p_atvv, ble_gatts_char_handles_t * p_handle, uint16_t uuid, uint16_t max_len)
{
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_md_t cccd_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          ble_uuid;
    ble_gatts_attr_md_t attr_md;

    memset(&cccd_md, 0, sizeof(cccd_md));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);

    cccd_md.vloc = BLE_GATTS_VLOC_STACK;

    memset(&char_md, 0, sizeof(char_md));

    char_md.char_props.notify = 1;
    char_md.char_props.read   = 1;
    char_md.p_cccd_md         = &cccd_md;

    ble_uuid.type = p_atvv->uuid_type;
    ble_uuid.uuid = uuid;

    memset(&attr_md, 0, sizeof(attr_md));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&attr_md.write_perm);

    attr_md.vloc    = BLE_GATTS_VLOC_STACK;
    attr_md.vlen    = 1;

    memset(&attr_char_value, 0, sizeof(attr_char_value));

    attr_char_value.p_uuid    = &ble_uuid;
    attr_char_value.p_attr_md = &attr_md;
    attr_char_value.init_len  = sizeof(uint8_t);
    attr_char_value.max_len   = max_len;

    return sd_ble_gatts_characteristic_add(p_atvv->service_handle,
                                           &char_md,
                                           &attr_char_value,
                                           p_handle);
}


/**@brief Function for adding an RX characteristic.
 *
 * @param[in] p_atvv ATVV Service structure.
 *
 * @return NRF_SUCCESS on success, otherwise an error code.
 */
static ret_code_t ble_atvv_tx_char_add(ble_atvv_t * p_atvv)
{
    ble_gatts_char_md_t char_md;
    ble_uuid_t          ble_uuid;
    ble_gatts_attr_md_t attr_md;
    ble_gatts_attr_t    attr_char_value;

    memset(&char_md, 0, sizeof(char_md));

    char_md.char_props.write = 1;

    ble_uuid.type = p_atvv->uuid_type;
    ble_uuid.uuid = BLE_UUID_ATV_TX_CHARACTERISTIC;

    memset(&attr_md, 0, sizeof(attr_md));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);

    attr_md.vloc    = BLE_GATTS_VLOC_STACK;
    attr_md.vlen    = 1;

    memset(&attr_char_value, 0, sizeof(attr_char_value));

    attr_char_value.p_uuid    = &ble_uuid;
    attr_char_value.p_attr_md = &attr_md;
    attr_char_value.init_len  = 1;
    attr_char_value.max_len   = BLE_ATVV_CHAR_TX_LEN;

    return sd_ble_gatts_characteristic_add(p_atvv->service_handle,
                                           &char_md,
                                           &attr_char_value,
                                           &p_atvv->tx_handles);
}


void ble_atvv_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context)
{
    if ((p_context == NULL) || (p_ble_evt == NULL))
    {
        return;
    }

    ble_atvv_t * p_atvv = (ble_atvv_t *)p_context;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            ble_atvv_on_connected(p_atvv, p_ble_evt);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            ble_atvv_on_disconnected(p_atvv, p_ble_evt);
            break;

        case BLE_GATTS_EVT_WRITE:
            ble_atvv_on_write(p_atvv, p_ble_evt);
            break;

        default:
            break;
    }
}


ret_code_t ble_atvv_init(ble_atvv_t * p_atvv, ble_atvv_evt_handler_t p_evt_handler)
{
    ret_code_t    err_code;
    ble_uuid128_t atvv_base_uuid = BLE_UUID_ATV_BASE;

    if ((p_atvv == 0) || (p_evt_handler == 0))
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    p_atvv->event_handler = p_evt_handler;

    // Add a custom base UUID.
    err_code = sd_ble_uuid_vs_add(&atvv_base_uuid, &p_atvv->uuid_type);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    };

    ble_uuid_t ble_uuid =
    {
        .type = p_atvv->uuid_type,
        .uuid = BLE_UUID_ATV_SERVICE
    };

    // Add the service.
    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                        &ble_uuid,
                                        &p_atvv->service_handle);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    };

    // Add ATVV_CHAR_TX
    err_code = ble_atvv_tx_char_add(p_atvv);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    };

    // Add ATVV_CHAR_RX
    err_code = ble_atvv_notify_char_add(p_atvv, &p_atvv->rx_handles, BLE_UUID_ATV_RX_CHARACTERISTIC, CONFIG_AUDIO_ATVV_FRAME_FRAG_LEN);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    };

    // Add ATVV_CHAR_CTL
    err_code = ble_atvv_notify_char_add(p_atvv, &p_atvv->ctl_handles, BLE_UUID_ATV_CTL_CHARACTERISTIC, BLE_ATVV_CHAR_CTL_LEN);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    };

    return NRF_SUCCESS;
}

ret_code_t ble_atvv_ctl_send(ble_atvv_t const * p_atvv, uint16_t conn_handle, ble_atvv_ctl_type_t type,  void * p_param)
{
    ble_atvv_ctl_t ctl;
    uint16_t       ctl_len;

    ASSERT(p_atvv != 0);

    switch (type)
    {
        case BLE_ATVV_CTL_AUDIO_SYNC:
            ASSERT(p_param != 0);

            ctl_len                 = sizeof(ctl.audio_sync);
            ctl.audio_sync.ctl_type = BLE_ATVV_CTL_AUDIO_SYNC;

            uint16_big_encode((uint16_t)((0xFFFF & (uint32_t) p_param)), ctl.audio_sync.frame_num);
            break;

        case BLE_ATVV_CTL_GET_CAPS_RESP:
            ctl_len                    = sizeof(ctl.get_caps_resp);
            ctl.get_caps_resp.ctl_type = BLE_ATVV_CTL_GET_CAPS_RESP;

            uint16_big_encode(((ATVV_VERSION_MAJOR << 8) | ATVV_VERSION_MINOR), ctl.get_caps_resp.version);
            uint16_big_encode((uint16_t) ATVV_CODECS_SUPPORTED,                 ctl.get_caps_resp.codecs_supported);
            uint16_big_encode((uint16_t) ATVV_FRAME_SIZE,                       ctl.get_caps_resp.bytes_per_frame);
            uint16_big_encode((uint16_t) CONFIG_AUDIO_ATVV_FRAME_FRAG_LEN,      ctl.get_caps_resp.bytes_per_char);
            break;

        case BLE_ATVV_CTL_MIC_OPEN_ERROR:
            ASSERT(p_param != 0);

            ctl_len                     = sizeof(ctl.mic_open_error);
            ctl.mic_open_error.ctl_type = BLE_ATVV_CTL_MIC_OPEN_ERROR;

            uint16_big_encode((uint16_t)(0xFFFF & ((uint32_t) p_param)), ctl.mic_open_error.error_code);
            break;

        case BLE_ATVV_CTL_AUDIO_STOP:
            ctl_len                 = sizeof(ctl.audio_stop);
            ctl.audio_stop.ctl_type = BLE_ATVV_CTL_AUDIO_STOP;
            break;

        case BLE_ATVV_CTL_AUDIO_START:
            ctl_len                  = sizeof(ctl.audio_start);
            ctl.audio_start.ctl_type = BLE_ATVV_CTL_AUDIO_START;
            break;

        case BLE_ATVV_CTL_START_SEARCH:
            ctl_len                   = sizeof(ctl.start_search);
            ctl.start_search.ctl_type = BLE_ATVV_CTL_START_SEARCH;
            break;

        default:
            return NRF_ERROR_INVALID_PARAM;
    }

    return ble_atvv_send_notification((uint8_t*) &ctl, ctl_len, p_atvv->ctl_handles.value_handle, conn_handle);
}

ret_code_t ble_atvv_frame_data_transmit(ble_atvv_t const * p_atvv, uint8_t const * p_frame_data, uint16_t len, uint16_t conn_handle)
{
    if (len > CONFIG_AUDIO_ATVV_FRAME_FRAG_LEN)
    {
        return NRF_ERROR_INVALID_LENGTH;
    }

    return ble_atvv_send_notification(p_frame_data, len, p_atvv->rx_handles.value_handle, conn_handle);
}

#endif /* CONFIG_AUDIO_ATVV_ENABLED */

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

/**@file
 * DFU Service.
 */

#include "nrf_atomic.h"
#include "nrf_fstorage.h"
#include "nrf_fstorage_sd.h"
#include "nrf_pwr_mgmt.h"
#include "peer_manager.h"
#include "crc32.h"

#include "m_coms_ble_dfu.h"
#include "resources.h"
#include "sr3_config.h"

#include "sr3_bootloader_commands.h"
#include "bootloader/dfu/nrf_dfu_handling_error.h"
#include "bootloader/dfu/nrf_dfu_types.h"

#if CONFIG_DFU_ENABLED

#define NRF_LOG_MODULE_NAME m_coms_ble_dfu
#define NRF_LOG_LEVEL       CONFIG_BLE_DFU_LOG_LEVEL
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();


#if !(CONFIG_PWR_MGMT_ENABLED)
# error "The DFU requires power management module."
#endif

/**@brief UUID the of DFU Service. */
#define BLE_DFU_SERVICE_UUID                0xFE59

/**@brief UUID of the DFU Buttonless control point (with bond sharing). */
#define BLE_DFU_BUTTONLESS_BONDED_CHAR_UUID 0x0004

/* Positions of fields in the DFU response message. */
enum
{
    DFU_RESPONSE_FIELD_POS_TYPE,
    DFU_RESPONSE_FIELD_POS_OP_CODE,
    DFU_RESPONSE_FIELD_POS_RESULT_CODE,

    DFU_RESPONSE_LENGTH
};

/**@brief Buttonless DFU operation codes. */
typedef enum
{
    DFU_OP_RESERVED                         = 0x00, /**< Reserved for future use. */
    DFU_OP_ENTER_BOOTLOADER                 = 0x01, /**< Enter bootloader. */
    DFU_OP_SET_ADV_NAME                     = 0x02, /**< Set advertisement name to use in DFU mode. */
    DFU_OP_RESPONSE_CODE                    = 0x20  /**< Response code. */
} ble_dfu_buttonless_op_code_t;

/**@brief Buttonless DFU response codes. */
typedef enum
{
    DFU_RSP_INVALID                         = 0x00,                                         /**< Invalid op code. */
    DFU_RSP_SUCCESS                         = 0x01,                                         /**< Success. */
    DFU_RSP_OP_CODE_NOT_SUPPORTED           = 0x02,                                         /**< Op code not supported. */
    DFU_RSP_OPERATION_FAILED                = 0x04,                                         /**< Operation failed. */
    DFU_RSP_ADV_NAME_INVALID                = 0x05,                                         /**< Requested advertisement name is too short or too long. */
    DFU_RSP_BUSY                            = 0x06,                                         /**< Ongoing async operation. */
    DFU_RSP_NOT_BONDED                      = 0x07,                                         /**< Buttonless unavailable due to device not bonded. */
    DFU_RSP_CCCD_CONFIG_IMPROPER            = BLE_GATT_STATUS_ATTERR_CPS_CCCD_CONFIG_ERROR  /**< CCCD is improperly configured. */
} ble_dfu_buttonless_rsp_code_t;

__ALIGN(4) static nrf_dfu_settings_t        m_dfu_settings;             /**< Local copy of DFU settings. */
static uint16_t                             m_dfu_conn_handle;          /**< Connection requesting DFU. */
static ble_gatts_char_handles_t             m_dfu_ctrl_char_handles;    /**< DFU Buttonless Control Characteristic handles. */
static nrf_atomic_flag_t                    m_dfu_flash_busy;           /**< Busy flag set if module waits for flash update. */
static bool                                 m_dfu_shutdown_requested;   /**< True if module shutdown was requested. */

/**@brief fstorage event handler. */
static void m_coms_ble_dfu_fs_evt_handler(nrf_fstorage_evt_t *p_evt);

/**@brief fstorage instance for accessing DFU settings page. */
NRF_FSTORAGE_DEF(nrf_fstorage_t m_coms_ble_dfu_fs) =
{
    .evt_handler    = m_coms_ble_dfu_fs_evt_handler,
    .start_addr     = BOOTLOADER_SETTINGS_ADDRESS,
    .end_addr       = BOOTLOADER_SETTINGS_ADDRESS + CODE_PAGE_SIZE - 1
};


/**@brief Calculate CRC of the DFU settings. */
static uint32_t m_coms_ble_dfu_calculate_dfu_settings_crc(const nrf_dfu_settings_t *p_dfu_settings)
{
    return crc32_compute((uint8_t *)(p_dfu_settings) + sizeof(p_dfu_settings->crc),
                         offsetof(nrf_dfu_settings_t, init_command) - sizeof(p_dfu_settings->crc),
                         NULL);
}

/**@brief Calculate CRC of the peer data block. */
static uint32_t m_coms_ble_dfu_calculate_shared_data_crc(const nrf_dfu_shared_data_t *p_shared_data)
{
    return crc32_compute((uint8_t *)(p_shared_data) + sizeof(p_shared_data->crc),
                         sizeof(*p_shared_data) - sizeof(p_shared_data->crc),
                         NULL);
}

/**@brief Send DFU response. */
static ret_code_t m_coms_ble_dfu_send_response(uint16_t                      conn_handle,
                                               ble_dfu_buttonless_op_code_t  op_code,
                                               ble_dfu_buttonless_rsp_code_t response_code)
{
    uint8_t response[DFU_RESPONSE_LENGTH];

    /* DFU_RSP_OPERATION_FAILED is larger than 8 bits, however it is cast to uint8_t to maintain compatibility. */
    response[DFU_RESPONSE_FIELD_POS_TYPE]        = DFU_OP_RESPONSE_CODE;
    response[DFU_RESPONSE_FIELD_POS_OP_CODE]     = (uint8_t)(op_code);
    response[DFU_RESPONSE_FIELD_POS_RESULT_CODE] = (uint8_t)(response_code);

    uint16_t response_len = sizeof(response);

    ble_gatts_hvx_params_t hvx_params = { 0 };

    hvx_params.handle = m_dfu_ctrl_char_handles.value_handle,
    hvx_params.type   = BLE_GATT_HVX_INDICATION;
    hvx_params.p_data = response;
    hvx_params.p_len  = &response_len;

    return sd_ble_gatts_hvx(conn_handle, &hvx_params);
}

/**@brief fstorage event handler. */
static void m_coms_ble_dfu_fs_evt_handler(nrf_fstorage_evt_t *p_evt)
{
    switch (p_evt->id)
    {
        case NRF_FSTORAGE_EVT_ERASE_RESULT:
            APP_ERROR_CHECK(p_evt->result);
            APP_ERROR_CHECK(nrf_fstorage_write(&m_coms_ble_dfu_fs,
                                               BOOTLOADER_SETTINGS_ADDRESS,
                                               &m_dfu_settings,
                                               ALIGN_NUM(sizeof(uint32_t), offsetof(nrf_dfu_settings_t, init_command)),
                                               NULL));
            break;

        case NRF_FSTORAGE_EVT_WRITE_RESULT:
            if (p_evt->addr == (BOOTLOADER_SETTINGS_ADDRESS + offsetof(nrf_dfu_settings_t, shared_data)))
            {
                /*
                 * Check if peer data write was successful and host is still connected.
                 * Enter DFU if everything is OK, otherwise try to recover.
                 */
                if (m_dfu_conn_handle != BLE_CONN_HANDLE_INVALID)
                {
                    ble_dfu_buttonless_rsp_code_t rsp_code = (p_evt->result == NRF_SUCCESS) ? DFU_RSP_SUCCESS :
                                                                                              DFU_RSP_OPERATION_FAILED;

                    ret_code_t status = m_coms_ble_dfu_send_response(m_dfu_conn_handle, DFU_OP_ENTER_BOOTLOADER, rsp_code);
                    if ((status == NRF_SUCCESS) && (p_evt->result == NRF_SUCCESS))
                    {
                        status = sd_power_gpregret_clr(0, 0xFF);
                        if (status == NRF_SUCCESS)
                        {
                            status = sd_power_gpregret_set(0, BOOTLOADER_DFU_START);
                        }

                        if (status == NRF_SUCCESS)
                        {
                            NRF_LOG_INFO("Peer data stored. Entering DFU...");
                            nrf_atomic_flag_clear(&m_dfu_flash_busy);
                            nrf_pwr_mgmt_shutdown((m_dfu_shutdown_requested) ? NRF_PWR_MGMT_SHUTDOWN_CONTINUE :
                                                                               NRF_PWR_MGMT_SHUTDOWN_GOTO_DFU);
                            return;
                        }

                        /* Cannot start DFU mode. Settings will be updated. */
                    }
                }

                m_dfu_conn_handle = BLE_CONN_HANDLE_INVALID;

                if (p_evt->result == NRF_SUCCESS)
                {
                    /* Something went wrong. Settings page has to be rewritten. */
                    NRF_LOG_WARNING("%s(): Could not enter DFU mode. Starting recovery...", __func__);
                    APP_ERROR_CHECK(nrf_fstorage_erase(&m_coms_ble_dfu_fs,
                                                       BOOTLOADER_SETTINGS_ADDRESS,
                                                       1,
                                                       NULL));
                }
                else
                {
                    NRF_LOG_WARNING("%s(): Could not enter DFU mode. FLASH operation failed.", __func__);
                    nrf_atomic_flag_clear(&m_dfu_flash_busy);
                }

            }
            else
            {
                /* Check if new DFU settings were written correctly. */
                APP_ERROR_CHECK(p_evt->result);
                NRF_LOG_INFO("DFU settings updated.");

                nrf_atomic_flag_clear(&m_dfu_flash_busy);
                if (m_dfu_shutdown_requested)
                {
                    nrf_pwr_mgmt_shutdown(NRF_PWR_MGMT_SHUTDOWN_CONTINUE);
                }
            }
            break;

        default:
            ASSERT(false);
            break;
    }
}

/**@brief Authorize DFU characteristic write request. */
static bool m_coms_ble_dfu_authorize_write(const ble_evt_t *p_ble_evt)
{
    ble_gatts_rw_authorize_reply_params_t       auth_reply = { 0 };
    const ble_gatts_evt_rw_authorize_request_t  *p_authorize_request;
    const ble_gatts_evt_write_t                 *p_ble_write_evt;

    p_authorize_request = &(p_ble_evt->evt.gatts_evt.params.authorize_request);
    p_ble_write_evt = &(p_ble_evt->evt.gatts_evt.params.authorize_request.request.write);

    if ((p_authorize_request->type == BLE_GATTS_AUTHORIZE_TYPE_WRITE)                       &&
        (p_authorize_request->request.write.handle == m_dfu_ctrl_char_handles.value_handle) &&
        (p_authorize_request->request.write.op != BLE_GATTS_OP_PREP_WRITE_REQ)              &&
        (p_authorize_request->request.write.op != BLE_GATTS_OP_EXEC_WRITE_REQ_NOW)          &&
        (p_authorize_request->request.write.op != BLE_GATTS_OP_EXEC_WRITE_REQ_CANCEL)        )
    {
        uint8_t           cccd_value_buffer[BLE_CCCD_VALUE_LEN];
        ble_gatts_value_t cccd_value = { 0 };
        ret_code_t        status;

        auth_reply.type                 = BLE_GATTS_AUTHORIZE_TYPE_WRITE;
        auth_reply.params.write.update  = 1;
        auth_reply.params.write.offset  = p_ble_write_evt->offset;
        auth_reply.params.write.len     = p_ble_write_evt->len;
        auth_reply.params.write.p_data  = p_ble_write_evt->data;

        /* Check if CCCD is configured on the DFU Buttonless Control Characteristic. */
        cccd_value.p_value = cccd_value_buffer;
        cccd_value.len     = sizeof(cccd_value_buffer);

        status = sd_ble_gatts_value_get(p_ble_evt->evt.gatts_evt.conn_handle,
                                        m_dfu_ctrl_char_handles.cccd_handle,
                                        &cccd_value);

        if ((status == NRF_SUCCESS) && ble_srv_is_indication_enabled(cccd_value.p_value))
        {
            /* Reply success if CCCD is configured. */
            auth_reply.params.write.gatt_status = BLE_GATT_STATUS_SUCCESS;
            status = sd_ble_gatts_rw_authorize_reply(p_ble_evt->evt.gatts_evt.conn_handle,
                                                     &auth_reply);

            return (status == NRF_SUCCESS) ? true : false;
        }

        /* Reply CCCD config error if CCCD is not configured. */
        auth_reply.params.write.gatt_status = DFU_RSP_CCCD_CONFIG_IMPROPER;
        sd_ble_gatts_rw_authorize_reply(p_ble_evt->evt.gatts_evt.conn_handle, &auth_reply);
    }

    return false;
}

/**@brief Store current peer data in DFU settings page. */
static ret_code_t m_coms_ble_dfu_store_shared_data(uint16_t conn_handle, pm_peer_id_t peer_id)
{
    nrf_dfu_shared_data_t *p_shared_data = &m_dfu_settings.shared_data;

    ASSERT(conn_handle != BLE_CONN_HANDLE_INVALID);
    ASSERT(peer_id != PM_PEER_ID_INVALID);

    /* Fetch device address. */
    ret_code_t status = pm_id_addr_get(&p_shared_data->ble_addr);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    /* Fetch bonding data. */
    pm_peer_data_bonding_t bonding_data;
    status = pm_peer_data_bonding_load(peer_id, &bonding_data);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    memcpy(&p_shared_data->peer_data.ble_id,  &bonding_data.peer_ble_id, sizeof(p_shared_data->peer_data.ble_id));
    memcpy(&p_shared_data->peer_data.enc_key, &bonding_data.own_ltk,     sizeof(p_shared_data->peer_data.enc_key));

    /* Fetch system attributes. */
    uint16_t sys_attr_len = sizeof(p_shared_data->peer_data.sys_serv_attr);
    status = sd_ble_gatts_sys_attr_get(conn_handle,
                                       p_shared_data->peer_data.sys_serv_attr,
                                       &sys_attr_len,
                                       BLE_GATTS_SYS_ATTR_FLAG_SYS_SRVCS);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    /* Calculate CRC of the shared data. */
    p_shared_data->crc = m_coms_ble_dfu_calculate_shared_data_crc(p_shared_data);

    /* Schedule DFU settings page update. */
    return nrf_fstorage_write(&m_coms_ble_dfu_fs,
                              BOOTLOADER_SETTINGS_ADDRESS + offsetof(nrf_dfu_settings_t, shared_data),
                              p_shared_data,
                              ALIGN_NUM(sizeof(uint32_t), sizeof(*p_shared_data)),
                              NULL);
}

/**@brief Handle DFU characteristic write. */
static void m_coms_ble_dfu_handle_write(uint16_t conn_handle,
                                        const ble_gatts_evt_write_t *p_ble_write_evt)
{
    ble_dfu_buttonless_rsp_code_t response_code;
    pm_peer_id_t peer_id;

    ASSERT(p_ble_write_evt->len > 0);

    if (p_ble_write_evt->len != 1)
    {
        m_coms_ble_dfu_send_response(conn_handle,
                                     DFU_OP_RESPONSE_CODE,
                                     DFU_RSP_OP_CODE_NOT_SUPPORTED);
        return;
    }

    switch (p_ble_write_evt->data[0])
    {
        case DFU_OP_ENTER_BOOTLOADER:
            if (pm_peer_id_get(conn_handle, &peer_id) != NRF_SUCCESS)
            {
                response_code = DFU_RSP_OPERATION_FAILED;
                break;
            }

            if (peer_id == PM_PEER_ID_INVALID)
            {
                response_code = DFU_RSP_NOT_BONDED;
                break;
            }

            if (nrf_atomic_flag_set_fetch(&m_dfu_flash_busy))
            {
                response_code = DFU_RSP_BUSY;
                break;
            }

            if (m_coms_ble_dfu_store_shared_data(conn_handle, peer_id) != NRF_SUCCESS)
            {
                nrf_atomic_flag_clear(&m_dfu_flash_busy);
                response_code = DFU_RSP_OPERATION_FAILED;
                break;
            }

            NRF_LOG_INFO("DFU mode requested. Storing peer data...");
            m_dfu_conn_handle = conn_handle;
            return;

        default:
            response_code = DFU_RSP_OP_CODE_NOT_SUPPORTED;
            break;
    }

    m_coms_ble_dfu_send_response(conn_handle,
                                 (ble_dfu_buttonless_op_code_t)(p_ble_write_evt->data[0]),
                                 response_code);
}

/**@brief Handle BLE events. */
static void m_coms_ble_dfu_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context)
{
    UNUSED_VARIABLE(p_context);

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST:
            if (m_coms_ble_dfu_authorize_write(p_ble_evt))
            {
                m_coms_ble_dfu_handle_write(p_ble_evt->evt.gatts_evt.conn_handle,
                                            &(p_ble_evt->evt.gatts_evt.params.authorize_request.request.write));
            }
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            if (p_ble_evt->evt.gap_evt.conn_handle == m_dfu_conn_handle)
            {
                m_dfu_conn_handle = BLE_CONN_HANDLE_INVALID;
            }
            break;

        default:
            /* Ignore */
            break;
    }
}
NRF_SDH_BLE_OBSERVER(m_coms_ble_dfu_observer, BLE_OBSERVER_PRIORITY_DEFAULT, m_coms_ble_dfu_on_ble_evt, NULL);

/**@brief Handle power management event. */
static bool m_coms_ble_dfu_shutdown(nrf_pwr_mgmt_evt_t event)
{
    m_dfu_shutdown_requested = true;

    return !m_dfu_flash_busy;
}
NRF_PWR_MGMT_HANDLER_REGISTER(m_coms_ble_dfu_shutdown, SHUTDOWN_PRIORITY_EARLY);


ret_code_t m_coms_ble_dfu_init(const m_coms_ble_params_t *p_ble_params)
{
    static const ble_uuid128_t dfu_base_uuid =
    {
        {
            0x50, 0xEA, 0xDA, 0x30, 0x88, 0x83, 0xB8, 0x9F,
            0x60, 0x4F, 0x15, 0xF3, 0x00, 0x00, 0xC9, 0x8E
        }
    };
    ble_uuid_t          dfu_service_uuid;
    uint16_t            dfu_service_handle;
    uint8_t             dfu_uuid_type;
    ble_gatts_char_md_t dfu_char_md = {{0}};
    ble_gatts_attr_t    dfu_attr    = {0};
    ble_gatts_attr_md_t dfu_attr_md = {{0}};
    ble_gatts_attr_md_t dfu_cccd_md = {{0}};
    ble_uuid_t          dfu_char_uuid;
    ret_code_t          status;

    /* Initialize data structures and read DFU settings. */
    m_dfu_conn_handle        = BLE_CONN_HANDLE_INVALID;
    m_dfu_shutdown_requested = false;

    nrf_atomic_flag_clear(&m_dfu_flash_busy);
    memcpy(&m_dfu_settings, (void *)(BOOTLOADER_SETTINGS_ADDRESS), sizeof(m_dfu_settings));

    /* Initialize fstorage. */
    status = nrf_fstorage_init(&m_coms_ble_dfu_fs, &nrf_fstorage_sd, NULL);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    /*
     * Check if DFU settings are valid and if the CRC verification of the firmware is enabled
     * or there is old peer data.
     */
    if ((m_dfu_settings.crc == m_coms_ble_dfu_calculate_dfu_settings_crc(&m_dfu_settings)) &&
        (m_dfu_settings.bank_0.bank_code == NRF_DFU_BANK_VALID_APP))
    {
        bool update_settings = false;

        if (m_dfu_settings.transport_activated != 0)
        {
            NRF_LOG_INFO("Triggering 'Service Changed' indication...");
            pm_local_database_has_changed();
            m_dfu_settings.transport_activated = 0;
            update_settings = true;
        }

        if (m_dfu_settings.shared_data.crc != 0xFFFFFFFF)
        {
            /*
             * Prepare peer data area for future update.
             * Erase settings page without storing new data.
             */
            update_settings = true;
        }

        if (update_settings)
        {
            /* Start DFU settings update procedure by erasing settings page. */
            NRF_LOG_INFO("Updating DFU settings...");
            m_dfu_settings.crc = m_coms_ble_dfu_calculate_dfu_settings_crc(&m_dfu_settings);
            nrf_atomic_flag_set(&m_dfu_flash_busy);
            status = nrf_fstorage_erase(&m_coms_ble_dfu_fs, BOOTLOADER_SETTINGS_ADDRESS, 1, &m_dfu_settings);
            if (status != NRF_SUCCESS)
            {
                nrf_atomic_flag_clear(&m_dfu_flash_busy);
                return status;
            }
        }
    }

    /* Add DFU service. */
    status = sd_ble_uuid_vs_add(&dfu_base_uuid, &dfu_uuid_type);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    BLE_UUID_BLE_ASSIGN(dfu_service_uuid, BLE_DFU_SERVICE_UUID);
    status = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                      &dfu_service_uuid,
                                      &dfu_service_handle);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    /* Add Buttonless DFU characteristic. */
    dfu_char_md.char_props.write    = 1;
    dfu_char_md.char_props.indicate = 1;
    dfu_char_md.p_cccd_md           = &dfu_cccd_md;

    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&dfu_attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&dfu_cccd_md.read_perm);

    if (p_ble_params->sec_params.io_capabilities == BLE_GAP_IO_CAPS_NONE)
    {
        BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&dfu_attr_md.write_perm);
        BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&dfu_cccd_md.write_perm);
    }
    else
    {
        BLE_GAP_CONN_SEC_MODE_SET_ENC_WITH_MITM(&dfu_attr_md.write_perm);
        BLE_GAP_CONN_SEC_MODE_SET_ENC_WITH_MITM(&dfu_cccd_md.write_perm);
    }

    dfu_char_uuid.type  = dfu_uuid_type;
    dfu_char_uuid.uuid  = BLE_DFU_BUTTONLESS_BONDED_CHAR_UUID;

    dfu_attr_md.vloc    = BLE_GATTS_VLOC_STACK;
    dfu_attr_md.wr_auth = 1;
    dfu_attr_md.vlen    = 1;

    dfu_cccd_md.vloc    = dfu_attr_md.vloc;

    dfu_attr.p_uuid     = &dfu_char_uuid;
    dfu_attr.p_attr_md  = &dfu_attr_md;
    dfu_attr.max_len    = BLE_GATT_ATT_MTU_DEFAULT;
    dfu_attr.p_value    = NULL;

    return sd_ble_gatts_characteristic_add(dfu_service_handle,
                                           &dfu_char_md,
                                           &dfu_attr,
                                           &m_dfu_ctrl_char_handles);
}

#endif /* CONFIG_DFU_ENABLED */

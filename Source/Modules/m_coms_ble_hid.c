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

#include "ble_flash.h"
#include "ble_hids.h"
#include "nrf_assert.h"
#include "nrf_error.h"
#include "nrf_sdh_ble.h"

#include "m_coms_ble_hid.h"
#include "resources.h"
#include "sr3_config.h"

#define NRF_LOG_MODULE_NAME m_coms_ble_hid
#define NRF_LOG_LEVEL       CONFIG_BLE_HID_LOG_LEVEL
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

static m_coms_hid_evt_handler_t   s_evt_handler;
static ble_srv_error_handler_t    s_error_handler;
static ble_hid_db_t const *       s_hid_db;
static ble_hids_hid_information_t s_hid_info;
static bool                       s_with_mitm;

static void on_hids_evt(ble_hids_t * p_hids, ble_hids_evt_t * p_evt)
{
    m_coms_ble_hid_evt_t evt;

    evt.interface_idx = 0xFF;
    evt.report_idx    = 0xFF;
    evt.report_type   = 0xFF;

    // Translating from ble_hids_t* to interface index.
    for (size_t i = 0; i < s_hid_db->report_maps_size; ++i)
    {
        if (p_hids == s_hid_db->report_maps[i].interface)
        {
            evt.interface_idx = i;
            break;
        }
    }

    APP_ERROR_CHECK_BOOL(evt.interface_idx != 0xFF);

    evt.hids_evt = p_evt;
    evt.len      = 0;

    if (p_evt->evt_type == BLE_HIDS_EVT_REP_CHAR_WRITE)
    {
        evt.report_idx  = p_evt->params.char_write.char_id.rep_index;
        evt.report_type =  p_evt->params.char_write.char_id.rep_type;
        evt.data        = p_evt->params.char_write.data;
        evt.len         = p_evt->params.char_write.len;
    }
    else if (p_evt->evt_type == BLE_HIDS_EVT_REPORT_READ)
    {
        evt.report_idx = p_evt->params.char_auth_read.char_id.rep_index;
        evt.report_type = p_evt->params.char_auth_read.char_id.rep_type;
    }

    s_evt_handler(&evt);
}

static void security_mode_cccd_set(ble_srv_cccd_security_mode_t * p_sec_mode, bool p_mitm_use)
{
    if (p_mitm_use)
    {
        BLE_GAP_CONN_SEC_MODE_SET_ENC_WITH_MITM(&p_sec_mode->cccd_write_perm);
        BLE_GAP_CONN_SEC_MODE_SET_ENC_WITH_MITM(&p_sec_mode->read_perm);
        BLE_GAP_CONN_SEC_MODE_SET_ENC_WITH_MITM(&p_sec_mode->write_perm);
    }
    else
    {
        BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&p_sec_mode->cccd_write_perm);
        BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&p_sec_mode->read_perm);
        BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&p_sec_mode->write_perm);
    }
}

static void security_mode_set(ble_srv_security_mode_t * p_sec_mode, bool p_read, bool p_write, bool p_mitm_use)
{
    if (p_read)
    {
        if (p_mitm_use)
        {
            BLE_GAP_CONN_SEC_MODE_SET_ENC_WITH_MITM(&p_sec_mode->read_perm);
        }
        else
        {
            BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&p_sec_mode->read_perm);
        }
    }
    else
    {
        BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&p_sec_mode->read_perm);
    }

    if (p_write)
    {
        if (p_mitm_use)
        {
            BLE_GAP_CONN_SEC_MODE_SET_ENC_WITH_MITM(&p_sec_mode->write_perm);
        }
        else
        {
            BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&p_sec_mode->write_perm);
        }
    }
    else
    {
        BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&p_sec_mode->write_perm);
    }
}

uint32_t m_coms_ble_hid_init(const m_coms_ble_hid_init_t * p_params,
                             uint16_t *p_last_input_report_cccd_handle)
{
    uint32_t                    err_code;
    ble_hids_init_t             hids_init_obj;
    ble_hids_inp_rep_init_t     inp_reps[BLE_HIDS_MAX_INPUT_REP];
    ble_hids_outp_rep_init_t    outp_reps[BLE_HIDS_MAX_OUTPUT_REP];
    ble_hids_feature_rep_init_t feature_reps[BLE_HIDS_MAX_FEATURE_REP];
    ble_uuid_t                  ext_rep_refs[BLE_HIDS_MAX_INPUT_REP];

    ASSERT(BLE_HIDS_MAX_INPUT_REP   <= s_hid_db->reports_in_size);
    ASSERT(BLE_HIDS_MAX_OUTPUT_REP  <= s_hid_db->reports_out_size);
    ASSERT(BLE_HIDS_MAX_FEATURE_REP <= s_hid_db->reports_feature_size);
    ASSERT(BLE_HIDS_MAX_INPUT_REP   <= s_hid_db->ext_maps_size);

    if (p_params->evt_handler       == 0 ||
        p_params->error_handler     == 0 ||
        p_params->base_hid_version  == 0 ||
        p_params->db_loc            == 0)
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    s_evt_handler             = p_params->evt_handler;
    s_error_handler           = p_params->error_handler;
    s_hid_db                  = p_params->db_loc;
    s_hid_info.bcd_hid        = p_params->base_hid_version;
    s_hid_info.b_country_code = p_params->b_country_code;
    s_hid_info.flags          = p_params->flags;
    s_with_mitm               = p_params->io_capabilities == BLE_GAP_IO_CAPS_NONE ? false : true;

    if (s_with_mitm)
    {
        BLE_GAP_CONN_SEC_MODE_SET_ENC_WITH_MITM(&s_hid_info.security_mode.read_perm);
    }
    else
    {
        BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&s_hid_info.security_mode.read_perm);
    }

    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&s_hid_info.security_mode.write_perm);

    memset(inp_reps, 0, sizeof(ble_hids_inp_rep_init_t) * s_hid_db->reports_in_size);
    memset(outp_reps, 0, sizeof(ble_hids_outp_rep_init_t) * s_hid_db->reports_out_size);
    memset(feature_reps, 0, sizeof(ble_hids_feature_rep_init_t) * s_hid_db->reports_feature_size);
    memset(ext_rep_refs, 0, sizeof(ble_uuid_t) * s_hid_db->ext_maps_size);

    // Warning: If the order in which reports are created is changed, make sure to update
    // report_idx_determine() to match the new behavior.
    // This function estimates which index a report will have once the service is created.
    for (size_t iface_idx = 0; iface_idx < s_hid_db->report_maps_size; ++iface_idx)
    {
        uint8_t inp_rep_count     = 0;
        uint8_t outp_rep_count    = 0;
        uint8_t feat_rep_count    = 0;
        uint8_t ext_rep_ref_count = 0;
        bool    keyboard_boot     = false;
        bool    mouse_boot        = false;

        // Are boot modes defined in the HID report map?
        if (s_hid_db->report_maps[iface_idx].boot_type & ble_boot_pkt_keyboard)
        {
            keyboard_boot = true;
        }

        if (s_hid_db->report_maps[iface_idx].boot_type & ble_boot_pkt_mouse)
        {
            mouse_boot = true;
        }

        for (size_t i = 0; i < s_hid_db->reports_in_size; ++i)
        {
            ble_hid_report_record_t const * rep = &s_hid_db->reports_in[i];
            if (rep->interface_idx != iface_idx)
            {
                // This report does not belong to this service.
                continue;
            }

            if (rep->report_type != hid_report_type_input)
            {
                return NRF_ERROR_INTERNAL;
            }

            inp_reps[inp_rep_count].max_len             = rep->report_len;
            inp_reps[inp_rep_count].rep_ref.report_id   = rep->report_id;
            inp_reps[inp_rep_count].rep_ref.report_type = rep->report_type;
            inp_reps[inp_rep_count].read_resp           = rep->read_resp;

            security_mode_cccd_set(&inp_reps[inp_rep_count].security_mode, s_with_mitm);

            ++inp_rep_count;
        }

        for (size_t i = 0; i < s_hid_db->reports_out_size; ++i)
        {
            ble_hid_report_record_t const * rep = &s_hid_db->reports_out[i];
            if (rep->interface_idx != iface_idx)
            {
                // This report does not belong to this service.
                continue;
            }

            if (rep->report_type != hid_report_type_output)
            {
                return NRF_ERROR_INTERNAL;
            }

            outp_reps[outp_rep_count].max_len             = rep->report_len;
            outp_reps[outp_rep_count].rep_ref.report_id   = rep->report_id;
            outp_reps[outp_rep_count].rep_ref.report_type = rep->report_type;
            outp_reps[outp_rep_count].read_resp           = rep->read_resp;

            security_mode_cccd_set(&outp_reps[outp_rep_count].security_mode, s_with_mitm);

            ++outp_rep_count;
        }

        for (size_t i = 0; i < s_hid_db->reports_feature_size; ++i)
        {
            ble_hid_report_record_t const * rep = &s_hid_db->reports_feature[i];
            if (rep->interface_idx != iface_idx)
            {
                // This report does not belong to this service.
                continue;
            }

            if (rep->report_type != hid_report_type_feature)
            {
                return NRF_ERROR_INTERNAL;
            }

            feature_reps[feat_rep_count].max_len             = rep->report_len;
            feature_reps[feat_rep_count].rep_ref.report_id   = rep->report_id;
            feature_reps[feat_rep_count].rep_ref.report_type = rep->report_type;
            feature_reps[feat_rep_count].read_resp           = rep->read_resp;

            security_mode_cccd_set(&feature_reps[feat_rep_count].security_mode, s_with_mitm);

            ++feat_rep_count;
        }

        for (size_t i = 0; i < s_hid_db->ext_maps_size; ++i)
        {
            ble_hid_ext_map_record_t const * ext_rep = &s_hid_db->ext_mappings[i];

            if (ext_rep->interface_idx != iface_idx)
            {
                // This external report reference does not belong to this service.
                continue;
            }

            ext_rep_refs[ext_rep_ref_count].type = BLE_UUID_TYPE_BLE;
            ext_rep_refs[ext_rep_ref_count].uuid = ext_rep->external_char_uuid;

            ++ext_rep_ref_count;
        }

        memset(&hids_init_obj, 0, sizeof(hids_init_obj));
        hids_init_obj.evt_handler                    = on_hids_evt;
        hids_init_obj.error_handler                  = s_error_handler;
        hids_init_obj.is_kb                          = keyboard_boot;
        hids_init_obj.is_mouse                       = mouse_boot;
        hids_init_obj.inp_rep_count                  = inp_rep_count;
        hids_init_obj.p_inp_rep_array                = inp_reps;
        hids_init_obj.outp_rep_count                 = outp_rep_count;
        hids_init_obj.p_outp_rep_array               = outp_reps;
        hids_init_obj.feature_rep_count              = feat_rep_count;
        hids_init_obj.p_feature_rep_array            = feature_reps;
        hids_init_obj.rep_map.data_len               = s_hid_db->report_maps[iface_idx].report_map_len;
        hids_init_obj.rep_map.p_data                 = s_hid_db->report_maps[iface_idx].report_map;
        hids_init_obj.rep_map.p_ext_rep_ref          = ext_rep_refs;
        hids_init_obj.rep_map.ext_rep_ref_num        = ext_rep_ref_count;
        hids_init_obj.included_services_count        = 0;
        hids_init_obj.p_included_services_array      = NULL;
        memcpy(&hids_init_obj.hid_information, &s_hid_info, sizeof(s_hid_info));

        // Configuring security modes.
        security_mode_set(&hids_init_obj.security_mode_protocol, true, true, s_with_mitm);
        security_mode_set(&hids_init_obj.security_mode_ctrl_point, false, true, s_with_mitm);
        security_mode_set(&hids_init_obj.rep_map.security_mode, true, false, s_with_mitm);

        if (keyboard_boot)
        {
            security_mode_set(&hids_init_obj.security_mode_boot_kb_outp_rep, true, true, s_with_mitm);
            security_mode_cccd_set(&hids_init_obj.security_mode_boot_kb_inp_rep, s_with_mitm);
        }

        if (mouse_boot)
        {
            security_mode_cccd_set(&hids_init_obj.security_mode_boot_mouse_inp_rep, s_with_mitm);
        }

        err_code = ble_hids_init(s_hid_db->report_maps[iface_idx].interface,
                                 &hids_init_obj);
        if (err_code != NRF_SUCCESS)
        {
            return err_code;
        }
    }

    if (p_last_input_report_cccd_handle != NULL)
    {
        const ble_hids_t *p_hids = s_hid_db->report_maps[s_hid_db->report_maps_size - 1].interface;
        *p_last_input_report_cccd_handle = p_hids->inp_rep_array[p_hids->inp_rep_count - 1].char_handles.cccd_handle;
        NRF_LOG_DEBUG("p_last_input_report_cccd_handle = 0x%04X", *p_last_input_report_cccd_handle);
    }

    return NRF_SUCCESS;
}

uint32_t m_coms_ble_hid_input_report_send(uint8_t   p_interface_idx,
                                          uint8_t   p_report_idx,
                                          uint8_t * p_data,
                                          uint8_t   p_len)
{
    return ble_hids_inp_rep_send(s_hid_db->report_maps[p_interface_idx].interface,
                                 p_report_idx,
                                 p_len,
                                 p_data);
}

uint32_t m_coms_ble_hid_keyboard_boot_report_send(const m_coms_hid_boot_pkt_t * p_pkt)
{
    for (size_t i = 0; i < s_hid_db->report_maps_size; ++i)
    {
        if (s_hid_db->report_maps[i].interface->boot_kb_inp_rep_handles.value_handle != 0)
        {
            return ble_hids_boot_kb_inp_rep_send(s_hid_db->report_maps[i].interface,
                                                 BOOT_KEYBOARD_LEN,
                                                 (uint8_t *)p_pkt->keyboard_data.keys);
        }
    }

    return NRF_ERROR_INVALID_STATE;
}

uint32_t m_coms_ble_hid_mouse_boot_report_send(const m_coms_hid_boot_pkt_t * p_pkt)
{
    for (size_t i = 0; i < s_hid_db->report_maps_size; ++i)
    {
        if (s_hid_db->report_maps[i].interface->boot_mouse_inp_rep_handles.value_handle != 0)
        {
            return ble_hids_boot_mouse_inp_rep_send(s_hid_db->report_maps[i].interface,
                                                    p_pkt->mouse_data.buttons,
                                                    p_pkt->mouse_data.x_delta,
                                                    p_pkt->mouse_data.y_delta,
                                                    0,
                                                    0);
        }
    }

    return NRF_ERROR_INVALID_STATE;
}

uint32_t m_coms_ble_hid_feature_report_set(uint8_t   p_interface_idx,
                                          uint8_t   p_report_idx,
                                          uint8_t * p_data,
                                          uint16_t   p_len)
{
    return NRF_ERROR_NOT_SUPPORTED;
}

static void m_coms_ble_hid_on_ble_evt(ble_evt_t const * p_ble_evt, void *p_context)
{
    for (size_t i = 0; i < s_hid_db->report_maps_size; ++i)
    {
        ble_hids_on_ble_evt(p_ble_evt, s_hid_db->report_maps[i].interface);
    }
}

NRF_SDH_BLE_OBSERVER(m_coms_ble_hid_observer, BLE_OBSERVER_PRIORITY_DEFAULT, m_coms_ble_hid_on_ble_evt, NULL);

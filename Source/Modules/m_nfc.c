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

#include "nrf.h"
#include "nrf_assert.h"
#include "nrf_drv_rng.h"
#include "nrf_pwr_mgmt.h"
#include "app_debug.h"
#include "app_scheduler.h"
#include "app_util.h"

#include "nfc_ble_pair_msg.h"
#include "nfc_ndef_msg.h"
#include "nfc_t2t_lib.h"
#include "nfc_text_rec.h"

#include "m_nfc.h"
#include "m_coms_ble_lesc.h"
#include "rng_monitor.h"
#include "resources.h"
#include "sr3_config.h"

#if CONFIG_NFC_ENABLED

#define NRF_LOG_MODULE_NAME m_nfc
#define NRF_LOG_LEVEL CONFIG_NFC_MODULE_LOG_LEVEL
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

// Check configuration
#if !(CONFIG_NFC_TEXT_TAG || CONFIG_NFC_PAIRING_TAG)
#error "At least one tag type (CONFIG_NFC_TEXT_TAG or CONFIG_NFC_PAIRING_TAG) has to be selected."
#endif

/**@brief Type of NFC Pairing message. */
#if CONFIG_NFC_USE_FULL_PAIRING_MSG
#define NFC_PAIRING_MESSAGE_TYPE    NFC_BLE_PAIR_MSG_FULL
#else
#define NFC_PAIRING_MESSAGE_TYPE    NFC_BLE_PAIR_MSG_BLUETOOTH_LE_SHORT
#endif

/**@brief Text embedded into the tag when pairing is not possible. */
#define NFC_TEXT_MESSAGE    "nRFready Smart Remote 3\n"                         \
                            "Board: " CONFIG_BOARD "\n"                         \
                            "Firmware Revision: " CONFIG_FIRMWARE_REVISION "\n" \
                            "Bluetooth Device Name: '" CONFIG_DEVICE_NAME "'\n"

/**@brief NFC module states. */
typedef enum
{
    M_NFC_STATE_INACTIVE,               //!< NFC module is not active.
    M_NFC_STATE_PREPARING_PAIRING,      //!< NFC module is preparing a pairing message.
    M_NFC_STATE_ACTIVE_PAIRING,         //!< NFC module is active and presents a pairing message.
    M_NFC_STATE_ACTIVE_TEXT,            //!< NFC module is active and presents a text message.
    M_NFC_STATE_SHUTTING_DOWN,          //!< NFC module is shutting down.
} m_nfc_state_t;

static m_nfc_state_t        m_nfc_state;                //!< State of the NFC module.
__ALIGN(4) static uint8_t   m_nfc_ndef_msg_buffer[288]; //!< Buffer for the NDEF message.

#if (CONFIG_NFC_PAIRING_TAG && CONFIG_SEC_LEGACY_PAIRING)
// Verify SDK configuration.
STATIC_ASSERT(RNG_ENABLED);
STATIC_ASSERT(RNG_CONFIG_POOL_SIZE >= 16);

static ble_advdata_tk_value_t   m_nfc_tk;                   //!< TK presented in a pairing tag.
static bool                     m_nfc_tk_valid;             //!< True if m_nfc_tk is valid.
#endif

/**@brief Handle NFC events. */
static void m_nfc_handler(void *p_evt, uint16_t evt_size)
{
    nfc_t2t_event_t *p_event = p_evt;

    ASSERT(evt_size == sizeof(*p_event));

    switch (*p_event)
    {
        case NFC_T2T_EVENT_FIELD_ON:
            NRF_LOG_DEBUG("Field ON");
            break;

        case NFC_T2T_EVENT_FIELD_OFF:
            NRF_LOG_DEBUG("Field OFF");
            break;

        case NFC_T2T_EVENT_DATA_READ:
            NRF_LOG_DEBUG("Data Read");
            break;

        default:
            /* Ignore */
            break;
    }

#if CONFIG_PWR_MGMT_ENABLED
    // Notify the power manager about the activity.
    switch (*p_event)
    {
        case NFC_T2T_EVENT_FIELD_ON:
        case NFC_T2T_EVENT_DATA_READ:
            nrf_pwr_mgmt_feed();
            break;

        default:
            /* Ignore */
            break;
    }
#endif /* CONFIG_PWR_MGMT_ENABLED */
}

/**@brief NFC Library callback. */
static void m_nfc_callback(void *p_context,
                           nfc_t2t_event_t event,
                           const uint8_t *p_data,
                           size_t data_length)
{
    // Schedule an event for execution out of ISR context.
    APP_ERROR_CHECK(app_sched_event_put(&event, sizeof(event), m_nfc_handler));
}

#if CONFIG_NFC_PAIRING_TAG
static ret_code_t m_nfc_create_pairing_msg(ble_advdata_tk_value_t  * const p_tk_value,
                                           ble_gap_lesc_oob_data_t * const p_lesc_oob_data)
{
    uint32_t ndef_msg_size = sizeof(m_nfc_ndef_msg_buffer);
    ret_code_t status;

    status = nfc_ble_pair_default_msg_encode(NFC_PAIRING_MESSAGE_TYPE,
                                             p_tk_value,
                                             p_lesc_oob_data,
                                             m_nfc_ndef_msg_buffer,
                                             &ndef_msg_size);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    return nfc_t2t_payload_set(m_nfc_ndef_msg_buffer, ndef_msg_size);
}

/**@brief Prepare keys needed for NFC pairing. */
static void m_nfc_prepare_pairing_keys(void *p_context)
{
    ble_gap_lesc_oob_data_t *p_lesc_oob_data = NULL;
    ble_advdata_tk_value_t  *p_tk = NULL;
#if CONFIG_SEC_LESC_PAIRING
    ble_gap_lesc_oob_data_t lesc_oob_data;
#endif
#if CONFIG_SEC_LEGACY_PAIRING
    ret_code_t status;
#endif

    // Do not perform any action if module state changed when we were waiting for RNG.
    if (m_nfc_state != M_NFC_STATE_PREPARING_PAIRING)
    {
        return;
    }

#if CONFIG_SEC_LEGACY_PAIRING
    ASSERT(m_nfc_tk_valid == false);

    status = nrf_drv_rng_rand(m_nfc_tk.tk, sizeof(m_nfc_tk.tk));
    switch (status)
    {
        case NRF_SUCCESS:
            // TK has been generated.
            p_tk = &m_nfc_tk;
            m_nfc_tk_valid = true;

            NRF_LOG_DEBUG("TK: %08X%08X%08X%08X",
                       uint32_big_decode(&p_tk->tk[0x00]),
                       uint32_big_decode(&p_tk->tk[0x04]),
                       uint32_big_decode(&p_tk->tk[0x08]),
                       uint32_big_decode(&p_tk->tk[0x0C]));
            break;

        case NRF_ERROR_NOT_FOUND:
            // Not enough entropy to generate a TK now. Try again later.
            APP_ERROR_CHECK(rng_monitor_request(sizeof(m_nfc_tk.tk),
                                                m_nfc_prepare_pairing_keys,
                                                p_context));
            return;

        default:
            APP_ERROR_CHECK(status);
            break;
    }
#endif /* CONFIG_SEC_LEGACY_PAIRING */

#if CONFIG_SEC_LESC_PAIRING
    if (m_coms_ble_lesc_generate_oob_data(&lesc_oob_data) == NRF_SUCCESS)
    {
        p_lesc_oob_data = &lesc_oob_data;

        NRF_LOG_DEBUG("LESC OOB[addr]: %02X:%02X:%02X:%02X:%02X:%02X",
                  p_lesc_oob_data->addr.addr[5],
                  p_lesc_oob_data->addr.addr[4],
                  p_lesc_oob_data->addr.addr[3],
                  p_lesc_oob_data->addr.addr[2],
                  p_lesc_oob_data->addr.addr[1],
                  p_lesc_oob_data->addr.addr[1]);

        NRF_LOG_DEBUG("LESC OOB[r]: %08X%08X%08X%08X",
                   uint32_big_decode(&p_lesc_oob_data->r[0x00]),
                   uint32_big_decode(&p_lesc_oob_data->r[0x04]),
                   uint32_big_decode(&p_lesc_oob_data->r[0x08]),
                   uint32_big_decode(&p_lesc_oob_data->r[0x0C]));

        NRF_LOG_DEBUG("LESC OOB[c]: %08X%08X%08X%08X",
                   uint32_big_decode(&p_lesc_oob_data->c[0x00]),
                   uint32_big_decode(&p_lesc_oob_data->c[0x04]),
                   uint32_big_decode(&p_lesc_oob_data->c[0x08]),
                   uint32_big_decode(&p_lesc_oob_data->c[0x0C]));
    }
    else
    {
        NRF_LOG_WARNING("%s(): WARNING: Cannot generate LESC OOB data!", (uint32_t)__func__);
    }
#endif /* CONFIG_SEC_LESC_PAIRING */

    // Create a pairing message.
    APP_ERROR_CHECK(m_nfc_create_pairing_msg(p_tk, p_lesc_oob_data));

    // Enable NFC.
    APP_ERROR_CHECK(nfc_t2t_emulation_start());
    m_nfc_state = M_NFC_STATE_ACTIVE_PAIRING;

    NRF_LOG_INFO("Tag Enabled (pairing mode%s%s)",
              (uint32_t)((p_tk != NULL) ? ", TK" : ""),
              (uint32_t)((p_lesc_oob_data != NULL) ? ", LESC-OOB" : ""));
}
#endif /* CONFIG_NFC_PAIRING_TAG */

#if CONFIG_NFC_TEXT_TAG
/**@brief Create an NFC tag with a text message. */
static ret_code_t m_nfc_create_text_msg(const char *p_text)
{
    uint32_t ndef_msg_size = sizeof(m_nfc_ndef_msg_buffer);
    static const uint8_t language[] = { 'e', 'n' };
    ret_code_t status;

    ASSERT(p_text != NULL);

    NFC_NDEF_MSG_DEF(text_msg, 1);
    NFC_NDEF_TEXT_RECORD_DESC_DEF(text_rec,
                                  UTF_8,
                                  language,
                                  sizeof(language),
                                  NULL,
                                  0);

    /*
     * The message and record definitions are static, therefore we have to reinitialize
     * data structures on every update.
     */
    nfc_ndef_msg_clear(&NFC_NDEF_MSG(text_msg));
    text_rec_nfc_text_rec_payload_desc.p_data   = (const uint8_t *)(p_text);
    text_rec_nfc_text_rec_payload_desc.data_len = strlen(p_text);

    status = nfc_ndef_msg_record_add(&NFC_NDEF_MSG(text_msg), &NFC_NDEF_TEXT_RECORD_DESC(text_rec));
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    status = nfc_ndef_msg_encode(&NFC_NDEF_MSG(text_msg), m_nfc_ndef_msg_buffer, &ndef_msg_size);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    return nfc_t2t_payload_set(m_nfc_ndef_msg_buffer, ndef_msg_size);
}
#endif /* CONFIG_NFC_TEXT_TAG */

bool m_nfc_event_handler(const event_t *p_event)
{
    ASSERT(p_event != NULL);

    switch (p_event->type)
    {
        case EVT_BT_ADV_STATE:
            switch(p_event->bt.data)
            {
#if CONFIG_NFC_PAIRING_TAG
                case BT_ADV_STATE_ACTIVE_BONDABLE:
                    switch (m_nfc_state)
                    {
                        // If NFC is active in text mode, shut it down.
                        case M_NFC_STATE_ACTIVE_TEXT:
                            APP_ERROR_CHECK(nfc_t2t_emulation_stop());
                            NRF_LOG_INFO("Tag Disabled");
                            /* Fallthrough */

                        // Prepare a pairing tag (if support for it is enabled).
                        case M_NFC_STATE_INACTIVE:
#if CONFIG_SEC_LEGACY_PAIRING
                            // Invalidate current TK.
                            m_nfc_tk_valid = false;
#endif
                            // Update module state and prepare a pairing tag.
                            m_nfc_state = M_NFC_STATE_PREPARING_PAIRING;
                            m_nfc_prepare_pairing_keys(NULL);
                            break;

                        default:
                            /* Do nothing */
                            break;
                    }
                    break;
#endif /* CONFIG_NFC_PAIRING_TAG */

                default:
                    switch (m_nfc_state)
                    {
                        // If NFC is active in pairing mode, shut it down.
                        case M_NFC_STATE_ACTIVE_PAIRING:
                            APP_ERROR_CHECK(nfc_t2t_emulation_stop());
                            NRF_LOG_INFO("Tag Disabled");
                            /* Fallthrough */

                        // Prepare a text tag (if support for it is enabled).
                        case M_NFC_STATE_INACTIVE:
                        case M_NFC_STATE_PREPARING_PAIRING:
#if CONFIG_NFC_TEXT_TAG
                            // Create a Text Tag.
                            APP_ERROR_CHECK(m_nfc_create_text_msg(NFC_TEXT_MESSAGE));
                            APP_ERROR_CHECK(nfc_t2t_emulation_start());

                            // Update module state.
                            m_nfc_state = M_NFC_STATE_ACTIVE_TEXT;
                            NRF_LOG_INFO("Tag Enabled (text mode)");
#else /* !CONFIG_NFC_TEXT_TAG */
                            m_nfc_state = M_NFC_STATE_INACTIVE;
#endif /* CONFIG_NFC_TEXT_TAG */
                            break;

                        default:
                            /* Do nothing */
                            break;
                    }
                    break;
            }
            break;

#if (CONFIG_NFC_PAIRING_TAG && CONFIG_SEC_LEGACY_PAIRING)
        case EVT_BT_CONN_STATE:
            switch(p_event->bt.data)
            {
                case BT_CONN_STATE_SECURED:
                    // Invalidate current TK.
                    m_nfc_tk_valid = false;
                    break;

                default:
                    /* Do nothing */
                    break;
            }
            break;
#endif /* (CONFIG_NFC_PAIRING_TAG && CONFIG_SEC_LEGACY_PAIRING) */

        default:
            /* Ignore */
            break;
    }

    return false;
}

ret_code_t m_nfc_init(void)
{
    ret_code_t status;

    status = nfc_t2t_setup(m_nfc_callback, NULL);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    m_nfc_state = M_NFC_STATE_INACTIVE;

#if (CONFIG_NFC_PAIRING_TAG && CONFIG_SEC_LEGACY_PAIRING)
    m_nfc_tk_valid = false;
#endif

    return NRF_SUCCESS;
}

ret_code_t m_nfc_get_tk(ble_advdata_tk_value_t *p_tk)
{
#if (CONFIG_NFC_PAIRING_TAG && CONFIG_SEC_LEGACY_PAIRING)
    if (p_tk == NULL)
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    if (m_nfc_tk_valid == false)
    {
        return NRF_ERROR_INVALID_STATE;
    }

    *p_tk = m_nfc_tk;

    return NRF_SUCCESS;
#else
    return NRF_ERROR_NOT_SUPPORTED;
#endif
}

#if CONFIG_PWR_MGMT_ENABLED
static bool m_nfc_shutdown(nrf_pwr_mgmt_evt_t event)
{
    switch (m_nfc_state)
    {
        // If NFC is active, disable it.
        case M_NFC_STATE_ACTIVE_PAIRING:
        case M_NFC_STATE_ACTIVE_TEXT:
            nfc_t2t_emulation_stop();
            NRF_LOG_INFO("Tag Disabled");
            /* Fallthrough */

        default:
            m_nfc_state = M_NFC_STATE_SHUTTING_DOWN;
            break;
    }

#if CONFIG_NFC_WAKEUP_SOURCE
    if (event == NRF_PWR_MGMT_EVT_PREPARE_WAKEUP)
    {
        NRF_NFCT->TASKS_SENSE = 1;
        NRF_LOG_INFO("Field Sensing Enabled.");
    }
#endif /* CONFIG_NFC_WAKEUP_SOURCE */

    return true;
}
NRF_PWR_MGMT_HANDLER_REGISTER(m_nfc_shutdown, SHUTDOWN_PRIORITY_EARLY);
#endif /* CONFIG_PWR_MGMT_ENABLED */
#endif /* CONFIG_NFC_ENABLED */

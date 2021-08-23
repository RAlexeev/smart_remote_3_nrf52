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

#include <stdbool.h>

#include "nrf_pwr_mgmt.h"
#include "app_debug.h"
#include "app_error.h"

#include "drv_board.h"

#include "m_acc.h"
#include "m_audio.h"
#include "m_buzzer.h"
#include "m_batt_meas.h"
#include "m_coms.h"
#include "m_gyro.h"
#include "m_ir_tx.h"
#include "m_keyboard.h"
#include "m_leds.h"
#include "m_nfc.h"
#include "m_touchpad.h"

#include "m_system_state.h"
#include "m_protocol_hid.h"
#include "m_protocol_ir.h"

#include "m_init.h"
#include "key_combo_util.h"
#include "twi_common.h"
#include "resources.h"
#include "sr3_config.h"

#if CONFIG_PWR_MGMT_ENABLED
/**@brief Prepare board for sleep and subsequent wakeup.
 *
 * @param[in] wakeup If false, go to deepest sleep and do not prepare to wake up the system.
 *
 * @return True if the board is ready for shutdown, false otherwise.
 */
static bool board_shutdown(nrf_pwr_mgmt_evt_t event)
{
    bool wakeup = (event == NRF_PWR_MGMT_EVT_PREPARE_WAKEUP) ? true : false;

    return drv_board_shutdown(wakeup);
}
NRF_PWR_MGMT_HANDLER_REGISTER(board_shutdown, SHUTDOWN_PRIORITY_FINAL);
#endif /* CONFIG_PWR_MGMT_ENABLED */

/**@brief Event Bus handlers. */
static const event_handler_t m_event_handlers[] =
{
    m_system_state_event_handler,

    m_protocol_hid_event_handler,

#if CONFIG_IR_TX_ENABLED
    m_protocol_ir_event_handler,
#endif
    m_coms_event_handler,
#if CONFIG_NFC_ENABLED
    m_nfc_event_handler,
#endif
#if CONFIG_IR_TX_ENABLED
    m_ir_tx_event_handler,
#endif
#if CONFIG_BUZZER_ENABLED
    m_buzzer_event_handler,
#endif
#if CONFIG_GYRO_ENABLED
    m_gyro_event_handler,
#endif
#if CONFIG_LED_ENABLED
    m_leds_event_handler,
#endif
#if CONFIG_TOUCHPAD_ENABLED
    m_touchpad_event_handler,
#endif
#if CONFIG_KBD_KEY_COMBO_ENABLED
    key_combo_util_key_process,
#endif
    NULL,
};

void m_init(bool is_resume)
{
    bool delete_bonds;

    // Perform board initialization as early as possible.
    APP_ERROR_CHECK(drv_board_init());

#if TWI_ENABLED
    // Initialize TWI buses.
    APP_ERROR_CHECK(twi_init());
#endif

    // Initialize event bus.
    APP_ERROR_CHECK(event_bus_init(m_event_handlers));

    // Initialize System State module.
    APP_ERROR_CHECK(m_system_state_init());

    // Initialize Protocol modules.
    APP_ERROR_CHECK(m_protocol_hid_init());

#if CONFIG_IR_TX_ENABLED
    APP_ERROR_CHECK(m_protocol_ir_init());
#endif

#if CONFIG_KBD_ENABLED
    // Initialize keyboard and check for Delete Bonds key.
    APP_ERROR_CHECK(m_keyboard_init(&delete_bonds));
#endif

#if CONFIG_KBD_KEY_COMBO_ENABLED
    APP_ERROR_CHECK(key_combo_util_init());
#endif /* CONFIG_KBD_KEY_COMBO_ENABLED */

    // Delete bonds only if it was requested during initial power-up.
    if (is_resume || !CONFIG_KBD_ENABLED)
    {
        delete_bonds = false;
    }

    // Initialize Communication module.
    APP_ERROR_CHECK(m_coms_init(delete_bonds));

#if CONFIG_NFC_ENABLED
    // Initialize NFC module.
    APP_ERROR_CHECK(m_nfc_init());
#endif

#if CONFIG_KBD_ENABLED
    // Enable keyboard early in order to catch all keys.
    APP_ERROR_CHECK(m_keyboard_enable());
#endif

#if CONFIG_ACC_ENABLED
    // Initialize Accelerometer.
    APP_ERROR_CHECK(m_acc_init());
#endif

#if CONFIG_AUDIO_ENABLED
    // Initialize Audio Subsystem.
    APP_ERROR_CHECK(m_audio_frame_init());
    APP_ERROR_CHECK(m_audio_init());
#endif

#if CONFIG_BATT_MEAS_ENABLED
    // Initialize Battery Measurement.
    APP_ERROR_CHECK(m_batt_meas_init());
#endif

#if CONFIG_GYRO_ENABLED
    // Initialize Gyroscope.
    APP_ERROR_CHECK(m_gyro_init(false));
#endif

#if CONFIG_IR_TX_ENABLED
    // Initialize IR Transmitter.
    APP_ERROR_CHECK(m_ir_tx_init());
#endif

#if CONFIG_BUZZER_ENABLED
    // Initialize Buzzer Transmitter.
    APP_ERROR_CHECK(m_buzzer_init());
#endif

#if CONFIG_LED_ENABLED
    // Initialize LED Signaling.
    APP_ERROR_CHECK(m_leds_init());
#endif

#if CONFIG_TOUCHPAD_ENABLED
    // Initialize Touchpad.
    APP_ERROR_CHECK(m_touchpad_init());

    // Enable Touchpad.
    APP_ERROR_CHECK(m_touchpad_enable());
#endif
}


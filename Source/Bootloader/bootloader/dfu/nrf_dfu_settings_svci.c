/**
 * Copyright (c) 2017 - 2018, Nordic Semiconductor ASA
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
#include "nrf_dfu_settings.h"
#include "nrf_nvmc.h"
#include "crc32.h"

#define NRF_LOG_MODULE_NAME nrf_dfu_settings_svci
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();


/**@brief Offset in the settings structure where the shared data is located. */
#define DFU_SETTINGS_APP_DATA_OFFSET   offsetof(nrf_dfu_settings_t, shared_data)


extern nrf_dfu_settings_t s_dfu_settings;
extern uint8_t m_dfu_settings_buffer[CODE_PAGE_SIZE];


bool nrf_dfu_settings_shared_data_is_valid(void)
{
    nrf_dfu_shared_data_t *p_shared_data = (nrf_dfu_shared_data_t *)&s_dfu_settings.shared_data;

    // Calculate the CRC for the structure excluding the CRC value itself.
    uint32_t crc = crc32_compute((uint8_t *)p_shared_data + sizeof(p_shared_data->crc),
                                 sizeof(*p_shared_data) - sizeof(p_shared_data->crc),
                                 NULL);

    return (p_shared_data->crc == crc);
}


//lint -save -e(14)
ret_code_t nrf_dfu_settings_shared_data_erase(void)
{
    ret_code_t ret_code = NRF_SUCCESS;

    if (s_dfu_settings.shared_data.crc != 0xFFFFFFFF)
    {
        NRF_LOG_DEBUG("Erasing shared data.");

        // Erasing and resetting the settings page without the peer data/adv data
        nrf_nvmc_page_erase(BOOTLOADER_SETTINGS_ADDRESS);
        nrf_nvmc_write_words(BOOTLOADER_SETTINGS_ADDRESS,
                             (uint32_t const *)&s_dfu_settings,
                             DFU_SETTINGS_APP_DATA_OFFSET / sizeof(uint32_t));
    }

    return ret_code;
}
//lint -restore

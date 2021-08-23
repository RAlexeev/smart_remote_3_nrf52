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

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>

#include "nrf_assert.h"
#include "nrf_error.h"
#include "nrf_gpio.h"
#include "nrf_drv_pdm.h"
#include "app_debug.h"

#include "drv_audio.h"
#include "m_audio_gauges.h"
#include "sr3_config.h"

#if CONFIG_AUDIO_ENABLED

#define NRF_LOG_MODULE_NAME drv_audio_pdm
#define NRF_LOG_LEVEL CONFIG_AUDIO_DRV_PDM_LOG_LEVEL
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();


#ifdef NRF52810_XXAA
# if (CONFIG_PDM_MIC != CONFIG_PDM_MIC_LEFT)
#  error "Devices based on nRF52810 does not support microphone selection."
# endif
#endif

// Verify SDK configuration.
STATIC_ASSERT(PDM_ENABLED);

// Check pin configuration.
STATIC_ASSERT(IS_IO_VALID(CONFIG_IO_PDM_CLK));
STATIC_ASSERT(IS_IO_VALID(CONFIG_IO_PDM_DATA));
#if CONFIG_PDM_MIC_PWR_CTRL_ENABLED
STATIC_ASSERT(IS_IO_VALID(CONFIG_IO_PDM_MIC_PWR_CTRL));
#endif

static nrf_balloc_t const *         mp_buffer_pool;
static drv_audio_buffer_handler_t   m_buffer_handler;
static uint8_t                      m_skip_buffers;
#if CONFIG_AUDIO_GAUGES_ENABLED
static m_audio_loss_gauge_t         m_loss_gauge;
#endif

ret_code_t drv_audio_enable(void)
{
    // Turn on microphone power.
#if CONFIG_PDM_MIC_PWR_CTRL_ENABLED
#if CONFIG_PDM_MIC_PWR_CTRL_ACT_LOW
    nrf_gpio_pin_clear(CONFIG_IO_PDM_MIC_PWR_CTRL);
#else /* !CONFIG_PDM_MIC_PWR_CTRL_ACT_LOW */
    nrf_gpio_pin_set(CONFIG_IO_PDM_MIC_PWR_CTRL);
#endif /* CONFIG_PDM_MIC_PWR_CTRL_ACT_LOW */
#endif /* CONFIG_PDM_MIC_PWR_CTRL_ENABLED */

    // Skip buffers with invalid data.
    m_skip_buffers = MAX(1, ROUNDED_DIV(CONFIG_PDM_TRANSIENT_STATE_LEN,
                                        CONFIG_AUDIO_FRAME_SIZE_MS));

#if CONFIG_AUDIO_GAUGES_ENABLED
    // Reset driver data loss gauge.
    m_audio_loss_gauge_reset(&m_loss_gauge);
#endif

    // Start audio capture.
    return nrf_drv_pdm_start();
}

ret_code_t drv_audio_disable(void)
{
    ret_code_t status;

    // Stop audio capture.
    status = nrf_drv_pdm_stop();
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    // Turn off microphone power.
#if CONFIG_PDM_MIC_PWR_CTRL_ENABLED
#if CONFIG_PDM_MIC_PWR_CTRL_ACT_LOW
    nrf_gpio_pin_set(CONFIG_IO_PDM_MIC_PWR_CTRL);
#else /* !CONFIG_PDM_MIC_PWR_CTRL_ACT_LOW */
    nrf_gpio_pin_clear(CONFIG_IO_PDM_MIC_PWR_CTRL);
#endif /* CONFIG_PDM_MIC_PWR_CTRL_ACT_LOW */
#endif /* CONFIG_PDM_MIC_PWR_CTRL_ENABLED */

#if CONFIG_AUDIO_GAUGES_ENABLED
    // Log state of driver data loss gauge.
    m_audio_loss_gauge_log(&m_loss_gauge, "Buffers");
#endif

    return NRF_SUCCESS;
}

static void drv_audio_pdm_event_handler(nrf_drv_pdm_evt_t const * const p_evt)
{
    int16_t *p_buffer_released = p_evt->buffer_released;

    if (p_evt->buffer_requested)
    {
        int16_t *p_buffer;

        p_buffer = nrf_balloc_alloc(mp_buffer_pool);
        if (!p_buffer)
        {
            NRF_LOG_WARNING("%s(): WARNING: Cannot allocate audio buffer!", __func__);

            /*
             * We have to provide a fresh buffer to keep PDM running.
             * Since pool is empty, our sole option is to reuse the released buffer.
             */
            if (p_buffer_released)
            {
                p_buffer            = p_buffer_released;
                p_buffer_released   = NULL;

                m_audio_count_lost(&m_loss_gauge);
            }
        }

        if (p_buffer)
        {
            NRF_LOG_DEBUG("Buffer Request: 0x%08X%s",
                          p_buffer,
                          (p_buffer == p_evt->buffer_released) ? " (reusing released buffer)" : "");

            m_audio_count_total(&m_loss_gauge);
            APP_ERROR_CHECK(nrf_drv_pdm_buffer_set(p_buffer, CONFIG_PDM_BUFFER_SIZE_SAMPLES));
        }
    }

    if (p_buffer_released)
    {
        NRF_LOG_DEBUG("Buffer Release: 0x%08X", p_buffer_released);

        if (m_skip_buffers)
        {
            m_skip_buffers -= 1;
            nrf_balloc_free(mp_buffer_pool, p_buffer_released);
        }
        else
        {
            DBG_PIN_PULSE(CONFIG_IO_DBG_AUDIO_CAPTURE);
            m_buffer_handler(p_buffer_released);
        }
    }
}

ret_code_t drv_audio_init(nrf_balloc_t const *p_buffer_pool, drv_audio_buffer_handler_t buffer_handler)
{
    nrf_drv_pdm_config_t pdm_cfg = NRF_DRV_PDM_DEFAULT_CONFIG(CONFIG_IO_PDM_CLK,
                                                              CONFIG_IO_PDM_DATA);

    if ((p_buffer_pool == NULL) || (buffer_handler == NULL))
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    m_buffer_handler    = buffer_handler;
    mp_buffer_pool      = p_buffer_pool;

    pdm_cfg.clock_freq  = (nrf_pdm_freq_t)(CONFIG_PDM_MCLKFREQ);
    pdm_cfg.gain_l      = CONFIG_PDM_GAIN;
    pdm_cfg.gain_r      = CONFIG_PDM_GAIN;

#if CONFIG_AUDIO_ANR_ENABLED
    pdm_cfg.mode        = NRF_PDM_MODE_STEREO;
#else /* !CONFIG_AUDIO_ANR_ENABLED */
    pdm_cfg.mode        = NRF_PDM_MODE_MONO;
#endif /* CONFIG_AUDIO_ANR_ENABLED */

#if   (CONFIG_PDM_MIC == CONFIG_PDM_MIC_LEFT)
    pdm_cfg.edge        = NRF_PDM_EDGE_LEFTFALLING;
#elif (CONFIG_PDM_MIC == CONFIG_PDM_MIC_RIGHT)
    pdm_cfg.edge        = NRF_PDM_EDGE_LEFTRISING;
#else
#error "Value of CONFIG_PDM_MIC is not valid!"
#endif /* (CONFIG_PDM_MIC == CONFIG_PDM_MIC_LEFT) */

    // Turn off microphone power.
#if CONFIG_PDM_MIC_PWR_CTRL_ENABLED
#if CONFIG_PDM_MIC_PWR_CTRL_ACT_LOW
    nrf_gpio_pin_set(CONFIG_IO_PDM_MIC_PWR_CTRL);
#else /* !CONFIG_PDM_MIC_PWR_CTRL_ACT_LOW */
    nrf_gpio_pin_clear(CONFIG_IO_PDM_MIC_PWR_CTRL);
#endif /* CONFIG_PDM_MIC_PWR_CTRL_ACT_LOW */
    nrf_gpio_cfg_output(CONFIG_IO_PDM_MIC_PWR_CTRL);
#endif /* CONFIG_PDM_MIC_PWR_CTRL_ENABLED */

    // Initialize PDM driver.
    return nrf_drv_pdm_init(&pdm_cfg, drv_audio_pdm_event_handler);
}

#if CONFIG_CLI_ENABLED

static void drv_audio_info_cmd(nrf_cli_t const * p_cli, size_t argc, char **argv)
{
    nrf_pdm_gain_t gain_l, gain_r;
    bool pdm_enabled;

    if (nrf_cli_help_requested(p_cli))
    {
        nrf_cli_help_print(p_cli, NULL, 0);
        nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Usage:\r\n  %s\r\n", argv[0]);
        return;
    }

    pdm_enabled = nrf_pdm_enable_check();
    nrf_pdm_gain_get(&gain_l, &gain_r);

    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Configuration:\r\n");
    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "\tDriver:\t\t\tPDM\r\n");
    nrf_cli_fprintf(p_cli,
                    NRF_CLI_NORMAL,
                    "\tDecimator Gain:\t\tLeft: %d.%u dB, Right: %d.%u dB\r\n",
                    5 * (gain_l + (-20 << 1)) / 10,
                    abs(5 * (gain_l + (-20 << 1))) % 10,
                    5 * (gain_r + (-20 << 1)) / 10,
                    abs(5 * (gain_r + (-20 << 1))) % 10);

    nrf_cli_fprintf(p_cli,
                    NRF_CLI_NORMAL,
                    "\r\nStatus: %s\r\n",
                     (pdm_enabled) ? "Enabled" : "Disabled");

#if CONFIG_AUDIO_GAUGES_ENABLED
    if (!pdm_enabled)
    {
        return;
    }

    uint32_t buffers_total = m_audio_gauge_get_total_count(&m_loss_gauge);
    uint32_t buffers_lost  = m_audio_gauge_get_lost_count(&m_loss_gauge);

    nrf_cli_fprintf(p_cli,
                    NRF_CLI_NORMAL,
                    "\tCapture time:\t\t%u:%02u\r\n",
                    buffers_total * CONFIG_AUDIO_FRAME_SIZE_SAMPLES / CONFIG_AUDIO_REAL_SAMPLING_FREQUENCY / 60,
                    buffers_total * CONFIG_AUDIO_FRAME_SIZE_SAMPLES / CONFIG_AUDIO_REAL_SAMPLING_FREQUENCY % 60);

    nrf_cli_fprintf(p_cli,
                    NRF_CLI_NORMAL,
                    "\tBuffer loss ratio:\t%u%% (%u out of %u buffers)\r\n",
                    (buffers_total != 0) ? (100ul * buffers_lost / buffers_total) : 0,
                    buffers_lost,
                    buffers_total);
#endif /* CONFIG_AUDIO_GAUGES_ENABLED */
}

static void drv_audio_set_cmd(nrf_cli_t const * p_cli, size_t argc, char **argv)
{
    if (nrf_cli_help_requested(p_cli))
    {
        nrf_cli_help_print(p_cli, NULL, 0);
        nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Usage:\r\n  %s <parameter> <value>\r\n", argv[0]);
        return;
    }

    if (argc >= 2)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Unknown parameter '%s'!\r\n", argv[1]);
    }
    else
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Please specify parameter!\r\n");
    }
}

static ret_code_t drv_audio_parse_gain(char *p_string, nrf_pdm_gain_t *p_gain)
{
    long value;

    ASSERT((p_string != NULL) && (p_gain != NULL));

    value = strtol(p_string, &p_string, 10) << 1;
    if (*p_string == '.')
    {
        char c = *(++p_string);

        if ((c >= '5') && (c <= '9'))
        {
            value += (value >= 0) ? +1 : -1;
        }

        while ((*p_string != '\0') && isdigit((int)(*p_string)))
        {
            p_string++;
        }
    }

    if (*p_string != '\0')
    {
        return NRF_ERROR_INVALID_DATA;
    }

    value += (20 << 1);

    if ((value < NRF_PDM_GAIN_MINIMUM) || (value > NRF_PDM_GAIN_MAXIMUM))
    {
       return NRF_ERROR_INVALID_PARAM;
    }

    *p_gain = value;

    return NRF_SUCCESS;
}

static void drv_audio_set_gain_cmd(nrf_cli_t const * p_cli, size_t argc, char **argv)
{
    nrf_pdm_gain_t gain_l, gain_r;
    ret_code_t status;

    if (nrf_cli_help_requested(p_cli))
    {
        nrf_cli_help_print(p_cli, NULL, 0);
        nrf_cli_fprintf(p_cli,
                        NRF_CLI_NORMAL,
                        "Usage:\r\n  %s <gain>\r\n  %s <L-gain> <R-gain>\r\n",
                        argv[0],
                        argv[0]);
        return;
    }

    if ((argc < 2) || (argc > 3))
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Please specify gain!\r\n");
        return;
    }

    // Parse value string.
    status = drv_audio_parse_gain(argv[1], &gain_l);
    if (status == NRF_SUCCESS)
    {
        if (argc >= 3)
        {
            status = drv_audio_parse_gain(argv[2], &gain_r);
        }
        else
        {
            gain_r = gain_l;
        }
    }

    switch (status)
    {
        case NRF_SUCCESS:
            break;

        case NRF_ERROR_INVALID_PARAM:
            nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Gain value ouf of range!\r\n");
            return;

        default:
            nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Invalid gain!\r\n");
            return;
    }

    nrf_pdm_gain_set(gain_l, gain_r);

    NRF_LOG_INFO("Decimator Gain: Left = %d.%u dB, Right = %d.%u dB",
                 5 * (gain_l + (-20 << 1)) / 10,
                 abs(5 * (gain_l + (-20 << 1))) % 10,
                 5 * (gain_r + (-20 << 1)) / 10,
                 abs(5 * (gain_r + (-20 << 1))) % 10);
}

NRF_CLI_CREATE_STATIC_SUBCMD_SET(drv_audio_set_subcmds)
{
    NRF_CLI_CMD(gain, NULL, "set PDM decimator gain", drv_audio_set_gain_cmd),
    { NULL }
};

static const nrf_cli_static_entry_t drv_audio_subcmds_table[] =
{
    NRF_CLI_CMD(info,   NULL,                   "print information about driver",   drv_audio_info_cmd),
    NRF_CLI_CMD(set,    &drv_audio_set_subcmds, "set driver parameter",             drv_audio_set_cmd),
    { NULL }
};

const nrf_cli_cmd_entry_t drv_audio_subcmds =
{
    .is_dynamic = false,
    .u.p_static = drv_audio_subcmds_table,
};
#endif /* CONFIG_CLI_ENABLED */
#endif /* CONFIG_AUDIO_ENABLED */

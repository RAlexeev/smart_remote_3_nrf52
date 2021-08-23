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

#include "nrf_atomic.h"
#include "nrf_assert.h"
#include "nrf_balloc.h"
#include "nrf_cli.h"
#include "nrf_pwr_mgmt.h"
#include "app_debug.h"
#include "app_error.h"
#include "app_isched.h"

#include "drv_audio.h"
#include "drv_audio_anr.h"
#include "drv_audio_dsp.h"
#include "drv_audio_codec.h"

#include "m_audio.h"
#include "m_audio_gauges.h"
#include "m_audio_probe.h"
#include "m_coms.h"

#include "resources.h"
#include "sr3_config.h"

#if CONFIG_AUDIO_ENABLED

#define NRF_LOG_MODULE_NAME m_audio
#define NRF_LOG_LEVEL CONFIG_AUDIO_MODULE_LOG_LEVEL
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

/* Make sure that ANR gets a two-channel input. */
#if CONFIG_AUDIO_ANR_ENABLED
STATIC_ASSERT(CONFIG_PDM_BUFFER_SIZE_SAMPLES == (2 * CONFIG_AUDIO_FRAME_SIZE_SAMPLES));
#else /* !CONFIG_AUDIO_ANR_ENABLED */
STATIC_ASSERT(CONFIG_PDM_BUFFER_SIZE_SAMPLES == (1 * CONFIG_AUDIO_FRAME_SIZE_SAMPLES));
#endif /* CONFIG_AUDIO_ANR_ENABLED */

#if !CONFIG_AUDIO_HID_ENABLED && !CONFIG_AUDIO_ATVV_ENABLED
#error At least one audio service (HID or AATV) has to be enabled!
#endif /* !CONFIG_AUDIO_HID_ENABLED && !CONFIG_AUDIO_ATVV_ENABLED */

NRF_BALLOC_DEF(m_audio_buffer_pool,
               (sizeof(int16_t) * CONFIG_PDM_BUFFER_SIZE_SAMPLES),
               CONFIG_AUDIO_BUFFER_POOL_SIZE);

static bool                     m_audio_enabled;

#if CONFIG_AUDIO_GAUGES_ENABLED
static m_audio_loss_gauge_t     m_loss_gauge;
static m_audio_bitrate_gauge_t  m_bitrate_gauge;
static m_audio_cpu_gauge_t      m_total_cpu_gauge;
static m_audio_cpu_gauge_t      m_codec_cpu_gauge;

#if CONFIG_AUDIO_ANR_ENABLED
static m_audio_cpu_gauge_t      m_anr_cpu_gauge;
#endif
#if CONFIG_AUDIO_EQUALIZER_ENABLED
static m_audio_cpu_gauge_t      m_eq_cpu_gauge;
#endif
#if CONFIG_AUDIO_GAIN_CONTROL_ENABLED
static m_audio_cpu_gauge_t      m_gain_cpu_gauge;
#endif

static void m_audio_reset_gauges(void *p_context)
{
    m_audio_loss_gauge_reset(&m_loss_gauge);
    m_audio_bitrate_gauge_reset(&m_bitrate_gauge);
    m_audio_cpu_gauge_reset(&m_total_cpu_gauge);
    m_audio_cpu_gauge_reset(&m_codec_cpu_gauge);

#if CONFIG_AUDIO_ANR_ENABLED
    m_audio_cpu_gauge_reset(&m_anr_cpu_gauge);
#endif
#if CONFIG_AUDIO_EQUALIZER_ENABLED
    m_audio_cpu_gauge_reset(&m_eq_cpu_gauge);
#endif
#if CONFIG_AUDIO_GAIN_CONTROL_ENABLED
    m_audio_cpu_gauge_reset(&m_gain_cpu_gauge);
#endif
}

static void m_audio_log_gauges(void *p_context)
{
    m_audio_loss_gauge_log(&m_loss_gauge, "Frames");
    m_audio_bitrate_gauge_log(&m_bitrate_gauge, "");
    m_audio_cpu_gauge_log(&m_total_cpu_gauge, "");

#if CONFIG_AUDIO_ANR_ENABLED
    m_audio_cpu_gauge_log(&m_anr_cpu_gauge, "\t- ANR");
#endif
#if CONFIG_AUDIO_EQUALIZER_ENABLED
    m_audio_cpu_gauge_log(&m_eq_cpu_gauge, "\t- Equalizer");
#endif
#if CONFIG_AUDIO_GAIN_CONTROL_ENABLED
    m_audio_cpu_gauge_log(&m_gain_cpu_gauge, "\t- Gain");
#endif

    m_audio_cpu_gauge_log(&m_codec_cpu_gauge, "\t- Codec");
}
#endif /* CONFIG_AUDIO_GAUGES_ENABLED */

static void m_audio_send(void *p_context)
{
    m_audio_frame_t *p_frame;
    ret_code_t status;

    ASSERT(p_context != NULL);
    p_frame = p_context;

    if (m_audio_enabled)
    {
        status = m_coms_send_audio(p_frame);
        if (status != NRF_SUCCESS)
        {
            m_audio_count_lost(&m_loss_gauge);

            NRF_LOG_WARNING("%s(): WARNING: Cannot send audio frame!", __func__);
        }
    }
    else
    {
        m_audio_count_discarded(&m_loss_gauge);
    }

    // The frame is no longer used by this module.
    m_audio_frame_put(p_frame);

#if CONFIG_PWR_MGMT_ENABLED
    // Notifying the power manager about the activity.
    nrf_pwr_mgmt_feed();
#endif
}

static void m_audio_process(void *p_context)
{
    m_audio_frame_t *p_frame;
    int16_t *p_buffer;
    ret_code_t status;

    ASSERT(p_context != NULL);
    p_buffer = p_context;

    if (!m_audio_enabled)
    {
        // Audio was disabled right after the buffer was filled: free audio buffer and skip processing.
        nrf_balloc_free(&m_audio_buffer_pool, p_buffer);
        return;
    }

    m_audio_measure_cpu_usage_start(&m_total_cpu_gauge);
    DBG_PIN_SET(CONFIG_IO_DBG_AUDIO_PROCESS);

    m_audio_count_total(&m_loss_gauge);
    m_audio_probe_point(M_AUDIO_PROBE_POINT_PDM_OUT, p_buffer, CONFIG_PDM_BUFFER_SIZE_SAMPLES);

    p_frame = m_audio_frame_get(NULL);
    if (p_frame != NULL)
    {
        // ---- ANR ----
        m_audio_probe_point(M_AUDIO_PROBE_POINT_ANR_IN, p_buffer, CONFIG_PDM_BUFFER_SIZE_SAMPLES);
#if CONFIG_AUDIO_ANR_ENABLED
        m_audio_measure_cpu_usage_start(&m_anr_cpu_gauge);
        drv_audio_anr_perfrom(p_buffer, CONFIG_AUDIO_FRAME_SIZE_SAMPLES);
        m_audio_measure_cpu_usage_end(&m_anr_cpu_gauge);
#endif /* CONFIG_AUDIO_ANR_ENABLED */
        m_audio_probe_point(M_AUDIO_PROBE_POINT_ANR_OUT, p_buffer, CONFIG_AUDIO_FRAME_SIZE_SAMPLES);

        // ---- EQ ----
        m_audio_probe_point(M_AUDIO_PROBE_POINT_EQ_IN, p_buffer, CONFIG_AUDIO_FRAME_SIZE_SAMPLES);
#if CONFIG_AUDIO_EQUALIZER_ENABLED
        m_audio_measure_cpu_usage_start(&m_eq_cpu_gauge);
        drv_audio_dsp_equalizer((q15_t *)p_buffer, CONFIG_AUDIO_FRAME_SIZE_SAMPLES);
        m_audio_measure_cpu_usage_end(&m_eq_cpu_gauge);
#endif /* CONFIG_AUDIO_EQUALIZER_ENABLED */
        m_audio_probe_point(M_AUDIO_PROBE_POINT_EQ_OUT, p_buffer, CONFIG_AUDIO_FRAME_SIZE_SAMPLES);

        // ---- GAIN ----
        m_audio_probe_point(M_AUDIO_PROBE_POINT_GAIN_IN, p_buffer, CONFIG_AUDIO_FRAME_SIZE_SAMPLES);
#if CONFIG_AUDIO_GAIN_CONTROL_ENABLED
        m_audio_measure_cpu_usage_start(&m_gain_cpu_gauge);
        drv_audio_dsp_gain_control((q15_t *)p_buffer, CONFIG_AUDIO_FRAME_SIZE_SAMPLES);
        m_audio_measure_cpu_usage_end(&m_gain_cpu_gauge);
#endif /* CONFIG_AUDIO_GAIN_CONTROL_ENABLED */
        m_audio_probe_point(M_AUDIO_PROBE_POINT_GAIN_OUT, p_buffer, CONFIG_AUDIO_FRAME_SIZE_SAMPLES);

        // ---- CODEC ----
        m_audio_probe_point(M_AUDIO_PROBE_POINT_CODEC_IN, p_buffer, CONFIG_AUDIO_FRAME_SIZE_SAMPLES);
        m_audio_measure_cpu_usage_start(&m_codec_cpu_gauge);
        drv_audio_codec_encode(p_buffer, p_frame);
        m_audio_measure_cpu_usage_end(&m_codec_cpu_gauge);
        m_audio_measure_bitrate(&m_bitrate_gauge, p_frame->data_size);

        // Free audio buffer since it is no longer needed.
        nrf_balloc_free(&m_audio_buffer_pool, p_buffer);

        // Schedule audio transmission. It cannot be done from this context.
        status = app_isched_event_put(&g_fg_scheduler, m_audio_send, p_frame);
        if (status != NRF_SUCCESS)
        {
            m_audio_frame_put(p_frame);
            m_audio_count_lost(&m_loss_gauge);

            NRF_LOG_WARNING("%s(): WARNING: Cannot schedule audio frame transmission!", __func__);
        }
    }
    else
    {
        // Frame allocation failed: Free audio buffer.
        nrf_balloc_free(&m_audio_buffer_pool, p_buffer);
        m_audio_count_lost(&m_loss_gauge);
        status = NRF_ERROR_NO_MEM;

        NRF_LOG_WARNING("%s(): WARNING: Cannot allocate audio frame!", __func__);
    }

    if (status != NRF_SUCCESS)
    {
        /*
         * Do not clear CONFIG_IO_DBG_AUDIO_PROCESS. This will make debugging pulse wider
         * than expected and easier to spot on the logic analyzer.
         */

        DBG_PIN_CLEAR(CONFIG_IO_DBG_AUDIO_PROCESS);
    }

    m_audio_measure_cpu_usage_end(&m_total_cpu_gauge);
}

static void m_audio_buffer_handler(int16_t *p_buffer)
{
    // Put audio processing in the background.
    APP_ERROR_CHECK(app_isched_event_put(&g_bg_scheduler, m_audio_process, p_buffer));
}

ret_code_t m_audio_init(void)
{
    ret_code_t status;

    m_audio_enabled = false;

#if CONFIG_AUDIO_PROBE_ENABLED
    m_audio_probe_init();
#endif /* CONFIG_AUDIO_PROBE_ENABLED */

    status = nrf_balloc_init(&m_audio_buffer_pool);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    return drv_audio_init(&m_audio_buffer_pool, m_audio_buffer_handler);
}

ret_code_t m_audio_enable(void)
{
    ret_code_t status;

    ASSERT(m_audio_enabled == false);
    NRF_LOG_INFO("Enabled");

#if CONFIG_AUDIO_ANR_ENABLED
    drv_audio_anr_init();
#endif
    drv_audio_codec_init();

#if CONFIG_AUDIO_GAUGES_ENABLED
    // To avoid race conditions, reset gauges using the same context in which they are updated.
    status = app_isched_event_put(&g_bg_scheduler, m_audio_reset_gauges, NULL);
    if (status != NRF_SUCCESS)
    {
        return status;
    }
#endif

    status = drv_audio_enable();
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    m_audio_enabled = true;

    return NRF_SUCCESS;
}

ret_code_t m_audio_disable(void)
{
    ret_code_t status;

    ASSERT(m_audio_enabled == true);
    NRF_LOG_INFO("Disabled");

    status = drv_audio_disable();
    if (status != NRF_SUCCESS)
    {
        return status;
    }

#if CONFIG_AUDIO_GAUGES_ENABLED
    /*
     * Audio is disabled but some buffers might be present in the background scheduler queue.
     * Put the gauges log request at the end of the processing queue to ensure that all buffers are counted.
     */
    status = app_isched_event_put(&g_bg_scheduler, m_audio_log_gauges, NULL);
    if (status != NRF_SUCCESS)
    {
        return status;
    }
#endif

    m_audio_enabled = false;

    return NRF_SUCCESS;
}

#if CONFIG_PWR_MGMT_ENABLED
static bool m_audio_shutdown(nrf_pwr_mgmt_evt_t event)
{
    if (m_audio_enabled)
    {
       m_audio_disable();
    }

    return true;
}

static bool m_audio_log_statistics(nrf_pwr_mgmt_evt_t event)
{
    NRF_LOG_INFO("Maximum Audio Buffers Pool usage: %d entries",
              nrf_balloc_max_utilization_get(&m_audio_buffer_pool));

    NRF_LOG_INFO("Maximum Audio Frames Pool usage: %d entries",
              m_audio_frame_pool_current_utilization_get());

    return true;
}

NRF_PWR_MGMT_HANDLER_REGISTER(m_audio_shutdown, SHUTDOWN_PRIORITY_DEFAULT);
NRF_PWR_MGMT_HANDLER_REGISTER(m_audio_log_statistics, SHUTDOWN_PRIORITY_STATISTICS);
#endif /* CONFIG_PWR_MGMT_ENABLED */

/*
 * ===== Console commands =====
 */

#if CONFIG_CLI_ENABLED
static void m_audio_cmd(nrf_cli_t const * p_cli, size_t argc, char **argv)
{
    if (nrf_cli_help_requested(p_cli))
    {
        nrf_cli_help_print(p_cli, NULL, 0);
        nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Usage:\r\n  %s <subcommand> ...\r\n", argv[0]);
        return;
    }

    if (argc >= 2)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Unknown subcommand '%s'!\r\n", argv[1]);
    }
    else
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Specify a subcommand!\r\n");
    }
}

static void m_audio_info_cmd(nrf_cli_t const * p_cli, size_t argc, char **argv)
{
    uint8_t buffer_pool_usage, buffer_pool_max_usage;
    uint8_t frame_pool_usage, frame_pool_max_usage;
    bool audio_enabled;


    buffer_pool_max_usage   = nrf_balloc_max_utilization_get(&m_audio_buffer_pool);
    buffer_pool_usage       = m_audio_buffer_pool.p_stack_limit
                              - m_audio_buffer_pool.p_cb->p_stack_pointer;

    frame_pool_max_usage    = m_audio_frame_pool_max_utilization_get();
    frame_pool_usage        = m_audio_frame_pool_current_utilization_get();

    audio_enabled           = m_audio_enabled;

    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Configuration:\r\n");

    nrf_cli_fprintf(p_cli,
                    NRF_CLI_NORMAL,
                    "\tSampling Frequency:\t%u Hz\r\n",
                    CONFIG_AUDIO_REAL_SAMPLING_FREQUENCY);

    nrf_cli_fprintf(p_cli,
                    NRF_CLI_NORMAL,
                    "\tFrame Length:\t\t%u.%02u ms (%u samples, up to %u bytes/frame = %u kbit/s)\r\n",
                    (1000ul * CONFIG_AUDIO_FRAME_SIZE_SAMPLES / CONFIG_AUDIO_REAL_SAMPLING_FREQUENCY),
                    (100000ul * CONFIG_AUDIO_FRAME_SIZE_SAMPLES / CONFIG_AUDIO_REAL_SAMPLING_FREQUENCY) % 100,
                    CONFIG_AUDIO_FRAME_SIZE_SAMPLES,
                    CONFIG_AUDIO_FRAME_SIZE_BYTES,
                    8ul * CONFIG_AUDIO_FRAME_SIZE_BYTES * CONFIG_AUDIO_REAL_SAMPLING_FREQUENCY / CONFIG_AUDIO_FRAME_SIZE_SAMPLES / 1000);

    nrf_cli_fprintf(p_cli,
                    NRF_CLI_NORMAL,
                    "\r\nStatus: %s\r\n",
                    (audio_enabled) ? "Enabled" : "Disabled");

    if (!audio_enabled)
    {
        return;
    }

#if CONFIG_AUDIO_GAUGES_ENABLED
    uint32_t frames_total = m_audio_gauge_get_total_count(&m_loss_gauge);
    uint32_t frames_lost  = m_audio_gauge_get_lost_count(&m_loss_gauge);

    nrf_cli_fprintf(p_cli,
                    NRF_CLI_NORMAL,
                    "\tCapture time:\t\t%u:%02u\r\n",
                    frames_total * CONFIG_AUDIO_FRAME_SIZE_SAMPLES / CONFIG_AUDIO_REAL_SAMPLING_FREQUENCY / 60,
                    frames_total * CONFIG_AUDIO_FRAME_SIZE_SAMPLES / CONFIG_AUDIO_REAL_SAMPLING_FREQUENCY % 60);

    nrf_cli_fprintf(p_cli,
                    NRF_CLI_NORMAL,
                    "\tFrame loss ratio:\t%u%% (%u out of %u frames)\r\n",
                    (frames_total != 0) ? (100ul * frames_lost / frames_total) : 0,
                    frames_lost,
                    frames_total);

    nrf_cli_fprintf(p_cli,
                    NRF_CLI_NORMAL,
                    "\tBit rate:\t\t%u kbit/s (min/avg/max: %u/%u/%u kbit/s)\r\n",
                    m_audio_gauge_get_cur_bitrate(&m_bitrate_gauge),
                    m_audio_gauge_get_min_bitrate(&m_bitrate_gauge),
                    m_audio_gauge_get_avg_bitrate(&m_bitrate_gauge),
                    m_audio_gauge_get_max_bitrate(&m_bitrate_gauge));

    nrf_cli_fprintf(p_cli,
                    NRF_CLI_NORMAL,
                    "\r\n\tCPU Usage:\t\t%u%% (min/avg/max: %u%%/%u%%/%u%%)\r\n",
                    m_audio_gauge_get_cur_cpu_usage(&m_total_cpu_gauge),
                    m_audio_gauge_get_min_cpu_usage(&m_total_cpu_gauge),
                    m_audio_gauge_get_avg_cpu_usage(&m_total_cpu_gauge),
                    m_audio_gauge_get_max_cpu_usage(&m_total_cpu_gauge));

#if CONFIG_AUDIO_ANR_ENABLED
    nrf_cli_fprintf(p_cli,
                    NRF_CLI_NORMAL,
                    "\t    - ANR:\t\t%u%% (min/avg/max: %u%%/%u%%/%u%%)\r\n",
                    m_audio_gauge_get_cur_cpu_usage(&m_anr_cpu_gauge),
                    m_audio_gauge_get_min_cpu_usage(&m_anr_cpu_gauge),
                    m_audio_gauge_get_avg_cpu_usage(&m_anr_cpu_gauge),
                    m_audio_gauge_get_max_cpu_usage(&m_anr_cpu_gauge));
#endif

#if CONFIG_AUDIO_EQUALIZER_ENABLED
    nrf_cli_fprintf(p_cli,
                    NRF_CLI_NORMAL,
                    "\t    - Equalizer:\t%u%% (min/avg/max: %u%%/%u%%/%u%%)\r\n",
                    m_audio_gauge_get_cur_cpu_usage(&m_eq_cpu_gauge),
                    m_audio_gauge_get_min_cpu_usage(&m_eq_cpu_gauge),
                    m_audio_gauge_get_avg_cpu_usage(&m_eq_cpu_gauge),
                    m_audio_gauge_get_max_cpu_usage(&m_eq_cpu_gauge));
#endif

#if CONFIG_AUDIO_GAIN_CONTROL_ENABLED
    nrf_cli_fprintf(p_cli,
                    NRF_CLI_NORMAL,
                    "\t    - Gain:\t\t%u%% (min/avg/max: %u%%/%u%%/%u%%)\r\n",
                    m_audio_gauge_get_cur_cpu_usage(&m_gain_cpu_gauge),
                    m_audio_gauge_get_min_cpu_usage(&m_gain_cpu_gauge),
                    m_audio_gauge_get_avg_cpu_usage(&m_gain_cpu_gauge),
                    m_audio_gauge_get_max_cpu_usage(&m_gain_cpu_gauge));
#endif

    nrf_cli_fprintf(p_cli,
                    NRF_CLI_NORMAL,
                    "\t    - Codec:\t\t%u%% (min/avg/max: %u%%/%u%%/%u%%)\r\n",
                    m_audio_gauge_get_cur_cpu_usage(&m_codec_cpu_gauge),
                    m_audio_gauge_get_min_cpu_usage(&m_codec_cpu_gauge),
                    m_audio_gauge_get_avg_cpu_usage(&m_codec_cpu_gauge),
                    m_audio_gauge_get_max_cpu_usage(&m_codec_cpu_gauge));
#endif /* CONFIG_AUDIO_GAUGES_ENABLED */

    nrf_cli_fprintf(p_cli,
                    NRF_CLI_NORMAL,
                    "\r\n\tBuffer Pool Usage:\t%u%% (%u out of %u buffers)\r\n",
                    100ul * buffer_pool_usage / CONFIG_AUDIO_BUFFER_POOL_SIZE,
                    buffer_pool_usage,
                    CONFIG_AUDIO_BUFFER_POOL_SIZE);

    nrf_cli_fprintf(p_cli,
                    NRF_CLI_NORMAL,
                    "\t    - Maximum:\t\t%u%% (%u out of %u buffers)\r\n",
                    100ul * buffer_pool_max_usage / CONFIG_AUDIO_BUFFER_POOL_SIZE,
                    buffer_pool_max_usage,
                    CONFIG_AUDIO_BUFFER_POOL_SIZE);

    nrf_cli_fprintf(p_cli,
                    NRF_CLI_NORMAL,
                    "\r\n\tFrame Pool Usage:\t%u%% (%u out of %u frames)\r\n",
                    100ul * frame_pool_usage / CONFIG_AUDIO_FRAME_POOL_SIZE,
                    frame_pool_usage,
                    CONFIG_AUDIO_FRAME_POOL_SIZE);

    nrf_cli_fprintf(p_cli,
                    NRF_CLI_NORMAL,
                    "\t    - Maximum:\t\t%u%% (%u out of %u frames)\r\n",
                    100ul * frame_pool_max_usage / CONFIG_AUDIO_FRAME_POOL_SIZE,
                    frame_pool_max_usage,
                    CONFIG_AUDIO_FRAME_POOL_SIZE);
}

NRF_CLI_CREATE_STATIC_SUBCMD_SET(m_audio_subcmds)
{
    NRF_CLI_CMD(codec,  &drv_audio_codec_subcmds,   "show or configure audio codec parameters",     m_audio_cmd),
    NRF_CLI_CMD(driver, &drv_audio_subcmds,         "show or configure audio driver parameters",    m_audio_cmd),
    NRF_CLI_CMD(info,   NULL,                       "print information about audio subsystem",      m_audio_info_cmd),
#if CONFIG_AUDIO_PROBE_ENABLED
    NRF_CLI_CMD(probe,  &m_audio_probe_subcmds,     "utility for injecting and tapping audio data", m_audio_probe_cmd),
#endif
    {NULL}
};

NRF_CLI_CMD_REGISTER(audio, &m_audio_subcmds, "show or configure audio settings", m_audio_cmd);
#endif /* CONFIG_CLI_ENABLED */
#endif /* CONFIG_AUDIO_ENABLED */

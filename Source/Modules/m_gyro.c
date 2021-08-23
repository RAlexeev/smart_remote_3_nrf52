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

#include <stddef.h>
#include <string.h>

#include "nrf_assert.h"
#include "nrf_sdh.h"
#include "nrf_sdm.h"
#include "nrf_pwr_mgmt.h"
#include "app_debug.h"
#include "app_error.h"
#include "app_timer.h"
#include "fds.h"

#include "event_bus.h"
#include "m_gyro.h"

#include "resources.h"
#include "sr3_config.h"

#if CONFIG_GYRO_ENABLED

#define NRF_LOG_MODULE_NAME m_gyro
#define NRF_LOG_LEVEL CONFIG_GYRO_MODULE_LOG_LEVEL
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

#include "drv_gyro.h"

#define M_GYRO_FILE_ID      0x4759  // "GY"
#define M_GYRO_RECORD_KEY   0x524F  // "RO"

// Check if we can cast FDS error codes to SDK error codes.
STATIC_ASSERT(FDS_SUCCESS == NRF_SUCCESS);

// Check if a compatible version of AirMotion library is used.
STATIC_ASSERT((AIRMOTIONLIB_VERSION_MAJOR == 4) &&
              (AIRMOTIONLIB_VERSION_MINOR == 1));


typedef struct
{
    int16_t     x_offset;
    int16_t     y_offset;
    int16_t     z_offset;

    uint8_t     __padding[2];
} m_gyro_file_t;

// Gyro file size has to be multiple of word size.
STATIC_ASSERT((sizeof(m_gyro_file_t) % sizeof(uint32_t)) == 0);

static ret_code_t m_gyro_start(void);
static ret_code_t m_gyro_stop(void);

APP_TIMER_DEF                                   (s_timer);
static t_struct_AIR_MOTION_Init                 s_lInitParameters;
static t_struct_AIR_MOTION_ProcessDeltaSamples  s_samples;
static bool                                     s_gyro_enabled;
static bool                                     s_gyro_calibration;
static bool                                     s_gyro_click_detected;
static bool                                     s_gyro_shutdown;
__ALIGN(4) static m_gyro_file_t                 s_gyro_file;

static void m_gyro_calibration_end(void)
{
    s_gyro_calibration = false;

    if (!s_gyro_enabled)
    {
        APP_ERROR_CHECK(m_gyro_stop());
    }
}

static void m_gyro_fds_evt_handler(fds_evt_t const * const p_fds_evt)
{
    switch (p_fds_evt->id)
    {
        case FDS_EVT_WRITE:
        case FDS_EVT_UPDATE:
            if ((p_fds_evt->write.file_id == M_GYRO_FILE_ID) &&
                (p_fds_evt->write.record_key == M_GYRO_RECORD_KEY))
            {
                APP_ERROR_CHECK(p_fds_evt->result);
                NRF_LOG_INFO("Calibration complete.");
            }
            break;

        default:
            /* Ignore */
            break;
    }
}

static ret_code_t m_gyro_file_read(m_gyro_file_t *file)
{
    fds_find_token_t ftok = { 0 };
    fds_flash_record_t record;
    fds_record_desc_t rdesc;
    ret_code_t status;

    ASSERT(file != NULL);

    status = fds_record_find(M_GYRO_FILE_ID, M_GYRO_RECORD_KEY, &rdesc, &ftok);
    if (status != FDS_SUCCESS)
    {
        return status;
    }

    status = fds_record_open(&rdesc, &record);
    if (status != FDS_SUCCESS)
    {
        return status;
    }

    ASSERT((record.p_header->length_words * sizeof(uint32_t)) >= sizeof(*file))
    memcpy(file, record.p_data, sizeof(*file));

    return fds_record_close(&rdesc);
}

static ret_code_t m_gyro_file_write(const m_gyro_file_t *file)
{
    fds_record_t record =
    {
        .file_id            = M_GYRO_FILE_ID,
        .key                = M_GYRO_RECORD_KEY,
        .data.p_data        = file,
        .data.length_words  = CEIL_DIV(sizeof(*file), sizeof(uint32_t)),
    };

    fds_find_token_t ftok = { 0 };
    fds_record_desc_t rdesc;
    ret_code_t status;

    ASSERT(file != NULL);

    status = fds_record_find(M_GYRO_FILE_ID, M_GYRO_RECORD_KEY, &rdesc, &ftok);
    switch (status)
    {
        case FDS_SUCCESS:
            // Update existing record.
            return fds_record_update(&rdesc, &record);

        case FDS_ERR_NOT_FOUND:
            // Create new file/record.
            return fds_record_write(&rdesc, &record);

        default:
            return status;
    }
}

static void m_gyro_evt_handler(void *p_context)
{
    t_struct_AIR_MOTION_ProcessDeltaSamples *p_samples = p_context;
    t_struct_AIR_MOTION_ProcessDeltaStatus lProcessDeltaStatus;

    // Pass click notification to Air Motion Library.
    p_samples->ClickSample  = s_gyro_click_detected;
    s_gyro_click_detected   = false;

    // Process rotation and acceleration values.
    lProcessDeltaStatus = AIR_MOTION_ProcessDelta(*p_samples);
    if (lProcessDeltaStatus.Status.NewGyroOffset)
    {
        s_lInitParameters.GyroOffsets.X = lProcessDeltaStatus.GyroOffsets.X;
        s_lInitParameters.GyroOffsets.Y = lProcessDeltaStatus.GyroOffsets.Y;
        s_lInitParameters.GyroOffsets.Z = lProcessDeltaStatus.GyroOffsets.Z;

        NRF_LOG_DEBUG("New Gyro Offsets:");
        NRF_LOG_DEBUG(" X = %d", s_lInitParameters.GyroOffsets.X);
        NRF_LOG_DEBUG(" Y = %d", s_lInitParameters.GyroOffsets.Y);
        NRF_LOG_DEBUG(" Z = %d", s_lInitParameters.GyroOffsets.Z);

        if (s_gyro_calibration)
        {
            if (nrf_sdh_is_enabled())
            {
                s_gyro_file.x_offset = s_lInitParameters.GyroOffsets.X;
                s_gyro_file.y_offset = s_lInitParameters.GyroOffsets.Y;
                s_gyro_file.z_offset = s_lInitParameters.GyroOffsets.Z;

                ret_code_t status = m_gyro_file_write(&s_gyro_file);
                switch (status)
                {
                    case FDS_SUCCESS:
                        m_gyro_calibration_end();
                        break;

                    case FDS_ERR_NOT_INITIALIZED:
                        /* Continue calibration. */
                        break;

                    case FDS_ERR_NO_SPACE_IN_FLASH:
                        /* Try to reclaim the space by running garbage collector and continue calibration. */
                        APP_ERROR_CHECK(fds_gc());
                        break;

                    default:
                        APP_ERROR_CHECK(status);
                        break;
                }
            }
            else
            {
                NRF_LOG_WARNING("The SoftDevice is disabled. Calibration data cannot be saved!");
                m_gyro_calibration_end();
            }
        }
    }

    if (s_gyro_enabled && lProcessDeltaStatus.Status.IsDeltaComputed)
    {
        if (lProcessDeltaStatus.Delta.X != 0)
        {
            event_send(EVT_REL_X, lProcessDeltaStatus.Delta.X);
        }

        if (lProcessDeltaStatus.Delta.Y != 0)
        {
            event_send(EVT_REL_Y, -lProcessDeltaStatus.Delta.Y);
        }
    }
}

static void m_gyro_read_handler(ret_code_t status,
                              t_struct_AIR_MOTION_ProcessDeltaSamples * p_samples)
{
    APP_ERROR_CHECK(status);

    // Move the rest of data processing out of the interrupt context.
    APP_ERROR_CHECK(app_isched_event_put(&g_fg_scheduler, m_gyro_evt_handler, p_samples));
}

static void m_gyro_ready_handler(void)
{
    APP_ERROR_CHECK(app_timer_start(s_timer, APP_TIMER_TICKS(CONFIG_GYRO_POLL_INTERVAL), NULL));
}

static void m_gyro_timer_handler(void * p_context)
{
    ret_code_t status = drv_gyro_schedule_read(&s_samples);

    if (status == NRF_ERROR_BUSY)
    {
        // Congestion on a bus may block read.
        NRF_LOG_WARNING("%s(): cannot schedule gyro read: driver is busy", __func__);
    }
    else
    {
        APP_ERROR_CHECK(status);
    }
}

ret_code_t m_gyro_init(bool force_calibration)
{
    ret_code_t err_code;
    bool run_calibration = false;

    err_code = fds_register(m_gyro_fds_evt_handler);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    s_gyro_enabled                          = false;
    s_gyro_calibration                      = false;

    s_lInitParameters.DeltaGain.X           = CONFIG_GYRO_X_GAIN;
    s_lInitParameters.DeltaGain.Y           = CONFIG_GYRO_Y_GAIN;
    s_lInitParameters.GyroStaticMaxNoise    = 4;
    s_lInitParameters.StaticSamples         = 100;
    s_lInitParameters.SwipeMinDist          = 64;
    s_lInitParameters.SwipeMaxNoise         = 16;
    s_lInitParameters.StartupSamples        = 16;
    s_lInitParameters.ClickStillSamples     = 32;
    s_lInitParameters.ClickStillTolerance   = AirMotionNormal;
    s_lInitParameters.IsRollCompEnabled     = true;
    s_lInitParameters.Acc1gLsb              = (1 << (16-1)) / 2;
    s_lInitParameters.GyroSensitivity       = (1 << (16-1)) * 16 / 1000;
    s_lInitParameters.GyroTremorCanceling   = 7;

    /*
     * Check gyroscope offsets saved in flash. If they are valid, use them.
     * Otherwise, request for calibration.
     */
    err_code = m_gyro_file_read(&s_gyro_file);
    if ((err_code == FDS_ERR_NOT_FOUND) ||
        (err_code == FDS_ERR_CRC_CHECK_FAILED) ||
        (err_code == FDS_ERR_NOT_INITIALIZED) ||
        force_calibration)
    {
        memset(&s_lInitParameters.GyroOffsets, 0, sizeof(s_lInitParameters.GyroOffsets));
        run_calibration = true;
    }
    else if (err_code == FDS_SUCCESS)
    {
        s_lInitParameters.GyroOffsets.X = s_gyro_file.x_offset;
        s_lInitParameters.GyroOffsets.Y = s_gyro_file.y_offset;
        s_lInitParameters.GyroOffsets.Z = s_gyro_file.z_offset;

        NRF_LOG_DEBUG("Gyro Offsets:");
        NRF_LOG_DEBUG(" X = %d", s_lInitParameters.GyroOffsets.X);
        NRF_LOG_DEBUG(" Y = %d", s_lInitParameters.GyroOffsets.Y);
        NRF_LOG_DEBUG(" Z = %d", s_lInitParameters.GyroOffsets.Z);
    }
    else
    {
        return err_code;
    }

    err_code = app_timer_create(&s_timer, APP_TIMER_MODE_REPEATED, m_gyro_timer_handler);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    err_code = drv_gyro_init(m_gyro_ready_handler, m_gyro_read_handler);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    if (run_calibration)
    {
        return m_gyro_calibrate();
    }

    return NRF_SUCCESS;
}

bool m_gyro_event_handler(const event_t *p_event)
{
    switch (p_event->type)
    {
        case EVT_KEY_DOWN:
            if ((p_event->key.id >= MOUSE_KEY_ID(0)) && (p_event->key.id <= MOUSE_KEY_ID(~0)))
            {
                s_gyro_click_detected = true;
            }
            break;

        case EVT_HID_REPORT_INPUT:
            if ((HID_USAGE_PAGE(p_event->hid.usage) == 0x09) && (p_event->hid.report != 0))
            {
                s_gyro_click_detected = true;
            }
            break;

        default:
            /* Ignore */
            break;
    }

    return false;
}

static ret_code_t m_gyro_start(void)
{
    AIR_MOTION_Init(&s_lInitParameters);

    return drv_gyro_enable();
}

ret_code_t m_gyro_enable(void)
{
    ASSERT(s_gyro_enabled == false);
    NRF_LOG_INFO("Enabled");

    s_gyro_enabled          = true;
    s_gyro_click_detected   = false;

    if (s_gyro_calibration)
    {
        return NRF_SUCCESS;
    }

    return m_gyro_start();
}

static ret_code_t m_gyro_stop(void)
{
    ret_code_t err_code;

    err_code = app_timer_stop(s_timer);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    return drv_gyro_disable();
}

ret_code_t m_gyro_disable(void)
{
    ASSERT(s_gyro_enabled == true);
    NRF_LOG_INFO("Disabled");

    s_gyro_enabled = false;

    if (s_gyro_calibration)
    {
        return NRF_SUCCESS;
    }

    return m_gyro_stop();
}

ret_code_t m_gyro_calibrate(void)
{
    if (s_gyro_calibration)
    {
        // Already running calibration
        return NRF_SUCCESS;
    }

    if (s_gyro_shutdown)
    {
        return NRF_ERROR_INVALID_STATE;
    }

    NRF_LOG_INFO("Starting calibration...");
    s_gyro_calibration = true;

    if (!s_gyro_enabled)
    {
        return m_gyro_start();
    }

    return NRF_SUCCESS;
}

#if CONFIG_PWR_MGMT_ENABLED
static bool m_gyro_shutdown(nrf_pwr_mgmt_evt_t event)
{
    s_gyro_shutdown = true;

    if (s_gyro_calibration)
    {
        m_gyro_calibration_end();
    }

    if (s_gyro_enabled)
    {
        m_gyro_disable();
    }

    return true;
}
NRF_PWR_MGMT_HANDLER_REGISTER(m_gyro_shutdown, SHUTDOWN_PRIORITY_DEFAULT);
#endif /* CONFIG_PWR_MGMT_ENABLED */
#endif /* CONFIG_GYRO_ENABLED */

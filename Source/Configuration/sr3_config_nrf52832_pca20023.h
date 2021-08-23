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

/** @file
 *
 * @defgroup sr3_config_nrf52832_pca20023 Smart Remote 3 for nRF52832 Product Example configuration
 * @{
 * @ingroup sr3_config
 *
 * @brief Smart Remote 3 configuration with target-specific default values.
 */

#ifndef _SR3_CONFIG_BOARD_H
#define _SR3_CONFIG_BOARD_H

// <<< Use Configuration Wizard in Context Menu >>>

// <h> Smart Remote 3 nRF52 Features
// <i> This section contains the configuration settings for all Smart Remote 3 nRF52 features.

// <e> Enable Accelerometer Support
// <i> Enables the low power accelerometer module.
/**@brief Accelerometer: Enable Accelerometer Support */
#define CONFIG_ACC_ENABLED 1

// <o> Accelerometer driver
// <i> Select the accelerometer driver
//  <1=>LIS3DH
//  <2=>BMA222E
/**@brief Accelerometer: driver */
#define CONFIG_ACC_DRIVER 1

// <o> TWI Bus
// <i> Select the TWI Bus that will be used to communicate with the accelerometer.
//  <0=>Primary
//  <1=>Secondary
/**@brief Accelerometer: TWI Bus */
#define CONFIG_ACC_TWI_BUS 1

// <e> Use Accelerometer Click Detection
// <i> Enable EXPERIMENTAL click detection with the accelerometer. With this option enabled, the user can tap the casing of the device for the click effect.
/**@brief Accelerometer: Use Accelerometer Click Detection */
#define CONFIG_ACC_USE_CLICK_DETECTION (0 && CONFIG_ACC_ENABLED)

// <o> Click Event Threshold <1-255>
// <i> Set the click event threshold for the click detection functionality.
/**@brief Accelerometer: Click Event Threshold <1-255> */
#define CONFIG_ACC_CLICK_THRESHOLD 8

// <o> Click Event Time Limit <1-255>
// <i> Set the click event time limit for the click detection functionality.
/**@brief Accelerometer: Click Event Time Limit <1-255> */
#define CONFIG_ACC_CLICK_TIMELIMIT 2

// <o> Click Event Latency <1-255>
// <i> Set the click event latency for the click detection functionality.
/**@brief Accelerometer: Click Event Latency <1-255> */
#define CONFIG_ACC_CLICK_LATENCY 7

// <q.0> Detect Click Events on the x-axis
// <q.2> Detect Click Events on the y-axis
// <q.4> Detect Click Events on the z-axis
/**@brief Accelerometer: Detect Click Events on the x, y and/or z-axis */
#define CONFIG_ACC_CLICK_AXES 0x10
// </e>

// <e> Use Low Power Accelerometer as a Wakeup Source
// <i> Enable motion-based wakeup.
/**@brief Accelerometer: Use Low Power Accelerometer as a Wakeup Source */
#define CONFIG_ACC_WAKEUP_SOURCE 1

// <o> Wakeup Threshold <1-255>
// <i> Set the minimal acceleration that causes system wakeup.
/**@brief Accelerometer: Wakeup Threshold <1-255> */
#define CONFIG_ACC_WAKEUP_THRESHOLD 8
// </e>

// <h> Logging Options
// <i> This section configures module-specific logging options.

// <o> Module logging level
//  <0=> None
//  <1=> Error
//  <2=> Warning
//  <3=> Info
//  <4=> Debug
/**@brief Accelerometer: Module logging level */
#define CONFIG_ACC_MODULE_LOG_LEVEL 4

// <o> Driver logging level
//  <0=> None
//  <1=> Error
//  <2=> Warning
//  <3=> Info
//  <4=> Debug
/**@brief Accelerometer: Driver logging level */
#define CONFIG_ACC_DRV_LOG_LEVEL 4
// </h>
// </e>

// <e> Enable Audio Support
// <i> Enable the audio subsystem. With this subsystem enabled, the remote will transmit voice commands to the host over Bluetooth.
/**@brief Enable Audio Support */
#define CONFIG_AUDIO_ENABLED 1

// <o> Audio Codec
// <i> ADPCM - the simplest codec that offers minimal usage of CPU and memory resources.
// <i> BV32FP - dedicated for voice. Moderate usage of memory and high usage of CPU resources. Better quality than ADCPM.
// <i> Opus - highly advanced and configurable codec. High memory consumption. CPU consumption depends on the settings.
// <i> SBC - low-complexity subband codec specified by Bluetooth SIG for A2DP.
// <i> Note: When using Keil, use the option "Rebuild all target files" to change stack size between codecs.
//  <1=>ADPCM
//  <2=>BV32FP
//  <3=>Opus
//  <4=>SBC
/**@brief Audio Codec */
#define CONFIG_AUDIO_CODEC 3

// <e> Enable Active Noise Reduction
// <i> The Active Noise Reduction (ANR) system involves two microphones. One of them picks up the voice of the user while the other one picks up background noise.
// <i> Background noise is then removed from the signal.
/**@brief Enable Active Noise Reduction */
#define CONFIG_AUDIO_ANR_ENABLED 0

// <o> Length of the adaptive filter for the noise canceller [samples] <16-1024:16>
/**@brief Length of the adaptive filter for the noise canceller [samples] <16-1024:16> */
#define CONFIG_AUDIO_ANR_LENGTH 32

// <o> Distance between noise and voice microphones [samples] <1-256>
/**@brief Distance between noise and voice microphones [samples] <1-256> */
#define CONFIG_AUDIO_ANR_DELAY_LENGTH 1
// </e>

// <q> Enable Equalizer
// <i> Enable the software equalizer. The equalizer characteristic is defined in the drv_audio_dsp.c file.
/**@brief Enable Equalizer */
#define CONFIG_AUDIO_EQUALIZER_ENABLED 0

// <q> Enable Gain Control
// <i> Enable software gain control. The gain is defined in the drv_audio_dsp.c file. It is recommended to use hardware gain control in the PDM configuration.
/**@brief Enable Gain Control */
#define CONFIG_AUDIO_GAIN_CONTROL_ENABLED 0

// <o> Sampling Frequency
// <i> Select audio sampling frequency.
// <i> Note that not all combinations of sampling frequency and codec are supported.
//  <8000=>8 kHz
//  <16000=>16 kHz
//  <24000=>24 kHz
//  <32000=>32 kHz
/**@brief Sampling Frequency */
#define CONFIG_AUDIO_SAMPLING_FREQUENCY 16000

// <h> ADPCM Options
// <o> Audio Frame Size
//  <128=>128 Samples
//  <256=>256 Samples
//  <512=>512 Samples
#if (CONFIG_AUDIO_CODEC == CONFIG_AUDIO_CODEC_ADPCM)
/**@brief ADPCM Options: Audio Frame Size */
#define CONFIG_AUDIO_FRAME_SIZE_SAMPLES 128
#endif /* CONFIG_AUDIO_CODEC == CONFIG_AUDIO_CODEC_ADPCM */
// </h>

// <h> Opus Options
// <o> Mode
// <i> SILK mode is specifically dedicated for voice but requires more CPU and memory resources than CELT.
// <i> CELT is a general-purpose mode that is more power efficient and uses less resources.
// <i> Hybrid mode is not supported.
// <i> Note: When using Keil, use the option "Rebuild all target files" to change stack size between CELT/SILK.
//  <1=>CELT Only
//  <2=>SILK Only
/**@brief Opus Options: Mode */
#define CONFIG_OPUS_MODE 1

// <o> Bit Rate
// <i> VBR - Variable Bit Rate is fully controlled by the codec.
// <i> CVBR - Constrained Variable Bit Rate is variable to some extent but allows you to set an average bit rate.
// <i> CBR - Constant Bit Rate allows you to set a specific bit rate that remains the same throughout the transmission.
//  <0=>VBR
//  <16000=>CVBR: 16 kbit/s
//  <24000=>CVBR: 24 kbit/s
//  <32000=>CVBR: 32 kbit/s
//  <40000=>CVBR: 40 kbit/s
//  <48000=>CVBR: 48 kbit/s
//  <56000=>CVBR: 56 kbit/s
//  <64000=>CVBR: 64 kbit/s
//  <80000=>CVBR: 80 kbit/s
//  <96000=>CVBR: 96 kbit/s
//  <112000=>CVBR: 112 kbit/s
//  <128000=>CVBR: 128 kbit/s
//  <16001=>CBR: 16 kbit/s
//  <24001=>CBR: 24 kbit/s
//  <32001=>CBR: 32 kbit/s
//  <40001=>CBR: 40 kbit/s
//  <48001=>CBR: 48 kbit/s
//  <56001=>CBR: 56 kbit/s
//  <64001=>CBR: 64 kbit/s
//  <80001=>CBR: 80 kbit/s
//  <96001=>CBR: 96 kbit/s
//  <112001=>CBR: 112 kbit/s
//  <128001=>CBR: 128 kbit/s
//
//  Bit rate:   (CONFIG_OPUS_BITRATE_CFG & ~0x0F)
//  Flags:      (CONFIG_OPUS_BITRATE_CFG & 0x0F)
//      Bit 0:  0 = VBR/CVBR
//              1 = CBR
//
/**@brief Opus Options: Bit Rate */
#define CONFIG_OPUS_BITRATE_CFG 0

// <o> Bit Rate Limit
// <i> Set a bit rate limit that cannot be exceeded during the transmission. Must be equal or higher than the configured bit rate.
//  <16000=>16 kbit/s
//  <24000=>24 kbit/s
//  <32000=>32 kbit/s
//  <40000=>40 kbit/s
//  <48000=>48 kbit/s
//  <56000=>56 kbit/s
//  <64000=>64 kbit/s
//  <72000=>72 kbit/s
//  <80000=>80 kbit/s
//  <88000=>88 kbit/s
//  <96000=>96 kbit/s
//  <112000=>112 kbit/s
//  <128000=>128 kbit/s
//  <144000=>144 kbit/s
//  <160000=>160 kbit/s
/**@brief Opus Options: Bit Rate Limit */
#define CONFIG_OPUS_BITRATE_LIMIT 40000

// <o> Complexity <0-10>
// <i> A number from range 0-10. Higher complexity assures better quality but also higher CPU and memory resources consumption.
/**@brief Opus Options: Complexity <0-10> */
#define CONFIG_OPUS_COMPLEXITY 0

// <o> Audio Frame Size
// <i> CELT supports 5 ms - 40 ms audio frames. SILK provides support for 10 ms - 60 ms frame sizes.
//  <5=>5 ms
//  <10=>10 ms
//  <20=>20 ms
//  <40=>40 ms
//  <60=>60 ms
#if (CONFIG_AUDIO_CODEC == CONFIG_AUDIO_CODEC_OPUS)
/**@brief Opus Options: Audio Frame Size */
#define CONFIG_AUDIO_FRAME_SIZE_MS 20
#endif /* CONFIG_AUDIO_CODEC == CONFIG_AUDIO_CODEC_OPUS */

// <q> Include frame header
// <i> Note: Host-side NVS implementation requires frame header for reassembly. Disabling this option breaks NVS host compatibility.
/**@brief Opus Options: Include frame header */
#define CONFIG_OPUS_HEADER_ENABLED 1
// </h>

// <h> SBC Options
// <o> Mode
// <i> mSBC - a modified version of the SBC codec specified in Hands-Free Profile, block length: 15, subbands: 8, allocation: Loudness, bitpool: 26.
// <i> Custom - standard A2DP codec with settings specified in the following section.
//  <0=>mSBC
//  <1=>Custom
/**@brief SBC Options: Mode */
#define CONFIG_SBC_MODE 0

// <h> Custom mode settings
// <o> Block length
// <i> Block length to be used by the SBC encoder.
//  <4=>4
//  <8=>8
//  <12=>12
//  <16=>16
#if (CONFIG_SBC_MODE == CONFIG_SBC_MODE_CUSTOM)
/**@brief SBC Options: Block length */
#define CONFIG_SBC_BLOCKS 16
#endif

// <o> Number of subbands
// <i> Number of subbands to be used by the SBC encoder.
//  <4=>4
//  <8=>8
#if (CONFIG_SBC_MODE == CONFIG_SBC_MODE_CUSTOM)
/**@brief SBC Options: Number of subbands */
#define CONFIG_SBC_SUBBANDS 8
#endif

// <o> Allocation method
// <i> Allocation method to be used by the SBC encoder.
//  <0=>Loudness
//  <1=>SNR
#if (CONFIG_SBC_MODE == CONFIG_SBC_MODE_CUSTOM)
/**@brief SBC Options: Allocation method */
#define CONFIG_SBC_ALLOCATION 0
#endif

// <o> Bitpool value <2-250>
// <i> Bitpool value to be used by the SBC encoder.
#if (CONFIG_SBC_MODE == CONFIG_SBC_MODE_CUSTOM)
/**@brief SBC Options: Bitpool value <2-250> */
#define CONFIG_SBC_BITPOOL 28
#endif
// </h>
// </h>

// <h> Pulse-Density Modulation (PDM) Interface Configuration
// <i> Specify the PDM interface configuration settings.
// <o> PDM Microphone
// <i> If ANR is enabled, select the PDM microphone that will be used to pick up voice. The other microphone will be used to pick up background noise.
// <i> For nRF52810 this option must be set to "Left" (1).
//  <1=>Left
//  <2=>Right
/**@brief PDM Microphone */
#define CONFIG_PDM_MIC 1

// <o> PDM Decimation Filter Gain
// <i> For details on the PDM decimation filter, see the 'Decimation filter' section in the nRF52 Product Specification document.
//  <0x00=>-20.0 dB
//  <0x01=>-19.5 dB
//  <0x02=>-19.0 dB
//  <0x03=>-18.5 dB
//  <0x04=>-18.0 dB
//  <0x05=>-17.5 dB
//  <0x06=>-17.0 dB
//  <0x07=>-16.5 dB
//  <0x08=>-16.0 dB
//  <0x09=>-15.5 dB
//  <0x0A=>-15.0 dB
//  <0x0B=>-14.5 dB
//  <0x0C=>-14.0 dB
//  <0x0D=>-13.5 dB
//  <0x0E=>-13.0 dB
//  <0x0F=>-12.5 dB
//  <0x10=>-12.0 dB
//  <0x11=>-11.5 dB
//  <0x12=>-11.0 dB
//  <0x13=>-10.5 dB
//  <0x14=>-10.0 dB
//  <0x15=>-9.5 dB
//  <0x16=>-9.0 dB
//  <0x17=>-8.5 dB
//  <0x18=>-8.0 dB
//  <0x19=>-7.5 dB
//  <0x1A=>-7.0 dB
//  <0x1B=>-6.5 dB
//  <0x1C=>-6.0 dB
//  <0x1D=>-5.5 dB
//  <0x1E=>-5.0 dB
//  <0x1F=>-4.5 dB
//  <0x20=>-4.0 dB
//  <0x21=>-3.5 dB
//  <0x22=>-3.0 dB
//  <0x23=>-2.5 dB
//  <0x24=>-2.0 dB
//  <0x25=>-1.5 dB
//  <0x26=>-1.0 dB
//  <0x27=>-0.5 dB
//  <0x28=>0.0 dB
//  <0x29=>+0.5 dB
//  <0x2A=>+1.0 dB
//  <0x2B=>+1.5 dB
//  <0x2C=>+2.0 dB
//  <0x2D=>+2.5 dB
//  <0x2E=>+3.0 dB
//  <0x2F=>+3.5 dB
//  <0x30=>+4.0 dB
//  <0x31=>+4.5 dB
//  <0x32=>+5.0 dB
//  <0x33=>+5.5 dB
//  <0x34=>+6.0 dB
//  <0x35=>+6.5 dB
//  <0x36=>+7.0 dB
//  <0x37=>+7.5 dB
//  <0x38=>+8.0 dB
//  <0x39=>+8.5 dB
//  <0x3A=>+9.0 dB
//  <0x3B=>+9.5 dB
//  <0x3C=>+10.0 dB
//  <0x3D=>+10.5 dB
//  <0x3E=>+11.0 dB
//  <0x3F=>+11.5 dB
//  <0x40=>+12.0 dB
//  <0x41=>+12.5 dB
//  <0x42=>+13.0 dB
//  <0x43=>+13.5 dB
//  <0x44=>+14.0 dB
//  <0x45=>+14.5 dB
//  <0x46=>+15.0 dB
//  <0x47=>+15.5 dB
//  <0x48=>+16.0 dB
//  <0x49=>+16.5 dB
//  <0x4A=>+17.0 dB
//  <0x4B=>+17.5 dB
//  <0x4C=>+18.0 dB
//  <0x4D=>+18.5 dB
//  <0x4E=>+19.0 dB
//  <0x4F=>+19.5 dB
//  <0x50=>+20.0 dB
/**@brief PDM Decimation Filter Gain */
#define CONFIG_PDM_GAIN 0x28

// <o> PDM Microphone Transient State Length [ms] <1-1000>
// <i> Set the length of the PDM microphone transient state. In this state, the microphone produces invalid data after wakeup or after it is powered on.
/**@brief PDM Microphone Transient State Length [ms] <1-1000> */
#define CONFIG_PDM_TRANSIENT_STATE_LEN 10

// <q> Enable PDM Microphone Power Control
// <i> Enable control over the microphone power line. Remember to configure a PDM Microphone Power Control Pin in the I/O Configuration.
/**@brief Enable PDM Microphone Power Control */
#define CONFIG_PDM_MIC_PWR_CTRL_ENABLED 1
// </h>

// <h> Bluetooth Audio Service Options
// <i> Configure the Bluetooth Audio options.

// <q> Audio HID Service enabled
// <i> HID Service for audio enabled and available for audio transmission.
/**@brief Audio HID Service enabled */
#define CONFIG_AUDIO_HID_ENABLED 1

// <e> Audio ATVV Service enabled
// <i> ATVV Service for audio enabled and available for audio transmission.
/**@brief Audio ATVV Service enabled */
#define CONFIG_AUDIO_ATVV_ENABLED 0

// <o> Remote control ID <0x00-0xFF>
// <i> ID value used in ATVV audio frame headers.
/**@brief ATVV: Remote control ID <0x00-0xFF> */
#define CONFIG_AUDIO_ATVV_REMOTE_CONTROL_ID 0x01

// <o> Frame sync message interval <1-0xFFFF>
// <i> Number of audio frames in-between frame sync messages.
/**@brief ATVV: Frame sync message interval <1-0xFFFF> */
#define CONFIG_AUDIO_ATVV_SYNC_INTERVAL 15

// <o> Open mic search timeout [ms] <1-0xFFFF>
// <i> Parameter for open mic timeout event generation. If a peer has not sent open mic message within this time after sending the audio search command, an event is generated.
/**@brief ATVV: Open mic search timeout [ms] <1-0xFFFF> */
#define CONFIG_AUDIO_ATVV_SEARCH_TIMEOUT 1000
// </e>
// </h>

// <q> Enable Audio Processing Gauges
// <i> An Audio Processing Gauge is a statistical report of bit rate and CPU usage during a given audio transmission.
/**@brief Enable Audio Processing Gauges */
#define CONFIG_AUDIO_GAUGES_ENABLED (1 && NRF_LOG_ENABLED && CONFIG_AUDIO_ENABLED)

// <h> Logging Options
// <i> This section configures module-specific logging options.

// <o> Module logging level
//  <0=> None
//  <1=> Error
//  <2=> Warning
//  <3=> Info
//  <4=> Debug
/**@brief Audio: Module logging level */
#define CONFIG_AUDIO_MODULE_LOG_LEVEL 4

// <o> Codec logging level
//  <0=> None
//  <1=> Error
//  <2=> Warning
//  <3=> Info
//  <4=> Debug
/**@brief Codec logging level */
#define CONFIG_AUDIO_DRV_CODEC_LOG_LEVEL 4

// <o> DSP driver logging level
//  <0=> None
//  <1=> Error
//  <2=> Warning
//  <3=> Info
//  <4=> Debug
/**@brief DSP driver logging level */
#define CONFIG_AUDIO_DRV_DSP_LOG_LEVEL 0

// <o> ANR driver logging level
//  <0=> None
//  <1=> Error
//  <2=> Warning
//  <3=> Info
//  <4=> Debug
/**@brief ANR driver logging level */
#define CONFIG_AUDIO_DRV_ANR_LOG_LEVEL 0

// <o> PDM driver logging level
//  <0=> None
//  <1=> Error
//  <2=> Warning
//  <3=> Info
//  <4=> Debug
/**@brief PDM driver logging level */
#define CONFIG_AUDIO_DRV_PDM_LOG_LEVEL 4
// </h>
// </e>

// <e> Enable Battery Measurement
// <i> Enable the battery measurement infrastructure and service.
/**@brief Enable Battery Measurement */
#define CONFIG_BATT_MEAS_ENABLED 1

// <o> Voltage of 0% Battery Level [mV]
// <i> Configure the battery voltage that corresponds to 0% battery level.
/**@brief Voltage of 0% Battery Level [mV] */
#define CONFIG_BATT_MEAS_MIN_LEVEL 1700

// <o> Voltage of 100% Battery Level [mV]
// <i> Configure the battery voltage that corresponds to 100% battery level.
/**@brief Voltage of 100% Battery Level [mV] */
#define CONFIG_BATT_MEAS_MAX_LEVEL 3000

// <o> Battery Voltage Polling Interval [s] <1-900>
// <i> Configure the time between subsequent battery measurements.
/**@brief Battery Voltage Polling Interval [s] <1-900> */
#define CONFIG_BATT_MEAS_POLL_INTERVAL 60

// <o> Notification Threshold [percentage point] <0-100>
// <i> Configure the threshold for sending a battery level notification.
// <i> To send the notification on every measurement, set this option to 0.
/**@brief Battery Level Notification Threshold [percentage point] <0-100> */
#define CONFIG_BATT_NOTIFICATION_THRESHOLD 1

// <h> Logging Options
// <i> This section configures module-specific logging options.

// <o> Module logging level
//  <0=> None
//  <1=> Error
//  <2=> Warning
//  <3=> Info
//  <4=> Debug
/**@brief Battery: Module logging level */
#define CONFIG_BATT_MEAS_MODULE_LOG_LEVEL 4
// </h>
// </e>

// <e> Enable Buzzer
// <i> Enable the buzzer function, which is controlled by Bluetooth Immediate Alert service.
/**@brief Enable Buzzer */
#define CONFIG_BUZZER_ENABLED 1

// <h> Logging Options
// <i> This section configures module-specific logging options.

// <o> Module logging level
//  <0=> None
//  <1=> Error
//  <2=> Warning
//  <3=> Info
//  <4=> Debug
/**@brief Buzzer: Module logging level */
#define CONFIG_BUZZER_MODULE_LOG_LEVEL 4

// <o> Driver logging level
//  <0=> None
//  <1=> Error
//  <2=> Warning
//  <3=> Info
//  <4=> Debug
/**@brief Buzzer: Driver logging level */
#define CONFIG_BUZZER_DRV_LOG_LEVEL 4
// </h>
// </e>

// <e> Enable DFU Support
// <i> Enable Device Firmware Update subsystem that allows for firmware update over Bluetooth.
/**@brief Enable DFU Support */
#define CONFIG_DFU_ENABLED 1

// <o> Hardware Version <0x00000000-0xFFFFFFFF>
// <i> Hardware version number used to validate DFU image.
/**@brief DFU: Hardware Version <0x00000000-0xFFFFFFFF> */
#define CONFIG_DFU_HW_VERSION 0x20023
// </e>

// <e> Enable Gyroscope Support
// <i> Enable the gyroscope subsystem that allows for the air mouse functionality.
/**@brief Enable Gyroscope Support */
#define CONFIG_GYRO_ENABLED 0

// <o> TWI Bus
// <i> Select the TWI Bus that is used to communicate with the gyroscope.
//  <0=>Primary
//  <1=>Secondary
/**@brief Gyroscope TWI Bus */
#define CONFIG_GYRO_TWI_BUS 0

// <o> Gyroscope Idle Time-out [s] <1-255>
// <i> Set the time after which the gyroscope mode is automatically disabled if no motion is detected.
/**@brief Gyroscope Idle Time-out [s] <1-255> */
#define CONFIG_GRYO_IDLE_TIMEOUT 7

// <o> Gyroscope Polling Interval [ms] <1-100>
// <i> Configure the gyroscope polling interval.
// <i> Note 1: The Air Motion Library which translates gyroscope data to mouse cursor movements is optimized for 10 ms polling interval.
// <i> Note 2: Polling interval shorter than 5 ms might cause congestion on the I2C bus if other modules using the same bus are enabled.
/**@brief Gyroscope Polling Interval [ms] <1-100> */
#define CONFIG_GYRO_POLL_INTERVAL 10

// <o> Gyroscope X Gain <1-255>
// <i> Set the cursor movement gain in the x-axis.
/**@brief Gyroscope X Gain <1-255> */
#define CONFIG_GYRO_X_GAIN 8

// <o> Gyroscope Y Gain <1-255>
// <i> Set the cursor movement gain in the y-axis.
/**@brief Gyroscope Y Gain <1-255> */
#define CONFIG_GYRO_Y_GAIN 8

// <h> Special Key Mapping
// <i> Define the mapping of special keys.

// <o> Gyroscope Mode Toggle Key ID <0x00-0xFF>
// <i> Select the key that toggles the gyroscope mode.
/**@brief Gyroscope Mode Toggle Key ID <0x00-0xFF> */
#define CONFIG_GYRO_MODE_KEY_ID 0x26

// <o> Left Click Key ID <0x00-0xFF>
// <i> Select the key that acts as the left mouse button in the gyroscope mode.
/**@brief Gyroscope Left Click Key ID <0x00-0xFF> */
#define CONFIG_GYRO_LEFT_CLICK_KEY_ID 0x30

// <o> Right Click Key ID <0x00-0xFF>
// <i> Select the key that acts as the right mouse button in the gyroscope mode.
/**@brief Gyroscope Right Click Key ID <0x00-0xFF> */
#define CONFIG_GYRO_RIGHT_CLICK_KEY_ID 0x32
// </h>

// <h> Logging Options
// <i> This section configures module-specific logging options.

// <o> Module logging level
//  <0=> None
//  <1=> Error
//  <2=> Warning
//  <3=> Info
//  <4=> Debug
/**@brief Gyroscope: Module logging level */
#define CONFIG_GYRO_MODULE_LOG_LEVEL 4

// <o> Driver logging level
//  <0=> None
//  <1=> Error
//  <2=> Warning
//  <3=> Info
//  <4=> Debug
/**@brief Gyroscope: Driver logging level */
#define CONFIG_GYRO_DRV_LOG_LEVEL 4
// </h>
// </e>

// <e> Enable IR Transmitter Support
// <i> Enable the infrared transmitter subsystem.
/**@brief Enable IR Transmitter Support */
#define CONFIG_IR_TX_ENABLED 1

// <o> IR Protocol selection
//  <1=>SIRC
//  <2=>NEC
/**@brief IR Protocol selection */
#define CONFIG_IR_PROTOCOL 1

// <o> Intersymbol gap length [ms] <10-250>
// <i> Set the intersymbol gap length.
/**@brief IR: Intersymbol gap length [ms] <10-250> */
#define CONFIG_IR_TX_INTER_SYMBOL_GAP 50

// <h> Logging Options
// <i> This section configures module-specific logging options.

// <o> Module logging level
//  <0=> None
//  <1=> Error
//  <2=> Warning
//  <3=> Info
//  <4=> Debug
/**@brief IR: Module logging level */
#define CONFIG_IR_MODULE_LOG_LEVEL 4

// <o> Driver logging level
//  <0=> None
//  <1=> Error
//  <2=> Warning
//  <3=> Info
//  <4=> Debug
/**@brief IR: Driver logging level */
#define CONFIG_IR_DRV_LOG_LEVEL 4
// </h>
// </e>

// <e> Enable Keyboard Support
// <i> Enable the keyboard subsystem.
/**@brief Enable Keyboard Support */
#define CONFIG_KBD_ENABLED 1

// <o> Keyboard Driver
// <i> Select the keyboard driver.
//  <1=>GPIO-Based Matrix Keyboard
//  <2=>SX1509-Based Matrix Keyboard
/**@brief Keyboard Driver */
#define CONFIG_KBD_DRIVER 1

// <o> TWI Bus
// <i> Select the TWI Bus that is used to communicate with the keyboard. Relevant only if the SX1509 keyboard driver is selected.
//  <0=>Primary
//  <1=>Secondary
/**@brief Keyboard TWI Bus */
#define CONFIG_KBD_TWI_BUS 0

// <q> Use Keyboard as Wakeup Source
// <i> Enable system wakeup based on key press. Available only if the GPIO keyboard driver is selected.
/**@brief Use Keyboard as Wakeup Source */
#define CONFIG_KBD_WAKEUP_SOURCE 0

// <o> Keyboard Polling Interval [ms] <1-100>
// <i> Configure the keyboard polling interval.
// <i> Note: Polling interval shorter than 5 ms might cause congestion on the I2C bus if the SX1509-based driver is used and other modules using the same bus are enabled.
/**@brief Keyboard Polling Interval [ms] <1-100> */
#define CONFIG_KBD_POLL_INTERVAL 15

// <o> Key held event generation interval [ms] <0-10000>
// <i> Configure the key held event rate (0 => Disable key held event generation).
/**@brief Keyboard: Key held event generation interval [ms] <0-10000> */
#define CONFIG_KBD_HELD_EVENT_INTERVAL_MS 1000

// <q> Enable detection of key combinations
// <i> Note: Key combinations are created via key_combo_util.h macros.
/**@brief Keyboard: Enable detection of key combinations */
#define CONFIG_KBD_KEY_COMBO_ENABLED (1 && CONFIG_KBD_ENABLED)

// <h> Special Key Mapping
// <i> Define the mapping of special keys.

// <o> Delete Bonds Key ID <0x00-0xFF>
// <i> Select the key that removes bonds if it is held during firmware boot.
/**@brief Keyboard: Delete Bonds Key ID <0x00-0xFF> */
#define CONFIG_KBD_DELETE_BONDS_KEY_ID 0x26

// <o> Android TV voice command Key ID <0x00-0xFF>
// <i> Select the key that triggers transmission of Android TV Voice command when ATVV Service is enabled in Bluetooth options.
/**@brief Keyboard: Android TV voice command Key ID <0x00-0xFF> */
#define CONFIG_KBD_ATVV_START_SEARCH_KEY_ID 0x20


// <e> Enable Android TV Voice pairing key chords
// <i> ATVV suggests key chords for pairing and delete pairing:
// <i> Pairing mode: Long Press {HOME + BACK}
// <i> Delete pairing: Long Press {DPAD_CENTER + BACK + HOME}
/**@brief Keyboard: Enable Android TV voice pairing key chords */
#define CONFIG_KBD_ATVV_KEY_CHORDS_ENABLED 1

// <o> Android TV remote "HOME" Key ID <0x00-0xFF>
/**@brief Keyboard: Android TV remote "HOME" Key ID <0x00-0xFF> */
#define CONFIG_KBD_ATVV_HOME_KEY_ID 0x30

// <o> Android TV remote "BACK" Key ID <0x00-0xFF>
/**@brief Keyboard: Android TV remote "BACK" Key ID <0x00-0xFF> */
#define CONFIG_KBD_ATVV_BACK_KEY_ID 0x32

// <o> Android TV remote "DPAD_CENTER" Key ID <0x00-0xFF>
/**@brief Keyboard: Android TV remote "DPAD_CENTER" Key ID <0x00-0xFF> */
#define CONFIG_KBD_ATVV_DPAD_CENTER_KEY_ID 0x34
// </e>

// <e> Enable Sending Google Chrome Voice Search Shortcut (Ctrl+Shift+.)
// <i> Enable activating the Google Chrome voice search with the remote.
/**@brief Keyboard: Enable Sending Google Chrome Voice Search Shortcut (Ctrl+Shift+.) */
#define CONFIG_GOOGLE_VOICE_SHORTCUT_ENABLED 1

// <o> Google Chrome Voice Search Shortcut Key ID <0x00-0xFF>
// <i> Select the key that functions as the equivalent of the Google Chrome voice search shortcut (Ctrl+Shift+.).
/**@brief Keyboard: Google Chrome Voice Search Shortcut Key ID <0x00-0xFF> */
#define CONFIG_GOOGLE_VOICE_SHORTCUT_KEY_ID 0x23
// </e>
// </h>

// <h> Logging Options
// <i> This section configures module-specific logging options.

// <o> Module logging level
//  <0=> None
//  <1=> Error
//  <2=> Warning
//  <3=> Info
//  <4=> Debug
/**@brief Keyboard: Module logging level */
#define CONFIG_KBD_MODULE_LOG_LEVEL 4

// <o> Key combo logging level
//  <0=> None
//  <1=> Error
//  <2=> Warning
//  <3=> Info
//  <4=> Debug
/**@brief Keyboard: Key combo logging level */
#define CONFIG_KBD_COMBO_LOG_LEVEL 4

// <o> Driver logging level
//  <0=> None
//  <1=> Error
//  <2=> Warning
//  <3=> Info
//  <4=> Debug
/**@brief Keyboard: Driver logging level */
#define CONFIG_KBD_DRV_LOG_LEVEL 4
// </h>
// </e>

// <e> Enable LED Support
// <i> Enable the LED signaling subsystem.
/**@brief Enable LED Support */
#define CONFIG_LED_ENABLED 0

// <o> LED Flip Interval [ms] <10-1000>
// <i> Time between each on/off switching of the LED.
/**@brief LED Flip Interval [ms] <10-1000> */
#define CONFIG_LED_FLIP_INTERVAL 250

// <e> Signal Advertising
// <i> Enable LED signaling when the remote starts advertising.
/**@brief LED: Signal Advertising */
#define CONFIG_LED_SIGNAL_ADVERTISING (0 && CONFIG_LED_ENABLED)

// <h> Action: Clear LEDs
// <i> The Clear action clears the signaling that was configured by another event.

// <o.0> Clear LED 1
// <o.1> Clear LED 2
// <o.2> Clear LED 3
// <o.3> Clear LED 4
/**@brief LED: Signal Advertising - Clear LED 1, 2, 3 and/or 4 (bitmask) */
#define CONFIG_LED_ADVERTISING_LEDS_CLEAR 0
// </h>

// <h> Action: Set LEDs
// <i> The Set action turns on the selected LEDs. Once set, the LED must be cleared by another event.

// <o.0> Set LED 1
// <o.1> Set LED 2
// <o.2> Set LED 3
// <o.3> Set LED 4
/**@brief LED: Signal Advertising - Set LED 1, 2, 3 and/or 4 (bitmask) */
#define CONFIG_LED_ADVERTISING_LEDS_SET 0
// </h>

// <h> Action: Blink LEDs
// <i> The Blink action turns on blinking on the selected LEDs.

// <o> Number of LED blinks <0-127>
// <i> Specify how many times the LED will blink.
// <i> If set to 0, the LED will blink until cleared by another event.
/**@brief LED: Signal Advertising - Number of LED blinks <0-127> */
#define CONFIG_LED_ADVERTISING_FLASHES 0

// <o.0> Use LED 1
// <o.1> Use LED 2
// <o.2> Use LED 3
// <o.3> Use LED 4
/**@brief LED: Signal Advertising - Use LED 1, 2, 3 and/or 4 (bitmask) */
#define CONFIG_LED_ADVERTISING_LEDS_FLASH 0
// </h>
// </e>

// <e> Signal Connection Establishment
// <i> Enable LED signaling when the remote connects to the host.
/**@brief LED: Signal Connection Establishment */
#define CONFIG_LED_SIGNAL_CONNECTION (1 && CONFIG_LED_ENABLED)

// <h> Action: Clear LEDs
// <i> The Clear action clears the signaling that was configured by another event.

// <o.0> Clear LED 1
// <o.1> Clear LED 2
// <o.2> Clear LED 3
// <o.3> Clear LED 4
/**@brief LED: Signal Connection Establishment - Clear LED 1, 2, 3 and/or 4 (bitmask) */
#define CONFIG_LED_CONNECTION_LEDS_CLEAR 0
// </h>

// <h> Action: Set LEDs
// <i> The Set action turns on the selected LEDs. Once set, the LED must be cleared by another event.

// <o.0> Set LED 1
// <o.1> Set LED 2
// <o.2> Set LED 3
// <o.3> Set LED 4
/**@brief LED: Signal Connection Establishment - Set LED 1, 2, 3 and/or 4 (bitmask) */
#define CONFIG_LED_CONNECTION_LEDS_SET 0
// </h>

// <h> Action: Blink LEDs
// <i> The Blink action turns on blinking on the selected LEDs.

// <o> Number of LED Blinks <0-127>
// <i> Specify how many times the LED will blink.
// <i> If set to 0, the LED will blink until cleared by another event.
/**@brief LED: Signal Connection Establishment - Number of LED Blinks <0-127> */
#define CONFIG_LED_CONNECTION_FLASHES 1

// <o.0> Use LED 1
// <o.1> Use LED 2
// <o.2> Use LED 3
// <o.3> Use LED 4
/**@brief LED: Signal Connection Establishment - Use LED 1, 2, 3 and/or 4 (bitmask) */
#define CONFIG_LED_CONNECTION_LEDS_FLASH 1
// </h>
// </e>

// <e> Signal Connection Error
// <i> Enables LED signaling when the remote cannot connect to the host or when it has been diconnected.
/**@brief LED: Signal Connection Error */
#define CONFIG_LED_SIGNAL_CONNECTION_ERROR (1 && CONFIG_LED_ENABLED)

// <h> Action: Clear LEDs
// <i> The Clear action clears the signaling that was configured by another event.

// <o.0> Clear LED 1
// <o.1> Clear LED 2
// <o.2> Clear LED 3
// <o.3> Clear LED 4
/**@brief LED: Signal Connection Error - Clear LED 1, 2, 3 and/or 4 (bitmask) */
#define CONFIG_LED_CONNECTION_ERROR_LEDS_CLEAR 0
// </h>

// <h> Action: Set LEDs
// <i> The Set action turns on the selected LEDs. Once set, the LED must be cleared by another event.

// <o.0> Set LED 1
// <o.1> Set LED 2
// <o.2> Set LED 3
// <o.3> Set LED 4
/**@brief LED: Signal Connection Error - Set LED 1, 2, 3 and/or 4 (bitmask) */
#define CONFIG_LED_CONNECTION_ERROR_LEDS_SET 0
// </h>

// <h> Action: Blink LEDs
// <i> The Blink action turns on blinking on the selected LEDs.

// <o> Number of LED Blinks <0-127>
// <i> Specify how many times the LED will blink.
// <i> If set to 0, the LED will blink until cleared by another event.
/**@brief LED: Signal Connection Error - Number of LED Blinks <0-127> */
#define CONFIG_LED_CONNECTION_ERROR_FLASHES 3

// <o.0> Use LED 1
// <o.1> Use LED 2
// <o.2> Use LED 3
// <o.3> Use LED 4
/**@brief LED: Signal Connection Error - Use LED 1, 2, 3 and/or 4 (bitmask) */
#define CONFIG_LED_CONNECTION_ERROR_LEDS_FLASH 1
// </h>
// </e>

// <e> Signal Low Battery Condition
// <i> Enable LED signaling when the battery level is low.
/**@brief LED: Signal Low Battery Condition */
#define CONFIG_LED_SIGNAL_LOW_BATTERY (0 && CONFIG_LED_ENABLED && CONFIG_BATT_MEAS_ENABLED)

// <o> Low Battery Threshold [%] <0-100>
// <i> Specify what battery level is considered low.
/**@brief LED: Low Battery Threshold [%] <0-100> */
#define CONFIG_LED_LOW_BATTERY_THRESHOLD 5

// <h> Action: Clear LEDs
// <i> The Clear action clears signaling that was configured by another event.

// <o.0> Clear LED 1
// <o.1> Clear LED 2
// <o.2> Clear LED 3
// <o.3> Clear LED 4
/**@brief LED: Signal Low Battery Condition - Clear LED 1, 2, 3 and/or 4 (bitmask) */
#define CONFIG_LED_LOW_BATTERY_LEDS_CLEAR 0
// </h>

// <h> Action: Set LEDs
// <i> The Set action turns on the selected LEDs. Once set, the LED must be cleared by another event.

// <o.0> Set LED 1
// <o.1> Set LED 2
// <o.2> Set LED 3
// <o.3> Set LED 4
/**@brief LED: Signal Low Battery Condition - Set LED 1, 2, 3 and/or 4 (bitmask) */
#define CONFIG_LED_LOW_BATTERY_LEDS_SET 0
// </h>

// <h> Action: Blink LEDs
// <i> The Blink action turns on blinking on the selected LEDs.

// <o> Number of LED Blinks <0-127>
// <i> Specify how many times the LED will blink.
// <i> If set to 0, the LED will blink until cleared by another event.
/**@brief LED: Signal Low Battery Condition - Number of LED Blinks <0-127> */
#define CONFIG_LED_LOW_BATTERY_FLASHES 1

// <o.0> Use LED 1
// <o.1> Use LED 2
// <o.2> Use LED 3
// <o.3> Use LED 4
/**@brief LED: Signal Low Battery Condition - Use LED 1, 2, 3 and/or 4 (bitmask) */
#define CONFIG_LED_LOW_BATTERY_LEDS_FLASH 0
// </h>
// </e>

// <e> Signal Immediate Alert
// <i> Enable LED signaling when the Immediate Alert is requested.
/**@brief LED: Signal Immediate Alert */
#define CONFIG_LED_SIGNAL_IMMEDIATE_ALERT (0 && CONFIG_LED_ENABLED)

// <h> Alert Level 0 (None) Actions
// <i> This section configures LED signaling when the Immediate Alert Level is set to 0 (none).

// <h> Action: Clear LEDs
// <i> The Clear action clears the signaling that was configured by another event.

// <o.0> Clear LED 1
// <o.1> Clear LED 2
// <o.2> Clear LED 3
// <o.3> Clear LED 4
/**@brief LED: Signal Immediate Alert Level 0 - Clear LED 1, 2, 3 and/or 4 (bitmask) */
#define CONFIG_LED_IMMEDIATE_ALERT_0_CLEAR 0
// </h>

// <h> Action: Set LEDs
// <i> The Set action turns on the selected LEDs. Once set, the LED must be cleared by another event.

// <o.0> Set LED 1
// <o.1> Set LED 2
// <o.2> Set LED 3
// <o.3> Set LED 4
/**@brief LED: Signal Immediate Alert Level 0 - Set LED 1, 2, 3 and/or 4 (bitmask) */
#define CONFIG_LED_IMMEDIATE_ALERT_0_SET 0
// </h>

// <h> Action: Blink LEDs
// <i> The Blink action turns on blinking on the selected LEDs.

// <o> Number of LED Blinks <0-127>
// <i> Specify how many times the LED will blink.
// <i> If set to 0, the LED will blink until cleared by another event.
/**@brief LED: Signal Immediate Alert Level 0 - Number of LED Blinks <0-127> */
#define CONFIG_LED_IMMEDIATE_ALERT_0_FLASHES 1

// <o.0> Use LED 1
// <o.1> Use LED 2
// <o.2> Use LED 3
// <o.3> Use LED 4
/**@brief LED: Signal Immediate Alert Level 0 - Use LED 1, 2, 3 and/or 4 (bitmask) */
#define CONFIG_LED_IMMEDIATE_ALERT_0_FLASH 0
// </h>
// </h>

// <h> Alert Level 1 (Mild) Actions
// <i> This section configures LED signaling when the Immediate Alert Level is set to 1 (mild).

// <h> Action: Clear LEDs
// <i> The Clear action clears the signaling that was configured by another event.

// <o.0> Clear LED 1
// <o.1> Clear LED 2
// <o.2> Clear LED 3
// <o.3> Clear LED 4
/**@brief LED: Signal Immediate Alert Level 1 - Clear LED 1, 2, 3 and/or 4 (bitmask) */
#define CONFIG_LED_IMMEDIATE_ALERT_1_CLEAR 0
// </h>

// <h> Action: Set LEDs
// <i> The Set action turns on the selected LEDs. Once set, the LED must be cleared by another event.

// <o.0> Set LED 1
// <o.1> Set LED 2
// <o.2> Set LED 3
// <o.3> Set LED 4
/**@brief LED: Signal Immediate Alert Level 1 - Set LED 1, 2, 3 and/or 4 (bitmask) */
#define CONFIG_LED_IMMEDIATE_ALERT_1_SET 0
// </h>

// <h> Action: Blink LEDs
// <i> The Blink action turns on blinking on the selected LEDs.

// <o> Number of LED Blinks <0-127>
// <i> Specify how many times the LED will blink.
// <i> If set to 0, the LED will blink until cleared by another event.
/**@brief LED: Signal Immediate Alert Level 1 - Number of LED Blinks <0-127> */
#define CONFIG_LED_IMMEDIATE_ALERT_1_FLASHES 1

// <o.0> Use LED 1
// <o.1> Use LED 2
// <o.2> Use LED 3
// <o.3> Use LED 4
/**@brief LED: Signal Immediate Alert Level 1 - Use LED 1, 2, 3 and/or 4 (bitmask) */
#define CONFIG_LED_IMMEDIATE_ALERT_1_FLASH 0
// </h>
// </h>

// <h> Alert Level 2 (High) Actions
// <i> This section configures LED signaling when the Immediate Alert Level is set to 2 (high).

// <h> Action: Clear LEDs
// <i> The Clear action clears the signaling that was configured by another event.

// <o.0> Clear LED 1
// <o.1> Clear LED 2
// <o.2> Clear LED 3
// <o.3> Clear LED 4
/**@brief LED: Signal Immediate Alert Level 2 - Clear LED 1, 2, 3 and/or 4 (bitmask) */
#define CONFIG_LED_IMMEDIATE_ALERT_2_CLEAR 0
// </h>

// <h> Action: Set LEDs
// <i> The Set action turns on the selected LEDs. Once set, the LED must be cleared by another event.

// <o.0> Set LED 1
// <o.1> Set LED 2
// <o.2> Set LED 3
// <o.3> Set LED 4
/**@brief LED: Signal Immediate Alert Level 2 - Set LED 1, 2, 3 and/or 4 (bitmask) */
#define CONFIG_LED_IMMEDIATE_ALERT_2_SET 0
// </h>

// <h> Action: Blink LEDs
// <i> The Blink action turns on blinking on the selected LEDs.

// <o> Number of LED Blinks <0-127>
// <i> Specify how many times the LED will blink.
// <i> If set to 0, the LED will blink until cleared by another event.
/**@brief LED: Signal Immediate Alert Level 2 - Number of LED Blinks <0-127> */
#define CONFIG_LED_IMMEDIATE_ALERT_2_FLASHES 1

// <o.0> Use LED 1
// <o.1> Use LED 2
// <o.2> Use LED 3
// <o.3> Use LED 4
/**@brief LED: Signal Immediate Alert Level 2 - Use LED 1, 2, 3 and/or 4 (bitmask) */
#define CONFIG_LED_IMMEDIATE_ALERT_2_FLASH 0
// </h>
// </h>
// </e>

// <q> Enable LED Power Control
// <i> Enable control over the LED power line. Remember to configure a LED Power Control Pin in the I/O Configuration.
/**@brief Enable LED Power Control */
#define CONFIG_LED_PWR_CTRL_ENABLED 0

// <h> Logging Options
// <i> This section configures module-specific logging options.

// <o> Module logging level
//  <0=> None
//  <1=> Error
//  <2=> Warning
//  <3=> Info
//  <4=> Debug
/**@brief LED: Module logging level */
#define CONFIG_LED_MODULE_LOG_LEVEL 4

// <o> Driver logging level
//  <0=> None
//  <1=> Error
//  <2=> Warning
//  <3=> Info
//  <4=> Debug
/**@brief LED: Driver logging level */
#define CONFIG_LED_DRV_LOG_LEVEL 4
// </h>
// </e>

// <e> Enable NFC Tag
// <i> Enables NFC Tag subsystem.
/**@brief Enable NFC Tag */
#define CONFIG_NFC_ENABLED 1

// <e> Provide tap-to-pair tag
// <i> If this option is enabled, the tag will present an NFC pairing message when the device is bondable.
/**@brief NFC: Provide tap-to-pair tag */
#define CONFIG_NFC_PAIRING_TAG 1

// <q> Use full pairing message that supports both Bluetooth Classic and LE.
/**@brief NFC: Use full pairing message that supports both Bluetooth Classic and LE. */
#define CONFIG_NFC_USE_FULL_PAIRING_MSG 0
// </e>

// <q> Provide text tag
// <i> If this option is enabled, the tag will present textual information about the firmware.
/**@brief NFC: Provide text tag */
#define CONFIG_NFC_TEXT_TAG 1

// <q> Use NFC as a Wakeup Source
// <i> Enables system wakeup on NFC tag read attempt.
/**@brief NFC: Use NFC as a Wakeup Source */
#define CONFIG_NFC_WAKEUP_SOURCE 1

// <h> Logging Options
// <i> This section configures module-specific logging options.

// <o> Module logging level
//  <0=> None
//  <1=> Error
//  <2=> Warning
//  <3=> Info
//  <4=> Debug
/**@brief NFC: Module logging level */
#define CONFIG_NFC_MODULE_LOG_LEVEL 4
// </h>
// </e>

// <e> Enable Touchpad Support
// <i> Enable the touchpad subsystem.
/**@brief Enable Touchpad Support */
#define CONFIG_TOUCHPAD_ENABLED 0

// <o> TWI Bus
// <i> Select the TWI Bus that is used to communicate with the touchpad.
//  <0=>Primary
//  <1=>Secondary
/**@brief Touchpad TWI Bus */
#define CONFIG_TOUCHPAD_TWI_BUS 0

// <o> Touchpad Polling Interval [ms] <1-100>
// <i> Configure the touchpad polling interval.
// <i> Note: Polling interval shorter than 5 ms might cause congestion on the I2C bus if other modules using the same bus are enabled.
/**@brief Touchpad Polling Interval [ms] <1-100> */
#define CONFIG_TOUCHPAD_POLL_INTERVAL 15

// <h> Logging Options
// <i> This section configures module-specific logging options.

// <o> Module logging level
//  <0=> None
//  <1=> Error
//  <2=> Warning
//  <3=> Info
//  <4=> Debug
/**@brief Touchpad: Module logging level */
#define CONFIG_TOUCHPAD_MODULE_LOG_LEVEL 4

// <o> Driver logging level
//  <0=> None
//  <1=> Error
//  <2=> Warning
//  <3=> Info
//  <4=> Debug
/**@brief Touchpad: Driver logging level */
#define CONFIG_TOUCHPAD_DRV_LOG_LEVEL 4
// </h>
// </e>

// <e> Enable Power Management
// <i> Enable the power management subsystem.
/**@brief Enable Power Management */
#define CONFIG_PWR_MGMT_ENABLED 1

// <q> Enable CPU Usage Monitor
// <i> Once per second, the CPU Usage Monitor logs the CPU usage for the last second.
/**@brief Enable CPU Usage Monitor */
#define CONFIG_PWR_MGMT_CPU_USAGE_MONITOR_ENABLED 0

// <o> Standby Time-out [s] <1-3600>
// <i> Set the time after which the remote is automatically put into deep sleep mode if no user interaction is detected.
/**@brief Standby Time-out [s] <1-3600> */
#define CONFIG_PWR_MGMT_CONFIG_STANDBY_TIMEOUT_S 60

// <h> Logging Options
// <i> This section configures module-specific logging options.

// <o> Module logging level
//  <0=> None
//  <1=> Error
//  <2=> Warning
//  <3=> Info
//  <4=> Debug
/**@brief Power Management: Module logging level */
#define CONFIG_PWR_MGMT_MODULE_LOG_LEVEL 4
// </h>
// </e>

// <e> Enable Watchdog
// <i> This option enables the watchdog timer.
/**@brief Enable Watchdog */
#define CONFIG_WATCHDOG_ENABLED 1

// <o> Watchdog Time-out [s] <1-300>
// <i> Set the watchdog time-out.
/**@brief Watchdog Time-out [s] <1-300> */
#define CONFIG_WATCHDOG_TIMEOUT 3
// </e>

// <h> Logging and CLI options
// <i> Configure the Logger and the command line interface.

// <e> Enable CLI/Logger on RTT
// <i> This option enables the Logger and/or CLI on RTT.
/**@brief Enable CLI/Logger on RTT */
#define CONFIG_CL_RTT_ENABLED 1

// <o> Interface
//  <1=> Logger
//  <2=> CLI
/**@brief Logger RTT: Interface */
#define CONFIG_CL_RTT_INTERFACE 2

// <h> Logger options
// <i> Configure the settings of the RTT Logger.

// <o> RTT Console Log Queue Size <4-254>
// <i> Determines the number of log messages queued into CLI log backend.
/**@brief Logger RTT: RTT Console Log Queue Size <4-254> */
#define CONFIG_LOG_RTT_QSIZE 8

// <o> Default log level on RTT
//  <0=> None
//  <1=> Error
//  <2=> Warning
//  <3=> Info
//  <4=> Debug
/**@brief Logger RTT: Default log level on RTT */
#define CONFIG_LOG_RTT_SEVERITY 3
// </h>

// <h> CLI options
// <i> Configure the settings of the RTT CLI.

// <q> Use CLI as logger backend
// <i> Enables the Logger on RTT concurrently with CLI.
/**@brief Logger RTT: Use CLI as logger backend */
#define CONFIG_CLI_RTT_LOG_BACKEND_ENABLED 1
// </h>
// </e>

// <e> Enable CLI/Logger on UART
// <i> This option enables the Logger and/or CLI on UART.
/**@brief Logger UART: Enable CLI/Logger on UART */
#define CONFIG_CL_UART_ENABLED 0

// <o> Interface
//  <1=> Logger
//  <2=> CLI
/**@brief Logger UART: Interface */
#define CONFIG_CL_UART_INTERFACE 1

// <h> UART options
// <o> Hardware Flow Control
//  <0=> Disabled
//  <1=> Enabled
/**@brief Logger UART: Hardware Flow Control */
#define CONFIG_CL_UART_HWFC 0

// <o> Parity
//  <0=> Excluded
//  <14=> Included
/**@brief Logger UART: Parity */
#define CONFIG_CL_UART_PARITY 0

// <o> Baudrate
//  <323584=> 1200 baud
//  <643072=> 2400 baud
//  <1290240=> 4800 baud
//  <2576384=> 9600 baud
//  <3862528=> 14400 baud
//  <5152768=> 19200 baud
//  <7716864=> 28800 baud
//  <10289152=> 38400 baud
//  <15400960=> 57600 baud
//  <20615168=> 76800 baud
//  <30801920=> 115200 baud
//  <61865984=> 230400 baud
//  <67108864=> 250000 baud
//  <121634816=> 460800 baud
//  <251658240=> 921600 baud
//  <268435456=> 1000000 baud
/**@brief Logger UART: Baudrate */
#define CONFIG_CL_UART_BAUDRATE 30801920

// <o> TX buffer size for CLI
//  <4=>4 bytes
//  <8=>8 bytes
//  <16=>16 bytes
//  <32=>32 bytes
//  <64=>64 bytes
//  <128=>128 bytes
//  <256=>256 bytes
/**@brief Logger UART: TX buffer size for CLI */
#define CONFIG_CLI_UART_TX_BUFFER_SIZE 64

// <o> RX buffer size for CLI
//  <4=>4 bytes
//  <8=>8 bytes
//  <16=>16 bytes
//  <32=>32 bytes
//  <64=>64 bytes
//  <128=>128 bytes
//  <256=>256 bytes
/**@brief Logger UART: RX buffer size for CLI */
#define CONFIG_CLI_UART_RX_BUFFER_SIZE 16
// </h>

// <h> Logger options
// <i> Configure the settings of the UART Logger.

// <o> UART Console Log Queue Size <4-254>
// <i> Determines the number of log messages queued into CLI log backend.
/**@brief Logger UART: UART Console Log Queue Size <4-254> */
#define CONFIG_LOG_UART_QSIZE 8

// <o> Default log level on UART
//  <0=> None
//  <1=> Error
//  <2=> Warning
//  <3=> Info
//  <4=> Debug
/**@brief Logger UART: Default log level on UART */
#define CONFIG_LOG_UART_SEVERITY 3
// </h>

// <h> CLI options
// <i> Configure the settings of the UART CLI.

// <q> Use CLI as logger backend
// <i> Enables the Logger on UART concurrently with CLI.
/**@brief Logger UART: Use CLI as logger backend */
#define CONFIG_CLI_UART_LOG_BACKEND_ENABLED 0
// </h>
// </e>

// <e> Enable CLI over BLE
// <i> This option enables CLI over BLE.
/**@brief Logger BLE: Enable CLI over BLE */
#define CONFIG_CLI_BLE_ENABLED 1

// <h> Logger options
// <i> Configure the settings of the BLE Logger.

// <o> BLE Console Log Queue Size <4-254>
// <i> Determines the number of log messages queued into CLI log backend.
/**@brief Logger BLE: BLE Console Log Queue Size <4-254> */
#define CONFIG_LOG_BLE_QSIZE 8

// <o> Default log level on BLE
//  <0=> None
//  <1=> Error
//  <2=> Warning
//  <3=> Info
//  <4=> Debug
/**@brief Logger BLE: Default log level on BLE */
#define CONFIG_LOG_BLE_SEVERITY 0
// </h>

// <h> CLI options
// <i> Configure the settings of the BLE CLI.

// <o> BLE Console RX buffer size
// <i> Determines the size of RX ring buffer for CLI over BLE transport.
//  <4=>4 bytes
//  <8=>8 bytes
//  <16=>16 bytes
//  <32=>32 bytes
//  <64=>64 bytes
//  <128=>128 bytes
//  <256=>256 bytes
/**@brief Logger BLE: BLE Console RX buffer size */
#define CONFIG_CLI_BLE_RX_BUFFER_SIZE 16

// <o> BLE Console TX buffer size
// <i> Determines the size of TX ring buffer for CLI over BLE transport.
//  <4=>4 bytes
//  <8=>8 bytes
//  <16=>16 bytes
//  <32=>32 bytes
//  <64=>64 bytes
//  <128=>128 bytes
//  <256=>256 bytes
/**@brief Logger BLE: BLE Console TX buffer size */
#define CONFIG_CLI_BLE_TX_BUFFER_SIZE 64

// <q> Use CLI as logger backend
// <i> Enables the Logger on BLE concurrently with CLI.
/**@brief Logger BLE: Use CLI as logger backend */
#define CONFIG_CLI_BLE_LOG_BACKEND_ENABLED 1
// </h>
// </e>

// <o> Global logging level cap
// <i> This option defines the highest possible logging level. All other log level settings are capped at the level specified in this setting.
//  <0=> None
//  <1=> Error
//  <2=> Warning
//  <3=> Info
//  <4=> Debug
/**@brief Global logging level cap */
#define CONFIG_LOG_LEVEL_CAP 4
// </h>
// </h>

// <h> Bluetooth Options
// <i> This section contains the configuration settings for Bluetooth services.

// <h> Device Information
// <i> Configure the settings of the Bluetooth Device Information service.

// <s.20> Device Name
// <i> UTF-8 string representing the name of the device.
/**@brief Device Name */
#define CONFIG_DEVICE_NAME "Smart Remote 3"

// <s> Manufacturer Name
// <i> UTF-8 string representing the name of the manufacturer of the device.
/**@brief Manufacturer Name */
#define CONFIG_MANUFACTURER_NAME "Nordic Semiconductor"

// <s> Firmware Revision
// <i> UTF-8 string representing the firmware revision for the firmware within the device.
/**@brief Firmware Revision */
#define CONFIG_FIRMWARE_REVISION "1.2.1alpha"

// <s> Hardware Revision
// <i> UTF-8 string representing the hardware revision for the hardware within the device.
/**@brief Hardware Revision */
#define CONFIG_HARDWARE_REVISION "PCA20023"

// <s> Serial Number
// <i> UTF-8 string representing the serial number of the particular instance of the device.
/**@brief Serial Number */
#define CONFIG_SERIAL_NUMBER "0"

// <o> PNP Product ID <0x0000-0xFFFF>
// <i> Manufacturer-managed identifier of this product.
/**@brief PNP Product ID <0x0000-0xFFFF> */
#define CONFIG_PNP_PRODUCT_ID 0xEEEE

// <o> PNP Product Version <0x0000-0xFFFF>
// <i> Manufacturer-managed version of this product.
/**@brief PNP Product Version <0x0000-0xFFFF> */
#define CONFIG_PNP_PRODUCT_VERSION 0x0001

// <o> PNP Vendor ID Source <0x00-0xFF>
// <i> Identifies the source of the Vendor ID field.
/**@brief PNP Vendor ID Source <0x00-0xFF> */
#define CONFIG_PNP_VENDOR_ID_SOURCE 0x02

// <o> PNP Vendor ID <0x0000-0xFFFF>
// <i> Identifies the product vendor from the namespace in the Vendor ID Source.
/**@brief PNP Vendor ID <0x0000-0xFFFF> */
#define CONFIG_PNP_VENDOR_ID 0x1915
// </h>

// <h> Advertising Parameters
// <i> Configure the Bluetooth advertising parameters.

// <q> Use Directed Advertising
// <i> Enable Connectable Directed Advertising.
/**@brief Use Directed Advertising */
#define CONFIG_ADV_DIRECTED 0

// <q> Use Whitelist
// <i> Enable whitelisting of bonded peers.
// <i> Note: Bonds will automatically be deleted when Maximum Bonds has been reached and non-whitelisted advertisement is used.
/**@brief Use Whitelist */
#define CONFIG_ADV_WHITELIST 1

// <o> Advertising Time-out [s] <1-255>
// <i> Set the time after which the remote is automatically put into deep sleep mode if it cannot connect to the host.
/**@brief Advertising Time-out [s] <1-255> */
#define CONFIG_ADV_TIMEOUT 5

// <o> Advertising Interval [ms] <20-10240>
// <i> Configure the interval between advertising packets in the Advertising mode.
/**@brief Advertising Interval [ms] <20-10240> */
#define CONFIG_ADV_INTERVAL_MS 20
#define CONFIG_ADV_INTERVAL ROUNDED_DIV(1000u * CONFIG_ADV_INTERVAL_MS, 625)

// <e> Manufacturer data payload
// <i> This option adds manufacturer-specific data to the advertisement payload.
/**@brief Manufacturer data payload */
#define CONFIG_ADV_MANUF_ENABLED 0

// <o> Manufacturer ID <0x0000-0xFFFF>
// <i> Specify the manufacturer ID (as per Bluetooth Assigned numbers).
/**@brief Manufacturer ID <0x0000-0xFFFF> */
#define CONFIG_ADV_MANUF_ID 0x0B0D

// <s> Manufacturer data
// <i> Specify manufacturer data contents. Use %02x printf-style formatting.
/**@brief Manufacturer data */
#define CONFIG_ADV_MANUF_DATA "0A0B"
// </e>
// </h>

// <h> Connection Parameters
// <i> Configure the Bluetooth connection parameters.

// <o> Minimum Connection Interval [ms] <7-4000>
// <i> Set the Minimum Connection Interval that is sent in the Connection Parameter Update request.
/**@brief Minimum Connection Interval [ms] <7-4000> */
#define CONFIG_MIN_CONN_INTERVAL_MS 7
#define CONFIG_MIN_CONN_INTERVAL ROUNDED_DIV(100u * CONFIG_MIN_CONN_INTERVAL_MS, 125)

// <o> Maximum Connection Interval [ms] <7-4000>
// <i> Set the Maximum Connection Interval that is sent in the Connection Parameter Update request.
/**@brief Maximum Connection Interval [ms] <7-4000> */
#define CONFIG_MAX_CONN_INTERVAL_MS 7
#define CONFIG_MAX_CONN_INTERVAL ROUNDED_DIV(100u * CONFIG_MAX_CONN_INTERVAL_MS, 125)

// <o> Slave Latency [number of connection events] <0-1000>
// <i> Set the Slave Latency that is sent in the Connection Parameter Update request.
/**@brief Slave Latency [number of connection events] <0-1000> */
#define CONFIG_SLAVE_LATENCY 99

// <o> Connection Supervisory Time-out [ms] <100-32000>
// <i> Set the Connection Supervisory Time-out that is sent in the Connection Parameter Update request.
/**@brief Connection Supervisory Time-out [ms] <100-32000> */
#define CONFIG_CONN_SUP_TIMEOUT_MS 3000
#define CONFIG_CONN_SUP_TIMEOUT ROUNDED_DIV(CONFIG_CONN_SUP_TIMEOUT_MS, 10)

// <o> First Connection Parameter Update Delay [ms] <10-30000>
// <i> Set the time between establishing the connection and the first Connection Parameter Update request.
/**@brief First Connection Parameter Update Delay [ms] <10-30000> */
#define CONFIG_FIRST_CONN_PARAMS_UPDATE_DELAY 10

// <o> Next Connection Parameter Update Delay [s] <1-300>
// <i> Set the time between subsequent Connection Parameter Update requests.
/**@brief Next Connection Parameter Update Delay [s] <1-300> */
#define CONFIG_NEXT_CONN_PARAMS_UPDATE_DELAY 10

// <o> Maximum Attempts of Connection Parameter Negotiation <1-16>.
// <i> Set the number of attempts before giving up the Connection Parameter negotiation.
/**@brief Maximum Attempts of Connection Parameter Negotiation <1-16>. */
#define CONFIG_MAX_CONN_PARAMS_UPDATE_COUNT 3
// </h>

// <h> GATT Options
// <i> Configure the Bluetooth HID service.

// <o> Maximum MTU size <23-247>
// <i> Set the size of maximum transmission unit (MTU).
/**@brief Maximum MTU size <23-247> */
#define CONFIG_BLE_GATT_MAX_MTU_SIZE 23

// <o> GATT table size <:4>
// <i> SoftDevice Attribute Table size in bytes. The size must be a multiple of 4.
/**@brief GATT table size <:4> */
#define CONFIG_BLE_GATTS_ATTR_TAB_SIZE 1756
// </h>

// <h> Bonding Options
// <i> Configure the Bluetooth bonding settings.

// <o> Maximum Bonds <1-254>
// <i> Set the maximum number of bonds stored in flash memory.
/**@brief Maximum Bonds <1-254> */
#define CONFIG_MAX_BONDS 1

// <q> Change the device address for each new bond.
// <i> When enabled, the Bluetooth address is changed for each new bond.
// <i> This prevents the previously connected masters from attempting to reconnect.
/**@brief Change the device address for each new bond. */
#define CONFIG_CHANGE_ADDRESS 0

// <q> Attempt to reconnect to all bonded masters instead of only to the last one.
/**@brief Attempt to reconnect to all bonded masters instead of only to the last one. */
#define CONFIG_RECONNECT_ALL 0

// <q> When pairing is requested, attempt to reconnect to other bonded masters before advertising as bondable.
/**@brief When pairing is requested, attempt to reconnect to other bonded masters before advertising as bondable. */
#define CONFIG_BOND_RECONNECT_ALL 0

// <e> Use public address
// <i> This option will use a public 48-bit Bluetooth address programmed into UICR.CUSTOMER address instead of the default static random address.
// <i> Note: The address must be programmed separately during production into the appropriate UICR.CUSTOMER registers.
/**@brief Use public address */
#define CONFIG_BOND_PUBLIC_ADDR_ENABLED 0

// <o> Public Bluetooth address location
// <i> This is a 48-bit address, so 2 x 32-bit registers in UICR must be used.
// <i> If UICR.CUSTOMER[0] is selected, the last 16 bits of the address will be located in UICR.CUSTOMER[1]
// <i> Example of programming address AA:BB:CC:DD:EE:FF using nrfjprog to UICR.CUSTOMER[0] and UICR.CUSTOMER[1]:
// <i> nrfjprog --family NRF52 --memwr 0x10001080 --val 0xccddeeff
// <i> nrfjprog --family NRF52 --memwr 0x10001084 --val 0x0000aabb
//  <0x10001080=>0x10001080 (UICR.CUSTOMER[0])  <0x10001084=>0x10001084 (UICR.CUSTOMER[1])  <0x10001088=>0x10001088 (UICR.CUSTOMER[2])  <0x1000108c=>0x1000108c (UICR.CUSTOMER[3])
//  <0x10001090=>0x10001090 (UICR.CUSTOMER[4])  <0x10001094=>0x10001094 (UICR.CUSTOMER[5])  <0x10001098=>0x10001098 (UICR.CUSTOMER[6])  <0x1000109c=>0x1000109c (UICR.CUSTOMER[7])
//  <0x100010a0=>0x100010a0 (UICR.CUSTOMER[8])  <0x100010a4=>0x100010a4 (UICR.CUSTOMER[9])  <0x100010a8=>0x100010a8 (UICR.CUSTOMER[10]) <0x100010ac=>0x100010ac (UICR.CUSTOMER[11])
//  <0x100010b0=>0x100010b0 (UICR.CUSTOMER[12]) <0x100010b4=>0x100010b4 (UICR.CUSTOMER[13]) <0x100010b8=>0x100010b8 (UICR.CUSTOMER[14]) <0x100010bc=>0x100010bc (UICR.CUSTOMER[15])
//  <0x100010c0=>0x100010c0 (UICR.CUSTOMER[16]) <0x100010c4=>0x100010c4 (UICR.CUSTOMER[17]) <0x100010c8=>0x100010c8 (UICR.CUSTOMER[18]) <0x100010cc=>0x100010cc (UICR.CUSTOMER[19])
//  <0x100010d0=>0x100010d0 (UICR.CUSTOMER[20]) <0x100010d4=>0x100010d4 (UICR.CUSTOMER[21]) <0x100010d8=>0x100010d8 (UICR.CUSTOMER[22]) <0x100010dc=>0x100010dc (UICR.CUSTOMER[23])
//  <0x100010e0=>0x100010e0 (UICR.CUSTOMER[24]) <0x100010e4=>0x100010e4 (UICR.CUSTOMER[25]) <0x100010e8=>0x100010e8 (UICR.CUSTOMER[26]) <0x100010ec=>0x100010ec (UICR.CUSTOMER[27])
//  <0x100010f0=>0x100010f0 (UICR.CUSTOMER[28]) <0x100010f4=>0x100010f4 (UICR.CUSTOMER[29]) <0x100010f8=>0x100010f8 (UICR.CUSTOMER[30])
/**@brief Public Bluetooth address location */
#define CONFIG_BOND_PUBLIC_ADDR_LOC 268439680
// </e>
// </h>

// <h> Security Options
// <i> Configure the Bluetooth link security settings.

// <q> Allow Legacy Pairing
// <i> Allow for Legacy pairing. Refer to Bluetooth specification document for details.
/**@brief Allow Legacy Pairing */
#define CONFIG_SEC_LEGACY_PAIRING 1

// <q> Allow LESC Pairing
// <i> Allow for Low Energy Secure Connections pairing. Refer to Bluetooth specification document for details.
/**@brief Allow LESC Pairing */
#define CONFIG_SEC_LESC_PAIRING 1

// <q> Allow Repairing
// <i> Choose whether to allow a peer to pair if it wants to, when it is already bonded. In a production environment, this option should be disabled for increased security.
/**@brief Allow Repairing */
#define CONFIG_SEC_ALLOW_REPAIRING 1

// <q> Enable Bonding
// <i> Choose whether to allow the device to bond.
/**@brief Enable Bonding */
#define CONFIG_SEC_BOND 1

// <o> Minimum Encryption Key Size [octets] <7-16>
/**@brief Minimum Encryption Key Size [octets] <7-16> */
#define CONFIG_SEC_MIN_KEY_SIZE 7

// <o> Maximum Encryption Key Size [octets] <7-16>
/**@brief Maximum Encryption Key Size [octets] <7-16> */
#define CONFIG_SEC_MAX_KEY_SIZE 16

// <o> Encryption Request Delay [ms] <100-3000>
// <i> Set the time for which the remote will wait for connection encryption before sending Bluetooth authentication request.
/**@brief Encryption Request Delay [ms] <100-3000> */
#define CONFIG_ENC_REQ_DELAY 500
// </h>

// <h> PHY Configuration
// <i> Configure PHY Parameters.

// <q> Send PHY Update Request
// <i> If this option is enabled, the remote will send PHY Update Request to each connected host.
/**@brief Send PHY Update Request */
#define CONFIG_PHY_SEND_UPDATE_REQUEST 0

// <h> TX PHY Selection
// <q> Signal support for 1 Mbit/s PHY
/**@brief Signal support for 1 Mbit/s PHY */
#define CONFIG_PHY_TX_1MBPS 1

// <q> Signal support for 2 Mbit/s PHY
/**@brief Signal support for 2 Mbit/s PHY */
#define CONFIG_PHY_TX_2MBPS 1
// </h>

// <h> RX PHY Selection
// <q> Signal support for 1 Mbit/s PHY
/**@brief Signal support for 1 Mbit/s PHY */
#define CONFIG_PHY_RX_1MBPS 1

// <q> Signal support for 2 Mbit/s PHY
/**@brief Signal support for 2 Mbit/s PHY */
#define CONFIG_PHY_RX_2MBPS 1
// </h>
// </h>

// <h> HID Options
// <i> Configure the Bluetooth HID service.

// <o> HID Version <0x0000-0xFFFF>
// <i> Specify the HID Version number.
/**@brief HID Version <0x0000-0xFFFF> */
#define CONFIG_HID_VERSION 0x0101

// <o> HID Country Code <0x00-0xFF>
// <i> Numeric expression identifying the country code of the localized hardware.
/**@brief HID Country Code <0x00-0xFF> */
#define CONFIG_HID_COUNTRY_CODE 0
// </h>

// <h> Logging Options
// <i> This section configures module-specific logging options.

// <o> Module logging level
//  <0=> None
//  <1=> Error
//  <2=> Warning
//  <3=> Info
//  <4=> Debug
/**@brief BLE Module logging level */
#define CONFIG_BLE_MODULE_LOG_LEVEL 4

// <o> Advertising submodule logging level
//  <0=> None
//  <1=> Error
//  <2=> Warning
//  <3=> Info
//  <4=> Debug
/**@brief Advertising submodule logging level */
#define CONFIG_BLE_ADV_LOG_LEVEL 4

// <o> Address management submodule logging level
//  <0=> None
//  <1=> Error
//  <2=> Warning
//  <3=> Info
//  <4=> Debug
/**@brief Address management submodule logging level */
#define CONFIG_BLE_ADDR_LOG_LEVEL 4

// <o> HID submodule logging level
//  <0=> None
//  <1=> Error
//  <2=> Warning
//  <3=> Info
//  <4=> Debug
/**@brief HID submodule logging level */
#define CONFIG_BLE_HID_LOG_LEVEL 4

// <o> ATVV submodule logging level
//  <0=> None
//  <1=> Error
//  <2=> Warning
//  <3=> Info
//  <4=> Debug
/**@brief ATVV submodule logging level */
#define CONFIG_BLE_ATVV_LOG_LEVEL 4

// <o> DFU submodule logging level
//  <0=> None
//  <1=> Error
//  <2=> Warning
//  <3=> Info
//  <4=> Debug
/**@brief DFU submodule logging level */
#define CONFIG_BLE_DFU_LOG_LEVEL 4

// <o> LESC submodule logging level
//  <0=> None
//  <1=> Error
//  <2=> Warning
//  <3=> Info
//  <4=> Debug
/**@brief LESC submodule logging level */
#define CONFIG_BLE_LESC_LOG_LEVEL 4
// </h>

// <o> Radio TX Power
//  <0xD8=>-40 dBm  <0xE2=>-30 dBm  <0xEC=>-20 dBm  <0xF0=>-16 dBm
//  <0xF4=>-12 dBm  <0xF8=>-8 dBm   <0xFC=>-4 dBm   <0x00=>0 dBm
//  <0x04=>4 dBm
// <i> Set the power of the radio transmitter.
/**@brief Radio TX Power */
#define CONFIG_RADIO_TX_POWER 0

// <q> Enable PA/LNA Interface
// <i> This option enables the Power Amplifier (output) and/or Low Noise Amplifier (input) signaling pins.
/**@brief Enable PA/LNA Interface */
#define CONFIG_PA_LNA_ENABLED 0

// <o> Transmission Queue Size <1-16>
// <i> For audio, it is recommended to use value 6. For buttons or other non-data intensive uses, 1 or 2.
/**@brief Transmission Queue Size <1-16> */
#define CONFIG_GATTS_CONN_HVN_TX_QUEUE_SIZE 6
// </h>

// <h> I/O Configuration
// <i> This section contains configuration for the I/O pin assignment.

// <h> Board Power Control
// <i> Configure the Board Power Control settings.
// <o> Board Power Control Switch Pin
// <i> The Board Power Control Switch cuts off power to peripherals during sleep.
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief Board Power Control Switch Pin */
#define CONFIG_IO_BOARD_PWR_CTRL 0x07

// <q> Board Power Control Switch Active Low
// <i> Select this option if the Board Power Control Switch is active-low.
/**@brief Board Power Control Switch Active Low */
#define CONFIG_BOARD_PWR_CTRL_ACT_LOW 1
// </h>

// <h> IR Receiver
// <i> Configure the IR Receiver pin assignment.
// <i> Note that the IR Receiver is not implemented in the firmware and this section is only meant to help you correctly configure the pins.
// <o> IR Receiver Output Pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief IR Receiver Output Pin */
#define CONFIG_IO_IR_RX_OUT 0x15

// <o> IR Receiver Enable Pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief IR Receiver Enable Pin */
#define CONFIG_IO_IR_RX_ENABLE 0x17

// <q> IR Receiver Enable Active-Low
// <i> Select this option if the IR Receiver Enable signal is active-low.
/**@brief IR Receiver Enable Active-Low */
#define CONFIG_IR_RX_ENABLE_ACT_LOW 0
// </h>

// <h> IR Transmitter
// <i> Configure the IR Transmitter pin assignment.
// <o> IR Transmitter LED Pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief IR Transmitter LED Pin */
#define CONFIG_IO_IR_TX_LED 0x18
// </h>

// <h> GPIO-Based Matrix Keyboard
// <i> Configure the GPIO-Based Matrix Keyboard pin assignment.
// <o> Column 0 Pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief Matrix Keyboard: Column 0 Pin */
#define CONFIG_IO_KEY_COL_0 0x0C

// <o> Column 1 Pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief Matrix Keyboard: Column 1 Pin */
#define CONFIG_IO_KEY_COL_1 0x1C

// <o> Column 2 Pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief Matrix Keyboard: Column 2 Pin */
#define CONFIG_IO_KEY_COL_2 0x08

// <o> Column 3 Pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief Matrix Keyboard: Column 3 Pin */
#define CONFIG_IO_KEY_COL_3 0x0B

// <o> Column 4 Pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief Matrix Keyboard: Column 4 Pin */
#define CONFIG_IO_KEY_COL_4 0x03

// <o> Column 5 Pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief Matrix Keyboard: Column 5 Pin */
#define CONFIG_IO_KEY_COL_5 0x1B

// <o> Column 6 Pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief Matrix Keyboard: Column 6 Pin */
#define CONFIG_IO_KEY_COL_6 0x14

// <o> Column 7 Pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief Matrix Keyboard: Column 7 Pin */
#define CONFIG_IO_KEY_COL_7 0x19

// <o> Row 0 Pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief Matrix Keyboard: Row 0 Pin */
#define CONFIG_IO_KEY_ROW_0 0x10

// <o> Row 1 Pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief Matrix Keyboard: Row 1 Pin */
#define CONFIG_IO_KEY_ROW_1 0x11

// <o> Row 2 Pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief Matrix Keyboard: Row 2 Pin */
#define CONFIG_IO_KEY_ROW_2 0x12

// <o> Row 3 Pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief Matrix Keyboard: Row 3 Pin */
#define CONFIG_IO_KEY_ROW_3 0x1D

// <o> Row 4 Pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief Matrix Keyboard: Row 4 Pin */
#define CONFIG_IO_KEY_ROW_4 0x13

// <o> Row 5 Pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief Matrix Keyboard: Row 5 Pin */
#define CONFIG_IO_KEY_ROW_5 0xFFFFFFFF

// <o> Row 6 Pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief Matrix Keyboard: Row 6 Pin */
#define CONFIG_IO_KEY_ROW_6 0xFFFFFFFF

// <o> Row 7 Pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief Matrix Keyboard: Row 7 Pin */
#define CONFIG_IO_KEY_ROW_7 0xFFFFFFFF
// </h>

// <h> LEDs
// <i> Configure the LED output.

// <o> LED 1 Pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief LED 1 Pin */
#define CONFIG_IO_LED_1 0x00000002

// <o> LED 2 Pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief LED 2 Pin */
#define CONFIG_IO_LED_2 0xFFFFFFFF

// <o> LED 3 Pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief LED 3 Pin */
#define CONFIG_IO_LED_3 0xFFFFFFFF

// <o> LED 4 Pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief LED 4 Pin */
#define CONFIG_IO_LED_4 0xFFFFFFFF

// <o.0> LED 1 Active Low
// <i> Select this option if LED 1 output is active-low.
// <o.1> LED 2 Active Low
// <i> Select this option if LED 2 output is active-low.
// <o.2> LED 3 Active Low
// <i> Select this option if LED 3 output is active-low.
// <o.3> LED 4 Active Low
// <i> Select this option if LED 4 output is active-low.
/**@brief LED 1, 2, 3 and/or 4 (bitmask) Active Low */
#define CONFIG_LED_ACT_LOW_MASK 1

// <o> LED Power Control Pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief LED Power Control Pin */
#define CONFIG_IO_LED_PWR_CTRL 0xFFFFFFFF

// <q> LED Power Control Pin Active Low
// <i> Select this option if LED Power Control signal is active-low.
/**@brief LED Power Control Pin Active Low */
#define CONFIG_LED_PWR_CTRL_ACT_LOW 0
// </h>

// <h> PDM Interface
// <i> Configure the PDM interface.

// <o> PDM CLK Pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief PDM CLK Pin */
#define CONFIG_IO_PDM_CLK 0x06

// <o> PDM DATA Pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief PDM DATA Pin */
#define CONFIG_IO_PDM_DATA 0x05

// <o> PDM Microphone Power Control Pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief PDM Microphone Power Control Pin */
#define CONFIG_IO_PDM_MIC_PWR_CTRL 0x1A

// <q> PDM Microphone Power Control Pin Active Low
// <i> Select this option if PDM Microphone Power Control signal is active-low.
/**@brief PDM Microphone Power Control Pin Active Low */
#define CONFIG_PDM_MIC_PWR_CTRL_ACT_LOW 1
// </h>

// <h> UART Interface
// <i> Configure the UART interface.

// <o> TX Pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief UART: TX Pin */
#define CONFIG_UART_TX_PIN 0x06

// <o> RX Pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief UART: RX Pin */
#define CONFIG_UART_RX_PIN 0x08
// </h>

// <h> TWI Interface
// <i> Configure the primary and secondary TWI interfaces.

// <h> Primary TWI Interface
// <i> Configure the primary TWI interface.

// <o> SCL Pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief Primary TWI: SCL Pin */
#define CONFIG_IO_TWI0_SCL 0x0F

// <o> SDA Pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief Primary TWI: SDA Pin */
#define CONFIG_IO_TWI0_SDA 0x1F

// <q> Keep the interface powered up during System Off state.
/**@brief Primary TWI: Keep the interface powered up during System Off state. */
#define CONFIG_TWI0_UP_IN_SYSOFF_STATE 0
// </h>

// <h> Secondary TWI Interface
// <i> Configure the secondary TWI interface.

// <o> SCL Pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief Secondary TWI: SCL Pin */
#define CONFIG_IO_TWI1_SCL 0x0E

// <o> SDA Pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief Secondary TWI: SDA Pin */
#define CONFIG_IO_TWI1_SDA 0x0D

// <q> Keep the interface powered up during System Off state.
/**@brief Secondary TWI: Keep the interface powered up during System Off state. */
#define CONFIG_TWI1_UP_IN_SYSOFF_STATE 1
// </h>
// </h>

// <h> Touchpad DC/DC Converter Control
// <o> Discharge Pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief Touchpad DC/DC Converter: Discharge Pin */
#define CONFIG_IO_TP_DCDC_DISCHARGE 0xFFFFFFFF

// <q> Discharge Signal Active-Low
/**@brief Touchpad DC/DC Converter: Discharge Signal Active-Low */
#define CONFIG_TP_DCDC_DISCHARGE_ACT_LOW 0

// <o> Shutdown Pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief Touchpad DC/DC Converter: Shutdown Pin */
#define CONFIG_IO_TP_DCDC_SHUTDOWN 0xFFFFFFFF

// <q> Shutdown Signal Active-Low
/**@brief Touchpad DC/DC Converter: Shutdown Signal Active-Low */
#define CONFIG_TP_DCDC_SHUTDOWN_ACT_LOW 0
// </h>

// <h> PA/LNA Interface
// <i> Configure the PA/LNA interface.

// <o> PA pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief PA pin */
#define CONFIG_IO_PA 0xFFFFFFFF

// <o> LNA pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief LNA pin */
#define CONFIG_IO_LNA 0xFFFFFFFF

// <q> PA pin active low
/**@brief PA pin active low */
#define CONFIG_PA_ACTIVE_LOW 0

// <q> LNA pin active low
/**@brief LNA pin active low */
#define CONFIG_LNA_ACTIVE_LOW 0
// </h>

// <o> Accelerometer Interrupt Pin
// <i> Accelerometer Interrupt is used as a wakeup source during sleep.
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief Accelerometer Interrupt Pin */
#define CONFIG_IO_ACC_IRQ 0x1E

// <o> Buzzer Control Pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief Buzzer Control Pin */
#define CONFIG_IO_BUZZER 0x04
// </h>

// <h> Advanced Options
// <i> This section contains advanced configuration settings of the Smart Remote 3 nRF52 firmware.

// <h> Audio Buffering Configuration
// <i> This section configures the size of memory preallocated for audio processing.

// <o> Audio Buffer Pool Size <3-16>
// <i> More audio buffers provide better robustness of audio processing but require more memory resources.
/**@brief Audio Buffer Pool Size <3-16> */
#define CONFIG_AUDIO_BUFFER_POOL_SIZE 4

// <o> Audio Frame Pool Size <3-16>
// <i> More audio frames provide better robustness of audio transmission but require more memory resources.
/**@brief Audio Frame Pool Size <3-16> */
#define CONFIG_AUDIO_FRAME_POOL_SIZE 6
// </h>

// <h> Event Bus Configuration
// <i> Configure the Event Bus.

// <o> Event Pool size <4-254>
// <i> Specify how many events can be simultaneously queued in the Event Bus.
/**@brief Event Pool size <4-254> */
#define CONFIG_EVENT_POOL_SIZE 8

// <q> Force Event Bus Error Checking
// <i> By default, errors that appear during non-essential event handling are ignored.
// <i> When this option is enabled, all errors are fatal.
/**@brief Force Event Bus Error Checking */
#define CONFIG_EVENT_FORCE_ERROR_CHECKING 1

// <h> Logging Options
// <i> This section configures module-specific logging options.

// <o> Module logging level
//  <0=> None
//  <1=> Error
//  <2=> Warning
//  <3=> Info
//  <4=> Debug
/**@brief Event Bus Module logging level */
#define CONFIG_EVENT_BUS_LOG_LEVEL 4
// </h>
// </h>

// <h> HID Configuration
// <o> HID Report Pool Size <2-16>
// <i> More HID report buffers provide better robustness of keyboard operation but consume more memory resources.
/**@brief HID Report Pool Size <2-16> */
#define CONFIG_HID_REPORT_POOL_SIZE 8

// <o> HID Report Expiration [ms] <100-10000>
// <i> Define the time after which a HID report expires and is not sent again.
/**@brief HID Report Expiration [ms] <100-10000> */
#define CONFIG_HID_REPORT_EXPIRATION 500

// <o> Default protocol HID event queue size <2-255>
// <i> Specify protocol HID event queue size.
/**@brief Default protocol HID event queue size <2-255> */
#define CONFIG_PROTOCOL_HID_EVENT_QUEUE_SIZE 8

// <o> Default protocol HID state item count <1-255>
// <i> Specify protocol HID state item count.
/**@brief Default protocol HID state item count <1-255> */
#define CONFIG_PROTOCOL_HID_STATE_ITEM_COUNT 16

// <h> Logging Options
// <i> This section configures module-specific logging options.

// <o> Module logging level
//  <0=> None
//  <1=> Error
//  <2=> Warning
//  <3=> Info
//  <4=> Debug
/**@brief HID Protocol Module logging level */
#define CONFIG_PROTOCOL_HID_MODULE_LOG_LEVEL 4
// </h>
// </h>

// <h> ATVV Configuration

// <o> Control message queue size <1-100>
// <i> Determines how many control messages can be pending at a time.
/**@brief ATVV Configuration: Control message queue size <1-100> */
#define CONFIG_AUDIO_ATVV_CTL_MSG_QUEUE_LEN 2

// <o> Max number of ATVV peers <1-8>
// <i> Determines how many peers can interact with the ATVV Service.
// <i> Note that only one of these can stream audio at a time.
/**@brief ATVV Configuration: Max number of ATVV peers <1-8> */
#define CONFIG_AUDIO_ATVV_PEER_NUM 1

// <o> Frame fragment size [bytes] <20-244>
// <i> Maximum size for an audio frame fragment. Needs to be within MTU size, and follow the ATVV spec.
/**@brief ATVV Configuration: Frame fragment size [bytes] <20-244> */
#define CONFIG_AUDIO_ATVV_FRAME_FRAG_LEN 20

// <o> Key chord duration [ms] <0-1073741823>
// <i> Duration key chord must be held to generate ATVV key chord action.
/**@brief ATVV Configuration:  Key chord duration [ms] <0-1073741823> */
#define CONFIG_AUDIO_ATVV_KEY_CHORD_DURATION 3000
// </h>

// <h> TWI Transaction Scheduling Configuration
// <i> Configure the advanced TWI settings.
// <o> TWI Transaction Queue Size <1-16>
/**@brief TWI Transaction Queue Size <1-16> */
#define CONFIG_TWI_QSIZE 4

// <o> TWI Initialization Delay [ms] <0-1000>
// <i> Define the time to wait after TWI interface initialization to ensure all TWI devices are ready to respond.
/**@brief TWI Initialization Delay [ms] <0-1000> */
#define CONFIG_TWI_INIT_DELAY 4
// </h>

// <h> RNG Monitor Configuration
// <i> Configure Random Number Generator Monitor.
// <o> Request Queue Size <2-8>
// <i> Maximum number of requests enqueued to RNG Monitor.
/**@brief RNG Request Queue Size <2-8> */
#define CONFIG_RNG_MONITOR_MAX_REQUESTS 3

// <o> RNG Polling Interval [ms] <10-100>
// <i> If there is not enough entropy available, the RNG is polled at regular intervals to check if there is enough random data to generate keys.
/**@brief RNG Polling Interval [ms] <10-100> */
#define CONFIG_RNG_MONITOR_POLL_INTERVAL 20

// <h> Logging Options
// <i> This section configures module-specific logging options.

// <o> Module logging level
//  <0=> None
//  <1=> Error
//  <2=> Warning
//  <3=> Info
//  <4=> Debug
/**@brief RNG Module logging level */
#define CONFIG_RNG_MONITOR_LOG_LEVEL 4
// </h>
// </h>

// <h> Stack Configuration
// <i> Configure the stack settings.

// <o> Base Stack Size [bytes] <2048-65536:8>
// <i> Stack space reserved for the core of the Smart Remote 3 nRF52 firmware.
/**@brief Base Stack Size [bytes] <2048-65536:8> */
#define CONFIG_STACK_SIZE_BASE 2048

// <o> BV32FP Codec Stack Size [bytes] <0-65536:8>
// <i> Additional stack space reserved if the BV32 codec is used.
/**@brief BV32FP Codec Stack Size [bytes] <0-65536:8> */
#define CONFIG_STACK_SIZE_AUDIO_BV32FP 6144

// <o> OPUS/CELT Codec Stack Size [bytes] <0-65536:8>
// <i> Additional stack space reserved if the OPUC/CELT codec is used.
/**@brief OPUS/CELT Codec Stack Size [bytes] <0-65536:8> */
#define CONFIG_STACK_SIZE_AUDIO_OPUS_CELT 19456

// <o> OPUS/SILK Codec Stack Size [bytes] <0-65536:8>
// <i> Additional stack space reserved if the OPUC/SILK codec is used.
/**@brief OPUS/SILK Codec Stack Size [bytes] <0-65536:8> */
#define CONFIG_STACK_SIZE_AUDIO_OPUS_SILK 25344

// <o> Task Manager Stack Size for each task [bytes] <0-65536:8>
// <i> Stack space reserved by the task manager for each task.
/**@brief Task Manager Stack Size for each task [bytes] <0-65536:8> */
#define CONFIG_TASK_MANAGER_CONFIG_STACK_SIZE 640

// <e> Enable stack guard
// <i> Enable the module used for protecting the stack.
/**@brief Enable stack guard */
#define CONFIG_STACK_GUARD_ENABLED 1

// <o> Size of the stack guard
//  <5=> 32 bytes
//  <6=> 64 bytes
//  <7=> 128 bytes
//  <8=> 256 bytes
//  <9=> 512 bytes
//  <10=> 1024 bytes
//  <11=> 2048 bytes
//  <12=> 4096 bytes
/**@brief Size of the stack guard */
#define CONFIG_STACK_GUARD_CONFIG_SIZE 8

// <o> Task manager stack guard size.
//  <0=> Disabled
//  <4=> 32 bytes
//  <5=> 64 bytes
//  <6=> 128 bytes
//  <7=> 256 bytes
//  <8=> 512 bytes
/**@brief Task manager stack guard size. */
#define CONFIG_TASK_MANAGER_CONFIG_STACK_GUARD 4
// </e>
// </h>

// <q> Enable nRF52 DC/DC Converter
// <i> Use the nRF52 DC/DC converter. For details on the DC/DC converter, see the nRF52 Product Specification document.
/**@brief Enable nRF52 DC/DC Converter */
#define CONFIG_DCDC_ENABLED 1
// </h>

// <e> Debug Build
// <i> This option enables the debugging infrastructure of the Smart Remote 3 nRF52 firmware.
/**@brief Debug Build */
#define CONFIG_DEBUG_ENABLED 1

// <e> Enable Debug Pins
// <i> Enable the pins that are used for debugging.
/**@brief Enable Debug Pins */
#define CONFIG_DEBUG_PIN_ENABLED (0 && CONFIG_DEBUG_ENABLED)

// <o> CONFIG_IO_DBG_MARK Pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief Debug Pins: CONFIG_IO_DBG_MARK Pin */
#define CONFIG_IO_DBG_MARK 0xFFFFFFFF

// <o> CONFIG_IO_DBG_CLOCK Pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief Debug Pins: CONFIG_IO_DBG_CLOCK Pin */
#define CONFIG_IO_DBG_CLOCK 0xFFFFFFFF

// <o> CONFIG_IO_DBG_DATA Pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief Debug Pins: CONFIG_IO_DBG_DATA Pin */
#define CONFIG_IO_DBG_DATA 0xFFFFFFFF

// <o> CONFIG_IO_DBG_AUDIO_CAPTURE Pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief Debug Pins: CONFIG_IO_DBG_AUDIO_CAPTURE Pin */
#define CONFIG_IO_DBG_AUDIO_CAPTURE 0xFFFFFFFF

// <o> CONFIG_IO_DBG_AUDIO_PROCESS Pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief Debug Pins: CONFIG_IO_DBG_AUDIO_PROCESS Pin */
#define CONFIG_IO_DBG_AUDIO_PROCESS 0xFFFFFFFF

// <o> CONFIG_IO_DBG_SLEEP Pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief Debug Pins: CONFIG_IO_DBG_SLEEP Pin */
#define CONFIG_IO_DBG_SLEEP 0xFFFFFFFF

// <o> CONFIG_IO_DBG_IR_TX_PWM_INT Pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief Debug Pins: CONFIG_IO_DBG_IR_TX_PWM_INT Pin */
#define CONFIG_IO_DBG_IR_TX_PWM_INT 0xFFFFFFFF

// <o> CONFIG_IO_DBG_IR_TX_SACK Pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief Debug Pins: CONFIG_IO_DBG_IR_TX_SACK Pin */
#define CONFIG_IO_DBG_IR_TX_SACK 0xFFFFFFFF

// <o> CONFIG_IO_DBG_IR_TX_EACK Pin
//  <0=>P0.0   <1=>P0.1   <2=>P0.2   <3=>P0.3   <4=>P0.4   <5=>P0.5   <6=>P0.6   <7=>P0.7
//  <8=>P0.8   <9=>P0.9   <10=>P0.10 <11=>P0.11 <12=>P0.12 <13=>P0.13 <14=>P0.14 <15=>P0.15
//  <16=>P0.16 <17=>P0.17 <18=>P0.18 <19=>P0.19 <20=>P0.20 <21=>P0.21 <22=>P0.22 <23=>P0.23
//  <24=>P0.24 <25=>P0.25 <26=>P0.26 <27=>P0.27 <28=>P0.28 <29=>P0.29 <30=>P0.30 <31=>P0.31
//  <0xFFFFFFFF=>Disable
/**@brief Debug Pins: CONFIG_IO_DBG_IR_TX_EACK Pin */
#define CONFIG_IO_DBG_IR_TX_EACK 0xFFFFFFFF
// </e>

// <e> Enable Event Bus Monitor
// <i> Enable monitoring of the Event Bus activity on the RTT console.
/**@brief Enable Event Bus Monitor */
#define CONFIG_EVENT_MONITOR_ENABLED (1 && CONFIG_DEBUG_ENABLED && NRF_LOG_ENABLED)

// <o.0> Monitor System Events
// <i> Monitor system events in the Event Bus monitor.
// <o.1> Monitor Keyboard Events
// <i> Monitor keyboard events in the Event Bus monitor.
// <o.2> Monitor Motion Events
// <i> Monitor motion events in the Event Bus monitor.
// <o.3> Monitor HID Events
// <i> Monitor HID events in the Event Bus monitor.
// <o.4> Monitor IR Events
// <i> Monitor IR events in the Event Bus monitor.
// <o.5> Monitor Bluetooth Events
// <i> Monitor Bluetooth events in the Event Bus monitor.
/**@brief Monitor System, Keyboard, Motion, HID, IR and/or Bluetooth Events */
#define CONFIG_EVENT_MONITOR_TYPES 0x21
// </e>

// <e> Enable Audio Probe
// <i> This option enables receiving and sending audio through RTT pipes.
/**@brief Enable Audio Probe */
#define CONFIG_AUDIO_PROBE_ENABLED (0 && CONFIG_DEBUG_ENABLED && CONFIG_AUDIO_ENABLED)

// <o> Down Audio RTT Channels <0-64>
// <i> Set how many RTT channels are allocated from host to target for audio.
/**@brief Audio Probe: Down Audio RTT Channels <0-64> */
#define CONFIG_AUDIO_PROBE_RTT_CHANNELS_DOWN 0

// <o> Up Audio RTT Channels <0-64>
// <i> Set how many RTT channels are allocated from target to host for audio.
/**@brief Audio Probe: Up Audio RTT Channels <0-64> */
#define CONFIG_AUDIO_PROBE_RTT_CHANNELS_UP 1

// <o> Size of RTT buffer for audio injection
// <i> Number of audio buffers which fit into the RTT down buffer. With ANR enabled, the buffer takes twice the space.
/**@brief Audio Probe: Size of RTT buffer for audio injection */
#define CONFIG_AUDIO_PROBE_RTT_INJECT_BUFFERS 20

// <o> Size of RTT buffer for audio tapping
// <i> Number of audio buffers which fit into the RTT up buffer. With ANR enabled, the buffer takes twice the space.
/**@brief Audio Probe: Size of RTT buffer for audio tapping */
#define CONFIG_AUDIO_PROBE_RTT_TAP_BUFFERS 8

// <o> First audio channel index <2-63>
// <i> Index of the first RTT channel occupied by audio.
/**@brief Audio Probe: First audio channel index <2-63> */
#define CONFIG_AUDIO_PROBE_RTT_CHANNEL_FIRST 2
// </e>

// <q> Enable Stack Usage Profiler
// <i> When the system switches to sleep mode, Stack Usage Profiler displays the stack usage report in the given run.
/**@brief Enable Stack Usage Profiler */
#define CONFIG_STACK_PROFILER_ENABLED (1 && CONFIG_DEBUG_ENABLED && NRF_LOG_ENABLED)

// <q> Enable J-Link Monitor
// <i> When monitor mode debugging is enabled, the CPU can service time-critical tasks when the application is halted.
// <i> As a result, the wireless link can be maintained during a debugging session.
/**@brief Enable J-Link Monitor */
#define CONFIG_JLINK_MONITOR_ENABLED (0 && CONFIG_DEBUG_ENABLED)
// </e>
// <<< end of configuration section >>>

#endif /* _SR3_CONFIG_BOARD_H */
/** @} */

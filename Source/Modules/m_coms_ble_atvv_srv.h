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
 *
 * @defgroup ble_atvv Android TV Voice Service (ATVV)
 * @{
 * @ingroup  ble
 * @brief    Android TV Voice Service implementation.
 *
 * @details
 *
 * @note    The application must register this module as a BLE event observer using the
 *          NRF_SDH_BLE_OBSERVER macro. Example:
 *          @code
 *              ble_atvv_t instance;
 *              NRF_SDH_BLE_OBSERVER(anything, BLE_ATVV_BLE_OBSERVER_PRIO,
 *                                   ble_atvv_on_ble_evt, &instance);
 *          @endcode
 */
#ifndef BLE_ATVV_H__
#define BLE_ATVV_H__

#include <stdint.h>
#include <stdbool.h>

#include "sdk_config.h"
#include "ble.h"
#include "ble_srv_common.h"
#include "nrf_sdh_ble.h"
#include "resources.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ATVV_BLE_OBSERVER_PRIO BLE_OBSERVER_PRIORITY_LOW

/**@brief   Macro for defining a ble_atvv instance.
 *
 * @param   _name   Name of the instance.
 * @hideinitializer
 */
#define BLE_ATVV_DEF(_name)                      \
static ble_atvv_t _name;                         \
NRF_SDH_BLE_OBSERVER(_name ## _obs,              \
                     ATVV_BLE_OBSERVER_PRIO,     \
                     ble_atvv_on_ble_evt, &_name)

#if PM_BLE_OBSERVER_PRIO >= ATVV_BLE_OBSERVER_PRIO
#error Peer Manager should have higher observer priority than ATVV
// PM is needed to reapply sys_attr when connecting to bonded peer
#endif

#define BLE_UUID_ATV_BASE              {{0x64, 0xB6, 0x17, 0xF6,                                    \
                                         0x01, 0xAF, 0x7D, 0xBC,                                    \
                                         0x05, 0x4F, 0x21, 0x5A,                                    \
                                         0x00, 0x00, 0x5E, 0xAB}} /**< Used vendor-specific UUID.             */
#define BLE_UUID_ATV_SERVICE             0x0001                   /**< 16-bit UUID of the ATVV Service.       */
#define BLE_UUID_ATV_TX_CHARACTERISTIC   0x0002                   /**< 16-bit UUID of the TX Characteristic.  */
#define BLE_UUID_ATV_RX_CHARACTERISTIC   0x0003                   /**< 16-bit UUID of the RX Characteristic.  */
#define BLE_UUID_ATV_CTL_CHARACTERISTIC  0x0004                   /**< 16-bit UUID of the CTL Characteristic. */

#define BLE_ATVV_CHAR_TX_LEN  20
#define BLE_ATVV_CHAR_CTL_LEN 20

#define OPCODE_LENGTH 1
#define HANDLE_LENGTH 2

/**@brief   Maximum length of data (in bytes) that can be transmitted to the peer by the ATVV Service module. */
#if defined(NRF_SDH_BLE_GATT_MAX_MTU_SIZE) && (NRF_SDH_BLE_GATT_MAX_MTU_SIZE != 0)
    #define BLE_ATVV_MAX_DATA_LEN (NRF_SDH_BLE_GATT_MAX_MTU_SIZE - OPCODE_LENGTH - HANDLE_LENGTH)
#else
    #define BLE_ATVV_MAX_DATA_LEN (BLE_GATT_MTU_SIZE_DEFAULT - OPCODE_LENGTH - HANDLE_LENGTH)
    #warning NRF_SDH_BLE_GATT_MAX_MTU_SIZE is not defined.
#endif

/** @defgroup ATVV_VERSION Profile version
 * @{ */
#define ATVV_VERSION_MAJOR 0x00
#define ATVV_VERSION_MINOR 0x05
/** @} */

/** @defgroup ATVV_CMDS Command message values
 * @{ */
#define ATVV_CMD_GET_CAPS                          (0x0A)
#define ATVV_CMD_MIC_OPEN                          (0x0C)
#define ATVV_CMD_MIC_CLOSE                         (0x0D)
/** @} */

/** @defgroup ATVV_CTLS Control message values
 * @{ */
#define ATVV_CTL_AUDIO_STOP                        (0x00)
#define ATVV_CTL_AUDIO_START                       (0x04)
#define ATVV_CTL_START_SEARCH                      (0x08)
#define ATVV_CTL_AUDIO_SYNC                        (0x0A)
#define ATVV_CTL_GET_CAPS_RESP                     (0x0B)
#define ATVV_CTL_MIC_OPEN_ERROR                    (0x0C)
/** @} */

/** @defgroup ATVV_ERRORS Error codes
 * @{ */
#define ATVV_ERROR_INVALID_CODEC                   (0x0F01)
#define ATVV_ERROR_MIC_OPEN_TIMEOUT                (0x0F02)
#define ATVV_ERROR_INVALID_STATE                   (0x0F03)
/** @} */

/** @defgroup ATVV_CODECS_SUPPORTED Supported audio codec and rate combinations
 * @{ */
#define ATVV_CAPS_SUPP_CODEC_ADPCM_8KHZ            (0x0001)
#define ATVV_CAPS_SUPP_CODEC_ADPCM_8_KHZ_16KHZ     (0x0003)
#define ATVV_CAPS_SUPP_CODEC_ADPCM_OPUS_8KHZ       (0x0005)
#define ATVV_CAPS_SUPP_CODEC_ADPCM_OPUS_8KHZ_16KHZ (0x0007)
/** @} */

/** @defgroup ATVV_CODEC_USED Used audio codec
 * @{ */
#define ATVV_CAPS_USED_CODEC_ADPCM_8KHZ            (0x0001)
#define ATVV_CAPS_USED_CODEC_ADPCM_16KHZ           (0x0002)
#define ATVV_CAPS_USED_CODEC_OPUS                  (0x0004)
/** @} */

/** @defgroup ATVV_FRAME_SIZE Bytes per audio frame
 * @{ */
#define ATVV_CAPS_FRAME_SIZE_DEFAULT_134           (0x0086)
/** @} */

/** @defgroup ATVV_CHAR_SIZE Transmit Characteristic size
 * @{ */
#define ATVV_CAPS_CHAR_SIZE_DEFAULT_20             (0x0014)
#define ATVV_CAPS_CHAR_SIZE_MAX                    (ATVV_CAPS_CHAR_SIZE_DEFAULT_20)
/** @} */

/** @defgroup ATVV_TIMEOUTS Timeout values [millisecond]
 * @{ */
#define ATVV_TIMEOUT_1_MIC_OPEN                    (1000)
#define ATVV_TIMEOUT_2_STREAMING_DURATION          (7000)
/** @} */


/**@brief ATVV Service event types. */
typedef enum
{
    BLE_ATVV_EVT_ENABLED,   /**< Service enabled (CCCDs written).                                    */
    BLE_ATVV_EVT_DISABLED,  /**< Service disabled (CCCDs written, or disconnect).                    */
    BLE_ATVV_EVT_MIC_OPEN,  /**< Open mic command received.                                         */
    BLE_ATVV_EVT_MIC_CLOSE, /**< Close mic command received.                                        */
    BLE_ATVV_EVT_GET_CAPS,  /**< Get capability command received. Respond with @ref ble_atvv_ctl_send. */
} ble_atvv_evt_type_t;

/**@brief ATVV Service control message types. */
typedef enum
{
    BLE_ATVV_CTL_AUDIO_STOP     = ATVV_CTL_AUDIO_STOP,    /**< End of an audio message.          */
    BLE_ATVV_CTL_AUDIO_START    = ATVV_CTL_AUDIO_START,   /**< Beginning of an audio message.    */
    BLE_ATVV_CTL_START_SEARCH   = ATVV_CTL_START_SEARCH,  /**< Start a voice search message.     */
    BLE_ATVV_CTL_AUDIO_SYNC     = ATVV_CTL_AUDIO_SYNC,    /**< Frame count sync message.       */
    BLE_ATVV_CTL_GET_CAPS_RESP  = ATVV_CTL_GET_CAPS_RESP, /**< Reply to "get caps" command.    */
    BLE_ATVV_CTL_MIC_OPEN_ERROR = ATVV_CTL_MIC_OPEN_ERROR,/**< Error when opening the mic.         */
} ble_atvv_ctl_type_t;

/**@brief Supported codecs. */
typedef enum
{
    BLE_ATVV_SUPPORTED_CODECS_ADPCM_8KHZ            = ATVV_CAPS_SUPP_CODEC_ADPCM_8KHZ,
    BLE_ATVV_SUPPORTED_CODECS_ADPCM_8KHZ_16KHZ      = ATVV_CAPS_SUPP_CODEC_ADPCM_8_KHZ_16KHZ,
    BLE_ATVV_SUPPORTED_CODECS_ADPCM_OPUS_8KHZ       = ATVV_CAPS_SUPP_CODEC_ADPCM_OPUS_8KHZ,
    BLE_ATVV_SUPPORTED_CODECS_ADPCM_OPUS_8KHZ_16KHZ = ATVV_CAPS_SUPP_CODEC_ADPCM_OPUS_8KHZ_16KHZ,
} ble_atvv_supported_codecs_t;

/**@brief Used codec. */
typedef enum
{
    BLE_ATVV_USED_CODEC_ADPCM_8KHZ                  = ATVV_CAPS_USED_CODEC_ADPCM_8KHZ,
    BLE_ATVV_USED_CODEC_ADPCM_16KHZ                 = ATVV_CAPS_USED_CODEC_ADPCM_16KHZ,
    BLE_ATVV_USED_CODEC_OPUS                        = ATVV_CAPS_USED_CODEC_OPUS,
} ble_atvv_used_codec_t;

/**@brief Frame size. */
typedef enum
{
    BLE_ATVV_FRAME_SIZE_DEFAULT_134                 = ATVV_CAPS_FRAME_SIZE_DEFAULT_134,
} ble_atvv_frame_size_t;

/**@brief Transmit characteristic size. */
typedef enum
{
    BLE_ATVV_CHAR_SIZE_DEFAULT_20                   = ATVV_CAPS_CHAR_SIZE_DEFAULT_20,
} ble_atvv_rx_char_size_t;

/**@brief Error codes. */
typedef enum
{
    BLE_ATVV_ERROR_INVALID_CODEC                    = ATVV_ERROR_INVALID_CODEC,
    BLE_ATVV_ERROR_MIC_OPEN_TIMEOUT                 = ATVV_ERROR_MIC_OPEN_TIMEOUT,
    BLE_ATVV_ERROR_INVALID_STATE                    = ATVV_ERROR_INVALID_STATE,
} ble_atvv_error_code_t;

/* Forward declaration of the ble_atvv_t type. */
typedef struct ble_atvv_s ble_atvv_t;

/**@brief   ATVV Service @ref BLE_ATVV_EVT_MIC_OPEN event data.
 *
 * @details This structure is passed to an event when @ref BLE_ATVV_EVT_MIC_OPEN occurs.
 */
typedef struct
{
    ble_atvv_used_codec_t codec;                    /**< Mic should be opened and this codec used. */
} ble_atvv_evt_mic_open_t;

/**@brief   ATVV Service @ref BLE_ATVV_EVT_GET_CAPS event data.
 *
 * @details This structure is passed to an event when @ref BLE_ATVV_EVT_GET_CAPS occurs.
 */
typedef struct
{
    uint16_t                    peer_version;       /**< Peer ATVV Profile version. */
    ble_atvv_supported_codecs_t peer_codec_support; /**< Peer supports these codecs. */
} ble_atvv_evt_get_caps_t;

/**@brief   ATVV Service event structure.
 *
 * @details This structure is passed to an event coming from service.
 */
typedef struct
{
    ble_atvv_evt_type_t type;                /**< Event type. */
    ble_atvv_t const *  p_atvv;              /**< Pointer to the instance. */
    uint16_t            conn_handle;         /**< Connection which triggered the event. */
    union
    {
        ble_atvv_evt_mic_open_t mic_open;    /**< @ref BLE_ATVV_EVT_MIC_OPEN event data. */
        ble_atvv_evt_get_caps_t get_caps;    /**< @ref BLE_ATVV_EVT_GET_CAPS event data. */
    } params;
} ble_atvv_evt_t;

/**@brief   ATVV Service event handler type. */
typedef void (*ble_atvv_evt_handler_t) (ble_atvv_evt_t const * p_evt);

/**@brief   ATVV Service structure.
 *
 * @details This structure contains status information related to the service.
 */
struct ble_atvv_s
{
    uint8_t                     uuid_type;          /**< UUID type for ATVV Service Base UUID. */
    uint16_t                    service_handle;     /**< Handle of ATVV Service (as provided by the SoftDevice). */
    ble_gatts_char_handles_t    tx_handles;         /**< Handles related to the TX characteristic (as provided by the SoftDevice). */
    ble_gatts_char_handles_t    rx_handles;         /**< Handles related to the RX characteristic (as provided by the SoftDevice). */
    ble_gatts_char_handles_t    ctl_handles;        /**< Handles related to the CTL characteristic (as provided by the SoftDevice). */
    ble_atvv_evt_handler_t      event_handler;      /**< Event handler to be called for handling the received data. */
};


/**@brief   Function for initializing the ATVV Service.
 *
 * @param[out] p_atvv       ATVV Service structure. This structure must be supplied
 *                          by the application. It is initialized by this function and will
 *                          later be used to identify this particular service instance.
 * @param[in] p_evt_handler Event handler callback.
 *
 * @return NRF_SUCCESS If the service was successfully initialized. Otherwise, an error code is returned.
 */
ret_code_t ble_atvv_init(ble_atvv_t * p_atvv, ble_atvv_evt_handler_t p_evt_handler);


/**@brief   Function for handling BLE events of the ATVV Service.
 *
 * @param[in] p_ble_evt     Event received from the SoftDevice.
 * @param[in] p_context     ATVV Service structure.
 */
void ble_atvv_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context);

/**@brief   Function for sending control and error messages to the peer.
 *
 * @note "Get caps resp" message is automatically filled, based on parameters given in @ref ble_atvv_init.
 *
 * @param[in] p_atvv      ATVV Service structure. This structure must be supplied
 *                        by the application. It is initialized by this function and will
 *                        later be used to identify this particular service instance.
 * @param[in] type        Control message type.
 * @param[in] conn_handle Connection handle to use for message transmission.
 * @param[in] p_param     Depends on the @p type parameter:
 *                        "uint16_t frame_count" for BLE_ATVV_CTL_AUDIO_SYNC
 *                        "ble_atvv_error_code_t error" for BLE_ATVV_CTL_MIC_OPEN_ERROR
 *                        Otherwise, this parameter is ignored.
 *
 * @return NRF_SUCCESS If the control message was successfully sent.
 */
ret_code_t ble_atvv_ctl_send(ble_atvv_t const * p_atvv, uint16_t conn_handle, ble_atvv_ctl_type_t type,  void * p_param);

/**@brief   Function for transmitting frame data.
 *
 * @note This function does not perform frame buffering.
 *       Frame data size must conform to maximum bytes/characteristic parameter.
 *
 * @param[out] p_atvv      ATVV Service structure. This structure must be supplied
 *                         by the application. It is initialized by this function and will
 *                         later be used to identify this particular service instance.
 * @param[in] p_frame_data Frame data to transmit.
 * @param[in] len          Length of frame data.
 * @param[in] conn_handle  Connection handle to use for message transmission.
 *
 * @return NRF_SUCCESS If the frame data was successfully sent.
 */
ret_code_t ble_atvv_frame_data_transmit(ble_atvv_t const * p_atvv, uint8_t const * p_frame_data, uint16_t len, uint16_t conn_handle);

#ifdef __cplusplus
}
#endif

#endif // BLE_ATVV_H__

/** @} */

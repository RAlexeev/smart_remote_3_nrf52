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

#include "nrf_assert.h"
#include "nrf_queue.h"
#include "nrf_pwr_mgmt.h"
#include "app_debug.h"
#include "app_timer.h"

#include "m_audio.h"
#include "m_coms.h"
#include "m_coms_ble.h"
#include "m_coms_ble_adv.h"
#include "m_coms_ble_atvv.h"
#include "m_coms_ble_hid.h"
#include "m_protocol_hid_state.h"
#include "m_nfc.h"

#include "resources.h"
#include "sr3_config.h"

#define NRF_LOG_MODULE_NAME m_coms
#define NRF_LOG_LEVEL CONFIG_BLE_MODULE_LOG_LEVEL
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

// ----------------------------------------------------------------------------
// HID Descriptor
// ----------------------------------------------------------------------------

#if   (CONFIG_AUDIO_SAMPLING_FREQUENCY == 16000)
#define AUDIO_SAMPLING_FREQUENCY_ID     0x00
#elif (CONFIG_AUDIO_SAMPLING_FREQUENCY == 8000)
#define AUDIO_SAMPLING_FREQUENCY_ID     0x10
#elif (CONFIG_AUDIO_SAMPLING_FREQUENCY == 24000)
#define AUDIO_SAMPLING_FREQUENCY_ID     0x20
#elif (CONFIG_AUDIO_SAMPLING_FREQUENCY == 32000)
#define AUDIO_SAMPLING_FREQUENCY_ID     0x30
#else
# error "Unsuppored CONFIG_AUDIO_SAMPLING_FREQUENCY value!"
#endif

#if (CONFIG_AUDIO_CODEC == CONFIG_AUDIO_CODEC_BV32FP)
# define AUDIO_IN_REP_ID                1       /**< Audio Input Report ID. */
# define AUDIO_IN_REP_SIZE              8       /**< Size of a single field in the Audio Input Report [bits]. */
# define AUDIO_IN_REP_COUNT             20      /**< Number of fields in the Audio Input Report. */
# define AUDIO_CODEC_CONFIG_ID          0x00    /**< Audio codec configuration ID. */
#elif (CONFIG_AUDIO_CODEC == CONFIG_AUDIO_CODEC_ADPCM)
# define AUDIO_IN_REP_ID                2       /**< Audio Input Report ID. */
# define AUDIO_IN_REP_SIZE              8       /**< Size of a single field in the Audio Input Report [bits]. */
# define AUDIO_IN_REP_COUNT             20      /**< Number of fields in the Audio Input Report. */
# if (CONFIG_AUDIO_FRAME_SIZE_SAMPLES == 512)
#  define AUDIO_CODEC_CONFIG_ID         0x00    /**< Audio codec configuration ID. */
# elif (CONFIG_AUDIO_FRAME_SIZE_SAMPLES == 256)
#  define AUDIO_CODEC_CONFIG_ID         0x01    /**< Audio codec configuration ID. */
# elif (CONFIG_AUDIO_FRAME_SIZE_SAMPLES == 128)
#  define AUDIO_CODEC_CONFIG_ID         0x02    /**< Audio codec configuration ID. */
# else
#  error "Unsupported Audio Buffer Size!"
# endif
#elif (CONFIG_AUDIO_CODEC == CONFIG_AUDIO_CODEC_OPUS)
# define AUDIO_IN_REP_ID                3       /**< Audio Input Report ID. */
# define AUDIO_IN_REP_SIZE              8       /**< Size of a single field in the Audio Input Report [bits]. */
# define AUDIO_IN_REP_COUNT             (NRF_SDH_BLE_GATT_MAX_MTU_SIZE - 3) /**< Number of fields in the Audio Input Report. */
# define AUDIO_CODEC_CONFIG_ID          0x00    /**< Audio codec configuration ID. */
#elif (CONFIG_AUDIO_CODEC == CONFIG_AUDIO_CODEC_SBC)
# define AUDIO_IN_REP_ID                4       /**< Audio Input Report ID. */
# define AUDIO_IN_REP_SIZE              8       /**< Size of a single field in the Audio Input Report [bits]. */
# define AUDIO_IN_REP_COUNT             (NRF_SDH_BLE_GATT_MAX_MTU_SIZE - 3) /**< Number of fields in the Audio Input Report. */
# if (CONFIG_SBC_MODE == CONFIG_SBC_MODE_MSBC)
#  define AUDIO_CODEC_CONFIG_ID         0x00    /**< Audio codec configuration ID. */
# else
#  define AUDIO_CODEC_CONFIG_ID         0x01    /**< Audio codec configuration ID. */
# endif
#else
# error "Unsupported Audio Compression"
#endif

// Use AUDIO_CONFIG_ID to pass details about audio processing chain configuration.
#define AUDIO_CONFIG_ID                 (AUDIO_SAMPLING_FREQUENCY_ID | AUDIO_CODEC_CONFIG_ID)

// Make sure that the complete Audio Input Report size is byte aligned.
STATIC_ASSERT(((AUDIO_IN_REP_SIZE * AUDIO_IN_REP_COUNT) % 8) == 0);

#define AUDIO_OUT_REP_ID                1   /**< Audio Output Report ID. */
#define AUDIO_OUT_REP_SIZE              8   /**< Size of a single field in the Audio Output Report [bits]. */
#define AUDIO_OUT_REP_COUNT             1   /**< Number of fields in the Audio Output Report. */

// Make sure that the complete Audio Output Report size is byte aligned.
STATIC_ASSERT(((AUDIO_OUT_REP_SIZE * AUDIO_OUT_REP_COUNT) % 8) == 0);

#define KEYBOARD_IN_REP_ID              5   /**< Keyboard Input Report ID. */
#define KEYBOARD_IN_REP_SIZE            8   /**< Size of a single field in the Keyboard Input Report [bits]. */
#define KEYBOARD_IN_REP_COUNT           3   /**< Number of fields in the Keyboard Input Report. */

// Make sure that the complete Keyboard Input Report size is byte aligned.
STATIC_ASSERT(((KEYBOARD_IN_REP_SIZE * KEYBOARD_IN_REP_COUNT) % 8) == 0);

#define CONSUMER_CTRL_IN_REP_ID         6   /**< Consumer Control Input Report ID. */
#define CONSUMER_CTRL_IN_REP_SIZE       12  /**< Size of a single field in the Consumer Control Input Report [bits]. */
#define CONSUMER_CTRL_IN_REP_COUNT      2   /**< Number of fields in the Consumer Control Input Report. */

// Make sure that the complete Consumer Control Input Report size is byte aligned.
STATIC_ASSERT(((CONSUMER_CTRL_IN_REP_SIZE * CONSUMER_CTRL_IN_REP_COUNT) % 8) == 0);

#define MOUSE_BTN_IN_REP_ID             7   /**< Mouse Button Input Report ID. */
#define MOUSE_BTN_IN_REP_SIZE           1   /**< Size of a single field in the Mouse Button Input Report [bits]. */
#define MOUSE_BTN_IN_REP_COUNT          8   /**< Number of fields in the Mouse Button Input Report. */

// Make sure that the complete Mouse Button Input Report size is byte aligned.
STATIC_ASSERT(((MOUSE_BTN_IN_REP_SIZE * MOUSE_BTN_IN_REP_COUNT) % 8) == 0);

#define MOUSE_XY_IN_REP_ID              8   /**< Mouse X/Y Input Report ID. */
#define MOUSE_XY_IN_REP_SIZE            12  /**< Size of a single field in the Mouse X/Y Input Report [bits]. */
#define MOUSE_XY_IN_REP_COUNT           2   /**< Number of fields in the Mouse X/Y Input Report. */

// Make sure that the complete Mouse X/Y Input Report size is byte aligned.
STATIC_ASSERT(((MOUSE_XY_IN_REP_SIZE * MOUSE_XY_IN_REP_COUNT) % 8) == 0);

#define MOUSE_WP_IN_REP_ID              9   /**< Mouse Wheel/Pan Input Report ID. */
#define MOUSE_WP_IN_REP_SIZE            8   /**< Size of a single field in the Mouse Wheel/Pan Input Report [bits]. */
#define MOUSE_WP_IN_REP_COUNT           2   /**< Number of fields in the Mouse Wheel/Pan Input Report. */

// Make sure that the complete Mouse Wheel/Pan Input Report size is byte aligned.
STATIC_ASSERT(((MOUSE_WP_IN_REP_SIZE * MOUSE_WP_IN_REP_COUNT) % 8) == 0);

// Make sure that Mouse Wheel/Pan report count matches HID descriptor.
STATIC_ASSERT(MOUSE_WP_IN_REP_COUNT == 2);

/**@brief Get the ID of a given HID report. */
#define HID_REPORT_ID(_name)    (_name ## _REP_ID)

/**@brief Get the Index of a given HID report. */
#define HID_REPORT_IDX(_name)   (HID_##_name##_REP_IDX)

/**@brief Get the total size of a given HID report in bytes. */
#define HID_REPORT_SIZE(_name)  ((_name ## _REP_SIZE) * (_name ## _REP_COUNT) / 8)

__ALIGN(4) static const uint8_t m_coms_hid_desc[] =
{
#if CONFIG_AUDIO_ENABLED && CONFIG_AUDIO_HID_ENABLED
    /*
     * Audio
     */
    0x06, 0x00, 0xFF,               // Usage Page (Vendor Defined Page 1)
    0x09, AUDIO_CONFIG_ID,          // Usage (Vendor Usage)
    0xA1, 0x01,                     // Collection (Application)
    0x85, AUDIO_IN_REP_ID,          //     Report ID (AUDIO_IN_REP_ID)
    0x15, 0x00,                     //     Logical Minimum (0)
    0x25, 0xFF,                     //     Logical Maximum (255)
    0x75, AUDIO_IN_REP_SIZE,        //     Report Size (AUDIO_IN_REP_SIZE)
    0x95, AUDIO_IN_REP_COUNT,       //     Report Count (AUDIO_IN_REP_COUNT)
    0x09, 0x00,                     //     Usage (Vendor Usage 1)
    0x81, 0x02,                     //     Input (Data, Var, Abs)
#if (AUDIO_IN_REP_ID != AUDIO_OUT_REP_ID)
    0x85, AUDIO_OUT_REP_ID,         //     Report ID (AUDIO_OUT_REP_ID)
#endif
    0x95, AUDIO_OUT_REP_COUNT,      //     Report Count (AUDIO_OUT_REP_COUNT)
    0x75, AUDIO_OUT_REP_SIZE,       //     Report Size (AUDIO_OUT_REP_SIZE)
    0x09, 0x00,                     //     Usage (Vendor Usage 1)
    0x91, 0x02,                     //     Output (Data, Var, Abs)
    0xC0,                           // End Collection
#endif /* CONFIG_AUDIO_ENABLED && CONFIG_AUDIO_HID_ENABLED */

    /*
     * Keyboard
     */
    0x05, 0x01,                     // Usage Page (Generic Desktop)
    0x09, 0x06,                     // Usage (Keyboard)
    0xA1, 0x01,                     // Collection (Application)
    0x85, KEYBOARD_IN_REP_ID,       //      Report ID (KEYBOARD_IN_REP_ID)
    0x75, KEYBOARD_IN_REP_SIZE,     //      Report Size (KEYBOARD_IN_REP_SIZE)
    0x95, KEYBOARD_IN_REP_COUNT,    //      Report Count (KEYBOARD_IN_REP_COUNT)
    0x15, 0x00,                     //      Logical Minimum (0)
    0x25, 0xFF,                     //      Logical Maximum (255)
    0x05, 0x07,                     //      Usage Page (Keyboard)
    0x19, 0x00,                     //      Usage Minimum (0)
    0x29, 0xFF,                     //      Usage Maximum (255)
    0x81, 0x00,                     //      Input (Data, Ary, Abs)
    0xC0,                           // End Collection

    /*
     * Consumer Control
     */
    0x05, 0x0C,                     // Usage Page (Consumer Devices)
    0x09, 0x01,                     // Usage (Consumer Control)
    0xA1, 0x01,                     // Collection (Application)
    0x85, CONSUMER_CTRL_IN_REP_ID,  //      Report ID (CONSUMER_CTRL_IN_REP_ID)
    0x75, CONSUMER_CTRL_IN_REP_SIZE,//      Report Size (CONSUMER_CTRL_IN_REP_SIZE)
    0x95, CONSUMER_CTRL_IN_REP_COUNT,//     Report Count (CONSUMER_CTRL_IN_REP_COUNT)
    0x15, 0x00,                     //      Logical Minimum (0)
    0x26, 0xFF, 0x07,               //      Logical Maximum (2047)
    0x19, 0x00,                     //      Usage Minimum (0)
    0x2A, 0xFF, 0x07,               //      Usage Maximum (2047)
    0x81, 0x00,                     //      Input (Data, Ary, Abs)
    0xC0,                           // End Collection

    /*
     * Mouse
     */
    0x05, 0x01,                     // Usage Page (Generic Desktop)
    0x09, 0x02,                     // Usage (Mouse)
    0xA1, 0x01,                     // Collection (Application)
    0x09, 0x01,                     //       Usage (Pointer)
    0xA1, 0x00,                     //       Collection (Physical)
    0x85, MOUSE_BTN_IN_REP_ID,      //          Report ID (MOUSE_BTN_IN_REP_ID)
    0x75, MOUSE_BTN_IN_REP_SIZE,    //          Report Size (MOUSE_BTN_IN_REP_SIZE)
    0x95, MOUSE_BTN_IN_REP_COUNT,   //          Report Count (MOUSE_BTN_IN_REP_COUNT)
    0x15, 0x00,                     //          Logical Minimum (0)
    0x25, 0x01,                     //          Logical Maximum (1)
    0x05, 0x09,                     //          Usage Page (Button)
    0x19, 0x01,                     //          Usage Minimum (Button 1)
    0x29, 0x08,                     //          Usage Maximum (Button 8)
    0x81, 0x02,                     //          Input (Data, Var, Abs)
    0xC0,                           //      End Collection
    0xA1, 0x00,                     //      Collection (Physical)
    0x85, MOUSE_XY_IN_REP_ID,       //          Report ID (MOUSE_XY_IN_REP_ID)
    0x75, MOUSE_XY_IN_REP_SIZE,     //          Report Size (MOUSE_XY_IN_REP_SIZE)
    0x95, MOUSE_XY_IN_REP_COUNT,    //          Report Count (MOUSE_XY_IN_REP_COUNT)
    0x16, 0x01, 0xF8,               //          Logical Minimum (-2047)
    0x26, 0xFF, 0x07,               //          Logical Maximum (2047)
    0x05, 0x01,                     //          Usage Page (Generic Desktop)
    0x09, 0x30,                     //          Usage (X)
    0x09, 0x31,                     //          Usage (Y)
    0x81, 0x06,                     //          Input (Data, Var, Rel)
    0xC0,                           //      End Collection
    0xA1, 0x00,                     //      Collection (Physical)
    0x85, MOUSE_WP_IN_REP_ID,       //          Report ID (MOUSE_WP_IN_REP_ID)
    0x75, MOUSE_WP_IN_REP_SIZE,     //          Report Size (MOUSE_WP_IN_REP_SIZE)
    0x95, 0x01,                     //          Report Count (1)
    0x15, 0x81,                     //          Logical Minimum (-127)
    0x25, 0x7F,                     //          Logical Maximum (127)
    0x05, 0x01,                     //          Usage Page (Generic Desktop)
    0x09, 0x38,                     //          Usage (Wheel)
    0x81, 0x06,                     //          Input (Data, Var, Rel)
    0x75, MOUSE_WP_IN_REP_SIZE,     //          Report Size (MOUSE_WP_IN_REP_SIZE)
    0x95, 0x01,                     //          Report Count (1)
    0x15, 0x81,                     //          Logical Minimum (-127)
    0x25, 0x7F,                     //          Logical Maximum (127)
    0x05, 0x0C,                     //          Usage Page (Consumer Devices)
    0x0A, 0x38, 0x02,               //          Usage (AC Pan)
    0x81, 0x06,                     //          Input (Data, Var, Rel)
    0xC0,                           //      End Collection
    0xC0,                           // End Collection
};

// ----------------------------------------------------------------------------
// BLE Parameters
// ----------------------------------------------------------------------------

// Ensure that the connection supervision timeout value is set according to GAP specification.
#if (CONFIG_CONN_SUP_TIMEOUT_MS <= ((1 + CONFIG_SLAVE_LATENCY) * CONFIG_MIN_CONN_INTERVAL_MS * 2))
# error "Invalid CONFIG_CONN_SUP_TIMEOUT value!"
#endif

// Validate that CONFIG_ADV_TIMEOUT can fit into used data type and is not zero.
STATIC_ASSERT((CONFIG_ADV_TIMEOUT <= UINT16_MAX) && (CONFIG_ADV_TIMEOUT != 0));


static void m_coms_ble_evt_handler(void *p_event_data, uint16_t event_size);
static void m_coms_hid_boot_mode_callback(m_coms_hid_boot_pkt_t *      p_boot_pkt,
                                          m_coms_ble_hid_boot_type_t * p_pkt_type,
                                          const uint8_t *              p_data,
                                          uint8_t                      len,
                                          uint8_t                      hid_interface,
                                          uint8_t                      report_idx);

static const m_coms_ble_params_t m_ble_params =
{
    // Device information:
    .device_info.device_name            = CONFIG_DEVICE_NAME,
    .device_info.manufacturer_name      = CONFIG_MANUFACTURER_NAME,
    .device_info.fw_revision            = CONFIG_FIRMWARE_REVISION,
    .device_info.hw_revision            = CONFIG_HARDWARE_REVISION,
    .device_info.serial_number          = CONFIG_SERIAL_NUMBER,
    .device_info.pnp_product_id         = CONFIG_PNP_PRODUCT_ID,
    .device_info.pnp_product_version    = CONFIG_PNP_PRODUCT_VERSION,
    .device_info.pnp_vendor_id          = CONFIG_PNP_VENDOR_ID,
    .device_info.pnp_vendor_id_src      = CONFIG_PNP_VENDOR_ID_SOURCE,

    // Advertising parameters:
    .adv_params.interval                = CONFIG_ADV_INTERVAL,
    .adv_params.timeout                 = CONFIG_ADV_TIMEOUT,

    // Bond behavior parameters:
    .bond_params.directed_adv           = CONFIG_ADV_DIRECTED,
    .bond_params.change_address         = CONFIG_CHANGE_ADDRESS,
    .bond_params.bond_reconnect_all     = CONFIG_BOND_RECONNECT_ALL,
    .bond_params.reconnect_all          = CONFIG_RECONNECT_ALL,

    // Preferred connection parameters:
    .conn_params.min_conn_interval      = CONFIG_MIN_CONN_INTERVAL,
    .conn_params.max_conn_interval      = CONFIG_MAX_CONN_INTERVAL,
    .conn_params.slave_latency          = CONFIG_SLAVE_LATENCY,
    .conn_params.conn_sup_timeout       = CONFIG_CONN_SUP_TIMEOUT,

    // Security parameters:
    .sec_params.io_capabilities         = BLE_GAP_IO_CAPS_NONE,

    // Misc parameters:
    .appearance                         = BLE_APPEARANCE_GENERIC_REMOTE_CONTROL,
    .base_hid_version                   = CONFIG_HID_VERSION,
    .hid_country_code                   = CONFIG_HID_COUNTRY_CODE,

    // HID Service flags:
    // Remote wakeup indicates that this device considers itself capable of waking up the central device.
    // A normally connectable flag indicates that this device is normally advertising (and can be connected to) when not already connected.
    .hids_flags                         = HID_INFO_FLAG_REMOTE_WAKE_MSK | HID_INFO_FLAG_NORMALLY_CONNECTABLE_MSK,

    // Event handlers:
    .event_handler                      = m_coms_ble_evt_handler,
    .boot_mode_callback                 = m_coms_hid_boot_mode_callback,
};

// ----------------------------------------------------------------------------
// Communication module state
// ----------------------------------------------------------------------------

/**@brief Communication module state definition. */
typedef enum {
    M_COMS_STATE_SHUTDOWN,                          /**< Module shutdown. */
    M_COMS_STATE_INITIALIZED,                       /**< Module initialized and not connected. */
    M_COMS_STATE_CONNECTED,                         /**< Module initialized and connected. */
    M_COMS_STATE_SECURED,                           /**< Module initialized and connected. Connection secured. */
} m_coms_state_t;

static m_coms_state_t   m_coms_state;               /**< State of the module. See \ref m_coms_state_t. */
static uint8_t          m_coms_packets_in_fly;      /**< Number of packets enqueued for sending, but not acknowledged. */
static uint8_t          m_coms_max_packets_in_fly;  /**< Maximum number of packets in SD queue. */
#if CONFIG_AUDIO_ENABLED
static uint8_t          m_coms_effective_mtu;       /**< Effective MTU of the connection. */
static uint8_t          m_coms_audio_srv_bitmsk;    /**< Audio services enabled for transmission. */
#endif
#if CONFIG_PWR_MGMT_ENABLED
static bool             m_coms_going_down;          /**< True if module is executing a shutdown procedure. */
#endif

// ----------------------------------------------------------------------------
// HID Interface
// ----------------------------------------------------------------------------

enum {
    HID_BASE_INTERFACE_IDX,       /**< HID Interface index. */
    HID_NUMBER_OF_INTERFACES,     /**< Last Interface IDX.*/
};

enum {
#if CONFIG_AUDIO_ENABLED && CONFIG_AUDIO_HID_ENABLED
    HID_AUDIO_IN_REP_IDX,         /**< HID Audio Input Report Index. */
#endif
    HID_KEYBOARD_IN_REP_IDX,      /**< HID Keyboard Input Report Index. */
    HID_MOUSE_BTN_IN_REP_IDX,     /**< HID Mouse Buttons Input Report Index. */
    HID_MOUSE_XY_IN_REP_IDX,      /**< HID Mouse X/Y Input Report Index. */
    HID_MOUSE_WP_IN_REP_IDX,      /**< HID Mouse Wheel/Pan Input Report Index. */
    HID_CONSUMER_CTRL_IN_REP_IDX, /**< HID Consumer Control Input Report Index. */

    HID_NUMBER_OF_IN_REPS,        /**< Number of IN reports.*/
};

enum {
#if CONFIG_AUDIO_ENABLED && CONFIG_AUDIO_HID_ENABLED
    HID_AUDIO_OUT_REP_IDX,    /**< HID Audio Output Report Index. */
#endif
    HID_NUMBER_OF_OUT_REPS    /**< Number of OUT reports.*/
};

// ----------------------------------------------------------------------------
// HID report pools and queues
// ----------------------------------------------------------------------------

/*
 * The KEYBOARD_IN, CONSUMER_CTRL_IN, and MOUSE_BTN_IN reports are queued and sent one by one.
 * The MOUSE_XY_IN and MOUSE_WP_IN are created dynamically based on the accumulated motion values.
 */
#define HID_MAX_QUEUED_REPORT_SIZE  MAX(HID_REPORT_SIZE(KEYBOARD_IN), MAX(HID_REPORT_SIZE(CONSUMER_CTRL_IN), HID_REPORT_SIZE(MOUSE_BTN_IN)))

typedef enum
{
    M_COMS_SERVICE_TYPE_HID,
    M_COMS_SERVICE_TYPE_ATVV
} m_coms_service_type_t;

typedef struct
{
    uint8_t            *p_data;                  /**< Pointer to the data to send. */
    m_coms_free_func_t free_func;                /**< Free function that is called when all data is sent. */
    void               *p_free_func_context;     /**< Context that is passed to the free function. */
    uint16_t           data_size;                /**< Size of the data to send. */

    union
    {
        struct
        {
            uint8_t  interface_idx;              /**< HID Interface index. */
            uint8_t  report_idx;                 /**< HID Report index. */
            uint16_t max_packet_size;            /**< Maximum packet size. */
        } hid;
    } service_params;

    m_coms_service_type_t type;                  /**< Type of data. */
} m_coms_data_desc_t;

typedef struct
{
    m_coms_data_desc_t *p_current_data_desc;     /**< Descriptor of the data that is currently transmitted. */
    const nrf_queue_t  *p_backlog;               /**< Transmission queue. */
} m_coms_channel_t;

#if CONFIG_AUDIO_ENABLED && CONFIG_AUDIO_HID_ENABLED
static m_coms_channel_t m_coms_audio_hid_channel;  /**< Audio channel (HID service) */

/**@brief Backlog queue for audio channel. */
NRF_QUEUE_DEF(m_coms_data_desc_t *,
              m_coms_audio_hid_channel_backlog,
              (CONFIG_AUDIO_FRAME_POOL_SIZE - 1),
              NRF_QUEUE_MODE_NO_OVERFLOW);
#endif /* CONFIG_AUDIO_ENABLED && CONFIG_AUDIO_HID_ENABLED */

#if CONFIG_AUDIO_ENABLED && CONFIG_AUDIO_ATVV_ENABLED
static m_coms_channel_t m_coms_audio_atvv_channel; /**< Audio channel (ATVV service) */

/**@brief Backlog queue for audio channel. */
NRF_QUEUE_DEF(m_coms_data_desc_t *,
              m_coms_audio_atvv_channel_backlog,
              (CONFIG_AUDIO_FRAME_POOL_SIZE - 1),
              NRF_QUEUE_MODE_NO_OVERFLOW);
#endif /* CONFIG_AUDIO_ENABLED && CONFIG_AUDIO_ATVV_ENABLED */

static m_coms_channel_t m_coms_keys_channel;       /**< Keys channel. */

/**@brief Backlog queue for keys channel. */
NRF_QUEUE_DEF(m_coms_data_desc_t *,
              m_coms_keys_channel_backlog,
              (CONFIG_AUDIO_FRAME_POOL_SIZE - 1),
              NRF_QUEUE_MODE_NO_OVERFLOW);

/**@brief Allocator for report buffers. */
NRF_BALLOC_DEF(m_coms_report_pool,
               HID_MAX_QUEUED_REPORT_SIZE,
               (CONFIG_HID_REPORT_POOL_SIZE + 1));


/**@brief Allocator for report descriptors. */
NRF_BALLOC_DEF(m_coms_data_desc_pool,
               sizeof(m_coms_data_desc_t),
               (CONFIG_AUDIO_FRAME_POOL_SIZE + CONFIG_HID_REPORT_POOL_SIZE + 1));

static ble_hids_t m_ble_hids_instances[HID_NUMBER_OF_INTERFACES];

static const ble_hid_report_map_record_t m_report_maps[] = {
        [HID_BASE_INTERFACE_IDX] = BLE_HID_REPORT_MAP_CONF(
            &m_ble_hids_instances[HID_BASE_INTERFACE_IDX],
            (uint8_t *)m_coms_hid_desc,
            sizeof(m_coms_hid_desc),
            ble_boot_pkt_none
        )
};

static const ble_hid_report_record_t m_reports_in[] = {
#if CONFIG_AUDIO_ENABLED && CONFIG_AUDIO_HID_ENABLED
            [HID_AUDIO_IN_REP_IDX] = BLE_HID_REPORT_CONF(
                    HID_BASE_INTERFACE_IDX,
                    hid_report_type_input,
                    false,
                    HID_REPORT_ID(AUDIO_IN),
                    HID_REPORT_SIZE(AUDIO_IN)
            ),
#endif
            [HID_KEYBOARD_IN_REP_IDX] = BLE_HID_REPORT_CONF(
                    HID_BASE_INTERFACE_IDX,
                    hid_report_type_input,
                    false,
                    HID_REPORT_ID(KEYBOARD_IN),
                    HID_REPORT_SIZE(KEYBOARD_IN)
            ),
            [HID_MOUSE_BTN_IN_REP_IDX] = BLE_HID_REPORT_CONF(
                    HID_BASE_INTERFACE_IDX,
                    hid_report_type_input,
                    false,
                    HID_REPORT_ID(MOUSE_BTN_IN),
                    HID_REPORT_SIZE(MOUSE_BTN_IN)
            ),
            [HID_MOUSE_XY_IN_REP_IDX] = BLE_HID_REPORT_CONF(
                    HID_BASE_INTERFACE_IDX,
                    hid_report_type_input,
                    false,
                    HID_REPORT_ID(MOUSE_XY_IN),
                    HID_REPORT_SIZE(MOUSE_XY_IN)
            ),
            [HID_MOUSE_WP_IN_REP_IDX] = BLE_HID_REPORT_CONF(
                    HID_BASE_INTERFACE_IDX,
                    hid_report_type_input,
                    false,
                    HID_REPORT_ID(MOUSE_WP_IN),
                    HID_REPORT_SIZE(MOUSE_WP_IN)
            ),
            [HID_CONSUMER_CTRL_IN_REP_IDX] = BLE_HID_REPORT_CONF(
                    HID_BASE_INTERFACE_IDX,
                    hid_report_type_input,
                    false,
                    HID_REPORT_ID(CONSUMER_CTRL_IN),
                    HID_REPORT_SIZE(CONSUMER_CTRL_IN)
            ),
            [HID_NUMBER_OF_IN_REPS] = { 0 }
};

static const ble_hid_report_record_t m_reports_out[] = {
#if CONFIG_AUDIO_ENABLED && CONFIG_AUDIO_HID_ENABLED
        [HID_AUDIO_OUT_REP_IDX] = BLE_HID_REPORT_CONF(
                HID_BASE_INTERFACE_IDX,
                hid_report_type_output,
                false,
                HID_REPORT_ID(AUDIO_OUT),
                HID_REPORT_SIZE(AUDIO_OUT)
        ),
#endif
        [HID_NUMBER_OF_OUT_REPS] = { 0 }
};

// Persistent instance of ble_hid_db
static const ble_hid_db_t m_ble_hid_db = {
        .report_maps_size = ARRAY_SIZE(m_report_maps),
        .reports_in_size = HID_NUMBER_OF_IN_REPS,
        .reports_out_size = HID_NUMBER_OF_OUT_REPS,
        .reports_feature_size = 0,
        .ext_maps_size = 0,
        .report_maps = m_report_maps,
        .reports_in = m_reports_in,
        .reports_out = m_reports_out,
        .reports_feature = NULL,
        .ext_mappings = NULL
};

// ----------------------------------------------------------------------------
// HID report transmission scheduling
// ----------------------------------------------------------------------------

/**@brief Report-freeing function compatible with m_coms_free_func_t. */
static void m_coms_report_free_func(void *p_report)
{
    nrf_balloc_free(&m_coms_report_pool, p_report);
}

/**@brief Create a data descriptor. */
static m_coms_data_desc_t *m_coms_data_desc_create(void)
{
    m_coms_data_desc_t *p_data_desc = nrf_balloc_alloc(&m_coms_data_desc_pool);

    if (p_data_desc != NULL)
    {
        p_data_desc->free_func = NULL;
    }

    return p_data_desc;
}

/**@brief Destroy the data descriptor and free the attached data buffer. */
static void m_coms_data_desc_destroy(m_coms_data_desc_t *p_data_desc)
{
    ASSERT(p_data_desc != NULL);

    if (p_data_desc->free_func != NULL)
    {
        p_data_desc->free_func(p_data_desc->p_free_func_context);
    }

    nrf_balloc_free(&m_coms_data_desc_pool, p_data_desc);
}

/**@brief Expire packets in a given channel. */
static void m_coms_channel_drop(m_coms_channel_t *p_channel)
{
    ASSERT(p_channel != NULL);

    NRF_LOG_WARNING("Packet lost!");

    m_coms_data_desc_destroy(p_channel->p_current_data_desc);
    if (nrf_queue_pop(p_channel->p_backlog, &(p_channel->p_current_data_desc)) != NRF_SUCCESS)
    {
        p_channel->p_current_data_desc = NULL;
    }
}

/**@brief Enqueue data in the channel. */
static ret_code_t m_coms_channel_enqueue(m_coms_channel_t *p_channel,
                                         m_coms_data_desc_t *p_data_desc)
{
    ret_code_t status;

    ASSERT((p_channel != NULL) && (p_data_desc != NULL));

    if (p_channel->p_current_data_desc == NULL)
    {
        p_channel->p_current_data_desc = p_data_desc;
        status = NRF_SUCCESS;
    }
    else
    {
        status = nrf_queue_push(p_channel->p_backlog, &p_data_desc);
        if (status != NRF_SUCCESS)
        {
            m_coms_channel_drop(p_channel);
            status = nrf_queue_push(p_channel->p_backlog, &p_data_desc);
        }
    }

    return status;
}

/**@brief Send one packet from a given channel. */
static ret_code_t m_coms_channel_process(m_coms_channel_t *p_channel, m_coms_data_process_status_t * p_status)
{
    m_coms_data_desc_t *p_data_desc;
    uint16_t            packet_size;
    uint16_t            bytes_sent;
    ret_code_t          err_code;

    ASSERT(p_channel != NULL);

    if (p_channel->p_current_data_desc == NULL)
    {
        *p_status = M_COMS_STATUS_QUEUE_EMPTY;
        return NRF_SUCCESS;
    }

    p_data_desc = p_channel->p_current_data_desc;
    bytes_sent  = 0;

    switch (p_data_desc->type)
    {
        case M_COMS_SERVICE_TYPE_HID:
            packet_size = MIN(p_data_desc->data_size,
                              p_data_desc->service_params.hid.max_packet_size);

            err_code = m_coms_ble_hid_report_send(p_data_desc->p_data,
                                                  packet_size,
                                                  p_data_desc->service_params.hid.interface_idx,
                                                  p_data_desc->service_params.hid.report_idx);
            switch (err_code)
            {
                case NRF_SUCCESS:
                    bytes_sent = packet_size;
                    *p_status  = M_COMS_STATUS_SUCCESS;
                    break;

                case BLE_ERROR_GATTS_SYS_ATTR_MISSING:
                    /* Fall through */
                case NRF_ERROR_INVALID_STATE:
                    /* Fall through */
                case NRF_ERROR_BUSY:
                    /* Fall through */
                case NRF_ERROR_FORBIDDEN:
                    *p_status = M_COMS_STATUS_CANNOT_SEND;
                    return NRF_SUCCESS;

                case NRF_ERROR_RESOURCES:
                    *p_status = M_COMS_STATUS_SD_BUFFER_FULL;
                    return NRF_SUCCESS;

                default:
                    return err_code;

            }
            break;

#if CONFIG_AUDIO_ATVV_ENABLED
        case M_COMS_SERVICE_TYPE_ATVV:
            err_code = m_coms_ble_atvv_audio_send(p_data_desc->p_data, p_data_desc->data_size, &bytes_sent, p_status);
            if (err_code != NRF_SUCCESS)
            {
                return err_code;
            }
            if (*p_status == M_COMS_STATUS_CANNOT_SEND)
            {
                // Cannot send audio data because of wrong state: don't keep it buffered
                p_data_desc->data_size = 0;
                bytes_sent             = 0;
            }
            break;

#endif /* CONFIG_AUDIO_ATVV_ENABLED */

        default:
            // Invalid type
            APP_ERROR_CHECK_BOOL(false);
            break;
    }

    p_data_desc->p_data    += bytes_sent;
    p_data_desc->data_size -= bytes_sent;

    if (p_data_desc->data_size == 0)
    {
        m_coms_data_desc_destroy(p_data_desc);
        if (nrf_queue_pop(p_channel->p_backlog, &(p_channel->p_current_data_desc)) != NRF_SUCCESS)
        {
            p_channel->p_current_data_desc = NULL;
        }
    }

    return NRF_SUCCESS;
}

#if CONFIG_AUDIO_ENABLED && CONFIG_AUDIO_HID_ENABLED
/**@brief Send one packet from an audio stream. */
static ret_code_t m_coms_process_audio_hid(m_coms_data_process_status_t * p_status)
{
    return m_coms_channel_process(&m_coms_audio_hid_channel, p_status);
}
#endif /* CONFIG_AUDIO_ENABLED && CONFIG_AUDIO_HID_ENABLED */

#if CONFIG_AUDIO_ENABLED && CONFIG_AUDIO_ATVV_ENABLED
/**@brief Send one packet from an audio stream. */
static ret_code_t m_coms_process_audio_atvv_data(m_coms_data_process_status_t * p_status)
{
    return m_coms_channel_process(&m_coms_audio_atvv_channel, p_status);
}
/**@brief Send one packet from the ATVV control data stream. */
static ret_code_t m_coms_process_audio_atvv_ctl(m_coms_data_process_status_t * p_status)
{
    ret_code_t err_code;

    err_code = m_coms_ble_atvv_ctl_pkt_queue_process();
    switch (err_code)
    {
        case NRF_SUCCESS:
            *p_status = M_COMS_STATUS_SUCCESS;
            return NRF_SUCCESS;

        case BLE_ERROR_GATTS_SYS_ATTR_MISSING:
            /* Fall through */
        case NRF_ERROR_INVALID_STATE:
            /* Fall through */
        case NRF_ERROR_BUSY:
            /* Fall through */
        case NRF_ERROR_FORBIDDEN:
            *p_status = M_COMS_STATUS_CANNOT_SEND;
            return NRF_SUCCESS;

        case NRF_ERROR_NOT_FOUND:
            *p_status = M_COMS_STATUS_QUEUE_EMPTY;
            return NRF_SUCCESS;

        case NRF_ERROR_RESOURCES:
            *p_status = M_COMS_STATUS_SD_BUFFER_FULL;
            return NRF_SUCCESS;

        default:
            return err_code;
    }
}
#endif /* CONFIG_AUDIO_ENABLED && CONFIG_AUDIO_HID_ENABLED */

/**@brief Send one packet from the keys stream. */
static ret_code_t m_coms_process_keys(m_coms_data_process_status_t * p_status)
{
    return m_coms_channel_process(&m_coms_keys_channel, p_status);
}

/**@brief Send a Mouse X/Y motion packet. */
static ret_code_t m_coms_process_xy_motion(m_coms_data_process_status_t * p_status)
{
    uint8_t report[HID_REPORT_SIZE(MOUSE_XY_IN)];
    ret_code_t err_code;
    int16_t x = 0;
    int16_t y = 0;
    m_protocol_hid_state_item_t *p_x = m_protocol_hid_state_get(HID_USAGE(0x01, 0x30));
    m_protocol_hid_state_item_t *p_y = m_protocol_hid_state_get(HID_USAGE(0x01, 0x31));

    if (p_x)
    {
        x = p_x->value;
    }

    if (p_y)
    {
        y = p_y->value;
    }

    if ((x == 0) && (y == 0))
    {
        // There is nothing to send.
        *p_status = M_COMS_STATUS_QUEUE_EMPTY;
        return NRF_SUCCESS;
    }

    // Make sure that the reported values are within limits defined in the HID report descriptor.
    x = MIN(x,  2047);
    x = MAX(x, -2047);

    y = MIN(y,  2047);
    y = MAX(y, -2047);

    STATIC_ASSERT(sizeof(report) == 3);
    report[0] = x & 0x00FF;
    report[1] = ((y & 0x000F) << 4) | ((x & 0x0F00) >> 8);
    report[2] = (y & 0x0FF0) >> 4;

    err_code = m_coms_ble_hid_report_send(report,
                                        sizeof(report),
                                        HID_BASE_INTERFACE_IDX,
                                        HID_REPORT_IDX(MOUSE_XY_IN));
    switch (err_code)
    {
        case NRF_SUCCESS:
            *p_status = M_COMS_STATUS_SUCCESS;
            break;

        case BLE_ERROR_GATTS_SYS_ATTR_MISSING:
            /* Fall through */
        case NRF_ERROR_INVALID_STATE:
            /* Fall through */
        case NRF_ERROR_BUSY:
            /* Fall through */
        case NRF_ERROR_FORBIDDEN:
            *p_status = M_COMS_STATUS_CANNOT_SEND;
            return NRF_SUCCESS;

        case NRF_ERROR_RESOURCES:
            *p_status = M_COMS_STATUS_SD_BUFFER_FULL;
            return NRF_SUCCESS;

        default:
            return err_code;
    }

    if (p_x)
    {
        p_x->value -= x;
    }

    if (p_y)
    {
        p_y->value -= y;
    }

    return NRF_SUCCESS;
}

/**@brief Send a Mouse Wheel/Pan packet. */
static ret_code_t m_coms_process_wp_motion(m_coms_data_process_status_t * p_status)
{
    uint8_t report[HID_REPORT_SIZE(MOUSE_WP_IN)];
    ret_code_t err_code;
    int16_t w = 0;
    int16_t p = 0;
    m_protocol_hid_state_item_t * p_w;
    m_protocol_hid_state_item_t * p_p;

    p_w = m_protocol_hid_state_get(HID_USAGE(0x01, 0x38));
    p_p = m_protocol_hid_state_get(HID_USAGE(0x0C, 0x238));

    if (p_w)
    {
        w = p_w->value;
    }

    if (p_p)
    {
        p = p_p->value;
    }

    if ((w == 0) && (p == 0))
    {
        // There is nothing to send.
        *p_status = M_COMS_STATUS_QUEUE_EMPTY;
        return NRF_SUCCESS;
    }

    // Make sure that the reported values are within limits defined in the HID report descriptor.
    w = MIN(w,  127);
    w = MAX(w, -127);

    p = MIN(p,  127);
    p = MAX(p, -127);

    STATIC_ASSERT(sizeof(report) == 2);
    report[0] = w;
    report[1] = p;

    err_code = m_coms_ble_hid_report_send(report,
                                        sizeof(report),
                                        HID_BASE_INTERFACE_IDX,
                                        HID_REPORT_IDX(MOUSE_WP_IN));
    switch (err_code)
    {
        case NRF_SUCCESS:
            *p_status = M_COMS_STATUS_SUCCESS;
            break;

        case BLE_ERROR_GATTS_SYS_ATTR_MISSING:
            /* Fall through */
        case NRF_ERROR_INVALID_STATE:
            /* Fall through */
        case NRF_ERROR_BUSY:
            /* Fall through */
        case NRF_ERROR_FORBIDDEN:
            *p_status = M_COMS_STATUS_CANNOT_SEND;
            return NRF_SUCCESS;

        case NRF_ERROR_RESOURCES:
            *p_status = M_COMS_STATUS_SD_BUFFER_FULL;
            return NRF_SUCCESS;

        default:
            return err_code;
    }

    if (p_w)
    {
        p_w->value -= w;
    }

    if (p_p)
    {
        p_p->value -= p;
    }

    return NRF_SUCCESS;
}

/**@brief Process the enqueued data. */
static void m_coms_process_data(void)
{
    ret_code_t (* const stream_processors[])(m_coms_data_process_status_t * p_status) =
    {
#if CONFIG_AUDIO_ENABLED
# if CONFIG_AUDIO_HID_ENABLED
        m_coms_process_audio_hid,
# endif /* CONFIG_AUDIO_HID_ENABLED*/
# if CONFIG_AUDIO_ATVV_ENABLED
        m_coms_process_audio_atvv_data,
        m_coms_process_audio_atvv_ctl,
# endif /* CONFIG_AUDIO_ATVV_ENABLED*/
#endif /* CONFIG_AUDIO_ENABLED */
        m_coms_process_xy_motion,
        m_coms_process_keys,
        m_coms_process_xy_motion,
        m_coms_process_wp_motion,
    };
    unsigned int empty_stream_mask = 0;
    static uint8_t rr;

    // HID data can be sent only over a secure connection.
    if (m_coms_state != M_COMS_STATE_SECURED)
    {
        return;
    }

    // Skip processing if the SoftDevice queue is not becoming empty.
    if (m_coms_packets_in_fly > MAX(CONFIG_GATTS_CONN_HVN_TX_QUEUE_SIZE / 3 , 2))
    {
        return;
    }

    // Try to send data from all streams until all of them are empty.
    while (empty_stream_mask != ((1 << ARRAY_SIZE(stream_processors)) - 1))
    {
        m_coms_data_process_status_t status;
        ret_code_t                   err_code;

        if (++rr >= ARRAY_SIZE(stream_processors))
        {
            rr = 0;
        }

        if (empty_stream_mask & (1 << rr))
        {
            continue;
        }

        err_code = stream_processors[rr](&status);
        APP_ERROR_CHECK(err_code);
        switch (status)
        {
            // Packet sent.
            case M_COMS_STATUS_SUCCESS:
                m_coms_packets_in_fly += 1;
                NRF_LOG_DEBUG("TX%u [+1 = %u]", rr, m_coms_packets_in_fly);

                if (m_coms_max_packets_in_fly < m_coms_packets_in_fly)
                {
                    m_coms_max_packets_in_fly = m_coms_packets_in_fly;
                }

                // Keep handing packets to the SoftDevice until buffers are full
                continue;

            // No more data in the stream: continue to the next stream
            case M_COMS_STATUS_QUEUE_EMPTY:
                empty_stream_mask |= (1 << rr);
                continue;

            // This stream cannot send more data at this time: continue to the next stream
            case M_COMS_STATUS_CANNOT_SEND:
                empty_stream_mask |= (1 << rr);
                continue;

            // The SoftDevice will not accept more data at this time: continue processing later
            case M_COMS_STATUS_SD_BUFFER_FULL:
                rr -= 1;
                break;

            default:
                ASSERT(false);
                break;
        }

        break;
    }
}

// ----------------------------------------------------------------------------
// HID report creation
// ----------------------------------------------------------------------------


/**@brief Create a Keyboard Report from the keyboard state array. */
static size_t m_coms_create_keyboard_report(uint8_t *p_report)
{
    size_t i = 0;

    memset(p_report, 0, HID_REPORT_SIZE(KEYBOARD_IN));
    m_protocol_hid_state_item_t const *p_item = m_protocol_hid_state_page_it_init(0x07);

    do {
        if (!p_item || (HID_USAGE_PAGE(p_item->usage) != 0x07))
        {
            break;
        }

        ASSERT(p_item->value != 0);
        p_report[i] = HID_USAGE_ID(p_item->usage);
        ++i;
        p_item = m_protocol_hid_state_page_it_next(p_item);
    } while (i < HID_REPORT_SIZE(KEYBOARD_IN));

    return HID_REPORT_SIZE(KEYBOARD_IN);
}

/**@brief Create a Consumer Control report from the consumer control state array. */
static size_t m_coms_create_consumer_ctrl_report(uint8_t *p_report)
{
    size_t i = 0;

    memset(p_report, 0, HID_REPORT_SIZE(CONSUMER_CTRL_IN));
    m_protocol_hid_state_item_t const *p_item = m_protocol_hid_state_page_it_init(0x0C);

    do {
        if (!p_item || (HID_USAGE_PAGE(p_item->usage) != 0x0C))
        {
            break;
        }

        /* AC Pan value is stored on the same page. Ignore it as this value is handled
         * by a different function. */
        if (p_item->usage != HID_USAGE(0x0C, 0x238))
        {
            ASSERT(p_item->value != 0);
            if ((i & 1) == 0)
            {
                p_report[3*i/2 + 0]  = ((HID_USAGE_ID(p_item->usage) >> 0) & 0xFF) << 0;
                p_report[3*i/2 + 1]  = ((HID_USAGE_ID(p_item->usage) >> 8) & 0x0F) << 0;
            }
            else
            {
                p_report[3*i/2 + 0] |= ((HID_USAGE_ID(p_item->usage) >> 0) & 0x0F) << 4;
                p_report[3*i/2 + 1]  = ((HID_USAGE_ID(p_item->usage) >> 4) & 0xFF) << 0;
            }
        }

        ++i;
        p_item = m_protocol_hid_state_page_it_next(p_item);
    } while (i < HID_REPORT_SIZE(CONSUMER_CTRL_IN));

    return HID_REPORT_SIZE(CONSUMER_CTRL_IN);
}

/**@brief Create a Mouse Button report from the mouse button state array. */
static size_t m_coms_create_mouse_btn_report(uint8_t *p_report)
{
    size_t i = 0;

    memset(p_report, 0, HID_REPORT_SIZE(MOUSE_BTN_IN));

    m_protocol_hid_state_item_t const *p_item = m_protocol_hid_state_page_it_init(0x09);
    do {
        if (!p_item || (HID_USAGE_PAGE(p_item->usage) != 0x09))
        {
            break;
        }

        size_t key = HID_USAGE_ID(p_item->usage) - 1;

        ASSERT(p_item->value != 0);
        ASSERT(key < 8);
        p_report[key >> 3] |= 1 << (key & 0x07);

        ++i;
        p_item = m_protocol_hid_state_page_it_next(p_item);
    } while (i < HID_REPORT_SIZE(MOUSE_BTN_IN));

    return HID_REPORT_SIZE(MOUSE_BTN_IN);
}

/**@brief Enqueue a Keys report. */
static ret_code_t m_coms_enqueue_report(m_coms_channel_t *p_channel,
                                        uint8_t report_idx,
                                        size_t (*create_report_func)(uint8_t *p_report))
{
    m_coms_data_desc_t *p_data_desc;
    unsigned int report_size;
    uint8_t *p_report_buff;
    ret_code_t status;

    ASSERT(p_channel != NULL);

    // Allocate report buffer.
    p_report_buff = nrf_balloc_alloc(&m_coms_report_pool);
    if (p_report_buff == NULL)
    {
        return NRF_ERROR_NO_MEM;
    }

    // Allocate report descriptor.
    p_data_desc = m_coms_data_desc_create();
    if (p_data_desc == NULL)
    {
        nrf_balloc_free(&m_coms_report_pool, p_report_buff);
        return NRF_ERROR_NO_MEM;
    }

    // Create report.
    report_size = create_report_func(p_report_buff);
    ASSERT(report_size <= HID_MAX_QUEUED_REPORT_SIZE);

    // Fill in report descriptor.
    p_data_desc->type                               = M_COMS_SERVICE_TYPE_HID;
    p_data_desc->p_data                             = p_report_buff;
    p_data_desc->data_size                          = report_size;
    p_data_desc->service_params.hid.max_packet_size = report_size;

    p_data_desc->service_params.hid.interface_idx   = HID_BASE_INTERFACE_IDX;
    p_data_desc->service_params.hid.report_idx      = report_idx;

    p_data_desc->free_func                          = m_coms_report_free_func;
    p_data_desc->p_free_func_context                = p_report_buff;

    // Append report descriptor to the queue.
    status = m_coms_channel_enqueue(p_channel, p_data_desc);
    if (status != NRF_SUCCESS)
    {
        m_coms_data_desc_destroy(p_data_desc);
    }

    return status;
}

// ----------------------------------------------------------------------------
// BLE Interface
// ----------------------------------------------------------------------------

static void m_coms_hid_boot_mode_callback(m_coms_hid_boot_pkt_t *      p_boot_pkt,
                                          m_coms_ble_hid_boot_type_t * p_pkt_type,
                                          const uint8_t *              p_data,
                                          uint8_t                      p_len,
                                          uint8_t                      p_hid_interface,
                                          uint8_t                      p_report_idx)
{
    // Boot mode HID is not supported.
    APP_ERROR_CHECK_BOOL(false);
}

/**@brief Utility function for handling the received data (output reports). */
static void m_coms_handle_rx(m_coms_ble_evt_t *p_evt)
{
    if (p_evt->data.data_received.report_type != BLE_HIDS_REP_TYPE_OUTPUT)
    {
        return;
    }

#if CONFIG_AUDIO_ENABLED && CONFIG_AUDIO_HID_ENABLED
    if ((p_evt->data.data_received.report_idx == HID_REPORT_IDX(AUDIO_OUT)) &&
        (p_evt->data.data_received.len == 1))
    {
        APP_ERROR_CHECK(event_send(EVT_HID_REPORT_OUTPUT,
                                   HID_USAGE(0xFF00, 0x00),
                                   p_evt->data.data_received.data[0],
                                   -1));
    }
#endif
}


/**@brief Utility function for handling connection events.
 */
static void m_coms_ble_evt_handler(void *p_event_data, uint16_t event_size)
{
    m_coms_ble_evt_t *p_evt = p_event_data;
    pm_peer_id_t peer_id = PM_PEER_ID_INVALID;

    ASSERT(event_size == sizeof(m_coms_ble_evt_t));

    switch(p_evt->type)
    {
        case M_COMS_BLE_EVT_CONNECTED:
            NRF_LOG_INFO("M_COMS_BLE_EVT_CONNECTED");

            m_coms_state            = M_COMS_STATE_CONNECTED;
#if CONFIG_AUDIO_ENABLED
            m_coms_effective_mtu    = BLE_GATT_ATT_MTU_DEFAULT;
#endif

            APP_ERROR_CHECK(event_send(EVT_BT_CONN_STATE,
                                       BT_CONN_STATE_CONNECTED,
                                       p_evt->data.conn_data.conn_handle,
                                       p_evt->data.conn_data.peer_id));

            APP_ERROR_CHECK(event_send(EVT_BT_ADV_STATE,
                                       BT_ADV_STATE_IDLE,
                                       p_evt->data.conn_data.conn_handle,
                                       p_evt->data.conn_data.peer_id));
            break;

        case M_COMS_BLE_EVT_DISCONNECTED:
            NRF_LOG_INFO("M_COMS_BLE_EVT_DISCONNECTED");
            m_coms_state = M_COMS_STATE_INITIALIZED;

#if CONFIG_PWR_MGMT_ENABLED
            if (m_coms_going_down)
            {
                // Shutdown in progress - do nothing more.
                break;
            }
#endif /* CONFIG_PWR_MGMT_ENABLED */

            APP_ERROR_CHECK(event_send(EVT_BT_CONN_STATE,
                                       BT_CONN_STATE_DISCONNECTED,
                                       p_evt->data.conn_data.conn_handle,
                                       p_evt->data.conn_data.peer_id));
            break;

        case M_COMS_BLE_EVT_DATA_RECEIVED:
            NRF_LOG_INFO("M_COMS_BLE_EVT_DATA_RECEIVED");
            m_coms_handle_rx(p_evt);
            break;

        case M_COMS_BLE_EVT_READ_REQ:
            NRF_LOG_INFO("M_COMS_BLE_EVT_READ_REQ");
            break;

        case M_COMS_BLE_EVT_CONN_UPDATE:
            NRF_LOG_INFO("M_COMS_BLE_EVT_CONN_UPDATE");
            NRF_LOG_INFO("Max Connection Interval: %d.%02d ms",
                      p_evt->data.conn_update.max_conn_interval * 125 / 100,
                      p_evt->data.conn_update.max_conn_interval * 125 % 100);
            break;

        case M_COMS_BLE_EVT_CONN_SECURED:
            NRF_LOG_INFO("M_COMS_BLE_EVT_CONN_SECURED");
            m_coms_state = M_COMS_STATE_SECURED;

            // Trigger data transmission.
            m_coms_process_data();
            APP_ERROR_CHECK(event_send(EVT_BT_CONN_STATE,
                                       BT_CONN_STATE_SECURED,
                                       p_evt->data.conn_data.conn_handle,
                                       p_evt->data.conn_data.peer_id));
            break;

        case M_COMS_BLE_EVT_ADV:
            NRF_LOG_INFO("M_COMS_BLE_EVT_ADV");
            ASSERT(p_evt->data.adv_data.peer_id != PM_PEER_ID_INVALID);
            APP_ERROR_CHECK(event_send(EVT_BT_ADV_STATE,
                                       BT_ADV_STATE_ACTIVE,
                                       BLE_CONN_HANDLE_INVALID,
                                       p_evt->data.adv_data.peer_id));
            break;

        case M_COMS_BLE_EVT_ADV_BONDABLE:
            NRF_LOG_INFO("M_COMS_BLE_EVT_ADV_BONDABLE");
            ASSERT(p_evt->data.adv_data.peer_id == PM_PEER_ID_INVALID);
            APP_ERROR_CHECK(event_send(EVT_BT_ADV_STATE,
                                       BT_ADV_STATE_ACTIVE_BONDABLE,
                                       BLE_CONN_HANDLE_INVALID,
                                       p_evt->data.adv_data.peer_id));
            break;

        case M_COMS_BLE_EVT_ADV_TIMEOUT:
            NRF_LOG_INFO("M_COMS_BLE_EVT_ADV_TIMEOUT");

#if CONFIG_PWR_MGMT_ENABLED
            if (m_coms_going_down)
            {
                // Shutdown in progress - do nothing more.
                return;
            }
#endif /* CONFIG_PWR_MGMT_ENABLED */
            APP_ERROR_CHECK(event_send(EVT_BT_ADV_STATE,
                                       BT_ADV_STATE_IDLE,
                                       BLE_CONN_HANDLE_INVALID,
                                       PM_PEER_ID_INVALID));
            break;

        case M_COMS_BLE_EVT_ADDR_CHANGED:
            NRF_LOG_INFO("M_COMS_BLE_EVT_ADDR_CHANGED");
            break;

        case M_COMS_BLE_EVT_PASSKEY_REQ:
            NRF_LOG_INFO("M_COMS_BLE_EVT_PASSKEY_REQ");
            break;

        case M_COMS_BLE_EVT_OOBKEY_REQ:
            NRF_LOG_INFO("M_COMS_BLE_EVT_OOBKEY_REQ");
#if (CONFIG_NFC_ENABLED && CONFIG_NFC_PAIRING_TAG && CONFIG_SEC_LEGACY_PAIRING)
            {
                ble_advdata_tk_value_t tk;
                uint8_t *p_tk = NULL;

                // Respond with TK presented in NFC Tag if it is avaiable.
                if (m_nfc_get_tk(&tk) == NRF_SUCCESS)
                {
                    p_tk = tk.tk;
                }

                APP_ERROR_CHECK(m_coms_ble_oobkey_set(p_tk));
            }
#else
            APP_ERROR_CHECK(m_coms_ble_oobkey_set(NULL));
#endif
            break;

        case M_COMS_BLE_EVT_ALERT:
            NRF_LOG_INFO("M_COMS_BLE_EVT_ALERT");
            NRF_LOG_INFO("Alert Level: %d", p_evt->data.alert.level);

            pm_peer_id_get(p_evt->data.alert.conn_handle, &peer_id);
            APP_ERROR_CHECK(event_send(EVT_BT_ALERT_LEVEL,
                                       p_evt->data.alert.level,
                                       p_evt->data.alert.conn_handle,
                                       peer_id));
            break;

        case M_COMS_BLE_EVT_SHUTDOWN:
            NRF_LOG_INFO("M_COMS_BLE_EVT_SHUTDOWN");
            m_coms_state = M_COMS_STATE_SHUTDOWN;

#if CONFIG_PWR_MGMT_ENABLED
            if (m_coms_going_down)
            {
                nrf_pwr_mgmt_shutdown(NRF_PWR_MGMT_SHUTDOWN_CONTINUE);
            }
#endif /* CONFIG_PWR_MGMT_ENABLED */
            break;

        case M_COMS_BLE_EVT_TX_COMPLETE:
            NRF_LOG_DEBUG("M_COMS_BLE_EVT_TX_COMPLETE");

            // Update packet counter.
            m_coms_packets_in_fly -= MIN(m_coms_packets_in_fly, p_evt->data.tx_complete.count);
            NRF_LOG_DEBUG("ACK [-%u = %u]", p_evt->data.tx_complete.count, m_coms_packets_in_fly);
            m_coms_process_data();
            break;

        case M_COMS_BLE_EVT_MTU_CHANGED:
            NRF_LOG_INFO("M_COMS_BLE_EVT_MTU_CHANGED");
#if CONFIG_AUDIO_ENABLED
            m_coms_effective_mtu = p_evt->data.mtu_changed.effective_mtu;
            NRF_LOG_INFO("Effective MTU: %u bytes", m_coms_effective_mtu);
#endif
            break;

        default:
            APP_ERROR_CHECK_BOOL(false);
            break;
    }
}

// ----------------------------------------------------------------------------
// Communication Module API
// ----------------------------------------------------------------------------

ret_code_t m_coms_init(bool delete_bonds)
{
    ret_code_t status;

    m_coms_state                = M_COMS_STATE_SHUTDOWN;
    m_coms_packets_in_fly       = 0;
    m_coms_max_packets_in_fly   = 0;

#if CONFIG_PWR_MGMT_ENABLED
    m_coms_going_down           = false;
#endif

#if CONFIG_AUDIO_ENABLED
    m_coms_audio_srv_bitmsk     = 0;
#if CONFIG_AUDIO_HID_ENABLED
    memset(&m_coms_audio_hid_channel, 0, sizeof(m_coms_audio_hid_channel));
    m_coms_audio_hid_channel.p_backlog = &m_coms_audio_hid_channel_backlog;
#endif /* CONFIG_AUDIO_HID_ENABLED */
#if CONFIG_AUDIO_ATVV_ENABLED
    memset(&m_coms_audio_atvv_channel, 0, sizeof(m_coms_audio_atvv_channel));
    m_coms_audio_atvv_channel.p_backlog = &m_coms_audio_atvv_channel_backlog;
#endif /* CONFIG_AUDIO_ATVV_ENABLED */
#endif /* CONFIG_AUDIO_ENABLED*/

    memset(&m_coms_keys_channel, 0, sizeof(m_coms_keys_channel));
    m_coms_keys_channel.p_backlog = &m_coms_keys_channel_backlog;

    status = nrf_balloc_init(&m_coms_report_pool);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    status = nrf_balloc_init(&m_coms_data_desc_pool);
    if (status != NRF_SUCCESS)
    {
        return status;
    }

    status = m_coms_ble_init(&m_ble_params, &m_ble_hid_db, delete_bonds);
    if (status == NRF_SUCCESS)
    {
        m_coms_state = M_COMS_STATE_INITIALIZED;
    }

    return status;
}

/**@brief Send HID Report. */
static ret_code_t m_coms_send_hid_report(uint32_t usage, int16_t report)
{
    ret_code_t status = NRF_SUCCESS;

    switch (HID_USAGE_PAGE(usage))
    {
        case 0x01: // Mouse
            ASSERT(HID_USAGE_ID(usage) != 0);

            switch (HID_USAGE_ID(usage))
            {
                case 0x30: // X Axis
                case 0x31: // Y Axis
                case 0x38: // Wheel
                    break;

                default:
                    return NRF_ERROR_NOT_SUPPORTED;
            }
            break;

        case 0x07: // Keyboard
            status = m_coms_enqueue_report(&m_coms_keys_channel,
                                           HID_REPORT_IDX(KEYBOARD_IN),
                                           m_coms_create_keyboard_report);
            break;

        case 0x09: // Button
            if (HID_USAGE_ID(usage) > MOUSE_BTN_IN_REP_COUNT)
            {
                return NRF_ERROR_NOT_SUPPORTED;
            }

            status = m_coms_enqueue_report(&m_coms_keys_channel,
                                           HID_REPORT_IDX(MOUSE_BTN_IN),
                                           m_coms_create_mouse_btn_report);
            break;

        case 0x0C: // Consumer Control
            if (usage == HID_USAGE(0x0C, 0x238))
            {
                /* AC Pan event should not trigger consumer control report creation. */
                break;
            }

            status = m_coms_enqueue_report(&m_coms_keys_channel,
                                           HID_REPORT_IDX(CONSUMER_CTRL_IN),
                                           m_coms_create_consumer_ctrl_report);
            break;

        default:
            return NRF_ERROR_NOT_SUPPORTED;
    }

    // Trigger data processing.
    if (status == NRF_SUCCESS)
    {
        m_coms_process_data();
    }

    return status;
}

bool m_coms_event_handler(const event_t *p_event)
{
    ret_code_t status = NRF_SUCCESS;

    ASSERT(p_event != NULL);

    switch (p_event->type)
    {
        case EVT_HID_REPORT_INPUT:
            status = m_coms_send_hid_report(p_event->hid.usage, p_event->hid.report);
            break;

#if CONFIG_BATT_MEAS_ENABLED
        case EVT_SYSTEM_BATTERY_LEVEL:
            m_coms_ble_battery_level_update(p_event->system.data);
            break;
#endif

        default:
            /* Ignore */
            break;
    }

    APP_ERROR_CHECK(status);

    return false;
}

#if CONFIG_AUDIO_ENABLED
void m_coms_audio_service_enable(m_coms_audio_service_t service)
{
    ASSERT((m_coms_audio_srv_bitmsk & service) == 0);
    m_coms_audio_srv_bitmsk |= service;
}

void m_coms_audio_service_disable(m_coms_audio_service_t service)
{
    ASSERT((m_coms_audio_srv_bitmsk & service) != 0);
    m_coms_audio_srv_bitmsk &= ~service;
}

/**@brief Audio frame freeing function compatible with m_coms_free_func_t. */
static void m_coms_audio_frame_free_func(void *p_frame)
{
    m_audio_frame_put(p_frame);
}

ret_code_t m_coms_send_audio(m_audio_frame_t *p_audio_frame)
{
    bool trigger_processing;
    ret_code_t status;

    trigger_processing = false;
    status             = NRF_ERROR_INVALID_STATE;

#if CONFIG_AUDIO_HID_ENABLED
    if (m_coms_audio_srv_bitmsk & M_COMS_AUDIO_SERVICE_HID)
    {
        m_coms_data_desc_t *p_data_desc;

        // Allocate report descriptor.
        p_data_desc = m_coms_data_desc_create();
        if (p_data_desc == NULL)
        {
            return NRF_ERROR_NO_MEM;
        }

        // Fill in report descriptor.
        p_data_desc->type                               = M_COMS_SERVICE_TYPE_HID;
        p_data_desc->p_data                             = p_audio_frame->data;
        p_data_desc->data_size                          = p_audio_frame->data_size;

        p_data_desc->service_params.hid.max_packet_size = MIN(HID_REPORT_SIZE(AUDIO_IN), (m_coms_effective_mtu - 3));
        p_data_desc->service_params.hid.interface_idx   = HID_BASE_INTERFACE_IDX;
        p_data_desc->service_params.hid.report_idx      = HID_REPORT_IDX(AUDIO_IN);

        p_data_desc->free_func                          = m_coms_audio_frame_free_func;
        p_data_desc->p_free_func_context                = m_audio_frame_get(p_audio_frame);

        // Append descriptor to the channel queue.
        status = m_coms_channel_enqueue(&m_coms_audio_hid_channel, p_data_desc);
        if (status != NRF_SUCCESS)
        {
            m_coms_data_desc_destroy(p_data_desc);
        }
        else
        {
            trigger_processing = true;
        }
    }
#endif /* CONFIG_AUDIO_HID_ENABLED */

#if CONFIG_AUDIO_ATVV_ENABLED
    if (m_coms_audio_srv_bitmsk & M_COMS_AUDIO_SERVICE_ATVV)
    {
        m_coms_data_desc_t *p_data_desc;

        // Allocate report descriptor.
        p_data_desc = m_coms_data_desc_create();
        if (p_data_desc == NULL)
        {
            return NRF_ERROR_NO_MEM;
        }

        // Fill in report descriptor.
        p_data_desc->type                 = M_COMS_SERVICE_TYPE_ATVV;
        p_data_desc->p_data               = p_audio_frame->data;
        p_data_desc->data_size            = p_audio_frame->data_size;

        p_data_desc->free_func            = m_coms_audio_frame_free_func;
        p_data_desc->p_free_func_context  = m_audio_frame_get(p_audio_frame);

        // Append descriptor to the channel queue.
        status = m_coms_channel_enqueue(&m_coms_audio_atvv_channel, p_data_desc);
        if (status != NRF_SUCCESS)
        {
            m_coms_data_desc_destroy(p_data_desc);
        }
        else
        {
            trigger_processing = true;
        }
    }
#endif /* CONFIG_AUDIO_ATVV_ENABLED */

    if (trigger_processing)
    {
        // Trigger data processing.
        m_coms_process_data();
    }

    return status;
}
#endif /* CONFIG_AUDIO_ENABLED */

#if CONFIG_PWR_MGMT_ENABLED
static bool m_coms_shutdown(nrf_pwr_mgmt_evt_t event)
{
    if (m_coms_state == M_COMS_STATE_SHUTDOWN)
    {
        // The module is ready for shutdown.
        return true;
    }

    if (!m_coms_going_down)
    {
        m_coms_going_down = true;
        APP_ERROR_CHECK(m_coms_ble_shutdown());
    }

    return false;
}

static bool m_coms_log_statistics(nrf_pwr_mgmt_evt_t event)
{
    NRF_LOG_INFO("Maximum HID Report Pool usage: %d entries",
              nrf_balloc_max_utilization_get(&m_coms_report_pool));

    NRF_LOG_INFO("Maximum HID Report Descriptor Pool usage: %d entries",
              nrf_balloc_max_utilization_get(&m_coms_data_desc_pool));

#if CONFIG_AUDIO_ENABLED && CONFIG_AUDIO_HID_ENABLED
    NRF_LOG_INFO("Maximum HID Audio Packets queue usage: %d entries",
              nrf_queue_max_utilization_get(m_coms_audio_hid_channel.p_backlog));
#endif /* CONFIG_AUDIO_ENABLED && CONFIG_AUDIO_HID_ENABLED*/

#if CONFIG_AUDIO_ENABLED && CONFIG_AUDIO_ATVV_ENABLED
    NRF_LOG_INFO("Maximum ATVV Audio Packets queue usage: %d entries",
              nrf_queue_max_utilization_get(m_coms_audio_atvv_channel.p_backlog));
#endif /* CONFIG_AUDIO_ENABLED && CONFIG_AUDIO_ATVV_ENABLED */

    NRF_LOG_INFO("Maximum Keys Packets queue usage: %d entries",
              nrf_queue_max_utilization_get(m_coms_keys_channel.p_backlog));

    NRF_LOG_INFO("Maximum SoftDevice queue usage: %d entries",
              m_coms_max_packets_in_fly);

    return true;
}

NRF_PWR_MGMT_HANDLER_REGISTER(m_coms_shutdown, SHUTDOWN_PRIORITY_DEFAULT);
NRF_PWR_MGMT_HANDLER_REGISTER(m_coms_log_statistics, SHUTDOWN_PRIORITY_STATISTICS);
#endif /* CONFIG_PWR_MGMT_ENABLED */

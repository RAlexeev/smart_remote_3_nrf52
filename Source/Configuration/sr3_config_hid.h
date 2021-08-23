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

#ifndef _SR3_CONFIG_HID_H
#define _SR3_CONFIG_HID_H

#if (!defined(__ASSEMBLER__) && !defined(__IAR_SYSTEMS_ASM__))
#include "app_util.h"

#define KEYBOARD_KEY_ID(_row, _column)  ((0x00 << 8) | (((_row) & 0x0F) << 4) | ((_column) & 0x0F))
#define MOUSE_KEY_ID(_button)           ((0x01 << 8) | ((_button) & 0xFF))
#define KEY_ID_INVALID                  0xFFFF

#define HID_USAGE(_page, _id)           (((unsigned long)((_page) & 0xFFFF) << 16) | ((_id) & 0xFFFF))
#define HID_USAGE_ID(_usage)            (((_usage) >>  0) & 0xFFFF)
#define HID_USAGE_PAGE(_usage)          (((_usage) >> 16) & 0xFFFF)

#if defined(CONFIG_BOARD_NRF52832_PCA20023) || \
    defined(CONFIG_BOARD_NRF52832_PCA63519) || \
    defined(CONFIG_BOARD_NRF52810_PCA20031)

/*
 * Names of the keys to simplify keymap creation.
 * Keep names ordered by the Key ID. The keymap tables must have the same order.
 */
#define KEY_MEDIA       KEYBOARD_KEY_ID(0, 0)
#define KEY_0           KEYBOARD_KEY_ID(0, 1)
#define KEY_BACK        KEYBOARD_KEY_ID(0, 2)
#define KEY_7           KEYBOARD_KEY_ID(0, 3)
#define KEY_8           KEYBOARD_KEY_ID(0, 4)
#define KEY_9           KEYBOARD_KEY_ID(0, 5)
#define KEY_4           KEYBOARD_KEY_ID(0, 6)
#define KEY_5           KEYBOARD_KEY_ID(0, 7)
#define KEY_6           KEYBOARD_KEY_ID(1, 0)
#define KEY_1           KEYBOARD_KEY_ID(1, 1)
#define KEY_2           KEYBOARD_KEY_ID(1, 2)
#define KEY_3           KEYBOARD_KEY_ID(1, 3)
#define KEY_AC_SEARCH   KEYBOARD_KEY_ID(2, 0)
#define KEY_VOL_DOWN    KEYBOARD_KEY_ID(2, 2)
#define KEY_CH_DOWN     KEYBOARD_KEY_ID(2, 4)
#define KEY_VOL_UP      KEYBOARD_KEY_ID(2, 5)
#define KEY_CH_UP       KEYBOARD_KEY_ID(2, 7)
#define KEY_DOWN        KEYBOARD_KEY_ID(3, 1)
#define KEY_LEFT        KEYBOARD_KEY_ID(3, 3)
#define KEY_OK          KEYBOARD_KEY_ID(3, 4)
#define KEY_RIGHT       KEYBOARD_KEY_ID(3, 5)
#define KEY_UP          KEYBOARD_KEY_ID(4, 0)
#define KEY_PLAY_PAUSE  KEYBOARD_KEY_ID(4, 2)
#define KEY_PREV_TRACK  KEYBOARD_KEY_ID(4, 3)
#define KEY_NEXT_TRACK  KEYBOARD_KEY_ID(4, 4)
#define KEY_STOP        KEYBOARD_KEY_ID(4, 5)
#define KEY_MUTE        KEYBOARD_KEY_ID(4, 6)
#define KEY_POWER       KEYBOARD_KEY_ID(4, 7)

#elif defined(CONFIG_BOARD_NRF52832_CUSTOM) || \
      defined(CONFIG_BOARD_NRF52810_CUSTOM)

/*
 * Names of the keys to simplify keymap creation.
 * Keep names ordered by the Key ID. The keymap tables must have the same order.
 */
#define KEY_POWER       KEYBOARD_KEY_ID(0, 0)
#define KEY_MIC         KEYBOARD_KEY_ID(0, 1)
#define KEY_UP          KEYBOARD_KEY_ID(0, 2)
#define KEY_LEFT        KEYBOARD_KEY_ID(0, 3)
#define KEY_OK          KEYBOARD_KEY_ID(1, 0)
#define KEY_RIGHT       KEYBOARD_KEY_ID(1, 1)
#define KEY_DOWN        KEYBOARD_KEY_ID(1, 2)
#define KEY_BACK        KEYBOARD_KEY_ID(1, 3)
#define KEY_HOME        KEYBOARD_KEY_ID(2, 0)
#define KEY_ALL_APPS    KEYBOARD_KEY_ID(2, 1)
#define KEY_MUTE        KEYBOARD_KEY_ID(2, 2)
#define KEY_PLAY_PAUSE  KEYBOARD_KEY_ID(2, 3)
#define KEY_VOL_UP      KEYBOARD_KEY_ID(3, 0)
#define KEY_REW         KEYBOARD_KEY_ID(3, 1)
#define KEY_FF          KEYBOARD_KEY_ID(3, 2)
#define KEY_VOL_DOWN    KEYBOARD_KEY_ID(3, 3)
#endif

/**@brief HID keyboard map entry. */
typedef struct
{
    uint16_t        key_id;                                 /**< Key ID. */
    uint32_t        hid_usage;                              /**< HID usage code. */
} sr3_hid_keymap_t;

extern const sr3_hid_keymap_t   g_sr3_hid_keymap[];         /**< HID keyboard map. */
extern const size_t             g_sr3_hid_keymap_size;      /**< HID keyboard map size. */

#endif /* !defined(__ASSEMBLER__) && !defined(__IAR_SYSTEMS_ASM__) */
#endif /* _SR3_CONFIG_HID_H */

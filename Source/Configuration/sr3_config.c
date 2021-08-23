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

#include "sr3_config.h"

#if defined(CONFIG_BOARD_NRF52832_PCA20023) || \
    defined(CONFIG_BOARD_NRF52832_PCA63519) || \
    defined(CONFIG_BOARD_NRF52810_PCA20031)

/*
 * HID keymap. The Consumer Control keys are defined in section 15 of the HID Usage Tables document under the following URL:
 * http://www.usb.org/developers/hidpage/Hut1_12v2.pdf
 */
const sr3_hid_keymap_t g_sr3_hid_keymap[] =
{
    { KEY_MEDIA,        HID_USAGE(0x0C, 0x193) },   /* Consumer Control: AL A/V Capture/Playback */
    { KEY_0,            HID_USAGE(0x07, 0x27) },    /* Keyboard '0' and ')' */
    { KEY_BACK,         HID_USAGE(0x07, 0x2A) },    /* Keyboard Backspace */
    { KEY_7,            HID_USAGE(0x07, 0x24) },    /* Keyboard '7' and '&' */
    { KEY_8,            HID_USAGE(0x07, 0x25) },    /* Keyboard '8' and '*' */
    { KEY_9,            HID_USAGE(0x07, 0x26) },    /* Keyboard '9' and '(' */
    { KEY_4,            HID_USAGE(0x07, 0x21) },    /* Keyboard '4' and '$' */
    { KEY_5,            HID_USAGE(0x07, 0x22) },    /* Keyboard '5' and '%' */
    { KEY_6,            HID_USAGE(0x07, 0x23) },    /* Keyboard '6' and '^' */
    { KEY_1,            HID_USAGE(0x07, 0x1E) },    /* Keyboard '1' and '!' */
    { KEY_2,            HID_USAGE(0x07, 0x1F) },    /* Keyboard '2' and '@' */
    { KEY_3,            HID_USAGE(0x07, 0x20) },    /* Keyboard '3' and '#' */
    { KEY_AC_SEARCH,    HID_USAGE(0x0C, 0x221) },   /* Consumer Control: AC Search */
    { KEY_VOL_DOWN,     HID_USAGE(0x0C, 0xEA) },    /* Consumer Control: Volume Decrement */
    { KEY_CH_DOWN,      HID_USAGE(0x07, 0x4E) },    /* Keyboard Page Down */
    { KEY_VOL_UP,       HID_USAGE(0x0C, 0xE9) },    /* Consumer Control: Volume Increment */
    { KEY_CH_UP,        HID_USAGE(0x07, 0x4B) },    /* Keyboard Page Up */
    { KEY_DOWN,         HID_USAGE(0x07, 0x51) },    /* Keyboard Down Arrow */
    { KEY_LEFT,         HID_USAGE(0x07, 0x50) },    /* Keyboard Left Arrow */
    { KEY_OK,           HID_USAGE(0x07, 0x28) },    /* Keyboard Enter */
    { KEY_RIGHT,        HID_USAGE(0x07, 0x4F) },    /* Keyboard Right Arrow */
    { KEY_UP,           HID_USAGE(0x07, 0x52) },    /* Keyboard Up Arrow */
    { KEY_PLAY_PAUSE,   HID_USAGE(0x0C, 0xCD) },    /* Consumer Control: Play/Pause */
    { KEY_PREV_TRACK,   HID_USAGE(0x0C, 0xB6) },    /* Consumer Control: Scan Prev Track */
    { KEY_NEXT_TRACK,   HID_USAGE(0x0C, 0xB5) },    /* Consumer Control: Scan Next Track */
    { KEY_STOP,         HID_USAGE(0x0C, 0xB7) },    /* Consumer Control: Stop */
    { KEY_MUTE,         HID_USAGE(0x07, 0x7F) },    /* Keyboard Mute */

    { MOUSE_KEY_ID(0),  HID_USAGE(0x09, 0x01) },    /* Left Mouse Button */
    { MOUSE_KEY_ID(1),  HID_USAGE(0x09, 0x02) },    /* Right Mouse Button */
};

const size_t g_sr3_hid_keymap_size = sizeof(g_sr3_hid_keymap) / sizeof(g_sr3_hid_keymap[0]);

/*
 * IR keymap (SIRC).
 */
const sr3_ir_keymap_t g_sr3_ir_keymap[] =
{
    { KEY_0,            SR3_IR_SYMBOL( 9, 1) },    /* SIRC 0 */
    { KEY_7,            SR3_IR_SYMBOL( 6, 1) },    /* SIRC 7 */
    { KEY_8,            SR3_IR_SYMBOL( 7, 1) },    /* SIRC 8 */
    { KEY_9,            SR3_IR_SYMBOL( 8, 1) },    /* SIRC 9 */
    { KEY_4,            SR3_IR_SYMBOL( 3, 1) },    /* SIRC 4 */
    { KEY_5,            SR3_IR_SYMBOL( 4, 1) },    /* SIRC 5 */
    { KEY_6,            SR3_IR_SYMBOL( 5, 1) },    /* SIRC 6 */
    { KEY_1,            SR3_IR_SYMBOL( 0, 1) },    /* SIRC 1 */
    { KEY_2,            SR3_IR_SYMBOL( 1, 1) },    /* SIRC 2 */
    { KEY_3,            SR3_IR_SYMBOL( 2, 1) },    /* SIRC 3 */
    { KEY_VOL_DOWN,     SR3_IR_SYMBOL(19, 1) },    /* SIRC Volume Down */
    { KEY_CH_DOWN,      SR3_IR_SYMBOL(17, 1) },    /* SIRC Channel Down */
    { KEY_VOL_UP,       SR3_IR_SYMBOL(18, 1) },    /* SIRC Volume Up */
    { KEY_CH_UP,        SR3_IR_SYMBOL(16, 1) },    /* SIRC Channel Up */
    { KEY_OK,           SR3_IR_SYMBOL(11, 1) },    /* SIRC Enter */
    { KEY_MUTE,         SR3_IR_SYMBOL(20, 1) },    /* SIRC Mute */
    { KEY_POWER,        SR3_IR_SYMBOL(21, 1) },    /* SIRC Power */
};

const size_t g_sr3_ir_keymap_size = sizeof(g_sr3_ir_keymap) / sizeof(g_sr3_ir_keymap[0]);

/*
 * Buzzer sound
 */
const sr3_buzzer_sound_t g_sr3_buzzer_sound_mild[] =
{
    {  20, 180 },
    {  20, 180 },
    {  20, 180 },
    {   0, 400 },
    {   0,   0 },
};

const sr3_buzzer_sound_t g_sr3_buzzer_sound_high[] =
{
    { 750, 250 },
    { 250, 250 },
    {   0, 750 },
    { 750, 250 },
    { 750, 250 },
    { 750, 250 },
    {   0, 750 },
    { 250, 250 },
    { 750, 250 },
    { 250, 250 },
    {   0, 750 },
    { 750, 250 },
    { 250, 250 },
    { 250, 250 },
    {   0, 750 },
    { 250, 250 },
    { 250, 250 },
    {   0, 750 },
    { 750, 250 },
    { 250, 250 },
    { 750, 250 },
    { 250, 250 },
    {   0, 750 },
    {   0,   0 },
};
#elif defined(CONFIG_BOARD_NRF52832_CUSTOM) || \
      defined(CONFIG_BOARD_NRF52810_CUSTOM)
/*
 * HID keymap. The Consumer Control keys are defined following Android keys, HID Consumer Page (0x0c) 
 * https://source.android.com/devices/input/keyboard-devices
 */
const sr3_hid_keymap_t g_sr3_hid_keymap[] =
{
    { KEY_POWER,        HID_USAGE(0x0C, 0x30) },    /* Consumer Control: Power */
    { KEY_MIC,          HID_USAGE(0x0C, 0x221) },   /* Consumer Control: AC Search */
    { KEY_UP,           HID_USAGE(0x0C, 0x42) },    /* Consumer Control: Menu Up */
    { KEY_LEFT,         HID_USAGE(0x0C, 0x44) },    /* Consumer Control: Menu Left */
    { KEY_OK,           HID_USAGE(0x0C, 0x41) },    /* Consumer Control: Menu Pick */
    { KEY_RIGHT,        HID_USAGE(0x0C, 0x45) },    /* Consumer Control: Menu Right */
    { KEY_DOWN,         HID_USAGE(0x0C, 0x43) },    /* Consumer Control: Menu Down */
    { KEY_BACK,         HID_USAGE(0x0C, 0x224) },   /* Consumer Control: AC Back */
    { KEY_HOME,         HID_USAGE(0x0C, 0x223) },   /* Consumer Control: AC Home */
    { KEY_ALL_APPS,     HID_USAGE(0x0C, 0x1A2) },   /* Consumer Control: All Apps */
    { KEY_MUTE,         HID_USAGE(0x0C, 0xE2) },    /* Consumer Control: Mute */
    { KEY_PLAY_PAUSE,   HID_USAGE(0x0C, 0xCD) },    /* Consumer Control: Play/Pause */
    { KEY_VOL_UP,       HID_USAGE(0x0C, 0xE9) },    /* Consumer Control: Volume Increment */
    { KEY_REW,          HID_USAGE(0x0C, 0xB4) },    /* Consumer Control: Rewind */
    { KEY_FF,           HID_USAGE(0x0C, 0xB3) },    /* Consumer Control: Fast Forward */
    { KEY_VOL_DOWN,     HID_USAGE(0x0C, 0xEA) },    /* Consumer Control: Volume Decrement */
};

const size_t g_sr3_hid_keymap_size = sizeof(g_sr3_hid_keymap) / sizeof(g_sr3_hid_keymap[0]);

/*
 * IR keymap (SIRC).
 */
const sr3_ir_keymap_t g_sr3_ir_keymap[] =
{
    { KEY_POWER, SR3_IR_SYMBOL(21, 1) }, /* SIRC Power */
};

const size_t g_sr3_ir_keymap_size = sizeof(g_sr3_ir_keymap) / sizeof(g_sr3_ir_keymap[0]);

/*
 * Buzzer sound
 */
const sr3_buzzer_sound_t g_sr3_buzzer_sound_mild[] =
{
    {  20, 180 },
    {  20, 180 },
    {  20, 180 },
    {   0, 400 },
    {   0,   0 },
};

const sr3_buzzer_sound_t g_sr3_buzzer_sound_high[] =
{
    { 750, 250 },
    { 250, 250 },
    {   0, 750 },
    { 750, 250 },
    { 750, 250 },
    { 750, 250 },
    {   0, 750 },
    { 250, 250 },
    { 750, 250 },
    { 250, 250 },
    {   0, 750 },
    { 750, 250 },
    { 250, 250 },
    { 250, 250 },
    {   0, 750 },
    { 250, 250 },
    { 250, 250 },
    {   0, 750 },
    { 750, 250 },
    { 250, 250 },
    { 750, 250 },
    { 250, 250 },
    {   0, 750 },
    {   0,   0 },
};
#else
# error "CONFIG_BOARD_xxx is not defined!"
#endif

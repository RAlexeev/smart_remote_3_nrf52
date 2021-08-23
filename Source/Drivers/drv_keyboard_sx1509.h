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

/**
 * @brief Keyboard driver using SX1509
 *
 * @details This header defines the prototypes for keyboard matrix driver functions.
 */

#ifndef __SX1509_H__
#define __SX1509_H__

#define RegInputDisableB    0x00
#define RegInputDisableA    0x01
#define RegLongSlewB        0x02
#define RegLongSlewA        0x03
#define RegLowDriveB        0x04
#define RegLowDriveA        0x05
#define RegPullUpB          0x06
#define RegPullUpA          0x07
#define RegPullDownB        0x08
#define RegPullDownA        0x09
#define RegOpenDrainB       0x0A
#define RegOpenDrainA       0x0B
#define RegPolarityB        0x0C
#define RegPolarityA        0x0D
#define RegDirB             0x0E
#define RegDirA             0x0F
#define RegDataB            0x10
#define RegDataA            0x11
#define RegInterruptMaskB   0x12
#define RegInterruptMaskA   0x13
#define RegSenseHighB       0x14
#define RegSenseLowB        0x15
#define RegSenseHighA       0x16
#define RegSenseLowA        0x17
#define RegInterruptSourceB 0x18
#define RegInterruptSourceA 0x19
#define RegEventStatusB     0x1A
#define RegEventStatusA     0x1B
#define RegLevelShifter1    0x1C
#define RegLevelShifter2    0x1D
#define RegClock            0x1E
#define RegMisc             0x1F
#define RegLEDDriverEnableB 0x20
#define RegLEDDriverEnableA 0x21

#define RegDebounceConfig   0x22
#define RegDebounceEnableB  0x23
#define RegDebounceEnableA  0x24
#define RegKeyConfig1       0x25
#define RegKeyConfig2       0x26
#define RegKeyData1         0x27
#define RegKeyData2         0x28

#define RegHighInputB       0x69
#define RegHighInputA       0x6A
#define RegReset            0x7D

#endif

/** @} */

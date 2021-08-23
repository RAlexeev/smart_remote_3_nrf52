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

/** @file LIS3DH accelerometer definitions.
 *
 * @defgroup DRIVERS_ACC_DEF LIS3DH definitions
 * @{
 * @ingroup MOD_ACC
 * @brief Definitions of LIS3DH constants.
 *
 * @details
 */
#ifndef __DRV_ACC_LIS3DH_H__
#define __DRV_ACC_LIS3DH_H__

//-----------------------------------------------------------------------------
// Enumerations
//-----------------------------------------------------------------------------
/**
  Output Data Rates (ODR)
*/
typedef enum
{
  ODR_1Hz                 = 0x01,
  ODR_10Hz                = 0x02,
  ODR_25Hz                = 0x03,
  ODR_50Hz                = 0x04,
  ODR_100Hz               = 0x05,
  ODR_200Hz               = 0x06,
  ODR_400Hz               = 0x07,
  ODR_1620Hz_LP           = 0x08,
  ODR_1344Hz_NP_5367HZ_LP = 0x09
} ODR_t;

/**
  Operation modes
*/
typedef enum
{
  POWER_DOWN              = 0x00,
  LOW_POWER               = 0x01,
  NORMAL                  = 0x02
} Mode_t;

/**
  Acceleration ranges
*/
typedef enum
{
  FULLSCALE_2             = 0x00,
  FULLSCALE_4             = 0x01,
  FULLSCALE_8             = 0x02,
  FULLSCALE_16            = 0x03
} Fullscale_t;

/**
  Axis selection
*/
typedef enum
{
  X_ENABLE                = 0x01,
  Y_ENABLE                = 0x02,
  Z_ENABLE                = 0x04,
} AXISenable_t;

/**
  Interrupt modes
*/
typedef enum
{
  IntOR                   = 0x00,    // Sources are ORed.
  Int6DMov                = 0x01,    // 6-direction movement interrupt.
  IntAND                  = 0x02,    // Sources are ANDed.
  Int6DPos                = 0x03,    // 6-direction position change interrupt.
  Int4DMov                = 0x05,    // 4-direction  movement interrupt.
  Int4DPos                = 0x07,    // 4-direction  position interrupt.
} IntMode_t;

/**
  Interrupt pin polarity
*/
typedef enum
{
  IntActiveHigh           = 0x00,
  IntActiveLow            = 0x02
} IntPolarity_t;

/**
  Click interrupt modes
*/
typedef enum
{
  ClickSingle             = 0x00,    // Single-click interrupt.
  ClickDouble             = 0x01,    // Double-click interrupt.
} ClickMode_t;

/**
  High-pass filter modes
*/
typedef enum
{
  HPNormalRes             = 0x00,    // Normal mode, reference register reset when read.
  HPReference             = 0x01,    // Reference mode.
  HPNormal                = 0x02,    // Normal mode.
  HPAutoResInt            = 0x03     // Autoreset upon interrupt.
} HPFilterMode_t;

//-----------------------------------------------------------------------------
// Structs
//-----------------------------------------------------------------------------/
/**
  Acceleration data struct
*/
typedef struct
{
  int16_t AXIS_X;
  int16_t AXIS_Y;
  int16_t AXIS_Z;
} AccAxesRaw_t;

//-----------------------------------------------------------------------------
// SPI register map
//-----------------------------------------------------------------------------
// Device identification register
#define WHO_AM_I          0x0F
#define INT_COUNTER_REG   0x0E
#define INT1_SOURCE       0x31

// Control register 1
#define CTRL_REG1         0x20
#define ODR_BIT           BIT_4
#define LPEN              BIT_3
#define ZEN               BIT_2
#define YEN               BIT_1
#define XEN               BIT_0

// Control register 2
#define CTRL_REG2         0x21
#define HPM               BIT_6
#define HPCF              BIT_4
#define FDS               BIT_3
#define HPCLICK           BIT_2
#define HPIS2             BIT_1
#define HPIS1             BIT_0

// High-pass filter bitmask
#define HP_INT1           BIT_0
#define HP_INT2           BIT_1
#define HP_CLICK          BIT_2
#define HP_DATA           BIT_3

// Control register 3
#define CTRL_REG3         0x22
#define I1_CLICK          BIT_7
#define I1_AOI1           BIT_6
#define I1_AOI2           BIT_5
#define I1_DRDY1          BIT_4
#define I1_DRDY2          BIT_3
#define I1_WTM            BIT_2
#define I1_ORUN           BIT_1

// Control register 6
#define CTRL_REG6         0x25
#define I2_CLICK          BIT_7
#define I2_INT1           BIT_6
#define I2_BOOT           BIT_4
#define H_LACTIVE         BIT_1

// Temperature configuration register
#define TEMP_CFG_REG      0x1F
#define ADC_PD            BIT_7
#define TEMP_EN           BIT_6

// Control register 4
#define CTRL_REG4         0x23
#define BDU               BIT_7
#define BLE               BIT_6
#define FS1               BIT_5
#define FS0               BIT_4
#define HR                BIT_3
#define ST                BIT_1
#define SIM               BIT_0

// Control register 5
#define CTRL_REG5         0x24
#define BOOT              BIT_7
#define FIFO_EN           BIT_6
#define LIR_INT1          BIT_3
#define D4D_INT1          BIT_2

// Reference/datacapture register
#define REFERENCE_REG     0x26
#define REF               BIT_0

// Axis status register
#define STATUS_REG        0x27
#define ZYXOR             BIT_7
#define ZOR               BIT_6
#define YOR               BIT_5
#define XOR               BIT_4
#define ZYXDA             BIT_3
#define ZDA               BIT_2
#define YDA               BIT_1
#define XDA               BIT_0

// Aux status register
#define STATUS_AUX        0x07

// Interrupt 1 configuration
#define INT1_CFG          0x30
#define ANDOR             BIT_7
#define INT_6D            BIT_6
#define ZHIE              BIT_5
#define ZLIE              BIT_4
#define YHIE              BIT_3
#define YLIE              BIT_2
#define XHIE              BIT_1
#define XLIE              BIT_0

// FIFO control register
#define FIFO_CTRL_REG     0x2E
#define FM                BIT_6
#define TR                BIT_5
#define FTH               BIT_0

// Control register 3 bitmask
#define CLICK_ON_PIN_INT1_ENABLE                BIT_7
#define I1_INT1_ON_PIN_INT1_ENABLE              BIT_6
#define I1_INT2_ON_PIN_INT1_ENABLE              BIT_5
#define I1_DRDY1_ON_INT1_ENABLE                 BIT_4
#define I1_DRDY2_ON_INT1_ENABLE                 BIT_3
#define WTM_ON_INT1_ENABLE                      BIT_2
#define INT1_OVERRUN_ENABLE                     BIT_1

// Control register 6 bitmask
#define CLICK_ON_PIN_INT2_ENABLE                BIT_7
#define I2_INT1_ON_PIN_INT2_ENABLE              BIT_6
#define I2_INT2_ON_PIN_INT2_ENABLE              BIT_5
#define I2_BOOT_ON_INT2_ENABLE                  BIT_4
#define INT_ACTIVE_LOW                          BIT_1

// INT1_CFG bitmask
#define INT1_AND                                BIT_7
#define INT1_D_ENABLE                           BIT_6
#define INT1_ZHIE_ENABLE                        BIT_5
#define INT1_ZLIE_ENABLE                        BIT_4
#define INT1_YHIE_ENABLE                        BIT_3
#define INT1_YLIE_ENABLE                        BIT_2
#define INT1_XHIE_ENABLE                        BIT_1
#define INT1_XLIE_ENABLE                        BIT_0

// INT1_SRC bitmask
#define INT1_SRC_IA                             BIT_6
#define INT1_SRC_ZH                             BIT_5
#define INT1_SRC_ZL                             BIT_4
#define INT1_SRC_YH                             BIT_3
#define INT1_SRC_YL                             BIT_2
#define INT1_SRC_XH                             BIT_1
#define INT1_SRC_XL                             BIT_0

// INT1 registers
#define INT1_THS                                0x32
#define INT1_DURATION                           0x33

// Interrupt 1 source register
#define INT1_SRC        0x31

// FIFO Source Register bitmask
#define FIFO_SRC_WTM                            BIT_7
#define FIFO_SRC_OVRUN                          BIT_6
#define FIFO_SRC_EMPTY                          BIT_5

// Click interrupt register
#define CLICK_CFG       0x38
// Click interrupt configuration bitmask
#define ZD_ENABLE                               BIT_5
#define ZS_ENABLE                               BIT_4
#define YD_ENABLE                               BIT_3
#define YS_ENABLE                               BIT_2
#define XD_ENABLE                               BIT_1
#define XS_ENABLE                               BIT_0

// Click interrupt source register
#define CLICK_SRC                               0x39
// Click interrupt source register bitmask
#define IA                                      BIT_6
#define DCLICK                                  BIT_5
#define SCLICK                                  BIT_4
#define CLICK_SIGN                              BIT_3
#define CLICK_Z                                 BIT_2
#define CLICK_Y                                 BIT_1
#define CLICK_X                                 BIT_0

// Click-click register
#define CLICK_THS                               0x3A
#define TIME_LIMIT                              0x3B
#define TIME_LATENCY                            0x3C
#define TIME_WINDOW                             0x3D

// Output register
#define OUT_X_L         0x28
#define OUT_X_H         0x29
#define OUT_Y_L         0x2A
#define OUT_Y_H         0x2B
#define OUT_Z_L         0x2C
#define OUT_Z_H         0x2D

// Aux register
#define OUT_1_L         0x08
#define OUT_1_H         0x09
#define OUT_2_L         0x0A
#define OUT_2_H         0x0B
#define OUT_3_L         0x0C
#define OUT_3_H         0x0D

// Status register bitmask
#define STATUS_REG_ZYXOR                        BIT_7    // 1  : New data set has overwritten the previous one.
                                                         // 0  : No overrun has occurred (default).
#define STATUS_REG_ZOR                          BIT_6    // 0  : No overrun has occurred (default).
                                                         // 1  : New Z-axis data has overwritten the previous one.
#define STATUS_REG_YOR                          BIT_5    // 0  : No overrun has occurred (default).
                                                         // 1  : New Y-axis data has overwritten the previous one.
#define STATUS_REG_XOR                          BIT_4    // 0  : No overrun has occurred (default).
                                                         // 1  : New X-axis data has overwritten the previous one.
#define STATUS_REG_ZYXDA                        BIT_3    // 0  : A new set of data is not yet available.
                                                         // 1  : A new set of data is available.
#define STATUS_REG_ZDA                          BIT_2    // 0  : A new data for the Z-Axis is not available.
                                                         // 1  : A new data for the Z-Axis is available.
#define STATUS_REG_YDA                          BIT_1    // 0  : A new data for the Y-Axis is not available.
                                                         // 1  : A new data for the Y-Axis is available.
#define STATUS_REG_XDA                          BIT_0    // 0  : A new data for the X-Axis is not available.

#define DATAREADY_BIT                           STATUS_REG_ZYXDA


// Aux status register bitmask
#define STATUS_AUX_321OR                        BIT_7
#define STATUS_AUX_3OR                          BIT_6
#define STATUS_AUX_2OR                          BIT_5
#define STATUS_AUX_1OR                          BIT_4
#define STATUS_AUX_321DA                        BIT_3
#define STATUS_AUX_3DA                          BIT_2
#define STATUS_AUX_2DA                          BIT_1
#define STATUS_AUX_1DA                          BIT_0

#define I_AM_LIS3DH     0x33

// FIFO registers
#define FIFO_CTRL_REG   0x2E
#define FIFO_SRC_REG    0x2F

#endif /* __DRV_ACC_LIS3DH_TYPES_H__ */
/** @} */

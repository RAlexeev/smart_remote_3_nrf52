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

/** @file BMA222E accelerometer definitions.
 *
 * @defgroup DRIVERS_ACC_DEF BMA222E definitions
 * @{
 * @ingroup MOD_ACC
 * @brief Definitions of BMA222E constants.
 *
 * @details
 */
#ifndef __DRV_ACC_BMA222E_H__
#define __DRV_ACC_BMA222E_H__

//-----------------------------------------------------------------------------
// Register map
//-----------------------------------------------------------------------------
#define BMA222E_BGW_CHIPID    (0x00)
#define BMA222E_PMU_RANGE     (0x0F)
#define BMA222E_PMU_BW        (0x10)
#define BMA222E_PMU_LPW       (0x11)
#define BMA222E_PMU_LOW_POWER (0x12)
#define BMA222E_ACCD_HBW      (0x13)
#define BMA222E_BGW_SOFTRESET (0x14)
#define BMA222E_INT_EN_0      (0x16)
#define BMA222E_INT_EN_1      (0x17)
#define BMA222E_INT_EN_2      (0x18)
#define BMA222E_INT_MAP_0     (0x19)
#define BMA222E_INT_MAP_1     (0x1A)
#define BMA222E_INT_MAP_2     (0x1B)
#define BMA222E_INT_SRC       (0x1E)
#define BMA222E_INT_OUT_CTRL  (0x20)
#define BMA222E_INT_RST_LATCH (0x21)
#define BMA222E_INT_0         (0x22)
#define BMA222E_INT_1         (0x23)
#define BMA222E_INT_2         (0x24)
#define BMA222E_INT_3         (0x25)
#define BMA222E_INT_4         (0x26)
#define BMA222E_INT_5         (0x27)
#define BMA222E_INT_6         (0x28)
#define BMA222E_INT_7         (0x29)
#define BMA222E_INT_8         (0x2A)
#define BMA222E_INT_9         (0x2B)
#define BMA222E_INT_A         (0x2C)
#define BMA222E_INT_B         (0x2D)
#define BMA222E_INT_C         (0x2E)
#define BMA222E_INT_D         (0x2F)
#define BMA222E_FIFO_CONFIG_0 (0x30)

#define BMA222E_I_AM_BMA222E  (0xF8)

/****************************/
/**\name SLEEP DURATION */
/****************************/
#define BMA2x2_SLEEP_DURN_POS               (1)
#define BMA2x2_SLEEP_DURN_LEN               (4)
#define BMA2x2_SLEEP_DURN_MSK               (0x1E)
#define BMA2x2_SLEEP_DURN_REG               BMA222E_PMU_LPW
/****************************/
/**\name POWER MODEPOWER MODE */
/****************************/
#define BMA2x2_MODE_CTRL_POS                (5)
#define BMA2x2_MODE_CTRL_LEN                (3)
#define BMA2x2_MODE_CTRL_MSK                (0xE0)
#define BMA2x2_MODE_CTRL_REG                BMA222E_PMU_LPW

/***************************************************/
/* SOFT RESET VALUE                                */
/***************************************************/
#define BMA2x2_ENABLE_SOFT_RESET_VALUE      (0xB6)

/**********************************************/
/* INTERRUPT ENABLE OF SLOPE-XYZ              */
/**********************************************/
#define BMA2x2_ENABLE_SLOPE_X_INTR_POS      (0)
#define BMA2x2_ENABLE_SLOPE_X_INTR_LEN      (1)
#define BMA2x2_ENABLE_SLOPE_X_INTR_MSK      (0x01)
#define BMA2x2_ENABLE_SLOPE_X_INTR_REG      BMA222E_INT_EN_0

#define BMA2x2_ENABLE_SLOPE_Y_INTR_POS      (1)
#define BMA2x2_ENABLE_SLOPE_Y_INTR_LEN      (1)
#define BMA2x2_ENABLE_SLOPE_Y_INTR_MSK      (0x02)
#define BMA2x2_ENABLE_SLOPE_Y_INTR_REG      BMA222E_INT_EN_0

#define BMA2x2_ENABLE_SLOPE_Z_INTR_POS      (2)
#define BMA2x2_ENABLE_SLOPE_Z_INTR_LEN      (1)
#define BMA2x2_ENABLE_SLOPE_Z_INTR_MSK      (0x04)
#define BMA2x2_ENABLE_SLOPE_Z_INTR_REG      BMA222E_INT_EN_0

/**********************************************/
/**\name INTERRUPT ENABLE OF DOUBLE TAP   */
/**********************************************/
#define BMA2x2_ENABLE_DOUBLE_TAP_INTR_POS      (4)
#define BMA2x2_ENABLE_DOUBLE_TAP_INTR_LEN      (1)
#define BMA2x2_ENABLE_DOUBLE_TAP_INTR_MSK      (0x10)
#define BMA2x2_ENABLE_DOUBLE_TAP_INTR_REG      BMA222E_INT_EN_0
/**********************************************/
/**\name INTERRUPT ENABLE OF SINGLE TAP   */
/**********************************************/
#define BMA2x2_ENABLE_SINGLE_TAP_INTR_POS      (5)
#define BMA2x2_ENABLE_SINGLE_TAP_INTR_LEN      (1)
#define BMA2x2_ENABLE_SINGLE_TAP_INTR_MSK      (0x20)
#define BMA2x2_ENABLE_SINGLE_TAP_INTR_REG      BMA222E_INT_EN_0

/**********************************************/
/* INTERRUPT1 ENABLE OF PAD LOW_G             */
/**********************************************/
#define BMA2x2_ENABLE_INTR1_PAD_LOW_G_POS      (0)
#define BMA2x2_ENABLE_INTR1_PAD_LOW_G_LEN      (1)
#define BMA2x2_ENABLE_INTR1_PAD_LOW_G_MSK      (0x01)
#define BMA2x2_ENABLE_INTR1_PAD_LOW_G_REG      BMA222E_INT_MAP_0

/**********************************************/
/* INTERRUPT1 ENABLE OF PAD HIGH_G            */
/**********************************************/
#define BMA2x2_ENABLE_INTR1_PAD_HIGH_G_POS     (1)
#define BMA2x2_ENABLE_INTR1_PAD_HIGH_G_LEN     (1)
#define BMA2x2_ENABLE_INTR1_PAD_HIGH_G_MSK     (0x02)
#define BMA2x2_ENABLE_INTR1_PAD_HIGH_G_REG     BMA222E_INT_MAP_0
/**********************************************/
/* INTERRUPT1 ENABLE OF PAD SLOPE             */
/**********************************************/
#define BMA2x2_ENABLE_INTR1_PAD_SLOPE_POS      (2)
#define BMA2x2_ENABLE_INTR1_PAD_SLOPE_LEN      (1)
#define BMA2x2_ENABLE_INTR1_PAD_SLOPE_MSK      (0x04)
#define BMA2x2_ENABLE_INTR1_PAD_SLOPE_REG      BMA222E_INT_MAP_0
/**********************************************/
/* INTERRUPT1 ENABLE OF SLOW NO MOTION        */
/**********************************************/
#define BMA2x2_ENABLE_INTR1_PAD_SLOW_NO_MOTION_POS  (3)
#define BMA2x2_ENABLE_INTR1_PAD_SLOW_NO_MOTION_LEN  (1)
#define BMA2x2_ENABLE_INTR1_PAD_SLOW_NO_MOTION_MSK  (0x08)
#define BMA2x2_ENABLE_INTR1_PAD_SLOW_NO_MOTION_REG  BMA222E_INT_MAP_0
/**********************************************/
/* INTERRUPT1 ENABLE OF PAD DOUBLE_TAP        */
/**********************************************/
#define BMA2x2_ENABLE_INTR1_PAD_DOUBLE_TAP_POS      (4)
#define BMA2x2_ENABLE_INTR1_PAD_DOUBLE_TAP_LEN      (1)
#define BMA2x2_ENABLE_INTR1_PAD_DOUBLE_TAP_MSK      (0x10)
#define BMA2x2_ENABLE_INTR1_PAD_DOUBLE_TAP_REG      BMA222E_INT_MAP_0
/**********************************************/
/* INTERRUPT1 ENABLE OF PAD SINGLE_TAP        */
/**********************************************/
#define BMA2x2_ENABLE_INTR1_PAD_SINGLE_TAP_POS      (5)
#define BMA2x2_ENABLE_INTR1_PAD_SINGLE_TAP_LEN      (1)
#define BMA2x2_ENABLE_INTR1_PAD_SINGLE_TAP_MSK      (0x20)
#define BMA2x2_ENABLE_INTR1_PAD_SINGLE_TAP_REG      BMA222E_INT_MAP_0
/**********************************************/
/* INTERRUPT1 ENABLE OF PAD ORIENT            */
/**********************************************/
#define BMA2x2_ENABLE_INTR1_PAD_ORIENT_POS          (6)
#define BMA2x2_ENABLE_INTR1_PAD_ORIENT_LEN          (1)
#define BMA2x2_ENABLE_INTR1_PAD_ORIENT_MSK          (0x40)
#define BMA2x2_ENABLE_INTR1_PAD_ORIENT_REG          BMA222E_INT_MAP_0
/**********************************************/
/* INTERRUPT1 ENABLE OF PAD FLAT              */
/**********************************************/
#define BMA2x2_ENABLE_INTR1_PAD_FLAT_POS            (7)
#define BMA2x2_ENABLE_INTR1_PAD_FLAT_LEN            (1)
#define BMA2x2_ENABLE_INTR1_PAD_FLAT_MSK            (0x80)
#define BMA2x2_ENABLE_INTR1_PAD_FLAT_REG            BMA222E_INT_MAP_0

/**********************************************/
/* INTERRUPT SOURCE SELECTION OF LOW_G        */
/**********************************************/
#define BMA2x2_UNFILT_INTR_SOURCE_LOW_G_POS         (0)
#define BMA2x2_UNFILT_INTR_SOURCE_LOW_G_LEN         (1)
#define BMA2x2_UNFILT_INTR_SOURCE_LOW_G_MSK         (0x01)
#define BMA2x2_UNFILT_INTR_SOURCE_LOW_G_REG         BMA222E_INT_SRC
/**********************************************/
/* INTERRUPT SOURCE SELECTION OF HIGH_G       */
/**********************************************/
#define BMA2x2_UNFILT_INTR_SOURCE_HIGH_G_POS        (1)
#define BMA2x2_UNFILT_INTR_SOURCE_HIGH_G_LEN        (1)
#define BMA2x2_UNFILT_INTR_SOURCE_HIGH_G_MSK        (0x02)
#define BMA2x2_UNFILT_INTR_SOURCE_HIGH_G_REG        BMA222E_INT_SRC
/**********************************************/
/* INTERRUPT SOURCE SELECTION OF SLOPE        */
/**********************************************/
#define BMA2x2_UNFILT_INTR_SOURCE_SLOPE_POS         (2)
#define BMA2x2_UNFILT_INTR_SOURCE_SLOPE_LEN         (1)
#define BMA2x2_UNFILT_INTR_SOURCE_SLOPE_MSK         (0x04)
#define BMA2x2_UNFILT_INTR_SOURCE_SLOPE_REG         BMA222E_INT_SRC
/************************************************/
/* INTERRUPT SOURCE SELECTION OF SLOW NO MOTION */
/************************************************/
#define BMA2x2_UNFILT_INTR_SOURCE_SLOW_NO_MOTION_POS    (3)
#define BMA2x2_UNFILT_INTR_SOURCE_SLOW_NO_MOTION_LEN    (1)
#define BMA2x2_UNFILT_INTR_SOURCE_SLOW_NO_MOTION_MSK    (0x08)
#define BMA2x2_UNFILT_INTR_SOURCE_SLOW_NO_MOTION_REG    BMA222E_INT_SRC
/**********************************************/
/* INTERRUPT SOURCE SELECTION OF TAP          */
/**********************************************/
#define BMA2x2_UNFILT_INTR_SOURCE_TAP_POS           (4)
#define BMA2x2_UNFILT_INTR_SOURCE_TAP_LEN           (1)
#define BMA2x2_UNFILT_INTR_SOURCE_TAP_MSK           (0x10)
#define BMA2x2_UNFILT_INTR_SOURCE_TAP_REG           BMA222E_INT_SRC
/**********************************************/
/* INTERRUPT SOURCE SELECTION OF DATA         */
/**********************************************/
#define BMA2x2_UNFILT_INTR_SOURCE_DATA_POS          (5)
#define BMA2x2_UNFILT_INTR_SOURCE_DATA_LEN          (1)
#define BMA2x2_UNFILT_INTR_SOURCE_DATA_MSK          (0x20)
#define BMA2x2_UNFILT_INTR_SOURCE_DATA_REG          BMA222E_INT_SRC

/****************************************************/
/* INTERRUPT PAD ACTIVE LEVEL AND OUTPUT TYPE       */
/****************************************************/
#define BMA2x2_INTR1_PAD_ACTIVE_LEVEL_POS           (0)
#define BMA2x2_INTR1_PAD_ACTIVE_LEVEL_LEN           (1)
#define BMA2x2_INTR1_PAD_ACTIVE_LEVEL_MSK           (0x01)
#define BMA2x2_INTR1_PAD_ACTIVE_LEVEL_REG           BMA222E_INT_OUT_CTRL

#define BMA2x2_INTR2_PAD_ACTIVE_LEVEL_POS           (2)
#define BMA2x2_INTR2_PAD_ACTIVE_LEVEL_LEN           (1)
#define BMA2x2_INTR2_PAD_ACTIVE_LEVEL_MSK           (0x04)
#define BMA2x2_INTR2_PAD_ACTIVE_LEVEL_REG           BMA222E_INT_OUT_CTRL

#define BMA2x2_INTR1_PAD_OUTPUT_TYPE_POS            (1)
#define BMA2x2_INTR1_PAD_OUTPUT_TYPE_LEN            (1)
#define BMA2x2_INTR1_PAD_OUTPUT_TYPE_MSK            (0x02)
#define BMA2x2_INTR1_PAD_OUTPUT_TYPE_REG            BMA222E_INT_OUT_CTRL

#define BMA2x2_INTR2_PAD_OUTPUT_TYPE_POS            (3)
#define BMA2x2_INTR2_PAD_OUTPUT_TYPE_LEN            (1)
#define BMA2x2_INTR2_PAD_OUTPUT_TYPE_MSK            (0x08)
#define BMA2x2_INTR2_PAD_OUTPUT_TYPE_REG            BMA222E_INT_OUT_CTRL

/****************************************************/
/* LATCH INTERRUPT                                  */
/****************************************************/
#define BMA2x2_LATCH_INTR_POS                       (0)
#define BMA2x2_LATCH_INTR_LEN                       (4)
#define BMA2x2_LATCH_INTR_MSK                       (0x0F)
#define BMA2x2_LATCH_INTR_REG                       BMA222E_INT_RST_LATCH
/****************************************************/
/* RESET LATCH INTERRUPT                            */
/****************************************************/
#define BMA2x2_RESET_INTR_POS                       (7)
#define BMA2x2_RESET_INTR_LEN                       (1)
#define BMA2x2_RESET_INTR_MSK                       (0x80)
#define BMA2x2_RESET_INTR_REG                       BMA222E_INT_RST_LATCH

/****************************************************/
/**\name   TAP DURATION */
/****************************************************/
#define BMA2x2_TAP_DURN_POS                         (0)
#define BMA2x2_TAP_DURN_LEN                         (3)
#define BMA2x2_TAP_DURN_MSK                         (0x07)
#define BMA2x2_TAP_DURN_REG                         BMA222E_INT_8

/****************************************************/
/**\name   TAP SHOCK DURATION */
/****************************************************/
#define BMA2x2_TAP_SHOCK_DURN_POS                   (6)
#define BMA2x2_TAP_SHOCK_DURN_LEN                   (1)
#define BMA2x2_TAP_SHOCK_DURN_MSK                   (0x40)
#define BMA2x2_TAP_SHOCK_DURN_REG                   BMA222E_INT_8

/* This advance tap interrupt only uses for the chip id 0xFB */
#define BMA2x2_ADV_TAP_INTR_POS                     (5)
#define BMA2x2_ADV_TAP_INTR_LEN                     (1)
#define BMA2x2_ADV_TAP_INTR_MSK                     (0x20)
#define BMA2x2_ADV_TAP_INTR_REG                     BMA222E_INT_8

/****************************************************/
/**\name   TAP QUIET DURATION */
/****************************************************/
#define BMA2x2_TAP_QUIET_DURN_POS                   (7)
#define BMA2x2_TAP_QUIET_DURN_LEN                   (1)
#define BMA2x2_TAP_QUIET_DURN_MSK                   (0x80)
#define BMA2x2_TAP_QUIET_DURN_REG                   BMA222E_INT_8

/****************************************************/
/**\name   TAP THRESHOLD */
/****************************************************/
#define BMA2x2_TAP_THRES_POS                        (0)
#define BMA2x2_TAP_THRES_LEN                        (5)
#define BMA2x2_TAP_THRES_MSK                        (0x1F)
#define BMA2x2_TAP_THRES_REG                        BMA222E_INT_9

/****************************************************/
/**\name   TAP SAMPLES */
/****************************************************/
#define BMA2x2_TAP_SAMPLES_POS                      (6)
#define BMA2x2_TAP_SAMPLES_LEN                      (2)
#define BMA2x2_TAP_SAMPLES_MSK                      (0xC0)
#define BMA2x2_TAP_SAMPLES_REG                      BMA222E_INT_9

/****************************/
/**\name BANDWIDTH */
/****************************/
#define BMA2x2_BW_POS                               (0)
#define BMA2x2_BW_LEN                               (5)
#define BMA2x2_BW_MSK                               (0x1F)
#define BMA2x2_BW_REG                               BMA222E_PMU_BW

#endif /* __DRV_ACC_BMA222E_TYPES_H__ */
/** @} */

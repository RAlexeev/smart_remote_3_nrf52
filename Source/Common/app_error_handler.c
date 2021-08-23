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

#include <stdlib.h>
#include <string.h>

#include "nrf.h"
#include "nrf_assert.h"
#include "nrf_delay.h"
#include "nrf_log_ctrl.h"
#include "nrf_sdm.h"
#include "nrf_strerror.h"
#include "app_debug.h"
#include "app_error.h"
#include "hardfault.h"

#ifdef __STARTUP_CONFIG
#include "startup_config.h"
#endif

#define NRF_LOG_MODULE_NAME app_error
#define NRF_LOG_LEVEL NRF_LOG_DEFAULT_LEVEL
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

#ifdef __GNUC__
// Fix linking weak symbols when LTO is enabled.
void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info) __attribute__((__used__));
void HardFault_process(HardFault_stack_t *p_stack) __attribute__((__used__));
#endif

void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info)
{
    __disable_irq();

#if NRF_LOG_ENABLED
    assert_info_t   *p_assert_info = (assert_info_t *)(info);
    error_info_t    *p_error_info  = (error_info_t *)(info);
    uint8_t const   *unknown       = (uint8_t const *)("UNKNOWN");

    // Print information about error.
    switch (id)
    {
        case NRF_FAULT_ID_SD_ASSERT:
            NRF_LOG_ERROR("SOFTDEVICE: ASSERTION FAILED");
            break;

        case NRF_FAULT_ID_APP_MEMACC:
            NRF_LOG_ERROR("SOFTDEVICE: INVALID MEMORY ACCESS");
            break;

        case NRF_FAULT_ID_SDK_ASSERT:
            NRF_LOG_ERROR("ASSERTION FAILED at %s:%u",
                               (p_assert_info->p_file_name) ? p_assert_info->p_file_name : unknown,
                               p_assert_info->line_num);
            break;

        case NRF_FAULT_ID_SDK_ERROR:
            NRF_LOG_ERROR("ERROR %u [%s] at %s:%u",
                               p_error_info->err_code,
                               nrf_strerror_get(p_error_info->err_code),
                               (p_error_info->p_file_name) ? p_error_info->p_file_name : unknown,
                               p_error_info->line_num);
            break;

        default:
            NRF_LOG_ERROR("UNKNOWN FAULT at 0x%08X", pc);
            break;
    }

    // Flush deffered logs.
    NRF_LOG_FINAL_FLUSH();
#endif /* NRF_LOG_ENABLED */

#if CONFIG_DEBUG_PIN_ENABLED
    switch (id)
    {
        case NRF_FAULT_ID_SDK_ASSERT:
            debug_pin_error_handler(0, ((assert_info_t *)(info))->line_num,
                                    ((assert_info_t *)(info))->p_file_name);
            break;

        case NRF_FAULT_ID_SDK_ERROR:
            debug_pin_error_handler(((error_info_t *)(info))->err_code,
                                   ((error_info_t *)(info))->line_num,
                                   ((error_info_t *)(info))->p_file_name);
            break;

        default:
            break;
    }
#endif /* CONFIG_DEBUG_PIN_ENABLED */

#if CONFIG_DEBUG_ENABLED
    for (;;)
    {
        if (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk)
        {
            __BKPT(0);
        }
    }
#else /* !CONFIG_DEBUG_ENABLED */
# if CONFIG_LOG_RTT_ENABLED
    // If RTT is used, we have to give host some time for log readout.
    if (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk)
    {
        nrf_delay_ms(100);
    }
# endif /* CONFIG_LOG_RTT_ENABLED */
    NVIC_SystemReset();
#endif /* CONFIG_DEBUG_ENABLED */
}

void HardFault_process(HardFault_stack_t *p_stack)
{
    __disable_irq();

#if NRF_LOG_ENABLED
#define CFSR_MMARVALID (1 << (0 + 7))
#define CFSR_BFARVALID (1 << (8 + 7))

    static const char *cfsr_msgs[] = {
        [0]  = "The processor has attempted to execute an undefined instruction",
        [1]  = "The processor attempted a load or store at a location that does not permit the operation",
        [2]  = NULL,
        [3]  = "Unstack for an exception return has caused one or more access violations",
        [4]  = "Stacking for an exception entry has caused one or more access violations",
        [5]  = "A MemManage fault occurred during floating-point lazy state preservation",
        [6]  = NULL,
        [7]  = NULL,
        [8]  = "Instruction bus error",
        [9]  = "Data bus error (PC value stacked for the exception return points to the instruction that caused the fault)",
        [10] = "Data bus error (return address in the stack frame is not related to the instruction that caused the error)",
        [11] = "Unstack for an exception return has caused one or more BusFaults",
        [12] = "Stacking for an exception entry has caused one or more BusFaults",
        [13] = "A bus fault occurred during floating-point lazy state preservation",
        [14] = NULL,
        [15] = NULL,
        [16] = "The processor has attempted to execute an undefined instruction",
        [17] = "The processor has attempted to execute an instruction that makes illegal use of the EPSR",
        [18] = "The processor has attempted an illegal load of EXC_RETURN to the PC, as a result of an invalid context, or an invalid EXC_RETURN value",
        [19] = "The processor has attempted to access a coprocessor",
        [20] = NULL,
        [21] = NULL,
        [22] = NULL,
        [23] = NULL,
        [24] = "The processor has made an unaligned memory access",
        [25] = "The processor has executed an SDIV or UDIV instruction with a divisor of 0",
    };

    uint32_t cfsr = SCB->CFSR;
    unsigned int i;

    // Print information about error.
    NRF_LOG_ERROR("HARD FAULT at 0x%08X", p_stack->pc);
    NRF_LOG_ERROR("  R0:  0x%08X  R1:  0x%08X  R2:  0x%08X  R3:  0x%08X",
                         p_stack->r0, p_stack->r1, p_stack->r2, p_stack->r3);
    NRF_LOG_ERROR("  R12: 0x%08X  LR:  0x%08X  PSR: 0x%08X",
                         p_stack->r12, p_stack->lr, p_stack->psr);

    if (SCB->HFSR & SCB_HFSR_VECTTBL_Msk)
    {
        NRF_LOG_ERROR("Cause: BusFault on a vector table read during exception processing.");
    }

    for (i = 0; i < sizeof(cfsr_msgs) / sizeof(cfsr_msgs[0]); i++)
    {
        if (((cfsr & (1 << i)) != 0) && (cfsr_msgs[i] != NULL))
        {
            NRF_LOG_ERROR("Cause: %s.", cfsr_msgs[i]);
        }
    }

    if (cfsr & CFSR_MMARVALID)
    {
        NRF_LOG_ERROR("MemManage Fault Address: 0x%08X", SCB->MMFAR);
    }

    if (cfsr & CFSR_BFARVALID)
    {
        NRF_LOG_ERROR("Bus Fault Address: 0x%08X", SCB->BFAR);
    }

    // Process all deferred log items
    NRF_LOG_FINAL_FLUSH();
#endif /* NRF_LOG_ENABLED */

#if CONFIG_DEBUG_ENABLED
    for (;;)
    {
        if (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk)
        {
            __BKPT(0);
        }
    }

#else /* !CONFIG_DEBUG_ENABLED */
# if CONFIG_LOG_RTT_ENABLED
    // If RTT is used, we have to give host some time for log readout.
    if (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk)
    {
        nrf_delay_ms(100);
    }
# endif /* CONFIG_LOG_RTT_ENABLED */
    NVIC_SystemReset();
#endif /* CONFIG_DEBUG_ENABLED */
}

#if (defined(__HEAP_SIZE) && (__HEAP_SIZE == 0)) || \
    (defined(__STARTUP_CONFIG) && (__STARTUP_CONFIG_HEAP_SIZE == 0))
/**@brief Fake malloc() to catch heap access.
*/
void *malloc(size_t size)
{
    APP_ERROR_CHECK(NRF_ERROR_NO_MEM);
    return NULL;
}

/**@brief Fake calloc() to catch heap access.
*/
void *calloc(size_t num, size_t len)
{
    APP_ERROR_CHECK(NRF_ERROR_NO_MEM);
    return NULL;
}

/**@brief Fake realloc() to catch heap access.
*/
void *realloc(void *mem, size_t size)
{
    APP_ERROR_CHECK(NRF_ERROR_NO_MEM);
    return NULL;
}

/**@brief Fake free() to catch heap access.
*/
void free(void *mem)
{
    APP_ERROR_CHECK(NRF_ERROR_NO_MEM);
}
#endif

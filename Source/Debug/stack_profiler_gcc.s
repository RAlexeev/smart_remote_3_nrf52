/**
 * Copyright (c) 2015 - 2017, Nordic Semiconductor ASA
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

    .syntax unified
    .arch armv6-m

    .equ MAGIC_WORD, 0xDEADDEAD
    .extern __StackLimit
    .extern __StackTop

    .text
    .thumb
    .thumb_func
    .align 1
    .globl  stack_profiler_init
    .type   stack_profiler_init, %function


stack_profiler_init:
    .fnstart

    LDR     R0, =__StackLimit
    LDR     R1, =MAGIC_WORD
    MOV     R2, SP

stack_profiler_init_loop:
    STMIA   R0!, {R1}
    CMP     R0, R2
    BNE     stack_profiler_init_loop

    BX      LR

    .pool
    .cantunwind
    .fnend
    .size stack_profiler_init, .-stack_profiler_init

    .text
    .thumb
    .thumb_func
    .align 1
    .globl  stack_profiler_get_max_stack_usage
    .type   stack_profiler_get_max_stack_usage, %function

stack_profiler_get_max_stack_usage:
    .fnstart

    LDR     R2, =__StackLimit
    LDR     R1, =MAGIC_WORD
    LDR     R0, =__StackTop
    SUBS    R0, R0, R2

stack_profiler_get_max_stack_usage_loop:
    LDMIA   R2!, {R3}
    CMP     R3, R1
    BNE     stack_profiler_get_max_stack_usage_end
    SUBS    R0, #0x04
    BNE     stack_profiler_get_max_stack_usage_loop

stack_profiler_get_max_stack_usage_end:
    BX      LR

    .pool
    .cantunwind
    .fnend
    .size stack_profiler_get_max_stack_usage, .-stack_profiler_get_max_stack_usage

    .end

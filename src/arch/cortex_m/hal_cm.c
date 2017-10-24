/*
 * Copyright (C) 2017 Sergey Koshkin <koshkin.sergey@gmail.com>
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Project: uKernel real-time kernel
 */

/*******************************************************************************
 *  includes
 ******************************************************************************/

#include "tn.h"
#include "tn_arch.h"
#include "tn_tasks.h"

/*******************************************************************************
 *  external declarations
 ******************************************************************************/

/*******************************************************************************
 *  defines and macros (scope: module-local)
 ******************************************************************************/

/* Interrupt Control State Register Address */
#define ICSR_ADDR                     (0xE000ED04)

/* pendSV bit in the Interrupt Control State Register */
#define PENDSVSET                     (0x10000000)

/* System Handlers 12-15 Priority Register Address */
#define PR_12_15_ADDR                 (0xE000ED20)

/* PRI_14 (PendSV) priority in the System Handlers 12-15 Priority Register Address
PendSV priority is minimal (0xFF) */
#define PENDS_VPRIORITY               (0x00FF0000)

/*******************************************************************************
 *  typedefs and structures (scope: module-local)
 ******************************************************************************/

/*******************************************************************************
 *  global variable definitions  (scope: module-exported)
 ******************************************************************************/

/*******************************************************************************
 *  global variable definitions (scope: module-local)
 ******************************************************************************/

/*******************************************************************************
 *  function prototypes (scope: module-local)
 ******************************************************************************/

/*******************************************************************************
 *  function implementations (scope: module-local)
 ******************************************************************************/

/*******************************************************************************
 *  function implementations (scope: module-exported)
 ******************************************************************************/

__asm
void tn_start_exe(void)
{
  EXPORT tn_switch_context_exit

  ldr    r1, =PR_12_15_ADDR        ;-- Load the System 12-15 Priority Register
  ldr    r0, [r1]
  ldr    r2, =PENDS_VPRIORITY
  orrs   r0, r0, r2
  str    r0, [r1]

;---------------

  ldr    r1, =__cpp(&run_task)
  ldr    r2, [r1]
  ldr    r0, [r2]                  ;-- in r0 - new task SP
  adds   r0, #32
  msr    PSP, r0

tn_switch_context_exit

  ldr    r1,  =ICSR_ADDR           ;  Trigger PendSV exception
  ldr    r0,  =PENDSVSET
  str    r0,  [r1]

  cpsie  I                         ;  Enable core interrupts

; - Should never reach

  b  .

  ALIGN
}

__asm
void tn_disable_irq(void)
{
  cpsid  I
  bx     lr
}

__asm
void tn_enable_irq(void)
{
  cpsie I
  bx   lr
}

__asm
void tn_switch_context_request(void)
{
  ldr    r1,  =ICSR_ADDR
  ldr    r0,  =PENDSVSET
  str    r0,  [r1]
  bx     lr

  ALIGN
}

#if (defined (__ARM_ARCH_6M__ ) && (__ARM_ARCH_6M__  == 1))

__asm
int tn_cpu_save_sr(void)
{
  mrs    r0, PRIMASK
  cpsid  I
  bx     lr
}

__asm
void tn_cpu_restore_sr(int sr)
{
  msr    PRIMASK, r0
  bx     lr
}

#elif ((defined (__ARM_ARCH_7M__ ) && (__ARM_ARCH_7M__  == 1)) || \
       (defined (__ARM_ARCH_7EM__) && (__ARM_ARCH_7EM__ == 1))     )

__asm
int tn_cpu_set_basepri(int basepri)
{
  mrs    r1, BASEPRI
  msr    BASEPRI, r0
  mov    r0, r1
  bx     lr
}

__asm
void tn_cpu_restore_basepri(int basepri)
{
  msr    BASEPRI, r0
  bx     lr
}

__asm
int ffs_asm(unsigned int val)
{
  mov    r1, r0                    ;-- tmp = in
  rsbs   r0, r1, #0                ;-- in = -in
  ands   r0, r0, r1                ;-- in = in & tmp
  CLZ.W  r0, r0
  rsb    r0, r0, #0x20             ;-- 32 - in
  bx     lr
}

#endif

unsigned int * tn_stack_init(void *task_func, unsigned int *stack_start, void *param)
{
  unsigned int *stk = ++stack_start;            //-- Load stack pointer

  *(--stk) = 0x01000000L;                       //-- xPSR
  *(--stk) = (unsigned int)task_func;           //-- Entry Point (1 for THUMB mode)
  *(--stk) = (unsigned int)task_exit;           //-- R14 (LR)    (1 for THUMB mode)
  *(--stk) = 0x12121212L;                       //-- R12
  *(--stk) = 0x03030303L;                       //-- R3
  *(--stk) = 0x02020202L;                       //-- R2
  *(--stk) = 0x01010101L;                       //-- R1
  *(--stk) = (unsigned int)param;               //-- R0 - task's function argument
  *(--stk) = 0x11111111L;                       //-- R11
  *(--stk) = 0x10101010L;                       //-- R10
  *(--stk) = 0x09090909L;                       //-- R9
  *(--stk) = 0x08080808L;                       //-- R8
  *(--stk) = 0x07070707L;                       //-- R7
  *(--stk) = 0x06060606L;                       //-- R6
  *(--stk) = 0x05050505L;                       //-- R5
  *(--stk) = 0x04040404L;                       //-- R4

  return stk;
}

__asm
void PendSV_Handler(void)
{
  PRESERVE8

#if (defined (__ARM_ARCH_6M__ ) && (__ARM_ARCH_6M__  == 1))
  cpsid  I                          ;   Disable core int

  ldr    r3, =__cpp(&run_task)      ;  in R3 - =run_task
  ldm    r3!, {r1,r2}
  cmp    r1, r2                     ;  in R1 - tn_curr_run_task, in R2 - tn_next_task_to_run
  beq    exit_context_switch

  subs   r3, #8
  mrs    r0, psp                    ; in PSP - process(task) stack pointer

  subs   r0, #32                    ; allocate space for r4-r11
  str    r0, [r1]                   ;  save own SP in TCB
  stmia  r0!, {r4-r7}
  mov    r4, r8
  mov    r5, r9
  mov    r6, r10
  mov    r7, r11
  stmia  r0!, {r4-r7}

  str    r2, [r3]                   ;  in r3 - =tn_curr_run_task
  ldr    r0, [r2]                   ;  in r0 - new task SP

  adds   r0, #16
  ldmia  r0!, {r4-r7}
  mov    r8, r4
  mov    r9, r5
  mov    r10, r6
  mov    r11, r7
  msr    psp, r0
  subs   r0, #32
  ldmia  r0!, {r4-r7}

exit_context_switch

  cpsie  I                          ;  enable core int

  ldr    r0, =0xFFFFFFFD

#elif ((defined (__ARM_ARCH_7M__ ) && (__ARM_ARCH_7M__  == 1)) || \
       (defined (__ARM_ARCH_7EM__) && (__ARM_ARCH_7EM__ == 1))     )

  ldr    r0, =__cpp(&max_syscall_interrupt_priority)
  msr    BASEPRI, r0                ;  Start critical section

  ldr    r3, =__cpp(&run_task)      ;  in R3 - =run_task
  ldm    r3, {r1,r2}
  cmp    r1, r2                     ;  in R1 - tn_curr_run_task, in R2 - tn_next_task_to_run
  beq    exit_context_switch

  mrs    r0, psp                    ; in PSP - process(task) stack pointer
  stmdb  r0!, {r4-r11}
  str    r0, [r1]                   ;  save own SP in TCB
  str    r2, [r3]                   ;  in r3 - =tn_curr_run_task
  ldr    r0, [r2]                   ;  in r0 - new task SP
  ldmia  r0!, {r4-r11}
  msr    psp, r0

exit_context_switch

  mov    r0, #0
  msr    BASEPRI, r0                ;  End critical section

  ldr    r0, =0xFFFFFFFD

#endif

  bx     r0

  ALIGN
}

/* ----------------------------- End of file ---------------------------------*/

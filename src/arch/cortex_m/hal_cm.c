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

#include "knl_lib.h"

/*******************************************************************************
 *  external declarations
 ******************************************************************************/

/*******************************************************************************
 *  defines and macros (scope: module-local)
 ******************************************************************************/

/* PendSV priority is minimal (0xFF) */
#define PENDSV_PRIORITY (0x00FF0000)

#define NVIC_AIR_CTRL   (*((volatile uint32_t *)0xE000ED0CU))
/* System Handler Priority Register 2 Address */
#define NVIC_SYS_PRI2   (*((volatile uint32_t *)0xE000ED1CU))
/* System Handler Priority Register 3 Address */
#define NVIC_SYS_PRI3   (*((volatile uint32_t *)0xE000ED20U))

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

static
void SystemIsrInit(void)
{
#if !defined(__TARGET_ARCH_6S_M)
  uint32_t sh, prigroup;
#endif
  NVIC_SYS_PRI3 |= PENDSV_PRIORITY;
#if defined(__TARGET_ARCH_6S_M)
  NVIC_SYS_PRI2 |= (NVIC_SYS_PRI3<<(8+1)) & 0xFC000000U;
#else
  sh       = 8U - __clz(~((NVIC_SYS_PRI3 << 8) & 0xFF000000U));
  prigroup = ((NVIC_AIR_CTRL >> 8) & 0x07U);
  if (prigroup >= sh) {
    sh = prigroup + 1U;
  }
  NVIC_SYS_PRI2 = ((0xFEFFFFFFU << sh) & 0xFF000000U) | (NVIC_SYS_PRI2 & 0x00FFFFFFU);
#endif
}

/**
 * @fn      void archKernelStart(void)
 * @brief
 */
void archKernelStart(void)
{
  SystemIsrInit();
  __set_PSP((uint32_t)knlInfo.run.curr->task_stk + 32UL);
  archSwitchContextRequest();
  __enable_irq();
}

#if (defined (__ARM_ARCH_6M__ ) && (__ARM_ARCH_6M__  == 1))

/**
 * @fn      uint32_t tn_cpu_save_sr(void)
 * @brief
 */
__asm
uint32_t tn_cpu_save_sr(void)
{
  mrs    r0, PRIMASK
  cpsid  I
  bx     lr
}

/**
 * @fn      void tn_cpu_restore_sr(uint32_t sr)
 * @brief
 */
__asm
void tn_cpu_restore_sr(uint32_t sr)
{
  msr    PRIMASK, r0
  bx     lr
}

#elif ((defined (__ARM_ARCH_7M__ ) && (__ARM_ARCH_7M__  == 1)) || \
       (defined (__ARM_ARCH_7EM__) && (__ARM_ARCH_7EM__ == 1))     )

/**
 * @fn      uint32_t tn_cpu_set_basepri(uint32_t basepri)
 * @brief
 */
__asm
uint32_t tn_cpu_set_basepri(uint32_t basepri)
{
  mrs    r1, BASEPRI
  msr    BASEPRI, r0
  mov    r0, r1
  bx     lr
}

/**
 * @fn      void tn_cpu_restore_basepri(uint32_t basepri)
 * @brief
 */
__asm
void tn_cpu_restore_basepri(uint32_t basepri)
{
  msr    BASEPRI, r0
  bx     lr
}

/**
 * @fn      int32_t ffs_asm(uint32_t val)
 * @brief
 */
__asm
int32_t ffs_asm(uint32_t val)
{
  mov    r1, r0                    ;-- tmp = in
  rsbs   r0, r1, #0                ;-- in = -in
  ands   r0, r0, r1                ;-- in = in & tmp
  CLZ.W  r0, r0
  rsb    r0, r0, #0x20             ;-- 32 - in
  bx     lr
}

#endif

/**
 * @fn    uint32_t* StackInit(const osTask_t *task)
 * @brief
 * @param[in] task
 * @return
 */
void StackInit(osTask_t *task)
{
  uint32_t *stk = task->stk_start;              //-- Load stack pointer
  stk++;

  *(--stk) = 0x01000000L;                       //-- xPSR
  *(--stk) = (uint32_t)task->func_addr;         //-- Entry Point
  *(--stk) = (uint32_t)ThreadExit;              //-- R14 (LR)    (1 for THUMB mode)
  *(--stk) = 0x12121212L;                       //-- R12
  *(--stk) = 0x03030303L;                       //-- R3
  *(--stk) = 0x02020202L;                       //-- R2
  *(--stk) = 0x01010101L;                       //-- R1
  *(--stk) = (uint32_t)task->func_param;        //-- R0 - task's function argument
  *(--stk) = 0x11111111L;                       //-- R11
  *(--stk) = 0x10101010L;                       //-- R10
  *(--stk) = 0x09090909L;                       //-- R9
  *(--stk) = 0x08080808L;                       //-- R8
  *(--stk) = 0x07070707L;                       //-- R7
  *(--stk) = 0x06060606L;                       //-- R6
  *(--stk) = 0x05050505L;                       //-- R5
  *(--stk) = 0x04040404L;                       //-- R4

  task->task_stk = stk;
}

/**
 * @fn      void PendSV_Handler(void)
 * @brief
 */
__asm
void PendSV_Handler(void)
{
  PRESERVE8

#if (defined (__ARM_ARCH_6M__ ) && (__ARM_ARCH_6M__  == 1))
  cpsid  I                          ; Disable core int

  ldr    r3, =__cpp(&knlInfo.run)   ; in R3 - =run_task
  ldm    r3!, {r1,r2}
  cmp    r1, r2                     ; in R1 - tn_curr_run_task, in R2 - tn_next_task_to_run
  beq    exit_context_switch

  subs   r3, #8
  mrs    r0, psp                    ; in PSP - process(task) stack pointer

  subs   r0, #32                    ; allocate space for r4-r11
  str    r0, [r1]                   ; save own SP in TCB
  stmia  r0!, {r4-r7}
  mov    r4, r8
  mov    r5, r9
  mov    r6, r10
  mov    r7, r11
  stmia  r0!, {r4-r7}

  str    r2, [r3]                   ; in r3 - =tn_curr_run_task
  ldr    r0, [r2]                   ; in r0 - new task SP

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

  cpsie  I                          ; enable core int

  ldr    r0, =0xFFFFFFFD

#elif ((defined (__ARM_ARCH_7M__ ) && (__ARM_ARCH_7M__  == 1)) || \
       (defined (__ARM_ARCH_7EM__) && (__ARM_ARCH_7EM__ == 1))     )

  ldr    r0, =__cpp(&knlInfo.max_syscall_interrupt_priority)
  msr    BASEPRI, r0                ; Start critical section

  ldr    r3, =__cpp(&knlInfo.run)   ; in R3 - =run_task
  ldm    r3, {r1,r2}
  cmp    r1, r2                     ; in R1 - tn_curr_run_task, in R2 - tn_next_task_to_run
  beq    exit_context_switch

  mrs    r0, psp                    ; in PSP - process(task) stack pointer
  stmdb  r0!, {r4-r11}
  str    r0, [r1]                   ; save own SP in TCB
  str    r2, [r3]                   ; in r3 - =tn_curr_run_task
  ldr    r0, [r2]                   ; in r0 - new task SP
  ldmia  r0!, {r4-r11}
  msr    psp, r0

exit_context_switch

  mov    r0, #0
  msr    BASEPRI, r0                ; End critical section

  ldr    r0, =0xFFFFFFFD

#endif

  bx     r0

  ALIGN
}

/**
 * @fn      void SVC_Handler(void)
 * @brief
 */
__asm
void SVC_Handler(void)
{
  PRESERVE8

  MRS     R0,PSP                  ; Get PSP
  LDR     R1,[R0,#24]             ; Read Saved PC from Stack
  SUBS    R1,R1,#2                ; Point to SVC Instruction
  LDRB    R1,[R1]                 ; Load SVC Number
  CMP     R1,#0
  BNE     SVC_Exit                ; User SVC Number > 0

  PUSH    {R0,LR}                 ; Save SP and EXC_RETURN
  LDMIA   R0,{R0-R3}              ; Read R0-R3 from stack
  BLX     R12                     ; Call SVC Function
  POP     {R2,R3}                 ; Restore SP and EXC_RETURN
  MOV     LR,R3                   ; Set EXC_RETURN
  STMIA   R2!,{R0-R1}             ; Store function return values

SVC_Exit
  BX      LR                      ; Exit from handler

  ALIGN
}

/* ----------------------------- End of file ---------------------------------*/

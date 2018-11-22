/*
 * Copyright (C) 2017-2018 Sergey Koshkin <koshkin.sergey@gmail.com>
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

#if ((defined (__ARM_ARCH_7M__ ) && (__ARM_ARCH_7M__  == 1)) || \
       (defined (__ARM_ARCH_7EM__) && (__ARM_ARCH_7EM__ == 1))     )

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
 * @fn      void PendSV_Handler(void)
 * @brief
 */
__asm
void PendSV_Handler(void)
{
  PRESERVE8

#if (defined (__ARM_ARCH_6M__ ) && (__ARM_ARCH_6M__  == 1))

  LDR     R3,=__cpp(&knlInfo.run) ; in R3 - =run_task
  LDM     R3!, {R1,R2}
  CMP     R1,R2                   ; in R1 - tn_curr_run_task, in R2 - tn_next_task_to_run
  BEQ     Context_Exit

  CMP     R1,#0
  BEQ     Context_Switch

  MRS     R0,PSP                  ; in PSP - process(task) stack pointer
  SUBS    R0,#32                  ; allocate space for r4-r11
  STR     R0,[R1]                 ; save own SP in TCB
  STMIA   R0!,{R4-R7}
  MOV     R4,R8
  MOV     R5,R9
  MOV     R6,R10
  MOV     R7,R11
  STMIA   R0!,{R4-R7}

Context_Switch
  SUBS    R3,#8
  STR     R2,[R3]                 ; in r3 - =tn_curr_run_task

  LDR     R0,[R2]                 ; in r0 - new task SP
  ADDS    R0,#16
  LDMIA   R0!,{R4-R7}
  MOV     R8,R4
  MOV     R9,R5
  MOV     R10,R6
  MOV     R11,R7
  MSR     PSP,R0
  SUBS    R0,#32
  LDMIA   R0!,{R4-R7}

  LDR     R0,=0xFFFFFFFD
  BX      R0

Context_Exit
  BX      LR                      ; Exit from handler


#elif ((defined (__ARM_ARCH_7M__ ) && (__ARM_ARCH_7M__  == 1)) || \
       (defined (__ARM_ARCH_7EM__) && (__ARM_ARCH_7EM__ == 1))     )

;  LDR     R0,=__cpp(&knlInfo.max_syscall_interrupt_priority)
;  MSR     BASEPRI,R0              ; Start critical section

  LDR     R3,=__cpp(&knlInfo.run) ; in R3 - =run_task
  LDM     R3,{R1,R2}
  CMP     R1,R2                   ; in R1 - tn_curr_run_task, in R2 - tn_next_task_to_run
  BEQ     Context_Exit            ; Exit when threads are the same

  CBZ     R1,Context_Switch

  MRS     R0,PSP                  ; in PSP - process(task) stack pointer
  STMDB   R0!,{R4-R11}
  STR     R0,[R1]                 ; save own SP in TCB

Context_Switch
  STR     R2,[R3]                 ; in r3 - =tn_curr_run_task

  LDR     R0,[R2]                 ; in r0 - new task SP
  LDMIA   R0!,{R4-R11}
  MSR     PSP,R0

Context_Exit
;  MOV     R0,#0
;  MSR     BASEPRI,R0              ; End critical section

  MVN     LR,#~0xFFFFFFFD         ; Set EXC_RETURN value
  BX      LR                      ; Exit from handler

#endif

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

#if (defined (__ARM_ARCH_6M__ ) && (__ARM_ARCH_6M__  == 1))

  MOV     R0,LR
  LSRS    R0,R0,#3                ; Determine return stack from EXC_RETURN bit 2
  BCC     SVC_MSP                 ; Branch if return stack is MSP
  MRS     R0,PSP                  ; Get PSP

SVC_Number
  LDR     R1,[R0,#24]             ; Read Saved PC from Stack
  SUBS    R1,R1,#2                ; Point to SVC Instruction
  LDRB    R1,[R1]                 ; Load SVC Number
  CMP     R1,#0
  BNE     SVC_Exit                ; User SVC Number > 0

  PUSH    {R0,LR}                 ; Save SP and EXC_RETURN
  LDMIA   R0,{R0-R3}              ; Read R0-R3 from stack
  BLX     R7                      ; Call service function
  POP     {R2,R3}                 ; Restore SP and EXC_RETURN
  STMIA   R2!,{R0-R1}             ; Store return values
  MOV     LR,R3                   ; Set EXC_RETURN

SVC_Exit
  BX      LR                      ; Exit from handler

SVC_MSP
  MRS     R0,MSP                  ; Get MSP
  B       SVC_Number

#elif ((defined (__ARM_ARCH_7M__ ) && (__ARM_ARCH_7M__  == 1)) || \
       (defined (__ARM_ARCH_7EM__) && (__ARM_ARCH_7EM__ == 1))     )

  TST     LR,#0x04                ; Determine return stack from EXC_RETURN bit 2
  ITE     EQ
  MRSEQ   R0,MSP                  ; Get MSP if return stack is MSP
  MRSNE   R0,PSP                  ; Get PSP if return stack is PSP

  LDR     R1,[R0,#24]             ; Read Saved PC from Stack
  LDRB    R1,[R1,#-2]             ; Load SVC Number
  CBNZ    R1,SVC_Exit

  PUSH    {R0,LR}                 ; Save SP and EXC_RETURN
  LDM     R0,{R0-R3,R12}          ; Read R0-R3,R12 from stack
  BLX     R12                     ; Call SVC Function
  POP     {R12,LR}                ; Restore SP and EXC_RETURN
  STM     R12,{R0-R1}             ; Store return values

SVC_Exit
  BX      LR                      ; Exit from handler

#endif

  ALIGN
}

/* ----------------------------- End of file ---------------------------------*/

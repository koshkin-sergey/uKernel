/******************** Copyright (c) 2011-2012. All rights reserved *************
  File Name  : tn_timer.c
  Author     : Koshkin Sergey
  Version    : 2.7
  Date       : 18/12/2012
  Description: Этот файл содержит реализацию низкоуровневых функций зависящих
               от ядра микроконтроллера. Этот файл написан под компилятор
               ARM применяемый в Keil.

  All rights reserved.

  Permission to use, copy, modify, and distribute this software in source
  and binary forms and its documentation for any purpose and without fee
  is hereby granted, provided that the above copyright notice appear
  in all copies and that both that copyright notice and this permission
  notice appear in supporting documentation.

  THIS SOFTWARE IS PROVIDED BY THE SERGEY KOSHKIN AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL SERGEY KOSHKIN OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
  SUCH DAMAGE.

*******************************************************************************/
#include "tn.h"
#include "tn_utils.h"

#if defined (__CC_ARM)

#pragma arm

#define USERMODE    0x10
#define FIQMODE     0x11
#define IRQMODE     0x12
#define SVCMODE     0x13
#define ABORTMODE   0x17
#define UNDEFMODE   0x1b
#define MODEMASK    0x1f
#define NOINT       0xc0
#define TBIT        0x20
#define IRQMASK     0x80
#define FIQMASK     0x40

unsigned char L_ffs_table[] = {
 0,  1,  2, 13,  3,  7,  0, 14,
 4,  0,  8,  0,  0,  0,  0, 15,
11,  5,  0,  0,  9,  0,  0, 26,
 0,  0,  0,  0,  0, 22, 28, 16,
32, 12,  6,  0,  0,  0,  0,  0,
10,  0,  0, 25,  0,  0, 21, 27,
31,  0,  0,  0,  0, 24,  0, 20,
30,  0, 23, 19, 29, 18, 17,  0
};

BOOL switch_context_request;

/*-----------------------------------------------------------------------------*
  Название :  tn_start_exe
  Описание :  
  Параметры:  
  Результат:  
*-----------------------------------------------------------------------------*/
__asm void tn_start_exe(void)
{
  EXPORT  tn_switch_context_exit

tn_switch_context_exit
  ldr    r0,  =__cpp(&tn_curr_run_task)
  ldr    r1,  =__cpp(&tn_next_task_to_run)
  ldr    r1,  [r1]                   ; get stack pointer
  ldr    sp,  [r1]                   ; switch to the new stack
  str    r1,  [r0]                   ; set new current running task tcb address
  
  ldmfd  sp!, {r0}
  msr    SPSR_cxsf, r0
  ldmfd  sp!, {r0-r12,lr,pc}^
}

/*-----------------------------------------------------------------------------*
  Название :  tn_switch_context
  Описание :  
  Параметры:  
  Результат:  
*-----------------------------------------------------------------------------*/
__asm void tn_switch_context(void)
{
  ldr    r0,  =__cpp(&switch_context_request)
  ldrb   r1,  [r0,#0]
  cmp    r1,  #0
  bxeq   lr
  mov    r1,  #0
  strb   r1,  [r0,#0]

  stmfd  sp!, {lr}          ; Save return address
  stmfd  sp!, {r0-r12,lr}   ; Save curr task registers
  mrs    r0,  cpsr
  tst    LR,  #1            ; from Thumb mode ?
  orrne  R0,  R0, #0x20     ; set the THUMB bit
  stmfd  sp!, {r0}          ; Save current CPSR
  
  mrs    r0,  cpsr
  orr    r0,  r0, #NOINT    ; Disable Int
  msr    CPSR_c,  r0
  
  ldr    r1,  =__cpp(&tn_curr_run_task)
  ldr    r2,  [r1]
  ldr    r0,  =__cpp(&tn_next_task_to_run)
  ldr    r0,  [r0]
  cmp    r2,  r0
  beq    tn_sw_restore
  
  str    sp,  [r2]          ; store sp in preempted tasks TCB
  ldr    sp,  [r0]          ; get new task sp
  str    r0,  [r1]          ; tn_curr_run_task = tn_next_task_to_run
  
tn_sw_restore
  ldmfd  sp!, {r0}
  msr    SPSR_cxsf, r0
  ldmfd  sp!, {r0-r12,lr,pc}^
}

/*-----------------------------------------------------------------------------*
  Название :  tn_cpu_irq_isr
  Описание :  
  Параметры:  
  Результат:  
*-----------------------------------------------------------------------------*/
__asm void tn_cpu_irq_isr(void)
{
  PRESERVE8

  sub    lr,  lr, #4              ; Set lr to the actual return address
  stmfd  sp!, {r0-r12, lr}        ; save all registers
  
  ldr    r0,  =__cpp(tn_cpu_irq_handler)
  mov    lr,  pc
  bx     r0
  
  ldr    r0,  =__cpp(&tn_curr_run_task)   ; context switch ?
  ldr    r1,  [r0]
  ldr    r0,  =__cpp(&tn_next_task_to_run)
  ldr    r2,  [r0]
  cmp    r1,  r2                  ; if equal - return
  beq    exit_irq_int

  // Our target - get all registers of interrrupted task, saved in IRQ's stack
  // and save them in the interrupted task's stack
  mrs    r0,  spsr               // Get interrupted task's CPRS
  stmfd  sp!, {r0}               ; Save it in the IRQ stack
  add    sp,  sp, #4             ; Restore stack ptr after CPSR saving
  ldmfd  sp!, {r0-r12, lr}       ; Put all saved registers from IRQ
                                ;                 stack back to registers
  mov    r1,  sp                 // r1 <-  IRQ's SP
  sub    r1,  r1, #4
  msr    cpsr_c, #(NOINT :OR: SVCMODE) ; Change to SVC mode; INT are disabled
  
  // Now - in SVC mode; in r1 - IRQ's SP
  
  ldr    r0,  [r1], #-14*4       // r0 <- task's lr (instead pc)+ rewind stack ptr
  stmfd  sp!, {r0}
  stmfd  sp!, {r2-r12, lr}       // Save registers in interrupted task's
                                ; stack,except CPSR,r0,r1
  ldr    r0,  [r1],#2*4          // Get interrupted task's CPSR (pos 0(-15))
  ldr    r2,  [r1],#-4           ; Get valid r1 to save (pos 2(-13))
  ldr    r1,  [r1]               ; Get valid r0 to save (pos 1(-14))
  stmfd  sp!, {r0-r2}            ; Save r0, r1, and CPSR
  
  ; Registers has been saved. Now - switch context
  
  ldr    r0,  =__cpp(&tn_curr_run_task)
  ldr    r0,  [r0]
  str    sp,  [r0]               ; SP <- curr task
  
  ldr    r0,  =__cpp(&tn_next_task_to_run)
  ldr    r2,  [r0]
  ldr    r0,  =__cpp(&tn_curr_run_task)
  str    r2,  [r0]
  ldr    sp,  [r2]
  
  ; Return
  
  ldmfd  sp!, {r0}               ; Get CPSR
  msr    spsr_cxsf, r0           ; SPSR <- CPSR
  ldmfd  sp!, {r0-r12, lr, pc}^  ; Restore all registers, CPSR also

exit_irq_int
  ldmfd  sp!, {r0-r12, pc}^       ; exit

  ALIGN
}

/*-----------------------------------------------------------------------------*
  Название :  tn_cpu_fiq_isr
  Описание :  
  Параметры:  
  Результат:  
*-----------------------------------------------------------------------------*/
__asm void tn_cpu_fiq_isr(void)
{
  stmfd  SP!, {R0-R12,LR}
  ldmfd  SP!, {R0-R12,LR}        // restore registers of interrupted task's stack
  subs   PC,  LR, #4             ; return from FIQ
}

/*-----------------------------------------------------------------------------*
  Название :  tn_stack_init
  Описание :  
  Параметры:  
  Результат:  
*-----------------------------------------------------------------------------*/
unsigned int* tn_stack_init(void *task_func, void *stack_start, void *param)
{
  unsigned int *stk;
  
  //-- filling register's position in stack (except R0,CPSR,SPSR positions) -
  //-- for debugging only
  
  stk  = (unsigned int *)stack_start;      //-- Load stack pointer
  
  *stk = ((unsigned int)task_func) & ~1;   //-- Entry Point
  stk--;
  
  *stk = (unsigned int)tn_task_exit;       //-- LR  //0x14141414L
  stk--;
  
  *stk = 0x12121212L;              //-- R12
  stk--;
  
  *stk = 0x11111111L;              //-- R11
  stk--;
  
  *stk = 0x10101010L;              //-- R10
  stk--;
  
  *stk = 0x09090909L;              //-- R9
  stk--;
  
  *stk = 0x08080808L;              //-- R8
  stk--;
  
  *stk = 0x07070707L;              //-- R7
  stk--;
  
  *stk = 0x06060606L;              //-- R6
  stk--;
  
  *stk = 0x05050505L;              //-- R5
  stk--;
  
  *stk = 0x04040404L;              //-- R4
  stk--;
  
  *stk = 0x03030303L;              //-- R3
  stk--;
  
  *stk = 0x02020202L;              //-- R2
  stk--;
  
  *stk = 0x01010101L;              //-- R1
  stk--;
  
  *stk = (unsigned int)param;      //-- R0 : task's function argument
  stk--;
  if ((unsigned int)task_func & 1) //-- task func is in the THUMB mode
    *stk = (unsigned int)0x33;    //-- CPSR - Enable both IRQ and FIQ ints + THUMB
  else
    *stk = (unsigned int)0x13;    //-- CPSR - Enable both IRQ and FIQ ints
  
  return stk;
}

/*-----------------------------------------------------------------------------*
  Название :  tn_arm_enable_interrupts
  Описание :  
  Параметры:  
  Результат:  
*-----------------------------------------------------------------------------*/
__asm void tn_arm_enable_interrupts(void)
{
  mrs  r0, cpsr
  bic  r0, r0, #NOINT
  msr  cpsr_c, r0
  bx   lr
}

/*-----------------------------------------------------------------------------*
  Название :  tn_arm_disable_interrupts
  Описание :  
  Параметры:  
  Результат:  
*-----------------------------------------------------------------------------*/
__asm void tn_arm_disable_interrupts(void)
{
  mrs  r0, cpsr
  orr  r0, r0, #NOINT
  msr  cpsr_c, r0
  bx   lr
}

/*-----------------------------------------------------------------------------*
  Название :  tn_cpu_save_sr
  Описание :  
  Параметры:  
  Результат:  
*-----------------------------------------------------------------------------*/
__asm int tn_cpu_save_sr(void)
{
  mrs    r0,  CPSR               ; Disable both IRQ & FIQ interrupts
  orr    r1,  r0, #NOINT
  msr    CPSR_c, r1
  
  ;-- Atmel add-on
  ;  mrs   r1,  CPSR               ; Check CPSR for correct contents
  ;  and   r1,  r1, #NOINT
  ;  cmp   r1,  #NOINT
  ;  bne   tn_cpu_save_sr          ; Not disabled - loop to try again
  
  bx     lr
}

/*-----------------------------------------------------------------------------*
  Название :  tn_cpu_restore_sr
  Описание :  
  Параметры:  
  Результат:  
*-----------------------------------------------------------------------------*/
__asm void tn_cpu_restore_sr(int sr)
{
  msr    CPSR_c, r0
  bx     lr
}

/*-----------------------------------------------------------------------------*
  Название :  tn_inside_irq
  Описание :  
  Параметры:  
  Результат:  
*-----------------------------------------------------------------------------*/
__asm int tn_inside_irq(void)
{
  mrs    r0, cpsr
  and    r0, r0, #MODEMASK
  cmp    r0, #IRQMODE
  bne    tn_inside_int_1
  bx     lr
  
tn_inside_int_1
  mov    r0, #0
  bx     lr
}

/*-----------------------------------------------------------------------------*
  Название :  ffs_asm
  Описание :  
  Параметры:  
  Результат:  
*-----------------------------------------------------------------------------*/
__asm int ffs_asm(unsigned int val)
{
  ; Standard trick to isolate bottom bit in r0 or 0 if r0 = 0 on entry
  rsb    r1, r0, #0
  ands   r0, r0, r1

  ; now r0 has at most one set bit, call this X
  ; if X = 0, all further instructions are skipped
  ldrne  r2, =__cpp(L_ffs_table)
  orrne  r0, r0, r0, lsl #4       ; r0 = X * 0x11
  orrne  r0, r0, r0, lsl #6       ; r0 = X * 0x451
  rsbne  r0, r0, r0, lsl #16      ; r0 = X * 0x0450fbaf

  ; now lookup in table indexed on top 6 bits of r0
  ldrneb r0, [ r2, r0, lsr #26 ]
  bx     lr
}

/*-----------------------------------------------------------------------------*
  Название :  tn_switch_context_request
  Описание :  
  Параметры:  
  Результат:  
*-----------------------------------------------------------------------------*/
void tn_switch_context_request(void)
{
  switch_context_request = TRUE;
}

#else
  #error "The incorrect compiler"
#endif

/*------------------------------ Конец файла ---------------------------------*/

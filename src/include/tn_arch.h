/*******************************************************************************
 *
 * TNKernel real-time kernel
 *
 * Copyright © 2004, 2013 Yuri Tiomkin
 * Copyright © 2013-2016 Sergey Koshkin <koshkin.sergey@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 ******************************************************************************/

#ifndef  _TN_ARCH_H_
#define  _TN_ARCH_H_

/*******************************************************************************
 *  includes
 ******************************************************************************/

/*******************************************************************************
 *  defines and macros (scope: module-local)
 ******************************************************************************/

#if (defined (__TARGET_ARCH_4T) && (__TARGET_ARCH_4T == 1))
  #define __ARM_ARCH_4T__           1
#endif

#if ((defined (__TARGET_ARCH_6_M  ) && (__TARGET_ARCH_6_M   == 1)) || \
     (defined (__TARGET_ARCH_6S_M ) && (__TARGET_ARCH_6S_M  == 1))   )
  #define __ARM_ARCH_6M__           1
#endif

#if (defined (__TARGET_ARCH_7_M ) && (__TARGET_ARCH_7_M  == 1))
  #define __ARM_ARCH_7M__           1
#endif

#if (defined (__TARGET_ARCH_7E_M) && (__TARGET_ARCH_7E_M == 1))
  #define __ARM_ARCH_7EM__          1
#endif

#define TN_TIMER_STACK_SIZE           64
#define TN_IDLE_STACK_SIZE            48
#define TN_MIN_STACK_SIZE             40      //--  +20 for exit func when ver GCC > 4

#define TN_BITS_IN_INT                32
#define TN_ALIG                       sizeof(void*)
#define MAKE_ALIG(a)                  ((sizeof(a)+(TN_ALIG-1))&(~(TN_ALIG-1)))

#define TN_NUM_PRIORITY               TN_BITS_IN_INT  //-- 0..31  Priority 0 always is used by timers task
#define TN_FILL_STACK_VAL             0xFFFFFFFF

#if (defined (__ARM_ARCH_4T__ ) && (__ARM_ARCH_4T__  == 1))

  #define USE_ASM_FFS

  /* - Interrupt processing - processor specific -------------------------------*/
  #define BEGIN_CRITICAL_SECTION  int tn_save_status_reg = tn_cpu_save_sr();
  #define END_CRITICAL_SECTION    tn_cpu_restore_sr(tn_save_status_reg);  \
                                  if (!tn_inside_irq())                   \
                                    tn_switch_context();

  #define BEGIN_DISABLE_INTERRUPT int tn_save_status_reg = tn_cpu_save_sr();
  #define END_DISABLE_INTERRUPT   tn_cpu_restore_sr(tn_save_status_reg);

#endif

#if (defined (__ARM_ARCH_6M__ ) && (__ARM_ARCH_6M__  == 1))

  /* - Assembler functions prototypes ----------------------------------------*/
  #define  tn_switch_context()

  /* - Interrupt processing - processor specific -----------------------------*/
  #define BEGIN_DISABLE_INTERRUPT int tn_save_status_reg = tn_cpu_save_sr();
  #define END_DISABLE_INTERRUPT   tn_cpu_restore_sr(tn_save_status_reg);

  #define BEGIN_CRITICAL_SECTION  BEGIN_DISABLE_INTERRUPT
  #define END_CRITICAL_SECTION    END_DISABLE_INTERRUPT

#endif

#if ((defined (__ARM_ARCH_7M__ ) && (__ARM_ARCH_7M__  == 1)) || \
     (defined (__ARM_ARCH_7EM__) && (__ARM_ARCH_7EM__ == 1))     )

  #define USE_ASM_FFS

  /* - Assembler functions prototypes ----------------------------------------*/
  #define  tn_switch_context()

  /* - Interrupt processing - processor specific -----------------------------*/
  #define BEGIN_DISABLE_INTERRUPT int tn_save_status_reg = tn_cpu_set_basepri(max_syscall_interrupt_priority);
  #define END_DISABLE_INTERRUPT   tn_cpu_restore_basepri(tn_save_status_reg);

  #define BEGIN_ENABLE_INTERRUPT  int tn_save_status_reg = tn_cpu_set_basepri(0);
  #define END_ENABLE_INTERRUPT    tn_cpu_restore_basepri(tn_save_status_reg);

  #define BEGIN_CRITICAL_SECTION  BEGIN_DISABLE_INTERRUPT
  #define END_CRITICAL_SECTION    END_DISABLE_INTERRUPT

#endif

#ifndef BEGIN_CRITICAL_SECTION
  #define BEGIN_CRITICAL_SECTION
  #define END_CRITICAL_SECTION
#endif

#ifndef BEGIN_DISABLE_INTERRUPT
  #define BEGIN_DISABLE_INTERRUPT
  #define END_DISABLE_INTERRUPT
#endif

#ifndef BEGIN_ENABLE_INTERRUPT
  #define BEGIN_ENABLE_INTERRUPT
  #define END_ENABLE_INTERRUPT
#endif

/*******************************************************************************
 *  typedefs and structures (scope: module-local)
 ******************************************************************************/

/*******************************************************************************
 *  exported variables
 ******************************************************************************/

extern unsigned int max_syscall_interrupt_priority;

/*******************************************************************************
 *  exported function prototypes
 ******************************************************************************/

#ifdef __cplusplus
extern "C"  {
#endif

extern void tn_start_exe(void);
extern unsigned int* tn_stack_init(void *task_func, unsigned int *stack_start, void *param);
extern void tn_switch_context_request(void);
extern void tn_switch_context_exit(void);

#if (defined (__ARM_ARCH_4T__ ) && (__ARM_ARCH_4T__  == 1))

  extern void tn_switch_context(void);
  extern void tn_cpu_irq_handler(void);
  extern void tn_cpu_irq_isr(void);
  extern void tn_cpu_fiq_isr(void);
  extern int ffs_asm(unsigned int val);
  extern int tn_cpu_save_sr(void);
  extern void tn_cpu_restore_sr(int sr);
  extern int tn_inside_irq(void);

#endif

#if (defined (__ARM_ARCH_6M__ ) && (__ARM_ARCH_6M__  == 1))

  extern int tn_cpu_save_sr(void);
  extern void tn_cpu_restore_sr(int sr);

#endif

#if ((defined (__ARM_ARCH_7M__ ) && (__ARM_ARCH_7M__  == 1)) || \
     (defined (__ARM_ARCH_7EM__) && (__ARM_ARCH_7EM__ == 1))     )

  extern int ffs_asm(unsigned int val);
  extern int tn_cpu_set_basepri(int);
  extern void tn_cpu_restore_basepri(int);

#endif

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  // _TN_ARCH_H_

/*------------------------------ End of file ---------------------------------*/

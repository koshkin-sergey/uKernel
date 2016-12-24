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

#ifndef  _TN_PORT_H_
#define  _TN_PORT_H_

/*******************************************************************************
 *  includes
 ******************************************************************************/

#if defined __TARGET_ARCH_4T
  #include "../arch/arm/tn_port_arm.h"
#elif defined __TARGET_ARCH_6_M || __TARGET_ARCH_6S_M
  #include "../arch/cortex_m0/tn_port_cm0.h"
#elif defined __TARGET_ARCH_7_M
  #include "../arch/cortex_m3/tn_port_cm3.h"
#elif defined __TARGET_ARCH_7E_M
  #include "../arch/cortex_m3/tn_port_cm3.h"
#else
  #error "Wrong processor architecture"
#endif

/*******************************************************************************
 *  defines and macros (scope: module-local)
 ******************************************************************************/

#define TN_TIMER_STACK_SIZE           64
#define TN_IDLE_STACK_SIZE            48
#define TN_MIN_STACK_SIZE             40      //--  +20 for exit func when ver GCC > 4

#define TN_BITS_IN_INT                32
#define TN_ALIG                       sizeof(void*)
#define MAKE_ALIG(a)                  ((sizeof(a)+(TN_ALIG-1))&(~(TN_ALIG-1)))

#define TN_NUM_PRIORITY               TN_BITS_IN_INT  //-- 0..31  Priority 0 always is used by timers task
#define TN_FILL_STACK_VAL             0xFFFFFFFF

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

extern void tn_switch_context_request(void);
extern void tn_switch_context_exit(void);
extern int tn_cpu_save_sr(void);
extern void tn_cpu_restore_sr(int sr);
extern int tn_cpu_set_basepri(int);
extern void tn_cpu_restore_basepri(int);
extern void tn_start_exe(void);
extern int tn_inside_irq(void);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  // _TN_PORT_H_

/*------------------------------ End of file ---------------------------------*/

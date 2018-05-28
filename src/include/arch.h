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

#ifndef  _ARCH_H_
#define  _ARCH_H_

/*******************************************************************************
 *  includes
 ******************************************************************************/

#include <stdint.h>

/*******************************************************************************
 *  defines and macros (scope: module-local)
 ******************************************************************************/

#define TIMER_STACK_SIZE              (48U)
#define IDLE_STACK_SIZE               (48U)

#define TN_ALIG                       sizeof(void*)
#define MAKE_ALIG(a)                  ((sizeof(a)+(TN_ALIG-1))&(~(TN_ALIG-1)))
#define TN_FILL_STACK_VAL             0xFFFFFFFF

#if (defined (__ARM_ARCH_4T__ ) && (__ARM_ARCH_4T__  == 1))

  #define USE_ASM_FFS

  /* - Interrupt processing - processor specific -------------------------------*/
  #define BEGIN_CRITICAL_SECTION  uint32_t tn_save_status_reg = tn_cpu_save_sr();
  #define END_CRITICAL_SECTION    tn_cpu_restore_sr(tn_save_status_reg);  \
                                  if (!tn_inside_irq())                   \
                                    tn_switch_context();

  #define BEGIN_DISABLE_INTERRUPT uint32_t tn_save_status_reg = tn_cpu_save_sr();
  #define END_DISABLE_INTERRUPT   tn_cpu_restore_sr(tn_save_status_reg);

#endif

#if (defined (__ARM_ARCH_6M__ ) && (__ARM_ARCH_6M__  == 1))

  #include "core_cm.h"

__STATIC_INLINE bool IsIrqMode(void)
{
  return (__get_IPSR() != 0U);
}

__STATIC_INLINE bool IsIrqMasked(void)
{
  return (__get_PRIMASK() != 0U);
}

  #define STACK_OFFSET_R0()       (32U)

  /* - Interrupt processing - processor specific -----------------------------*/
  #define __SVC(num)              __svc_indirect_r7(num)
  #define SVC_CALL                __SVC(0)

  #define BEGIN_DISABLE_INTERRUPT uint32_t primask = __get_PRIMASK(); \
                                  __disable_irq();
  #define END_DISABLE_INTERRUPT   __set_PRIMASK(primask);

  #define BEGIN_CRITICAL_SECTION  BEGIN_DISABLE_INTERRUPT
  #define END_CRITICAL_SECTION    END_DISABLE_INTERRUPT

#endif

#if ((defined (__ARM_ARCH_7M__ ) && (__ARM_ARCH_7M__  == 1)) || \
     (defined (__ARM_ARCH_7EM__) && (__ARM_ARCH_7EM__ == 1))     )

  #include "core_cm.h"

  #define USE_ASM_FFS

__STATIC_INLINE bool IsIrqMode(void)
{
  return (__get_IPSR() != 0U);
}

__STATIC_INLINE bool IsIrqMasked(void)
{
  return ((__get_PRIMASK() != 0U) || (__get_BASEPRI() != 0U));
}

  #define STACK_OFFSET_R0()       (32U)

  /* - Interrupt processing - processor specific -----------------------------*/
  #define __SVC(num)              __svc_indirect(num)
  #define SVC_CALL                __SVC(0)

  #define BEGIN_DISABLE_INTERRUPT uint32_t basepri = __get_BASEPRI(); \
                                  __set_BASEPRI(knlInfo.max_syscall_interrupt_priority);
  #define END_DISABLE_INTERRUPT   __set_BASEPRI(basepri);

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

#ifndef SVC_CALL
  #define SVC_CALL
#endif

/*******************************************************************************
 *  typedefs and structures (scope: module-local)
 ******************************************************************************/

/*******************************************************************************
 *  exported variables
 ******************************************************************************/

/*******************************************************************************
 *  exported function prototypes
 ******************************************************************************/

void archKernelStart(void);
void archStackInit(osTask_t *task);
void archSwitchContextRequest(void);

#if (defined (__ARM_ARCH_4T__ ) && (__ARM_ARCH_4T__  == 1))

  extern void tn_switch_context(void);
  extern void tn_cpu_irq_handler(void);
  extern void tn_cpu_irq_isr(void);
  extern void tn_cpu_fiq_isr(void);
  extern int32_t ffs_asm(uint32_t val);
  extern uint32_t tn_cpu_save_sr(void);
  extern void tn_cpu_restore_sr(uint32_t sr);
  extern int32_t tn_inside_irq(void);

#endif

#if ((defined (__ARM_ARCH_7M__ ) && (__ARM_ARCH_7M__  == 1)) || \
     (defined (__ARM_ARCH_7EM__) && (__ARM_ARCH_7EM__ == 1))     )

  int32_t ffs_asm(uint32_t val);

#endif

#endif  // _ARCH_H_

/*------------------------------ End of file ---------------------------------*/

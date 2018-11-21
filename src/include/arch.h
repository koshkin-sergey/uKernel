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

#include <stdbool.h>
#include "cmsis_compiler.h"
#include "knl_lib.h"

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
/* Interrupt Control State Register Address */
#define ICSR                          (*((volatile uint32_t *)0xE000ED04))
/* PendSV bit in the Interrupt Control State Register */
#define PENDSVSET                     (0x10000000)

#define TIMER_STACK_SIZE              (48U)
#define IDLE_STACK_SIZE               (48U)

#define TN_ALIG                       sizeof(void*)
#define MAKE_ALIG(a)                  ((sizeof(a)+(TN_ALIG-1))&(~(TN_ALIG-1)))
#define TN_FILL_STACK_VAL             0xFFFFFFFF
#define STACK_OFFSET_R0()             (32U)

/**
 * @fn          bool IsIrqMode(void)
 * @brief       Check if in IRQ Mode
 * @return      true=IRQ, false=thread
 */
__STATIC_INLINE
bool IsIrqMode(void)
{
  return (__get_IPSR() != 0U);
}

/**
 * @fn          bool IsIrqMasked(void)
 * @brief       Check if IRQ is Masked
 * @return      true=masked, false=not masked
 */
__STATIC_INLINE
bool IsIrqMasked(void)
{
#if   ((defined(__ARM_ARCH_7M__)      && (__ARM_ARCH_7M__      != 0)) || \
       (defined(__ARM_ARCH_7EM__)     && (__ARM_ARCH_7EM__     != 0)) || \
       (defined(__ARM_ARCH_8M_MAIN__) && (__ARM_ARCH_8M_MAIN__ != 0)))
  return ((__get_PRIMASK() != 0U) || (__get_BASEPRI() != 0U));
#else
  return (__get_PRIMASK() != 0U);
#endif
}

#if (defined (__ARM_ARCH_6M__ ) && (__ARM_ARCH_6M__  == 1))

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

  #define USE_ASM_FFS

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

__STATIC_INLINE
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

__STATIC_INLINE
void archSwitchContextRequest(void)
{
  ICSR = PENDSVSET;
}

/**
 * @fn      void archKernelStart(void)
 * @brief
 */
__STATIC_INLINE __NO_RETURN
void archKernelStart(void)
{
  SystemIsrInit();
  __set_PSP((uint32_t)knlInfo.run.curr->stk + STACK_OFFSET_R0());
  archSwitchContextRequest();

  __enable_irq();

  for(;;);
}

#if ((defined (__ARM_ARCH_7M__ ) && (__ARM_ARCH_7M__  == 1)) || \
     (defined (__ARM_ARCH_7EM__) && (__ARM_ARCH_7EM__ == 1))     )

  int32_t ffs_asm(uint32_t val);

#endif

#endif  // _ARCH_H_

/*------------------------------ End of file ---------------------------------*/

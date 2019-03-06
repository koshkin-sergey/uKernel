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

/*******************************************************************************
 *  defines and macros (scope: module-local)
 ******************************************************************************/

/* PendSV priority is minimal (0xFF) */
#define PENDSV_PRIORITY               (0x00FF0000)

#define NVIC_AIR_CTRL                 (*((volatile uint32_t *)0xE000ED0CU))
/* System Handler Priority Register 2 Address */
#define NVIC_SYS_PRI2                 (*((volatile uint32_t *)0xE000ED1CU))
/* System Handler Priority Register 3 Address */
#define NVIC_SYS_PRI3                 (*((volatile uint32_t *)0xE000ED20U))
/* Interrupt Control State Register Address */
#define ICSR                          (*((volatile uint32_t *)0xE000ED04))
/* PendSV bit in the Interrupt Control State Register */
#define PENDSVSET                     (0x10000000)

#define TN_ALIG                       sizeof(void*)
#define FILL_STACK_VALUE              0xFFFFFFFF

#if   ((defined(__ARM_ARCH_7M__)      && (__ARM_ARCH_7M__      != 0)) ||       \
       (defined(__ARM_ARCH_7EM__)     && (__ARM_ARCH_7EM__     != 0)) ||       \
       (defined(__ARM_ARCH_8M_MAIN__) && (__ARM_ARCH_8M_MAIN__ != 0)))

  #if defined(__CC_ARM)
    #define SVC_INDIRECT_REG          r12
  #elif defined(__ICCARM__)
    #define SVC_FUNC(f)               __asm (                                  \
                                        "mov r12,%0\n"                         \
                                        :: "r"(f): "r12"                       \
                                      )
  #else
    #define SVC_INDIRECT_REG          "r12"
  #endif

#elif ((defined(__ARM_ARCH_6M__)      && (__ARM_ARCH_6M__      != 0)) ||       \
       (defined(__ARM_ARCH_8M_BASE__) && (__ARM_ARCH_8M_BASE__ != 0)))

  #if defined(__CC_ARM)
    #define SVC_INDIRECT_REG          r7
  #elif defined(__ICCARM__)
    #define SVC_FUNC(f)               __asm (                                  \
                                        "mov r7,%0\n"                          \
                                        :: "r"(f): "r7"                        \
                                      )
  #else
    #define SVC_INDIRECT_REG          "r7"
  #endif

#endif

#if   ((defined(__ARM_ARCH_7M__)      && (__ARM_ARCH_7M__      != 0)) ||       \
       (defined(__ARM_ARCH_7EM__)     && (__ARM_ARCH_7EM__     != 0)) ||       \
       (defined(__ARM_ARCH_8M_MAIN__) && (__ARM_ARCH_8M_MAIN__ != 0)))

#define BEGIN_CRITICAL_SECTION        uint32_t basepri = __get_BASEPRI(); \
                                      __set_BASEPRI(osInfo.base_priority);
#define END_CRITICAL_SECTION          __set_BASEPRI(basepri);

#elif ((defined(__ARM_ARCH_6M__)      && (__ARM_ARCH_6M__      != 0)) ||       \
       (defined(__ARM_ARCH_8M_BASE__) && (__ARM_ARCH_8M_BASE__ != 0)))

#define BEGIN_CRITICAL_SECTION        uint32_t primask = __get_PRIMASK(); \
                                      __disable_irq();
#define END_CRITICAL_SECTION          __set_PRIMASK(primask);

#endif

#ifndef BEGIN_CRITICAL_SECTION
  #define BEGIN_CRITICAL_SECTION
#endif

#ifndef END_CRITICAL_SECTION
  #define END_CRITICAL_SECTION
#endif

#ifndef __STATIC_INLINE
  #define __STATIC_INLINE
#endif

#ifndef __STATIC_FORCEINLINE
  #define __STATIC_FORCEINLINE
#endif

#ifndef __ASM
  #define __ASM
#endif

#ifndef __NO_RETURN
  #define __NO_RETURN
#endif

#ifndef __WEAK
  #define __WEAK
#endif

#ifndef   __USED
  #define __USED
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

__STATIC_INLINE
uint32_t SystemIsrInit(void)
{
#if !defined(__TARGET_ARCH_6S_M)
  uint32_t sh, prigroup;
#endif
  NVIC_SYS_PRI3 |= PENDSV_PRIORITY;
#if defined(__TARGET_ARCH_6S_M)
  NVIC_SYS_PRI2 |= (NVIC_SYS_PRI3<<(8+1)) & 0xFC000000U;

  return (0U);
#else
  sh       = 8U - __CLZ(~((NVIC_SYS_PRI3 << 8) & 0xFF000000U));
  prigroup = ((NVIC_AIR_CTRL >> 8) & 0x07U);
  if (prigroup >= sh) {
    sh = prigroup + 1U;
  }
  NVIC_SYS_PRI2 = ((0xFEFFFFFFU << sh) & 0xFF000000U) | (NVIC_SYS_PRI2 & 0x00FFFFFFU);

  return (sh);
#endif
}

__STATIC_INLINE
void archSwitchContextRequest(void)
{
  ICSR = PENDSVSET;
}

__STATIC_FORCEINLINE
uint32_t svc_0(uint32_t func)
{
#if defined(__CC_ARM)

  register uint32_t __r0 __ASM("r0");

  __ASM {
    SVC 0, {SVC_INDIRECT_REG=func}, {__r0=r0}
  }

  return (__r0);

#elif defined(__ICCARM__)

  _Pragma("swi_number = 0") __swi uint32_t __svc_0(void);

  SVC_FUNC(func);

  return __svc_0();

#else   // !(defined(__CC_ARM) || defined(__ICCARM__))

  register uint32_t __rf __ASM(SVC_INDIRECT_REG) = func;
  register uint32_t __r0 __ASM("r0");

  __ASM volatile ("svc 0\n"
                 :"=r"(__r0)
                 :"r"(__rf)
  );

  return (__r0);

#endif
}

__STATIC_FORCEINLINE
uint32_t svc_1(uint32_t param1, uint32_t func)
{
#if defined(__CC_ARM)

  register uint32_t __r0 __ASM("r0");

  __ASM {
    SVC 0, {SVC_INDIRECT_REG=func, r0=param1}, {__r0=r0}
  }

  return (__r0);

#elif defined(__ICCARM__)

  _Pragma("swi_number = 0") __swi uint32_t __svc_1(uint32_t);

  SVC_FUNC(func);

  return __svc_1(param1);

#else   // !(defined(__CC_ARM) || defined(__ICCARM__))

  register uint32_t __rf __ASM(SVC_INDIRECT_REG) = func;
  register uint32_t __r0 __ASM("r0") = param1;

  __ASM volatile ("svc 0\n"
                 :"=r"(__r0)
                 :"r"(__rf),"r"(__r0)
  );

  return (__r0);

#endif
}

__STATIC_FORCEINLINE
uint32_t svc_2(uint32_t param1, uint32_t param2, uint32_t func)
{
#if defined(__CC_ARM)

  register uint32_t __r0 __ASM("r0");

  __ASM {
    SVC 0, {SVC_INDIRECT_REG=func, r0=param1, r1=param2}, {__r0=r0}
  }

  return (__r0);

#elif defined(__ICCARM__)

  _Pragma("swi_number = 0") __swi uint32_t __svc_2(uint32_t, uint32_t);

  SVC_FUNC(func);

  return __svc_2(param1, param2);

#else   // !(defined(__CC_ARM) || defined(__ICCARM__))

  register uint32_t __rf __ASM(SVC_INDIRECT_REG) = func;
  register uint32_t __r0 __ASM("r0") = param1;
  register uint32_t __r1 __ASM("r1") = param2;

  __ASM volatile ("svc 0\n"
                 :"=r"(__r0)
                 :"r"(__rf),"r"(__r0),"r"(__r1)
  );

  return (__r0);

#endif
}

__STATIC_FORCEINLINE
uint32_t svc_3(uint32_t param1, uint32_t param2, uint32_t param3, uint32_t func)
{
#if defined(__CC_ARM)

  register uint32_t __r0 __ASM("r0");

  __ASM {
    SVC 0, {SVC_INDIRECT_REG=func, r0=param1, r1=param2, r2=param3}, {__r0=r0}
  }

  return (__r0);

#elif defined(__ICCARM__)

  _Pragma("swi_number = 0") __swi uint32_t __svc_3(uint32_t, uint32_t, uint32_t);

  SVC_FUNC(func);

  return __svc_3(param1, param2, param3);

#else   // !(defined(__CC_ARM) || defined(__ICCARM__))

  register uint32_t __rf __ASM(SVC_INDIRECT_REG) = func;
  register uint32_t __r0 __ASM("r0") = param1;
  register uint32_t __r1 __ASM("r1") = param2;
  register uint32_t __r2 __ASM("r2") = param3;

  __ASM volatile ("svc 0\n"
                 :"=r"(__r0)
                 :"r"(__rf),"r"(__r0),"r"(__r1),"r"(__r2)
  );

  return (__r0);

#endif
}

__STATIC_FORCEINLINE
uint32_t svc_4(uint32_t param1, uint32_t param2, uint32_t param3, uint32_t param4, uint32_t func)
{
#if defined(__CC_ARM)

  register uint32_t __r0 __ASM("r0");

  __ASM {
    SVC 0, {SVC_INDIRECT_REG=func, r0=param1, r1=param2, r2=param3, r3=param4}, {__r0=r0}
  }

  return (__r0);

#elif defined(__ICCARM__)

  _Pragma("swi_number = 0") __swi uint32_t __svc_4(uint32_t, uint32_t, uint32_t, uint32_t);

  SVC_FUNC(func);

  return __svc_4(param1, param2, param3, param4);

#else   // !(defined(__CC_ARM) || defined(__ICCARM__))

  register uint32_t __rf __ASM(SVC_INDIRECT_REG) = func;
  register uint32_t __r0 __ASM("r0") = param1;
  register uint32_t __r1 __ASM("r1") = param2;
  register uint32_t __r2 __ASM("r2") = param3;
  register uint32_t __r3 __ASM("r3") = param4;

  __ASM volatile ("svc 0\n"
                 :"=r"(__r0)
                 :"r"(__rf),"r"(__r0),"r"(__r1),"r"(__r2),"r"(__r3)
  );

  return (__r0);

#endif
}

#endif  // _ARCH_H_

/*------------------------------ End of file ---------------------------------*/

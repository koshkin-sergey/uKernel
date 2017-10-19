/*******************************************************************************
 *
 * TNKernel real-time kernel
 *
 * Copyright © 2004, 2013 Yuri Tiomkin
 * Copyright © 2014 Dmitry Frank
 * Copyright © 2013-2017 Sergey Koshkin <koshkin.sergey@gmail.com>
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

/**
 * \file
 *
 * Define macros for platform identification.
 */

#ifndef _TN_ARCH_DETECT_H
#define _TN_ARCH_DETECT_H

/*******************************************************************************
 *  defines and macros (scope: module-local)
 ******************************************************************************/

#undef __TN_ARCH_ARM7__
#undef __TN_ARCH_CORTEX_M0__
#undef __TN_ARCH_CORTEX_M3__
#undef __TN_ARCH_CORTEX_M4__
#undef __TN_ARCH_CORTEX_M4_FP__

#undef __TN_ARCHFEAT_CORTEX_M_FPU__
#undef __TN_ARCHFEAT_CORTEX_M_ARMv6M_ISA__
#undef __TN_ARCHFEAT_CORTEX_M_ARMv7M_ISA__
#undef __TN_ARCHFEAT_CORTEX_M_ARMv7EM_ISA__

#undef __TN_COMPILER_ARMCC__
#undef __TN_COMPILER_ARMCLANG__
#undef __TN_COMPILER_IAR__
#undef __TN_COMPILER_GCC__
#undef __TN_COMPILER_CLANG__


/*
 * ARM Compiler 4/5
 */
#if defined (__CC_ARM)

#  define __TN_COMPILER_ARMCC__

#  if defined(__TARGET_ARCH_4T)
#     define __TN_ARCH_ARM7__
#     define __TN_ARCHFEAT_ARM_ARMv4T_ISA__
#  elif defined(__TARGET_ARCH_6_M) || defined(__TARGET_ARCH_6S_M)
#     define __TN_ARCH_CORTEX_M0__
#     define __TN_ARCHFEAT_CORTEX_M_ARMv6M_ISA__
#  elif defined(__TARGET_ARCH_7_M)
#     define __TN_ARCH_CORTEX_M3__
#     define __TN_ARCHFEAT_CORTEX_M_ARMv6M_ISA__
#     define __TN_ARCHFEAT_CORTEX_M_ARMv7M_ISA__
#  elif defined(__TARGET_ARCH_7E_M)
#     if defined (__TARGET_FPU_VFP)
#       define __TN_ARCH_CORTEX_M4_FP__
#       define __TN_ARCHFEAT_CORTEX_M_FPU__
#     else
#       define __TN_ARCH_CORTEX_M4__
#     endif

#     define __TN_ARCHFEAT_CORTEX_M_ARMv6M_ISA__
#     define __TN_ARCHFEAT_CORTEX_M_ARMv7M_ISA__
#     define __TN_ARCHFEAT_CORTEX_M_ARMv7EM_ISA__
#  else
#     error Unknown architecture for ARMCC compiler
#  endif

/*
 * ARM Compiler 6 (armclang)
 */
#elif defined (__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)

#  define __TN_COMPILER_ARMCLANG__

#  if defined(__ARM_ARCH)
#    if defined(__ARM_ARCH_6M__)
#      define __TN_ARCH_CORTEX_M0__
#      define __TN_ARCHFEAT_CORTEX_M_ARMv6M_ISA__
#    elif defined(__ARM_ARCH_7M__)
#      define __TN_ARCH_CORTEX_M3__
#      define __TN_ARCHFEAT_CORTEX_M_ARMv6M_ISA__
#      define __TN_ARCHFEAT_CORTEX_M_ARMv7M_ISA__
#    elif defined(__ARM_ARCH_7EM__)
#      if defined(__ARM_PCS_VFP)
#        define __TN_ARCH_CORTEX_M4_FP__
#        define __TN_ARCHFEAT_CORTEX_M_FPU__
#      else
#        define __TN_ARCH_CORTEX_M4__
#      endif

#      define __TN_ARCHFEAT_CORTEX_M_ARMv6M_ISA__
#      define __TN_ARCHFEAT_CORTEX_M_ARMv7M_ISA__
#      define __TN_ARCHFEAT_CORTEX_M_ARMv7EM_ISA__
#    else
#      error Unknown ARM architecture for ARM compiler 6
#    endif
#  else
#    error Unknown architecture for ARM compiler 6
#  endif

#elif defined (__IAR_SYSTEMS_ICC__) || defined (__IAR_SYSTEMS_ASM__)

#  if defined(__ICCARM__) || defined(__IASMARM__)
#     define __TN_COMPILER_IAR__

#     if !defined(__CORE__)
#        error __CORE__ is not defined
#     endif

#     if defined(__ARM_PROFILE_M__)
#        define __TN_ARCH_CORTEX_M__
#     else
#        error IAR: the only supported core family is Cortex-M
#     endif

#     if (__CORE__ == __ARM6M__)
#        define __TN_ARCH_CORTEX_M0__
#        define __TN_ARCHFEAT_CORTEX_M_ARMv6M_ISA__
#     elif (__CORE__ == __ARM7M__)
#        define __TN_ARCH_CORTEX_M3__
#        define __TN_ARCHFEAT_CORTEX_M_ARMv6M_ISA__
#        define __TN_ARCHFEAT_CORTEX_M_ARMv7M_ISA__
#     elif (__CORE__ == __ARM7EM__)
#        if !defined(__ARMVFP__)
#           define __TN_ARCH_CORTEX_M4__
#           define __TN_ARCHFEAT_CORTEX_M_ARMv6M_ISA__
#           define __TN_ARCHFEAT_CORTEX_M_ARMv7M_ISA__
#           define __TN_ARCHFEAT_CORTEX_M_ARMv7EM_ISA__
#        else
#           define __TN_ARCH_CORTEX_M4_FP__
#           define __TN_ARCHFEAT_CORTEX_M_FPU__
#           define __TN_ARCHFEAT_CORTEX_M_ARMv6M_ISA__
#           define __TN_ARCHFEAT_CORTEX_M_ARMv7M_ISA__
#           define __TN_ARCHFEAT_CORTEX_M_ARMv7EM_ISA__
#        endif
#     else
#        error __CORE__ is unsupported
#     endif

#  else
#     error unknown compiler of IAR
#  endif

#elif defined(__GNUC__)

#  if defined(__clang__)
#     define __TN_COMPILER_CLANG__
#  else
#     define __TN_COMPILER_GCC__
#  endif

#  if defined(__ARM_ARCH)

#     define __TN_ARCH_CORTEX_M__

#     if defined(__ARM_ARCH_6M__)
#        define __TN_ARCH_CORTEX_M0__
#        define __TN_ARCHFEAT_CORTEX_M_ARMv6M_ISA__
#     elif defined(__ARM_ARCH_7M__)
#        define __TN_ARCH_CORTEX_M3__
#        define __TN_ARCHFEAT_CORTEX_M_ARMv6M_ISA__
#        define __TN_ARCHFEAT_CORTEX_M_ARMv7M_ISA__
#     elif defined(__ARM_ARCH_7EM__)
#        if defined(__SOFTFP__)
#           define __TN_ARCH_CORTEX_M4__
#           define __TN_ARCHFEAT_CORTEX_M_ARMv6M_ISA__
#           define __TN_ARCHFEAT_CORTEX_M_ARMv7M_ISA__
#           define __TN_ARCHFEAT_CORTEX_M_ARMv7EM_ISA__
#        else
#           define __TN_ARCH_CORTEX_M4_FP__
#           define __TN_ARCHFEAT_CORTEX_M_FPU__
#           define __TN_ARCHFEAT_CORTEX_M_ARMv6M_ISA__
#           define __TN_ARCHFEAT_CORTEX_M_ARMv7M_ISA__
#           define __TN_ARCHFEAT_CORTEX_M_ARMv7EM_ISA__
#        endif
#     else
#        error unknown ARM architecture for GCC compiler
#     endif
#  else
#     error unknown architecture for GCC compiler
#  endif
#else
#  error Unknown platform
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

#endif  /* _TN_ARCH_DETECT_H */

/*------------------------------ End of file ---------------------------------*/

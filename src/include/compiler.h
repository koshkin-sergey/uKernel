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

#ifndef _COMPILER_H_
#define _COMPILER_H_

/*******************************************************************************
 *  includes
 ******************************************************************************/

/*******************************************************************************
 *  defines and macros (scope: module-local)
 ******************************************************************************/

/*
 * ARM Compiler 4/5
 */
#if   defined ( __CC_ARM )
  #if defined(__ARMCC_VERSION) && (__ARMCC_VERSION < 400677)
    #error "Please use ARM Compiler Toolchain V4.0.677 or later!"
  #endif

  /* Compiler control architecture macros */
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

  /* CMSIS compiler specific defines */
  #ifndef   __ASM
    #define __ASM                                  __asm
  #endif
  #ifndef   __INLINE
    #define __INLINE                               __inline
  #endif
  #ifndef   __FORCEINLINE
    #define __FORCEINLINE                          __attribute__((always_inline))
  #endif
  #ifndef   __STATIC_INLINE
    #define __STATIC_INLINE                        static __inline
  #endif
  #ifndef   __STATIC_FORCEINLINE
    #define __STATIC_FORCEINLINE                   __attribute__((always_inline)) static __inline
  #endif
  #ifndef   __NO_RETURN
    #define __NO_RETURN                            __declspec(noreturn)
  #endif
  #ifndef   __USED
    #define __USED                                 __attribute__((used))
  #endif
  #ifndef   __WEAK
    #define __WEAK                                 __attribute__((weak))
  #endif
  #ifndef   __PACKED
    #define __PACKED                               __attribute__((packed))
  #endif
  #ifndef   __PACKED_STRUCT
    #define __PACKED_STRUCT                        __packed struct
  #endif
  #ifndef   __PACKED_UNION
    #define __PACKED_UNION                         __packed union
  #endif
  #ifndef   __UNALIGNED_UINT32        /* deprecated */
    #define __UNALIGNED_UINT32(x)                  (*((__packed uint32_t *)(x)))
  #endif
  #ifndef   __UNALIGNED_UINT16_WRITE
    #define __UNALIGNED_UINT16_WRITE(addr, val)    ((*((__packed uint16_t *)(addr))) = (val))
  #endif
  #ifndef   __UNALIGNED_UINT16_READ
    #define __UNALIGNED_UINT16_READ(addr)          (*((const __packed uint16_t *)(addr)))
  #endif
  #ifndef   __UNALIGNED_UINT32_WRITE
    #define __UNALIGNED_UINT32_WRITE(addr, val)    ((*((__packed uint32_t *)(addr))) = (val))
  #endif
  #ifndef   __UNALIGNED_UINT32_READ
    #define __UNALIGNED_UINT32_READ(addr)          (*((const __packed uint32_t *)(addr)))
  #endif
  #ifndef   __ALIGNED
    #define __ALIGNED(x)                           __attribute__((aligned(x)))
  #endif
  #ifndef   __RESTRICT
    #define __RESTRICT                             __restrict
  #endif

/*
 * ARM Compiler 6 (armclang)
 */
#elif defined (__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
  #ifndef   __ASM
    #define __ASM                                  __asm
  #endif
  #ifndef   __INLINE
    #define __INLINE                               __inline
  #endif
  #ifndef   __FORCEINLINE
    #define __FORCEINLINE                          __attribute__((always_inline))
  #endif
  #ifndef   __STATIC_INLINE
    #define __STATIC_INLINE                        static __inline
  #endif
  #ifndef   __STATIC_FORCEINLINE
    #define __STATIC_FORCEINLINE                   __attribute__((always_inline)) static __inline
  #endif
  #ifndef   __NO_RETURN
    #define __NO_RETURN                            __attribute__((__noreturn__))
  #endif
  #ifndef   __USED
    #define __USED                                 __attribute__((used))
  #endif
  #ifndef   __WEAK
    #define __WEAK                                 __attribute__((weak))
  #endif
  #ifndef   __PACKED
    #define __PACKED                               __attribute__((packed, aligned(1)))
  #endif
  #ifndef   __PACKED_STRUCT
    #define __PACKED_STRUCT                        struct __attribute__((packed, aligned(1)))
  #endif
  #ifndef   __PACKED_UNION
    #define __PACKED_UNION                         union __attribute__((packed, aligned(1)))
  #endif
  #ifndef   __UNALIGNED_UINT32        /* deprecated */
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wpacked"
  /*lint -esym(9058, T_UINT32)*/ /* disable MISRA 2012 Rule 2.4 for T_UINT32 */
    struct __attribute__((packed)) T_UINT32 { uint32_t v; };
    #pragma clang diagnostic pop
    #define __UNALIGNED_UINT32(x)                  (((struct T_UINT32 *)(x))->v)
  #endif
  #ifndef   __UNALIGNED_UINT16_WRITE
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wpacked"
  /*lint -esym(9058, T_UINT16_WRITE)*/ /* disable MISRA 2012 Rule 2.4 for T_UINT16_WRITE */
    __PACKED_STRUCT T_UINT16_WRITE { uint16_t v; };
    #pragma clang diagnostic pop
    #define __UNALIGNED_UINT16_WRITE(addr, val)    (void)((((struct T_UINT16_WRITE *)(void *)(addr))->v) = (val))
  #endif
  #ifndef   __UNALIGNED_UINT16_READ
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wpacked"
  /*lint -esym(9058, T_UINT16_READ)*/ /* disable MISRA 2012 Rule 2.4 for T_UINT16_READ */
    __PACKED_STRUCT T_UINT16_READ { uint16_t v; };
    #pragma clang diagnostic pop
    #define __UNALIGNED_UINT16_READ(addr)          (((const struct T_UINT16_READ *)(const void *)(addr))->v)
  #endif
  #ifndef   __UNALIGNED_UINT32_WRITE
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wpacked"
  /*lint -esym(9058, T_UINT32_WRITE)*/ /* disable MISRA 2012 Rule 2.4 for T_UINT32_WRITE */
    __PACKED_STRUCT T_UINT32_WRITE { uint32_t v; };
    #pragma clang diagnostic pop
    #define __UNALIGNED_UINT32_WRITE(addr, val)    (void)((((struct T_UINT32_WRITE *)(void *)(addr))->v) = (val))
  #endif
  #ifndef   __UNALIGNED_UINT32_READ
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wpacked"
  /*lint -esym(9058, T_UINT32_READ)*/ /* disable MISRA 2012 Rule 2.4 for T_UINT32_READ */
    __PACKED_STRUCT T_UINT32_READ { uint32_t v; };
    #pragma clang diagnostic pop
    #define __UNALIGNED_UINT32_READ(addr)          (((const struct T_UINT32_READ *)(const void *)(addr))->v)
  #endif
  #ifndef   __ALIGNED
    #define __ALIGNED(x)                           __attribute__((aligned(x)))
  #endif
  #ifndef   __RESTRICT
    #define __RESTRICT                             __restrict
  #endif

/*
 * GNU Compiler
 */
#elif defined ( __GNUC__ )
  #ifndef   __ASM
    #define __ASM                                  __asm
  #endif
  #ifndef   __INLINE
    #define __INLINE                               inline
  #endif
  #ifndef   __FORCEINLINE
    #define __FORCEINLINE                          __attribute__((always_inline))
  #endif
  #ifndef   __STATIC_INLINE
    #define __STATIC_INLINE                        static inline
  #endif
  #ifndef   __STATIC_FORCEINLINE
    #define __STATIC_FORCEINLINE                   __attribute__((always_inline)) static inline
  #endif
  #ifndef   __NO_RETURN
    #define __NO_RETURN                            __attribute__((__noreturn__))
  #endif
  #ifndef   __USED
    #define __USED                                 __attribute__((used))
  #endif
  #ifndef   __WEAK
    #define __WEAK                                 __attribute__((weak))
  #endif
  #ifndef   __PACKED
    #define __PACKED                               __attribute__((packed, aligned(1)))
  #endif
  #ifndef   __PACKED_STRUCT
    #define __PACKED_STRUCT                        struct __attribute__((packed, aligned(1)))
  #endif
  #ifndef   __PACKED_UNION
    #define __PACKED_UNION                         union __attribute__((packed, aligned(1)))
  #endif
  #ifndef   __UNALIGNED_UINT32        /* deprecated */
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpacked"
    #pragma GCC diagnostic ignored "-Wattributes"
    struct __attribute__((packed)) T_UINT32 { uint32_t v; };
    #pragma GCC diagnostic pop
    #define __UNALIGNED_UINT32(x)                  (((struct T_UINT32 *)(x))->v)
  #endif
  #ifndef   __UNALIGNED_UINT16_WRITE
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpacked"
    #pragma GCC diagnostic ignored "-Wattributes"
    __PACKED_STRUCT T_UINT16_WRITE { uint16_t v; };
    #pragma GCC diagnostic pop
    #define __UNALIGNED_UINT16_WRITE(addr, val)    (void)((((struct T_UINT16_WRITE *)(void *)(addr))->v) = (val))
  #endif
  #ifndef   __UNALIGNED_UINT16_READ
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpacked"
    #pragma GCC diagnostic ignored "-Wattributes"
    __PACKED_STRUCT T_UINT16_READ { uint16_t v; };
    #pragma GCC diagnostic pop
    #define __UNALIGNED_UINT16_READ(addr)          (((const struct T_UINT16_READ *)(const void *)(addr))->v)
  #endif
  #ifndef   __UNALIGNED_UINT32_WRITE
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpacked"
    #pragma GCC diagnostic ignored "-Wattributes"
    __PACKED_STRUCT T_UINT32_WRITE { uint32_t v; };
    #pragma GCC diagnostic pop
    #define __UNALIGNED_UINT32_WRITE(addr, val)    (void)((((struct T_UINT32_WRITE *)(void *)(addr))->v) = (val))
  #endif
  #ifndef   __UNALIGNED_UINT32_READ
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpacked"
    #pragma GCC diagnostic ignored "-Wattributes"
    __PACKED_STRUCT T_UINT32_READ { uint32_t v; };
    #pragma GCC diagnostic pop
    #define __UNALIGNED_UINT32_READ(addr)          (((const struct T_UINT32_READ *)(const void *)(addr))->v)
  #endif
  #ifndef   __ALIGNED
    #define __ALIGNED(x)                           __attribute__((aligned(x)))
  #endif
  #ifndef   __RESTRICT
    #define __RESTRICT                             __restrict
  #endif

/*
 * IAR Compiler
 */
#elif defined ( __ICCARM__ )


/*
 * TI ARM Compiler
 */
#elif defined ( __TI_ARM__ )
  #ifndef   __ASM
    #define __ASM                                  __asm
  #endif
  #ifndef   __INLINE
    #define __INLINE                               inline
  #endif
  #ifndef   __STATIC_INLINE
    #define __STATIC_INLINE                        static inline
  #endif
  #ifndef   __NO_RETURN
    #define __NO_RETURN                            __attribute__((noreturn))
  #endif
  #ifndef   __USED
    #define __USED                                 __attribute__((used))
  #endif
  #ifndef   __WEAK
    #define __WEAK                                 __attribute__((weak))
  #endif
  #ifndef   __PACKED
    #define __PACKED                               __attribute__((packed))
  #endif
  #ifndef   __PACKED_STRUCT
    #define __PACKED_STRUCT                        struct __attribute__((packed))
  #endif
  #ifndef   __PACKED_UNION
    #define __PACKED_UNION                         union __attribute__((packed))
  #endif
  #ifndef   __UNALIGNED_UINT32        /* deprecated */
    struct __attribute__((packed)) T_UINT32 { uint32_t v; };
    #define __UNALIGNED_UINT32(x)                  (((struct T_UINT32 *)(x))->v)
  #endif
  #ifndef   __UNALIGNED_UINT16_WRITE
    __PACKED_STRUCT T_UINT16_WRITE { uint16_t v; };
    #define __UNALIGNED_UINT16_WRITE(addr, val)    (void)((((struct T_UINT16_WRITE *)(void*)(addr))->v) = (val))
  #endif
  #ifndef   __UNALIGNED_UINT16_READ
    __PACKED_STRUCT T_UINT16_READ { uint16_t v; };
    #define __UNALIGNED_UINT16_READ(addr)          (((const struct T_UINT16_READ *)(const void *)(addr))->v)
  #endif
  #ifndef   __UNALIGNED_UINT32_WRITE
    __PACKED_STRUCT T_UINT32_WRITE { uint32_t v; };
    #define __UNALIGNED_UINT32_WRITE(addr, val)    (void)((((struct T_UINT32_WRITE *)(void *)(addr))->v) = (val))
  #endif
  #ifndef   __UNALIGNED_UINT32_READ
    __PACKED_STRUCT T_UINT32_READ { uint32_t v; };
    #define __UNALIGNED_UINT32_READ(addr)          (((const struct T_UINT32_READ *)(const void *)(addr))->v)
  #endif
  #ifndef   __ALIGNED
    #define __ALIGNED(x)                           __attribute__((aligned(x)))
  #endif
  #ifndef   __RESTRICT
    #warning No compiler specific solution for __RESTRICT. __RESTRICT is ignored.
    #define __RESTRICT
  #endif

/*
 * TASKING Compiler
 */
#elif defined ( __TASKING__ )
  /*
   * The CMSIS functions have been implemented as intrinsics in the compiler.
   * Please use "carm -?i" to get an up to date list of all intrinsics,
   * Including the CMSIS ones.
   */

  #ifndef   __ASM
    #define __ASM                                  __asm
  #endif
  #ifndef   __INLINE
    #define __INLINE                               inline
  #endif
  #ifndef   __STATIC_INLINE
    #define __STATIC_INLINE                        static inline
  #endif
  #ifndef   __NO_RETURN
    #define __NO_RETURN                            __attribute__((noreturn))
  #endif
  #ifndef   __USED
    #define __USED                                 __attribute__((used))
  #endif
  #ifndef   __WEAK
    #define __WEAK                                 __attribute__((weak))
  #endif
  #ifndef   __PACKED
    #define __PACKED                               __packed__
  #endif
  #ifndef   __PACKED_STRUCT
    #define __PACKED_STRUCT                        struct __packed__
  #endif
  #ifndef   __PACKED_UNION
    #define __PACKED_UNION                         union __packed__
  #endif
  #ifndef   __UNALIGNED_UINT32        /* deprecated */
    struct __packed__ T_UINT32 { uint32_t v; };
    #define __UNALIGNED_UINT32(x)                  (((struct T_UINT32 *)(x))->v)
  #endif
  #ifndef   __UNALIGNED_UINT16_WRITE
    __PACKED_STRUCT T_UINT16_WRITE { uint16_t v; };
    #define __UNALIGNED_UINT16_WRITE(addr, val)    (void)((((struct T_UINT16_WRITE *)(void *)(addr))->v) = (val))
  #endif
  #ifndef   __UNALIGNED_UINT16_READ
    __PACKED_STRUCT T_UINT16_READ { uint16_t v; };
    #define __UNALIGNED_UINT16_READ(addr)          (((const struct T_UINT16_READ *)(const void *)(addr))->v)
  #endif
  #ifndef   __UNALIGNED_UINT32_WRITE
    __PACKED_STRUCT T_UINT32_WRITE { uint32_t v; };
    #define __UNALIGNED_UINT32_WRITE(addr, val)    (void)((((struct T_UINT32_WRITE *)(void *)(addr))->v) = (val))
  #endif
  #ifndef   __UNALIGNED_UINT32_READ
    __PACKED_STRUCT T_UINT32_READ { uint32_t v; };
    #define __UNALIGNED_UINT32_READ(addr)          (((const struct T_UINT32_READ *)(const void *)(addr))->v)
  #endif
  #ifndef   __ALIGNED
    #define __ALIGNED(x)              __align(x)
  #endif
  #ifndef   __RESTRICT
    #warning No compiler specific solution for __RESTRICT. __RESTRICT is ignored.
    #define __RESTRICT
  #endif

/*
 * COSMIC Compiler
 */
#elif defined ( __CSMC__ )
 #ifndef   __ASM
    #define __ASM                                  _asm
  #endif
  #ifndef   __INLINE
    #define __INLINE                               inline
  #endif
  #ifndef   __STATIC_INLINE
    #define __STATIC_INLINE                        static inline
  #endif
  #ifndef   __NO_RETURN
    // NO RETURN is automatically detected hence no warning here
    #define __NO_RETURN
  #endif
  #ifndef   __USED
    #warning No compiler specific solution for __USED. __USED is ignored.
    #define __USED
  #endif
  #ifndef   __WEAK
    #define __WEAK                                 __weak
  #endif
  #ifndef   __PACKED
    #define __PACKED                               @packed
  #endif
  #ifndef   __PACKED_STRUCT
    #define __PACKED_STRUCT                        @packed struct
  #endif
  #ifndef   __PACKED_UNION
    #define __PACKED_UNION                         @packed union
  #endif
  #ifndef   __UNALIGNED_UINT32        /* deprecated */
    @packed struct T_UINT32 { uint32_t v; };
    #define __UNALIGNED_UINT32(x)                  (((struct T_UINT32 *)(x))->v)
  #endif
  #ifndef   __UNALIGNED_UINT16_WRITE
    __PACKED_STRUCT T_UINT16_WRITE { uint16_t v; };
    #define __UNALIGNED_UINT16_WRITE(addr, val)    (void)((((struct T_UINT16_WRITE *)(void *)(addr))->v) = (val))
  #endif
  #ifndef   __UNALIGNED_UINT16_READ
    __PACKED_STRUCT T_UINT16_READ { uint16_t v; };
    #define __UNALIGNED_UINT16_READ(addr)          (((const struct T_UINT16_READ *)(const void *)(addr))->v)
  #endif
  #ifndef   __UNALIGNED_UINT32_WRITE
    __PACKED_STRUCT T_UINT32_WRITE { uint32_t v; };
    #define __UNALIGNED_UINT32_WRITE(addr, val)    (void)((((struct T_UINT32_WRITE *)(void *)(addr))->v) = (val))
  #endif
  #ifndef   __UNALIGNED_UINT32_READ
    __PACKED_STRUCT T_UINT32_READ { uint32_t v; };
    #define __UNALIGNED_UINT32_READ(addr)          (((const struct T_UINT32_READ *)(const void *)(addr))->v)
  #endif
  #ifndef   __ALIGNED
    #warning No compiler specific solution for __ALIGNED. __ALIGNED is ignored.
    #define __ALIGNED(x)
  #endif
  #ifndef   __RESTRICT
    #warning No compiler specific solution for __RESTRICT. __RESTRICT is ignored.
    #define __RESTRICT
  #endif

#else
  #define __ASM
  #define __INLINE
  #define __FORCEINLINE
  #define __STATIC_INLINE
  #define __STATIC_FORCEINLINE
  #define __NO_RETURN
  #define __USED
  #define __WEAK
  #define __PACKED
  #define __PACKED_STRUCT
  #define __PACKED_UNION
  #define __UNALIGNED_UINT32(x)
  #define __UNALIGNED_UINT16_WRITE(addr, val)
  #define __UNALIGNED_UINT16_READ(addr)
  #define __UNALIGNED_UINT32_WRITE(addr, val)
  #define __UNALIGNED_UINT32_READ(addr)
  #define __ALIGNED(x)
  #define __RESTRICT

  #error Unknown compiler.
#endif

#endif /* _COMPILER_H_ */

/* ----------------------------- End of file ---------------------------------*/

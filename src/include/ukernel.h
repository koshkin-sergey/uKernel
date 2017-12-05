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

#ifndef _UKERNEL_H_
#define _UKERNEL_H_

/*******************************************************************************
 *  includes
 ******************************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>

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

#define osStack_t     __attribute__((aligned(8), section("STACK"), zero_init)) uint32_t

/* - The system configuration (change it for your particular project) --------*/
#define USE_MUTEXES           1
#define USE_EVENTS            1

/* - Constants ---------------------------------------------------------------*/
#define osTaskStarOnCreating            1

#define TN_EVENT_ATTR_SINGLE            1
#define TN_EVENT_ATTR_MULTI             2
#define TN_EVENT_ATTR_CLR               4

#define TN_EVENT_WCOND_OR               8
#define TN_EVENT_WCOND_AND           0x10

#define osMutexPrioCeiling            (1UL<<0)
#define osMutexRecursive              (1UL<<1)

#define TIME_WAIT_INFINITE             (0xFFFFFFFF)

#define NO_TIME_SLICE                  (0)
#define MAX_TIME_SLICE            (0xFFFE)

#define time_after(a,b)      ((int32_t)(b) - (int32_t)(a) < 0)
#define time_before(a,b)     time_after(b,a)

#define time_after_eq(a,b)   ((int32_t)(a) - (int32_t)(b) >= 0)
#define time_before_eq(a,b)  time_after_eq(b,a)


/*******************************************************************************
 *  typedefs and structures (scope: module-local)
 ******************************************************************************/

typedef enum {
  ID_INVALID        = 0x00000000,
  ID_TASK           = 0x47ABCF69,
  ID_SEMAPHORE      = 0x6FA173EB,
  ID_EVENT          = 0x5E224F25,
  ID_DATAQUEUE      = 0x0C8A6C89,
  ID_FSMEMORYPOOL   = 0x26B7CE8B,
  ID_MUTEX          = 0x17129E45,
  ID_RENDEZVOUS     = 0x74289EBD,
  ID_ALARM          = 0x7A5762BC,
  ID_CYCLIC         = 0x2B8F746B,
  ID_MESSAGE_QUEUE  = 0x1C9A6C89,
} id_t;

/// Error code values returned by uKernel functions.
typedef enum {
  TERR_TRUE         =  1,
  TERR_NO_ERR       =  0,
  TERR_OVERFLOW     = -1,         ///< OOV
  TERR_WCONTEXT     = -2,         ///< Wrong context error
  TERR_WSTATE       = -3,         ///< Wrong state error
  TERR_TIMEOUT      = -4,         ///< Polling failure or timeout
  TERR_WRONG_PARAM  = -5,
  TERR_UNDERFLOW    = -6,
  TERR_OUT_OF_MEM   = -7,
  TERR_ILUSE        = -8,         ///< Illegal using
  TERR_NOEXS        = -9,         ///< Non-valid or Non-existent object
  TERR_DLT          = -10,        ///< Waiting object deleted
  TERR_ISR          = -11,
  osErrorReserved   = 0x7FFFFFFF  ///< Prevents enum down-size compiler optimization.
} osError_t;

typedef enum {
  TASK_EXIT,
  TASK_EXIT_AND_DELETE,
} task_exit_attr_t;

/* Task states */
typedef enum {
  TSK_STATE_RUNNABLE  = 0x01,
  TSK_STATE_WAIT      = 0x02,
  TSK_STATE_SUSPEND   = 0x04,
  TSK_STATE_DORMANT   = 0x08,
  task_state_reserved = 0x7FFFFFFF  ///< Prevents enum down-size compiler optimization.
} task_state_t;

/* Task Wait Reason */
typedef enum {
  WAIT_REASON_NO            = 0x0000,
  WAIT_REASON_SLEEP         = 0x0001,
  WAIT_REASON_SEM           = 0x0002,
  WAIT_REASON_EVENT         = 0x0004,
  WAIT_REASON_DQUE_WSEND    = 0x0008,
  WAIT_REASON_DQUE_WRECEIVE = 0x0010,
  WAIT_REASON_MUTEX_C       = 0x0020,
  WAIT_REASON_MUTEX_I       = 0x0040,
  WAIT_REASON_MQUE_WSEND    = 0x0080,
  WAIT_REASON_MQUE_WRECEIVE = 0x0100,
  WAIT_REASON_WFIXMEM       = 0x0200,
  wait_reason_reserved      = 0x7fffffff
} wait_reason_t;

typedef uint32_t osTime_t;

typedef void (*CBACK)(void *);

/* - Circular double-linked list queue - for internal using ------------------*/
typedef struct _CDLL_QUEUE {
  struct _CDLL_QUEUE *next;
  struct _CDLL_QUEUE *prev;
} CDLL_QUEUE;

typedef struct timer_event_block {
  CDLL_QUEUE queue; /**< Timer event queue */
  uint32_t time;    /**< Event time */
  CBACK callback;   /**< Callback function */
  void *arg;        /**< Argument to be sent to callback function */
} TMEB;

/* - Message Queue -----------------------------------------------------------*/
typedef struct osMessageQueue_s {
  id_t id;                // Message buffer ID
  CDLL_QUEUE send_queue;  // Message buffer send wait queue
  CDLL_QUEUE recv_queue;  // Message buffer receive wait queue
  uint8_t *buf;           // Message buffer address
  uint32_t msg_size;      // Message size in bytes
  uint32_t num_entries;   // Capacity of data_fifo(num entries)
  uint32_t cnt;           // Number of queued messages
  uint32_t tail;          // Next to the last message store address
  uint32_t head;          // First message store address
} osMessageQueue_t;

typedef enum {
  osMsgPriorityLow        = 0,
  osMsgPriorityHigh       = 1,
  osMsgPriority_reserved  = 0x7fffffff,
} osMsgPriority_t;


typedef struct {
  void **data_elem;
} WINFO_RDQUE;

typedef struct {
  void *data_elem;
  bool send_to_first;
} WINFO_SDQUE;

typedef struct {
  void *data_elem;
} WINFO_FMEM;

/*
 * Message buffer receive/send wait (TTW_RMBF, TTW_SMBF)
 */
typedef struct {
  void *msg; /* Address that has a received message */
} WINFO_RMQUE;

typedef struct {
  const void *msg; /* Send message head address */
  osMsgPriority_t msg_pri;
} WINFO_SMQUE;

typedef struct {
  unsigned int pattern;   // Event wait pattern
  int mode;               // Event wait mode:  _AND or _OR
  unsigned int *flags_pattern;
} WINFO_EVENT;

/*
 * Definition of wait information in task control block
 */
typedef union {
  WINFO_RDQUE rdque;
  WINFO_SDQUE sdque;
  WINFO_RMQUE rmque;
  WINFO_SMQUE smque;
  WINFO_FMEM fmem;
  WINFO_EVENT event;
} WINFO;

/* - Task Control Block ------------------------------------------------------*/
typedef struct osTask_s {
  uint32_t stk;               ///< Address of task's top of stack
  CDLL_QUEUE task_queue;      ///< Queue is used to include task in ready/wait lists
  CDLL_QUEUE *pwait_queue;    ///< Ptr to object's(semaphor,event,etc.) wait list
#ifdef USE_MUTEXES
  CDLL_QUEUE mutex_queue;     ///< List of all mutexes that tack locked
#endif
  uint32_t *stk_start;        ///< Base address of task's stack space
  uint32_t stk_size;          ///< Task's stack size (in sizeof(void*),not bytes)
  const void *func_addr;      ///< filled on creation
  const void *func_param;     ///< filled on creation
  uint32_t base_priority;     ///< Task base priority
  uint32_t priority;          ///< Task current priority
  id_t id;                    ///< ID for verification(is it a task or another object?)
  task_state_t state;         ///< Task state
  wait_reason_t wait_reason;  ///< Reason for waiting
  WINFO wait_info;            ///< Wait information
  TMEB wait_timer;            ///< Wait timer
  int tslice_count;           ///< Time slice counter
  osTime_t time;              ///< Time work task
} osTask_t;

/* - Semaphore ---------------------------------------------------------------*/
typedef struct osSemaphore_s {
  CDLL_QUEUE wait_queue;
  uint32_t count;
  uint32_t max_count;
  id_t id;                    ///< ID for verification(is it a semaphore or another object?)
} osSemaphore_t;

/* - Eventflag ---------------------------------------------------------------*/
typedef struct _TN_EVENT {
  CDLL_QUEUE wait_queue;
  int attr;                   //-- Eventflag attribute
  unsigned int pattern;       //-- Initial value of the eventflag bit pattern
  id_t id;                    //-- ID for verification(is it a event or another object?)
} TN_EVENT;

/* - Data queue --------------------------------------------------------------*/
typedef struct _TN_DQUE {
  CDLL_QUEUE wait_send_list;
  CDLL_QUEUE wait_receive_list;
  void **data_fifo;        //-- Array of void* to store data queue entries
  int num_entries;        //-- Capacity of data_fifo(num entries)
  int cnt;                // Кол-во данных в очереди
  int tail_cnt;           //-- Counter to processing data queue's Array of void*
  int header_cnt;         //-- Counter to processing data queue's Array of void*
  id_t id;                //-- ID for verification(is it a data queue or another object?)
} TN_DQUE;

/* - Fixed-sized blocks memory pool ------------------------------------------*/
typedef struct _TN_FMP {
  CDLL_QUEUE wait_queue;
  unsigned int block_size;   //-- Actual block size (in bytes)
  int num_blocks;   //-- Capacity (Fixed-sized blocks actual max qty)
  void *start_addr;  //-- Memory pool actual start address
  void *free_list;   //-- Ptr to free block list
  int fblkcnt;      //-- Num of free blocks
  id_t id;          //-- ID for verification(is it a fixed-sized blocks memory pool or another object?)
} TN_FMP;

/* - Mutex -------------------------------------------------------------------*/

typedef struct osMutexAttr_s {
  uint32_t attr_bits;     /**< Creation attribute bits. The following predefined
                               bit masks can be assigned to set options for a mutex object:
                                 osMutexPrioCeiling − Mutex uses the priority ceiling protocol;
                                                      Default mutex uses the priority inheritance protocol;
                                 osMutexRecursive - Mutex is recursive. */
  uint32_t ceil_priority; /**< Valid only when the osMutexPrioCeiling bit is set.
                               The ceil_priority parameter should be set to
                               the maximum priority of the task(s) that may lock the mutex. */
} osMutexAttr_t;

typedef struct osMutex_s {
  id_t id;                ///< ID for verification(is it a mutex or another object?)
  CDLL_QUEUE wait_que;    ///< List of tasks that wait a mutex
  CDLL_QUEUE mutex_que;   ///< To include in task's locked mutexes list (if any)
  uint32_t attr;          ///< Mutex creation attr - CEILING or INHERIT
  osTask_t *holder;       ///< Current mutex owner(task that locked mutex)
  uint32_t ceil_priority; ///< When mutex created with CEILING attr
  int cnt;                ///< Reserved
} osMutex_t;


/* - Alarm Timer -------------------------------------------------------------*/

typedef enum {
  TIMER_STOP            = 0x00,
  TIMER_START           = 0x01,
  timer_state_reserved  = 0x7fffffff
} timer_state_t;

typedef struct _TN_ALARM {
  void *exinf;              /**< Extended information */
  CBACK handler;            /**< Alarm handler address */
  timer_state_t state;      /**< Timer state */
  TMEB timer;               /**< Timer event block */
  id_t id;                  /**< ID for verification */
} TN_ALARM;

/* Cyclic attributes */
typedef enum {
  CYCLIC_ATTR_NO        = 0x00,
  CYCLIC_ATTR_START     = 0x01,
  CYCLIC_ATTR_PHS       = 0x02,
  cyclic_attr_reserved  = 0x7FFFFFFF  ///< Prevents enum down-size compiler optimization.
} cyclic_attr_t;

typedef struct {
  uint32_t cyc_time;
  uint32_t cyc_phs;
  cyclic_attr_t cyc_attr;
} cyclic_param_t;

/* - Cyclic Timer ------------------------------------------------------------*/
typedef struct _TN_CYCLIC {
  void *exinf;              /**< Extended information */
  CBACK handler;            /**< Cyclic handler address */
  timer_state_t state;      /**< Timer state */
  cyclic_attr_t attr;       /**< Cyclic handler attributes */
  uint32_t time;            /**< Cyclic time */
  TMEB timer;               /**< Timer event block */
  id_t id;                  /**< ID for verification */
} TN_CYCLIC;


/* - User functions ----------------------------------------------------------*/
typedef void (*TN_USER_FUNC)(void);

typedef struct {
  TN_USER_FUNC app_init;
  uint32_t freq_timer;
  uint32_t max_syscall_interrupt_priority;
} TN_OPTIONS;

/*******************************************************************************
 *  exported variables
 ******************************************************************************/

/*******************************************************************************
 *  exported function prototypes
 ******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/* - system.c ----------------------------------------------------------------*/
__NO_RETURN
void osKernelStart(TN_OPTIONS *opt);

#if defined(ROUND_ROBIN_ENABLE)
int tn_sys_tslice_ticks(int priority, int value);
#endif

/* - Timer -------------------------------------------------------------------*/

void osTimerHandle(void);

osTime_t osGetTickCount(void);

osError_t osAlarmCreate(TN_ALARM *alarm, CBACK handler, void *exinf);

osError_t osAlarmDelete(TN_ALARM *alarm);

osError_t osAlarmStart(TN_ALARM *alarm, osTime_t timeout);

osError_t osAlarmStop(TN_ALARM *alarm);

osError_t osCyclicCreate(TN_CYCLIC *cyc, CBACK handler, const cyclic_param_t *param, void *exinf);

osError_t osCyclicDelete(TN_CYCLIC *cyc);

osError_t osCyclicStart(TN_CYCLIC *cyc);

osError_t osCyclicStop(TN_CYCLIC *cyc);

/* - Task --------------------------------------------------------------------*/

/**
 * @fn          osError_t osTaskCreate(osTask_t *task, void (*func)(void *), int32_t priority, const uint32_t *stack_start, int32_t stack_size, const void *param, int32_t option)
 * @brief       Creates a task.
 * @param[out]  task          Pointer to the task TCB to be created
 * @param[in]   func          Task body function
 * @param[in]   priority      Task priority. User tasks may have priorities 1…30
 * @param[in]   stack_start   Task's stack bottom address
 * @param[in]   stack_size    Task's stack size – number of task stack elements (not bytes)
 * @param[in]   param         task_func parameter. param will be passed to task_func on creation time
 * @param[in]   option        Creation option. Option values:
 *                              0                           After creation task has a DORMANT state
 *                              osTaskStarOnCreating   After creation task is switched to the runnable state (READY/RUNNING)
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_ISR          The function cannot be called from interrupt service routines
 */
osError_t osTaskCreate(osTask_t *task, void (*func)(void *), uint32_t priority, const uint32_t *stack_start, uint32_t stack_size, const void *param, uint32_t option);

/**
 * @fn          osError_t osTaskDelete(osTask_t *task)
 * @brief       Deletes the task specified by the task.
 * @param[out]  task  Pointer to the task TCB to be deleted
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_WCONTEXT     Unacceptable system's state for this request
 *              TERR_NOEXS        Object is not a task or non-existent
 *              TERR_ISR          The function cannot be called from interrupt service routines
 */
osError_t osTaskDelete(osTask_t *task);

/**
 * @fn          osError_t osTaskActivate(osTask_t *task)
 * @brief       Activates a task specified by the task
 * @param[out]  task  Pointer to the task TCB to be activated
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_OVERFLOW     Task is already active (not in DORMANT state)
 *              TERR_NOEXS        Object is not a task or non-existent
 *              TERR_ISR          The function cannot be called from interrupt service routines
 */
osError_t osTaskActivate(osTask_t *task);

/**
 * @fn          osError_t osTaskTerminate(osTask_t *task)
 * @brief       Terminates the task specified by the task
 * @param[out]  task  Pointer to the task TCB to be terminated
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_WCONTEXT     Unacceptable system state for this request
 *              TERR_NOEXS        Object is not a task or non-existent
 *              TERR_ISR          The function cannot be called from interrupt service routines
 */
osError_t osTaskTerminate(osTask_t *task);

/**
 * @fn        void osTaskExit(task_exit_attr_t attr)
 * @brief     Terminates the currently running task
 * @param[in] attr  Exit option. Option values:
 *                    TASK_EXIT             Currently running task will be terminated.
 *                    TASK_EXIT_AND_DELETE  Currently running task will be terminated and then deleted
 */
void osTaskExit(task_exit_attr_t attr);

/**
 * @fn          osError_t osTaskSuspend(osTask_t *task)
 * @brief       Suspends the task specified by the task
 * @param[out]  task  Pointer to the task TCB to be suspended
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_OVERFLOW     Task already suspended
 *              TERR_WSTATE       Task is not active (i.e in DORMANT state )
 *              TERR_NOEXS        Object is not a task or non-existent
 *              TERR_ISR          The function cannot be called from interrupt service routines
 */
osError_t osTaskSuspend(osTask_t *task);

/**
 * @fn          osError_t osTaskResume(osTask_t *task)
 * @brief       Releases the task specified by the task from the SUSPENDED state
 * @param[out]  task  Pointer to task TCB to be resumed
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_WSTATE       Task is not in SUSPEDED or WAITING_SUSPEDED state
 *              TERR_NOEXS        Object is not a task or non-existent
 *              TERR_ISR          The function cannot be called from interrupt service routines
 */
osError_t osTaskResume(osTask_t *task);

/**
 * @fn        osError_t osTaskSleep(osTime_t timeout)
 * @brief     Puts the currently running task to the sleep for at most timeout system ticks.
 * @param[in] timeout   Timeout value must be greater than 0.
 *                      A value of TIME_WAIT_INFINITE causes an infinite delay.
 * @return    TERR_NO_ERR       Normal completion
 *            TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *            TERR_NOEXS        Object is not a task or non-existent
 *            TERR_ISR          The function cannot be called from interrupt service routines
 */
osError_t osTaskSleep(osTime_t timeout);

/**
 * @fn          osError_t osTaskWakeup(osTask_t *task)
 * @brief       Wakes up the task specified by the task from sleep mode
 * @param[out]  task  Pointer to the task TCB to be wake up
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_NOEXS        Object is not a task or non-existent
 *              TERR_WSTATE       Task is not in WAIT state
 *              TERR_ISR          The function cannot be called from interrupt service routines
 */
osError_t osTaskWakeup(osTask_t *task);

/**
 * @fn          osError_t osTaskReleaseWait(osTask_t *task)
 * @brief       Forcibly releases the task specified by the task from waiting
 * @param[out]  task  Pointer to the task TCB to be released from waiting or sleep
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_WCONTEXT     Unacceptable system's state for function's request executing
 *              TERR_NOEXS        Object is not a task or non-existent
 *              TERR_ISR          The function cannot be called from interrupt service routines
 */
osError_t osTaskReleaseWait(osTask_t *task);

osError_t osTaskSetPriority(osTask_t *task, uint32_t new_priority);

osTime_t osTaskGetTime(osTask_t *task);

/* - Semaphore ---------------------------------------------------------------*/
/**
 * @fn          osError_t osSemaphoreNew(osSemaphore_t *sem, uint32_t initial_count, uint32_t max_count)
 * @brief       Creates a semaphore
 * @param[out]  sem             Pointer to the semaphore structure to be created
 * @param[in]   initial_count   Initial number of available tokens
 * @param[in]   max_count       Maximum number of available tokens
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_ISR          The function cannot be called from interrupt service routines
 */
osError_t osSemaphoreNew(osSemaphore_t *sem, uint32_t initial_count, uint32_t max_count);

/**
 * @fn          osError_t osSemaphoreDelete(osSemaphore_t *sem)
 * @brief       Deletes a semaphore
 * @param[out]  sem   Pointer to the semaphore structure to be deleted
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_NOEXS        Object is not a semaphore or non-existent
 *              TERR_ISR          The function cannot be called from interrupt service routines
 */
osError_t osSemaphoreDelete(osSemaphore_t *sem);

/**
 * @fn          osError_t osSemaphoreRelease(osSemaphore_t *sem)
 * @brief       Release a Semaphore token up to the initial maximum count.
 * @param[out]  sem   Pointer to the semaphore structure to be released
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_OVERFLOW     Semaphore Resource has max_count value
 *              TERR_NOEXS        Object is not a semaphore or non-existent
 */
osError_t osSemaphoreRelease(osSemaphore_t *sem);

/**
 * @fn          osError_t osSemaphoreAcquire(osSemaphore_t *sem, osTime_t timeout)
 * @brief       Acquire a Semaphore token or timeout if no tokens are available.
 * @param[out]  sem       Pointer to the semaphore structure to be acquired
 * @param[in]   timeout   Timeout value must be equal or greater than 0
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_TIMEOUT      Timeout expired
 *              TERR_NOEXS        Object is not a semaphore or non-existent
 */
osError_t osSemaphoreAcquire(osSemaphore_t *sem, osTime_t timeout);

/**
 * @fn          uint32_t osSemaphoreGetCount(osSemaphore_t *sem)
 * @brief       Returns the number of available tokens of the semaphore object
 * @param[out]  sem   Pointer to the semaphore structure to be acquired
 * @return      Number of tokens available or 0 in case of an error
 */
uint32_t osSemaphoreGetCount(osSemaphore_t *sem);


/* - tn_dqueue.c -------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*
 * Название : tn_queue_create
 * Описание : Создает очередь данных. Поле id структуры TN_DQUE предварительно
 *            должно быть установлено в 0.
 * Параметры: dque  - Указатель на существующую структуру TN_DQUE.
 *            data_fifo - Указатель на существующий массив void *.
 *                        Может быть равен 0.
 *            num_entries - Емкость очереди. Может быть равна 0, тогда задачи
 *                          общаются через эту очередь в синхронном режиме.
 * Результат: Возвращает один из вариантов:
 *              TERR_NO_ERR - функция выполнена без ошибок;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *----------------------------------------------------------------------------*/
osError_t tn_queue_create(TN_DQUE *dque, void **data_fifo, int num_entries);

/*-----------------------------------------------------------------------------*
 * Название : tn_queue_delete
 * Описание : Удаляет очередь данных.
 * Параметры: dque  - Указатель на существующую структуру TN_DQUE.
 * Результат: Возвращает один из вариантов:
 *              TERR_NO_ERR - функция выполнена без ошибок;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_NOEXS  - очередь не существует;
 *----------------------------------------------------------------------------*/
osError_t tn_queue_delete(TN_DQUE *dque);

/*-----------------------------------------------------------------------------*
 * Название : tn_queue_send
 * Описание : Помещает данные в конец очереди за установленный интервал времени.
 * Параметры: dque  - Дескриптор очереди данных.
 *            data_ptr  - Указатель на данные.
 *            timeout - Время, в течении которого данные должны быть помещены
 *                      в очередь.
 * Результат: Возвращает один из вариантов:
 *              TERR_NO_ERR - функция выполнена без ошибок;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_NOEXS  - очередь не существует;
 *              TERR_TIMEOUT  - Превышен заданный интервал времени;
 *----------------------------------------------------------------------------*/
osError_t tn_queue_send(TN_DQUE *dque, void *data_ptr, unsigned long timeout);

/*-----------------------------------------------------------------------------*
 * Название : tn_queue_send_first
 * Описание : Помещает данные в начало очереди за установленный интервал времени.
 * Параметры: dque  - Дескриптор очереди данных.
 *            data_ptr  - Указатель на данные.
 *            timeout - Время, в течении которого данные должны быть помещены
 *                      в очередь.
 * Результат: Возвращает один из вариантов:
 *              TERR_NO_ERR - функция выполнена без ошибок;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_NOEXS  - очередь не существует;
 *              TERR_TIMEOUT  - Превышен заданный интервал времени;
 *----------------------------------------------------------------------------*/
osError_t tn_queue_send_first(TN_DQUE *dque, void *data_ptr, unsigned long timeout);

/*-----------------------------------------------------------------------------*
 * Название : tn_queue_receive
 * Описание : Считывает один элемент данных из начала очереди за установленный
 *            интервал времени.
 * Параметры: dque  - Дескриптор очереди данных.
 *            data_ptr  - Указатель на буфер, куда будут считаны данные.
 *            timeout - Время, в течении которого данные должны быть считаны.
 * Результат: Возвращает один из вариантов:
 *              TERR_NO_ERR - функция выполнена без ошибок;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_NOEXS  - очередь не существует;
 *              TERR_TIMEOUT  - Превышен заданный интервал времени;
 *----------------------------------------------------------------------------*/
osError_t tn_queue_receive(TN_DQUE *dque, void **data_ptr, unsigned long timeout);

/*-----------------------------------------------------------------------------*
 * Название : tn_queue_flush
 * Описание : Сбрасывает очередь данных.
 * Параметры: dque  - Указатель на очередь данных.
 * Результат: Возвращает один из вариантов:
 *              TERR_NO_ERR - функция выполнена без ошибок;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_NOEXS  - очередь данных не была создана.
 *----------------------------------------------------------------------------*/
osError_t tn_queue_flush(TN_DQUE *dque);

/*-----------------------------------------------------------------------------*
 * Название : tn_queue_empty
 * Описание : Проверяет очередь данных на пустоту.
 * Параметры: dque  - Указатель на очередь данных.
 * Результат: Возвращает один из вариантов:
 *              TERR_TRUE - очередь данных пуста;
 *              TERR_NO_ERR - в очереди данные есть;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_NOEXS  - очередь данных не была создана.
 *----------------------------------------------------------------------------*/
osError_t tn_queue_empty(TN_DQUE *dque);

/*-----------------------------------------------------------------------------*
 * Название : tn_queue_full
 * Описание : Проверяет очередь данных на полное заполнение.
 * Параметры: dque  - Указатель на очередь данных.
 * Результат: Возвращает один из вариантов:
 *              TERR_TRUE - очередь данных полная;
 *              TERR_NO_ERR - очередь данных не полная;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_NOEXS  - очередь данных не была создана.
 *----------------------------------------------------------------------------*/
osError_t tn_queue_full(TN_DQUE *dque);

/*-----------------------------------------------------------------------------*
 * Название : tn_queue_cnt
 * Описание : Функция возвращает количество элементов в очереди данных.
 * Параметры: dque  - Дескриптор очереди данных.
 *            cnt - Указатель на ячейку памяти, в которую будет возвращено
 *                  количество элементов.
 * Результат: Возвращает один из вариантов:
 *              TERR_NO_ERR - функция выполнена без ошибок;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_NOEXS  - очередь данных не была создана.
 *----------------------------------------------------------------------------*/
osError_t tn_queue_cnt(TN_DQUE *dque, int *cnt);

/* - Message Queue -----------------------------------------------------------*/

/**
 * @fn          osError_t osMessageQueueNew(osMessageQueue_t *mq, void *buf, uint32_t bufsz, uint32_t msz)
 * @brief       Creates and initializes a message queue object
 * @param[out]  mq      Pointer to the osMessageQueue_t structure
 * @param[out]  buf     Pointer to buffer for message
 * @param[in]   bufsz   Buffer size in bytes
 * @param[in]   msz     Maximum message size in bytes
 * @return      TERR_NO_ERR       The message queue object has been created
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_NOEXS        Object is not a Message Queue or non-existent
 *              TERR_ISR          Cannot be called from interrupt service routines
 */
osError_t osMessageQueueNew(osMessageQueue_t *mq, void *buf, uint32_t bufsz, uint32_t msz);

/**
 * @fn          osError_t osMessageQueueDelete(osMessageQueue_t *mq)
 * @brief       Deletes a message queue object
 * @param[out]  mq  Pointer to the osMessageQueue_t structure
 * @return      TERR_NO_ERR       The message queue object has been deleted
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_NOEXS        Object is not a Message Queue or non-existent
 *              TERR_ISR          Cannot be called from interrupt service routines
 */
osError_t osMessageQueueDelete(osMessageQueue_t *mq);

/**
 * @fn          osError_t osMessageQueuePut(osMessageQueue_t *mq, const void *msg, osMsgPriority_t msg_pri, osTime_t timeout)
 * @brief       Puts the message into the the message queue
 * @param[out]  mq        Pointer to the osMessageQueue_t structure
 * @param[in]   msg       Pointer to buffer with message to put into a queue
 * @param[in]   msg_pri   Message priority
 * @param[in]   timeout   Timeout Value or 0 in case of no time-out
 * @return      TERR_NO_ERR       The message has been put into the queue
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_NOEXS        Object is not a Message Queue or non-existent
 *              TERR_TIMEOUT      The message could not be put into the queue in the given time
 */
osError_t osMessageQueuePut(osMessageQueue_t *mq, const void *msg, osMsgPriority_t msg_pri, osTime_t timeout);

/**
 * @fn          osError_t osMessageQueueGet(osMessageQueue_t *mq, void *msg, osTime_t timeout)
 * @brief       Retrieves a message from the message queue and saves it to the buffer
 * @param[out]  mq        Pointer to the osMessageQueue_t structure
 * @param[out]  msg       Pointer to buffer for message to get from a queue
 * @param[in]   timeout   Timeout Value or 0 in case of no time-out
 * @return      TERR_NO_ERR       The message has been retrieved from the queue
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_NOEXS        Object is not a Message Queue or non-existent
 *              TERR_TIMEOUT      The message could not be retrieved from the queue in the given time
 */
osError_t osMessageQueueGet(osMessageQueue_t *mq, void *msg, osTime_t timeout);

/**
 * @fn          uint32_t osMessageQueueGetMsgSize(osMessageQueue_t *mq)
 * @brief       Returns the maximum message size in bytes for the message queue
 * @param[out]  mq  Pointer to the osMessageQueue_t structure
 * @return      Maximum message size in bytes or 0 in case of an error
 */
uint32_t osMessageQueueGetMsgSize(osMessageQueue_t *mq);

/**
 * @fn          uint32_t osMessageQueueGetCapacity(osMessageQueue_t *mq)
 * @brief       Returns the maximum number of messages in the message queue
 * @param[out]  mq  Pointer to the osMessageQueue_t structure
 * @return      Maximum number of messages or 0 in case of an error
 */
uint32_t osMessageQueueGetCapacity(osMessageQueue_t *mq);

/**
 * @fn          uint32_t osMessageQueueGetCount(osMessageQueue_t *mq)
 * @brief       Returns the number of queued messages in the message queue
 * @param[out]  mq  Pointer to the osMessageQueue_t structure
 * @return      Number of queued messages or 0 in case of an error
 */
uint32_t osMessageQueueGetCount(osMessageQueue_t *mq);

/**
 * @fn          uint32_t osMessageQueueGetSpace(osMessageQueue_t *mq)
 * @brief       Returns the number available slots for messages in the message queue
 * @param[out]  mq  Pointer to the osMessageQueue_t structure
 * @return      Number of available slots for messages or 0 in case of an error
 */
uint32_t osMessageQueueGetSpace(osMessageQueue_t *mq);

/**
 * @fn          osError_t osMessageQueueReset(osMessageQueue_t *mq)
 * @brief       Resets the message queue
 * @param[out]  mq  Pointer to the osMessageQueue_t structure
 * @return      TERR_NO_ERR       The message queue has been reset
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_NOEXS        Object is not a Message Queue or non-existent
 *              TERR_ISR          Cannot be called from interrupt service routines
 */
osError_t osMessageQueueReset(osMessageQueue_t *mq);


/* - tn_event.c --------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*
 * Название : tn_event_create
 * Описание : Создает флаг события.
 * Параметры: evf - Указатель на инициализируемую структуру TN_EVENT
 *            attr  - Атрибуты создаваемого флага.
 *                    Возможно сочетание следующих определений:
 *                    TN_EVENT_ATTR_CLR - Выполнять автоматическую очистку флага
 *                                        после его обработки. Возможно
 *                                        применение только совместно с атрибутом
 *                                        TN_EVENT_ATTR_SINGLE.
 *                    TN_EVENT_ATTR_SINGLE  - Использование флага только в одной
 *                                            задаче. Исрользование в нескольких
 *                                            задачах не допускается.
 *                    TN_EVENT_ATTR_MULTI - Использование флага возможно в
 *                                          нескольких задачах.
 *                    Атрибуты TN_EVENT_ATTR_SINGLE и TN_EVENT_ATTR_MULTI
 *                    взаимно исключающие. Не допускается использовать их
 *                    одновременно, но также не допускается вообще не указывать
 *                    ни один из этих атрибутов.
 *            pattern - Начальное битовое поле по которому идет определение
 *                      установки флага. Обычно должно быть равно 0.
 * Результат: Возвращает TERR_NO_ERR если выполнено без ошибок, в противном
 *            случае TERR_WRONG_PARAM
 *----------------------------------------------------------------------------*/
extern osError_t tn_event_create(TN_EVENT *evf, int attr, unsigned int pattern);

/*-----------------------------------------------------------------------------*
 * Название : tn_event_delete
 * Описание : Удаляет флаг события.
 * Параметры: evf - Указатель на существующую структуру TN_EVENT.
 * Результат: Возвращает один из вариантов:
 *              TERR_NO_ERR - функция выполнена без ошибок;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_NOEXS  - флаг события не существует;
 *----------------------------------------------------------------------------*/
extern osError_t tn_event_delete(TN_EVENT *evf);

/*-----------------------------------------------------------------------------*
 * Название : tn_event_wait
 * Описание : Ожидает установки флага события в течении заданного интервала
 *            времени.
 * Параметры: evf - Указатель на существующую структуру TN_EVENT.
 *            wait_pattern  - Ожидаемая комбинация битов. Не может быть равно 0.
 *            wait_mode - Режим ожидания.
 *                        Возможно одно из определений:
 *                          TN_EVENT_WCOND_OR - Ожидается установка любого бита
 *                                              из ожидаемых.
 *                          TN_EVENT_WCOND_AND  - Ожидается установка всех битов
 *                                                из ожидаемых.
 *            p_flags_pattern - Указатель на переменную, в которую будет записано
 *                              значение комбинации битов по окончании ожидания.
 *            timeout - Время ожидания установки флагов событий.
 * Результат: Возвращает один из вариантов:
 *              TERR_NO_ERR - функция выполнена без ошибок;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_NOEXS  - флаг события не существует;
 *              TERR_ILUSE  - флаг был создан с атрибутом TN_EVENT_ATTR_SINGLE,
 *                            а его пытается использовать более одной задачи.
 *              TERR_TIMEOUT  - Время ожидания истекло.
 *----------------------------------------------------------------------------*/
extern osError_t tn_event_wait(TN_EVENT *evf, unsigned int wait_pattern,
                         int wait_mode, unsigned int *p_flags_pattern,
                         unsigned long timeout);

/*-----------------------------------------------------------------------------*
 * Название : tn_event_set
 * Описание : Устанавливает флаги событий.
 * Параметры: evf - Указатель на существующую структуру TN_EVENT.
 *            pattern - Комбинация битов. 1 в разряде соответствует
 *                      устанавливаемому флагу. Не может быть равен 0.
 * Результат: Возвращает один из вариантов:
 *              TERR_NO_ERR - функция выполнена без ошибок;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_NOEXS  - флаг события не существует;
 *----------------------------------------------------------------------------*/
extern osError_t tn_event_set(TN_EVENT *evf, unsigned int pattern);

/*-----------------------------------------------------------------------------*
 * Название : tn_event_clear
 * Описание : Очищает флаг события.
 * Параметры: evf - Указатель на существующую структуру TN_EVENT.
 *            pattern - Комбинация битов. 1 в разряде соответствует
 *                      очищаемому флагу. Не может быть равен 0.
 * Результат: Возвращает один из вариантов:
 *              TERR_NO_ERR - функция выполнена без ошибок;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_NOEXS  - флаг события не существует;
 *----------------------------------------------------------------------------*/
extern osError_t tn_event_clear(TN_EVENT *evf, unsigned int pattern);

/* - tn_mem.c ----------------------------------------------------------------*/
osError_t tn_fmem_create(TN_FMP *fmp, void *start_addr, unsigned int block_size,
                   int num_blocks);
osError_t tn_fmem_delete(TN_FMP *fmp);
osError_t tn_fmem_get(TN_FMP *fmp, void **p_data, unsigned long timeout);
osError_t tn_fmem_release(TN_FMP *fmp, void *p_data);

/* - Mutex Management --------------------------------------------------------*/

/**
 * @fn          osError_t osMutexNew(osMutex_t *mutex, const osMutexAttr_t *attr)
 * @brief       Creates and initializes a new mutex object
 * @param[out]  mutex   Pointer to osMutex_t structure of the mutex
 * @param[in]   attr    Sets the mutex object attributes (refer to osMutexAttr_t).
 *                      Default attributes will be used if set to NULL.
 * @return      TERR_NO_ERR       The mutex object has been created
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_ISR          Cannot be called from interrupt service routines
 */
osError_t osMutexNew(osMutex_t *mutex, const osMutexAttr_t *attr);

/**
 * @fn          osError_t osMutexDelete(osMutex_t *mutex)
 * @brief       Deletes a mutex object
 * @param[out]  mutex   Pointer to osMutex_t structure of the mutex
 * @return      TERR_NO_ERR       The mutex object has been deleted
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_ISR          Cannot be called from interrupt service routines
 */
osError_t osMutexDelete(osMutex_t *mutex);

/**
 * @fn          osError_t osMutexAcquire(osMutex_t *mutex, osTime_t timeout)
 * @brief       Waits until a mutex object becomes available
 * @param[out]  mutex     Pointer to osMutex_t structure of the mutex
 * @param[in]   timeout   Timeout Value or 0 in case of no time-out. Specifies
 *                        how long the system waits to acquire the mutex.
 * @return      TERR_NO_ERR       The mutex has been obtained
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_TIMEOUT      The mutex could not be obtained in the given time
 *              TERR_ILUSE        Illegal usage, e.g. trying to acquire already obtained mutex
 *              TERR_ISR          Cannot be called from interrupt service routines
 */
osError_t osMutexAcquire(osMutex_t *mutex, osTime_t timeout);

/**
 * @fn          osError_t osMutexRelease(osMutex_t *mutex)
 * @brief       Releases a mutex
 * @param[out]  mutex     Pointer to osMutex_t structure of the mutex
 * @return      TERR_NO_ERR       The mutex has been correctly released
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_ILUSE        Illegal usage, e.g. trying to release already free mutex
 *              TERR_ISR          Cannot be called from interrupt service routines
 */
osError_t osMutexRelease(osMutex_t *mutex);

/**
 * @fn          osTask_t* osMutexGetOwner(osMutex_t *mutex)
 * @brief       Returns the pointer to the task that acquired a mutex. In case
 *              of an error or if the mutex is not blocked by any task, it returns NULL.
 * @param[out]  mutex     Pointer to osMutex_t structure of the mutex
 * @return      Pointer to owner task or NULL when mutex was not acquired
 */
osTask_t* osMutexGetOwner(osMutex_t *mutex);


/* - tn_delay.c --------------------------------------------------------------*/
void tn_mdelay(unsigned long ms);
void tn_udelay(unsigned long usecs);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  // _UKERNEL_H_

/*------------------------------ End of file ---------------------------------*/
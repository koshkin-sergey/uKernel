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

#ifdef  __cplusplus
extern "C"
{
#endif

/*******************************************************************************
 *  defines and macros (scope: module-local)
 ******************************************************************************/

#if   defined (__CC_ARM)
  #pragma push
  #pragma anon_unions
#elif defined (__ICCARM__)
  #pragma language=extended
#elif defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wc11-extensions"
  #pragma clang diagnostic ignored "-Wreserved-id-macro"
#elif defined (__GNUC__)
  /* anonymous unions are enabled by default */
#elif defined (__TMS470__)
  /* anonymous unions are enabled by default */
#elif defined (__TASKING__)
  #pragma warning 586
#elif defined (__CSMC__)
  /* anonymous unions are enabled by default */
#else
  #warning Not supported compiler type
#endif

#ifndef __NO_RETURN
#if   defined(__CC_ARM)
#define __NO_RETURN __declspec(noreturn)
#elif defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
#define __NO_RETURN __attribute__((__noreturn__))
#elif defined(__GNUC__)
#define __NO_RETURN __attribute__((__noreturn__))
#elif defined(__ICCARM__)
#define __NO_RETURN __noreturn
#else
#define __NO_RETURN
#endif
#endif

#define osStackSizeMin                (24U)

#define osTaskStartOnCreating         (1U)

#define osFlagsWaitAny                0x00000000U ///< Wait for any flag (default).
#define osFlagsWaitAll                0x00000001U ///< Wait for all flags.
#define osFlagsNoClear                0x00000002U ///< Do not clear flags which have been specified to wait for.

/* Mutex attributes */
#define osMutexPrioInherit            (1UL<<0)  ///< Priority inherit protocol.
#define osMutexRecursive              (1UL<<1)  ///< Recursive mutex.
#define osMutexRobust                 (1UL<<2)  ///< Robust mutex.

#define TIME_WAIT_INFINITE            (0xFFFFFFFF)

#define NO_TIME_SLICE                 (0)
#define MAX_TIME_SLICE                (0xFFFE)

#define time_after(a,b)               ((int32_t)(b) - (int32_t)(a) < 0)
#define time_before(a,b)              time_after(b,a)

#define time_after_eq(a,b)            ((int32_t)(a) - (int32_t)(b) >= 0)
#define time_before_eq(a,b)           time_after_eq(b,a)

/*******************************************************************************
 *  typedefs and structures (scope: module-local)
 ******************************************************************************/

typedef enum {
  ID_INVALID        = 0x00000000,//!< ID_INVALID
  ID_TASK           = 0x47ABCF69,//!< ID_TASK
  ID_SEMAPHORE      = 0x6FA173EB,//!< ID_SEMAPHORE
  ID_EVENT_FLAGS    = 0x5E224F25,//!< ID_EVENT_FLAGS
  ID_DATAQUEUE      = 0x0C8A6C89,//!< ID_DATAQUEUE
  ID_FSMEMORYPOOL   = 0x26B7CE8B,//!< ID_FSMEMORYPOOL
  ID_MUTEX          = 0x17129E45,//!< ID_MUTEX
  ID_RENDEZVOUS     = 0x74289EBD,//!< ID_RENDEZVOUS
  ID_ALARM          = 0x7A5762BC,//!< ID_ALARM
  ID_CYCLIC         = 0x2B8F746B,//!< ID_CYCLIC
  ID_MESSAGE_QUEUE  = 0x1C9A6C89,//!< ID_MESSAGE_QUEUE
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
  TERR_WAIT         = -12,
  osErrorReserved   = 0x7FFFFFFF  ///< Prevents enum down-size compiler optimization.
} osError_t;

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
  WAIT_REASON_MUTEX         = 0x0020,
  WAIT_REASON_MUTEX_I       = 0x0040,
  WAIT_REASON_MQUE_WSEND    = 0x0080,
  WAIT_REASON_MQUE_WRECEIVE = 0x0100,
  WAIT_REASON_WFIXMEM       = 0x0200,
  wait_reason_reserved      = 0x7fffffff
} wait_reason_t;

typedef uint32_t osTime_t;

typedef void (*CBACK)(void *);

/* - Circular double-linked list queue - for internal using ------------------*/
typedef struct queue_s {
  struct queue_s *next;
  struct queue_s *prev;
} queue_t;

typedef struct timer_event_block {
  queue_t timer_que;      /**< Timer event queue */
  uint32_t time;          /**< Event time */
  CBACK callback;         /**< Callback function */
  void *arg;              /**< Argument to be sent to callback function */
} timer_t;

/* - Message Queue -----------------------------------------------------------*/
typedef struct osMessageQueue_s {
  id_t id;                // Message buffer ID
  queue_t send_queue;     // Message buffer send wait queue
  queue_t recv_queue;     // Message buffer receive wait queue
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
  uint32_t flags;
  uint32_t options;
} WINFO_EVENT;

/*
 * Definition of wait information in task control block
 */
typedef struct winfo_s {
  union {
    WINFO_RDQUE rdque;
    WINFO_SDQUE sdque;
    WINFO_RMQUE rmque;
    WINFO_SMQUE smque;
    WINFO_FMEM fmem;
    WINFO_EVENT event;
  };
  uint32_t ret_val;
} wait_info;

/* - Task Control Block ------------------------------------------------------*/
typedef struct osTask_s {
  uint32_t stk;               ///< Address of task's top of stack
  queue_t task_que;           ///< Queue is used to include task in ready/wait lists
  queue_t *pwait_que;         ///< Ptr to object's(semaphor,event,etc.) wait list
  queue_t mutex_que;          ///< List of all mutexes that tack locked
  uint32_t *stk_start;        ///< Base address of task's stack space
  uint32_t stk_size;          ///< Task's stack size (in sizeof(void*),not bytes)
  const void *func_addr;      ///< filled on creation
  const void *func_param;     ///< filled on creation
  uint32_t base_priority;     ///< Task base priority
  uint32_t priority;          ///< Task current priority
  id_t id;                    ///< ID for verification(is it a task or another object?)
  task_state_t state;         ///< Task state
  wait_reason_t wait_reason;  ///< Reason for waiting
  wait_info wait_info;        ///< Wait information
  timer_t wait_timer;         ///< Wait timer
  int tslice_count;           ///< Time slice counter
  osTime_t time;              ///< Time work task
} osTask_t;

/* - Semaphore ---------------------------------------------------------------*/
typedef struct osSemaphore_s {
  queue_t wait_queue;
  uint32_t count;
  uint32_t max_count;
  id_t id;                    ///< ID for verification(is it a semaphore or another object?)
} osSemaphore_t;

/* - Event Flags -------------------------------------------------------------*/
typedef struct _TN_EVENT {
  id_t id;                    ///< ID for verification(is it a event or another object?)
  queue_t wait_queue;
  uint32_t pattern;           ///< Initial value of the eventflag bit pattern
} osEventFlags_t;

/* - Data queue --------------------------------------------------------------*/
typedef struct _TN_DQUE {
  queue_t wait_send_list;
  queue_t wait_receive_list;
  void **data_fifo;        //-- Array of void* to store data queue entries
  int num_entries;        //-- Capacity of data_fifo(num entries)
  int cnt;                // Кол-во данных в очереди
  int tail_cnt;           //-- Counter to processing data queue's Array of void*
  int header_cnt;         //-- Counter to processing data queue's Array of void*
  id_t id;                //-- ID for verification(is it a data queue or another object?)
} TN_DQUE;

/* - Fixed-sized blocks memory pool ------------------------------------------*/
typedef struct _TN_FMP {
  queue_t wait_queue;
  unsigned int block_size;   //-- Actual block size (in bytes)
  int num_blocks;   //-- Capacity (Fixed-sized blocks actual max qty)
  void *start_addr;  //-- Memory pool actual start address
  void *free_list;   //-- Ptr to free block list
  int fblkcnt;      //-- Num of free blocks
  id_t id;          //-- ID for verification(is it a fixed-sized blocks memory pool or another object?)
} TN_FMP;


/* - Mutex -------------------------------------------------------------------*/

/* Attributes structure for mutex */
typedef struct osMutexAttr_s {
  uint32_t attr_bits;     ///< attribute bits
} osMutexAttr_t;

/* Mutex Control Block */
typedef struct osMutex_s {
  id_t id;                ///< ID for verification(is it a mutex or another object?)
  queue_t wait_que;       ///< List of tasks that wait a mutex
  queue_t mutex_que;      ///< To include in task's locked mutexes list (if any)
  uint32_t attr;          ///< Mutex creation attributes
  osTask_t *holder;       ///< Current mutex owner(task that locked mutex)
  uint32_t cnt;           ///< Lock counter
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
  timer_t timer;               /**< Timer event block */
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
  timer_t timer;               /**< Timer event block */
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
 * @fn          osError_t osTaskCreate(osTask_t *task, void (*func)(void *), int32_t priority, const uint64_t *stack_start, int32_t stack_size, const void *param, int32_t option)
 * @brief       Creates a task.
 * @param[out]  task          Pointer to the task TCB to be created
 * @param[in]   func          Task body function
 * @param[in]   priority      Task priority. User tasks may have priorities 1…30
 * @param[in]   stack_start   Task's stack bottom address
 * @param[in]   stack_size    Task's stack size – number of task stack elements (not bytes)
 * @param[in]   param         task_func parameter. param will be passed to task_func on creation time
 * @param[in]   option        Creation option. Option values:
 *                              0                           After creation task has a DORMANT state
 *                              osTaskStartOnCreating   After creation task is switched to the runnable state (READY/RUNNING)
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_ISR          The function cannot be called from interrupt service routines
 */
osError_t osTaskCreate(osTask_t *task, void (*func)(void *), uint32_t priority, const uint64_t *stack_start, uint32_t stack_size, const void *param, uint32_t option);

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
 * @fn          void osTaskExit(void)
 * @brief       Terminates the currently running task
 */
__NO_RETURN
void osTaskExit(void);

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
 * @brief       Wakes up the task specified by the task from sleep options
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


/* - Event Flags -------------------------------------------------------------*/

/**
 * @fn          osError_t osEventFlagsNew(osEventFlags_t *evf)
 * @brief       Creates a new event flags object
 * @param[out]  evf   Pointer to osEventFlags_t structure of the event
 * @return      TERR_NO_ERR       The event flags object has been created
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_ISR          Cannot be called from interrupt service routines
 */
osError_t osEventFlagsNew(osEventFlags_t *evf);

/**
 * @fn          osError_t osEventFlagsDelete(osEventFlags_t *evf)
 * @brief       Deletes the event flags object
 * @param[out]  evf   Pointer to osEventFlags_t structure of the event
 * @return      TERR_NO_ERR       The event flags object has been deleted
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_NOEXS        Object is not a Message Queue or non-existent
 *              TERR_ISR          Cannot be called from interrupt service routines
 */
osError_t osEventFlagsDelete(osEventFlags_t *evf);

/**
 * @fn          uint32_t osEventFlagsSet(osEventFlags_t *evf, uint32_t flags)
 * @brief       Sets the event flags
 * @param[out]  evf   Pointer to osEventFlags_t structure of the event
 * @param[in]   flags Specifies the flags that shall be set
 * @return      The event flags after setting or an error code if highest bit is set
 *              (refer to osError_t)
 */
uint32_t osEventFlagsSet(osEventFlags_t *evf, uint32_t flags);

/**
 * @fn          uint32_t osEventFlagsWait(osEventFlags_t *evf, uint32_t flags, uint32_t options, osTime_t timeout)
 * @brief       Suspends the execution of the currently RUNNING task until any
 *              or all event flags in the event object are set. When these event
 *              flags are already set, the function returns instantly.
 * @param[out]  evf       Pointer to osEventFlags_t structure of the event
 * @param[in]   flags     Specifies the flags to wait for
 * @param[in]   options   Specifies flags options (osFlagsXxxx)
 * @param[in]   timeout   Timeout Value or 0 in case of no time-out
 * @return      Event flags before clearing or error code if highest bit set
 */
uint32_t osEventFlagsWait(osEventFlags_t *evf, uint32_t flags, uint32_t options, osTime_t timeout);

/**
 * @fn          uint32_t osEventFlagsClear(osEventFlags_t *evf, uint32_t flags)
 * @brief       Clears the event flags in an event flags object
 * @param[out]  evf     Pointer to osEventFlags_t structure of the event
 * @param[in]   flags   Specifies the flags that shall be cleared
 * @return      Event flags before clearing or error code if highest bit set
 */
uint32_t osEventFlagsClear(osEventFlags_t *evf, uint32_t flags);


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


#if   defined (__CC_ARM)
  #pragma pop
#elif defined (__ICCARM__)
  /* leave anonymous unions enabled */
#elif (defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050))
  #pragma clang diagnostic pop
#elif defined (__GNUC__)
  /* anonymous unions are enabled by default */
#elif defined (__TMS470__)
  /* anonymous unions are enabled by default */
#elif defined (__TASKING__)
  #pragma warning restore
#elif defined (__CSMC__)
  /* anonymous unions are enabled by default */
#else
  #warning Not supported compiler type
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  // _UKERNEL_H_

/*------------------------------ End of file ---------------------------------*/

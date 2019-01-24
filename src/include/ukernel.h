/*
 * Copyright (C) 2017-2019 Sergey Koshkin <koshkin.sergey@gmail.com>
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

#define osFlagsWaitAny                0x00000000U ///< Wait for any flag (default).
#define osFlagsWaitAll                0x00000001U ///< Wait for all flags.
#define osFlagsNoClear                0x00000002U ///< Do not clear flags which have been specified to wait for.

/* Mutex attributes */
#define osMutexPrioInherit            (1UL<<0)  ///< Priority inherit protocol.
#define osMutexRecursive              (1UL<<1)  ///< Recursive mutex.
#define osMutexRobust                 (1UL<<2)  ///< Robust mutex.

#define osWaitForever                 (0xFFFFFFFF)

#define NO_TIME_SLICE                 (0)
#define MAX_TIME_SLICE                (0xFFFE)

#define time_after(a,b)               ((int32_t)(b) - (int32_t)(a) < 0)
#define time_before(a,b)              time_after(b,a)

#define time_after_eq(a,b)            ((int32_t)(a) - (int32_t)(b) >= 0)
#define time_before_eq(a,b)           time_after_eq(b,a)

/*******************************************************************************
 *  typedefs and structures (scope: module-local)
 ******************************************************************************/

/// Status code values returned by CMSIS-RTOS functions.
typedef enum {
  osOK                    =  0,         ///< Operation completed successfully.
  osError                 = -1,         ///< Unspecified RTOS error: run-time error but no other error message fits.
  osErrorTimeout          = -2,         ///< Operation not completed within the timeout period.
  osErrorResource         = -3,         ///< Resource not available.
  osErrorParameter        = -4,         ///< Parameter error.
  osErrorNoMemory         = -5,         ///< System is out of memory: it was impossible to allocate or reserve memory for the operation.
  osErrorISR              = -6,         ///< Not allowed in ISR context: the function cannot be called from interrupt service routines.
  osStatusReserved        = 0x7FFFFFFF  ///< Prevents enum down-size compiler optimization.
} osStatus_t;

/// Priority values.
typedef enum {
  osPriorityNone          =  0,         ///< No priority (not initialized).
  osPriorityIdle          =  1,         ///< Reserved for Idle thread.
  osPriorityLow           =  4,         ///< Priority: low
  osPriorityLow1          =  4+1,       ///< Priority: low + 1
  osPriorityLow2          =  4+2,       ///< Priority: low + 2
  osPriorityLow3          =  4+3,       ///< Priority: low + 3
  osPriorityBelowNormal   =  8,         ///< Priority: below normal
  osPriorityBelowNormal1  =  8+1,       ///< Priority: below normal + 1
  osPriorityBelowNormal2  =  8+2,       ///< Priority: below normal + 2
  osPriorityBelowNormal3  =  8+3,       ///< Priority: below normal + 3
  osPriorityNormal        = 12,         ///< Priority: normal
  osPriorityNormal1       = 12+1,       ///< Priority: normal + 1
  osPriorityNormal2       = 12+2,       ///< Priority: normal + 2
  osPriorityNormal3       = 12+3,       ///< Priority: normal + 3
  osPriorityAboveNormal   = 16,         ///< Priority: above normal
  osPriorityAboveNormal1  = 16+1,       ///< Priority: above normal + 1
  osPriorityAboveNormal2  = 16+2,       ///< Priority: above normal + 2
  osPriorityAboveNormal3  = 16+3,       ///< Priority: above normal + 3
  osPriorityHigh          = 20,         ///< Priority: high
  osPriorityHigh1         = 20+1,       ///< Priority: high + 1
  osPriorityHigh2         = 20+2,       ///< Priority: high + 2
  osPriorityHigh3         = 20+3,       ///< Priority: high + 3
  osPriorityRealtime      = 24,         ///< Priority: realtime
  osPriorityRealtime1     = 24+1,       ///< Priority: realtime + 1
  osPriorityRealtime2     = 24+2,       ///< Priority: realtime + 2
  osPriorityRealtime3     = 24+3,       ///< Priority: realtime + 3
  osPriorityISR           = 32,         ///< Reserved for ISR deferred thread.
  osPriorityError         = -1,         ///< System cannot determine priority or illegal priority.
  osPriorityReserved      = 0x7FFFFFFF  ///< Prevents enum down-size compiler optimization.
} osPriority_t;

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

/// Thread state.
typedef enum {
  osThreadInactive        =  0,         ///< Inactive.
  osThreadReady           =  1,         ///< Ready.
  osThreadRunning         =  2,         ///< Running.
  osThreadBlocked         =  3,         ///< Blocked.
  osThreadTerminated      =  4,         ///< Terminated.
  osThreadError           = -1,         ///< Error.
  osThreadReserved        = 0x7FFFFFFF  ///< Prevents enum down-size compiler optimization.
} osThreadState_t;

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
  uint8_t                          id;  ///< Object Identifier
  uint8_t              reserved_state;  ///< Object State (not used)
  uint8_t                       flags;  ///< Object Flags
  uint8_t                        attr;  ///< Object Attributes
  queue_t                  send_queue;  ///< Message buffer send wait queue
  queue_t                  recv_queue;  ///< Message buffer receive wait queue
  uint8_t                        *buf;  ///< Message buffer address
  uint32_t                   msg_size;  ///< Message size in bytes
  uint32_t                num_entries;  ///< Capacity of data_fifo(num entries)
  uint32_t                        cnt;  ///< Number of queued messages
  uint32_t                       tail;  ///< Next to the last message store address
  uint32_t                       head;  ///< First message store address
} osMessageQueue_t;

typedef enum {
  osMsgPriorityLow        = 0,
  osMsgPriorityHigh       = 1,
  osMsgPriority_reserved  = 0x7fffffff,
} osMsgPriority_t;

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
    WINFO_RMQUE rmque;
    WINFO_SMQUE smque;
    WINFO_FMEM  fmem;
    WINFO_EVENT event;
  };
  uint32_t ret_val;
} wait_info;

/// @details Thread ID identifies the thread.
typedef void *osThreadId_t;

/// @details Event Flags ID identifies the event flags.
typedef void *osEventFlagsId_t;

/// \details Semaphore ID identifies the semaphore.
typedef void *osSemaphoreId_t;

/// \details Message Queue ID identifies the message queue.
typedef void *osMessageQueueId_t;

/// Entry point of a thread.
typedef void (*osThreadFunc_t) (void *argument);

/* - Task Control Block ------------------------------------------------------*/
typedef struct osThread_s {
  uint32_t                        stk;  ///< Address of task's top of stack
  queue_t                    task_que;  ///< Queue is used to include task in ready/wait lists
  queue_t                   mutex_que;  ///< List of all mutexes that tack locked
  void                       *stk_mem;  ///< Base address of task's stack space
  uint32_t                   stk_size;  ///< Task's stack size (in bytes)
  int8_t                base_priority;  ///< Task base priority
  int8_t                     priority;  ///< Task current priority
  uint8_t                          id;  ///< ID for verification(is it a task or another object?)
  uint8_t                       state;  ///< Task state
  const char                    *name;  ///< Object Name
  wait_info                 wait_info;  ///< Wait information
  timer_t                  wait_timer;  ///< Wait timer
  int32_t                tslice_count;  ///< Time slice counter
} osThread_t;

/* - Semaphore ---------------------------------------------------------------*/
typedef struct osSemaphore_s {
  uint8_t                          id;  ///< Object Identifier
  uint8_t              reserved_state;  ///< Object State (not used)
  uint8_t                       flags;  ///< Object Flags
  uint8_t                    reserved;
  const char                    *name;  ///< Object Name
  queue_t                  wait_queue;  ///< Waiting Threads queue
  uint16_t                      count;  ///< Current number of tokens
  uint16_t                  max_count;  ///< Maximum number of tokens
} osSemaphore_t;

/* - Event Flags -------------------------------------------------------------*/
typedef struct osEventFlags_s {
  uint8_t                          id;  ///< Object Identifier
  uint8_t              reserved_state;  ///< Object State (not used)
  uint8_t                       flags;  ///< Object Flags
  uint8_t                    reserved;
  const char                    *name;  ///< Object Name
  queue_t                  wait_queue;  ///< Waiting Threads queue
  uint32_t                event_flags;  ///< Initial value of the eventflag bit pattern
} osEventFlags_t;

/* - Fixed-sized blocks memory pool ------------------------------------------*/
typedef struct _TN_FMP {
  uint8_t                          id;  ///< Object Identifier
  uint8_t              reserved_state;  ///< Object State (not used)
  uint8_t                       flags;  ///< Object Flags
  uint8_t                        attr;  ///< Object Attributes
  queue_t                  wait_queue;
  unsigned int             block_size;  ///< Actual block size (in bytes)
  int                      num_blocks;  ///< Capacity (Fixed-sized blocks actual max qty)
  void                    *start_addr;  ///< Memory pool actual start address
  void                     *free_list;  ///< Ptr to free block list
  int                         fblkcnt;  ///< Num of free blocks
} TN_FMP;


/* - Mutex -------------------------------------------------------------------*/

/* Attributes structure for mutex */
typedef struct osMutexAttr_s {
  uint32_t attr_bits;     ///< attribute bits
} osMutexAttr_t;

/* Mutex Control Block */
typedef struct osMutex_s {
  uint8_t                          id;  ///< Object Identifier
  uint8_t              reserved_state;  ///< Object State (not used)
  uint8_t                       flags;  ///< Object Flags
  uint8_t                        attr;  ///< Object Attributes
  queue_t                    wait_que;  ///< List of tasks that wait a mutex
  queue_t                   mutex_que;  ///< To include in task's locked mutexes list (if any)
  osThread_t                  *holder;  ///< Current mutex owner(task that locked mutex)
  uint32_t                        cnt;  ///< Lock counter
} osMutex_t;


/* - Alarm Timer -------------------------------------------------------------*/

typedef enum {
  TIMER_STOP            = 0x00,
  TIMER_START           = 0x01,
  timer_state_reserved  = 0x7fffffff
} timer_state_t;

typedef struct osAlarm_s {
  uint8_t                          id;  ///< Object Identifier
  uint8_t              reserved_state;  ///< Object State (not used)
  uint8_t                       flags;  ///< Object Flags
  uint8_t                        attr;  ///< Object Attributes
  void                         *exinf;  ///< Extended information
  CBACK                       handler;  ///< Alarm handler address
  timer_state_t                 state;  ///< Timer state
  timer_t                       timer;  ///< Timer event block
} osAlarm_t;

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
typedef struct osCyclic_s {
  uint8_t                          id;  ///< Object Identifier
  uint8_t              reserved_state;  ///< Object State (not used)
  uint8_t                       flags;  ///< Object Flags
  uint8_t               reserved_attr;  ///< Object Attributes
  void                         *exinf;  ///< Extended information
  CBACK                       handler;  ///< Cyclic handler address
  timer_state_t                 state;  ///< Timer state
  cyclic_attr_t                  attr;  ///< Cyclic handler attributes
  uint32_t                       time;  ///< Cyclic time
  timer_t                       timer;  ///< Timer event block
} osCyclic_t;


/* - User functions ----------------------------------------------------------*/
typedef void (*TN_USER_FUNC)(void);

typedef struct {
  TN_USER_FUNC app_init;
  uint32_t freq_timer;
  uint32_t max_syscall_interrupt_priority;
} TN_OPTIONS;

#ifndef TZ_MODULEID_T
#define TZ_MODULEID_T
/// \details Data type that identifies secure software modules called by a process.
typedef uint32_t TZ_ModuleId_t;
#endif

/// Attributes structure for thread.
typedef struct {
  const char                   *name;   ///< name of the thread
  uint32_t                 attr_bits;   ///< attribute bits
  void                      *cb_mem;    ///< memory for control block
  uint32_t                   cb_size;   ///< size of provided memory for control block
  void                   *stack_mem;    ///< memory for stack
  uint32_t                stack_size;   ///< size of stack
  osPriority_t              priority;   ///< initial thread priority (default: osPriorityNormal)
  TZ_ModuleId_t            tz_module;   ///< TrustZone module identifier
  uint32_t                  reserved;   ///< reserved (must be 0)
} osThreadAttr_t;

/// Attributes structure for event flags.
typedef struct {
  const char                   *name;   ///< name of the event flags
  uint32_t                 attr_bits;   ///< attribute bits
  void                       *cb_mem;   ///< memory for control block
  uint32_t                   cb_size;   ///< size of provided memory for control block
} osEventFlagsAttr_t;

/// Attributes structure for semaphore.
typedef struct {
  const char                   *name;   ///< name of the semaphore
  uint32_t                 attr_bits;   ///< attribute bits
  void                       *cb_mem;   ///< memory for control block
  uint32_t                   cb_size;   ///< size of provided memory for control block
} osSemaphoreAttr_t;

/// Attributes structure for message queue.
typedef struct {
  const char                   *name;   ///< name of the message queue
  uint32_t                 attr_bits;   ///< attribute bits
  void                       *cb_mem;   ///< memory for control block
  uint32_t                   cb_size;   ///< size of provided memory for control block
  void                       *mq_mem;   ///< memory for data storage
  uint32_t                   mq_size;   ///< size of provided memory for data storage
} osMessageQueueAttr_t;

/*******************************************************************************
 *  exported variables
 ******************************************************************************/

/*******************************************************************************
 *  exported function prototypes
 ******************************************************************************/

/*******************************************************************************
 *  Kernel Information and Control
 ******************************************************************************/

__NO_RETURN
void osKernelStart(TN_OPTIONS *opt);

#if defined(ROUND_ROBIN_ENABLE)
int tn_sys_tslice_ticks(int priority, int value);
#endif

/*******************************************************************************
 *  Timer Management
 ******************************************************************************/

void osTimerHandle(void);

uint32_t osGetTickCount(void);

osError_t osAlarmCreate(osAlarm_t *alarm, CBACK handler, void *exinf);

osError_t osAlarmDelete(osAlarm_t *alarm);

osError_t osAlarmStart(osAlarm_t *alarm, uint32_t timeout);

osError_t osAlarmStop(osAlarm_t *alarm);

osError_t osCyclicCreate(osCyclic_t *cyc, CBACK handler, const cyclic_param_t *param, void *exinf);

osError_t osCyclicDelete(osCyclic_t *cyc);

osError_t osCyclicStart(osCyclic_t *cyc);

osError_t osCyclicStop(osCyclic_t *cyc);

/*******************************************************************************
 *  Thread Management
 ******************************************************************************/

/**
 * @fn          osThreadId_t osThreadNew(osThreadFunc_t func, void *argument, const osThreadAttr_t *attr)
 * @brief       Create a thread and add it to Active Threads.
 * @param[in]   func      thread function.
 * @param[in]   argument  pointer that is passed to the thread function as start argument.
 * @param[in]   attr      thread attributes.
 * @return      thread ID for reference by other functions or NULL in case of error.
 */
osThreadId_t osThreadNew(osThreadFunc_t func, void *argument, const osThreadAttr_t *attr);

/**
 * @fn          const char *osThreadGetName(osThreadId_t thread_id)
 * @brief       Get name of a thread.
 * @param[in]   thread_id   thread ID obtained by \ref osThreadNew or \ref osThreadGetId.
 * @return      name as null-terminated string.
 */
const char *osThreadGetName(osThreadId_t thread_id);

/**
 * @fn          osThreadId_t osThreadGetId(void)
 * @brief       Return the thread ID of the current running thread.
 * @return      thread ID for reference by other functions or NULL in case of error.
 */
osThreadId_t osThreadGetId(void);

/**
 * @fn          osThreadState_t osThreadGetState(osThreadId_t thread_id)
 * @brief       Get current thread state of a thread.
 * @param[in]   thread_id   thread ID obtained by \ref osThreadNew or \ref osThreadGetId.
 * @return      current thread state of the specified thread.
 */
osThreadState_t osThreadGetState(osThreadId_t thread_id);

/**
 * @fn          uint32_t osThreadGetStackSize(osThreadId_t thread_id)
 * @brief       Get stack size of a thread.
 * @param[in]   thread_id   thread ID obtained by \ref osThreadNew or \ref osThreadGetId.
 * @return      stack size in bytes.
 */
uint32_t osThreadGetStackSize(osThreadId_t thread_id);

/**
 * @fn          uint32_t osThreadGetStackSpace(osThreadId_t thread_id)
 * @brief       Get available stack space of a thread based on stack watermark recording during execution.
 * @param[in]   thread_id   thread ID obtained by \ref osThreadNew or \ref osThreadGetId.
 * @return      remaining stack space in bytes.
 */
uint32_t osThreadGetStackSpace(osThreadId_t thread_id);

/**
 * @fn          osStatus_t osThreadSetPriority(osThreadId_t thread_id, osPriority_t priority)
 * @brief       Change priority of a thread.
 * @param[in]   thread_id   thread ID obtained by \ref osThreadNew or \ref osThreadGetId.
 * @param[in]   priority    new priority value for the thread function.
 * @return      status code that indicates the execution status of the function.
 */
osStatus_t osThreadSetPriority(osThreadId_t thread_id, osPriority_t priority);

/**
 * @fn          osPriority_t osThreadGetPriority(osThreadId_t thread_id)
 * @brief       Get current priority of a thread.
 * @param[in]   thread_id   thread ID obtained by \ref osThreadNew or \ref osThreadGetId.
 * @return      current priority value of the specified thread.
 */
osPriority_t osThreadGetPriority(osThreadId_t thread_id);

/**
 * @fn          osStatus_t osThreadYield(void)
 * @brief       Pass control to next thread that is in state READY.
 * @return      status code that indicates the execution status of the function.
 */
osStatus_t osThreadYield(void);

/**
 * @fn          osStatus_t osThreadSuspend(osThreadId_t thread_id)
 * @brief       Suspend execution of a thread.
 * @param[in]   thread_id   thread ID obtained by \ref osThreadNew or \ref osThreadGetId.
 * @return      status code that indicates the execution status of the function.
 */
osStatus_t osThreadSuspend(osThreadId_t thread_id);

/**
 * @fn          osStatus_t osThreadResume(osThreadId_t thread_id)
 * @brief       Resume execution of a thread.
 * @param[in]   thread_id   thread ID obtained by \ref osThreadNew or \ref osThreadGetId.
 * @return      status code that indicates the execution status of the function.
 */
osStatus_t osThreadResume(osThreadId_t thread_id);

/**
 * @fn          void osThreadExit(void)
 * @brief       Terminate execution of current running thread.
 */
__NO_RETURN void osThreadExit(void);

/**
 * @fn          osStatus_t osThreadTerminate(osThreadId_t thread_id)
 * @brief       Terminate execution of a thread.
 * @param[in]   thread_id   thread ID obtained by \ref osThreadNew or \ref osThreadGetId.
 * @return      status code that indicates the execution status of the function.
 */
osStatus_t osThreadTerminate(osThreadId_t thread_id);

/**
 * @fn          uint32_t osThreadGetCount(void)
 * @brief       Get number of active threads.
 * @return      number of active threads or 0 in case of an error.
 */
uint32_t osThreadGetCount(void);

/**
 * @fn          uint32_t osThreadEnumerate(osThreadId_t *thread_array, uint32_t array_items)
 * @brief       Enumerate active threads.
 * @param[out]  thread_array  pointer to array for retrieving thread IDs.
 * @param[in]   array_items   maximum number of items in array for retrieving thread IDs.
 * @return      number of enumerated threads or 0 in case of an error.
 */
uint32_t osThreadEnumerate(osThreadId_t *thread_array, uint32_t array_items);

/*******************************************************************************
 *  Generic Wait Functions
 ******************************************************************************/

/**
 * @fn          osStatus_t osDelay(uint32_t ticks)
 * @brief       Wait for Timeout (Time Delay).
 * @param[in]   ticks   \ref CMSIS_RTOS_TimeOutValue "time ticks" value
 * @return      status code that indicates the execution status of the function.
 */
osStatus_t osDelay(uint32_t ticks);

/**
 * @fn          osStatus_t osDelayUntil(uint32_t ticks)
 * @brief       Wait until specified time.
 * @param[in]   ticks   absolute time in ticks
 * @return      status code that indicates the execution status of the function.
 */
osStatus_t osDelayUntil(uint32_t ticks);

/*******************************************************************************
 *  Semaphores
 ******************************************************************************/

/**
 * @fn          osSemaphoreId_t osSemaphoreNew(uint32_t max_count, uint32_t initial_count, const osSemaphoreAttr_t *attr)
 * @brief       Create and Initialize a Semaphore object.
 * @param[in]   max_count       maximum number of available tokens.
 * @param[in]   initial_count   initial number of available tokens.
 * @param[in]   attr            semaphore attributes.
 * @return      semaphore ID for reference by other functions or NULL in case of error.
 */
osSemaphoreId_t osSemaphoreNew(uint32_t max_count, uint32_t initial_count, const osSemaphoreAttr_t *attr);

/**
 * @fn          const char *osSemaphoreGetName(osSemaphoreId_t semaphore_id)
 * @brief       Get name of a Semaphore object.
 * @param[in]   semaphore_id  semaphore ID obtained by \ref osSemaphoreNew.
 * @return      name as null-terminated string or NULL in case of an error.
 */
const char *osSemaphoreGetName(osSemaphoreId_t semaphore_id);

/**
 * @fn          osStatus_t osSemaphoreAcquire(osSemaphoreId_t semaphore_id, uint32_t timeout)
 * @brief       Acquire a Semaphore token or timeout if no tokens are available.
 * @param[in]   semaphore_id  semaphore ID obtained by \ref osSemaphoreNew.
 * @param[in]   timeout       \ref CMSIS_RTOS_TimeOutValue or 0 in case of no time-out.
 * @return      status code that indicates the execution status of the function.
 */
osStatus_t osSemaphoreAcquire(osSemaphoreId_t semaphore_id, uint32_t timeout);

/**
 * @fn          osStatus_t osSemaphoreRelease(osSemaphoreId_t semaphore_id)
 * @brief       Release a Semaphore token that was acquired by osSemaphoreAcquire.
 * @param[in]   semaphore_id  semaphore ID obtained by \ref osSemaphoreNew.
 * @return      status code that indicates the execution status of the function.
 */
osStatus_t osSemaphoreRelease(osSemaphoreId_t semaphore_id);

/**
 * @fn          uint32_t osSemaphoreGetCount(osSemaphoreId_t semaphore_id)
 * @brief       Get current Semaphore token count.
 * @param[in]   semaphore_id  semaphore ID obtained by \ref osSemaphoreNew.
 * @return      number of tokens available or 0 in case of an error.
 */
uint32_t osSemaphoreGetCount(osSemaphoreId_t semaphore_id);

/**
 * @fn          osStatus_t osSemaphoreDelete(osSemaphoreId_t semaphore_id)
 * @brief       Delete a Semaphore object.
 * @param[in]   semaphore_id  semaphore ID obtained by \ref osSemaphoreNew.
 * @return      status code that indicates the execution status of the function.
 */
osStatus_t osSemaphoreDelete(osSemaphoreId_t semaphore_id);


/*******************************************************************************
 *  Message Queue
 ******************************************************************************/

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
 * @fn          osError_t osMessageQueuePut(osMessageQueue_t *mq, const void *msg, osMsgPriority_t msg_pri, uint32_t timeout)
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
osError_t osMessageQueuePut(osMessageQueue_t *mq, const void *msg, osMsgPriority_t msg_pri, uint32_t timeout);

/**
 * @fn          osError_t osMessageQueueGet(osMessageQueue_t *mq, void *msg, uint32_t timeout)
 * @brief       Retrieves a message from the message queue and saves it to the buffer
 * @param[out]  mq        Pointer to the osMessageQueue_t structure
 * @param[out]  msg       Pointer to buffer for message to get from a queue
 * @param[in]   timeout   Timeout Value or 0 in case of no time-out
 * @return      TERR_NO_ERR       The message has been retrieved from the queue
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_NOEXS        Object is not a Message Queue or non-existent
 *              TERR_TIMEOUT      The message could not be retrieved from the queue in the given time
 */
osError_t osMessageQueueGet(osMessageQueue_t *mq, void *msg, uint32_t timeout);

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


/*******************************************************************************
 *  Event Flags
 ******************************************************************************/

/**
 * @fn          osEventFlagsId_t osEventFlagsNew(const osEventFlagsAttr_t *attr)
 * @brief       Create and Initialize an Event Flags object.
 * @param[in]   attr  event flags attributes.
 * @return      event flags ID for reference by other functions or NULL in case of error.
 */
osEventFlagsId_t osEventFlagsNew(const osEventFlagsAttr_t *attr);

/**
 * @fn          const char *osEventFlagsGetName(osEventFlagsId_t ef_id)
 * @brief       Get name of an Event Flags object.
 * @param[in]   ef_id   event flags ID obtained by \ref osEventFlagsNew.
 * @return      name as null-terminated string or NULL in case of an error.
 */
const char *osEventFlagsGetName(osEventFlagsId_t ef_id);

/**
 * @fn          uint32_t osEventFlagsSet(osEventFlagsId_t ef_id, uint32_t flags)
 * @brief       Set the specified Event Flags.
 * @param[in]   ef_id   event flags ID obtained by \ref osEventFlagsNew.
 * @param[in]   flags   specifies the flags that shall be set.
 * @return      event flags after setting or error code if highest bit set.
 */
uint32_t osEventFlagsSet(osEventFlagsId_t ef_id, uint32_t flags);

/**
 * @fn          uint32_t osEventFlagsClear(osEventFlagsId_t ef_id, uint32_t flags)
 * @brief       Clear the specified Event Flags.
 * @param[in]   ef_id   event flags ID obtained by \ref osEventFlagsNew.
 * @param[in]   flags   specifies the flags that shall be cleared.
 * @return      event flags before clearing or error code if highest bit set.
 */
uint32_t osEventFlagsClear(osEventFlagsId_t ef_id, uint32_t flags);

/**
 * @fn          uint32_t osEventFlagsGet(osEventFlagsId_t ef_id)
 * @brief       Get the current Event Flags.
 * @param[in]   ef_id   event flags ID obtained by \ref osEventFlagsNew.
 * @return      current event flags or 0 in case of an error.
 */
uint32_t osEventFlagsGet(osEventFlagsId_t ef_id);

/**
 * @fn          uint32_t osEventFlagsWait(osEventFlagsId_t ef_id, uint32_t flags, uint32_t options, uint32_t timeout)
 * @brief       Wait for one or more Event Flags to become signaled.
 * @param[in]   ef_id     event flags ID obtained by \ref osEventFlagsNew.
 * @param[in]   flags     specifies the flags to wait for.
 * @param[in]   options   specifies flags options (osFlagsXxxx).
 * @param[in]   timeout   \ref CMSIS_RTOS_TimeOutValue or 0 in case of no time-out.
 * @return      event flags before clearing or error code if highest bit set.
 */
uint32_t osEventFlagsWait(osEventFlagsId_t ef_id, uint32_t flags, uint32_t options, uint32_t timeout);

/**
 * @fn          osStatus_t osEventFlagsDelete(osEventFlagsId_t ef_id)
 * @brief       Delete an Event Flags object.
 * @param[in]   ef_id   event flags ID obtained by \ref osEventFlagsNew.
 * @return      status code that indicates the execution status of the function.
 */
osStatus_t osEventFlagsDelete(osEventFlagsId_t ef_id);


/* - tn_mem.c ----------------------------------------------------------------*/
osError_t tn_fmem_create(TN_FMP *fmp, void *start_addr, unsigned int block_size,
                   int num_blocks);
osError_t tn_fmem_delete(TN_FMP *fmp);
osError_t tn_fmem_get(TN_FMP *fmp, void **p_data, unsigned long timeout);
osError_t tn_fmem_release(TN_FMP *fmp, void *p_data);


/*******************************************************************************
 *  Mutex Management
 ******************************************************************************/

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
 * @fn          osError_t osMutexAcquire(osMutex_t *mutex, uint32_t timeout)
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
osError_t osMutexAcquire(osMutex_t *mutex, uint32_t timeout);

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
 * @fn          osThread_t* osMutexGetOwner(osMutex_t *mutex)
 * @brief       Returns the pointer to the task that acquired a mutex. In case
 *              of an error or if the mutex is not blocked by any task, it returns NULL.
 * @param[out]  mutex     Pointer to osMutex_t structure of the mutex
 * @return      Pointer to owner task or NULL when mutex was not acquired
 */
osThread_t* osMutexGetOwner(osMutex_t *mutex);


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

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

#define stack_t     __attribute__((aligned(8), section("STACK"), zero_init)) uint32_t

/* - The system configuration (change it for your particular project) --------*/
#define USE_MUTEXES           1
#define USE_EVENTS            1

/* - Constants ---------------------------------------------------------------*/
#define TN_TASK_START_ON_CREATION        1 
#define TN_TASK_TIMER                 0x80
#define TN_TASK_IDLE                  0x40

#define TN_EVENT_ATTR_SINGLE            1
#define TN_EVENT_ATTR_MULTI             2
#define TN_EVENT_ATTR_CLR               4

#define TN_EVENT_WCOND_OR               8
#define TN_EVENT_WCOND_AND           0x10

#define TN_MUTEX_ATTR_INHERIT           0
#define TN_MUTEX_ATTR_CEILING         (1UL<<0)
#define TN_MUTEX_ATTR_RECURSIVE       (1UL<<1)

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
  ID_INVALID      = 0x00000000,
  ID_TASK         = 0x47ABCF69,
  ID_SEMAPHORE    = 0x6FA173EB,
  ID_EVENT        = 0x5E224F25,
  ID_DATAQUEUE    = 0x0C8A6C89,
  ID_FSMEMORYPOOL = 0x26B7CE8B,
  ID_MUTEX        = 0x17129E45,
  ID_RENDEZVOUS   = 0x74289EBD,
  ID_ALARM        = 0x7A5762BC,
  ID_CYCLIC       = 0x2B8F746B,
  ID_MESSAGEBUF   = 0x1C9A6C89,
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
  WAIT_REASON_MUTEX_C_BLK   = 0x0040,
  WAIT_REASON_MUTEX_I       = 0x0080,
  WAIT_REASON_MUTEX_H       = 0x0100,
  WAIT_REASON_RENDEZVOUS    = 0x0200,
  WAIT_REASON_MBF_WSEND     = 0x0400,
  WAIT_REASON_MBF_WRECEIVE  = 0x0800,
  WAIT_REASON_WFIXMEM       = 0x1000,
  wait_reason_reserved      = 0x7fffffff
} wait_reason_t;

typedef uint32_t TIME_t;

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
} WINFO_RMBF;

typedef struct {
  void *msg; /* Send message head address */
  bool send_to_first;
} WINFO_SMBF;

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
  WINFO_RMBF rmbf;
  WINFO_SMBF smbf;
  WINFO_FMEM fmem;
  WINFO_EVENT event;
} WINFO;

/* - Task Control Block ------------------------------------------------------*/
typedef struct _TN_TCB {
  uint32_t *task_stk;         ///< Pointer to task's top of stack
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
  osError_t *wait_rc;         ///< Waiting return code(reason why waiting  finished)
  WINFO wait_info;            ///< Wait information
  TMEB wait_timer;            ///< Wait timer
  int tslice_count;           ///< Time slice counter
  TIME_t time;                ///< Time work task
} TN_TCB;

/* - Semaphore ---------------------------------------------------------------*/
typedef struct _TN_SEM {
  CDLL_QUEUE wait_queue;
  uint32_t count;
  uint32_t max_count;
  id_t id;                    ///< ID for verification(is it a semaphore or another object?)
} TN_SEM;

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
typedef struct _TN_MUTEX {
  CDLL_QUEUE wait_queue;       //-- List of tasks that wait a mutex
  CDLL_QUEUE mutex_queue; //-- To include in task's locked mutexes list (if any)
  int attr;             //-- Mutex creation attr - CEILING or INHERIT
  TN_TCB *holder;          //-- Current mutex owner(task that locked mutex)
  int ceil_priority;    //-- When mutex created with CEILING attr
  int cnt;              //-- Reserved
  id_t id;              //-- ID for verification(is it a mutex or another object?)
} TN_MUTEX;

typedef enum {
  TIMER_STOP            = 0x00,
  TIMER_START           = 0x01,
  timer_state_reserved  = 0x7fffffff
} timer_state_t;

/* - Alarm Timer -------------------------------------------------------------*/
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

/* - Message Buffer ----------------------------------------------------------*/
typedef struct _TN_MBF {
  CDLL_QUEUE send_queue;  // Message buffer send wait queue
  CDLL_QUEUE recv_queue;  // Message buffer receive wait queue
  char *buf;              // Message buffer address
  int msz;                // Length of message
  int num_entries;        // Capacity of data_fifo(num entries)
  int cnt;                // Кол-во данных в очереди
  int tail;               // Next to the last message store address
  int head;               // First message store address
  id_t id;                // Message buffer ID
} TN_MBF;

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
void KernelStart(TN_OPTIONS *opt);

#if defined(ROUND_ROBIN_ENABLE)
int tn_sys_tslice_ticks(int priority, int value);
#endif

/* - Timer -------------------------------------------------------------------*/

void osTimerHandle(void);

TIME_t osGetTickCount(void);

osError_t osAlarmCreate(TN_ALARM *alarm, CBACK handler, void *exinf);

osError_t osAlarmDelete(TN_ALARM *alarm);

osError_t osAlarmStart(TN_ALARM *alarm, TIME_t timeout);

osError_t osAlarmStop(TN_ALARM *alarm);

osError_t osCyclicCreate(TN_CYCLIC *cyc, CBACK handler, const cyclic_param_t *param, void *exinf);

osError_t osCyclicDelete(TN_CYCLIC *cyc);

osError_t osCyclicStart(TN_CYCLIC *cyc);

osError_t osCyclicStop(TN_CYCLIC *cyc);

/* - Task --------------------------------------------------------------------*/

/**
 * @fn          osError_t osTaskCreate(TN_TCB *task, void (*func)(void *), int32_t priority, const uint32_t *stack_start, int32_t stack_size, const void *param, int32_t option)
 * @brief       Creates a task.
 * @param[out]  task          Pointer to the task TCB to be created
 * @param[in]   func          Task body function
 * @param[in]   priority      Task priority. User tasks may have priorities 1…30
 * @param[in]   stack_start   Task's stack bottom address
 * @param[in]   stack_size    Task's stack size – number of task stack elements (not bytes)
 * @param[in]   param         task_func parameter. param will be passed to task_func on creation time
 * @param[in]   option        Creation option. Option values:
 *                              0                           After creation task has a DORMANT state
 *                              TN_TASK_START_ON_CREATION   After creation task is switched to the runnable state (READY/RUNNING)
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_ISR          The function cannot be called from interrupt service routines
 */
osError_t osTaskCreate(TN_TCB *task, void (*func)(void *), int32_t priority, const uint32_t *stack_start, int32_t stack_size, const void *param, int32_t option);

/**
 * @fn          osError_t osTaskDelete(TN_TCB *task)
 * @brief       Deletes the task specified by the task.
 * @param[out]  task  Pointer to the task TCB to be deleted
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_WCONTEXT     Unacceptable system's state for this request
 *              TERR_NOEXS        Object is not a task or non-existent
 *              TERR_ISR          The function cannot be called from interrupt service routines
 */
osError_t osTaskDelete(TN_TCB *task);

/**
 * @fn          osError_t osTaskActivate(TN_TCB *task)
 * @brief       Activates a task specified by the task
 * @param[out]  task  Pointer to the task TCB to be activated
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_OVERFLOW     Task is already active (not in DORMANT state)
 *              TERR_NOEXS        Object is not a task or non-existent
 *              TERR_ISR          The function cannot be called from interrupt service routines
 */
osError_t osTaskActivate(TN_TCB *task);

/**
 * @fn          osError_t osTaskTerminate(TN_TCB *task)
 * @brief       Terminates the task specified by the task
 * @param[out]  task  Pointer to the task TCB to be terminated
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_WCONTEXT     Unacceptable system state for this request
 *              TERR_NOEXS        Object is not a task or non-existent
 *              TERR_ISR          The function cannot be called from interrupt service routines
 */
osError_t osTaskTerminate(TN_TCB *task);

/**
 * @fn        void osTaskExit(task_exit_attr_t attr)
 * @brief     Terminates the currently running task
 * @param[in] attr  Exit option. Option values:
 *                    TASK_EXIT             Currently running task will be terminated.
 *                    TASK_EXIT_AND_DELETE  Currently running task will be terminated and then deleted
 */
void osTaskExit(task_exit_attr_t attr);

/**
 * @fn          osError_t osTaskSuspend(TN_TCB *task)
 * @brief       Suspends the task specified by the task
 * @param[out]  task  Pointer to the task TCB to be suspended
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_OVERFLOW     Task already suspended
 *              TERR_WSTATE       Task is not active (i.e in DORMANT state )
 *              TERR_NOEXS        Object is not a task or non-existent
 *              TERR_ISR          The function cannot be called from interrupt service routines
 */
osError_t osTaskSuspend(TN_TCB *task);

/**
 * @fn          osError_t osTaskResume(TN_TCB *task)
 * @brief       Releases the task specified by the task from the SUSPENDED state
 * @param[out]  task  Pointer to task TCB to be resumed
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_WSTATE       Task is not in SUSPEDED or WAITING_SUSPEDED state
 *              TERR_NOEXS        Object is not a task or non-existent
 *              TERR_ISR          The function cannot be called from interrupt service routines
 */
osError_t osTaskResume(TN_TCB *task);

/**
 * @fn        osError_t osTaskSleep(TIME_t timeout)
 * @brief     Puts the currently running task to the sleep for at most timeout system ticks.
 * @param[in] timeout   Timeout value must be greater than 0.
 *                      A value of TIME_WAIT_INFINITE causes an infinite delay.
 * @return    TERR_NO_ERR       Normal completion
 *            TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *            TERR_NOEXS        Object is not a task or non-existent
 *            TERR_ISR          The function cannot be called from interrupt service routines
 */
osError_t osTaskSleep(TIME_t timeout);

/**
 * @fn          osError_t osTaskWakeup(TN_TCB *task)
 * @brief       Wakes up the task specified by the task from sleep mode
 * @param[out]  task  Pointer to the task TCB to be wake up
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_NOEXS        Object is not a task or non-existent
 *              TERR_WSTATE       Task is not in WAIT state
 *              TERR_ISR          The function cannot be called from interrupt service routines
 */
osError_t osTaskWakeup(TN_TCB *task);

/**
 * @fn          osError_t osTaskReleaseWait(TN_TCB *task)
 * @brief       Forcibly releases the task specified by the task from waiting
 * @param[out]  task  Pointer to the task TCB to be released from waiting or sleep
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_WCONTEXT     Unacceptable system's state for function's request executing
 *              TERR_NOEXS        Object is not a task or non-existent
 *              TERR_ISR          The function cannot be called from interrupt service routines
 */
osError_t osTaskReleaseWait(TN_TCB *task);

osError_t osTaskSetPriority(TN_TCB *task, uint32_t new_priority);

TIME_t osTaskGetTime(TN_TCB *task);

/* - Semaphore ---------------------------------------------------------------*/
/**
 * @fn          osError_t osSemaphoreNew(TN_SEM *sem, uint32_t initial_count, uint32_t max_count)
 * @brief       Creates a semaphore
 * @param[out]  sem             Pointer to the semaphore structure to be created
 * @param[in]   initial_count   Initial number of available tokens
 * @param[in]   max_count       Maximum number of available tokens
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_ISR          The function cannot be called from interrupt service routines
 */
osError_t osSemaphoreNew(TN_SEM *sem, uint32_t initial_count, uint32_t max_count);

/**
 * @fn          osError_t osSemaphoreDelete(TN_SEM *sem)
 * @brief       Deletes a semaphore
 * @param[out]  sem   Pointer to the semaphore structure to be deleted
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_NOEXS        Object is not a semaphore or non-existent
 *              TERR_ISR          The function cannot be called from interrupt service routines
 */
osError_t osSemaphoreDelete(TN_SEM *sem);

/**
 * @fn          osError_t osSemaphoreRelease(TN_SEM *sem)
 * @brief       Release a Semaphore token up to the initial maximum count.
 * @param[out]  sem   Pointer to the semaphore structure to be released
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_OVERFLOW     Semaphore Resource has max_count value
 *              TERR_NOEXS        Object is not a semaphore or non-existent
 */
osError_t osSemaphoreRelease(TN_SEM *sem);

/**
 * @fn          osError_t osSemaphoreAcquire(TN_SEM *sem, TIME_t timeout)
 * @brief       Acquire a Semaphore token or timeout if no tokens are available.
 * @param[out]  sem       Pointer to the semaphore structure to be acquired
 * @param[in]   timeout   Timeout value must be equal or greater than 0
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_TIMEOUT      Timeout expired
 *              TERR_NOEXS        Object is not a semaphore or non-existent
 */
osError_t osSemaphoreAcquire(TN_SEM *sem, TIME_t timeout);

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

/* - tn_message_buf.c --------------------------------------------------------*/

/*-----------------------------------------------------------------------------*
 * Название : tn_mbf_create
 * Описание : Создает буфер сообщений.
 * Параметры: mbf - Указатель на существующую структуру TN_MBF.
 *            buf - Указатель на выделенную под буфер сообщений область памяти.
 *                  Может быть равен NULL.
 *            bufsz - Размер буфера сообщений в байтах. Может быть равен 0,
 *                    тогда задачи общаются через буфер в синхронном режиме.
 *            msz - Размер сообщения в байтах. Должен быть больше нуля.
 * Результат: Возвращает один из вариантов:
 *              TERR_NO_ERR - функция выполнена без ошибок;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *----------------------------------------------------------------------------*/
extern osError_t tn_mbf_create(TN_MBF *mbf, void *buf, int bufsz, int msz);

/*-----------------------------------------------------------------------------*
 * Название : tn_mbf_delete
 * Описание : Удаляет буфер сообщений.
 * Параметры: mbf - Указатель на существующую структуру TN_MBF.
 * Результат: Возвращает один из вариантов:
 *              TERR_NO_ERR - функция выполнена без ошибок;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_NOEXS  - буфер сообщений не существует;
 *----------------------------------------------------------------------------*/
extern osError_t tn_mbf_delete(TN_MBF *mbf);

/*-----------------------------------------------------------------------------*
 * Название : tn_mbf_send
 * Описание : Помещает данные в конец буфера сообщений за установленный
 *            интервал времени.
 * Параметры: mbf - Дескриптор буфера сообщений.
 *            msg - Указатель на данные.
 *            timeout - Время, в течении которого данные должны быть помещены
 *                      в буфер.
 * Результат: Возвращает один из вариантов:
 *              TERR_NO_ERR - функция выполнена без ошибок;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_NOEXS  - буфер не существует;
 *              TERR_TIMEOUT  - Превышен заданный интервал времени;
 *----------------------------------------------------------------------------*/
extern osError_t tn_mbf_send(TN_MBF *mbf, void *msg, unsigned long timeout);

/*-----------------------------------------------------------------------------*
 * Название : tn_mbf_send_first
 * Описание : Помещает данные в начало буфера сообщений за установленный
 *            интервал времени.
 * Параметры: mbf - Дескриптор буфера сообщений.
 *            msg - Указатель на данные.
 *            timeout - Время, в течении которого данные должны быть помещены
 *                      в буфер.
 * Результат: Возвращает один из вариантов:
 *              TERR_NO_ERR - функция выполнена без ошибок;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_NOEXS  - буфер не существует;
 *              TERR_TIMEOUT  - Превышен заданный интервал времени;
 *----------------------------------------------------------------------------*/
extern osError_t tn_mbf_send_first(TN_MBF *mbf, void *msg, unsigned long timeout);

/*-----------------------------------------------------------------------------*
 * Название : tn_mbf_receive
 * Описание : Считывает один элемент данных из начала буфера сообщений
 *            за установленный интервал времени.
 * Параметры: mbf - Дескриптор буфера сообщений.
 *            msg - Указатель на буфер, куда будут считаны данные.
 *            timeout - Время, в течении которого данные должны быть считаны.
 * Результат: Возвращает один из вариантов:
 *              TERR_NO_ERR - функция выполнена без ошибок;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_NOEXS  - буфер не существует;
 *              TERR_TIMEOUT  - Превышен заданный интервал времени;
 *----------------------------------------------------------------------------*/
extern osError_t tn_mbf_receive(TN_MBF *mbf, void *msg, unsigned long timeout);

/*-----------------------------------------------------------------------------*
 * Название : tn_mbf_flush
 * Описание : Сбрасывает буфер сообщений.
 * Параметры: mbf - Дескриптор буфера сообщений.
 * Результат: Возвращает один из вариантов:
 *              TERR_NO_ERR - функция выполнена без ошибок;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_NOEXS  - буфер не был создан.
 *----------------------------------------------------------------------------*/
extern osError_t tn_mbf_flush(TN_MBF *mbf);

/*-----------------------------------------------------------------------------*
 * Название : tn_mbf_empty
 * Описание : Проверяет буфер сообщений на пустоту.
 * Параметры: mbf - Дескриптор буфера сообщений.
 * Результат: Возвращает один из вариантов:
 *              TERR_TRUE - буфер сообщений пуст;
 *              TERR_NO_ERR - в буфере данные есть;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_NOEXS  - буфер не был создан.
 *----------------------------------------------------------------------------*/
extern osError_t tn_mbf_empty(TN_MBF *mbf);

/*-----------------------------------------------------------------------------*
 * Название : tn_mbf_full
 * Описание : Проверяет буфер сообщений на полное заполнение.
 * Параметры: mbf - Дескриптор буфера сообщений.
 * Результат: Возвращает один из вариантов:
 *              TERR_TRUE - буфер сообщений полный;
 *              TERR_NO_ERR - буфер сообщений не полный;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_NOEXS  - буфер сообщений не был создан.
 *----------------------------------------------------------------------------*/
extern osError_t tn_mbf_full(TN_MBF *mbf);

/*-----------------------------------------------------------------------------*
 * Название : tn_mbf_cnt
 * Описание : Функция возвращает количество элементов в буфере сообщений.
 * Параметры: mbf - Дескриптор буфера сообщений.
 *            cnt - Указатель на ячейку памяти, в которую будет возвращено
 *                  количество элементов.
 * Результат: Возвращает один из вариантов:
 *              TERR_NO_ERR - функция выполнена без ошибок;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_NOEXS  - буфер сообщений не был создан.
 *----------------------------------------------------------------------------*/
extern osError_t tn_mbf_cnt(TN_MBF *mbf, int *cnt);

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

/* - tn_mutex.c --------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*
 * Название : tn_mutex_create
 * Описание : Создает мьютекс
 * Параметры: mutex - Указатель на инициализируемую структуру TN_MUTEX
 *                    (дескриптор мьютекса)
 *            attribute - Атрибуты создаваемого мьютекса.
 *                        Возможно сочетание следующих значений:
 *                        TN_MUTEX_ATTR_CEILING - Используется протокол
 *                                                увеличения приоритета для
 *                                                исключения инверсии приоритета
 *                                                и взаимной блокировки.
 *                        TN_MUTEX_ATTR_INHERIT - Используется протокол
 *                                                наследования приоритета для
 *                                                исключения инверсии приоритета.
 *                        TN_MUTEX_ATTR_RECURSIVE - Разрешен рекурсивный захват
 *                                                  мьютекса, задачей, которая
 *                                                  уже захватила мьютекс.
 *            ceil_priority - Максимальный приоритет из всех задач, которые
 *                            могут владеть мютексом. Параметр игнорируется,
 *                            если attribute = TN_MUTEX_ATTR_INHERIT
 * Результат: Возвращает TERR_NO_ERR если выполнено без ошибок, в противном
 *            случае TERR_WRONG_PARAM
 *----------------------------------------------------------------------------*/
osError_t tn_mutex_create(TN_MUTEX *mutex, int attribute, int ceil_priority);
osError_t tn_mutex_delete(TN_MUTEX *mutex);
osError_t tn_mutex_lock(TN_MUTEX *mutex, unsigned long timeout);
osError_t tn_mutex_unlock(TN_MUTEX *mutex);

/* - tn_delay.c --------------------------------------------------------------*/
void tn_mdelay(unsigned long ms);
void tn_udelay(unsigned long usecs);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  // _UKERNEL_H_

/*------------------------------ End of file ---------------------------------*/

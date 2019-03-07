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

#ifndef _KNL_LIB_H_
#define _KNL_LIB_H_

/*******************************************************************************
 *  includes
 ******************************************************************************/

#include "arch.h"
#include "ukernel.h"

/*******************************************************************************
 *  defines and macros (scope: module-local)
 ******************************************************************************/

/* Object Identifier definitions */
#define ID_INVALID                  0x00
#define ID_THREAD                   0x47
#define ID_SEMAPHORE                0x6F
#define ID_EVENT_FLAGS              0x5E
#define ID_DATAQUEUE                0x0C
#define ID_MEMORYPOOL               0x26
#define ID_MUTEX                    0x17
#define ID_ALARM                    0x7A
#define ID_CYCLIC                   0x2B
#define ID_MESSAGE_QUEUE            0x1C
#define ID_MESSAGE                  0x1D
#define ID_DATA_QUEUE               0x1E

/* Thread State definitions */
#define ThreadStateInactive         ((uint8_t)osThreadInactive)
#define ThreadStateReady            ((uint8_t)osThreadReady)
#define ThreadStateRunning          ((uint8_t)osThreadRunning)
#define ThreadStateBlocked          ((uint8_t)osThreadBlocked)
#define ThreadStateTerminated       ((uint8_t)osThreadTerminated)

#define container_of(ptr, type, member) ((type *)((uint8_t *)(ptr) - offsetof(type, member)))

#define GetThreadByQueue(que)       container_of(que, osThread_t, task_que)
#define GetMutexByMutexQueque(que)  container_of(que, osMutex_t, mutex_que)
#define GetMutexByWaitQueque(que)   container_of(que, osMutex_t, wait_que)
#define GetTimerByQueue(que)        container_of(que, timer_t, timer_que)
#define GetMessageByQueue(que)      container_of(que, osMessage_t, msg_que)

#define NUM_PRIORITY                (32U)
#define osThreadWait                (-16)

/* OS Configuration flags */
#define osConfigPrivilegedMode      (1UL<<0)    ///< Threads in Privileged mode
#define osConfigStackCheck          (1UL<<1)    ///< Stack overrun checking
#define osConfigStackWatermark      (1UL<<2)    ///< Stack usage Watermark

/*******************************************************************************
 *  typedefs and structures (scope: module-local)
 ******************************************************************************/

/* OS Runtime Information structure */
typedef struct osInfo_s {
  struct {
    struct {
      osThread_t                         *curr;   /// Task that is running now
      osThread_t                         *next;   /// Task to be run after switch context
    } run;
    osThreadId_t                         *idle;
    osThreadId_t                        *timer;
  } thread;
  struct {
    osKernelState_t                      state;   ///< State
    uint32_t                              tick;
  } kernel;
  uint32_t                       base_priority;
  uint32_t                    ready_to_run_bmp;
  queue_t             ready_list[NUM_PRIORITY];   ///< all ready to run(RUNNABLE) tasks
  queue_t                          timer_queue;
} osInfo_t;

/* OS Configuration structure */
typedef struct osConfig_s {
  uint32_t                             flags;   ///< OS Configuration Flags
  uint32_t                         tick_freq;   ///< Kernel Tick Frequency
  uint32_t                     robin_timeout;   ///< Round Robin Timeout Tick
  uint32_t        max_api_interrupt_priority;
  const
  osThreadAttr_t           *idle_thread_attr;   ///< Idle Thread Attributes
  const
  osThreadAttr_t          *timer_thread_attr;   ///< Timer Thread Attributes
} osConfig_t;

typedef enum {
  DISPATCH_NO  = 0,
  DISPATCH_YES = 1,
} dispatch_t;

/*******************************************************************************
 *  exported variables
 ******************************************************************************/

extern osInfo_t osInfo;                   ///< OS Runtime Information
extern const osConfig_t osConfig;         ///< OS Configuration

/*******************************************************************************
 *  exported function prototypes
 ******************************************************************************/

/* Thread */

/**
 * @brief       Thread startup (Idle and Timer Thread).
 * @return      true - success, false - failure.
 */
bool libThreadStartup(void);

/**
 * @brief       Exit Thread wait state.
 * @param[out]  thread    thread object.
 * @param[in]   ret_val   return value.
 */
void libThreadWaitExit(osThread_t *thread, uint32_t ret_val, dispatch_t dispatch);

/**
 * @brief       Enter Thread wait state.
 * @param[out]  thread    thread object.
 * @param[out]  wait_que  Pointer to wait queue.
 * @param[in]   timeout   Timeout
 */
void libThreadWaitEnter(osThread_t *thread, queue_t *wait_que, uint32_t timeout);

/**
 * @brief
 * @param wait_que
 */
void libThreadWaitDelete(queue_t *que);

/**
 * @brief       Change priority of a thread.
 * @param[in]   thread    thread object.
 * @param[in]   priority  new priority value for the thread.
 */
void libThreadSetPriority(osThread_t *thread, int8_t priority);

void libThreadTimerResume(void);

osThread_t *libThreadHighestPrioGet(void);

void libThreadSwitch(osThread_t *thread);

/**
 * @brief       Dispatch specified Thread or Ready Thread with Highest Priority.
 * @param[in]   thread  thread object or NULL.
 */
void libThreadDispatch(osThread_t *thread);

__STATIC_INLINE
osThread_t *ThreadGetRunning(void)
{
  return osInfo.thread.run.curr;
}

/* Queue */
void QueueReset(queue_t *que);
bool isQueueEmpty(queue_t *que);

/**
 * @fn          void QueueAddHead(queue_t *que, queue_t *entry)
 * @brief       Inserts an entry at the head of the queue.
 * @param[out]  que     Pointer to the queue
 * @param[out]  entry   Pointer to an entry
 */
void QueueAddHead(queue_t *que, queue_t *entry);

/**
 * @fn          void QueueAddTail(queue_t *que, queue_t *entry)
 * @brief       Inserts an entry at the tail of the queue.
 * @param[out]  que     Pointer to the queue
 * @param[out]  entry   Pointer to an entry
 */
void QueueAddTail(queue_t *que, queue_t *entry);

/**
 * @fn          void QueueRemoveEntry(queue_t *entry)
 * @brief       Removes an entry from the queue.
 * @param[out]  entry   Pointer to an entry of the queue
 */
void QueueRemoveEntry(queue_t *entry);

/**
 * @fn          queue_t* QueueRemoveHead(queue_t *que)
 * @brief       Remove and return an entry at the head of the queue.
 * @param[out]  que   Pointer to the queue
 * @return      Returns a pointer to an entry at the head of the queue
 */
queue_t* QueueRemoveHead(queue_t *que);

/**
 * @fn          queue_t* QueueRemoveTail(queue_t *que)
 * @brief       Remove and return an entry at the tail of the queue.
 * @param[out]  que   Pointer to the queue
 * @return      Returns a pointer to an entry at the tail of the queue
 */
queue_t* QueueRemoveTail(queue_t *que);

/* Timer */

// Timer Library functions
extern void libTimerThread(void *argument);

/**
 * @fn          void TimerInsert(timer_t *event, uint32_t time, CBACK callback, void *arg);
 * @brief
 * @param event
 * @param time
 * @param callback
 * @param arg
 */
void TimerInsert(timer_t *event, uint32_t time, CBACK callback, void *arg);

/**
 * @fn          void TimerDelete(timer_t *event)
 * @brief
 */
void TimerDelete(timer_t *event);

/**
 * @brief       Release Mutexes when owner Task terminates.
 * @param[in]   que   Queue of mutexes
 */
void libMutexOwnerRelease(queue_t *que);

/**
 * @brief       Initialize Memory Pool.
 * @param[in]   block_count   maximum number of memory blocks in memory pool.
 * @param[in]   block_size    size of a memory block in bytes.
 * @param[in]   block_mem     pointer to memory for block storage.
 * @param[in]   mp_info       memory pool info.
 */
void libMemoryPoolInit(uint32_t block_count, uint32_t block_size, void *block_mem, osMemoryPoolInfo_t *mp_info);

/**
 * @brief       Reset Memory Pool.
 * @param[in]   mp_info       memory pool info.
 */
void libMemoryPoolReset(osMemoryPoolInfo_t *mp_info);

/**
 * @brief       Allocate a memory block from a Memory Pool.
 * @param[in]   mp_info   memory pool info.
 * @return      address of the allocated memory block or NULL in case of no memory is available.
 */
void *libMemoryPoolAlloc(osMemoryPoolInfo_t *mp_info);

/**
 * @brief       Return an allocated memory block back to a Memory Pool.
 * @param[in]   mp_info   memory pool info.
 * @param[in]   block     address of the allocated memory block to be returned to the memory pool.
 * @return      status code that indicates the execution status of the function.
 */
osStatus_t libMemoryPoolFree(osMemoryPoolInfo_t *mp_info, void *block);

#endif /* _KNL_LIB_H_ */
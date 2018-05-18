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

#ifndef _KNL_LIB_H_
#define _KNL_LIB_H_

/*******************************************************************************
 *  includes
 ******************************************************************************/

#include "ukernel.h"
#include "arch.h"

/*******************************************************************************
 *  defines and macros (scope: module-local)
 ******************************************************************************/

#define BITS_IN_INT                (32UL)
#define NUM_PRIORITY               BITS_IN_INT  //-- 0..31  Priority 0 always is used by timers task

#define container_of(ptr, type, member) ((type *)((uint8_t *)(ptr) - offsetof(type, member)))

#define GetTaskByQueue(que)         container_of(que, osTask_t, task_queue)
#define GetMutexByMutexQueque(que)  container_of(que, osMutex_t, mutex_que)
#define GetMutexByWaitQueque(que)   container_of(que, osMutex_t, wait_que)
#define GetTimerByQueue(que)        container_of(que, timer_t, queue)

/*******************************************************************************
 *  typedefs and structures (scope: module-local)
 ******************************************************************************/

typedef enum {
  KERNEL_STATE_NOT_RUN = 0,
  KERNEL_STATE_RUNNING = 1,
  kernel_state_reserved = 0x7fffffff
} kernel_state_t;

typedef struct {
  osTask_t *curr;                     // Task that is running now
  osTask_t *next;                     // Task to be run after switch context
} knlRun_t;

typedef struct {
  knlRun_t run;
  uint32_t HZ;                            ///< Frequency system timer
  uint32_t os_period;
  volatile osTime_t jiffies;
  uint32_t max_syscall_interrupt_priority;
  kernel_state_t kernel_state;            ///< Kernel state -(running/not running)
  uint32_t ready_to_run_bmp;
  queue_t ready_list[NUM_PRIORITY];       ///< all ready to run(RUNNABLE) tasks
  queue_t timer_queue;
#if defined(ROUND_ROBIN_ENABLE)
  uint16_t tslice_ticks[NUM_PRIORITY];    ///< For round-robin only
#endif
} knlInfo_t;

typedef struct {
  uint32_t *stk_start;
  uint32_t stk_size;
  const void *func_addr;
  const void *func_param;
  uint32_t priority;
  int32_t option;
} task_create_attr_t;

/*******************************************************************************
 *  exported variables
 ******************************************************************************/

extern knlInfo_t knlInfo;

/*******************************************************************************
 *  exported function prototypes
 ******************************************************************************/

/* Thread */
void TaskSetReady(osTask_t *thread);
void TaskWaitComplete(osTask_t *task, uint32_t ret_val);
void TaskWaitEnter(osTask_t *task, queue_t *wait_que, wait_reason_t wait_reason,
                           osTime_t timeout);
void TaskChangePriority(osTask_t *task, uint32_t new_priority);
void TaskWaitDelete(queue_t *que);

void TaskCreate(osTask_t *task, const task_create_attr_t *attr);
void TaskToRunnable(osTask_t *task);
__FORCEINLINE osTask_t* TaskGetCurrent(void);
__FORCEINLINE void TaskSetCurrent(osTask_t *task);
__FORCEINLINE osTask_t* TaskGetNext(void);
void TaskSetNext(osTask_t *task);

/* Timer */
void TimerTaskCreate(void *par);
void TimerInsert(timer_t *event, osTime_t time, CBACK callback, void *arg);
void TimerDelete(timer_t *event);

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

/* - Mutex Management --------------------------------------------------------*/

void MutexSetPriority(osTask_t *task, uint32_t priority);
uint32_t MutexGetMaxPriority(osMutex_t *mutex, uint32_t ref_priority);
void MutexUnLock(osMutex_t *mutex);

void calibrate_delay(void);

#endif /* _KNL_LIB_H_ */

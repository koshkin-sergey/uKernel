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

#define BITS_IN_INT                32
#define NUM_PRIORITY               BITS_IN_INT  //-- 0..31  Priority 0 always is used by timers task

#define container_of(ptr, type, member) ((type *)((uint8_t *)(ptr) - offsetof(type, member)))

#define GetTaskByQueue(que)         container_of(que, TN_TCB, task_queue)
#define GetMutexByMutexQueque(que)  container_of(que, TN_MUTEX, mutex_queue)
#define GetMutexByWaitQueque(que)   container_of(que, TN_MUTEX, wait_queue)
#define GetTimerByQueue(que)        container_of(que, TMEB, queue)

/*******************************************************************************
 *  typedefs and structures (scope: module-local)
 ******************************************************************************/

typedef enum {
  KERNEL_STATE_NOT_RUN = 0,
  KERNEL_STATE_RUNNING = 1,
  kernel_state_reserved = 0x7fffffff
} kernel_state_t;

typedef struct {
  TN_TCB *curr;                     // Task that is running now
  TN_TCB *next;                     // Task to be run after switch context
} knlRun_t;

typedef struct {
  knlRun_t run;
  uint32_t HZ;                            ///< Frequency system timer
  uint32_t os_period;
  volatile TIME_t jiffies;
  uint32_t max_syscall_interrupt_priority;
  kernel_state_t kernel_state;            ///< Kernel state -(running/not running)
  uint32_t ready_to_run_bmp;
  CDLL_QUEUE ready_list[NUM_PRIORITY];    ///< all ready to run(RUNNABLE) tasks
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
void ThreadSetReady(TN_TCB *thread);
void ThreadWaitComplete(TN_TCB *task);
void ThreadToWaitAction(TN_TCB *task, CDLL_QUEUE *wait_que, wait_reason_t wait_reason,
                           TIME_t timeout);
void ThreadChangePriority(TN_TCB *task, int32_t new_priority);
void ThreadSetPriority(TN_TCB *task, int32_t priority);
void ThreadWaitDelete(CDLL_QUEUE *que);
void ThreadExit(void);

void TaskCreate(TN_TCB *task, const task_create_attr_t *attr);
void TaskToRunnable(TN_TCB *task);
TN_TCB* TaskGetCurrent(void);
void TaskSetCurrent(TN_TCB *task);
TN_TCB* TaskGetNext(void);
void TaskSetNext(TN_TCB *task);

/* Timer */
void TimerTaskCreate(void *par);
void TimerInsert(TMEB *event, TIME_t time, CBACK callback, void *arg);
void TimerDelete(TMEB *event);

/* Queue */
void QueueReset(CDLL_QUEUE *que);
bool isQueueEmpty(CDLL_QUEUE *que);

/**
 * @fn          void QueueAddHead(CDLL_QUEUE *que, CDLL_QUEUE *entry)
 * @brief       Inserts an entry at the head of the queue.
 * @param[out]  que     Pointer to the queue
 * @param[out]  entry   Pointer to an entry
 */
void QueueAddHead(CDLL_QUEUE *que, CDLL_QUEUE *entry);

/**
 * @fn          void QueueAddTail(CDLL_QUEUE *que, CDLL_QUEUE *entry)
 * @brief       Inserts an entry at the tail of the queue.
 * @param[out]  que     Pointer to the queue
 * @param[out]  entry   Pointer to an entry
 */
void QueueAddTail(CDLL_QUEUE *que, CDLL_QUEUE *entry);

/**
 * @fn          void QueueRemoveEntry(CDLL_QUEUE *entry)
 * @brief       Removes an entry from the queue.
 * @param[out]  entry   Pointer to an entry of the queue
 */
void QueueRemoveEntry(CDLL_QUEUE *entry);

/**
 * @fn          CDLL_QUEUE* QueueRemoveHead(CDLL_QUEUE *que)
 * @brief       Remove and return an entry at the head of the queue.
 * @param[out]  que   Pointer to the queue
 * @return      Returns a pointer to an entry at the head of the queue
 */
CDLL_QUEUE* QueueRemoveHead(CDLL_QUEUE *que);

/**
 * @fn          CDLL_QUEUE* QueueRemoveTail(CDLL_QUEUE *que)
 * @brief       Remove and return an entry at the tail of the queue.
 * @param[out]  que   Pointer to the queue
 * @return      Returns a pointer to an entry at the tail of the queue
 */
CDLL_QUEUE* QueueRemoveTail(CDLL_QUEUE *que);

#endif /* _KNL_LIB_H_ */

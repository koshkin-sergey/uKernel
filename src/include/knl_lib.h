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

/*******************************************************************************
 *  typedefs and structures (scope: module-local)
 ******************************************************************************/

typedef struct {
  TN_TCB *curr;                     // Task that is running now
  TN_TCB *next;                     // Task to be run after switch context
} knlRun_t;

typedef struct {
  knlRun_t run;
  uint32_t ready_to_run_bmp;
  CDLL_QUEUE ready_list[NUM_PRIORITY];    // all ready to run(RUNNABLE) tasks
} knlInfo_t;

/*******************************************************************************
 *  exported variables
 ******************************************************************************/

extern knlInfo_t knlInfo;

/*******************************************************************************
 *  exported function prototypes
 ******************************************************************************/

__STATIC_FORCEINLINE
TN_TCB* knlThreadGetCurrent(void)
{
  return knlInfo.run.curr;
}

__STATIC_FORCEINLINE
TN_TCB* knlThreadGetNext(void)
{
  return knlInfo.run.next;
}

__STATIC_FORCEINLINE
void knlThreadSetCurrent(TN_TCB *thread)
{
  knlInfo.run.curr = thread;
}

__STATIC_FORCEINLINE
void knlThreadSetNext(TN_TCB *thread)
{
  knlInfo.run.next = thread;
}

/* Thread */
void knlThreadSetReady(TN_TCB *thread);
bool knlThreadWaitComplete(TN_TCB *task);
void knlThreadToWaitAction(TN_TCB *task, CDLL_QUEUE *wait_que, int wait_reason,
                           TIME_t timeout);
void knlThreadChangePriority(TN_TCB *task, int32_t new_priority);
void knlThreadSetPriority(TN_TCB *task, int32_t priority);
void knlThreadWaitDelete(CDLL_QUEUE *que);
void knlThreadExit(void);

#endif /* _KNL_LIB_H_ */

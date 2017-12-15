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
 * Copyright © 2011-2016 Sergey Koshkin <koshkin.sergey@gmail.com>
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
 * @file
 *
 * Kernel system routines.
 *
 */

/*******************************************************************************
 *  includes
 ******************************************************************************/

#include "knl_lib.h"

/*******************************************************************************
 *  external declarations
 ******************************************************************************/

/*******************************************************************************
 *  defines and macros (scope: module-local)
 ******************************************************************************/

/*******************************************************************************
 *  typedefs and structures (scope: module-local)
 ******************************************************************************/

/*******************************************************************************
 *  global variable definitions  (scope: module-exported)
 ******************************************************************************/

/*******************************************************************************
 *  global variable definitions (scope: module-local)
 ******************************************************************************/

/*******************************************************************************
 *  function prototypes (scope: module-local)
 ******************************************************************************/

__SVC(0)
osError_t svcTask(osError_t (*)(osTask_t*), osTask_t*);
__SVC(0)
void svcTaskCreate(void (*)(osTask_t*, const task_create_attr_t*), osTask_t*, const task_create_attr_t*);
__SVC(0)
void svcTaskExit(void (*)(task_exit_attr_t), task_exit_attr_t);
__SVC(0)
void svcTaskSleep(void (*)(osTime_t), osTime_t);
__SVC(0)
osError_t svcTaskSetPriority(osError_t (*)(osTask_t*, uint32_t), osTask_t*, uint32_t);
__SVC(0)
osTime_t svcTaskGetTime(osTime_t (*)(osTask_t*), osTask_t*);

static
void TaskWaitExit(osTask_t *task, uint32_t ret_val);
static
void TaskWaitExit_Handler(osTask_t *task);
static
osError_t TaskActivate(osTask_t *task);

/*******************************************************************************
 *  function implementations (scope: module-local)
 ******************************************************************************/

__STATIC_FORCEINLINE
uint32_t* TaskRegPtr(osTask_t *task)
{
  uint32_t *addr;
  
  if (task != TaskGetCurrent())
    addr = (uint32_t *)(task->stk + STACK_OFFSET_R0());
  else
    addr = (uint32_t *)__get_PSP();
  
  return addr;
}

/**
 * @brief
 * @param
 * @return
 */
static
void TaskDispatch(void)
{
  int32_t priority = 0;

#if defined USE_ASM_FFS
  priority = ffs_asm(knlInfo.ready_to_run_bmp);
  priority--;
#else
  uint32_t run_bmp = knlInfo.ready_to_run_bmp;

  for (int i = 0; i < BITS_IN_INT; ++i) {
    if (run_bmp & (1UL << i)) {
      priority = i;
      break;
    }
  }
#endif

  TaskSetNext(GetTaskByQueue(knlInfo.ready_list[priority].next));
}

/**
 * @brief
 * @param
 * @return
 */
void TaskToRunnable(osTask_t *task)
{
  task->state = TSK_STATE_RUNNABLE;
  task->pwait_queue = NULL;

  /* Add the task to the end of 'ready queue' for the current priority */
  TaskSetReady(task);

  /* less value - greater priority, so '<' operation is used here */
  if (task->priority < TaskGetNext()->priority) {
    TaskSetNext(task);
  }
}

/**
 * @brief
 * @param
 * @return
 */
static
void TaskToNonRunnable(osTask_t *task)
{
  uint32_t priority = task->priority;
  queue_t *que = &(knlInfo.ready_list[priority]);

  /* Remove the current task from any queue (now - from ready queue) */
  QueueRemoveEntry(&task->task_queue);

  if (isQueueEmpty(que)) {
    /* No ready tasks for the curr priority */
    /* Remove 'ready to run' from the curr priority */
    knlInfo.ready_to_run_bmp &= ~(1 << priority);

    /* Find highest priority ready to run -
       at least, MSB bit must be set for the idle task */
    TaskDispatch();
  }
  else if (task == TaskGetNext()) {
    /* There are 'ready to run' task(s) for the curr priority */
    TaskSetNext(GetTaskByQueue(que->next));
  }
}

void TaskWaitEnter(osTask_t *task, queue_t * wait_que, wait_reason_t wait_reason, osTime_t timeout)
{
  TaskToNonRunnable(task);

  task->state = TSK_STATE_WAIT;
  task->wait_reason = wait_reason;

  /* Add to the wait queue - FIFO */
  if (wait_que != NULL) {
    QueueAddTail(wait_que, &(task->task_queue));
    task->pwait_queue = wait_que;
  }

  /* Add to the timers queue */
  if (timeout != TIME_WAIT_INFINITE)
    TimerInsert(&task->wait_timer, knlInfo.jiffies + timeout, (CBACK)TaskWaitExit_Handler, task);
}

/**
 * @brief
 * @param
 * @return
 */
static
void TaskWaitExit(osTask_t *task, uint32_t ret_val)
{
#ifdef USE_MUTEXES

  queue_t *que = NULL;

  if (task->wait_reason & (WAIT_REASON_MUTEX_I | WAIT_REASON_MUTEX_C)) {
    que = task->pwait_queue;
  }

#endif

  task->pwait_queue = NULL;
  QueueRemoveEntry(&task->task_queue);

  uint32_t *reg = TaskRegPtr(task);
  *reg = ret_val;

  if (!(task->state & TSK_STATE_SUSPEND)) {
    TaskToRunnable(task);
  }
  else {
    //-- remove WAIT state
    task->state = TSK_STATE_SUSPEND;
  }

#ifdef USE_MUTEXES

  if (que && !isQueueEmpty(que)) {
    osMutex_t *mutex = GetMutexByWaitQueque(que);
    osTask_t *holder = mutex->holder;

    if (holder != NULL) {
      /* If task was blocked by another task and its priority was changed */
      /* Recalculate current priority */
      if (holder->priority != holder->base_priority && holder->priority == task->priority) {
        MutexSetPriority(holder, MutexGetMaxPriority(mutex, holder->base_priority));
      }
    }
  }

#endif

  task->wait_reason = WAIT_REASON_NO;
}

/**
 * @brief
 * @param
 * @return
 */
static
void TaskWaitExit_Handler(osTask_t *task)
{
  BEGIN_CRITICAL_SECTION
  TaskWaitExit(task, (uint32_t)TERR_TIMEOUT);
  END_CRITICAL_SECTION
}

void TaskWaitComplete(osTask_t *task, uint32_t ret_val)
{
  TimerDelete(&task->wait_timer);
  TaskWaitExit(task, ret_val);
}

void TaskWaitDelete(queue_t *wait_que)
{
  while (!isQueueEmpty(wait_que)) {
    TaskWaitComplete(GetTaskByQueue(QueueRemoveHead(wait_que)), (uint32_t)TERR_DLT);
  }
}

/**
 * @brief
 * @param
 * @return
 */
static
void TaskSetDormantState(osTask_t* task)
{
  QueueReset(&(task->task_queue));
  QueueReset(&(task->wait_timer.queue));
#ifdef USE_MUTEXES
  QueueReset(&(task->mutex_queue));
#endif

  task->pwait_queue = NULL;
  task->priority = task->base_priority;
  task->state = TSK_STATE_DORMANT;
  task->wait_reason = WAIT_REASON_NO;
  task->tslice_count = 0;
}

__FORCEINLINE
osTask_t* TaskGetCurrent(void)
{
  return knlInfo.run.curr;
}

__FORCEINLINE
void TaskSetCurrent(osTask_t *task)
{
  knlInfo.run.curr = task;
}

__FORCEINLINE
osTask_t* TaskGetNext(void)
{
  return knlInfo.run.next;
}

void TaskSetNext(osTask_t *task)
{
  knlInfo.run.next = task;
  archSwitchContextRequest();
}

/**
 * @fn    void TaskSetReady(osTask_t *thread)
 * @brief Adds task to the end of ready queue for current priority
 * @param thread
 */
void TaskSetReady(osTask_t *thread)
{
  knlInfo_t *info = &knlInfo;
  int32_t priority = thread->priority;

  QueueAddTail(&info->ready_list[priority], &thread->task_queue);
  info->ready_to_run_bmp |= (1 << priority);
}

void TaskChangePriority(osTask_t *task, uint32_t new_priority)
{
  knlInfo_t *info = &knlInfo;
  uint32_t old_priority = task->priority;

  //-- remove curr task from any (wait/ready) queue
  QueueRemoveEntry(&task->task_queue);

  //-- If there are no ready tasks for the old priority
  //-- clear ready bit for old priority
  if (isQueueEmpty(&info->ready_list[old_priority]))
    info->ready_to_run_bmp &= ~(1 << old_priority);

  task->priority = new_priority;

  //-- Add task to the end of ready queue for current priority
  TaskSetReady(task);
  TaskDispatch();
}

/**
 * @fn          void TaskCreate(osTask_t *task, const task_create_attr_t *attr)
 * @param[out]  task  Pointer to the task TCB to be created
 * @param[in]   attr  Pointer to the structure with the attributes of the task to be created
 */
void TaskCreate(osTask_t *task, const task_create_attr_t *attr)
{
  /* Init task TCB */
  task->func_addr = attr->func_addr;
  task->func_param = attr->func_param;
  task->stk_start = attr->stk_start;
  task->stk_size = attr->stk_size;
  task->base_priority = attr->priority;
  task->id = ID_TASK;
  task->time = 0;

  /* Fill all task stack space by TN_FILL_STACK_VAL - only inside create_task */
  uint32_t *ptr = task->stk_start;
  for (uint32_t i = 0; i < task->stk_size; i++) {
    *ptr-- = TN_FILL_STACK_VAL;
  }

  TaskSetDormantState(task);

  if (attr->option & osTaskStarOnCreating)
    TaskActivate(task);
}

/**
 * @fn          osError_t TaskDelete(osTask_t *task)
 * @param[out]  task  Pointer to the task TCB to be deleted
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WCONTEXT     Unacceptable system's state for this request
 */
static
osError_t TaskDelete(osTask_t *task)
{
  /* Cannot delete not-terminated task */
  if (task->state != TSK_STATE_DORMANT)
    return TERR_WCONTEXT;

  task->id = ID_INVALID;

  return TERR_NO_ERR;
}

/**
 * @fn          osError_t TaskActivate(osTask_t *task)
 * @param[out]  task  Pointer to the task TCB to be activated
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_OVERFLOW     Task is already active (not in DORMANT state)
 */
static
osError_t TaskActivate(osTask_t *task)
{
  if (task->state != TSK_STATE_DORMANT)
    return TERR_OVERFLOW;

  StackInit(task);
  TaskToRunnable(task);

  return TERR_NO_ERR;
}

/**
 * @fn          osError_t TaskTerminate(osTask_t *task)
 * @param[out]  task  Pointer to the task TCB to be terminated
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WCONTEXT     Unacceptable system state for this request
 */
static
osError_t TaskTerminate(osTask_t *task)
{
  /* Cannot terminate running task */
  if (task->state == TSK_STATE_DORMANT || task == TaskGetCurrent())
    return TERR_WCONTEXT;

  if (task->state == TSK_STATE_RUNNABLE) {
    TaskToNonRunnable(task);
  }
  else if (task->state & TSK_STATE_WAIT) {
    /* Free all queues, involved in the 'waiting' */
    QueueRemoveEntry(&task->task_queue);
    TimerDelete(&task->wait_timer);
  }

  /* Unlock all mutexes, locked by the task */
#ifdef USE_MUTEXES
  queue_t *que;
  osMutex_t *mutex;

  while (!isQueueEmpty(&task->mutex_queue)) {
    que = QueueRemoveHead(&task->mutex_queue);
    mutex = GetMutexByMutexQueque(que);
    MutexUnLock(mutex);
  }
#endif

  TaskSetDormantState(task);

  return TERR_NO_ERR;
}

/**
 * @fn        void TaskExit(task_exit_attr_t attr)
 * @brief     Terminates the currently running task
 * @param[in] attr  Exit option. Option values:
 *                    TASK_EXIT             Currently running task will be terminated.
 *                    TASK_EXIT_AND_DELETE  Currently running task will be terminated and then deleted
 */
static
void TaskExit(task_exit_attr_t attr)
{
  osTask_t *task = TaskGetCurrent();

  /* Unlock all mutexes, locked by the task */
#ifdef USE_MUTEXES
  queue_t *que;
  osMutex_t *mutex;

  while (!isQueueEmpty(&task->mutex_queue)) {
    que = QueueRemoveHead(&task->mutex_queue);
    mutex = GetMutexByMutexQueque(que);
    MutexUnLock(mutex);
  }
#endif

  TaskToNonRunnable(task);
  TaskSetDormantState(task);

  if (attr == TASK_EXIT_AND_DELETE) {
    task->id = ID_INVALID;
  }
}

/**
 * @fn          osError_t TaskSuspend(osTask_t *task)
 * @param[out]  task  Pointer to the task TCB to be suspended
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_OVERFLOW     Task already suspended
 *              TERR_WSTATE       Task is not active (i.e in DORMANT state )
 */
static
osError_t TaskSuspend(osTask_t *task)
{
  if (task->state & TSK_STATE_SUSPEND)
    return TERR_OVERFLOW;
  if (task->state == TSK_STATE_DORMANT)
    return TERR_WSTATE;

  if (task->state == TSK_STATE_RUNNABLE) {
    task->state = TSK_STATE_SUSPEND;
    TaskToNonRunnable(task);
  }
  else {
    task->state |= TSK_STATE_SUSPEND;
  }

  return TERR_NO_ERR;
}

/**
 * @fn          osError_t TaskResume(osTask_t *task)
 * @param[out]  task  Pointer to task TCB to be resumed
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_WSTATE       Task is not in SUSPEDED or WAITING_SUSPEDED state
 *              TERR_NOEXS        Object is not a task or non-existent
 *              TERR_ISR          The function cannot be called from interrupt service routines
 */
static
osError_t TaskResume(osTask_t *task)
{
  if (!(task->state & TSK_STATE_SUSPEND))
    return TERR_WSTATE;

  if (!(task->state & TSK_STATE_WAIT))
    /* The task is not in the WAIT-SUSPEND state */
    TaskToRunnable(task);
  else
    /* Just remove TSK_STATE_SUSPEND from the task state */
    task->state &= ~TSK_STATE_SUSPEND;

  return TERR_NO_ERR;
}

/**
 * @fn        void TaskSleep(osTime_t timeout)
 * @param[in] timeout   Timeout value must be greater than 0.
 */
static
void TaskSleep(osTime_t timeout)
{
  TaskWaitEnter(TaskGetCurrent(), NULL, WAIT_REASON_SLEEP, timeout);
}

/**
 * @fn          osError_t TaskWakeup(osTask_t *task)
 * @param[out]  task  Pointer to the task TCB to be wake up
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WSTATE       Task is not in WAIT state
 */
static
osError_t TaskWakeup(osTask_t *task)
{
  if (!(task->state & TSK_STATE_WAIT) || (task->wait_reason != WAIT_REASON_SLEEP))
    return TERR_WSTATE;

  TaskWaitComplete(task, (uint32_t)TERR_NO_ERR);

  return TERR_NO_ERR;
}

/**
 * @fn          osError_t TaskReleaseWait(osTask_t *task)
 * @param[out]  task  Pointer to the task TCB to be released from waiting or sleep
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WCONTEXT     Unacceptable system's state for function's request executing
 */
static
osError_t TaskReleaseWait(osTask_t *task)
{
  if (!(task->state & TSK_STATE_WAIT))
    return TERR_WCONTEXT;

  TaskWaitComplete(task, (uint32_t)TERR_NO_ERR);

  return TERR_NO_ERR;
}

static
osError_t TaskSetPriority(osTask_t *task, uint32_t new_priority)
{
  if (new_priority == 0)
    new_priority = task->base_priority;

  if (task->state == TSK_STATE_DORMANT)
    return TERR_WCONTEXT;

  if (task->state == TSK_STATE_RUNNABLE)
    TaskChangePriority(task, new_priority);
  else
    task->priority = new_priority;

  return TERR_NO_ERR;
}

static
osTime_t TaskGetTime(osTask_t *task)
{
  return task->time;
}

osTime_t osTaskGetTime(osTask_t *task)
{
  if (task == NULL)
    return 0;
  if (task->id != ID_TASK)
    return 0;
  if (IS_IRQ_MODE() || IS_IRQ_MASKED())
    return 0;

  return svcTaskGetTime(TaskGetTime, task);
}

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
osError_t osTaskCreate(osTask_t *task,
                       void (*func)(void *),
                       uint32_t priority,
                       const uint32_t *stack_start,
                       uint32_t stack_size,
                       const void *param,
                       uint32_t option)
{
  if (priority == 0U || priority >= (NUM_PRIORITY-1))
    return TERR_WRONG_PARAM;
  if ((stack_size < osStackSizeMin) || (func == NULL) || (task == NULL)
    || (stack_start == NULL) || (task->id != 0) )
    return TERR_WRONG_PARAM;
  if (IS_IRQ_MODE() || IS_IRQ_MASKED())
    return TERR_ISR;

  task_create_attr_t task_create_attr;

  task_create_attr.func_addr = (void *)func;
  task_create_attr.func_param = param;
  task_create_attr.stk_start = (uint32_t *)stack_start;
  task_create_attr.stk_size = stack_size;
  task_create_attr.priority = priority;
  task_create_attr.option = option;

  svcTaskCreate(TaskCreate, task, &task_create_attr);

  return TERR_NO_ERR;
}

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
osError_t osTaskDelete(osTask_t *task)
{
  if (task == NULL)
    return TERR_WRONG_PARAM;
  if (task->id != ID_TASK)
    return TERR_NOEXS;
  if (IS_IRQ_MODE() || IS_IRQ_MASKED())
    return TERR_ISR;

  return svcTask(TaskDelete, task);
}

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
osError_t osTaskActivate(osTask_t *task)
{
  if (task == NULL)
    return TERR_WRONG_PARAM;
  if (task->id != ID_TASK)
    return TERR_NOEXS;
  if (IS_IRQ_MODE() || IS_IRQ_MASKED())
    return TERR_ISR;

  return svcTask(TaskActivate, task);
}

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
osError_t osTaskTerminate(osTask_t *task)
{
  if (task == NULL)
    return TERR_WRONG_PARAM;
  if (task->id != ID_TASK)
    return TERR_NOEXS;
  if (IS_IRQ_MODE() || IS_IRQ_MASKED())
    return TERR_ISR;

  return svcTask(TaskTerminate, task);
}

/**
 * @fn        void osTaskExit(task_exit_attr_t attr)
 * @brief     Terminates the currently running task
 * @param[in] attr  Exit option. Option values:
 *                    TASK_EXIT             Currently running task will be terminated.
 *                    TASK_EXIT_AND_DELETE  Currently running task will be terminated and then deleted
 */
void osTaskExit(task_exit_attr_t attr)
{
  if (IS_IRQ_MODE() || IS_IRQ_MASKED())
    return;

  svcTaskExit(TaskExit, attr);
}

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
osError_t osTaskSuspend(osTask_t *task)
{
  if (task == NULL)
    return TERR_WRONG_PARAM;
  if (task->id != ID_TASK)
    return TERR_NOEXS;
  if (IS_IRQ_MODE() || IS_IRQ_MASKED())
    return TERR_ISR;

  return svcTask(TaskSuspend, task);
}

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
osError_t osTaskResume(osTask_t *task)
{
  if (task == NULL)
    return TERR_WRONG_PARAM;
  if (task->id != ID_TASK)
    return TERR_NOEXS;
  if (IS_IRQ_MODE() || IS_IRQ_MASKED())
    return TERR_ISR;

  return svcTask(TaskResume, task);
}

/**
 * @fn        osError_t osTaskSleep(osTime_t timeout)
 * @brief     Puts the currently running task to the sleep for at most timeout system ticks.
 * @param[in] timeout   Timeout value must be greater than 0.
 *                      A value of TIME_WAIT_INFINITE causes an infinite delay.
 * @return    TERR_NO_ERR       Normal completion
 *            TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *            TERR_ISR          The function cannot be called from interrupt service routines
 */
osError_t osTaskSleep(osTime_t timeout)
{
  if (timeout == 0)
    return TERR_WRONG_PARAM;
  if (IS_IRQ_MODE() || IS_IRQ_MASKED())
    return TERR_ISR;

  svcTaskSleep(TaskSleep, timeout);

  return TERR_NO_ERR;
}

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
osError_t osTaskWakeup(osTask_t *task)
{
  if (task == NULL)
    return TERR_WRONG_PARAM;
  if (task->id != ID_TASK)
    return TERR_NOEXS;
  if (IS_IRQ_MODE() || IS_IRQ_MASKED())
    return TERR_ISR;

  return svcTask(TaskWakeup, task);
}

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
osError_t osTaskReleaseWait(osTask_t *task)
{
  if (task == NULL)
    return TERR_WRONG_PARAM;
  if (task->id != ID_TASK)
    return TERR_NOEXS;
  if (IS_IRQ_MODE() || IS_IRQ_MASKED())
    return TERR_ISR;

  return svcTask(TaskReleaseWait, task);
}

osError_t osTaskSetPriority(osTask_t *task, uint32_t new_priority)
{
  if (task == NULL)
    return TERR_WRONG_PARAM;
  if (task->id != ID_TASK)
    return TERR_NOEXS;
  if (new_priority > (NUM_PRIORITY - 2))
    return TERR_WRONG_PARAM;
  if (IS_IRQ_MODE() || IS_IRQ_MASKED())
    return TERR_ISR;

  return svcTaskSetPriority(TaskSetPriority, task, new_priority);
}

/* ----------------------------- End of file ---------------------------------*/

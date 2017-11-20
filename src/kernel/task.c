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
#include "timer.h"
#include "utils.h"

/*******************************************************************************
 *  external declarations
 ******************************************************************************/

extern int find_max_blocked_priority(TN_MUTEX *mutex, int ref_priority);
extern int do_unlock_mutex(TN_MUTEX *mutex);

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

__svc_indirect(0)
osError_t svcTask(osError_t (*)(TN_TCB*), TN_TCB*);
__svc_indirect(0)
void svcTaskCreate(void (*)(TN_TCB*, const task_create_attr_t*), TN_TCB*, const task_create_attr_t*);
__svc_indirect(0)
void svcTaskExit(void (*)(task_exit_attr_t), task_exit_attr_t);
__svc_indirect(0)
void svcTaskSleep(void (*)(TIME_t), TIME_t);
__svc_indirect(0)
osError_t svcTaskSetPriority(osError_t (*)(TN_TCB*, uint32_t), TN_TCB*, uint32_t);
__svc_indirect(0)
TIME_t svcTaskGetTime(TIME_t (*)(TN_TCB*), TN_TCB*);

/*******************************************************************************
 *  function implementations (scope: module-local)
 ******************************************************************************/

/**
 * @brief
 * @param
 * @return
 */
static
void ThreadDispatch(void)
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

  knlThreadSetNext(get_task_by_tsk_queue(knlInfo.ready_list[priority].next));
  SwitchContextRequest();
}

/**
 * @brief
 * @param
 * @return
 */
static
void TaskToRunnable(TN_TCB *task)
{
  task->task_state = TSK_STATE_RUNNABLE;
  task->pwait_queue = NULL;

  /* Add the task to the end of 'ready queue' for the current priority */
  ThreadSetReady(task);

  /* less value - greater priority, so '<' operation is used here */
  if (task->priority < knlThreadGetNext()->priority) {
    knlThreadSetNext(task);
    SwitchContextRequest();
  }
}

/**
 * @brief
 * @param
 * @return
 */
static
void TaskToNonRunnable(TN_TCB *task)
{
  int32_t priority = task->priority;
  CDLL_QUEUE *que = &(knlInfo.ready_list[priority]);

  /* Remove the current task from any queue (now - from ready queue) */
  queue_remove_entry(&(task->task_queue));

  if (is_queue_empty(que)) {
    /* No ready tasks for the curr priority */
    /* Remove 'ready to run' from the curr priority */
    knlInfo.ready_to_run_bmp &= ~(1 << priority);

    /* Find highest priority ready to run -
       at least, MSB bit must be set for the idle task */
    ThreadDispatch();
  }
  else {
    /* There are 'ready to run' task(s) for the curr priority */
    if (knlThreadGetNext() == task) {
      knlThreadSetNext(get_task_by_tsk_queue(que->next));
      SwitchContextRequest();
    }
  }
}

/**
 * @brief
 * @param
 * @return
 */
static
void task_wait_release(TN_TCB *task)
{
#ifdef USE_MUTEXES
  int fmutex;
  int curr_priority;
  TN_MUTEX * mutex;
  TN_TCB * mt_holder_task;
  CDLL_QUEUE * t_que;

  t_que = NULL;
  if (task->task_wait_reason == TSK_WAIT_REASON_MUTEX_I
    || task->task_wait_reason == TSK_WAIT_REASON_MUTEX_C) {
    fmutex = 1;
    t_que = task->pwait_queue;
  }
  else
    fmutex = 0;
#endif

  task->pwait_queue = NULL;

  if (!(task->task_state & TSK_STATE_SUSPEND)) {
    TaskToRunnable(task);
  }
  else {
    //-- remove WAIT state
    task->task_state = TSK_STATE_SUSPEND;
  }

#ifdef USE_MUTEXES

  if (fmutex) {
    mutex = get_mutex_by_wait_queque(t_que);

    mt_holder_task = mutex->holder;
    if (mt_holder_task != NULL) {
      //-- if task was blocked by another task and its pri was changed
      //--  - recalc current priority

      if (mt_holder_task->priority != mt_holder_task->base_priority
        && mt_holder_task->priority == task->priority) {
        curr_priority = find_max_blocked_priority(
          mutex, mt_holder_task->base_priority);

        ThreadSetPriority(mt_holder_task, curr_priority);
      }
    }
  }
#endif

  task->task_wait_reason = 0; //-- Clear wait reason
}

/**
 * @brief
 * @param
 * @return
 */
static
void task_set_dormant_state(TN_TCB* task)
{
  queue_reset(&(task->task_queue));
  queue_reset(&(task->wtmeb.queue));
#ifdef USE_MUTEXES
  queue_reset(&(task->mutex_queue));
#endif

  task->pwait_queue = NULL;
  task->priority = task->base_priority; //-- Task curr priority
  task->task_state = TSK_STATE_DORMANT;   //-- Task state
  task->task_wait_reason = 0;              //-- Reason for waiting
  task->wercd = NULL;
  task->tslice_count = 0;
}

/**
 * @brief
 * @param
 * @return
 */
static
void task_wait_release_handler(TN_TCB *task)
{
  if (task == NULL)
    return;

  queue_remove_entry(&task->task_queue);
  task_wait_release(task);

  if (task->wercd != NULL)
    *task->wercd = TERR_TIMEOUT;
}

/*******************************************************************************
 *  function implementations (scope: module-exported)
 ******************************************************************************/

/**
 * @fn    void ThreadSetReady(TN_TCB *thread)
 * @brief Adds task to the end of ready queue for current priority
 * @param thread
 */
void ThreadSetReady(TN_TCB *thread)
{
  knlInfo_t *info = &knlInfo;
  int32_t priority = thread->priority;

  queue_add_tail(&info->ready_list[priority], &thread->task_queue);
  info->ready_to_run_bmp |= (1 << priority);
}

/*-----------------------------------------------------------------------------*
 * Название : ThreadWaitComplete
 * Описание : Выводит задачу из состояния ожидания и удаляет из очереди таймеров
 * Параметры: task - Указатель на задачу
 * Результат: Возвращает true при успешном выполнении, иначе возвращает false
 *----------------------------------------------------------------------------*/
void ThreadWaitComplete(TN_TCB *task)
{
  if (task == NULL)
    return;

  timer_delete(&task->wtmeb);
  task_wait_release(task);

  if (task->wercd != NULL)
    *task->wercd = TERR_NO_ERR;
}

void ThreadChangePriority(TN_TCB * task, int32_t new_priority)
{
  knlInfo_t *info = &knlInfo;
  int32_t old_priority = task->priority;

  //-- remove curr task from any (wait/ready) queue
  queue_remove_entry(&(task->task_queue));

  //-- If there are no ready tasks for the old priority
  //-- clear ready bit for old priority
  if (is_queue_empty(&info->ready_list[old_priority]))
    info->ready_to_run_bmp &= ~(1 << old_priority);

  task->priority = new_priority;

  //-- Add task to the end of ready queue for current priority
  ThreadSetReady(task);
  ThreadDispatch();
}

#ifdef USE_MUTEXES

void ThreadSetPriority(TN_TCB * task, int32_t priority)
{
  TN_MUTEX * mutex;

  //-- transitive priority changing

  // if we have a task A that is blocked by the task B and we changed priority
  // of task A,priority of task B also have to be changed. I.e, if we have
  // the task A that is waiting for the mutex M1 and we changed priority
  // of this task, a task B that holds a mutex M1 now, also needs priority's
  // changing.  Then, if a task B now is waiting for the mutex M2, the same
  // procedure have to be applied to the task C that hold a mutex M2 now
  // and so on.

  //-- This function in ver 2.6 is more "lightweight".
  //-- The code is derived from Vyacheslav Ovsiyenko version

  for (;;) {
    if (task->priority <= priority)
      return;

    if (task->task_state == TSK_STATE_RUNNABLE) {
      ThreadChangePriority(task, priority);
      return;
    }

    if (task->task_state & TSK_STATE_WAIT) {
      if (task->task_wait_reason == TSK_WAIT_REASON_MUTEX_I) {
        task->priority = priority;

        mutex = get_mutex_by_wait_queque(task->pwait_queue);
        task = mutex->holder;

        continue;
      }
    }

    task->priority = priority;
    return;
  }
}

#endif

void ThreadWaitDelete(CDLL_QUEUE *wait_que)
{
  CDLL_QUEUE *que;
  TN_TCB *task;

  while (!is_queue_empty(wait_que)) {
    que = queue_remove_head(wait_que);
    task = get_task_by_tsk_queue(que);
    ThreadWaitComplete(task);
    if (task->wercd != NULL)
      *task->wercd = TERR_DLT;
  }
}

void ThreadToWaitAction(TN_TCB *task, CDLL_QUEUE * wait_que, int wait_reason,
                         TIME_t timeout)
{
  TaskToNonRunnable(task);

  task->task_state = TSK_STATE_WAIT;
  task->task_wait_reason = wait_reason;

  /* Add to the wait queue - FIFO */
  if (wait_que != NULL) {
    queue_add_tail(wait_que, &(task->task_queue));
    task->pwait_queue = wait_que;
  }

  /* Add to the timers queue */
  if (timeout != TN_WAIT_INFINITE)
    timer_insert(&task->wtmeb, timeout, (CBACK)task_wait_release_handler, task);
}

/**
 * @fn          void TaskCreate(TN_TCB *task, const task_create_attr_t *attr)
 * @param[out]  task  Pointer to the task TCB to be created
 * @param[in]   attr  Pointer to the structure with the attributes of the task to be created
 */
void TaskCreate(TN_TCB *task, const task_create_attr_t *attr)
{
  /* Init task TCB */
  task->func_addr = attr->func_addr;
  task->func_param = attr->func_param;
  task->stk_start = attr->stk_start;
  task->stk_size = attr->stk_size;
  task->base_priority = attr->priority;
  task->id_task = TN_ID_TASK;
  task->time = 0;
  task->wercd = NULL;

  /* Fill all task stack space by TN_FILL_STACK_VAL - only inside create_task */
  uint32_t *ptr = task->stk_start;
  for (uint32_t i = 0; i < task->stk_size; i++) {
    *ptr-- = TN_FILL_STACK_VAL;
  }

  task_set_dormant_state(task);

  /* Add task to created task queue */
  queue_add_tail(&tn_create_queue, &(task->create_queue));
  tn_created_tasks_qty++;

  if ((attr->option & TN_TASK_START_ON_CREATION) != 0) {
    StackInit(task);
    TaskToRunnable(task);
  }
}

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
osError_t osTaskCreate(TN_TCB *task,
                       void (*func)(void *),
                       int32_t priority,
                       const uint32_t *stack_start,
                       int32_t stack_size,
                       const void *param,
                       int32_t option)
{
  /* Light weight checking of system tasks recreation */

  if ( ((priority == 0) && ((option & TN_TASK_TIMER) == 0))
    || ((priority == (NUM_PRIORITY-1)) && ((option & TN_TASK_IDLE) == 0)) )
    return TERR_WRONG_PARAM;

  if ( ((priority < 0) || (priority > (NUM_PRIORITY - 1)))
    || (stack_size < TN_MIN_STACK_SIZE) || (func == NULL) || (task == NULL)
    || (stack_start == NULL) || (task->id_task != 0) )
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
 * @fn          osError_t TaskDelete(TN_TCB *task)
 * @param[out]  task  Pointer to the task TCB to be deleted
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WCONTEXT     Unacceptable system's state for this request
 */
static
osError_t TaskDelete(TN_TCB *task)
{
  /* Cannot delete not-terminated task */
  if (task->task_state != TSK_STATE_DORMANT)
    return TERR_WCONTEXT;

  queue_remove_entry(&(task->create_queue));
  tn_created_tasks_qty--;
  task->id_task = 0;

  return TERR_NO_ERR;
}

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
osError_t osTaskDelete(TN_TCB *task)
{
  if (task == NULL)
    return TERR_WRONG_PARAM;
  if (task->id_task != TN_ID_TASK)
    return TERR_NOEXS;
  if (IS_IRQ_MODE() || IS_IRQ_MASKED())
    return TERR_ISR;

  return svcTask(TaskDelete, task);
}

/**
 * @fn          osError_t TaskActivate(TN_TCB *task)
 * @param[out]  task  Pointer to the task TCB to be activated
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_OVERFLOW     Task is already active (not in DORMANT state)
 */
static
osError_t TaskActivate(TN_TCB *task)
{
  if (task->task_state != TSK_STATE_DORMANT)
    return TERR_OVERFLOW;

  StackInit(task);
  TaskToRunnable(task);

  return TERR_NO_ERR;
}

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
osError_t osTaskActivate(TN_TCB *task)
{
  if (task == NULL)
    return TERR_WRONG_PARAM;
  if (task->id_task != TN_ID_TASK)
    return TERR_NOEXS;
  if (IS_IRQ_MODE() || IS_IRQ_MASKED())
    return TERR_ISR;

  return svcTask(TaskActivate, task);
}

/**
 * @fn          osError_t TaskTerminate(TN_TCB *task)
 * @param[out]  task  Pointer to the task TCB to be terminated
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WCONTEXT     Unacceptable system state for this request
 */
static
osError_t TaskTerminate(TN_TCB *task)
{
  /* Cannot terminate running task */
  if (task->task_state == TSK_STATE_DORMANT || task == ThreadGetCurrent())
    return TERR_WCONTEXT;

  if (task->task_state == TSK_STATE_RUNNABLE) {
    TaskToNonRunnable(task);
  }
  else if (task->task_state & TSK_STATE_WAIT) {
    /* Free all queues, involved in the 'waiting' */
    queue_remove_entry(&(task->task_queue));
    timer_delete(&task->wtmeb);
  }

  //-- Unlock all mutexes, locked by the task
#ifdef USE_MUTEXES
  CDLL_QUEUE *que;
  TN_MUTEX *mutex;

  while (!is_queue_empty(&(task->mutex_queue))) {
    que = queue_remove_head(&(task->mutex_queue));
    mutex = get_mutex_by_mutex_queque(que);
    do_unlock_mutex(mutex);
  }
#endif

  task_set_dormant_state(task);

  return TERR_NO_ERR;
}

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
osError_t osTaskTerminate(TN_TCB *task)
{
  if (task == NULL)
    return TERR_WRONG_PARAM;
  if (task->id_task != TN_ID_TASK)
    return TERR_NOEXS;
  if (IS_IRQ_MODE() || IS_IRQ_MASKED())
    return TERR_ISR;

  return svcTask(TaskTerminate, task);
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
  TN_TCB *task = ThreadGetCurrent();

  /* Unlock all mutexes, locked by the task */
#ifdef USE_MUTEXES
  CDLL_QUEUE *que;
  TN_MUTEX *mutex;

  while (!is_queue_empty(&(task->mutex_queue))) {
    que = queue_remove_head(&(task->mutex_queue));
    mutex = get_mutex_by_mutex_queque(que);
    do_unlock_mutex(mutex);
  }
#endif

  TaskToNonRunnable(task);
  task_set_dormant_state(task);

  if (attr == TASK_EXIT_AND_DELETE) {
    queue_remove_entry(&(task->create_queue));
    tn_created_tasks_qty--;
    task->id_task = 0;
  }

  SwitchContextRequest();
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
 * @fn    void ThreadExit(void)
 */
void ThreadExit(void)
{
  osTaskExit(TASK_EXIT);
}

/**
 * @fn          osError_t TaskSuspend(TN_TCB *task)
 * @param[out]  task  Pointer to the task TCB to be suspended
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_OVERFLOW     Task already suspended
 *              TERR_WSTATE       Task is not active (i.e in DORMANT state )
 */
static
osError_t TaskSuspend(TN_TCB *task)
{
  if (task->task_state & TSK_STATE_SUSPEND)
    return TERR_OVERFLOW;
  if (task->task_state == TSK_STATE_DORMANT)
    return TERR_WSTATE;

  if (task->task_state == TSK_STATE_RUNNABLE) {
    task->task_state = TSK_STATE_SUSPEND;
    TaskToNonRunnable(task);
  }
  else {
    task->task_state |= TSK_STATE_SUSPEND;
  }

  return TERR_NO_ERR;
}

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
osError_t osTaskSuspend(TN_TCB *task)
{
  if (task == NULL)
    return TERR_WRONG_PARAM;
  if (task->id_task != TN_ID_TASK)
    return TERR_NOEXS;
  if (IS_IRQ_MODE() || IS_IRQ_MASKED())
    return TERR_ISR;

  return svcTask(TaskSuspend, task);
}

/**
 * @fn          osError_t TaskResume(TN_TCB *task)
 * @param[out]  task  Pointer to task TCB to be resumed
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_WSTATE       Task is not in SUSPEDED or WAITING_SUSPEDED state
 *              TERR_NOEXS        Object is not a task or non-existent
 *              TERR_ISR          The function cannot be called from interrupt service routines
 */
static
osError_t TaskResume(TN_TCB *task)
{
  if (!(task->task_state & TSK_STATE_SUSPEND))
    return TERR_WSTATE;

  if (!(task->task_state & TSK_STATE_WAIT))
    /* The task is not in the WAIT-SUSPEND state */
    TaskToRunnable(task);
  else
    /* Just remove TSK_STATE_SUSPEND from the task state */
    task->task_state &= ~TSK_STATE_SUSPEND;

  return TERR_NO_ERR;
}

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
osError_t osTaskResume(TN_TCB *task)
{
  if (task == NULL)
    return TERR_WRONG_PARAM;
  if (task->id_task != TN_ID_TASK)
    return TERR_NOEXS;
  if (IS_IRQ_MODE() || IS_IRQ_MASKED())
    return TERR_ISR;

  return svcTask(TaskResume, task);
}

/**
 * @fn        void TaskSleep(TIME_t timeout)
 * @param[in] timeout   Timeout value must be greater than 0.
 */
static
void TaskSleep(TIME_t timeout)
{
  TN_TCB *task = ThreadGetCurrent();

  task->wercd = NULL;
  ThreadToWaitAction(task, NULL, TSK_WAIT_REASON_SLEEP, timeout);
}

/**
 * @fn        osError_t osTaskSleep(TIME_t timeout)
 * @brief     Puts the currently running task to the sleep for at most timeout system ticks.
 * @param[in] timeout   Timeout value must be greater than 0.
 *                      A value of TN_WAIT_INFINITE causes an infinite delay.
 * @return    TERR_NO_ERR       Normal completion
 *            TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *            TERR_ISR          The function cannot be called from interrupt service routines
 */
osError_t osTaskSleep(TIME_t timeout)
{
  if (timeout == 0)
    return TERR_WRONG_PARAM;
  if (IS_IRQ_MODE() || IS_IRQ_MASKED())
    return TERR_ISR;

  svcTaskSleep(TaskSleep, timeout);

  return TERR_NO_ERR;
}

/**
 * @fn          osError_t TaskWakeup(TN_TCB *task)
 * @param[out]  task  Pointer to the task TCB to be wake up
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WSTATE       Task is not in WAIT state
 */
static
osError_t TaskWakeup(TN_TCB *task)
{
  if ((task->task_state & TSK_STATE_WAIT) && (task->task_wait_reason == TSK_WAIT_REASON_SLEEP))
    ThreadWaitComplete(task);
  else
    return TERR_WSTATE;

  return TERR_NO_ERR;
}

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
osError_t osTaskWakeup(TN_TCB *task)
{
  if (task == NULL)
    return TERR_WRONG_PARAM;
  if (task->id_task != TN_ID_TASK)
    return TERR_NOEXS;
  if (IS_IRQ_MODE() || IS_IRQ_MASKED())
    return TERR_ISR;

  return svcTask(TaskWakeup, task);
}

/**
 * @fn          osError_t TaskReleaseWait(TN_TCB *task)
 * @param[out]  task  Pointer to the task TCB to be released from waiting or sleep
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WCONTEXT     Unacceptable system's state for function's request executing
 */
static
osError_t TaskReleaseWait(TN_TCB *task)
{
  if (!(task->task_state & TSK_STATE_WAIT))
    return TERR_WCONTEXT;

  queue_remove_entry(&(task->task_queue));
  ThreadWaitComplete(task);

  return TERR_NO_ERR;
}

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
osError_t osTaskReleaseWait(TN_TCB *task)
{
  if (task == NULL)
    return TERR_WRONG_PARAM;
  if (task->id_task != TN_ID_TASK)
    return TERR_NOEXS;
  if (IS_IRQ_MODE() || IS_IRQ_MASKED())
    return TERR_ISR;

  return svcTask(TaskReleaseWait, task);
}

static
osError_t TaskSetPriority(TN_TCB *task, uint32_t new_priority)
{
  if (new_priority == 0)
    new_priority = task->base_priority;

  if (task->task_state == TSK_STATE_DORMANT)
    return TERR_WCONTEXT;

  if (task->task_state == TSK_STATE_RUNNABLE)
    ThreadChangePriority(task, new_priority);
  else
    task->priority = new_priority;

  return TERR_NO_ERR;
}

osError_t osTaskSetPriority(TN_TCB *task, uint32_t new_priority)
{
  if (task == NULL)
    return TERR_WRONG_PARAM;
  if (task->id_task != TN_ID_TASK)
    return TERR_NOEXS;
  if (new_priority > (NUM_PRIORITY - 2))
    return TERR_WRONG_PARAM;
  if (IS_IRQ_MODE() || IS_IRQ_MASKED())
    return TERR_ISR;

  return svcTaskSetPriority(TaskSetPriority, task, new_priority);
}

static
TIME_t TaskGetTime(TN_TCB *task)
{
  return task->time;
}

TIME_t osTaskGetTime(TN_TCB *task)
{
  if (task == NULL)
    return 0;
  if (task->id_task != TN_ID_TASK)
    return 0;
  if (IS_IRQ_MODE() || IS_IRQ_MASKED())
    return 0;

  return svcTaskGetTime(TaskGetTime, task);
}

/* ----------------------------- End of file ---------------------------------*/

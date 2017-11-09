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
int32_t svc_task_sleep(int32_t (*)(TIME_t), TIME_t);

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
  switch_context_request();
}

/**
 * @brief
 * @param
 * @return
 */
static
void task_to_runnable(TN_TCB *task)
{
  if (task->task_state == TSK_STATE_DORMANT) {
    //--- Init task stack
    task->task_stk = stack_init(task->task_func_addr, task->stk_start, task->task_func_param);
  }

  task->task_state = TSK_STATE_RUNNABLE;
  task->pwait_queue = NULL;

  //-- Add the task to the end of 'ready queue' for the current priority
  knlThreadSetReady(task);

  //-- less value - greater priority, so '<' operation is used here

  if (task->priority < knlThreadGetNext()->priority) {
    knlThreadSetNext(task);
    switch_context_request();
  }
}

/**
 * @brief
 * @param
 * @return
 */
static
void task_to_non_runnable(TN_TCB *task)
{
  int priority;
  CDLL_QUEUE *que;

  priority = task->priority;
  que = &(knlInfo.ready_list[priority]);

  //-- remove the curr task from any queue (now - from ready queue)
  queue_remove_entry(&(task->task_queue));

  if (is_queue_empty(que)) { //-- No ready tasks for the curr priority
    //-- remove 'ready to run' from the curr priority
    knlInfo.ready_to_run_bmp &= ~(1 << priority);

    //-- Find highest priority ready to run -
    //-- at least, MSB bit must be set for the idle task
    ThreadDispatch();   //-- v.2.6
  }
  else { //-- There are 'ready to run' task(s) for the curr priority
    if (knlThreadGetNext() == task) {
      knlThreadSetNext(get_task_by_tsk_queue(que->next));
      switch_context_request();
    }
  }
}

/**
 * @brief
 * @param
 * @return
 */
static
bool task_wait_release(TN_TCB *task)
{
  bool rc = false;
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
    task_to_runnable(task);
    rc = true;
  }
  else
    //-- remove WAIT state
    task->task_state = TSK_STATE_SUSPEND;

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

        knlThreadSetPriority(mt_holder_task, curr_priority);
        rc = true;
      }
    }
  }
#endif

  task->task_wait_reason = 0; //-- Clear wait reason

  return rc;
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

  queue_remove_entry(&(task->task_queue));
  task_wait_release(task);
  if (task->wercd != NULL)
    *task->wercd = TERR_TIMEOUT;
}

static
int32_t task_sleep(TIME_t timeout)
{
  TN_TCB *task = knlThreadGetCurrent();

  task->wercd = NULL;
  knlThreadToWaitAction(task, NULL, TSK_WAIT_REASON_SLEEP, timeout);

  return TERR_NO_ERR;
}

/*******************************************************************************
 *  function implementations (scope: module-exported)
 ******************************************************************************/

/**
 * @fn    void knlThreadSetReady(TN_TCB *thread)
 * @brief Adds task to the end of ready queue for current priority
 * @param thread
 */
void knlThreadSetReady(TN_TCB *thread)
{
  knlInfo_t *info = &knlInfo;
  int32_t priority = thread->priority;

  queue_add_tail(&info->ready_list[priority], &thread->task_queue);
  info->ready_to_run_bmp |= (1 << priority);
}

/*-----------------------------------------------------------------------------*
 * Название : knlThreadWaitComplete
 * Описание : Выводит задачу из состояния ожидания и удаляет из очереди таймеров
 * Параметры: task - Указатель на задачу
 * Результат: Возвращает true при успешном выполнении, иначе возвращает false
 *----------------------------------------------------------------------------*/
bool knlThreadWaitComplete(TN_TCB *task)
{
  bool rc;

  if (task == NULL)
    return false;

  timer_delete(&task->wtmeb);
  rc = task_wait_release(task);

  if (task->wercd != NULL)
    *task->wercd = TERR_NO_ERR;

  return rc;
}

void knlThreadChangePriority(TN_TCB * task, int32_t new_priority)
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
  knlThreadSetReady(task);
  ThreadDispatch();
}

#ifdef USE_MUTEXES

void knlThreadSetPriority(TN_TCB * task, int32_t priority)
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
      knlThreadChangePriority(task, priority);
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

void knlThreadExit(void)
{
  tn_task_exit(0);
}

void knlThreadWaitDelete(CDLL_QUEUE *wait_que)
{
  CDLL_QUEUE *que;
  TN_TCB *task;

  while (!is_queue_empty(wait_que)) {
    que = queue_remove_head(wait_que);
    task = get_task_by_tsk_queue(que);
    knlThreadWaitComplete(task);
    if (task->wercd != NULL)
      *task->wercd = TERR_DLT;
  }
}

void knlThreadToWaitAction(TN_TCB *task, CDLL_QUEUE * wait_que, int wait_reason,
                         TIME_t timeout)
{
  task_to_non_runnable(task);

  task->task_state = TSK_STATE_WAIT;
  task->task_wait_reason = wait_reason;

  //--- Add to the wait queue  - FIFO
  if (wait_que != NULL) {
    queue_add_tail(wait_que, &(task->task_queue));
    task->pwait_queue = wait_que;
  }

  //--- Add to the timers queue
  if (timeout != TN_WAIT_INFINITE)
    timer_insert(&task->wtmeb, timeout, (CBACK)task_wait_release_handler, task);
}

int tn_task_create(TN_TCB * task,                 //-- task TCB
  void (*task_func)(void *param),  //-- task function
  int priority,                    //-- task priority
  unsigned int * task_stack_start, //-- task stack first addr in memory (bottom)
  int task_stack_size,         //-- task stack size (in sizeof(void*),not bytes)
  void * param,                    //-- task function parameter
  int option)                      //-- Creation option
{
  int rc;

  unsigned int * ptr_stack;
  int i;

  //-- Light weight checking of system tasks recreation

  if ( ((priority == 0) && ((option & TN_TASK_TIMER) == 0))
    || ((priority == (NUM_PRIORITY-1)) && ((option & TN_TASK_IDLE) == 0)) )
    return TERR_WRONG_PARAM;

  if ( ((priority < 0) || (priority > (NUM_PRIORITY - 1)))
    || (task_stack_size < TN_MIN_STACK_SIZE) || (task_func == NULL) || (task == NULL)
    || (task_stack_start == NULL) || (task->id_task != 0) )  //-- recreation
    return TERR_WRONG_PARAM;

  rc = TERR_NO_ERR;

  BEGIN_DISABLE_INTERRUPT

  //--- Init task TCB

  task->task_func_addr = (void*)task_func;
  task->task_func_param = param;
  task->stk_start = (unsigned int*)task_stack_start; //-- Base address of task stack space
  task->stk_size = task_stack_size;              //-- Task stack size (in bytes)
  task->base_priority = priority;                        //-- Task base priority
  task->id_task = TN_ID_TASK;
  task->time = 0;
  task->wercd = NULL;

  //-- Fill all task stack space by TN_FILL_STACK_VAL - only inside create_task

  for (ptr_stack = task->stk_start, i = 0; i < task->stk_size; i++)
    *ptr_stack-- = TN_FILL_STACK_VAL;

  task_set_dormant_state(task);

  //-- Add task to created task queue

  queue_add_tail(&tn_create_queue, &(task->create_queue));
  tn_created_tasks_qty++;

  if ((option & TN_TASK_START_ON_CREATION) != 0)
    task_to_runnable(task);

  END_DISABLE_INTERRUPT

  return rc;
}

/*-----------------------------------------------------------------------------*
 * Название : tn_task_delete
 * Описание :
 * Параметры:
 * Результат:
 *----------------------------------------------------------------------------*/
int tn_task_delete(TN_TCB *task)
{
  int rc = TERR_NO_ERR;

  if (task == NULL)
    return TERR_WRONG_PARAM;
  if (task->id_task != TN_ID_TASK)
    return TERR_NOEXS;

  BEGIN_DISABLE_INTERRUPT

  if (task->task_state != TSK_STATE_DORMANT)
    rc = TERR_WCONTEXT;  //-- Cannot delete not-terminated task
  else {
    queue_remove_entry(&(task->create_queue));
    tn_created_tasks_qty--;
    task->id_task = 0;
  }

  END_DISABLE_INTERRUPT
  return rc;
}

/*-----------------------------------------------------------------------------*
 * Название : tn_task_suspend
 * Описание : If the task is runnable, it is moved to the SUSPENDED state.
 *            If the task is in the WAITING state, it is moved to the
 *            WAITING­SUSPENDED state.
 * Параметры:
 * Результат:
 *----------------------------------------------------------------------------*/
int tn_task_suspend(TN_TCB *task)
{
  int rc = TERR_NO_ERR;

  if (task == NULL)
    return TERR_WRONG_PARAM;
  if (task->id_task != TN_ID_TASK)
    return TERR_NOEXS;

  BEGIN_CRITICAL_SECTION

  if (task->task_state & TSK_STATE_SUSPEND)
    rc = TERR_OVERFLOW;
  else {
    if (task->task_state == TSK_STATE_DORMANT)
      rc = TERR_WSTATE;
    else {
      if (task->task_state == TSK_STATE_RUNNABLE) {
        task->task_state = TSK_STATE_SUSPEND;
        task_to_non_runnable(task);
      }
      else {
        task->task_state |= TSK_STATE_SUSPEND;
      }
    }
  }

  END_CRITICAL_SECTION
  return rc;
}

/*-----------------------------------------------------------------------------*
 * Название : tn_task_resume
 * Описание :
 * Параметры:
 * Результат:
 *----------------------------------------------------------------------------*/
int tn_task_resume(TN_TCB *task)
{
  int rc = TERR_NO_ERR;

  if (task == NULL)
    return TERR_WRONG_PARAM;
  if (task->id_task != TN_ID_TASK)
    return TERR_NOEXS;

  BEGIN_CRITICAL_SECTION

  if (!(task->task_state & TSK_STATE_SUSPEND))
    rc = TERR_WSTATE;
  else {
    if (!(task->task_state & TSK_STATE_WAIT)) //- The task is not in the WAIT-SUSPEND state
      task_to_runnable(task);
    else
      //-- Just remove TSK_STATE_SUSPEND from the task state
      task->task_state &= ~TSK_STATE_SUSPEND;
  }

  END_CRITICAL_SECTION
  return rc;
}

/*-----------------------------------------------------------------------------*
 * Название : tn_task_sleep
 * Описание :
 * Параметры:
 * Результат:
 *----------------------------------------------------------------------------*/
int32_t tn_task_sleep(TIME_t timeout)
{
  if (IS_IRQ_MODE() || IS_IRQ_MASKED()) {
    return TERR_ISR;
  }

  if (timeout == 0)
    return TERR_WRONG_PARAM;

  return svc_task_sleep(task_sleep, timeout);
}

/*-----------------------------------------------------------------------------*
 * Название : tn_task_time
 * Описание : Возвращает время работы задачи с момента ее создания
 * Параметры: task  - Указатель на дескриптор задачи
 * Результат: Возвращает беззнаковое 32 битное число
 *----------------------------------------------------------------------------*/
unsigned long tn_task_time(TN_TCB *task)
{
  unsigned long time;

  if (task == NULL)
    return 0;
  if (task->id_task != TN_ID_TASK)
    return 0;

  BEGIN_DISABLE_INTERRUPT

  time = task->time;

  END_DISABLE_INTERRUPT

  return time;
}

/*-----------------------------------------------------------------------------*
 * Название : tn_task_wakeup
 * Описание : Пробуждает заданную задачу, если заданная задача усыпила себя
 *            вызовом tn_task_sleep
 * Параметры: task  - Указатель на дескриптор задачи.
 * Результат: Возвращает следующие значения:
 *              TERR_NO_ERR - если выполнена без ошибок
 *              TERR_WRONG_PARAM - если задан неверный параметр функции
 *              TERR_NOEXS - если задача, которую надо пробудить, не существует
 *              TERR_WSTATE - если задача не находится в состоянии SLEEP
 *----------------------------------------------------------------------------*/
int tn_task_wakeup(TN_TCB *task)
{
  int rc = TERR_NO_ERR;

  if (task == NULL)
    return TERR_WRONG_PARAM;
  if (task->id_task != TN_ID_TASK)
    return TERR_NOEXS;

  BEGIN_CRITICAL_SECTION

  if ((task->task_state & TSK_STATE_WAIT) && task->task_wait_reason == TSK_WAIT_REASON_SLEEP)
    knlThreadWaitComplete(task);
  else
    rc = TERR_WSTATE;

  END_CRITICAL_SECTION

  return rc;
}

/*-----------------------------------------------------------------------------*
 * Название : tn_task_activate
 * Описание :
 * Параметры:
 * Результат:
 *----------------------------------------------------------------------------*/
int tn_task_activate(TN_TCB *task)
{
  int rc = TERR_NO_ERR;

  if (task == NULL)
    return TERR_WRONG_PARAM;
  if (task->id_task != TN_ID_TASK)
    return TERR_NOEXS;

  BEGIN_CRITICAL_SECTION

  if (task->task_state == TSK_STATE_DORMANT)
    task_to_runnable(task);
  else
    rc = TERR_OVERFLOW;

  END_CRITICAL_SECTION

  return rc;
}

/*-----------------------------------------------------------------------------*
 * Название : tn_task_release_wait
 * Описание :
 * Параметры:
 * Результат:
 *----------------------------------------------------------------------------*/
int tn_task_release_wait(TN_TCB *task)
{
  int rc = TERR_NO_ERR;

  if (task == NULL)
    return TERR_WRONG_PARAM;
  if (task->id_task != TN_ID_TASK)
    return TERR_NOEXS;

  BEGIN_CRITICAL_SECTION

  if ((task->task_state & TSK_STATE_WAIT) == 0)
    rc = TERR_WCONTEXT;
  else {
    queue_remove_entry(&(task->task_queue));
    knlThreadWaitComplete(task);
  }

  END_CRITICAL_SECTION

  return rc;
}

/*-----------------------------------------------------------------------------*
 * Название : tn_task_exit
 * Описание :
 * Параметры:
 * Результат:
 *----------------------------------------------------------------------------*/
void tn_task_exit(int attr)
{
#ifdef USE_MUTEXES
  CDLL_QUEUE *que;
  TN_MUTEX *mutex;
#endif

  TN_TCB *task;

  __disable_irq();

  task = knlThreadGetCurrent();

  //-- Unlock all mutexes, locked by the task

#ifdef USE_MUTEXES
  while (!is_queue_empty(&(task->mutex_queue))) {
    que = queue_remove_head(&(task->mutex_queue));
    mutex = get_mutex_by_mutex_queque(que);
    do_unlock_mutex(mutex);
  }
#endif

  task_to_non_runnable(task);

  task_set_dormant_state(task);

  if (attr == TN_EXIT_AND_DELETE_TASK) {
    queue_remove_entry(&(task->create_queue));
    tn_created_tasks_qty--;
    task->id_task = 0;
  }

  switch_context_exit(); // interrupts will be enabled inside switch_context_exit()
}

/*-----------------------------------------------------------------------------*
 * Название : tn_task_terminate
 * Описание :
 * Параметры:
 * Результат:
 *----------------------------------------------------------------------------*/
int tn_task_terminate(TN_TCB *task)
{
  int rc = TERR_NO_ERR;

#ifdef USE_MUTEXES
  CDLL_QUEUE * que;
  TN_MUTEX * mutex;
#endif

  if (task == NULL)
    return TERR_WRONG_PARAM;
  if (task->id_task != TN_ID_TASK)
    return TERR_NOEXS;

  BEGIN_DISABLE_INTERRUPT

  if (task->task_state == TSK_STATE_DORMANT || knlThreadGetCurrent() == task)
    rc = TERR_WCONTEXT; //-- Cannot terminate running task
  else {
    if (task->task_state == TSK_STATE_RUNNABLE) {
      task_to_non_runnable(task);
    }
    else if (task->task_state & TSK_STATE_WAIT) {
      //-- Free all queues, involved in the 'waiting'
      queue_remove_entry(&(task->task_queue));
      //-----------------------------------------
      timer_delete(&task->wtmeb);
    }

    //-- Unlock all mutexes, locked by the task
#ifdef USE_MUTEXES
    while (!is_queue_empty(&(task->mutex_queue))) {
      que = queue_remove_head(&(task->mutex_queue));
      mutex = get_mutex_by_mutex_queque(que);
      do_unlock_mutex(mutex);
    }
#endif

    task_set_dormant_state(task);
  }

  END_DISABLE_INTERRUPT

  return rc;
}

/*-----------------------------------------------------------------------------*
 * Название : tn_task_change_priority
 * Описание :
 * Параметры:
 * Результат:
 *----------------------------------------------------------------------------*/
int tn_task_change_priority(TN_TCB * task, int new_priority)
{
  int rc = TERR_NO_ERR;

  if (task == NULL)
    return TERR_WRONG_PARAM;
  if (task->id_task != TN_ID_TASK)
    return TERR_NOEXS;
  if ( (new_priority < 0) || (new_priority > (NUM_PRIORITY - 2)) )
    return TERR_WRONG_PARAM;

  BEGIN_CRITICAL_SECTION

  if (new_priority == 0)
    new_priority = task->base_priority;

  if (task->task_state == TSK_STATE_DORMANT)
    rc = TERR_WCONTEXT;
  else if (task->task_state == TSK_STATE_RUNNABLE)
    knlThreadChangePriority(task, new_priority);
  else
    task->priority = new_priority;

  END_CRITICAL_SECTION
  return rc;
}

/* ----------------------------- End of file ---------------------------------*/

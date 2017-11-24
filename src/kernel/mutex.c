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

#ifdef USE_MUTEXES

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

/*******************************************************************************
 *  function implementations (scope: module-local)
 ******************************************************************************/

/*******************************************************************************
 *  function implementations (scope: module-exported)
 ******************************************************************************/

/*-----------------------------------------------------------------------------*
 * Название : find_max_blocked_priority
 * Описание :
 * Параметры:
 * Результат:
 *----------------------------------------------------------------------------*/
int find_max_blocked_priority(TN_MUTEX *mutex, int ref_priority)
{
  int priority;
  CDLL_QUEUE *curr_que;
  TN_TCB *task;

  priority = ref_priority;
  curr_que = mutex->wait_queue.next;
  while (curr_que != &(mutex->wait_queue)) {
    task = get_task_by_tsk_queue(curr_que);
    if (task->priority < priority) //--  task priority is higher
      priority = task->priority;

    curr_que = curr_que->next;
  }

  return priority;
}

/*-----------------------------------------------------------------------------*
 * Название : do_unlock_mutex
 * Описание :
 * Параметры:
 * Результат:
 *----------------------------------------------------------------------------*/
int do_unlock_mutex(TN_MUTEX *mutex)
{
  CDLL_QUEUE *curr_que;
  TN_MUTEX *tmp_mutex;
  TN_TCB *task = TaskGetCurrent();
  int pr;

  //-- Delete curr mutex from task's locked mutexes queue

  QueueRemoveEntry(&(mutex->mutex_queue));
  pr = task->base_priority;

  //---- No more mutexes, locked by the our task
  if (!isQueueEmpty(&(task->mutex_queue))) {
    curr_que = task->mutex_queue.next;
    while (curr_que != &(task->mutex_queue)) {
      tmp_mutex = get_mutex_by_mutex_queque(curr_que);

      if (tmp_mutex->attr & TN_MUTEX_ATTR_CEILING) {
        if (tmp_mutex->ceil_priority < pr)
          pr = tmp_mutex->ceil_priority;
      }
      else {
        pr = find_max_blocked_priority(tmp_mutex, pr);
      }

      curr_que = curr_que->next;
    }
  }

  //-- Restore original priority
  if (pr != task->priority)
    ThreadChangePriority(task, pr);

  //-- Check for the task(s) that want to lock the mutex
  if (isQueueEmpty(&(mutex->wait_queue))) {
    mutex->holder = NULL;
    return true;
  }

  //--- Now lock the mutex by the first task in the mutex queue
  curr_que = QueueRemoveHead(&(mutex->wait_queue));
  task = get_task_by_tsk_queue(curr_que);
  mutex->holder = task;
  if (mutex->attr & TN_MUTEX_ATTR_RECURSIVE)
    mutex->cnt++;

  if ((mutex->attr & TN_MUTEX_ATTR_CEILING)
    && (task->priority > mutex->ceil_priority))
    task->priority = mutex->ceil_priority;

  ThreadWaitComplete(task);
  QueueAddTail(&(task->mutex_queue), &(mutex->mutex_queue));

  return true;
}

/*
 The ceiling protocol in ver 2.6 is more "lightweight" in comparison
 to previous versions.
 The code of ceiling protocol is derived from Vyacheslav Ovsiyenko version
 */

// L. Sha, R. Rajkumar, J. Lehoczky, Priority Inheritance Protocols: An Approach
// to Real-Time Synchronization, IEEE Transactions on Computers, Vol.39, No.9, 1990
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
osError_t tn_mutex_create(TN_MUTEX * mutex, int attribute, int ceil_priority)
{
  if (mutex == NULL)
    return TERR_WRONG_PARAM;
  if (mutex->id != 0) //-- no recreation
    return TERR_WRONG_PARAM;
  if ((attribute & TN_MUTEX_ATTR_CEILING)
    && ((ceil_priority < 1) || (ceil_priority > (NUM_PRIORITY-2))))
    return TERR_WRONG_PARAM;

  QueueReset(&(mutex->wait_queue));
  QueueReset(&(mutex->mutex_queue));

  mutex->attr = attribute;
  mutex->holder = NULL;
  mutex->ceil_priority = ceil_priority;
  mutex->cnt = 0;
  mutex->id = ID_MUTEX;

  return TERR_NO_ERR;
}

//----------------------------------------------------------------------------
osError_t tn_mutex_delete(TN_MUTEX *mutex)
{
  if (mutex == NULL)
    return TERR_WRONG_PARAM;
  if (mutex->id != ID_MUTEX)
    return TERR_NOEXS;

  BEGIN_CRITICAL_SECTION

  if (mutex->holder != NULL && mutex->holder != TaskGetCurrent()) {
    END_DISABLE_INTERRUPT
    return TERR_ILUSE;
  }

  //-- Remove all tasks(if any) from mutex's wait queue
  ThreadWaitDelete(&mutex->wait_queue);

  if (mutex->holder != NULL) {  //-- If the mutex is locked
    do_unlock_mutex(mutex);
    QueueReset(&(mutex->mutex_queue));
  }
  mutex->id = ID_INVALID; // Mutex not exists now

  END_CRITICAL_SECTION

  return TERR_NO_ERR;
}

//----------------------------------------------------------------------------
osError_t tn_mutex_lock(TN_MUTEX *mutex, unsigned long timeout)
{
  osError_t rc = TERR_NO_ERR;
  TN_TCB *task;

  if (mutex == NULL)
    return TERR_WRONG_PARAM;
  if (mutex->id != ID_MUTEX)
    return TERR_NOEXS;

  BEGIN_CRITICAL_SECTION

  task = TaskGetCurrent();

  if (task == mutex->holder) {
    if (mutex->attr & TN_MUTEX_ATTR_RECURSIVE) {
      /*Recursive locking enabled*/
      mutex->cnt++;
      END_DISABLE_INTERRUPT
      return rc;
    }
    else {
      /*Recursive locking not enabled*/
      END_DISABLE_INTERRUPT
      return TERR_ILUSE;
    }
  }

  if (mutex->attr & TN_MUTEX_ATTR_CEILING) {
    if (task->base_priority < mutex->ceil_priority) { //-- base pri of task higher
      END_DISABLE_INTERRUPT
      return TERR_ILUSE;
    }

    if (mutex->holder == NULL) {  //-- mutex not locked
      mutex->holder = task;

      if (mutex->attr & TN_MUTEX_ATTR_RECURSIVE)
        mutex->cnt++;

      //-- Add mutex to task's locked mutexes queue
      QueueAddTail(&(task->mutex_queue), &(mutex->mutex_queue));
      //-- Ceiling protocol
      if (task->priority > mutex->ceil_priority)
        ThreadChangePriority(task, mutex->ceil_priority);
    }
    else { //-- the mutex is already locked
      if (timeout == 0U)
        rc = TERR_TIMEOUT;
      else {
        task->wait_rc = &rc;
        //--- Task -> to the mutex wait queue
        ThreadToWaitAction(task, &(mutex->wait_queue), WAIT_REASON_MUTEX_C, timeout);
      }
    }
  }
  else {
    if (mutex->holder == NULL) {  //-- mutex not locked
      mutex->holder = task;

      if (mutex->attr & TN_MUTEX_ATTR_RECURSIVE)
        mutex->cnt++;

      QueueAddTail(&(task->mutex_queue), &(mutex->mutex_queue));
    }
    else {  //-- the mutex is already locked
      if (timeout == 0U)
        rc = TERR_TIMEOUT;
      else {
        //-- Base priority inheritance protocol
        //-- if run_task curr priority higher holder's curr priority
        if (task->priority < mutex->holder->priority)
          ThreadSetPriority(mutex->holder, task->priority);
        task->wait_rc = &rc;
        ThreadToWaitAction(task, &(mutex->wait_queue), WAIT_REASON_MUTEX_I, timeout);
      }
    }
  }

  END_CRITICAL_SECTION
  return rc;
}

//----------------------------------------------------------------------------
osError_t tn_mutex_unlock(TN_MUTEX *mutex)
{
  if (mutex == NULL)
    return TERR_WRONG_PARAM;
  if (mutex->id != ID_MUTEX)
    return TERR_NOEXS;

  BEGIN_CRITICAL_SECTION

  //-- Unlocking is enabled only for the owner and already locked mutex
  if (TaskGetCurrent() != mutex->holder) {
    END_DISABLE_INTERRUPT
    return TERR_ILUSE;
  }

  if (mutex->attr & TN_MUTEX_ATTR_RECURSIVE)
    mutex->cnt--;

  if (mutex->cnt == 0)
    do_unlock_mutex(mutex);

  END_CRITICAL_SECTION

  return TERR_NO_ERR;
}

#endif //-- USE_MUTEXES

/*------------------------------ End of file ---------------------------------*/

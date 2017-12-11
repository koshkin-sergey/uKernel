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

__SVC(0)
osError_t svcMutexNew(osError_t (*)(osMutex_t*, const osMutexAttr_t*), osMutex_t*, const osMutexAttr_t*);
__SVC(0)
osError_t svcMutexDelete(osError_t (*)(osMutex_t*), osMutex_t*);
__SVC(0)
osError_t svcMutexAcquire(osError_t (*)(osMutex_t*, osTime_t), osMutex_t*, osTime_t);
__SVC(0)
osError_t svcMutexRelease(osError_t (*)(osMutex_t*), osMutex_t*);
__SVC(0)
osTask_t* svcMutexGetOwner(osTask_t* (*)(osMutex_t*), osMutex_t*);

/*******************************************************************************
 *  function implementations (scope: module-local)
 ******************************************************************************/

static
osError_t MutexNew(osMutex_t *mutex, const osMutexAttr_t *attr)
{
  if (mutex->id == ID_MUTEX)
    return TERR_NO_ERR;

  if (attr != NULL) {
    if ((attr->attr_bits & osMutexPrioCeiling) &&
       ((attr->ceil_priority < 1) || (attr->ceil_priority > (NUM_PRIORITY-2))))
      return TERR_WRONG_PARAM;

    mutex->attr = attr->attr_bits;
    mutex->ceil_priority = attr->ceil_priority;
  }
  else {
    mutex->attr = 0U;
    mutex->ceil_priority = 0U;
  }

  QueueReset(&mutex->wait_que);
  QueueReset(&mutex->mutex_que);

  mutex->holder = NULL;
  mutex->cnt = 0;
  mutex->id = ID_MUTEX;

  return TERR_NO_ERR;
}

static
osError_t MutexDelete(osMutex_t *mutex)
{
  if (mutex->id != ID_MUTEX)
    return TERR_NOEXS;

  if (mutex->holder != NULL && mutex->holder != TaskGetCurrent()) {
    return TERR_ILUSE;
  }

  /* Remove all tasks(if any) from mutex's wait queue */
  TaskWaitDelete(&mutex->wait_que);

  /* If the mutex is locked */
  if (mutex->holder != NULL) {
    MutexUnLock(mutex);
    QueueReset(&mutex->mutex_que);
  }

  /* Mutex not exists now */
  mutex->id = ID_INVALID;

  return TERR_NO_ERR;
}

static
osError_t MutexAcquire(osMutex_t *mutex, osTime_t timeout)
{
  if (mutex->id != ID_MUTEX)
    return TERR_NOEXS;

  osTask_t *task = TaskGetCurrent();

  if (task == mutex->holder) {
    if (mutex->attr & osMutexRecursive) {
      /* Recursive locking enabled */
      mutex->cnt++;
      return TERR_NO_ERR;
    }
    else {
      /* Recursive locking not enabled */
      return TERR_ILUSE;
    }
  }

  if (mutex->attr & osMutexPrioCeiling) {
    if (task->base_priority < mutex->ceil_priority) {
      /* base priority of task higher */
      return TERR_ILUSE;
    }

    if (mutex->holder == NULL) {
      /* mutex not locked */
      mutex->holder = task;

      if (mutex->attr & osMutexRecursive)
        mutex->cnt++;

      /* Add mutex to task's locked mutexes queue */
      QueueAddTail(&task->mutex_queue, &mutex->mutex_que);
      /* Ceiling protocol */
      if (task->priority > mutex->ceil_priority)
        TaskChangePriority(task, mutex->ceil_priority);
    }
    else {
      /* the mutex is already locked */
      if (timeout == 0U)
        return TERR_TIMEOUT;

      /* Task -> to the mutex wait queue */
      TaskWaitEnter(task, &mutex->wait_que, WAIT_REASON_MUTEX_C, timeout);
    }
  }
  else {
    if (mutex->holder == NULL) {
      /* mutex not locked */
      mutex->holder = task;

      if (mutex->attr & osMutexRecursive)
        mutex->cnt++;

      QueueAddTail(&task->mutex_queue, &mutex->mutex_que);
    }
    else {
      /* the mutex is already locked */
      if (timeout == 0U)
        return TERR_TIMEOUT;

      //-- Base priority inheritance protocol
      //-- if run_task curr priority higher holder's curr priority
      if (task->priority < mutex->holder->priority)
        MutexSetPriority(mutex->holder, task->priority);

      TaskWaitEnter(task, &mutex->wait_que, WAIT_REASON_MUTEX_I, timeout);
    }
  }

  return TERR_NO_ERR;
}

static
osError_t MutexRelease(osMutex_t *mutex)
{
  if (mutex->id != ID_MUTEX)
    return TERR_NOEXS;

  /* Unlocking is enabled only for the owner and already locked mutex */
  if (TaskGetCurrent() != mutex->holder)
    return TERR_ILUSE;

  if (mutex->attr & osMutexRecursive)
    mutex->cnt--;

  if (mutex->cnt == 0)
    MutexUnLock(mutex);

  return TERR_NO_ERR;
}

static
osTask_t* MutexGetOwner(osMutex_t *mutex)
{
  if (mutex->id != ID_MUTEX)
    return NULL;

  return mutex->holder;
}

/*******************************************************************************
 *  function implementations (scope: module-exported)
 ******************************************************************************/

void MutexSetPriority(osTask_t *task, uint32_t priority)
{
  osMutex_t *mutex;

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

    if (task->state == TSK_STATE_RUNNABLE) {
      TaskChangePriority(task, priority);
      return;
    }

    if (task->state & TSK_STATE_WAIT) {
      if (task->wait_reason == WAIT_REASON_MUTEX_I) {
        task->priority = priority;

        mutex = GetMutexByWaitQueque(task->pwait_queue);
        task = mutex->holder;

        continue;
      }
    }

    task->priority = priority;
    return;
  }
}

uint32_t MutexGetMaxPriority(osMutex_t *mutex, uint32_t ref_priority)
{
  uint32_t priority;
  CDLL_QUEUE *curr_que;
  osTask_t *task;

  priority = ref_priority;
  curr_que = mutex->wait_que.next;
  while (curr_que != &mutex->wait_que) {
    task = GetTaskByQueue(curr_que);
    if (task->priority < priority) //--  task priority is higher
      priority = task->priority;

    curr_que = curr_que->next;
  }

  return priority;
}

void MutexUnLock(osMutex_t *mutex)
{
  CDLL_QUEUE *curr_que;
  osMutex_t *tmp_mutex;
  osTask_t *task = TaskGetCurrent();
  uint32_t pr;

  //-- Delete curr mutex from task's locked mutexes queue

  QueueRemoveEntry(&mutex->mutex_que);
  pr = task->base_priority;

  //---- No more mutexes, locked by the our task
  if (!isQueueEmpty(&task->mutex_queue)) {
    curr_que = task->mutex_queue.next;
    while (curr_que != &task->mutex_queue) {
      tmp_mutex = GetMutexByMutexQueque(curr_que);

      if (tmp_mutex->attr & osMutexPrioCeiling) {
        if (tmp_mutex->ceil_priority < pr)
          pr = tmp_mutex->ceil_priority;
      }
      else {
        pr = MutexGetMaxPriority(tmp_mutex, pr);
      }

      curr_que = curr_que->next;
    }
  }

  //-- Restore original priority
  if (pr != task->priority)
    TaskChangePriority(task, pr);

  //-- Check for the task(s) that want to lock the mutex
  if (isQueueEmpty(&mutex->wait_que)) {
    mutex->holder = NULL;
    return;
  }

  //--- Now lock the mutex by the first task in the mutex queue
  curr_que = QueueRemoveHead(&mutex->wait_que);
  task = GetTaskByQueue(curr_que);
  mutex->holder = task;
  if (mutex->attr & osMutexRecursive)
    mutex->cnt++;

  if ((mutex->attr & osMutexPrioCeiling) && (task->priority > mutex->ceil_priority))
    task->priority = mutex->ceil_priority;

  TaskWaitComplete(task, (uint32_t)TERR_NO_ERR);
  QueueAddTail(&(task->mutex_queue), &(mutex->mutex_que));
}

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
osError_t osMutexNew(osMutex_t *mutex, const osMutexAttr_t *attr)
{
  if (mutex == NULL)
    return TERR_WRONG_PARAM;
  if (IS_IRQ_MODE() || IS_IRQ_MASKED())
    return TERR_ISR;

  return svcMutexNew(MutexNew, mutex, attr);
}

/**
 * @fn          osError_t osMutexDelete(osMutex_t *mutex)
 * @brief       Deletes a mutex object
 * @param[out]  mutex   Pointer to osMutex_t structure of the mutex
 * @return      TERR_NO_ERR       The mutex object has been deleted
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_ISR          Cannot be called from interrupt service routines
 */
osError_t osMutexDelete(osMutex_t *mutex)
{
  if (mutex == NULL)
    return TERR_WRONG_PARAM;
  if (IS_IRQ_MODE() || IS_IRQ_MASKED())
    return TERR_ISR;

  return svcMutexDelete(MutexDelete, mutex);
}

/**
 * @fn          osError_t osMutexAcquire(osMutex_t *mutex, osTime_t timeout)
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
osError_t osMutexAcquire(osMutex_t *mutex, osTime_t timeout)
{
  if (mutex == NULL)
    return TERR_WRONG_PARAM;
  if (IS_IRQ_MODE() || IS_IRQ_MASKED())
    return TERR_ISR;

  return svcMutexAcquire(MutexAcquire, mutex, timeout);
}

/**
 * @fn          osError_t osMutexRelease(osMutex_t *mutex)
 * @brief       Releases a mutex
 * @param[out]  mutex     Pointer to osMutex_t structure of the mutex
 * @return      TERR_NO_ERR       The mutex has been correctly released
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_ILUSE        Illegal usage, e.g. trying to release already free mutex
 *              TERR_ISR          Cannot be called from interrupt service routines
 */
osError_t osMutexRelease(osMutex_t *mutex)
{
  if (mutex == NULL)
    return TERR_WRONG_PARAM;
  if (IS_IRQ_MODE() || IS_IRQ_MASKED())
    return TERR_ISR;

  return svcMutexRelease(MutexRelease, mutex);
}

/**
 * @fn          osTask_t* osMutexGetOwner(osMutex_t *mutex)
 * @brief       Returns the pointer to the task that acquired a mutex. In case
 *              of an error or if the mutex is not blocked by any task, it returns NULL.
 * @param[out]  mutex     Pointer to osMutex_t structure of the mutex
 * @return      Pointer to owner task or NULL when mutex was not acquired
 */
osTask_t* osMutexGetOwner(osMutex_t *mutex)
{
  if (mutex == NULL)
    return NULL;
  if (IS_IRQ_MODE() || IS_IRQ_MASKED())
    return NULL;

  return svcMutexGetOwner(MutexGetOwner, mutex);
}

#endif //-- USE_MUTEXES

/*------------------------------ End of file ---------------------------------*/

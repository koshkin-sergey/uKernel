/*
 * Copyright (C) 2017-2018 Sergey Koshkin <koshkin.sergey@gmail.com>
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

SVC_CALL
osError_t svcMutexNew(osError_t (*)(osMutex_t*, const osMutexAttr_t*), osMutex_t*, const osMutexAttr_t*);
SVC_CALL
osError_t svcMutexDelete(osError_t (*)(osMutex_t*), osMutex_t*);
SVC_CALL
osError_t svcMutexAcquire(osError_t (*)(osMutex_t*, osTime_t), osMutex_t*, osTime_t);
SVC_CALL
osError_t svcMutexRelease(osError_t (*)(osMutex_t*), osMutex_t*);
SVC_CALL
osTask_t* svcMutexGetOwner(osTask_t* (*)(osMutex_t*), osMutex_t*);

/*******************************************************************************
 *  function implementations (scope: module-local)
 ******************************************************************************/

static
void SetPriority(osTask_t *task, uint32_t priority)
{
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
      TaskChangeRunningPriority(task, priority);
      return;
    }

    if (task->state & TSK_STATE_WAIT) {
      if (task->wait_reason == WAIT_REASON_MUTEX_I) {
        task->priority = priority;
        task = GetMutexByWaitQueque(task->pwait_que)->holder;
        continue;
      }
    }

    task->priority = priority;
    return;
  }
}

static
uint32_t GetMaxPriority(osMutex_t *mutex, uint32_t ref_priority)
{
  uint32_t priority;
  queue_t *curr_que;
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

static
void MutexUnLock(osMutex_t *mutex)
{
  queue_t *que;
  uint32_t priority;
  osTask_t *task;

  task = mutex->holder;
  /* Remove Mutex from Task owner list */
  QueueRemoveEntry(&mutex->mutex_que);
  /* Restore owner Task priority */
  if ((mutex->attr & osMutexPrioInherit) != 0U) {
    priority = task->base_priority;

    if (!isQueueEmpty(&task->mutex_que)) {
      que = task->mutex_que.next;
      while (que != &task->mutex_que) {
        priority = GetMaxPriority(GetMutexByMutexQueque(que), priority);
        que = que->next;
      }
    }

    //-- Restore original priority
    if (priority != task->priority) {
      if (task->state == TSK_STATE_RUNNABLE) {
        TaskChangeRunningPriority(task, priority);
      }
      else {
        task->priority = priority;
      }
    }
  }

  //-- Check for the task(s) that want to lock the mutex
  if (isQueueEmpty(&mutex->wait_que)) {
    mutex->holder = NULL;
    mutex->cnt = 0U;
  }
  else {
    //--- Now lock the mutex by the first task in the mutex queue
    que = QueueRemoveHead(&mutex->wait_que);
    mutex->holder = GetTaskByQueue(que);
    QueueAddTail(&mutex->holder->mutex_que, &mutex->mutex_que);
    mutex->cnt = 1U;

    TaskWaitComplete(mutex->holder, (uint32_t)TERR_NO_ERR);
  }
}

/*******************************************************************************
 *  Library functions
 ******************************************************************************/

/**
 * @fn          void MutexOwnerRelease(queue_t *que)
 * @brief       Release Mutexes when owner Task terminates.
 * @param[in]   que   Queue of mutexes
 */
void MutexOwnerRelease(queue_t *que)
{
  osMutex_t *mutex;

  while (isQueueEmpty(que) == false) {
    mutex = GetMutexByMutexQueque(QueueRemoveHead(que));
    if ((mutex->attr & osMutexRobust) != 0U) {
      MutexUnLock(mutex);
    }
  }
}

/*******************************************************************************
 *  Service Calls
 ******************************************************************************/

static
osError_t MutexNew(osMutex_t *mutex, const osMutexAttr_t *attr)
{
  if (mutex->id == ID_MUTEX)
    return TERR_NO_ERR;

  if (attr != NULL) {
    mutex->attr = attr->attr_bits;
  }
  else {
    mutex->attr = 0U;
  }

  QueueReset(&mutex->wait_que);
  QueueReset(&mutex->mutex_que);

  mutex->holder = NULL;
  mutex->cnt    = 0U;
  mutex->id     = ID_MUTEX;

  return TERR_NO_ERR;
}

static
osError_t MutexDelete(osMutex_t *mutex)
{
  if (mutex->id != ID_MUTEX)
    return TERR_NOEXS;

  /* Check if Mutex is locked */
  if (mutex->cnt != 0U) {
    /* Unblock waiting tasks */
    TaskWaitDelete(&mutex->wait_que);
    /* Unlock Mutex */
    MutexUnLock(mutex);
  }

  /* Mutex not exists now */
  mutex->id = ID_INVALID;

  return TERR_NO_ERR;
}

static
osError_t MutexAcquire(osMutex_t *mutex, osTime_t timeout)
{
  osError_t rc;

  if (mutex->id != ID_MUTEX)
    return TERR_NOEXS;

  osTask_t *task = TaskGetCurrent();

  /* Check if Mutex is not locked */
  if (mutex->cnt == 0U) {
    /* Acquire Mutex */
    mutex->holder = task;
    QueueAddTail(&task->mutex_que, &mutex->mutex_que);
    mutex->cnt = 1U;
    rc = TERR_NO_ERR;
  }
  else {
    /* Check if running task is the owner */
    if (task == mutex->holder) {
      /* Check if Mutex is recursive */
      if ((mutex->attr & osMutexRecursive) != 0U) {
        /* Recursive locking enabled */
        mutex->cnt++;
        rc = TERR_NO_ERR;
      }
      else {
        /* Recursive locking not enabled */
        rc = TERR_ILUSE;
      }
    }
    else {
      /* Check if timeout is specified */
      if (timeout != 0U) {
        wait_reason_t wait_reason = WAIT_REASON_MUTEX;
        /* Check if Priority inheritance protocol is enabled */
        if ((mutex->attr & osMutexPrioInherit) != 0U) {
          wait_reason = WAIT_REASON_MUTEX_I;
          /* Raise priority of owner Task if lower than priority of running Task */
          if (task->priority < mutex->holder->priority) {
            /* TODO Сделать корректное обновление приоритета задачи */
            SetPriority(mutex->holder, task->priority);
          }
        }
        /* Suspend current Thread */
        TaskWaitEnter(task, &mutex->wait_que, wait_reason, timeout);
        rc = TERR_WAIT;
      }
      else {
        rc = TERR_TIMEOUT;
      }
    }
  }

  return rc;
}

static
osError_t MutexRelease(osMutex_t *mutex)
{
  if (mutex->id != ID_MUTEX)
    return TERR_NOEXS;

  /* Unlocking is enabled only for the owner and already locked mutex */
  if (TaskGetCurrent() != mutex->holder)
    return TERR_ILUSE;

  if (mutex->cnt == 0U)
    return TERR_ILUSE;

  mutex->cnt--;

  if (mutex->cnt == 0) {
    MutexUnLock(mutex);
  }

  return TERR_NO_ERR;
}

static
osTask_t* MutexGetOwner(osMutex_t *mutex)
{
  if (mutex->id != ID_MUTEX)
    return NULL;

  if (mutex->cnt == 0U)
    return NULL;

  return mutex->holder;
}

/*******************************************************************************
 *  function implementations (scope: module-exported)
 ******************************************************************************/

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
  if (IsIrqMode() || IsIrqMasked())
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
  if (IsIrqMode() || IsIrqMasked())
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
  if (IsIrqMode() || IsIrqMasked())
    return TERR_ISR;

  osError_t ret_val = svcMutexAcquire(MutexAcquire, mutex, timeout);

  if (ret_val == TERR_WAIT)
    return (osError_t)TaskGetCurrent()->wait_info.ret_val;

  return ret_val;
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
  if (IsIrqMode() || IsIrqMasked())
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
  if (IsIrqMode() || IsIrqMasked())
    return NULL;

  return svcMutexGetOwner(MutexGetOwner, mutex);
}

/*------------------------------ End of file ---------------------------------*/

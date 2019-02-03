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

/*******************************************************************************
 *  function implementations (scope: module-local)
 ******************************************************************************/

static
void SetPriority(osThread_t *task, uint8_t priority)
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
    if (task->priority >= priority)
      return;

//    if (task->state == TSK_STATE_RUNNABLE) {
//      TaskChangeRunningPriority(task, priority);
//      return;
//    }

    // TODO: поля wait_reason и pwait_que были удалены. Исправить!
//    if (task->state & TSK_STATE_WAIT) {
//      if (task->wait_reason == WAIT_REASON_MUTEX_I) {
//        task->priority = priority;
//        task = GetMutexByWaitQueque(task->pwait_que)->holder;
//        continue;
//      }
//    }

    task->priority = priority;
    return;
  }
}

static
int8_t GetMaxPriority(osMutex_t *mutex, int8_t ref_priority)
{
  int8_t priority;
  queue_t *curr_que;
  osThread_t *task;

  priority = ref_priority;
  curr_que = mutex->wait_que.next;
  while (curr_que != &mutex->wait_que) {
    task = GetThreadByQueue(curr_que);
    if (task->priority > priority) //--  task priority is higher
      priority = task->priority;

    curr_que = curr_que->next;
  }

  return priority;
}

static
void MutexUnLock(osMutex_t *mutex)
{
  queue_t *que;
  int8_t priority;
  osThread_t *task;

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
      if (task->state == osThreadReady || task->state == osThreadRunning) {
        _ThreadSetPriority(task, priority);
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
    mutex->holder = GetThreadByQueue(que);
    QueueAddTail(&mutex->holder->mutex_que, &mutex->mutex_que);
    mutex->cnt = 1U;

    _ThreadWaitExit(mutex->holder, (uint32_t)TERR_NO_ERR);
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
 *  function implementations (scope: module-local)
 ******************************************************************************/

static osMutexId_t MutexNew(const osMutexAttr_t *attr)
{
  if (mutex->id == ID_MUTEX)
    return TERR_NO_ERR;

  if (attr != NULL) {
    mutex->attr = (uint8_t)attr->attr_bits;
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

static const char *MutexGetName(osMutexId_t mutex_id)
{
  osMutex_t *mutex = mutex_id;

  /* Check parameters */
  if ((mutex == NULL) || (mutex->id != ID_MUTEX)) {
    return (NULL);
  }

  return (mutex->name);
}

static osStatus_t MutexAcquire(osMutexId_t mutex_id, uint32_t timeout)
{
  osError_t rc;

  if (mutex->id != ID_MUTEX)
    return TERR_NOEXS;

  osThread_t *task = ThreadGetRunning();

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
        /* Check if Priority inheritance protocol is enabled */
        if ((mutex->attr & osMutexPrioInherit) != 0U) {
          /* Raise priority of owner Task if lower than priority of running Task */
          if (task->priority > mutex->holder->priority) {
            SetPriority(mutex->holder, task->priority);
          }
        }
        /* Suspend current Thread */
        _ThreadWaitEnter(task, &mutex->wait_que, timeout);
        rc = TERR_WAIT;
      }
      else {
        rc = TERR_TIMEOUT;
      }
    }
  }

  return rc;
}

static osStatus_t MutexRelease(osMutexId_t mutex_id)
{
  if (mutex->id != ID_MUTEX)
    return TERR_NOEXS;

  /* Unlocking is enabled only for the owner and already locked mutex */
  if (ThreadGetRunning() != mutex->holder)
    return TERR_ILUSE;

  if (mutex->cnt == 0U)
    return TERR_ILUSE;

  mutex->cnt--;

  if (mutex->cnt == 0) {
    MutexUnLock(mutex);
  }

  return TERR_NO_ERR;
}

static osThreadId_t MutexGetOwner(osMutexId_t mutex_id)
{
  osMutex_t *mutex = mutex_id;

  /* Check parameters */
  if ((mutex == NULL) || (mutex->id != ID_MUTEX)) {
    return (NULL);
  }

  if (mutex->cnt == 0U) {
    return (NULL);
  }

  return (mutex->holder);
}

static osStatus_t MutexDelete(osMutexId_t mutex_id)
{
  if (mutex->id != ID_MUTEX)
    return TERR_NOEXS;

  /* Check if Mutex is locked */
  if (mutex->cnt != 0U) {
    /* Unblock waiting tasks */
    _ThreadWaitDelete(&mutex->wait_que);
    /* Unlock Mutex */
    MutexUnLock(mutex);
  }

  /* Mutex not exists now */
  mutex->id = ID_INVALID;

  return TERR_NO_ERR;
}

/*******************************************************************************
 *  Public API
 ******************************************************************************/

/**
 * @fn          osMutexId_t osMutexNew(const osMutexAttr_t *attr)
 * @brief       Create and Initialize a Mutex object.
 * @param[in]   attr  mutex attributes.
 * @return      mutex ID for reference by other functions or NULL in case of error.
 */
osMutexId_t osMutexNew(const osMutexAttr_t *attr)
{
  osMutexId_t mutex_id;

  if (IsIrqMode() || IsIrqMasked()) {
    mutex_id = NULL;
  }
  else {
    mutex_id = (osMutexId_t)svc_1((uint32_t)attr, (uint32_t)MutexNew);
  }

  return (mutex_id);
}

/**
 * @fn          const char *osMutexGetName(osMutexId_t mutex_id)
 * @brief       Get name of a Mutex object.
 * @param[in]   mutex_id  mutex ID obtained by \ref osMutexNew.
 * @return      name as null-terminated string or NULL in case of an error.
 */
const char *osMutexGetName(osMutexId_t mutex_id)
{
  const char *name;

  if (IsIrqMode() || IsIrqMasked()) {
    name = NULL;
  }
  else {
    name = (const char *)svc_1((uint32_t)mutex_id, (uint32_t)MutexGetName);
  }

  return (name);
}

/**
 * @fn          osStatus_t osMutexAcquire(osMutexId_t mutex_id, uint32_t timeout)
 * @brief       Acquire a Mutex or timeout if it is locked.
 * @param[in]   mutex_id  mutex ID obtained by \ref osMutexNew.
 * @param[in]   timeout   \ref CMSIS_RTOS_TimeOutValue or 0 in case of no time-out.
 * @return      status code that indicates the execution status of the function.
 */
osStatus_t osMutexAcquire(osMutexId_t mutex_id, uint32_t timeout)
{
  osStatus_t status;

  if (IsIrqMode() || IsIrqMasked()) {
    status = osErrorISR;
  }
  else {
    status = (osStatus_t)svc_2((uint32_t)mutex_id, timeout, (uint32_t)MutexAcquire);
    if (status == osThreadWait) {
      status = (osStatus_t)ThreadGetRunning()->winfo.ret_val;
    }
  }

  return (status);
}

/**
 * @fn          osStatus_t osMutexRelease(osMutexId_t mutex_id)
 * @brief       Release a Mutex that was acquired by \ref osMutexAcquire.
 * @param[in]   mutex_id  mutex ID obtained by \ref osMutexNew.
 * @return      status code that indicates the execution status of the function.
 */
osStatus_t osMutexRelease(osMutexId_t mutex_id)
{
  osStatus_t status;

  if (IsIrqMode() || IsIrqMasked()) {
    status = osErrorISR;
  }
  else {
    status = (osStatus_t)svc_1((uint32_t)mutex_id, (uint32_t)MutexRelease);
  }

  return (status);
}

/**
 * @fn          osThreadId_t osMutexGetOwner(osMutexId_t mutex_id)
 * @brief       Get Thread which owns a Mutex object.
 * @param[in]   mutex_id  mutex ID obtained by \ref osMutexNew.
 * @return      thread ID of owner thread or NULL when mutex was not acquired.
 */
osThreadId_t osMutexGetOwner(osMutexId_t mutex_id)
{
  osThreadId_t thread;

  if (IsIrqMode() || IsIrqMasked()) {
    thread = NULL;
  }
  else {
    thread = (osThreadId_t)svc_1((uint32_t)mutex_id, (uint32_t)MutexGetOwner);
  }

  return (thread);
}

/**
 * @fn          osStatus_t osMutexDelete(osMutexId_t mutex_id)
 * @brief       Delete a Mutex object.
 * @param[in]   mutex_id  mutex ID obtained by \ref osMutexNew.
 * @return      status code that indicates the execution status of the function.
 */
osStatus_t osMutexDelete(osMutexId_t mutex_id)
{
  osStatus_t status;

  if (IsIrqMode() || IsIrqMasked()) {
    status = osErrorISR;
  }
  else {
    status = (osStatus_t)svc_1((uint32_t)mutex_id, (uint32_t)MutexDelete);
  }

  return (status);
}

/*------------------------------ End of file ---------------------------------*/

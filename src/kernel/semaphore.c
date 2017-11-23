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

/*******************************************************************************
 *  function implementations (scope: module-local)
 ******************************************************************************/

/*******************************************************************************
 *  function implementations (scope: module-exported)
 ******************************************************************************/

/*-----------------------------------------------------------------------------*
 * Название : tn_sem_create
 * Описание :
 * Параметры:
 * Результат:
 *----------------------------------------------------------------------------*/
int tn_sem_create(TN_SEM *sem, int start_value, int max_val)
{
  if (sem == NULL)
    return  TERR_WRONG_PARAM;
  if (max_val <= 0 || start_value < 0 || start_value > max_val || sem->id == ID_SEMAPHORE)
    return TERR_WRONG_PARAM;

  QueueReset(&(sem->wait_queue));

  sem->count      = start_value;
  sem->max_count  = max_val;
  sem->id         = ID_SEMAPHORE;

  return TERR_NO_ERR;
}

/*-----------------------------------------------------------------------------*
 * Название : tn_sem_delete
 * Описание :
 * Параметры:
 * Результат:
 *----------------------------------------------------------------------------*/
int tn_sem_delete(TN_SEM *sem)
{
  if (sem == NULL)
    return TERR_WRONG_PARAM;
  if (sem->id != ID_SEMAPHORE)
    return TERR_NOEXS;

  BEGIN_CRITICAL_SECTION

  ThreadWaitDelete(&sem->wait_queue);

  sem->id = ID_INVALID; // Semaphore not exists now

  END_CRITICAL_SECTION

  return TERR_NO_ERR;
}

/*-----------------------------------------------------------------------------*
 * Название : tn_sem_signal
 * Описание :
 * Параметры:
 * Результат:
 *----------------------------------------------------------------------------*/
osError_t tn_sem_signal(TN_SEM *sem)
{
  osError_t rc = TERR_NO_ERR;
  CDLL_QUEUE *que;
  TN_TCB *task;

  if (sem == NULL)
    return TERR_WRONG_PARAM;
  if (sem->id != ID_SEMAPHORE)
    return TERR_NOEXS;

  BEGIN_CRITICAL_SECTION

  if (!(isQueueEmpty(&(sem->wait_queue)))) {
    //--- delete from the sem wait queue
    que = QueueRemoveHead(&(sem->wait_queue));
    task = get_task_by_tsk_queue(que);
    ThreadWaitComplete(task);
  }
  else {
    if (sem->count < sem->max_count) {
      sem->count++;
      rc = TERR_NO_ERR;
    }
    else
      rc = TERR_OVERFLOW;
  }

  END_CRITICAL_SECTION

  return rc;
}

/*-----------------------------------------------------------------------------*
 * Название : tn_sem_acquire
 * Описание :
 * Параметры:
 * Результат:
 *----------------------------------------------------------------------------*/
osError_t tn_sem_acquire(TN_SEM *sem, unsigned long timeout)
{
  osError_t rc; //-- return code
  TN_TCB *task;

  if (sem == NULL)
    return  TERR_WRONG_PARAM;
  if (sem->id != ID_SEMAPHORE)
    return TERR_NOEXS;

  BEGIN_CRITICAL_SECTION

  if (sem->count > 0) {
    sem->count--;
    rc = TERR_NO_ERR;
  }
  else {
    if (timeout == TN_POLLING) {
      rc = TERR_TIMEOUT;
    }
    else {
      task = TaskGetCurrent();
      task->wait_rc = &rc;
      ThreadToWaitAction(task, &(sem->wait_queue), WAIT_REASON_SEM, timeout);
    }
  }

  END_CRITICAL_SECTION

  return rc;
}

/*------------------------------ End of file ---------------------------------*/

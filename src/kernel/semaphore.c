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
void svcSemaphoreNew(void (*)(osSemaphore_t*, uint32_t, uint32_t), osSemaphore_t*, uint32_t, uint32_t);
__SVC(0)
osError_t svcSemaphoreDelete(osError_t (*)(osSemaphore_t*), osSemaphore_t*);
__SVC(0)
osError_t svcSemaphoreRelease(osError_t (*)(osSemaphore_t*), osSemaphore_t*);
__SVC(0)
osError_t svcSemaphoreAcquire(osError_t (*)(osSemaphore_t*, osTime_t), osSemaphore_t*, osTime_t);
__SVC(0)
uint32_t svcSemaphoreGetCount(uint32_t (*)(osSemaphore_t*), osSemaphore_t*);

/*******************************************************************************
 *  function implementations (scope: module-local)
 ******************************************************************************/

/**
 * @fn          osError_t SemaphoreNew(osSemaphore_t *sem, uint32_t initial_count, uint32_t max_count)
 * @brief       Creates a semaphore
 * @param[out]  sem             Pointer to the semaphore structure to be created
 * @param[in]   initial_count   Initial number of available tokens
 * @param[in]   max_count       Maximum number of available tokens
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 */
static
void SemaphoreNew(osSemaphore_t *sem, uint32_t initial_count, uint32_t max_count)
{
  QueueReset(&sem->wait_queue);

  sem->count      = initial_count;
  sem->max_count  = max_count;
  sem->id         = ID_SEMAPHORE;
}

/**
 * @fn          osError_t SemaphoreDelete(osSemaphore_t *sem)
 * @brief       Deletes a semaphore
 * @param[out]  sem   Pointer to the semaphore structure to be deleted
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_NOEXS        Object is not a semaphore or non-existent
 */
static
osError_t SemaphoreDelete(osSemaphore_t *sem)
{
  if (sem->id != ID_SEMAPHORE)
    return TERR_NOEXS;

  TaskWaitDelete(&sem->wait_queue);
  sem->id = ID_INVALID;

  return TERR_NO_ERR;
}

/**
 * @fn          osError_t SemaphoreRelease(osSemaphore_t *sem)
 * @brief       Release a Semaphore token up to the initial maximum count.
 * @param[out]  sem   Pointer to the semaphore structure be released
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_OVERFLOW     Semaphore Resource has max_count value
 *              TERR_NOEXS        Object is not a semaphore or non-existent
 */
static
osError_t SemaphoreRelease(osSemaphore_t *sem)
{
  if (sem->id != ID_SEMAPHORE)
    return TERR_NOEXS;

  BEGIN_CRITICAL_SECTION

  if (!isQueueEmpty(&sem->wait_queue)) {
    TaskWaitComplete(GetTaskByQueue(QueueRemoveHead(&sem->wait_queue)), (uint32_t)TERR_NO_ERR);
    END_CRITICAL_SECTION
    return TERR_NO_ERR;
  }

  if (sem->count < sem->max_count) {
    sem->count++;
    END_CRITICAL_SECTION
    return TERR_NO_ERR;
  }

  END_CRITICAL_SECTION
  return TERR_OVERFLOW;
}

/**
 * @fn          osError_t osSemaphoreAcquire(osSemaphore_t *sem, osTime_t timeout)
 * @brief       Acquire a Semaphore token or timeout if no tokens are available.
 * @param[out]  sem       Pointer to the semaphore structure to be acquired
 * @param[in]   timeout   Timeout value must be equal or greater than 0
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_TIMEOUT      Timeout expired
 *              TERR_NOEXS        Object is not a semaphore or non-existent
 */
static
osError_t SemaphoreAcquire(osSemaphore_t *sem, osTime_t timeout)
{
  if (sem->id != ID_SEMAPHORE)
    return TERR_NOEXS;

  BEGIN_CRITICAL_SECTION

  if (sem->count > 0U) {
    sem->count--;
    END_CRITICAL_SECTION
    return TERR_NO_ERR;
  }

  if (timeout == 0U) {
    END_CRITICAL_SECTION
    return TERR_TIMEOUT;
  }

  TaskWaitEnter(TaskGetCurrent(), &sem->wait_queue, WAIT_REASON_SEM, timeout);

  END_CRITICAL_SECTION
  return TERR_WAIT;
}

static
uint32_t SemaphoreGetCount(osSemaphore_t *sem)
{
  if (sem->id != ID_SEMAPHORE)
    return 0U;

  return sem->count;
}

/*******************************************************************************
 *  function implementations (scope: module-exported)
 ******************************************************************************/

/**
 * @fn          osError_t osSemaphoreNew(osSemaphore_t *sem, uint32_t initial_count, uint32_t max_count)
 * @brief       Creates a semaphore
 * @param[out]  sem             Pointer to the semaphore structure to be created
 * @param[in]   initial_count   Initial number of available tokens
 * @param[in]   max_count       Maximum number of available tokens
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_ISR          The function cannot be called from interrupt service routines
 */
osError_t osSemaphoreNew(osSemaphore_t *sem, uint32_t initial_count, uint32_t max_count)
{
  if (sem == NULL || max_count == 0U || initial_count > max_count)
    return TERR_WRONG_PARAM;
  if (sem->id == ID_SEMAPHORE)
    return TERR_NO_ERR;
  if (IS_IRQ_MODE() || IS_IRQ_MASKED())
    return TERR_ISR;

  svcSemaphoreNew(SemaphoreNew, sem, initial_count, max_count);

  return TERR_NO_ERR;
}

/**
 * @fn          osError_t osSemaphoreDelete(osSemaphore_t *sem)
 * @brief       Deletes a semaphore
 * @param[out]  sem   Pointer to the semaphore structure to be deleted
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_NOEXS        Object is not a semaphore or non-existent
 *              TERR_ISR          The function cannot be called from interrupt service routines
 */
osError_t osSemaphoreDelete(osSemaphore_t *sem)
{
  if (sem == NULL)
    return TERR_WRONG_PARAM;
  if (IS_IRQ_MODE() || IS_IRQ_MASKED())
    return TERR_ISR;

  return svcSemaphoreDelete(SemaphoreDelete, sem);
}

/**
 * @fn          osError_t osSemaphoreRelease(osSemaphore_t *sem)
 * @brief       Release a Semaphore token up to the initial maximum count.
 * @param[out]  sem   Pointer to the semaphore structure be released
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_NOEXS        Object is not a semaphore or non-existent
 *              TERR_OVERFLOW     Semaphore Resource has max_count value
 */
osError_t osSemaphoreRelease(osSemaphore_t *sem)
{
  if (sem == NULL)
    return TERR_WRONG_PARAM;

  if (IS_IRQ_MODE() || IS_IRQ_MASKED())
    return SemaphoreRelease(sem);
  else
    return svcSemaphoreRelease(SemaphoreRelease, sem);
}

/**
 * @fn          osError_t osSemaphoreAcquire(osSemaphore_t *sem, osTime_t timeout)
 * @brief       Acquire a Semaphore token or timeout if no tokens are available.
 * @param[out]  sem       Pointer to the semaphore structure to be acquired
 * @param[in]   timeout   Timeout value must be equal or greater than 0
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_TIMEOUT      Timeout expired
 *              TERR_NOEXS        Object is not a semaphore or non-existent
 */
osError_t osSemaphoreAcquire(osSemaphore_t *sem, osTime_t timeout)
{
  if (sem == NULL)
    return  TERR_WRONG_PARAM;

  if (IS_IRQ_MODE() || IS_IRQ_MASKED()) {
    if (timeout != 0U)
      return TERR_WRONG_PARAM;

    return SemaphoreAcquire(sem, timeout);
  }
  else {
    osError_t ret_val = svcSemaphoreAcquire(SemaphoreAcquire, sem, timeout);

    if (ret_val == TERR_WAIT)
      return (osError_t)TaskGetCurrent()->wait_info.ret_val;

    return ret_val;
  }
}

/**
 * @fn          uint32_t osSemaphoreGetCount(osSemaphore_t *sem)
 * @brief       Returns the number of available tokens of the semaphore object
 * @param[out]  sem   Pointer to the semaphore structure to be acquired
 * @return      Number of tokens available or 0 in case of an error
 */
uint32_t osSemaphoreGetCount(osSemaphore_t *sem)
{
  if (sem == NULL)
    return 0U;

  if (IS_IRQ_MODE() || IS_IRQ_MASKED()) {
    return SemaphoreGetCount(sem);
  }
  else {
    return svcSemaphoreGetCount(SemaphoreGetCount, sem);
  }
}

/*------------------------------ End of file ---------------------------------*/

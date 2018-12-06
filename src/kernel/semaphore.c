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

#if defined(__CC_ARM)

__SVC_INDIRECT(0)
void __svcSemaphoreNew(void (*)(osSemaphore_t*, uint32_t, uint32_t), osSemaphore_t*, uint32_t, uint32_t);

__STATIC_FORCEINLINE
void svcSemaphoreNew(osSemaphore_t *sem, uint32_t initial_count, uint32_t max_count)
{
  __svcSemaphoreNew(SemaphoreNew, sem, initial_count, max_count);
}

__SVC_INDIRECT(0)
osError_t __svcSemaphoreDelete(osError_t (*)(osSemaphore_t*), osSemaphore_t*);

__STATIC_FORCEINLINE
osError_t svcSemaphoreDelete(osSemaphore_t *sem)
{
  return __svcSemaphoreDelete(SemaphoreDelete, sem);
}

__SVC_INDIRECT(0)
osError_t __svcSemaphoreRelease(osError_t (*)(osSemaphore_t*), osSemaphore_t*);

__STATIC_FORCEINLINE
osError_t svcSemaphoreRelease(osSemaphore_t *sem)
{
  return __svcSemaphoreRelease(SemaphoreRelease, sem);
}

__SVC_INDIRECT(0)
osError_t __svcSemaphoreAcquire(osError_t (*)(osSemaphore_t*, osTime_t), osSemaphore_t*, osTime_t);

__STATIC_FORCEINLINE
osError_t svcSemaphoreAcquire(osSemaphore_t *sem, osTime_t timeout)
{
  return __svcSemaphoreAcquire(SemaphoreAcquire, sem, timeout);
}

__SVC_INDIRECT(0)
uint32_t __svcSemaphoreGetCount(uint32_t (*)(osSemaphore_t*), osSemaphore_t*);

__STATIC_FORCEINLINE
uint32_t svcSemaphoreGetCount(osSemaphore_t *sem)
{
  return __svcSemaphoreGetCount(SemaphoreGetCount, sem);
}

#elif defined(__ICCARM__)

__SVC_INDIRECT(0)
void __svcSemaphoreNew(osSemaphore_t*, uint32_t, uint32_t);

__STATIC_FORCEINLINE
void svcSemaphoreNew(osSemaphore_t *sem, uint32_t initial_count, uint32_t max_count)
{
  SVC_ArgF(SemaphoreNew);
  __svcSemaphoreNew(sem, initial_count, max_count);
}

__SVC_INDIRECT(0)
osError_t __svcSemaphoreDelete(osSemaphore_t*);

__STATIC_FORCEINLINE
osError_t svcSemaphoreDelete(osSemaphore_t *sem)
{
  SVC_ArgF(SemaphoreDelete);

  return __svcSemaphoreDelete(sem);
}

__SVC_INDIRECT(0)
osError_t __svcSemaphoreRelease(osSemaphore_t*);

__STATIC_FORCEINLINE
osError_t svcSemaphoreRelease(osSemaphore_t *sem)
{
  SVC_ArgF(SemaphoreRelease);

  return __svcSemaphoreRelease(sem);
}

__SVC_INDIRECT(0)
osError_t __svcSemaphoreAcquire(osSemaphore_t*, osTime_t);

__STATIC_FORCEINLINE
osError_t svcSemaphoreAcquire(osSemaphore_t *sem, osTime_t timeout)
{
  SVC_ArgF(SemaphoreAcquire);

  return __svcSemaphoreAcquire(sem, timeout);
}

__SVC_INDIRECT(0)
uint32_t __svcSemaphoreGetCount(osSemaphore_t*);

__STATIC_FORCEINLINE
uint32_t svcSemaphoreGetCount(osSemaphore_t *sem)
{
  SVC_ArgF(SemaphoreGetCount);

  return __svcSemaphoreGetCount(sem);
}

#else   // !(defined(__CC_ARM) || defined(__ICCARM__))

__STATIC_FORCEINLINE
void svcSemaphoreNew(osSemaphore_t *sem, uint32_t initial_count, uint32_t max_count)
{
  register uint32_t rf  __ASM(SVC_REG)  = (uint32_t)SemaphoreNew;
  register uint32_t r0  __ASM("r0")     = (uint32_t)sem;
  register uint32_t r1  __ASM("r1")     = (uint32_t)initial_count;
  register uint32_t r2  __ASM("r2")     = (uint32_t)max_count;

  __ASM volatile ("svc 0" :: "r"(rf),"r"(r0),"r"(r1),"r"(r2));
}

__STATIC_FORCEINLINE
osError_t svcSemaphoreDelete(osSemaphore_t *sem)
{
  register uint32_t rf  __ASM(SVC_REG)  = (uint32_t)SemaphoreDelete;
  register uint32_t r0  __ASM("r0")     = (uint32_t)sem;

  __ASM volatile ("svc 0" : "=r"(r0) : "r"(rf),"r"(r0) : "r1");

  return ((osError_t)r0);
}

__STATIC_FORCEINLINE
osError_t svcSemaphoreRelease(osSemaphore_t *sem)
{
  register uint32_t rf  __ASM(SVC_REG)  = (uint32_t)SemaphoreRelease;
  register uint32_t r0  __ASM("r0")     = (uint32_t)sem;

  __ASM volatile ("svc 0" : "=r"(r0) : "r"(rf),"r"(r0) : "r1");

  return ((osError_t)r0);
}

__STATIC_FORCEINLINE
osError_t svcSemaphoreAcquire(osSemaphore_t *sem, osTime_t timeout)
{
  register uint32_t rf  __ASM(SVC_REG)  = (uint32_t)SemaphoreAcquire;
  register uint32_t r0  __ASM("r0")     = (uint32_t)sem;
  register uint32_t r1  __ASM("r1")     = (uint32_t)timeout;

  __ASM volatile ("svc 0" : "=r"(r0) : "r"(rf),"r"(r0),"r"(r1));

  return ((osError_t)r0);
}

__STATIC_FORCEINLINE
uint32_t svcSemaphoreGetCount(osSemaphore_t *sem)
{
  register uint32_t rf  __ASM(SVC_REG)  = (uint32_t)SemaphoreGetCount;
  register uint32_t r0  __ASM("r0")     = (uint32_t)sem;

  __ASM volatile ("svc 0" : "=r"(r0) : "r"(rf),"r"(r0) : "r1");

  return (r0);
}

#endif

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
  if (IsIrqMode() || IsIrqMasked())
    return TERR_ISR;

  svcSemaphoreNew(sem, initial_count, max_count);

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
  if (IsIrqMode() || IsIrqMasked())
    return TERR_ISR;

  return svcSemaphoreDelete(sem);
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

  if (IsIrqMode() || IsIrqMasked())
    return SemaphoreRelease(sem);
  else
    return svcSemaphoreRelease(sem);
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

  if (IsIrqMode() || IsIrqMasked()) {
    if (timeout != 0U)
      return TERR_WRONG_PARAM;

    return SemaphoreAcquire(sem, timeout);
  }
  else {
    osError_t ret_val = svcSemaphoreAcquire(sem, timeout);

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

  if (IsIrqMode() || IsIrqMasked()) {
    return SemaphoreGetCount(sem);
  }
  else {
    return svcSemaphoreGetCount(sem);
  }
}

/*------------------------------ End of file ---------------------------------*/

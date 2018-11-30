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

static
uint32_t FlagsSet(osEventFlags_t *evf, uint32_t flags)
{
  uint32_t event_flags;

  evf->pattern |= flags;
  event_flags = evf->pattern;

  return event_flags;
}

static
uint32_t FlagsCheck (osEventFlags_t *evf, uint32_t flags, uint32_t options)
{
  uint32_t pattern;

  if ((options & osFlagsNoClear) == 0U) {
    pattern = evf->pattern;

    if ((((options & osFlagsWaitAll) != 0U) && ((pattern & flags) != flags)) ||
        (((options & osFlagsWaitAll) == 0U) && ((pattern & flags) == 0U)))
    {
      pattern = 0U;
    }
    else {
      evf->pattern &= ~flags;
    }
  }
  else {
    pattern = evf->pattern;

    if ((((options & osFlagsWaitAll) != 0U) && ((pattern & flags) != flags)) ||
        (((options & osFlagsWaitAll) == 0U) && ((pattern & flags) == 0U)))
    {
      pattern = 0U;
    }
  }

  return pattern;
}

static
osError_t EventFlagsNew(osEventFlags_t *evf)
{
  if (evf->id == ID_EVENT_FLAGS)
    return TERR_NO_ERR;

  QueueReset(&evf->wait_queue);

  evf->pattern = 0U;
  evf->id = ID_EVENT_FLAGS;

  return TERR_NO_ERR;
}

static
osError_t EventFlagsDelete(osEventFlags_t *evf)
{
  if (evf->id != ID_EVENT_FLAGS)
    return TERR_NOEXS;

  TaskWaitDelete(&evf->wait_queue);

  evf->id = ID_INVALID;

  return TERR_NO_ERR;
}

static
uint32_t EventFlagsSet(osEventFlags_t *evf, uint32_t flags)
{
  uint32_t event_flags, pattern;
  queue_t *que;
  osTask_t *task;

  if (evf->id != ID_EVENT_FLAGS)
    return (uint32_t)TERR_NOEXS;

  BEGIN_CRITICAL_SECTION

  /* Set Event Flags */
  event_flags = FlagsSet(evf, flags);

  que = evf->wait_queue.next;
  while (que != &evf->wait_queue) {
    task = GetTaskByQueue(que);
    que = que->next;

    pattern = FlagsCheck(evf, task->wait_info.event.flags, task->wait_info.event.options);

    if (pattern) {
      if (!(task->wait_info.event.options & osFlagsNoClear)) {
        event_flags = pattern & ~task->wait_info.event.flags;
      }
      else {
        event_flags = pattern;
      }
      TaskWaitComplete(task, pattern);
    }
  }

  END_CRITICAL_SECTION

  return event_flags;
}

static
uint32_t EventFlagsWait(osEventFlags_t *evf, uint32_t flags, uint32_t options, osTime_t timeout)
{
  uint32_t pattern;

  if (evf->id != ID_EVENT_FLAGS)
    return (uint32_t)TERR_NOEXS;

  BEGIN_CRITICAL_SECTION

  pattern = FlagsCheck(evf, flags, options);

  if (pattern) {
    END_CRITICAL_SECTION
    return pattern;
  }

  if (timeout) {
    osTask_t *task = TaskGetCurrent();
    task->wait_info.event.options = options;
    task->wait_info.event.flags = flags;
    TaskWaitEnter(task, &evf->wait_queue, WAIT_REASON_EVENT, timeout);
    END_CRITICAL_SECTION
    return (uint32_t)TERR_WAIT;
  }

  END_CRITICAL_SECTION

  return (uint32_t)TERR_TIMEOUT;
}

static
uint32_t EventFlagsClear(osEventFlags_t *evf, uint32_t flags)
{
  uint32_t pattern;

  if (evf->id != ID_EVENT_FLAGS)
    return (uint32_t)TERR_NOEXS;

  BEGIN_CRITICAL_SECTION

  pattern = evf->pattern;
  evf->pattern &= ~flags;

  END_CRITICAL_SECTION

  return pattern;
}

#if defined(__CC_ARM)

__SVC_INDIRECT(0)
osError_t __svcEventFlagsNew(osError_t (*)(osEventFlags_t*), osEventFlags_t*);

__STATIC_FORCEINLINE
osError_t svcEventFlagsNew(osEventFlags_t *evf)
{
  return __svcEventFlagsNew(EventFlagsNew, evf);
}

__SVC_INDIRECT(0)
osError_t __svcEventFlagsDelete(osError_t (*)(osEventFlags_t*), osEventFlags_t*);

__STATIC_FORCEINLINE
osError_t svcEventFlagsDelete(osEventFlags_t *evf)
{
  return __svcEventFlagsDelete(EventFlagsDelete, evf);
}

__SVC_INDIRECT(0)
uint32_t __svcEventFlagsSet(uint32_t (*)(osEventFlags_t*, uint32_t), osEventFlags_t*, uint32_t);

__STATIC_FORCEINLINE
uint32_t svcEventFlagsSet(osEventFlags_t *evf, uint32_t flags)
{
  return __svcEventFlagsSet(EventFlagsSet, evf, flags);
}

__SVC_INDIRECT(0)
uint32_t __svcEventFlagsWait(uint32_t (*)(osEventFlags_t*, uint32_t, uint32_t, osTime_t), osEventFlags_t*, uint32_t, uint32_t, osTime_t);

__STATIC_FORCEINLINE
uint32_t svcEventFlagsWait(osEventFlags_t *evf, uint32_t flags, uint32_t options, osTime_t timeout)
{
  return __svcEventFlagsWait(EventFlagsWait, evf, flags, options, timeout);
}

__SVC_INDIRECT(0)
uint32_t __svcEventFlagsClear(uint32_t (*)(osEventFlags_t*, uint32_t), osEventFlags_t*, uint32_t);

__STATIC_FORCEINLINE
uint32_t svcEventFlagsClear(osEventFlags_t *evf, uint32_t flags)
{
  return __svcEventFlagsClear(EventFlagsClear, evf, flags);
}

#elif defined(__ICCARM__)

#else   // !(defined(__CC_ARM) || defined(__ICCARM__))

__STATIC_FORCEINLINE
osError_t svcEventFlagsNew(osEventFlags_t *evf)
{
  register uint32_t rf  __ASM(SVC_REG)  = (uint32_t)EventFlagsNew;
  register uint32_t r0  __ASM("r0")     = (uint32_t)evf;

  __ASM volatile ("svc 0" : "=r"(r0) : "r"(rf),"r"(r0) : "r1");

  return ((osError_t)r0);
}

__STATIC_FORCEINLINE
osError_t svcEventFlagsDelete(osEventFlags_t *evf)
{
  register uint32_t rf  __ASM(SVC_REG)  = (uint32_t)EventFlagsDelete;
  register uint32_t r0  __ASM("r0")     = (uint32_t)evf;

  __ASM volatile ("svc 0" : "=r"(r0) : "r"(rf),"r"(r0) : "r1");

  return ((osError_t)r0);
}

__STATIC_FORCEINLINE
uint32_t svcEventFlagsSet(osEventFlags_t *evf, uint32_t flags)
{
  register uint32_t rf  __ASM(SVC_REG)  = (uint32_t)EventFlagsSet;
  register uint32_t r0  __ASM("r0")     = (uint32_t)evf;
  register uint32_t r1  __ASM("r1")     = (uint32_t)flags;

  __ASM volatile ("svc 0" : "=r"(r0) : "r"(rf),"r"(r0),"r"(r1));

  return (r0);
}

__STATIC_FORCEINLINE
uint32_t svcEventFlagsWait(osEventFlags_t *evf, uint32_t flags, uint32_t options, osTime_t timeout)
{
  register uint32_t rf  __ASM(SVC_REG)  = (uint32_t)EventFlagsWait;
  register uint32_t r0  __ASM("r0")     = (uint32_t)evf;
  register uint32_t r1  __ASM("r1")     = (uint32_t)flags;
  register uint32_t r2  __ASM("r2")     = (uint32_t)options;
  register uint32_t r3  __ASM("r3")     = (uint32_t)timeout;

  __ASM volatile ("svc 0" : "=r"(r0) : "r"(rf),"r"(r0),"r"(r1),"r"(r2),"r"(r3));

  return (r0);
}

__STATIC_FORCEINLINE
uint32_t svcEventFlagsClear(osEventFlags_t *evf, uint32_t flags)
{
  register uint32_t rf  __ASM(SVC_REG)  = (uint32_t)EventFlagsClear;
  register uint32_t r0  __ASM("r0")     = (uint32_t)evf;
  register uint32_t r1  __ASM("r1")     = (uint32_t)flags;

  __ASM volatile ("svc 0" : "=r"(r0) : "r"(rf),"r"(r0),"r"(r1));

  return (r0);
}

#endif

/*******************************************************************************
 *  function implementations (scope: module-exported)
 ******************************************************************************/

/**
 * @fn          osError_t osEventFlagsNew(osEventFlags_t *evf)
 * @brief       Creates a new event flags object
 * @param[out]  evf   Pointer to osEventFlags_t structure of the event
 * @return      TERR_NO_ERR       The event flags object has been created
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_ISR          Cannot be called from interrupt service routines
 */
osError_t osEventFlagsNew(osEventFlags_t *evf)
{
  if (evf == NULL)
    return TERR_WRONG_PARAM;
  if (IsIrqMode() || IsIrqMasked())
    return TERR_ISR;

  return svcEventFlagsNew(evf);
}

/**
 * @fn          osError_t osEventFlagsDelete(osEventFlags_t *evf)
 * @brief       Deletes the event flags object
 * @param[out]  evf   Pointer to osEventFlags_t structure of the event
 * @return      TERR_NO_ERR       The event flags object has been deleted
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_NOEXS        Object is not a Message Queue or non-existent
 *              TERR_ISR          Cannot be called from interrupt service routines
 */
osError_t osEventFlagsDelete(osEventFlags_t *evf)
{
  if (evf == NULL)
    return TERR_WRONG_PARAM;
  if (IsIrqMode() || IsIrqMasked())
    return TERR_ISR;

  return svcEventFlagsDelete(evf);
}

/**
 * @fn          uint32_t osEventFlagsSet(osEventFlags_t *evf, uint32_t flags)
 * @brief       Sets the event flags
 * @param[out]  evf   Pointer to osEventFlags_t structure of the event
 * @param[in]   flags Specifies the flags that shall be set
 * @return      The event flags after setting or an error code if highest bit is set
 *              (refer to osError_t)
 */
uint32_t osEventFlagsSet(osEventFlags_t *evf, uint32_t flags)
{
  if (evf == NULL || flags == 0U)
    return (uint32_t)TERR_WRONG_PARAM;

  if (IsIrqMode() || IsIrqMasked()) {
    return EventFlagsSet(evf, flags);
  }
  else {
    return svcEventFlagsSet(evf, flags);
  }
}

/**
 * @fn          uint32_t osEventFlagsWait(osEventFlags_t *evf, uint32_t flags, uint32_t options, osTime_t timeout)
 * @brief       Suspends the execution of the currently RUNNING task until any
 *              or all event flags in the event object are set. When these event
 *              flags are already set, the function returns instantly.
 * @param[out]  evf       Pointer to osEventFlags_t structure of the event
 * @param[in]   flags     Specifies the flags to wait for
 * @param[in]   options   Specifies flags options (osFlagsXxxx)
 * @param[in]   timeout   Timeout Value or 0 in case of no time-out
 * @return      Event flags before clearing or error code if highest bit set
 */
uint32_t osEventFlagsWait(osEventFlags_t *evf, uint32_t flags, uint32_t options, osTime_t timeout)
{
  if (evf == NULL || flags == 0)
    return (uint32_t)TERR_WRONG_PARAM;

  if (IsIrqMode() || IsIrqMasked()) {
    if (timeout != 0U)
      return (uint32_t)TERR_WRONG_PARAM;

    return EventFlagsWait(evf, flags, options, timeout);
  }
  else {
    uint32_t ret_val = svcEventFlagsWait(evf, flags, options, timeout);

    if (ret_val == (uint32_t)TERR_WAIT)
      return TaskGetCurrent()->wait_info.ret_val;

    return ret_val;
  }
}

/**
 * @fn          uint32_t osEventFlagsClear(osEventFlags_t *evf, uint32_t flags)
 * @brief       Clears the event flags in an event flags object
 * @param[out]  evf     Pointer to osEventFlags_t structure of the event
 * @param[in]   flags   Specifies the flags that shall be cleared
 * @return      Event flags before clearing or error code if highest bit set
 */
uint32_t osEventFlagsClear(osEventFlags_t *evf, uint32_t flags)
{
  if (evf == NULL || flags == 0)
    return (uint32_t)TERR_WRONG_PARAM;

  if (IsIrqMode() || IsIrqMasked()) {
    return EventFlagsClear(evf, flags);
  }
  else {
    return svcEventFlagsClear(evf, flags);
  }
}

/* ----------------------------- End of file ---------------------------------*/


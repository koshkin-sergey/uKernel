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

static osEventFlagsId_t EventFlagsNew(const osEventFlagsAttr_t *attr)
{
  osEventFlags_t *evf;

  if (attr == NULL)
    return (NULL);

  /* Initialize control block */
  evf->id = ID_EVENT_FLAGS;
  evf->flags = 0U;
  evf->name = attr->name;
  evf->pattern = 0U;

  QueueReset(&evf->wait_queue);

  return (evf);
}

static
osError_t EventFlagsDelete(osEventFlags_t *evf)
{
  if (evf->id != ID_EVENT_FLAGS)
    return TERR_NOEXS;

  _ThreadWaitDelete(&evf->wait_queue);

  evf->id = ID_INVALID;

  return TERR_NO_ERR;
}

static
uint32_t EventFlagsSet(osEventFlags_t *evf, uint32_t flags)
{
  uint32_t event_flags, pattern;
  queue_t *que;
  osThread_t *task;

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
      _ThreadWaitExit(task, pattern);
    }
  }

  END_CRITICAL_SECTION

  return event_flags;
}

static
uint32_t EventFlagsWait(osEventFlags_t *evf, uint32_t flags, uint32_t options, uint32_t timeout)
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
    osThread_t *task = ThreadGetRunning();
    task->wait_info.event.options = options;
    task->wait_info.event.flags = flags;
    _ThreadWaitEnter(task, &evf->wait_queue, timeout);
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

/*******************************************************************************
 *  function implementations (scope: module-exported)
 ******************************************************************************/

/**
 * @fn          osEventFlagsId_t osEventFlagsNew(const osEventFlagsAttr_t *attr)
 * @brief       Create and Initialize an Event Flags object.
 * @param[in]   attr  event flags attributes.
 * @return      event flags ID for reference by other functions or NULL in case of error.
 */
osEventFlagsId_t osEventFlagsNew(const osEventFlagsAttr_t *attr)
{
  osEventFlagsId_t ef_id;

  if (IsIrqMode() || IsIrqMasked()) {
    ef_id = NULL;
  }
  else {
    ef_id = (osEventFlagsId_t)svc_1((uint32_t)attr, (uint32_t)EventFlagsNew);
  }

  return ef_id;
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

  return (osError_t)svc_1((uint32_t)evf, (uint32_t)EventFlagsDelete);
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
    return svc_2((uint32_t)evf, flags, (uint32_t)EventFlagsSet);
  }
}

/**
 * @fn          uint32_t osEventFlagsWait(osEventFlags_t *evf, uint32_t flags, uint32_t options, uint32_t timeout)
 * @brief       Suspends the execution of the currently RUNNING task until any
 *              or all event flags in the event object are set. When these event
 *              flags are already set, the function returns instantly.
 * @param[out]  evf       Pointer to osEventFlags_t structure of the event
 * @param[in]   flags     Specifies the flags to wait for
 * @param[in]   options   Specifies flags options (osFlagsXxxx)
 * @param[in]   timeout   Timeout Value or 0 in case of no time-out
 * @return      Event flags before clearing or error code if highest bit set
 */
uint32_t osEventFlagsWait(osEventFlags_t *evf, uint32_t flags, uint32_t options, uint32_t timeout)
{
  if (evf == NULL || flags == 0)
    return (uint32_t)TERR_WRONG_PARAM;

  if (IsIrqMode() || IsIrqMasked()) {
    if (timeout != 0U)
      return (uint32_t)TERR_WRONG_PARAM;

    return EventFlagsWait(evf, flags, options, timeout);
  }
  else {
    uint32_t ret_val = svc_4((uint32_t)evf, flags, options, (uint32_t)timeout, (uint32_t)EventFlagsWait);

    if (ret_val == (uint32_t)TERR_WAIT)
      return ThreadGetRunning()->wait_info.ret_val;

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
    return svc_2((uint32_t)evf, flags, (uint32_t)EventFlagsClear);
  }
}

/* ----------------------------- End of file ---------------------------------*/

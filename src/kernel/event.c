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
osError_t svcEventFlagsNew(osError_t (*)(osEventFlags_t*), osEventFlags_t*);
__SVC(0)
osError_t svcEventFlagsDelete(osError_t (*)(osEventFlags_t*), osEventFlags_t*);
__SVC(0)
uint32_t svcEventFlagsSet(uint32_t (*)(osEventFlags_t*, uint32_t), osEventFlags_t*, uint32_t);
__SVC(0)
uint32_t svcEventFlagsWait(uint32_t (*)(osEventFlags_t*, uint32_t, uint32_t, osTime_t), osEventFlags_t*, uint32_t, uint32_t, osTime_t);
__SVC(0)
uint32_t svcEventFlagsClear(uint32_t (*)(osEventFlags_t*, uint32_t), osEventFlags_t*, uint32_t);

/*******************************************************************************
 *  function implementations (scope: module-local)
 ******************************************************************************/

static
uint32_t FlagsSet(osEventFlags_t *evf, uint32_t flags)
{
  uint32_t event_flags;

  BEGIN_CRITICAL_SECTION

  evf->pattern |= flags;
  event_flags = evf->pattern;

  END_CRITICAL_SECTION

  return event_flags;
}

static
uint32_t FlagsCheck (osEventFlags_t *evf, uint32_t flags, uint32_t options)
{
  uint32_t pattern;

  if ((options & osFlagsNoClear) == 0U) {

    BEGIN_CRITICAL_SECTION

    pattern = evf->pattern;

    if ((((options & osFlagsWaitAll) != 0U) && ((pattern & flags) != flags)) ||
        (((options & osFlagsWaitAll) == 0U) && ((pattern & flags) == 0U)))
    {
      pattern = 0U;
    }
    else {
      evf->pattern &= ~flags;
    }

    END_CRITICAL_SECTION
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

  /* Set Event Flags */
  event_flags = FlagsSet(evf, flags);

  BEGIN_CRITICAL_SECTION

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

  pattern = FlagsCheck(evf, flags, options);

  if (pattern)
    return pattern;

  BEGIN_CRITICAL_SECTION

  if (timeout) {
    osTask_t *task = TaskGetCurrent();
    task->wait_info.event.options = options;
    task->wait_info.event.flags = flags;
    TaskWaitEnter(task, &evf->wait_queue, WAIT_REASON_EVENT, timeout);
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

  BEGIN_DISABLE_INTERRUPT

  pattern = evf->pattern;
  evf->pattern &= ~flags;

  END_DISABLE_INTERRUPT

  return pattern;
}

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
  if (IS_IRQ_MODE() || IS_IRQ_MASKED())
    return TERR_ISR;

  return svcEventFlagsNew(EventFlagsNew, evf);
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
  if (IS_IRQ_MODE() || IS_IRQ_MASKED())
    return TERR_ISR;

  return svcEventFlagsDelete(EventFlagsDelete, evf);
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

  if (IS_IRQ_MODE() || IS_IRQ_MASKED()) {
    return EventFlagsSet(evf, flags);
  }
  else {
    return svcEventFlagsSet(EventFlagsSet, evf, flags);
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

  if (IS_IRQ_MODE() || IS_IRQ_MASKED()) {
    if (timeout != 0U)
      return (uint32_t)TERR_WRONG_PARAM;

    return EventFlagsWait(evf, flags, options, timeout);
  }
  else {
    return svcEventFlagsWait(EventFlagsWait, evf, flags, options, timeout);
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

  if (IS_IRQ_MODE() || IS_IRQ_MASKED()) {
    return EventFlagsClear(evf, flags);
  }
  else {
    return svcEventFlagsClear(EventFlagsClear, evf, flags);
  }
}

/* ----------------------------- End of file ---------------------------------*/


/*
 * Copyright (C) 2011-2018 Sergey Koshkin <koshkin.sergey@gmail.com>
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
osTime_t CyclicNextTime(TN_CYCLIC *cyc)
{
  osTime_t time, jiffies;
  uint32_t n;

  time = cyc->timer.time + cyc->time;
  jiffies = knlInfo.jiffies;

  if (time_before_eq(time, jiffies)) {
    time = jiffies - cyc->timer.time;
    n = time / cyc->time;
    n++;
    time = n * cyc->time;
    time = cyc->timer.time + time;
  }

  return time;
}

static
void AlarmHandler(TN_ALARM *alarm)
{
  if (alarm == NULL)
    return;

  alarm->state = TIMER_STOP;
  alarm->handler(alarm->exinf);
}

static
void CyclicHandler(TN_CYCLIC *cyc)
{
  TimerInsert(&cyc->timer, CyclicNextTime(cyc), (CBACK)CyclicHandler, cyc);
  cyc->handler(cyc->exinf);
}

void TimerInsert(timer_t *event, osTime_t time, CBACK callback, void *arg)
{
  queue_t *que;
  timer_t *timer;
  queue_t *timer_queue = &knlInfo.timer_queue;

  event->callback = callback;
  event->arg  = arg;
  event->time = time;

  for (que = timer_queue->next; que != timer_queue; que = que->next) {
    timer = GetTimerByQueue(que);
    if (time_before(event->time, timer->time))
      break;
  }

  QueueAddTail(que, &event->timer_que);
}

/**
 * @fn          void TimerDelete(timer_t *event)
 * @brief
 */
void TimerDelete(timer_t *event)
{
  QueueRemoveEntry(&event->timer_que);
}

/**
 * @fn          void AlarmCreate(TN_ALARM *alarm, CBACK handler, void *exinf)
 * @param[out]  alarm
 * @param[in]   handler
 * @param[in]   exinf
 */
static
void AlarmCreate(TN_ALARM *alarm, CBACK handler, void *exinf)
{
  alarm->exinf    = exinf;
  alarm->handler  = handler;
  alarm->state    = TIMER_STOP;
  alarm->id       = ID_ALARM;
}

/**
 * @fn          void AlarmDelete(TN_ALARM *alarm)
 * @param[out]  alarm
 */
static
void AlarmDelete(TN_ALARM *alarm)
{
  if (alarm->state == TIMER_START) {
    TimerDelete(&alarm->timer);
    alarm->state = TIMER_STOP;
  }

  alarm->handler = NULL;
  alarm->id = ID_INVALID;
}

/**
 * @fn          void AlarmStart(TN_ALARM *alarm, osTime_t time)
 * @param[out]  alarm
 * @param[in]   time
 */
static
void AlarmStart(TN_ALARM *alarm, osTime_t timeout)
{
  if (alarm->state == TIMER_START)
    TimerDelete(&alarm->timer);

  TimerInsert(&alarm->timer, knlInfo.jiffies + timeout, (CBACK)AlarmHandler, alarm);
  alarm->state = TIMER_START;
}

/**
 * @fn          void AlarmStop(TN_ALARM *alarm)
 * @param[out]  alarm
 */
static
void AlarmStop(TN_ALARM *alarm)
{
  if (alarm->state == TIMER_START) {
    TimerDelete(&alarm->timer);
    alarm->state = TIMER_STOP;
  }
}

/**
 * @fn          void CyclicCreate(TN_CYCLIC *cyc, CBACK handler, const cyclic_param_t *param, void *exinf)
 * @param[out]  cyc
 * @param[in]   handler
 * @param[in]   param
 * @param[in]   exinf
 */
static
void CyclicCreate(TN_CYCLIC *cyc, CBACK handler, const cyclic_param_t *param, void *exinf)
{
  cyc->exinf    = exinf;
  cyc->attr     = param->cyc_attr;
  cyc->handler  = handler;
  cyc->time     = param->cyc_time;
  cyc->id       = ID_CYCLIC;

  osTime_t time = knlInfo.jiffies + param->cyc_phs;

  if (cyc->attr & CYCLIC_ATTR_START) {
    cyc->state = TIMER_START;
    TimerInsert(&cyc->timer, time, (CBACK)CyclicHandler, cyc);
  }
  else {
    cyc->state = TIMER_STOP;
    cyc->timer.time = time;
  }
}

/**
 * @fn          void CyclicDelete(TN_CYCLIC *cyc)
 * @param[out]  cyc
 */
static
void CyclicDelete(TN_CYCLIC *cyc)
{
  if (cyc->state == TIMER_START) {
    TimerDelete(&cyc->timer);
    cyc->state = TIMER_STOP;
  }

  cyc->handler = NULL;
  cyc->id = ID_INVALID;
}

/**
 * @fn          void CyclicStart(TN_CYCLIC *cyc)
 * @param[out]  cyc
 */
static
void CyclicStart(TN_CYCLIC *cyc)
{
  osTime_t jiffies = knlInfo.jiffies;

  if (cyc->attr & CYCLIC_ATTR_PHS) {
    if (cyc->state == TIMER_STOP) {
      osTime_t time = cyc->timer.time;

      if (time_before_eq(time, jiffies))
        time = CyclicNextTime(cyc);

      TimerInsert(&cyc->timer, time, (CBACK)CyclicHandler, cyc);
    }
  }
  else {
    if (cyc->state == TIMER_START)
      TimerDelete(&cyc->timer);

    TimerInsert(&cyc->timer, jiffies + cyc->time, (CBACK)CyclicHandler, cyc);
  }

  cyc->state = TIMER_START;
}

/**
 * @fn          void CyclicStop(TN_CYCLIC *cyc)
 * @param[out]  cyc
 */
static
void CyclicStop(TN_CYCLIC *cyc)
{
  if (cyc->state == TIMER_START) {
    TimerDelete(&cyc->timer);
    cyc->state = TIMER_STOP;
  }
}

/*******************************************************************************
 *  function implementations (scope: module-exported)
 ******************************************************************************/

/**
 * @fn          osError_t osAlarmCreate(TN_ALARM *alarm, CBACK handler, void *exinf)
 * @param[out]  alarm
 * @param[in]   handler
 * @param[in]   exinf
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_ISR          The function cannot be called from interrupt service routines
 */
osError_t osAlarmCreate(TN_ALARM *alarm, CBACK handler, void *exinf)
{
  if (alarm == NULL)
    return TERR_WRONG_PARAM;
  if (alarm->id == ID_ALARM || handler == NULL)
    return TERR_WRONG_PARAM;
  if (IsIrqMode() || IsIrqMasked())
    return TERR_ISR;

  svc_3((uint32_t)alarm, (uint32_t)handler, (uint32_t)exinf, (uint32_t)AlarmCreate);

  return TERR_NO_ERR;
}

/**
 * @fn          osError_t osAlarmDelete(TN_ALARM *alarm)
 * @param[out]  alarm
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_NOEXS        Object is not a task or non-existent
 *              TERR_ISR          The function cannot be called from interrupt service routines
 */
osError_t osAlarmDelete(TN_ALARM *alarm)
{
  if (alarm == NULL)
    return TERR_WRONG_PARAM;
  if (alarm->id != ID_ALARM)
    return TERR_NOEXS;
  if (IsIrqMode() || IsIrqMasked())
    return TERR_ISR;

  svc_1((uint32_t)alarm, (uint32_t)AlarmDelete);

  return TERR_NO_ERR;
}

/**
 * @fn          osError_t osAlarmStart(TN_ALARM *alarm, osTime_t time)
 * @param[out]  alarm
 * @param[in]   time
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_NOEXS        Object is not a task or non-existent
 *              TERR_ISR          The function cannot be called from interrupt service routines
 */
osError_t osAlarmStart(TN_ALARM *alarm, osTime_t timeout)
{
  if (alarm == NULL || timeout == 0)
    return TERR_WRONG_PARAM;
  if (alarm->id != ID_ALARM)
    return TERR_NOEXS;
  if (IsIrqMode() || IsIrqMasked())
    return TERR_ISR;

  svc_2((uint32_t)alarm, (uint32_t)timeout, (uint32_t)AlarmStart);

  return TERR_NO_ERR;
}

/**
 * @fn          osError_t osAlarmStop(TN_ALARM *alarm)
 * @param[out]  alarm
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_NOEXS        Object is not a task or non-existent
 *              TERR_ISR          The function cannot be called from interrupt service routines
 */
osError_t osAlarmStop(TN_ALARM *alarm)
{
  if (alarm == NULL)
    return TERR_WRONG_PARAM;
  if (alarm->id != ID_ALARM)
    return TERR_NOEXS;
  if (IsIrqMode() || IsIrqMasked())
    return TERR_ISR;

  svc_1((uint32_t)alarm, (uint32_t)AlarmStop);

  return TERR_NO_ERR;
}

/**
 * @fn          osError_t osCyclicCreate(TN_CYCLIC *cyc, CBACK handler, const cyclic_param_t *param, void *exinf)
 * @param[out]  cyc
 * @param[in]   handler
 * @param[in]   param
 * @param[in]   exinf
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_ISR          The function cannot be called from interrupt service routines
 */
osError_t osCyclicCreate(TN_CYCLIC *cyc, CBACK handler, const cyclic_param_t *param, void *exinf)
{
  if (cyc == NULL || handler == NULL || param->cyc_time == 0)
    return TERR_WRONG_PARAM;
  if (cyc->id == ID_CYCLIC)
    return TERR_WRONG_PARAM;
  if (IsIrqMode() || IsIrqMasked())
    return TERR_ISR;

  svc_4((uint32_t)cyc, (uint32_t)handler, (uint32_t)param, (uint32_t)exinf, (uint32_t)CyclicCreate);

  return TERR_NO_ERR;
}

/**
 * @fn          osError_t osCyclicDelete(TN_CYCLIC *cyc)
 * @param[out]  cyc
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_NOEXS        Object is not a task or non-existent
 *              TERR_ISR          The function cannot be called from interrupt service routines
 */
osError_t osCyclicDelete(TN_CYCLIC *cyc)
{
  if (cyc == NULL)
    return TERR_WRONG_PARAM;
  if (cyc->id != ID_CYCLIC)
    return TERR_NOEXS;
  if (IsIrqMode() || IsIrqMasked())
    return TERR_ISR;

  svc_1((uint32_t)cyc, (uint32_t)CyclicDelete);

  return TERR_NO_ERR;
}

/**
 * @fn          osError_t osCyclicStart(TN_CYCLIC *cyc)
 * @param[out]  cyc
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_NOEXS        Object is not a task or non-existent
 *              TERR_ISR          The function cannot be called from interrupt service routines
 */
osError_t osCyclicStart(TN_CYCLIC *cyc)
{
  if (cyc == NULL)
    return TERR_WRONG_PARAM;
  if (cyc->id != ID_CYCLIC)
    return TERR_NOEXS;
  if (IsIrqMode() || IsIrqMasked())
    return TERR_ISR;

  svc_1((uint32_t)cyc, (uint32_t)CyclicStart);

  return TERR_NO_ERR;
}

/**
 * @fn          osError_t osCyclicStop(TN_CYCLIC *cyc)
 * @param[out]  cyc
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_NOEXS        Object is not a task or non-existent
 *              TERR_ISR          The function cannot be called from interrupt service routines
 */
osError_t osCyclicStop(TN_CYCLIC *cyc)
{
  if (cyc == NULL)
    return TERR_WRONG_PARAM;
  if (cyc->id != ID_CYCLIC)
    return TERR_NOEXS;
  if (IsIrqMode() || IsIrqMasked())
    return TERR_ISR;

  svc_1((uint32_t)cyc, (uint32_t)CyclicStop);

  return TERR_NO_ERR;
}

/*------------------------------ End of file ---------------------------------*/

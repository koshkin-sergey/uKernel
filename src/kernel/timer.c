/*
 * Copyright (C) 2011-2019 Sergey Koshkin <koshkin.sergey@gmail.com>
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

#include "os_lib.h"

/*******************************************************************************
 *  external declarations
 ******************************************************************************/

/*******************************************************************************
 *  defines and macros (scope: module-local)
 ******************************************************************************/

/* Timer State definitions */
#define osTimerInactive      0x00U   ///< Timer Inactive
#define osTimerStopped       0x01U   ///< Timer Stopped
#define osTimerRunning       0x02U   ///< Timer Running

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
uint32_t CyclicNextTime(osCyclic_t *cyc)
{
  uint32_t time, ticks;
  uint32_t n;

  time = cyc->timer.time + cyc->time;
  ticks = osInfo.kernel.tick;

  if (time_before_eq(time, ticks)) {
    time = ticks - cyc->timer.time;
    n = time / cyc->time;
    n++;
    time = n * cyc->time;
    time = cyc->timer.time + time;
  }

  return time;
}

static
void AlarmHandler(osAlarm_t *alarm)
{
  if (alarm == NULL)
    return;

  alarm->state = TIMER_STOP;
  alarm->handler(alarm->exinf);
}

static
void CyclicHandler(osCyclic_t *cyc)
{
  TimerInsert(&cyc->timer, CyclicNextTime(cyc), (CBACK)CyclicHandler, cyc);
  cyc->handler(cyc->exinf);
}

void TimerInsert(timer_t *event, uint32_t time, CBACK callback, void *arg)
{
  queue_t *que;
  timer_t *timer;
  queue_t *timer_queue = &osInfo.timer_queue;

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
void TimerRemove(timer_t *event)
{
  QueueRemoveEntry(&event->timer_que);
}

/**
 * @fn          void AlarmCreate(osAlarm_t *alarm, CBACK handler, void *exinf)
 * @param[out]  alarm
 * @param[in]   handler
 * @param[in]   exinf
 */
static
void AlarmCreate(osAlarm_t *alarm, CBACK handler, void *exinf)
{
  alarm->exinf    = exinf;
  alarm->handler  = handler;
  alarm->state    = TIMER_STOP;
  alarm->id       = ID_TIMER;
}

/**
 * @fn          void AlarmDelete(osAlarm_t *alarm)
 * @param[out]  alarm
 */
static
void AlarmDelete(osAlarm_t *alarm)
{
  if (alarm->state == TIMER_START) {
    TimerRemove(&alarm->timer);
    alarm->state = TIMER_STOP;
  }

  alarm->handler = NULL;
  alarm->id = ID_INVALID;
}

/**
 * @fn          void AlarmStart(osAlarm_t *alarm, uint32_t time)
 * @param[out]  alarm
 * @param[in]   time
 */
static
void AlarmStart(osAlarm_t *alarm, uint32_t timeout)
{
  if (alarm->state == TIMER_START)
    TimerRemove(&alarm->timer);

  TimerInsert(&alarm->timer, osInfo.kernel.tick + timeout, (CBACK)AlarmHandler, alarm);
  alarm->state = TIMER_START;
}

/**
 * @fn          void AlarmStop(osAlarm_t *alarm)
 * @param[out]  alarm
 */
static
void AlarmStop(osAlarm_t *alarm)
{
  if (alarm->state == TIMER_START) {
    TimerRemove(&alarm->timer);
    alarm->state = TIMER_STOP;
  }
}

/**
 * @fn          void CyclicCreate(osCyclic_t *cyc, CBACK handler, const cyclic_param_t *param, void *exinf)
 * @param[out]  cyc
 * @param[in]   handler
 * @param[in]   param
 * @param[in]   exinf
 */
static
void CyclicCreate(osCyclic_t *cyc, CBACK handler, const cyclic_param_t *param, void *exinf)
{
  cyc->exinf    = exinf;
  cyc->attr     = param->cyc_attr;
  cyc->handler  = handler;
  cyc->time     = param->cyc_time;
  cyc->id       = ID_CYCLIC;

  uint32_t time = osInfo.kernel.tick + param->cyc_phs;

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
 * @fn          void CyclicDelete(osCyclic_t *cyc)
 * @param[out]  cyc
 */
static
void CyclicDelete(osCyclic_t *cyc)
{
  if (cyc->state == TIMER_START) {
    TimerRemove(&cyc->timer);
    cyc->state = TIMER_STOP;
  }

  cyc->handler = NULL;
  cyc->id = ID_INVALID;
}

/**
 * @fn          void CyclicStart(osCyclic_t *cyc)
 * @param[out]  cyc
 */
static
void CyclicStart(osCyclic_t *cyc)
{
  uint32_t ticks = osInfo.kernel.tick;

  if (cyc->attr & CYCLIC_ATTR_PHS) {
    if (cyc->state == TIMER_STOP) {
      uint32_t time = cyc->timer.time;

      if (time_before_eq(time, ticks))
        time = CyclicNextTime(cyc);

      TimerInsert(&cyc->timer, time, (CBACK)CyclicHandler, cyc);
    }
  }
  else {
    if (cyc->state == TIMER_START)
      TimerRemove(&cyc->timer);

    TimerInsert(&cyc->timer, ticks + cyc->time, (CBACK)CyclicHandler, cyc);
  }

  cyc->state = TIMER_START;
}

/**
 * @fn          void CyclicStop(osCyclic_t *cyc)
 * @param[out]  cyc
 */
static
void CyclicStop(osCyclic_t *cyc)
{
  if (cyc->state == TIMER_START) {
    TimerRemove(&cyc->timer);
    cyc->state = TIMER_STOP;
  }
}

/*******************************************************************************
 *  Service Calls
 ******************************************************************************/

static osTimerId_t TimerNew(osTimerFunc_t func, osTimerType_t type, void *argument, const osTimerAttr_t *attr)
{

}

static const char *TimerGetName(osTimerId_t timer_id)
{
  osTimer_t *timer = timer_id;

  /* Check parameters */
  if ((timer == NULL) || (timer->id != ID_TIMER)) {
    return (NULL);
  }

  return (timer->name);
}

static osStatus_t TimerStart(osTimerId_t timer_id, uint32_t ticks)
{

}

static osStatus_t TimerStop(osTimerId_t timer_id)
{

}

static uint32_t TimerIsRunning(osTimerId_t timer_id)
{
  osTimer_t *timer = timer_id;
  uint32_t   is_running;

  /* Check parameters */
  if ((timer == NULL) || (timer->id != ID_TIMER)) {
    return (0U);
  }

  if (timer->state == osTimerRunning) {
    is_running = 1U;
  }
  else {
    is_running = 0;
  }

  return (is_running);
}

static osStatus_t TimerDelete(osTimerId_t timer_id)
{

}

/*******************************************************************************
 *  Public API
 ******************************************************************************/

/**
 * @fn          osTimerId_t osTimerNew(osTimerFunc_t func, osTimerType_t type, void *argument, const osTimerAttr_t *attr)
 * @brief       Create and Initialize a timer.
 * @param[in]   func      function pointer to callback function.
 * @param[in]   type      \ref osTimerOnce for one-shot or \ref osTimerPeriodic for periodic behavior.
 * @param[in]   argument  argument to the timer callback function.
 * @param[in]   attr      timer attributes; NULL: default values.
 * @return      timer ID for reference by other functions or NULL in case of error.
 */
osTimerId_t osTimerNew(osTimerFunc_t func, osTimerType_t type, void *argument, const osTimerAttr_t *attr)
{
  osTimerId_t timer_id;

  if (IsIrqMode() || IsIrqMasked()) {
    timer_id = NULL;
  }
  else {
    timer_id = (osTimerId_t)svc_4((uint32_t)func, (uint32_t)type, (uint32_t)argument, (uint32_t)attr, (uint32_t)TimerNew);
  }

  return (timer_id);
}

/**
 * @fn          const char *osTimerGetName(osTimerId_t timer_id)
 * @brief       Get name of a timer.
 * @param[in]   timer_id  timer ID obtained by \ref osTimerNew.
 * @return      name as null-terminated string or NULL in case of an error.
 */
const char *osTimerGetName(osTimerId_t timer_id)
{
  const char *name;

  if (IsIrqMode() || IsIrqMasked()) {
    name = NULL;
  }
  else {
    name = svc_1((uint32_t)timer_id, (uint32_t)TimerGetName);
  }

  return (name);
}

/**
 * @fn          osStatus_t osTimerStart(osTimerId_t timer_id, uint32_t ticks)
 * @brief       Start or restart a timer.
 * @param[in]   timer_id  timer ID obtained by \ref osTimerNew.
 * @param[in]   ticks     \ref CMSIS_RTOS_TimeOutValue "time ticks" value of the timer.
 * @return      status code that indicates the execution status of the function.
 */
osStatus_t osTimerStart(osTimerId_t timer_id, uint32_t ticks)
{
  osStatus_t status;

  if (IsIrqMode() || IsIrqMasked()) {
    status = osErrorISR;
  }
  else {
    status = (osStatus_t)svc_2((uint32_t)timer_id, ticks, (uint32_t)TimerStart);
  }

  return (status);
}

/**
 * @fn          osStatus_t osTimerStop(osTimerId_t timer_id)
 * @brief       Stop a timer.
 * @param[in]   timer_id  timer ID obtained by \ref osTimerNew.
 * @return      status code that indicates the execution status of the function.
 */
osStatus_t osTimerStop(osTimerId_t timer_id)
{
  osStatus_t status;

  if (IsIrqMode() || IsIrqMasked()) {
    status = osErrorISR;
  }
  else {
    status = (osStatus_t)svc_1((uint32_t)timer_id, (uint32_t)TimerStop);
  }

  return (status);
}

/**
 * @fn          uint32_t osTimerIsRunning(osTimerId_t timer_id)
 * @brief       Check if a timer is running.
 * @param[in]   timer_id  timer ID obtained by \ref osTimerNew.
 * @return      0 not running or an error occurred, 1 running.
 */
uint32_t osTimerIsRunning(osTimerId_t timer_id)
{
  uint32_t is_running;

  if (IsIrqMode() || IsIrqMasked()) {
    is_running = 0U;
  }
  else {
    is_running = svc_1((uint32_t)timer_id, (uint32_t)TimerIsRunning);
  }

  return (is_running);
}

/**
 * @fn          osStatus_t osTimerDelete(osTimerId_t timer_id)
 * @brief       Delete a timer.
 * @param[in]   timer_id  timer ID obtained by \ref osTimerNew.
 * @return      status code that indicates the execution status of the function.
 */
osStatus_t osTimerDelete(osTimerId_t timer_id)
{
  osStatus_t status;

  if (IsIrqMode() || IsIrqMasked()) {
    status = osErrorISR;
  }
  else {
    status = (osStatus_t)svc_1((uint32_t)timer_id, (uint32_t)TimerRemove);
  }

  return (status);
}

/*------------------------------ End of file ---------------------------------*/

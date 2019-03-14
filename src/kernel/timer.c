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

static uint32_t TimerNextTime(osTimer_t *timer)
{
  uint32_t time, ticks;
  uint32_t n;

  time = timer->event.time + timer->load;
  ticks = osInfo.kernel.tick;

  if (time_before_eq(time, ticks)) {
    time = ticks - timer->event.time;
    n = time / timer->load;
    n++;
    time = n * timer->load;
    time = timer->event.time + time;
  }

  return (time);
}

static void TimerHandler(void *argument)
{
  osTimer_t      *timer = (osTimer_t *)argument;
  osTimerFinfo_t *finfo = &timer->finfo;

  if (timer->type == osTimerOnce) {
    timer->state = osTimerStopped;
  }
  else {
    svc_4((uint32_t)&timer->event, (uint32_t)TimerNextTime(timer), (uint32_t)TimerHandler, (uint32_t)timer, (uint32_t)libTimerInsert);
  }

  finfo->func(finfo->arg);
}

/*******************************************************************************
 *  Service Calls
 ******************************************************************************/

static osTimerId_t TimerNew(osTimerFunc_t func, osTimerType_t type, void *argument, const osTimerAttr_t *attr)
{
  osTimer_t *timer;

  /* Check parameters */
  if ((func == NULL) || (attr == NULL) || ((type != osTimerOnce) && (type != osTimerPeriodic))) {
    return NULL;
  }

  timer = attr->cb_mem;

  /* Check parameters */
  if ((timer == NULL) || (((uint32_t)timer & 3U) != 0U) || (attr->cb_size < sizeof(osTimer_t))) {
    return (NULL);
  }

  /* Initialize control block */
  timer->id         = ID_TIMER;
  timer->state      = osTimerStopped;
  timer->flags      = 0U;
  timer->type       = (uint8_t)type;
  timer->name       = attr->name;
  timer->load       = 0U;
  timer->finfo.func = func;
  timer->finfo.arg  = argument;

  return (timer);
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
  osTimer_t *timer = timer_id;

  /* Check parameters */
  if ((timer == NULL) || (timer->id != ID_TIMER) || (ticks == 0U)) {
    return (osErrorParameter);
  }

  if (timer->state == osTimerRunning) {
    libTimerRemove(&timer->event);
  }
  else {
    timer->state = osTimerRunning;
    timer->load  = ticks;
  }

  libTimerInsert(&timer->event, osInfo.kernel.tick + ticks, TimerHandler, timer);

  return (osOK);
}

static osStatus_t TimerStop(osTimerId_t timer_id)
{
  osTimer_t *timer = timer_id;

  /* Check parameters */
  if ((timer == NULL) || (timer->id != ID_TIMER)) {
    return (osErrorParameter);
  }

  /* Check object state */
  if (timer->state != osTimerRunning) {
    return (osErrorResource);
  }

  timer->state = osTimerStopped;

  libTimerRemove(&timer->event);

  return (osOK);
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
    is_running = 0U;
  }

  return (is_running);
}

static osStatus_t TimerDelete(osTimerId_t timer_id)
{
  osTimer_t *timer = timer_id;

  /* Check parameters */
  if ((timer == NULL) || (timer->id != ID_TIMER)) {
    return (osErrorParameter);
  }

  /* Check object state */
  if (timer->state == osTimerRunning) {
    libTimerRemove(&timer->event);
  }

  /* Mark object as inactive and invalid */
  timer->state = osTimerInactive;
  timer->id    = ID_INVALID;

  return (osOK);
}

/*******************************************************************************
 *  Library functions
 ******************************************************************************/

void libTimerInsert(event_t *event, uint32_t time, osTimerFunc_t func, void *arg)
{
  queue_t *que;
  event_t *timer;
  queue_t *timer_queue;

  BEGIN_CRITICAL_SECTION

  timer_queue = &osInfo.timer_queue;
  event->finfo.func = func;
  event->finfo.arg  = arg;
  event->time = time;

  for (que = timer_queue->next; que != timer_queue; que = que->next) {
    timer = GetTimerByQueue(que);
    if (time_before(event->time, timer->time)) {
      break;
    }
  }

  QueueAddTail(que, &event->timer_que);

  END_CRITICAL_SECTION
}

/**
 * @fn          void TimerDelete(timer_t *event)
 * @brief
 */
void libTimerRemove(event_t *event)
{
  BEGIN_CRITICAL_SECTION

  QueueRemoveEntry(&event->timer_que);

  END_CRITICAL_SECTION
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
    name = (const char *)svc_1((uint32_t)timer_id, (uint32_t)TimerGetName);
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
    status = (osStatus_t)svc_1((uint32_t)timer_id, (uint32_t)TimerDelete);
  }

  return (status);
}

/*------------------------------ End of file ---------------------------------*/

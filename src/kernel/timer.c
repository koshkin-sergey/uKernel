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

#if defined(__CC_ARM)

__SVC_INDIRECT(0)
void __svcAlarmCreate(void (*)(TN_ALARM*, CBACK, void*), TN_ALARM*, CBACK, void*);

__STATIC_FORCEINLINE
void svcAlarmCreate(TN_ALARM *alarm, CBACK handler, void *exinf)
{
  __svcAlarmCreate(AlarmCreate, alarm, handler, exinf);
}

__SVC_INDIRECT(0)
void __svcAlarmDelete(void (*)(TN_ALARM*), TN_ALARM*);

__STATIC_FORCEINLINE
void svcAlarmDelete(TN_ALARM *alarm)
{
  __svcAlarmDelete(AlarmDelete, alarm);
}

__SVC_INDIRECT(0)
void __svcAlarmStart(void (*)(TN_ALARM*, osTime_t), TN_ALARM*, osTime_t);

__STATIC_FORCEINLINE
void svcAlarmStart(TN_ALARM *alarm, osTime_t timeout)
{
  __svcAlarmStart(AlarmStart, alarm, timeout);
}

__SVC_INDIRECT(0)
void __svcAlarmStop(void (*)(TN_ALARM*), TN_ALARM*);

__STATIC_FORCEINLINE
void svcAlarmStop(TN_ALARM *alarm)
{
  __svcAlarmStop(AlarmStop, alarm);
}

__SVC_INDIRECT(0)
void __svcCyclicCreate(void (*)(TN_CYCLIC*, CBACK, const cyclic_param_t*, void*), TN_CYCLIC*, CBACK, const cyclic_param_t*, void*);

__STATIC_FORCEINLINE
void svcCyclicCreate(TN_CYCLIC *cyc, CBACK handler, const cyclic_param_t *param, void *exinf)
{
  __svcCyclicCreate(CyclicCreate, cyc, handler, param, exinf);
}

__SVC_INDIRECT(0)
void __svcCyclicDelete(void (*)(TN_CYCLIC*), TN_CYCLIC*);

__STATIC_FORCEINLINE
void svcCyclicDelete(TN_CYCLIC *cyc)
{
  __svcCyclicDelete(CyclicDelete, cyc);
}

__SVC_INDIRECT(0)
void __svcCyclicStart(void (*)(TN_CYCLIC*), TN_CYCLIC*);

__STATIC_FORCEINLINE
void svcCyclicStart(TN_CYCLIC *cyc)
{
  __svcCyclicStart(CyclicStart, cyc);
}

__SVC_INDIRECT(0)
void __svcCyclicStop(void (*)(TN_CYCLIC*), TN_CYCLIC*);

__STATIC_FORCEINLINE
void svcCyclicStop(TN_CYCLIC *cyc)
{
  __svcCyclicStop(CyclicStop, cyc);
}

#elif defined(__ICCARM__)

__SVC_INDIRECT(0)
void __svcAlarmCreate(TN_ALARM*, CBACK, void*);

__STATIC_FORCEINLINE
void svcAlarmCreate(TN_ALARM *alarm, CBACK handler, void *exinf)
{
  SVC_ArgF(AlarmCreate);
  __svcAlarmCreate(alarm, handler, exinf);
}

__SVC_INDIRECT(0)
void __svcAlarmDelete(TN_ALARM*);

__STATIC_FORCEINLINE
void svcAlarmDelete(TN_ALARM *alarm)
{
  SVC_ArgF(AlarmDelete);
  __svcAlarmDelete(alarm);
}

__SVC_INDIRECT(0)
void __svcAlarmStart(TN_ALARM*, osTime_t);

__STATIC_FORCEINLINE
void svcAlarmStart(TN_ALARM *alarm, osTime_t timeout)
{
  SVC_ArgF(AlarmStart);
  __svcAlarmStart(alarm, timeout);
}

__SVC_INDIRECT(0)
void __svcAlarmStop(TN_ALARM*);

__STATIC_FORCEINLINE
void svcAlarmStop(TN_ALARM *alarm)
{
  SVC_ArgF(AlarmStop);
  __svcAlarmStop(alarm);
}

__SVC_INDIRECT(0)
void __svcCyclicCreate(TN_CYCLIC*, CBACK, const cyclic_param_t*, void*);

__STATIC_FORCEINLINE
void svcCyclicCreate(TN_CYCLIC *cyc, CBACK handler, const cyclic_param_t *param, void *exinf)
{
  SVC_ArgF(CyclicCreate);
  __svcCyclicCreate(cyc, handler, param, exinf);
}

__SVC_INDIRECT(0)
void __svcCyclicDelete(TN_CYCLIC*);

__STATIC_FORCEINLINE
void svcCyclicDelete(TN_CYCLIC *cyc)
{
  SVC_ArgF(CyclicDelete);
  __svcCyclicDelete(cyc);
}

__SVC_INDIRECT(0)
void __svcCyclicStart(TN_CYCLIC*);

__STATIC_FORCEINLINE
void svcCyclicStart(TN_CYCLIC *cyc)
{
  SVC_ArgF(CyclicStart);
  __svcCyclicStart(cyc);
}

__SVC_INDIRECT(0)
void __svcCyclicStop(TN_CYCLIC*);

__STATIC_FORCEINLINE
void svcCyclicStop(TN_CYCLIC *cyc)
{
  SVC_ArgF(CyclicStop);
  __svcCyclicStop(cyc);
}

#else   // !(defined(__CC_ARM) || defined(__ICCARM__))

__STATIC_FORCEINLINE
void svcAlarmCreate(TN_ALARM *alarm, CBACK handler, void *exinf)
{
  register uint32_t rf  __ASM(SVC_REG)  = (uint32_t)AlarmCreate;
  register uint32_t r0  __ASM("r0")     = (uint32_t)alarm;
  register uint32_t r1  __ASM("r1")     = (uint32_t)handler;
  register uint32_t r2  __ASM("r2")     = (uint32_t)exinf;

  __ASM volatile ("svc 0" :: "r"(rf),"r"(r0),"r"(r1),"r"(r2));
}

__STATIC_FORCEINLINE
void svcAlarmDelete(TN_ALARM *alarm)
{
  register uint32_t rf  __ASM(SVC_REG)  = (uint32_t)AlarmDelete;
  register uint32_t r0  __ASM("r0")     = (uint32_t)alarm;

  __ASM volatile ("svc 0" :: "r"(rf),"r"(r0) : "r1");
}

__STATIC_FORCEINLINE
void svcAlarmStart(TN_ALARM *alarm, osTime_t timeout)
{
  register uint32_t rf  __ASM(SVC_REG)  = (uint32_t)AlarmStart;
  register uint32_t r0  __ASM("r0")     = (uint32_t)alarm;
  register uint32_t r1  __ASM("r1")     = (uint32_t)timeout;

  __ASM volatile ("svc 0" :: "r"(rf),"r"(r0),"r"(r1));
}

__STATIC_FORCEINLINE
void svcAlarmStop(TN_ALARM *alarm)
{
  register uint32_t rf  __ASM(SVC_REG)  = (uint32_t)AlarmStop;
  register uint32_t r0  __ASM("r0")     = (uint32_t)alarm;

  __ASM volatile ("svc 0" :: "r"(rf),"r"(r0) : "r1");
}

__STATIC_FORCEINLINE
void svcCyclicCreate(TN_CYCLIC *cyc, CBACK handler, const cyclic_param_t *param, void *exinf)
{
  register uint32_t rf  __ASM(SVC_REG)  = (uint32_t)CyclicCreate;
  register uint32_t r0  __ASM("r0")     = (uint32_t)cyc;
  register uint32_t r1  __ASM("r1")     = (uint32_t)handler;
  register uint32_t r2  __ASM("r2")     = (uint32_t)param;
  register uint32_t r3  __ASM("r3")     = (uint32_t)exinf;

  __ASM volatile ("svc 0" :: "r"(rf),"r"(r0),"r"(r1),"r"(r2),"r"(r3));
}

__STATIC_FORCEINLINE
void svcCyclicDelete(TN_CYCLIC *cyc)
{
  register uint32_t rf  __ASM(SVC_REG)  = (uint32_t)CyclicDelete;
  register uint32_t r0  __ASM("r0")     = (uint32_t)cyc;

  __ASM volatile ("svc 0" :: "r"(rf),"r"(r0) : "r1");
}

__STATIC_FORCEINLINE
void svcCyclicStart(TN_CYCLIC *cyc)
{
  register uint32_t rf  __ASM(SVC_REG)  = (uint32_t)CyclicStart;
  register uint32_t r0  __ASM("r0")     = (uint32_t)cyc;

  __ASM volatile ("svc 0" :: "r"(rf),"r"(r0) : "r1");
}

__STATIC_FORCEINLINE
void svcCyclicStop(TN_CYCLIC *cyc)
{
  register uint32_t rf  __ASM(SVC_REG)  = (uint32_t)CyclicStop;
  register uint32_t r0  __ASM("r0")     = (uint32_t)cyc;

  __ASM volatile ("svc 0" :: "r"(rf),"r"(r0) : "r1");
}

#endif

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

  svcAlarmCreate(alarm, handler, exinf);

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

  svcAlarmDelete(alarm);

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

  svcAlarmStart(alarm, timeout);

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

  svcAlarmStop(alarm);

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

  svcCyclicCreate(cyc, handler, param, exinf);

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

  svcCyclicDelete(cyc);

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

  svcCyclicStart(cyc);

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

  svcCyclicStop(cyc);

  return TERR_NO_ERR;
}

/*------------------------------ End of file ---------------------------------*/

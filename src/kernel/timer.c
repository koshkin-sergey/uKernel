/*
 * Copyright (C) 2011-2017 Sergey Koshkin <koshkin.sergey@gmail.com>
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

extern void calibrate_delay(void);

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

static TN_TCB timer_task;
static CDLL_QUEUE timer_queue;

#if defined (__ICCARM__)    // IAR ARM
#pragma data_alignment=8
#endif

__WEAK stack_t timer_task_stack[TIMER_STACK_SIZE];

/*******************************************************************************
 *  function prototypes (scope: module-local)
 ******************************************************************************/

__svc_indirect(0)
void svcAlarmCreate(void (*)(TN_ALARM*, CBACK, void*), TN_ALARM*, CBACK, void*);
__svc_indirect(0)
void svcAlarm(void (*)(TN_ALARM*), TN_ALARM*);
__svc_indirect(0)
void svcAlarmStart(void (*)(TN_ALARM*, TIME_t), TN_ALARM*, TIME_t);
__svc_indirect(0)
void svcCyclic(void (*)(TN_CYCLIC*), TN_CYCLIC*);
__svc_indirect(0)
void svcCyclicCreate(void (*)(TN_CYCLIC*, CBACK, const cyclic_param_t*, void*), TN_CYCLIC*, CBACK, const cyclic_param_t*, void*);

/*-----------------------------------------------------------------------------*
 * Название : osSysTickInit
 * Описание : Функция заглушка для перекрытия в коде приложения если необходимо
 *            сконфигурировать системный таймер.
 * Параметры: hz  - частота в Гц. с которой системный таймер должен генерировать
 *            прерывание, в котором необходимо вызывать функцию osTimerHandle().
 * Результат: Нет.
 *----------------------------------------------------------------------------*/
__WEAK void osSysTickInit(uint32_t hz)
{
  ;
}

static
TMEB* GetTimer(void)
{
  TMEB *timer = NULL;

  BEGIN_CRITICAL_SECTION

  if (!isQueueEmpty(&timer_queue)) {
    timer = GetTimerByQueue(timer_queue.next);
    if (time_after(timer->time, knlInfo.jiffies))
      timer = NULL;
    else
      TimerDelete(timer);
  }

  END_CRITICAL_SECTION

  return timer;
}

__NO_RETURN static void TimerTaskFunc(void *par)
{
  TMEB *timer;

  if (((TN_OPTIONS *)par)->app_init)
    ((TN_OPTIONS *)par)->app_init();

  osSysTickInit(knlInfo.HZ);

  calibrate_delay();
  knlInfo.kernel_state = KERNEL_STATE_RUNNING;

  for (;;) {
    while ((timer = GetTimer()) != NULL) {
      (*timer->callback)(timer->arg);
    }

    osTaskSleep(TIME_WAIT_INFINITE);
  }
}

static
TIME_t CyclicNextTime(TN_CYCLIC *cyc)
{
  TIME_t time, jiffies;
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

/*-----------------------------------------------------------------------------*
  Название :  TimerTaskCreate
  Описание :  Функция создает задачу таймера с наивысшим приоритетом.
  Параметры:  Нет.
  Результат:  Нет.
*-----------------------------------------------------------------------------*/
void TimerTaskCreate(void *par)
{
  task_create_attr_t attr;

  QueueReset(&timer_queue);

  attr.func_addr = (void *)TimerTaskFunc;
  attr.func_param = par;
  attr.stk_size = sizeof(timer_task_stack)/sizeof(*timer_task_stack);
  attr.stk_start = (uint32_t *)&timer_task_stack[attr.stk_size-1];
  attr.priority = 0;
  attr.option = (TN_TASK_TIMER | TN_TASK_START_ON_CREATION);

  TaskCreate(&timer_task, &attr);
}

void TimerInsert(TMEB *event, TIME_t time, CBACK callback, void *arg)
{
  CDLL_QUEUE  *que;
  TMEB        *timer;

  event->callback = callback;
  event->arg  = arg;
  event->time = time;

  for (que = timer_queue.next; que != &timer_queue; que = que->next) {
    timer = GetTimerByQueue(que);
    if (time_before(event->time, timer->time))
      break;
  }

  QueueAddTail(que, &event->queue);
}

__FORCEINLINE void TimerDelete(TMEB *event)
{
  QueueRemoveEntry(&event->queue);
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
 * @fn          void AlarmStart(TN_ALARM *alarm, TIME_t time)
 * @param[out]  alarm
 * @param[in]   time
 */
static
void AlarmStart(TN_ALARM *alarm, TIME_t timeout)
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

  TIME_t time = knlInfo.jiffies + param->cyc_phs;

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
  TIME_t jiffies = knlInfo.jiffies;

  if (cyc->attr & CYCLIC_ATTR_PHS) {
    if (cyc->state == TIMER_STOP) {
      TIME_t time = cyc->timer.time;

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

void osTimerHandle(void)
{
  knlInfo.jiffies += knlInfo.os_period;
  if (knlInfo.kernel_state == KERNEL_STATE_RUNNING) {
    TaskGetCurrent()->time += knlInfo.os_period;

#if defined(ROUND_ROBIN_ENABLE)
    volatile CDLL_QUEUE *curr_que;   //-- Need volatile here only to solve
    volatile CDLL_QUEUE *pri_queue;  //-- IAR(c) compiler's high optimization mode problem
    volatile int        priority;
    TN_TCB *task = TaskGetCurrent();
    uint16_t *tslice_ticks = knlInfo.tslice_ticks;

    //-------  Round -robin (if is used)
    priority = task->priority;

    if (tslice_ticks[priority] != NO_TIME_SLICE) {
      task->tslice_count++;
      if (task->tslice_count > tslice_ticks[priority]) {
        task->tslice_count = 0;

        pri_queue = &(knlInfo.ready_list[priority]);
        //-- If ready queue is not empty and qty  of queue's tasks > 1
        if (!(isQueueEmpty((CDLL_QUEUE *)pri_queue)) &&
            pri_queue->next->next != pri_queue) {
          //-- Remove task from tail and add it to the head of
          //-- ready queue for current priority
          curr_que = queue_remove_tail(&(knlInfo.ready_list[priority]));
          queue_add_head(&(knlInfo.ready_list[priority]),(CDLL_QUEUE *)curr_que);
        }
      }
    }
#endif  // ROUND_ROBIN_ENABLE

    if (!isQueueEmpty(&timer_queue))
      TaskToRunnable(&timer_task);
  }
}

TIME_t osGetTickCount(void)
{
  return knlInfo.jiffies;
}

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
  if (IS_IRQ_MODE() || IS_IRQ_MASKED())
    return TERR_ISR;

  svcAlarmCreate(AlarmCreate, alarm, handler, exinf);

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
  if (IS_IRQ_MODE() || IS_IRQ_MASKED())
    return TERR_ISR;

  svcAlarm(AlarmDelete, alarm);

  return TERR_NO_ERR;
}

/**
 * @fn          osError_t osAlarmStart(TN_ALARM *alarm, TIME_t time)
 * @param[out]  alarm
 * @param[in]   time
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_NOEXS        Object is not a task or non-existent
 *              TERR_ISR          The function cannot be called from interrupt service routines
 */
osError_t osAlarmStart(TN_ALARM *alarm, TIME_t timeout)
{
  if (alarm == NULL || timeout == 0)
    return TERR_WRONG_PARAM;
  if (alarm->id != ID_ALARM)
    return TERR_NOEXS;
  if (IS_IRQ_MODE() || IS_IRQ_MASKED())
    return TERR_ISR;

  svcAlarmStart(AlarmStart, alarm, timeout);

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
  if (IS_IRQ_MODE() || IS_IRQ_MASKED())
    return TERR_ISR;

  svcAlarm(AlarmStop, alarm);

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
  if (IS_IRQ_MODE() || IS_IRQ_MASKED())
    return TERR_ISR;

  svcCyclicCreate(CyclicCreate, cyc, handler, param, exinf);

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
  if (IS_IRQ_MODE() || IS_IRQ_MASKED())
    return TERR_ISR;

  svcCyclic(CyclicDelete, cyc);

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
  if (IS_IRQ_MODE() || IS_IRQ_MASKED())
    return TERR_ISR;

  svcCyclic(CyclicStart, cyc);

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
  if (IS_IRQ_MODE() || IS_IRQ_MASKED())
    return TERR_ISR;

  svcCyclic(CyclicStop, cyc);

  return TERR_NO_ERR;
}

/*------------------------------ End of file ---------------------------------*/

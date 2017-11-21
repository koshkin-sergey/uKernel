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
#include "delay.h"
#include "utils.h"

/*******************************************************************************
 *  external declarations
 ******************************************************************************/

/*******************************************************************************
 *  defines and macros (scope: module-local)
 ******************************************************************************/

#define ALARM_STOP      0U
#define ALARM_START     1U

#define CYCLIC_STOP     0U
#define CYCLIC_START    1U

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

__WEAK tn_stack_t tn_timer_task_stack[TN_TIMER_STACK_SIZE];

/*******************************************************************************
 *  function prototypes (scope: module-local)
 ******************************************************************************/

static void cyclic_handler(TN_CYCLIC *cyc);

__svc_indirect(0)
void svcAlarmCreate(void (*)(TN_ALARM*, CBACK, void*), TN_ALARM*, CBACK, void*);
__svc_indirect(0)
void svcAlarm(void (*)(TN_ALARM*), TN_ALARM*);
__svc_indirect(0)
void svcAlarmStart(void (*)(TN_ALARM*, TIME_t), TN_ALARM*, TIME_t);
__svc_indirect(0)
void svcCyclic(void (*)(TN_CYCLIC*), TN_CYCLIC*);

/*******************************************************************************
 *  function implementations (scope: module-local)
 ******************************************************************************/

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

/*-----------------------------------------------------------------------------*
  Название :  timer_task_func
  Описание :  Функция таймерной задачи. В ней производится проверка очереди
              программных таймеров. Если время таймера истекло, то производит
              запуск соответствующего обработчика.
  Параметры:  par - Указатель на данные
  Результат:  Нет
*-----------------------------------------------------------------------------*/
static
TASK_FUNC timer_task_func(void *par)
{
  TMEB  *tm;

  if (((TN_OPTIONS *)par)->app_init)
    ((TN_OPTIONS *)par)->app_init();

  osSysTickInit(knlInfo.HZ);

  calibrate_delay();
  tn_system_state = TN_ST_STATE_RUNNING;

  for (;;) {
//    BEGIN_CRITICAL_SECTION

    /* Проводим проверку очереди программных таймеров */
    while (!is_queue_empty(&timer_queue)) {
      tm = get_timer_address(timer_queue.next);
      if (time_after(tm->time, knlInfo.jiffies))
        break;

      timer_delete(tm);

      if (tm->callback != NULL) {
        (*tm->callback)(tm->arg);
      }
    }

    osTaskSleep(TN_WAIT_INFINITE);

//    END_CRITICAL_SECTION
  }
}

/**
 * @brief
 * @param
 * @return
 */
__STATIC_FORCEINLINE void tick_int_processing(void)
{
#if defined(ROUND_ROBIN_ENABLE)

  volatile CDLL_QUEUE *curr_que;   //-- Need volatile here only to solve
  volatile CDLL_QUEUE *pri_queue;  //-- IAR(c) compiler's high optimization mode problem
  volatile int        priority;
  TN_TCB *task = ThreadGetCurrent();
  uint16_t *tslice_ticks = knlInfo.tslice_ticks;

  //-------  Round -robin (if is used)  
  priority = task->priority;
  
  if (tslice_ticks[priority] != NO_TIME_SLICE) {
    task->tslice_count++;
    if (task->tslice_count > tslice_ticks[priority]) {
      task->tslice_count = 0;
  
      pri_queue = &(knlInfo.ready_list[priority]);
      //-- If ready queue is not empty and qty  of queue's tasks > 1
      if (!(is_queue_empty((CDLL_QUEUE *)pri_queue)) &&
            pri_queue->next->next != pri_queue) {
        //-- Remove task from tail and add it to the head of
        //-- ready queue for current priority
        curr_que = queue_remove_tail(&(knlInfo.ready_list[priority]));
        queue_add_head(&(knlInfo.ready_list[priority]),(CDLL_QUEUE *)curr_que);
      }
    }
  }

#endif  // ROUND_ROBIN_ENABLE

  queue_remove_entry(&(timer_task.task_queue));

  timer_task.task_state = TSK_STATE_RUNNABLE;
  timer_task.pwait_queue = NULL;

  ThreadSetReady(&timer_task);

  knlThreadSetNext(&timer_task);
  SwitchContextRequest();
}

/*-----------------------------------------------------------------------------*
  Название :  do_timer_insert
  Описание :
  Параметры:
  Результат:
*-----------------------------------------------------------------------------*/
static void do_timer_insert(TMEB *event)
{
  CDLL_QUEUE  *que;
  TMEB        *tm;

  for (que = timer_queue.next; que != &timer_queue; que = que->next) {
    tm = get_timer_address(que);
    if (time_before(event->time, tm->time))
      break;
  }
  queue_add_tail(que, &event->queue);
}

/*-----------------------------------------------------------------------------*
  Название :  alarm_handler
  Описание :
  Параметры:
  Результат:
*-----------------------------------------------------------------------------*/
static void alarm_handler(TN_ALARM *alarm)
{
  if (alarm == NULL)
    return;

  alarm->stat = ALARM_STOP;

//  BEGIN_ENABLE_INTERRUPT
  alarm->handler(alarm->exinf);
//  END_ENABLE_INTERRUPT
}

/*-----------------------------------------------------------------------------*
  Название :  cyc_next_time
  Описание :
  Параметры:
  Результат:
*-----------------------------------------------------------------------------*/
static TIME_t cyc_next_time(TN_CYCLIC *cyc)
{
  TIME_t tm, jiffies;
  uint32_t n;

  tm = cyc->tmeb.time + cyc->time;
  jiffies = knlInfo.jiffies;

  if (time_before_eq(tm, jiffies)) {
    tm = jiffies - cyc->tmeb.time;
    n = tm / cyc->time;
    n++;
    tm = n * cyc->time;
    tm = cyc->tmeb.time + tm;
  }

  return tm;
}

/*-----------------------------------------------------------------------------*
  Название :  cyc_timer_insert
  Описание :
  Параметры:
  Результат:
*-----------------------------------------------------------------------------*/
static void cyc_timer_insert(TN_CYCLIC *cyc, TIME_t time)
{
  cyc->tmeb.callback  = (CBACK)cyclic_handler;
  cyc->tmeb.arg = cyc;
  cyc->tmeb.time  = time;

  do_timer_insert(&cyc->tmeb);
}

/*-----------------------------------------------------------------------------*
  Название :  cyclic_handler
  Описание :
  Параметры:
  Результат:
*-----------------------------------------------------------------------------*/
static void cyclic_handler(TN_CYCLIC *cyc)
{
  cyc_timer_insert(cyc, cyc_next_time(cyc));

  BEGIN_ENABLE_INTERRUPT
  cyc->handler(cyc->exinf);
  END_ENABLE_INTERRUPT
}

/*******************************************************************************
 *  function implementations (scope: module-exported)
 ******************************************************************************/

/*-----------------------------------------------------------------------------*
  Название :  TimerTaskCreate
  Описание :  Функция создает задачу таймера с наивысшим приоритетом.
  Параметры:  Нет.
  Результат:  Нет.
*-----------------------------------------------------------------------------*/
void TimerTaskCreate(void *par)
{
  task_create_attr_t attr;

  attr.func_addr = (void *)timer_task_func;
  attr.func_param = par;
  attr.stk_size = sizeof(tn_timer_task_stack)/sizeof(*tn_timer_task_stack);
  attr.stk_start = (uint32_t *)&tn_timer_task_stack[attr.stk_size-1];
  attr.priority = 0;
  attr.option = (TN_TASK_TIMER | TN_TASK_START_ON_CREATION);

  TaskCreate(&timer_task, &attr);

  queue_reset(&timer_queue);
}

/*-----------------------------------------------------------------------------*
  Название :  timer_insert
  Описание :  
  Параметры:  
  Результат:  
*-----------------------------------------------------------------------------*/
void timer_insert(TMEB *event, TIME_t time, CBACK callback, void *arg)
{
  event->callback = callback;
  event->arg  = arg;
  event->time = knlInfo.jiffies + time;

  do_timer_insert(event);
}

/*-----------------------------------------------------------------------------*
  Название :  timer_delete
  Описание :  
  Параметры:  
  Результат:  
*-----------------------------------------------------------------------------*/
void timer_delete(TMEB *event)
{
  queue_remove_entry(&event->queue);
}

void AlarmCreate(TN_ALARM *alarm, CBACK handler, void *exinf)
{
  alarm->exinf    = exinf;
  alarm->handler  = handler;
  alarm->stat     = ALARM_STOP;
  alarm->id       = TN_ID_ALARM;
}

void AlarmDelete(TN_ALARM *alarm)
{
  if (alarm->stat == ALARM_START)
    timer_delete(&alarm->tmeb);

  alarm->handler = NULL;
  alarm->stat = ALARM_STOP;
  alarm->id = 0;
}

void AlarmStart(TN_ALARM *alarm, TIME_t time)
{
  if (alarm->stat == ALARM_START)
    timer_delete(&alarm->tmeb);

  timer_insert(&alarm->tmeb, time, (CBACK)alarm_handler, alarm);
  alarm->stat = ALARM_START;
}

void AlarmStop(TN_ALARM *alarm)
{
  if (alarm->stat == ALARM_START) {
    timer_delete(&alarm->tmeb);
    alarm->stat = ALARM_STOP;
  }
}

void CyclicCreate(TN_CYCLIC *cyc, CBACK handler, void *exinf, uint32_t cyctime,
    uint32_t cycphs, uint32_t attr)
{
  TIME_t  tm;

  cyc->exinf    = exinf;
  cyc->attr     = attr;
  cyc->handler  = handler;
  cyc->time     = cyctime;
  cyc->id       = TN_ID_CYCLIC;

  tm = knlInfo.jiffies + cycphs;

  if (attr & TN_CYCLIC_ATTR_START) {
    cyc->stat = CYCLIC_START;
    cyc_timer_insert(cyc, tm);
  }
  else {
    cyc->stat = CYCLIC_STOP;
    cyc->tmeb.time = tm;
  }
}

void CyclicDelete(TN_CYCLIC *cyc)
{
  if (cyc->stat == CYCLIC_START)
    timer_delete(&cyc->tmeb);

  cyc->handler = NULL;
  cyc->stat = CYCLIC_STOP;
  cyc->id = 0;
}

void CyclicStart(TN_CYCLIC *cyc)
{
  TIME_t tm, jiffies;

  jiffies = knlInfo.jiffies;

  if (cyc->attr & TN_CYCLIC_ATTR_PHS) {
    if (cyc->stat == CYCLIC_STOP) {
      tm = cyc->tmeb.time;
      if (time_before_eq(tm, jiffies))
        tm = cyc_next_time(cyc);
      cyc_timer_insert(cyc, tm);
    }
  }
  else {
    if (cyc->stat == CYCLIC_START)
      timer_delete(&cyc->tmeb);

    tm = jiffies + cyc->time;
    cyc_timer_insert(cyc, tm);
  }

  cyc->stat = CYCLIC_START;
}

void CyclicStop(TN_CYCLIC *cyc)
{
  if (cyc->stat == CYCLIC_START)
    timer_delete(&cyc->tmeb);

  cyc->stat = CYCLIC_STOP;
}

void osTimerHandle(void)
{
  knlInfo.jiffies += knlInfo.os_period;
  if (tn_system_state == TN_ST_STATE_RUNNING) {
    ThreadGetCurrent()->time += knlInfo.os_period;
    tick_int_processing();
  }
}

TIME_t osGetTickCount(void)
{
  return knlInfo.jiffies;
}

osError_t osAlarmCreate(TN_ALARM *alarm, CBACK handler, void *exinf)
{
  if (alarm == NULL)
    return TERR_WRONG_PARAM;
  if (alarm->id == TN_ID_ALARM || handler == NULL)
    return TERR_WRONG_PARAM;

  svcAlarmCreate(AlarmCreate, alarm, handler, exinf);

  return TERR_NO_ERR;
}

osError_t osAlarmDelete(TN_ALARM *alarm)
{
  if (alarm == NULL)
    return TERR_WRONG_PARAM;
  if (alarm->id != TN_ID_ALARM)
    return TERR_NOEXS;

  svcAlarm(AlarmDelete, alarm);

  return TERR_NO_ERR;
}

osError_t osAlarmStart(TN_ALARM *alarm, TIME_t time)
{
  if (alarm == NULL || time == 0)
    return TERR_WRONG_PARAM;
  if (alarm->id != TN_ID_ALARM)
    return TERR_NOEXS;

  svcAlarmStart(AlarmStart, alarm, time);

  return TERR_NO_ERR;
}

osError_t osAlarmStop(TN_ALARM *alarm)
{
  if (alarm == NULL)
    return TERR_WRONG_PARAM;
  if (alarm->id != TN_ID_ALARM)
    return TERR_NOEXS;

  svcAlarm(AlarmStop, alarm);

  return TERR_NO_ERR;
}

osError_t osCyclicCreate(TN_CYCLIC *cyc, CBACK handler, void *exinf,
                           uint32_t cyctime, uint32_t cycphs, uint32_t attr)
{
  TIME_t  tm;

  if (cyc == NULL || handler == NULL || cyctime == 0)
    return TERR_WRONG_PARAM;
  if (cyc->id == TN_ID_CYCLIC)
    return TERR_WRONG_PARAM;

  BEGIN_CRITICAL_SECTION

  cyc->exinf    = exinf;
  cyc->attr     = attr;
  cyc->handler  = handler;
  cyc->time     = cyctime;
  cyc->id       = TN_ID_CYCLIC;

  tm = knlInfo.jiffies + cycphs;

  if (attr & TN_CYCLIC_ATTR_START) {
    cyc->stat = CYCLIC_START;
    cyc_timer_insert(cyc, tm);
  }
  else {
    cyc->stat = CYCLIC_STOP;
    cyc->tmeb.time = tm;
  }

  END_CRITICAL_SECTION
  return TERR_NO_ERR;
}

osError_t osCyclicDelete(TN_CYCLIC *cyc)
{
  if (cyc == NULL)
    return TERR_WRONG_PARAM;
  if (cyc->id != TN_ID_CYCLIC)
    return TERR_NOEXS;

  svcCyclic(CyclicDelete, cyc);

  return TERR_NO_ERR;
}

osError_t osCyclicStart(TN_CYCLIC *cyc)
{
  if (cyc == NULL)
    return TERR_WRONG_PARAM;
  if (cyc->id != TN_ID_CYCLIC)
    return TERR_NOEXS;

  svcCyclic(CyclicStart, cyc);

  return TERR_NO_ERR;
}

osError_t osCyclicStop(TN_CYCLIC *cyc)
{
  if (cyc == NULL)
    return TERR_WRONG_PARAM;
  if (cyc->id != TN_ID_CYCLIC)
    return TERR_NOEXS;

  svcCyclic(CyclicStop, cyc);

  return TERR_NO_ERR;
}

/*------------------------------ End of file ---------------------------------*/

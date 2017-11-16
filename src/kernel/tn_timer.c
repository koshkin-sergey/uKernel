/*******************************************************************************
 *
 * TNKernel real-time kernel
 *
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
#include "delay.h"
#include "timer.h"
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

volatile TIME_t jiffies;
unsigned long os_period;

#if defined(ROUND_ROBIN_ENABLE)
unsigned short tslice_ticks[TN_NUM_PRIORITY];  // for round-robin only
#endif

/*******************************************************************************
 *  global variable definitions (scope: module-local)
 ******************************************************************************/

static TN_TCB timer_task;
static CDLL_QUEUE timer_queue;

#if defined (__ICCARM__)    // IAR ARM
#pragma data_alignment=8
#endif

tn_stack_t tn_timer_task_stack[TN_TIMER_STACK_SIZE] __attribute__((weak));


/*******************************************************************************
 *  function prototypes (scope: module-local)
 ******************************************************************************/

static void cyclic_handler(TN_CYCLIC *cyc);

/*******************************************************************************
 *  function implementations (scope: module-local)
 ******************************************************************************/

/*-----------------------------------------------------------------------------*
 * Название : tn_systick_init
 * Описание : Функция заглушка для перекрытия в коде приложения если необходимо
 *            сконфигурировать системный таймер.
 * Параметры: hz  - частота в Гц. с которой системный таймер должен генерировать
 *            прерывание, в котором необходимо вызывать функцию tn_timer().
 * Результат: Нет.
 *----------------------------------------------------------------------------*/
__attribute__((weak))
void tn_systick_init(unsigned int hz)
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

  tn_systick_init(HZ);

  calibrate_delay();
  tn_system_state = TN_ST_STATE_RUNNING;

  for (;;) {
    BEGIN_CRITICAL_SECTION

    /* Проводим проверку очереди программных таймеров */
    while (!is_queue_empty(&timer_queue)) {
      tm = get_timer_address(timer_queue.next);
      if (tn_time_after(tm->time, jiffies))
        break;

      timer_delete(tm);

      if (tm->callback != NULL) {
        (*tm->callback)(tm->arg);
      }
    }

    knlThreadToWaitAction(&timer_task, NULL, TSK_WAIT_REASON_SLEEP, TN_WAIT_INFINITE);

    END_CRITICAL_SECTION
  }
}

/**
 * @brief
 * @param
 * @return
 */
__attribute__((always_inline)) static
void tick_int_processing(void)
{
#if defined(ROUND_ROBIN_ENABLE)

  volatile CDLL_QUEUE *curr_que;   //-- Need volatile here only to solve
  volatile CDLL_QUEUE *pri_queue;  //-- IAR(c) compiler's high optimization mode problem
  volatile int        priority;
  TN_TCB *task = knlThreadGetCurrent();

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

  knlThreadSetReady(&timer_task);

  knlThreadSetNext(&timer_task);
  switch_context_request();
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
    if (tn_time_before(event->time, tm->time))
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

  BEGIN_ENABLE_INTERRUPT
  alarm->handler(alarm->exinf);
  END_ENABLE_INTERRUPT
}

/*-----------------------------------------------------------------------------*
  Название :  cyc_next_time
  Описание :
  Параметры:
  Результат:
*-----------------------------------------------------------------------------*/
static TIME_t cyc_next_time(TN_CYCLIC *cyc)
{
  TIME_t  tm;
  unsigned int  n;

  tm = cyc->tmeb.time + cyc->time;

  if (tn_time_before_eq(tm, jiffies)) {
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
  Название :  create_timer_task
  Описание :  Функция создает задачу таймера с наивысшим приоритетом.
  Параметры:  Нет.
  Результат:  Нет.
*-----------------------------------------------------------------------------*/
void create_timer_task(void *par)
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
  Название :  tn_timer
  Описание :  Эту функцию необходимо вызывать в обработчике прерывания от
              системного таймера.
  Параметры:  Нет
  Результат:  Нет
*-----------------------------------------------------------------------------*/
void tn_timer(void)
{
  BEGIN_CRITICAL_SECTION

  jiffies += os_period;
  if (tn_system_state == TN_ST_STATE_RUNNING) {
    knlThreadGetCurrent()->time += os_period;
    tick_int_processing();
  }

  END_CRITICAL_SECTION
}

/*-----------------------------------------------------------------------------*
  Название :  tn_get_tick_count
  Описание :  Возвращает системное время в тиках RTOS.
  Параметры:  Нет.
  Результат:  Возвращает системное время в тиках RTOS.
*-----------------------------------------------------------------------------*/
unsigned long tn_get_tick_count(void)
{
  return jiffies;
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
  event->time = jiffies + time;

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

/*-----------------------------------------------------------------------------*
  Название :  tn_alarm_create
  Описание :  
  Параметры:  
  Результат:  
*-----------------------------------------------------------------------------*/
int tn_alarm_create(TN_ALARM *alarm, CBACK handler, void *exinf)
{
  if (alarm == NULL)
    return TERR_WRONG_PARAM;
  if (alarm->id == TN_ID_ALARM || handler == NULL)
    return TERR_WRONG_PARAM;

  BEGIN_CRITICAL_SECTION

  alarm->exinf    = exinf;
  alarm->handler  = handler;
  alarm->stat     = ALARM_STOP;
  alarm->id       = TN_ID_ALARM;

  END_CRITICAL_SECTION
  return TERR_NO_ERR;
}

/*-----------------------------------------------------------------------------*
  Название :  tn_alarm_delete
  Описание :  
  Параметры:  
  Результат:  
*-----------------------------------------------------------------------------*/
int tn_alarm_delete(TN_ALARM *alarm)
{
  if (alarm == NULL)
    return TERR_WRONG_PARAM;
  if (alarm->id != TN_ID_ALARM)
    return TERR_NOEXS;

  BEGIN_CRITICAL_SECTION

  if (alarm->stat == ALARM_START)
    timer_delete(&alarm->tmeb);

  alarm->handler = NULL;
  alarm->stat = ALARM_STOP;
  alarm->id = 0;

  END_CRITICAL_SECTION
  return TERR_NO_ERR;
}

/*-----------------------------------------------------------------------------*
  Название :  tn_alarm_start
  Описание :  
  Параметры:  
  Результат:  
*-----------------------------------------------------------------------------*/
int tn_alarm_start(TN_ALARM *alarm, TIME_t time)
{
  if (alarm == NULL || time == 0)
    return TERR_WRONG_PARAM;
  if (alarm->id != TN_ID_ALARM)
    return TERR_NOEXS;

  BEGIN_CRITICAL_SECTION

  if (alarm->stat == ALARM_START)
    timer_delete(&alarm->tmeb);

  timer_insert(&alarm->tmeb, time, (CBACK)alarm_handler, alarm);
  alarm->stat = ALARM_START;

  END_CRITICAL_SECTION
  return TERR_NO_ERR;
}

/*-----------------------------------------------------------------------------*
  Название :  tn_alarm_stop
  Описание :  
  Параметры:  
  Результат:  
*-----------------------------------------------------------------------------*/
int tn_alarm_stop(TN_ALARM *alarm)
{
  if (alarm == NULL)
    return TERR_WRONG_PARAM;
  if (alarm->id != TN_ID_ALARM)
    return TERR_NOEXS;

  BEGIN_CRITICAL_SECTION

  if (alarm->stat == ALARM_START) {
    timer_delete(&alarm->tmeb);
    alarm->stat = ALARM_STOP;
  }

  END_CRITICAL_SECTION
  return TERR_NO_ERR;
}

/*-----------------------------------------------------------------------------*
  Название :  tn_cyclic_create
  Описание :  
  Параметры:  
  Результат:  
*-----------------------------------------------------------------------------*/
int tn_cyclic_create(TN_CYCLIC *cyc, CBACK handler, void *exinf,
                     unsigned long cyctime, unsigned long cycphs,
                     unsigned int attr)
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

  tm = jiffies + cycphs;

  if (attr & TN_CYCLIC_ATTR_START)  {
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

/*-----------------------------------------------------------------------------*
  Название :  tn_cyclic_delete
  Описание :  
  Параметры:  
  Результат:  
*-----------------------------------------------------------------------------*/
int tn_cyclic_delete(TN_CYCLIC *cyc)
{
  if (cyc == NULL)
    return TERR_WRONG_PARAM;
  if (cyc->id != TN_ID_CYCLIC)
    return TERR_NOEXS;

  BEGIN_CRITICAL_SECTION

  if (cyc->stat == CYCLIC_START)
    timer_delete(&cyc->tmeb);

  cyc->handler = NULL;
  cyc->stat = CYCLIC_STOP;
  cyc->id = 0;

  END_CRITICAL_SECTION
  return TERR_NO_ERR;
}

/*-----------------------------------------------------------------------------*
  Название :  tn_cyclic_start
  Описание :  
  Параметры:  
  Результат:  
*-----------------------------------------------------------------------------*/
int tn_cyclic_start(TN_CYCLIC *cyc)
{
  TIME_t  tm;

  if (cyc == NULL)
    return TERR_WRONG_PARAM;
  if (cyc->id != TN_ID_CYCLIC)
    return TERR_NOEXS;

  BEGIN_CRITICAL_SECTION

  if (cyc->attr & TN_CYCLIC_ATTR_PHS) {
    if (cyc->stat == CYCLIC_STOP) {
      tm = cyc->tmeb.time;
      if (tn_time_before_eq(tm, jiffies))
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

  END_CRITICAL_SECTION
  return TERR_NO_ERR;
}

/*-----------------------------------------------------------------------------*
  Название :  tn_cyclic_stop
  Описание :  
  Параметры:  
  Результат:  
*-----------------------------------------------------------------------------*/
int tn_cyclic_stop(TN_CYCLIC *cyc)
{
  if (cyc == NULL)
    return TERR_WRONG_PARAM;
  if (cyc->id != TN_ID_CYCLIC)
    return TERR_NOEXS;

  BEGIN_CRITICAL_SECTION

  if (cyc->stat == CYCLIC_START)
    timer_delete(&cyc->tmeb);

  cyc->stat = CYCLIC_STOP;

  END_CRITICAL_SECTION
  return TERR_NO_ERR;
}

/*------------------------------ End of file ---------------------------------*/

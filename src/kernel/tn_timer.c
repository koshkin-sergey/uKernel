/*

  TNKernel real-time kernel

  Copyright � 2013, 2015 Sergey Koshkin <koshkin.sergey@gmail.com>
  All rights reserved.

  All rights reserved.

  Permission to use, copy, modify, and distribute this software in source
  and binary forms and its documentation for any purpose and without fee
  is hereby granted, provided that the above copyright notice appear
  in all copies and that both that copyright notice and this permission
  notice appear in supporting documentation.

  THIS SOFTWARE IS PROVIDED BY THE SERGEY KOSHKIN AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL SERGEY KOSHKIN OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
  SUCH DAMAGE.

*/

#include "tn_delay.h"
#include "tn_tasks.h"
#include "tn_timer.h"
#include "tn_utils.h"

volatile TIME jiffies;
unsigned long os_period;
unsigned short tslice_ticks[TN_NUM_PRIORITY];  // for round-robin only

/* - System tasks ------------------------------------------------------------*/

/* - timer task - priority 0 - highest */

#if defined (__ICCARM__)    // IAR ARM
#pragma data_alignment=8
#endif

tn_stack_t tn_timer_task_stack[TN_TIMER_STACK_SIZE] __attribute__((weak, section("STACK"), zero_init));

TN_TCB timer_task;
static CDLL_QUEUE timer_queue;

static void timer_task_func(void *par);
static void alarm_handler(TN_ALARM *alarm);
static void cyclic_handler(TN_CYCLIC *cyc);
static unsigned long cyc_next_time(TN_CYCLIC *cyc);

/*-----------------------------------------------------------------------------*
  �������� :  create_timer_task
  �������� :  ������� ������� ������ ������� � ��������� �����������.
  ���������:  ���.
  ���������:  ���.
*-----------------------------------------------------------------------------*/
void create_timer_task(void *par)
{
  unsigned int stack_size = sizeof(tn_timer_task_stack)/sizeof(*tn_timer_task_stack);

  tn_task_create(
    &timer_task,                              // task TCB
    timer_task_func,                          // task function
    0,                                        // task priority
    &(tn_timer_task_stack[stack_size-1]),     // task stack first addr in memory
    stack_size,                               // task stack size (in int,not bytes)
    par,                                      // task function parameter
    TN_TASK_TIMER | TN_TASK_START_ON_CREATION // Creation option
  );
  queue_reset(&timer_queue);
}

/*-----------------------------------------------------------------------------*
 * �������� : tn_systick_init
 * �������� : ������� �������� ��� ���������� � ���� ���������� ���� ����������
 *            ���������������� ��������� ������.
 * ���������: hz  - ������� � ��. � ������� ��������� ������ ������ ������������
 *            ����������, � ������� ���������� �������� ������� tn_timer().
 * ���������: ���.
 *----------------------------------------------------------------------------*/
__attribute__((weak)) void tn_systick_init(unsigned int hz)
{
  ;
}

/*-----------------------------------------------------------------------------*
  �������� :  timer_task_func
  �������� :  ������� ��������� ������. � ��� ������������ �������� �������
              ����������� ��������. ���� ����� ������� �������, �� ����������
              ������ ���������������� �����������.
  ���������:  par - ��������� �� ������
  ���������:  ���
*-----------------------------------------------------------------------------*/
static TASK_FUNC timer_task_func(void *par)
{
  TMEB  *tm;

  tn_disable_irq();

  if (((TN_OPTIONS *)par)->app_init)
    ((TN_OPTIONS *)par)->app_init();

  tn_systick_init(HZ);

  tn_enable_irq();

  calibrate_delay();
  tn_system_state = TN_ST_STATE_RUNNING;

  for (;;) {
    BEGIN_CRITICAL_SECTION
    
    /* �������� �������� ������� ����������� �������� */
    while (!is_queue_empty(&timer_queue)) {
      tm = get_timer_address(timer_queue.next);
      if (tn_time_after_eq(tm->time, jiffies))
        break;

      timer_delete(tm);
      if (tm->callback != NULL) {
        (*tm->callback)(tm->arg);
      }
    }

    task_curr_to_wait_action(NULL, TSK_WAIT_REASON_SLEEP, TN_WAIT_INFINITE);
    
    END_CRITICAL_SECTION
  }
}

/*-----------------------------------------------------------------------------*
  �������� :  tick_int_processing
  �������� :  
  ���������:  
  ���������:  
*-----------------------------------------------------------------------------*/
static void tick_int_processing()
{
  volatile CDLL_QUEUE *curr_que;   //-- Need volatile here only to solve
  volatile CDLL_QUEUE *pri_queue;  //-- IAR(c) compiler's high optimization mode problem
  volatile int        priority;
  
  //-------  Round -robin (if is used)  
  priority = tn_curr_run_task->priority;
  
  if (tslice_ticks[priority] != NO_TIME_SLICE) {
    tn_curr_run_task->tslice_count++;
    if (tn_curr_run_task->tslice_count > tslice_ticks[priority]) {
      tn_curr_run_task->tslice_count = 0;
  
      pri_queue = &(tn_ready_list[priority]);
      //-- If ready queue is not empty and qty  of queue's tasks > 1
      if (!(is_queue_empty((CDLL_QUEUE *)pri_queue)) &&
            pri_queue->next->next != pri_queue) {
        //-- Remove task from tail and add it to the head of
        //-- ready queue for current priority
        curr_que = queue_remove_tail(&(tn_ready_list[priority]));
        queue_add_head(&(tn_ready_list[priority]),(CDLL_QUEUE *)curr_que);
      }
    }
  }

  queue_remove_entry(&(timer_task.task_queue));

  timer_task.task_state       = TSK_STATE_RUNNABLE;
  timer_task.pwait_queue      = NULL;

  queue_add_tail(&(tn_ready_list[0]), &(timer_task.task_queue));
  tn_ready_to_run_bmp |= 1;  // priority 0;

  tn_next_task_to_run = &timer_task;
  tn_switch_context_request();
}

/*-----------------------------------------------------------------------------*
  �������� :  tn_timer
  �������� :  ��� ������� ���������� �������� � ����������� ���������� ��
              ���������� �������.
  ���������:  ���
  ���������:  ���
*-----------------------------------------------------------------------------*/
void tn_timer(void)
{
	BEGIN_CRITICAL_SECTION

  jiffies += os_period;
  if (tn_system_state == TN_ST_STATE_RUNNING) {
    tn_curr_run_task->time += os_period;
    tick_int_processing();
  }

  END_CRITICAL_SECTION
}

/*-----------------------------------------------------------------------------*
  �������� :  tn_get_tick_count
  �������� :  ���������� ��������� ����� � ����� RTOS.
  ���������:  ���.
  ���������:  ���������� ��������� ����� � ����� RTOS.
*-----------------------------------------------------------------------------*/
unsigned long tn_get_tick_count(void)
{
  return jiffies;
}

/*-----------------------------------------------------------------------------*
  �������� :  do_timer_insert
  �������� :  
  ���������:  
  ���������:  
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
  �������� :  timer_insert
  �������� :  
  ���������:  
  ���������:  
*-----------------------------------------------------------------------------*/
void timer_insert(TMEB *event, TIME time, CBACK callback, void *arg)
{
  event->callback = callback;
  event->arg  = arg;
  event->time = jiffies + time;

  do_timer_insert(event);
}

/*-----------------------------------------------------------------------------*
  �������� :  timer_delete
  �������� :  
  ���������:  
  ���������:  
*-----------------------------------------------------------------------------*/
void timer_delete(TMEB *event)
{
  queue_remove_entry(&event->queue);
}

/*-----------------------------------------------------------------------------*
  �������� :  alarm_handler
  �������� :  
  ���������:  
  ���������:  
*-----------------------------------------------------------------------------*/
static void alarm_handler(TN_ALARM *alarm)
{
  if (alarm == NULL)
    return;
  
  alarm->stat = ALARM_STOP;
  tn_enable_irq();
  alarm->handler(alarm->exinf);
  tn_disable_irq();
}

/*-----------------------------------------------------------------------------*
  �������� :  tn_alarm_create
  �������� :  
  ���������:  
  ���������:  
*-----------------------------------------------------------------------------*/
int tn_alarm_create(TN_ALARM *alarm, CBACK handler, void *exinf)
{
#if TN_CHECK_PARAM
  if (alarm == NULL)
    return TERR_WRONG_PARAM;
  if (alarm->id == TN_ID_ALARM || handler == NULL)
    return TERR_WRONG_PARAM;
#endif

  BEGIN_CRITICAL_SECTION

  alarm->exinf    = exinf;
  alarm->handler  = handler;
  alarm->stat     = ALARM_STOP;
  alarm->id       = TN_ID_ALARM;

  END_CRITICAL_SECTION
  return TERR_NO_ERR;
}

/*-----------------------------------------------------------------------------*
  �������� :  tn_alarm_delete
  �������� :  
  ���������:  
  ���������:  
*-----------------------------------------------------------------------------*/
int tn_alarm_delete(TN_ALARM *alarm)
{
#if TN_CHECK_PARAM
  if (alarm == NULL)
    return TERR_WRONG_PARAM;
  if (alarm->id != TN_ID_ALARM)
    return TERR_NOEXS;
#endif

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
  �������� :  tn_alarm_start
  �������� :  
  ���������:  
  ���������:  
*-----------------------------------------------------------------------------*/
int tn_alarm_start(TN_ALARM *alarm, TIME time)
{
#if TN_CHECK_PARAM
  if (alarm == NULL || time == 0)
    return TERR_WRONG_PARAM;
  if (alarm->id != TN_ID_ALARM)
    return TERR_NOEXS;
#endif

  BEGIN_CRITICAL_SECTION

  if (alarm->stat == ALARM_START)
    timer_delete(&alarm->tmeb);

  timer_insert(&alarm->tmeb, time, (CBACK)alarm_handler, alarm);
  alarm->stat = ALARM_START;

  END_CRITICAL_SECTION
  return TERR_NO_ERR;
}

/*-----------------------------------------------------------------------------*
  �������� :  tn_alarm_stop
  �������� :  
  ���������:  
  ���������:  
*-----------------------------------------------------------------------------*/
int tn_alarm_stop(TN_ALARM *alarm)
{
#if TN_CHECK_PARAM
  if (alarm == NULL)
    return TERR_WRONG_PARAM;
  if (alarm->id != TN_ID_ALARM)
    return TERR_NOEXS;
#endif

  BEGIN_CRITICAL_SECTION

  if (alarm->stat == ALARM_START) {
    timer_delete(&alarm->tmeb);
    alarm->stat = ALARM_STOP;
  }

  END_CRITICAL_SECTION
  return TERR_NO_ERR;
}

/*-----------------------------------------------------------------------------*
  �������� :  cyc_next_time
  �������� :  
  ���������:  
  ���������:  
*-----------------------------------------------------------------------------*/
static TIME cyc_next_time(TN_CYCLIC *cyc)
{
  TIME  tm;
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
  �������� :  cyc_timer_insert
  �������� :  
  ���������:  
  ���������:  
*-----------------------------------------------------------------------------*/
static void cyc_timer_insert(TN_CYCLIC *cyc, TIME time)
{
  cyc->tmeb.callback  = (CBACK)cyclic_handler;
  cyc->tmeb.arg = cyc;
  cyc->tmeb.time  = time;

  do_timer_insert(&cyc->tmeb);
}

/*-----------------------------------------------------------------------------*
  �������� :  cyclic_handler
  �������� :  
  ���������:  
  ���������:  
*-----------------------------------------------------------------------------*/
static void cyclic_handler(TN_CYCLIC *cyc)
{
  cyc_timer_insert(cyc, cyc_next_time(cyc));

  tn_enable_irq();
  cyc->handler(cyc->exinf);
  tn_disable_irq();
}

/*-----------------------------------------------------------------------------*
  �������� :  tn_cyclic_create
  �������� :  
  ���������:  
  ���������:  
*-----------------------------------------------------------------------------*/
int tn_cyclic_create(TN_CYCLIC *cyc, CBACK handler, void *exinf,
                     unsigned long cyctime, unsigned long cycphs,
                     unsigned int attr)
{
  TIME  tm;

#if TN_CHECK_PARAM
  if (cyc == NULL || handler == NULL || cyctime == 0)
    return TERR_WRONG_PARAM;
  if (cyc->id == TN_ID_CYCLIC)
    return TERR_WRONG_PARAM;
#endif

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
  �������� :  tn_cyclic_delete
  �������� :  
  ���������:  
  ���������:  
*-----------------------------------------------------------------------------*/
int tn_cyclic_delete(TN_CYCLIC *cyc)
{
#if TN_CHECK_PARAM
  if (cyc == NULL)
    return TERR_WRONG_PARAM;
  if (cyc->id != TN_ID_CYCLIC)
    return TERR_NOEXS;
#endif

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
  �������� :  tn_cyclic_start
  �������� :  
  ���������:  
  ���������:  
*-----------------------------------------------------------------------------*/
int tn_cyclic_start(TN_CYCLIC *cyc)
{
  TIME  tm;

#if TN_CHECK_PARAM
  if (cyc == NULL)
    return TERR_WRONG_PARAM;
  if (cyc->id != TN_ID_CYCLIC)
    return TERR_NOEXS;
#endif

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
  �������� :  tn_cyclic_stop
  �������� :  
  ���������:  
  ���������:  
*-----------------------------------------------------------------------------*/
int tn_cyclic_stop(TN_CYCLIC *cyc)
{
#if TN_CHECK_PARAM
  if (cyc == NULL)
    return TERR_WRONG_PARAM;
  if (cyc->id != TN_ID_CYCLIC)
    return TERR_NOEXS;
#endif

  BEGIN_CRITICAL_SECTION

  if (cyc->stat == CYCLIC_START)
    timer_delete(&cyc->tmeb);

  cyc->stat = CYCLIC_STOP;

  END_CRITICAL_SECTION
  return TERR_NO_ERR;
}
/*------------------------------ ����� ����� ---------------------------------*/

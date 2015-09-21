/*

  TNKernel real-time kernel

  Copyright © 2004, 2010 Yuri Tiomkin
  Copyright © 2013, 2015 Sergey Koshkin <koshkin.sergey@gmail.com>
  All rights reserved.

  Permission to use, copy, modify, and distribute this software in source
  and binary forms and its documentation for any purpose and without fee
  is hereby granted, provided that the above copyright notice appear
  in all copies and that both that copyright notice and this permission
  notice appear in supporting documentation.

  THIS SOFTWARE IS PROVIDED BY THE YURI TIOMKIN AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL YURI TIOMKIN OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
  SUCH DAMAGE.

*/

#include "tn.h"
#include "tn_tasks.h"
#include "tn_timer.h"
#include "tn_utils.h"

//  The System uses two levels of priorities for the own purpose:
//
//   - level 0                    (highest) for system timers task
//   - level (TN_NUM_PRIORITY-1)  (lowest)  for system idle task

CDLL_QUEUE tn_create_queue;            //-- all created tasks
unsigned long HZ;                      // Частота системного таймера.
volatile int tn_created_tasks_qty;     //-- num of created tasks
volatile int tn_system_state;          //-- System state -(running/not running/etc.)

/* - System tasks ------------------------------------------------------------*/

/* - idle task - priority (TN_NUM_PRIORITY-1) - lowest */

#if defined (__ICCARM__)    // IAR ARM
#pragma data_alignment=8
#endif

static TN_TCB  idle_task;
tn_stack_t tn_idle_task_stack[TN_IDLE_STACK_SIZE] __attribute__((weak, section("STACK"), zero_init));

/*-----------------------------------------------------------------------------*
 * Название : tn_idle_task_func
 * Описание :
 * Параметры:
 * Результат:
 *----------------------------------------------------------------------------*/
__attribute__((weak)) void tn_idle_task_func(void *par)
{
  for (;;) {
    ;
  }
}

/*-----------------------------------------------------------------------------*
 * Название : create_idle_task
 * Описание :
 * Параметры:
 * Результат:
 *----------------------------------------------------------------------------*/
static void create_idle_task(void)
{
  unsigned int stack_size = sizeof(tn_idle_task_stack)/sizeof(*tn_idle_task_stack);

  tn_task_create(
    &idle_task,                               // task TCB
    tn_idle_task_func,                        // task function
    TN_NUM_PRIORITY-1,                        // task priority
    &(tn_idle_task_stack[stack_size-1]),      // task stack first addr in memory
    stack_size,                               // task stack size (in int,not bytes)
    NULL,                                     // task function parameter
    TN_TASK_IDLE | TN_TASK_START_ON_CREATION  // Creation option
  );
}

//----------------------------------------------------------------------------
// TN main function (never return)
//----------------------------------------------------------------------------
void tn_start_system(TN_OPTIONS *opt)
{
  tn_system_state = TN_ST_STATE_NOT_RUN;

  //-- Clear/set all globals (vars, lists, etc)
  for (int i=0; i < TN_NUM_PRIORITY; i++) {
    queue_reset(&(tn_ready_list[i]));
    tslice_ticks[i] = NO_TIME_SLICE;
  }

  queue_reset(&tn_create_queue);
  HZ                    = opt->freq_timer;
  os_period             = 1000/HZ;

  //--- Idle task
  create_idle_task();
  //--- Timer task
  create_timer_task((void *)opt);

  tn_curr_run_task = &idle_task;

  //-- Run OS - first context switch
  tn_start_exe();
}

//--- Set time slice ticks value for priority for round-robin scheduling
//--- If value is NO_TIME_SLICE there are no round-robin scheduling
//--- for tasks with priority. NO_TIME_SLICE is default value.
//----------------------------------------------------------------------------
int tn_sys_tslice_ticks(int priority, int value)
{
#if TN_CHECK_PARAM
  if (priority <= 0 || priority >= TN_NUM_PRIORITY-1 ||
      value < 0 || value > MAX_TIME_SLICE)
    return TERR_WRONG_PARAM;
#endif

  BEGIN_DISABLE_INTERRUPT

  tslice_ticks[priority] = value;

  END_DISABLE_INTERRUPT
  return TERR_NO_ERR;
}

/*------------------------------ Конец файла ---------------------------------*/

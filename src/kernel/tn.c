/*******************************************************************************
 *
 * TNKernel real-time kernel
 *
 * Copyright © 2004, 2013 Yuri Tiomkin
 * Copyright © 2013-2016 Sergey Koshkin <koshkin.sergey@gmail.com>
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
 * The System uses two levels of priorities for the own purpose:
 *   - level 0                    (highest) for system timers task
 *   - level (TN_NUM_PRIORITY-1)  (lowest)  for system idle task
 */

/*******************************************************************************
 *  includes
 ******************************************************************************/

#include "tn.h"
#include "tn_tasks.h"
#include "tn_timer.h"
#include "tn_utils.h"

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

CDLL_QUEUE tn_create_queue;            /**< All created tasks */
uint32_t HZ;                           /**< Frequency system timer */
volatile int tn_created_tasks_qty;     /**< Number of created tasks */
volatile int tn_system_state;          /**< System state -(running/not running/etc.) */
uint32_t max_syscall_interrupt_priority;

/*******************************************************************************
 *  global variable definitions (scope: module-local)
 ******************************************************************************/

#if defined (__ICCARM__)    // IAR ARM
#pragma data_alignment=8
#endif

static TN_TCB idle_task;
tn_stack_t tn_idle_task_stack[TN_IDLE_STACK_SIZE] __attribute__((weak));

/*******************************************************************************
 *  function prototypes (scope: module-local)
 ******************************************************************************/

/*******************************************************************************
 *  function implementations (scope: module-local)
 ******************************************************************************/

/**
 * @brief Idle task function.
 * @param par
 */
__attribute__((weak)) void tn_idle_task_func(void *par)
{
  for (;;) {
    ;
  }
}

/**
 * @brief Create system idle task.
 *
 * Idle task priority (TN_NUM_PRIORITY-1) - lowest.
 */
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

/*******************************************************************************
 *  function implementations (scope: module-exported)
 ******************************************************************************/

/**
 * @brief Initial TNKernel system start function, never returns. Typically
 *        called from main().
 * @param opt - Pointer to struct TN_OPTIONS.
 */
void tn_start_system(TN_OPTIONS *opt)
{
  __disable_irq();

  tn_system_state = TN_ST_STATE_NOT_RUN;

  for (int i=0; i < TN_NUM_PRIORITY; i++) {
    queue_reset(&(tn_ready_list[i]));
#if defined(ROUND_ROBIN_ENABLE)
    tslice_ticks[i] = NO_TIME_SLICE;
#endif
  }

  queue_reset(&tn_create_queue);
  HZ = opt->freq_timer;
  os_period = 1000/HZ;
  max_syscall_interrupt_priority = opt->max_syscall_interrupt_priority;
  run_task.curr = &idle_task;
  run_task.next = &idle_task;

  //--- Idle task
  create_idle_task();
  //--- Timer task
  create_timer_task((void *)opt);

  //-- Run OS - first context switch
  start_kernel();
}

#if defined(ROUND_ROBIN_ENABLE)

/**
 * @brief   Set time slice ticks value for priority for round-robin scheduling.
 * @param   priority -  Priority of tasks for which time slice value should be
 *                      set. If value is NO_TIME_SLICE there are no round-robin
 *                      scheduling for tasks with priority. NO_TIME_SLICE is
 *                      default value.
 * @return  TERR_NO_ERR on success;
 *          TERR_WRONG_PARAM if given parameters are invalid.
 */
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

#endif  // ROUND_ROBIN_ENABLE

/*------------------------------ End of file ---------------------------------*/

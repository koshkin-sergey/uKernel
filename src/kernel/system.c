/*
 * Copyright (C) 2017 Sergey Koshkin <koshkin.sergey@gmail.com>
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
 * The System uses two levels of priorities for the own purpose:
 *   - level 0                    (highest) for system timers task
 *   - level (TN_NUM_PRIORITY-1)  (lowest)  for system idle task
 */

/*******************************************************************************
 *  includes
 ******************************************************************************/

#include "knl_lib.h"
#include "timer.h"
#include "utils.h"

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

knlInfo_t knlInfo;

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
static
void create_idle_task(void)
{
  task_create_attr_t attr;

  attr.func_addr = (void *)tn_idle_task_func;
  attr.func_param = NULL;
  attr.stk_size = sizeof(tn_idle_task_stack)/sizeof(*tn_idle_task_stack);
  attr.stk_start = (uint32_t *)&tn_idle_task_stack[attr.stk_size-1];
  attr.priority = NUM_PRIORITY-1;
  attr.option = (TN_TASK_IDLE | TN_TASK_START_ON_CREATION);

  TaskCreate(&idle_task, &attr);
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

  for (int i=0; i < NUM_PRIORITY; i++) {
    queue_reset(&knlInfo.ready_list[i]);
#if defined(ROUND_ROBIN_ENABLE)
    tslice_ticks[i] = NO_TIME_SLICE;
#endif
  }

  queue_reset(&tn_create_queue);
  HZ = opt->freq_timer;
  os_period = 1000/HZ;
  max_syscall_interrupt_priority = opt->max_syscall_interrupt_priority;

  knlThreadSetCurrent(&idle_task);
  knlThreadSetNext(&idle_task);

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
  if (priority <= 0 || priority >= TN_NUM_PRIORITY-1 ||
      value < 0 || value > MAX_TIME_SLICE)
    return TERR_WRONG_PARAM;

  BEGIN_DISABLE_INTERRUPT

  tslice_ticks[priority] = value;

  END_DISABLE_INTERRUPT
  return TERR_NO_ERR;
}

#endif  // ROUND_ROBIN_ENABLE

/*------------------------------ End of file ---------------------------------*/

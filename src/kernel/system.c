/*
 * Copyright (C) 2017-2018 Sergey Koshkin <koshkin.sergey@gmail.com>
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

/*******************************************************************************
 *  global variable definitions (scope: module-local)
 ******************************************************************************/

static osTask_t idle_task;
__WEAK osStack_t idle_task_stack[IDLE_STACK_SIZE];

static osTask_t timer_task;
__WEAK osStack_t timer_task_stack[TIMER_STACK_SIZE];

/*******************************************************************************
 *  function prototypes (scope: module-local)
 ******************************************************************************/

/*******************************************************************************
 *  function implementations (scope: module-local)
 ******************************************************************************/

__WEAK void osSysTickInit(uint32_t hz)
{
  ;
}

static
timer_t* GetTimer(void)
{
  timer_t *timer = NULL;
  queue_t *timer_queue = &knlInfo.timer_queue;

  BEGIN_CRITICAL_SECTION

  if (!isQueueEmpty(timer_queue)) {
    timer = GetTimerByQueue(timer_queue->next);
    if (time_after(timer->time, knlInfo.jiffies))
      timer = NULL;
    else
      TimerDelete(timer);
  }

  END_CRITICAL_SECTION

  return timer;
}

/**
 * @brief Idle task function.
 * @param par
 */
__WEAK __NO_RETURN
void osIdleTaskFunc(void *par)
{
  for (;;) {
    ;
  }
}

__NO_RETURN
static void TimerTaskFunc(void *par)
{
  timer_t *timer;

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

/**
 * @brief Create system idle task.
 *
 * Idle task priority (TN_NUM_PRIORITY-1) - lowest.
 */
static
void IdleTaskCreate(void)
{
  task_create_attr_t attr;

  attr.func_addr = (void *)osIdleTaskFunc;
  attr.func_param = NULL;
  attr.stk_size = sizeof(idle_task_stack)/sizeof(*idle_task_stack);
  attr.stk_start = (uint32_t *)&idle_task_stack[attr.stk_size-1];
  attr.priority = NUM_PRIORITY-1;
  attr.option = osTaskStartOnCreating;

  TaskCreate(&idle_task, &attr);
}

static
void TimerTaskCreate(void *par)
{
  task_create_attr_t attr;

  QueueReset(&knlInfo.timer_queue);

  attr.func_addr = (void *)TimerTaskFunc;
  attr.func_param = par;
  attr.stk_size = sizeof(timer_task_stack)/sizeof(*timer_task_stack);
  attr.stk_start = (uint32_t *)&timer_task_stack[attr.stk_size-1];
  attr.priority = 0;
  attr.option = osTaskStartOnCreating;

  TaskCreate(&timer_task, &attr);
}

void osTimerHandle(void)
{
  BEGIN_CRITICAL_SECTION

  knlInfo.jiffies += knlInfo.os_period;
  if (knlInfo.kernel_state == KERNEL_STATE_RUNNING) {
    TaskGetCurrent()->time += knlInfo.os_period;

#if defined(ROUND_ROBIN_ENABLE)
    volatile queue_t *curr_que;   //-- Need volatile here only to solve
    volatile queue_t *pri_queue;  //-- IAR(c) compiler's high optimization mode problem
    volatile int        priority;
    osTask_t *task = TaskGetCurrent();
    uint16_t *tslice_ticks = knlInfo.tslice_ticks;

    //-------  Round -robin (if is used)
    priority = task->priority;

    if (tslice_ticks[priority] != NO_TIME_SLICE) {
      task->tslice_count++;
      if (task->tslice_count > tslice_ticks[priority]) {
        task->tslice_count = 0;

        pri_queue = &knlInfo.ready_list[priority];
        //-- If ready queue is not empty and qty  of queue's tasks > 1
        if (!(isQueueEmpty((queue_t *)pri_queue)) &&
            pri_queue->next->next != pri_queue) {
          //-- Remove task from tail and add it to the head of
          //-- ready queue for current priority
          curr_que = queue_remove_tail(&knlInfo.ready_list[priority]);
          queue_add_head(&knlInfo.ready_list[priority],(queue_t *)curr_que);
        }
      }
    }
#endif  // ROUND_ROBIN_ENABLE

    TaskWaitComplete(&timer_task, TERR_NO_ERR);
  }

  END_CRITICAL_SECTION
}

/*******************************************************************************
 *  function implementations (scope: module-exported)
 ******************************************************************************/

/**
 * @brief Initial Kernel system start function, never returns. Typically
 *        called from main().
 * @param opt - Pointer to struct TN_OPTIONS.
 */
__NO_RETURN
void osKernelStart(TN_OPTIONS *opt)
{
  __disable_irq();

  knlInfo.kernel_state = KERNEL_STATE_NOT_RUN;

  for (int i=0; i < NUM_PRIORITY; i++) {
    QueueReset(&knlInfo.ready_list[i]);
#if defined(ROUND_ROBIN_ENABLE)
    knlInfo.tslice_ticks[i] = NO_TIME_SLICE;
#endif
  }

  knlInfo.HZ = opt->freq_timer;
  knlInfo.os_period = 1000/knlInfo.HZ;
  knlInfo.max_syscall_interrupt_priority = opt->max_syscall_interrupt_priority;

  TaskSetCurrent(&idle_task);
  TaskSetNext(&idle_task);

  /* Create Idle task */
  IdleTaskCreate();
  /* Create Timer task */
  TimerTaskCreate((void *)opt);

  //-- Run OS - first context switch
  archKernelStart();
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
  if (priority <= 0 || priority >= NUM_PRIORITY-1 ||
      value < 0 || value > MAX_TIME_SLICE)
    return TERR_WRONG_PARAM;

  BEGIN_DISABLE_INTERRUPT

  knlInfo.tslice_ticks[priority] = value;

  END_DISABLE_INTERRUPT
  return TERR_NO_ERR;
}

#endif  // ROUND_ROBIN_ENABLE

/*------------------------------ End of file ---------------------------------*/

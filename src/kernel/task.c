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

static
osError_t TaskActivate(osTask_t *task);

/*******************************************************************************
 *  function implementations (scope: module-local)
 ******************************************************************************/

/**
 * @fn    uint32_t* archStackInit(const osTask_t *task)
 * @brief
 * @param[in] task
 */
static
void StackInit(osTask_t *task)
{
  uint32_t *stk = task->stk_start;              //-- Load stack pointer
  stk++;

  *(--stk) = 0x01000000L;                       //-- xPSR
  *(--stk) = (uint32_t)task->func_addr;         //-- Entry Point
  *(--stk) = (uint32_t)osTaskExit;              //-- R14 (LR)
  *(--stk) = 0x12121212L;                       //-- R12
  *(--stk) = 0x03030303L;                       //-- R3
  *(--stk) = 0x02020202L;                       //-- R2
  *(--stk) = 0x01010101L;                       //-- R1
  *(--stk) = (uint32_t)task->func_param;        //-- R0 - task's function argument
  *(--stk) = 0x11111111L;                       //-- R11
  *(--stk) = 0x10101010L;                       //-- R10
  *(--stk) = 0x09090909L;                       //-- R9
  *(--stk) = 0x08080808L;                       //-- R8
  *(--stk) = 0x07070707L;                       //-- R7
  *(--stk) = 0x06060606L;                       //-- R6
  *(--stk) = 0x05050505L;                       //-- R5
  *(--stk) = 0x04040404L;                       //-- R4

  task->stk = (uint32_t)stk;
}

/**
 * @brief
 * @param
 * @return
 */
static
void TaskDispatch(void)
{
  uint8_t priority = TIMER_TASK_PRIORITY - __CLZ(knlInfo.ready_to_run_bmp);

  TaskSetNext(GetTaskByQueue(knlInfo.ready_list[priority].next));
}

/**
 * @fn    void TaskSetReady(osTask_t *thread)
 * @brief Adds task to the end of ready queue for current priority
 * @param thread
 */
static
void TaskSetReady(osTask_t *thread)
{
  knlInfo_t *info = &knlInfo;
  uint32_t priority = thread->priority;

  QueueAddTail(&info->ready_list[priority], &thread->task_que);
  info->ready_to_run_bmp |= (1UL << priority);
}

/**
 * @brief
 * @param
 * @return
 */
static
void TaskToRunnable(osTask_t *task)
{
  task->state = TSK_STATE_RUNNABLE;

  /* Add the task to the end of 'ready queue' for the current priority */
  TaskSetReady(task);

  if (task->priority > TaskGetNext()->priority) {
    TaskSetNext(task);
  }
}

/**
 * @brief
 * @param
 * @return
 */
static
void TaskToNonRunnable(osTask_t *task)
{
  uint32_t priority = task->priority;
  queue_t *que = &knlInfo.ready_list[priority];

  /* Remove the current task from any queue (now - from ready queue) */
  QueueRemoveEntry(&task->task_que);

  if (isQueueEmpty(que)) {
    /* No ready tasks for the curr priority */
    /* Remove 'ready to run' from the curr priority */
    knlInfo.ready_to_run_bmp &= ~(1UL << priority);

    /* Find highest priority ready to run -
       at least, MSB bit must be set for the idle task */
    TaskDispatch();
  }
  else if (task == TaskGetNext()) {
    /* There are 'ready to run' task(s) for the curr priority */
    TaskSetNext(GetTaskByQueue(que->next));
  }
}

/**
 * @fn          void TaskWaitExit(osTask_t *task, uint32_t ret_val)
 * @param[out]  task
 * @param[in]   ret_val
 */
static
void TaskWaitExit(osTask_t *task, uint32_t ret_val)
{
  task->wait_info.ret_val = ret_val;

  task->pwait_que = NULL;
  QueueRemoveEntry(&task->task_que);

  if ((task->state & TSK_STATE_SUSPEND) == 0U) {
    TaskToRunnable(task);
  }
  else {
    //-- remove WAIT state
    task->state = TSK_STATE_SUSPEND;
  }

  task->wait_reason = WAIT_REASON_NO;
}

/**
 * @fn          void TaskWaitExit_Handler(osTask_t *task)
 * @brief
 * @param[out]  task
 */
static
void TaskWaitExit_Handler(osTask_t *task)
{
  BEGIN_CRITICAL_SECTION

  TaskWaitExit(task, (uint32_t)TERR_TIMEOUT);

  END_CRITICAL_SECTION
}

/**
 * @fn          void TaskSetDormantState(osTask_t* task)
 * @brief
 * @param task
 */
static
void TaskSetDormantState(osTask_t* task)
{
  QueueReset(&task->task_que);
  QueueReset(&task->wait_timer.timer_que);
  QueueReset(&task->mutex_que);

  task->pwait_que = NULL;
  task->priority = task->base_priority;
  task->state = TSK_STATE_DORMANT;
  task->wait_reason = WAIT_REASON_NO;
  task->tslice_count = 0;
}

/**
 * @fn          void TaskWaitEnter(osTask_t *task, queue_t * wait_que, wait_reason_t wait_reason, osTime_t timeout)
 * @brief
 * @param task
 * @param wait_que
 * @param wait_reason
 * @param timeout
 */
void TaskWaitEnter(osTask_t *task, queue_t * wait_que, wait_reason_t wait_reason, osTime_t timeout)
{
  TaskToNonRunnable(task);

  task->state = TSK_STATE_WAIT;
  task->wait_reason = wait_reason;

  /* Add to the wait queue - FIFO */
  if (wait_que != NULL) {
    QueueAddTail(wait_que, &task->task_que);
    task->pwait_que = wait_que;
  }

  /* Add to the timers queue */
  if (timeout != TIME_WAIT_INFINITE)
    TimerInsert(&task->wait_timer, knlInfo.jiffies + timeout, (CBACK)TaskWaitExit_Handler, task);
}

/**
 * @fn          void TaskWaitComplete(osTask_t *task, uint32_t ret_val)
 * @brief
 * @param task
 * @param ret_val
 */
void TaskWaitComplete(osTask_t *task, uint32_t ret_val)
{
  TimerDelete(&task->wait_timer);
  TaskWaitExit(task, ret_val);
}

/**
 * @fn          void TaskWaitDelete(queue_t *wait_que)
 * @brief
 * @param wait_que
 */
void TaskWaitDelete(queue_t *wait_que)
{
  while (!isQueueEmpty(wait_que)) {
    TaskWaitComplete(GetTaskByQueue(QueueRemoveHead(wait_que)), (uint32_t)TERR_DLT);
  }
}

/**
 * @fn          void TaskChangePriority(osTask_t *task, uint32_t new_priority)
 * @brief
 * @param task
 * @param new_priority
 */
void TaskChangeRunningPriority(osTask_t *task, uint32_t new_priority)
{
  knlInfo_t *info = &knlInfo;
  uint32_t old_priority = task->priority;

  //-- remove curr task from any (wait/ready) queue
  QueueRemoveEntry(&task->task_que);

  //-- If there are no ready tasks for the old priority
  //-- clear ready bit for old priority
  if (isQueueEmpty(&info->ready_list[old_priority]))
    info->ready_to_run_bmp &= ~(1UL << old_priority);

  task->priority = new_priority;

  //-- Add task to the end of ready queue for current priority
  TaskSetReady(task);
  TaskDispatch();
}

/**
 * @fn          void TaskCreate(osTask_t *task, const task_create_attr_t *attr)
 * @param[out]  task  Pointer to the task TCB to be created
 * @param[in]   attr  Pointer to the structure with the attributes of the task to be created
 */
void TaskCreate(osTask_t *task, const task_create_attr_t *attr)
{
  /* Init task TCB */
  task->func_addr = attr->func_addr;
  task->func_param = attr->func_param;
  task->stk_start = attr->stk_start;
  task->stk_size = attr->stk_size;
  task->base_priority = attr->priority;
  task->id = ID_TASK;
  task->time = 0;

  /* Fill all task stack space by TN_FILL_STACK_VAL - only inside create_task */
  uint32_t *ptr = task->stk_start;
  for (uint32_t i = 0; i < task->stk_size; i++) {
    *ptr-- = TN_FILL_STACK_VAL;
  }

  TaskSetDormantState(task);

  if (attr->option & osTaskStartOnCreating)
    TaskActivate(task);
}

/**
 * @fn          osError_t TaskDelete(osTask_t *task)
 * @param[out]  task  Pointer to the task TCB to be deleted
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WCONTEXT     Unacceptable system's state for this request
 */
static
osError_t TaskDelete(osTask_t *task)
{
  /* Cannot delete not-terminated task */
  if (task->state != TSK_STATE_DORMANT)
    return TERR_WCONTEXT;

  task->id = ID_INVALID;

  return TERR_NO_ERR;
}

/**
 * @fn          osError_t TaskActivate(osTask_t *task)
 * @param[out]  task  Pointer to the task TCB to be activated
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_OVERFLOW     Task is already active (not in DORMANT state)
 */
static
osError_t TaskActivate(osTask_t *task)
{
  if (task->state != TSK_STATE_DORMANT)
    return TERR_OVERFLOW;

  StackInit(task);
  TaskToRunnable(task);

  return TERR_NO_ERR;
}

/**
 * @fn          osError_t TaskTerminate(osTask_t *task)
 * @param[out]  task  Pointer to the task TCB to be terminated
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WCONTEXT     Unacceptable system state for this request
 */
static
osError_t TaskTerminate(osTask_t *task)
{
  /* Cannot terminate running task */
  if (task->state == TSK_STATE_DORMANT || task == TaskGetCurrent())
    return TERR_WCONTEXT;

  if (task->state == TSK_STATE_RUNNABLE) {
    TaskToNonRunnable(task);
  }
  else if (task->state & TSK_STATE_WAIT) {
    /* Free all queues, involved in the 'waiting' */
    QueueRemoveEntry(&task->task_que);
    TimerDelete(&task->wait_timer);
  }

  /* Release owned Mutexes */
  MutexOwnerRelease(&task->mutex_que);

  TaskSetDormantState(task);

  return TERR_NO_ERR;
}

/**
 * @fn          void TaskExit(void)
 * @brief       Terminates the currently running task
 */
static
void TaskExit(void)
{
  osTask_t *task = TaskGetCurrent();

  /* Release owned Mutexes */
  MutexOwnerRelease(&task->mutex_que);

  TaskToNonRunnable(task);
  TaskSetDormantState(task);
}

/**
 * @fn          osError_t TaskSuspend(osTask_t *task)
 * @param[out]  task  Pointer to the task TCB to be suspended
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_OVERFLOW     Task already suspended
 *              TERR_WSTATE       Task is not active (i.e in DORMANT state )
 */
static
osError_t TaskSuspend(osTask_t *task)
{
  if (task->state & TSK_STATE_SUSPEND)
    return TERR_OVERFLOW;
  if (task->state == TSK_STATE_DORMANT)
    return TERR_WSTATE;

  if (task->state == TSK_STATE_RUNNABLE) {
    task->state = TSK_STATE_SUSPEND;
    TaskToNonRunnable(task);
  }
  else {
    task->state |= TSK_STATE_SUSPEND;
  }

  return TERR_NO_ERR;
}

/**
 * @fn          osError_t TaskResume(osTask_t *task)
 * @param[out]  task  Pointer to task TCB to be resumed
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_WSTATE       Task is not in SUSPEDED or WAITING_SUSPEDED state
 *              TERR_NOEXS        Object is not a task or non-existent
 *              TERR_ISR          The function cannot be called from interrupt service routines
 */
static
osError_t TaskResume(osTask_t *task)
{
  if ((task->state & TSK_STATE_SUSPEND) == 0U)
    return TERR_WSTATE;

  if ((task->state & TSK_STATE_WAIT) == 0U) {
    /* The task is not in the WAIT-SUSPEND state */
    TaskToRunnable(task);
  }
  else {
    /* Just remove TSK_STATE_SUSPEND from the task state */
    task->state &= ~TSK_STATE_SUSPEND;
  }

  return TERR_NO_ERR;
}

/**
 * @fn        void TaskSleep(osTime_t timeout)
 * @param[in] timeout   Timeout value must be greater than 0.
 */
static
void TaskSleep(osTime_t timeout)
{
  BEGIN_CRITICAL_SECTION

  TaskWaitEnter(TaskGetCurrent(), NULL, WAIT_REASON_SLEEP, timeout);

  END_CRITICAL_SECTION
}

/**
 * @fn          osError_t TaskWakeup(osTask_t *task)
 * @param[out]  task  Pointer to the task TCB to be wake up
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WSTATE       Task is not in WAIT state
 */
static
osError_t TaskWakeup(osTask_t *task)
{
  if ((task->state & TSK_STATE_WAIT) == 0U || task->wait_reason != WAIT_REASON_SLEEP)
    return TERR_WSTATE;

  TaskWaitComplete(task, (uint32_t)TERR_NO_ERR);

  return TERR_NO_ERR;
}

/**
 * @fn          osError_t TaskReleaseWait(osTask_t *task)
 * @param[out]  task  Pointer to the task TCB to be released from waiting or sleep
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WCONTEXT     Unacceptable system's state for function's request executing
 */
static
osError_t TaskReleaseWait(osTask_t *task)
{
  if (!(task->state & TSK_STATE_WAIT))
    return TERR_WCONTEXT;

  TaskWaitComplete(task, (uint32_t)TERR_NO_ERR);

  return TERR_NO_ERR;
}

static
osError_t TaskSetPriority(osTask_t *task, uint32_t new_priority)
{
  /* TODO При установке нового значения приоритета не меняется поле base_priority. Исправить */
  if (task->state == TSK_STATE_DORMANT)
    return TERR_WCONTEXT;

  if (task->state == TSK_STATE_RUNNABLE)
    TaskChangeRunningPriority(task, new_priority);
  else
    task->priority = new_priority;

  return TERR_NO_ERR;
}

static
osTime_t TaskGetTime(osTask_t *task)
{
  return task->time;
}

#if defined(__CC_ARM)

__SVC_INDIRECT(0)
void __svcTaskCreate(void (*)(osTask_t*, const task_create_attr_t*), osTask_t*, const task_create_attr_t*);

__STATIC_FORCEINLINE
void svcTaskCreate(osTask_t* task, const task_create_attr_t* task_create_attr)
{
  __svcTaskCreate(TaskCreate, task, task_create_attr);
}

__SVC_INDIRECT(0)
osError_t __svcTaskDelete(osError_t (*)(osTask_t*), osTask_t*);

__STATIC_FORCEINLINE
osError_t svcTaskDelete(osTask_t *task)
{
  return __svcTaskDelete(TaskDelete, task);
}

__SVC_INDIRECT(0)
osError_t __svcTaskActivate(osError_t (*)(osTask_t*), osTask_t*);

__STATIC_FORCEINLINE
osError_t svcTaskActivate(osTask_t *task)
{
  return __svcTaskActivate(TaskActivate, task);
}

__SVC_INDIRECT(0)
osError_t __svcTaskTerminate(osError_t (*)(osTask_t*), osTask_t*);

__STATIC_FORCEINLINE
osError_t svcTaskTerminate(osTask_t *task)
{
  return __svcTaskTerminate(TaskTerminate, task);
}

__SVC_INDIRECT(0)
void __svcTaskExit(void (*)(void));

__STATIC_FORCEINLINE
void svcTaskExit(void)
{
  __svcTaskExit(TaskExit);
}

__SVC_INDIRECT(0)
osError_t __svcTaskSuspend(osError_t (*)(osTask_t*), osTask_t*);

__STATIC_FORCEINLINE
osError_t svcTaskSuspend(osTask_t *task)
{
  return __svcTaskSuspend(TaskSuspend, task);
}

__SVC_INDIRECT(0)
osError_t __svcTaskResume(osError_t (*)(osTask_t*), osTask_t*);

__STATIC_FORCEINLINE
osError_t svcTaskResume(osTask_t *task)
{
  return __svcTaskResume(TaskResume, task);
}

__SVC_INDIRECT(0)
void __svcTaskSleep(void (*)(osTime_t), osTime_t);

__STATIC_FORCEINLINE
void svcTaskSleep(osTime_t timeout)
{
  __svcTaskSleep(TaskSleep, timeout);
}

__SVC_INDIRECT(0)
osError_t __svcTaskWakeup(osError_t (*)(osTask_t*), osTask_t*);

__STATIC_FORCEINLINE
osError_t svcTaskWakeup(osTask_t *task)
{
  return __svcTaskWakeup(TaskWakeup, task);
}

__SVC_INDIRECT(0)
osError_t __svcTaskReleaseWait(osError_t (*)(osTask_t*), osTask_t*);

__STATIC_FORCEINLINE
osError_t svcTaskReleaseWait(osTask_t *task)
{
  return __svcTaskReleaseWait(TaskReleaseWait, task);
}

__SVC_INDIRECT(0)
osError_t __svcTaskSetPriority(osError_t (*)(osTask_t*, uint32_t), osTask_t*, uint32_t);

__STATIC_FORCEINLINE
osError_t svcTaskSetPriority(osTask_t *task, uint32_t new_priority)
{
  return __svcTaskSetPriority(TaskSetPriority, task, new_priority);
}

__SVC_INDIRECT(0)
osTime_t __svcTaskGetTime(osTime_t (*)(osTask_t*), osTask_t*);

__STATIC_FORCEINLINE
osTime_t svcTaskGetTime(osTask_t *task)
{
  return __svcTaskGetTime(TaskGetTime, task);
}

#elif defined(__ICCARM__)

#else   // !(defined(__CC_ARM) || defined(__ICCARM__))

__STATIC_FORCEINLINE
void svcTaskCreate(osTask_t* task, const task_create_attr_t* task_create_attr)
{
  register uint32_t rf  __ASM(SVC_REG)  = (uint32_t)TaskCreate;
  register uint32_t r0  __ASM("r0")     = (uint32_t)task;
  register uint32_t r1  __ASM("r1")     = (uint32_t)task_create_attr;

  __ASM volatile ("svc 0" :: "r"(rf),"r"(r0),"r"(r1));
}

__STATIC_FORCEINLINE
osError_t svcTaskDelete(osTask_t *task)
{
  register uint32_t rf  __ASM(SVC_REG)  = (uint32_t)TaskDelete;
  register uint32_t r0  __ASM("r0")     = (uint32_t)task;

  __ASM volatile ("svc 0" : "=r"(r0) : "r"(rf),"r"(r0) : "r1");

  return ((osError_t)r0);
}

__STATIC_FORCEINLINE
osError_t svcTaskActivate(osTask_t *task)
{
  register uint32_t rf  __ASM(SVC_REG)  = (uint32_t)TaskActivate;
  register uint32_t r0  __ASM("r0")     = (uint32_t)task;

  __ASM volatile ("svc 0" : "=r"(r0) : "r"(rf),"r"(r0) : "r1");

  return ((osError_t)r0);
}

__STATIC_FORCEINLINE
osError_t svcTaskTerminate(osTask_t *task)
{
  register uint32_t rf  __ASM(SVC_REG)  = (uint32_t)TaskTerminate;
  register uint32_t r0  __ASM("r0")     = (uint32_t)task;

  __ASM volatile ("svc 0" : "=r"(r0) : "r"(rf),"r"(r0) : "r1");

  return ((osError_t)r0);
}

__STATIC_FORCEINLINE
void svcTaskExit(void)
{
  register uint32_t rf  __ASM(SVC_REG)  = (uint32_t)TaskExit;

  __ASM volatile ("svc 0" :: "r"(rf) : "r0","r1");
}

__STATIC_FORCEINLINE
osError_t svcTaskSuspend(osTask_t *task)
{
  register uint32_t rf  __ASM(SVC_REG)  = (uint32_t)TaskSuspend;
  register uint32_t r0  __ASM("r0")     = (uint32_t)task;

  __ASM volatile ("svc 0" : "=r"(r0) : "r"(rf),"r"(r0) : "r1");

  return ((osError_t)r0);
}

__STATIC_FORCEINLINE
osError_t svcTaskResume(osTask_t *task)
{
  register uint32_t rf  __ASM(SVC_REG)  = (uint32_t)TaskResume;
  register uint32_t r0  __ASM("r0")     = (uint32_t)task;

  __ASM volatile ("svc 0" : "=r"(r0) : "r"(rf),"r"(r0) : "r1");

  return ((osError_t)r0);
}

__STATIC_FORCEINLINE
void svcTaskSleep(osTime_t timeout)
{
  register uint32_t rf  __ASM(SVC_REG)  = (uint32_t)TaskSleep;
  register uint32_t r0  __ASM("r0")     = (uint32_t)timeout;

  __ASM volatile ("svc 0" :: "r"(rf),"r"(r0) : "r1");
}

__STATIC_FORCEINLINE
osError_t svcTaskWakeup(osTask_t *task)
{
  register uint32_t rf  __ASM(SVC_REG)  = (uint32_t)TaskWakeup;
  register uint32_t r0  __ASM("r0")     = (uint32_t)task;

  __ASM volatile ("svc 0" : "=r"(r0) : "r"(rf),"r"(r0) : "r1");

  return ((osError_t)r0);
}

__STATIC_FORCEINLINE
osError_t svcTaskReleaseWait(osTask_t *task)
{
  register uint32_t rf  __ASM(SVC_REG)  = (uint32_t)TaskReleaseWait;
  register uint32_t r0  __ASM("r0")     = (uint32_t)task;

  __ASM volatile ("svc 0" : "=r"(r0) : "r"(rf),"r"(r0) : "r1");

  return ((osError_t)r0);
}

__STATIC_FORCEINLINE
osError_t svcTaskSetPriority(osTask_t *task, uint32_t new_priority)
{
  register uint32_t rf  __ASM(SVC_REG)  = (uint32_t)TaskSetPriority;
  register uint32_t r0  __ASM("r0")     = (uint32_t)task;
  register uint32_t r1  __ASM("r1")     = (uint32_t)new_priority;

  __ASM volatile ("svc 0" : "=r"(r0) : "r"(rf),"r"(r0),"r"(r1));

  return ((osError_t)r0);
}

__STATIC_FORCEINLINE
osTime_t svcTaskGetTime(osTask_t *task)
{
  register uint32_t rf  __ASM(SVC_REG)  = (uint32_t)TaskGetTime;
  register uint32_t r0  __ASM("r0")     = (uint32_t)task;

  __ASM volatile ("svc 0" : "=r"(r0) : "r"(rf),"r"(r0) : "r1");

  return ((osTime_t)r0);
}

#endif

/*******************************************************************************
 *  function implementations (scope: module-exported)
 ******************************************************************************/

/**
 * @fn          osError_t osTaskCreate(osTask_t *task, void (*func)(void *), int32_t priority, const uint32_t *stack_start, int32_t stack_size, const void *param, int32_t option)
 * @brief       Creates a task.
 * @param[out]  task          Pointer to the task TCB to be created
 * @param[in]   func          Task body function
 * @param[in]   priority      Task priority. User tasks may have priorities 1…30
 * @param[in]   stack_start   Task's stack bottom address
 * @param[in]   stack_size    Task's stack size – number of task stack elements (not bytes)
 * @param[in]   param         task_func parameter. param will be passed to task_func on creation time
 * @param[in]   option        Creation option. Option values:
 *                              0                           After creation task has a DORMANT state
 *                              osTaskStartOnCreating   After creation task is switched to the runnable state (READY/RUNNING)
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_ISR          The function cannot be called from interrupt service routines
 */
osError_t osTaskCreate(osTask_t *task,
                       void (*func)(void *),
                       uint32_t priority,
                       const uint32_t *stack_start,
                       uint32_t stack_size,
                       const void *param,
                       uint32_t option)
{
  if (priority == IDLE_TASK_PRIORITY || priority >= TIMER_TASK_PRIORITY)
    return TERR_WRONG_PARAM;
  if ((stack_size < osStackSizeMin) || (func == NULL) || (task == NULL)
    || (stack_start == NULL) || (task->id != 0) )
    return TERR_WRONG_PARAM;
  if (IsIrqMode() || IsIrqMasked())
    return TERR_ISR;

  task_create_attr_t task_create_attr;

  task_create_attr.func_addr = (void *)func;
  task_create_attr.func_param = param;
  task_create_attr.stk_start = (uint32_t *)stack_start;
  task_create_attr.stk_size = stack_size;
  task_create_attr.priority = priority;
  task_create_attr.option = option;

  svcTaskCreate(task, &task_create_attr);

  return TERR_NO_ERR;
}

/**
 * @fn          osError_t osTaskDelete(osTask_t *task)
 * @brief       Deletes the task specified by the task.
 * @param[out]  task  Pointer to the task TCB to be deleted
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_WCONTEXT     Unacceptable system's state for this request
 *              TERR_NOEXS        Object is not a task or non-existent
 *              TERR_ISR          The function cannot be called from interrupt service routines
 */
osError_t osTaskDelete(osTask_t *task)
{
  if (task == NULL)
    return TERR_WRONG_PARAM;
  if (task->id != ID_TASK)
    return TERR_NOEXS;
  if (IsIrqMode() || IsIrqMasked())
    return TERR_ISR;

  return svcTaskDelete(task);
}

/**
 * @fn          osError_t osTaskActivate(osTask_t *task)
 * @brief       Activates a task specified by the task
 * @param[out]  task  Pointer to the task TCB to be activated
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_OVERFLOW     Task is already active (not in DORMANT state)
 *              TERR_NOEXS        Object is not a task or non-existent
 *              TERR_ISR          The function cannot be called from interrupt service routines
 */
osError_t osTaskActivate(osTask_t *task)
{
  if (task == NULL)
    return TERR_WRONG_PARAM;
  if (task->id != ID_TASK)
    return TERR_NOEXS;
  if (IsIrqMode() || IsIrqMasked())
    return TERR_ISR;

  return svcTaskActivate(task);
}

/**
 * @fn          osError_t osTaskTerminate(osTask_t *task)
 * @brief       Terminates the task specified by the task
 * @param[out]  task  Pointer to the task TCB to be terminated
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_WCONTEXT     Unacceptable system state for this request
 *              TERR_NOEXS        Object is not a task or non-existent
 *              TERR_ISR          The function cannot be called from interrupt service routines
 */
osError_t osTaskTerminate(osTask_t *task)
{
  if (task == NULL)
    return TERR_WRONG_PARAM;
  if (task->id != ID_TASK)
    return TERR_NOEXS;
  if (IsIrqMode() || IsIrqMasked())
    return TERR_ISR;

  return svcTaskTerminate(task);
}

/**
 * @fn          void osTaskExit(void)
 * @brief       Terminates the currently running task
 */
__NO_RETURN
void osTaskExit(void)
{
  svcTaskExit();
  for (;;);
}

/**
 * @fn          osError_t osTaskSuspend(osTask_t *task)
 * @brief       Suspends the task specified by the task
 * @param[out]  task  Pointer to the task TCB to be suspended
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_OVERFLOW     Task already suspended
 *              TERR_WSTATE       Task is not active (i.e in DORMANT state )
 *              TERR_NOEXS        Object is not a task or non-existent
 *              TERR_ISR          The function cannot be called from interrupt service routines
 */
osError_t osTaskSuspend(osTask_t *task)
{
  if (task == NULL)
    return TERR_WRONG_PARAM;
  if (task->id != ID_TASK)
    return TERR_NOEXS;
  if (IsIrqMode() || IsIrqMasked())
    return TERR_ISR;

  return svcTaskSuspend(task);
}

/**
 * @fn          osError_t osTaskResume(osTask_t *task)
 * @brief       Releases the task specified by the task from the SUSPENDED state
 * @param[out]  task  Pointer to task TCB to be resumed
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_WSTATE       Task is not in SUSPEDED or WAITING_SUSPEDED state
 *              TERR_NOEXS        Object is not a task or non-existent
 *              TERR_ISR          The function cannot be called from interrupt service routines
 */
osError_t osTaskResume(osTask_t *task)
{
  if (task == NULL)
    return TERR_WRONG_PARAM;
  if (task->id != ID_TASK)
    return TERR_NOEXS;
  if (IsIrqMode() || IsIrqMasked())
    return TERR_ISR;

  return svcTaskResume(task);
}

/**
 * @fn        osError_t osTaskSleep(osTime_t timeout)
 * @brief     Puts the currently running task to the sleep for at most timeout system ticks.
 * @param[in] timeout   Timeout value must be greater than 0.
 *                      A value of TIME_WAIT_INFINITE causes an infinite delay.
 * @return    TERR_NO_ERR       Normal completion
 *            TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *            TERR_ISR          The function cannot be called from interrupt service routines
 */
osError_t osTaskSleep(osTime_t timeout)
{
  if (timeout == 0)
    return TERR_WRONG_PARAM;
  if (IsIrqMode() || IsIrqMasked())
    return TERR_ISR;

  svcTaskSleep(timeout);

  return TERR_NO_ERR;
}

/**
 * @fn          osError_t osTaskWakeup(osTask_t *task)
 * @brief       Wakes up the task specified by the task from sleep mode
 * @param[out]  task  Pointer to the task TCB to be wake up
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_NOEXS        Object is not a task or non-existent
 *              TERR_WSTATE       Task is not in WAIT state
 *              TERR_ISR          The function cannot be called from interrupt service routines
 */
osError_t osTaskWakeup(osTask_t *task)
{
  if (task == NULL)
    return TERR_WRONG_PARAM;
  if (task->id != ID_TASK)
    return TERR_NOEXS;
  if (IsIrqMode() || IsIrqMasked())
    return TERR_ISR;

  return svcTaskWakeup(task);
}

/**
 * @fn          osError_t osTaskReleaseWait(osTask_t *task)
 * @brief       Forcibly releases the task specified by the task from waiting
 * @param[out]  task  Pointer to the task TCB to be released from waiting or sleep
 * @return      TERR_NO_ERR       Normal completion
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_WCONTEXT     Unacceptable system's state for function's request executing
 *              TERR_NOEXS        Object is not a task or non-existent
 *              TERR_ISR          The function cannot be called from interrupt service routines
 */
osError_t osTaskReleaseWait(osTask_t *task)
{
  if (task == NULL)
    return TERR_WRONG_PARAM;
  if (task->id != ID_TASK)
    return TERR_NOEXS;
  if (IsIrqMode() || IsIrqMasked())
    return TERR_ISR;

  return svcTaskReleaseWait(task);
}

osError_t osTaskSetPriority(osTask_t *task, uint32_t new_priority)
{
  if (task == NULL)
    return TERR_WRONG_PARAM;
  if (task->id != ID_TASK)
    return TERR_NOEXS;
  if ((new_priority == IDLE_TASK_PRIORITY) || (new_priority >= TIMER_TASK_PRIORITY))
    return TERR_WRONG_PARAM;
  if (IsIrqMode() || IsIrqMasked())
    return TERR_ISR;

  return svcTaskSetPriority(task, new_priority);
}

osTime_t osTaskGetTime(osTask_t *task)
{
  if (task == NULL)
    return 0;
  if (task->id != ID_TASK)
    return 0;
  if (IsIrqMode() || IsIrqMasked())
    return 0;

  return svcTaskGetTime(task);
}

/* ----------------------------- End of file ---------------------------------*/

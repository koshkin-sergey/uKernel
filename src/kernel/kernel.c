/*
 * Copyright (C) 2017-2019 Sergey Koshkin <koshkin.sergey@gmail.com>
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

#include "string.h"
#include "os_lib.h"

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

osInfo_t osInfo;

/*******************************************************************************
 *  global variable definitions (scope: module-local)
 ******************************************************************************/

/*******************************************************************************
 *  function prototypes (scope: module-local)
 ******************************************************************************/

/*******************************************************************************
 *  function implementations (scope: module-local)
 ******************************************************************************/

__WEAK void osSysTickInit(uint32_t hz)
{
  (void) hz;
}

static
timer_t* GetTimer(void)
{
  timer_t *timer = NULL;
  queue_t *timer_queue = &osInfo.timer_queue;

  BEGIN_CRITICAL_SECTION

  if (!isQueueEmpty(timer_queue)) {
    timer = GetTimerByQueue(timer_queue->next);
    if (time_after(timer->time, osInfo.kernel.tick))
      timer = NULL;
    else
      TimerDelete(timer);
  }

  END_CRITICAL_SECTION

  return timer;
}

void libTimerThread(void *argument)
{
  timer_t *timer;
  (void)   argument;

  for (;;) {
    while ((timer = GetTimer()) != NULL) {
      (*timer->callback)(timer->arg);
    }

    osDelay(osWaitForever);
  }
}

void osTimerHandle(void)
{
  BEGIN_CRITICAL_SECTION

  ++osInfo.kernel.tick;
  if (osInfo.kernel.state == osKernelRunning) {
    libThreadSuspend(osInfo.thread.timer);
  }

  END_CRITICAL_SECTION
}

static osStatus_t KernelInitialize(void)
{
  if (osInfo.kernel.state == osKernelReady) {
    return osOK;
  }

  if (osInfo.kernel.state != osKernelInactive) {
    return osError;
  }

  /* Initialize osInfo */
  memset(&osInfo, 0, sizeof(osInfo));

  for (uint32_t i = 0U; i < NUM_PRIORITY; i++) {
    QueueReset(&osInfo.ready_list[i]);
  }

  QueueReset(&osInfo.timer_queue);

  osInfo.kernel.state = osKernelReady;

  return (osOK);
}

static osStatus_t KernelStart(void)
{
  osThread_t *thread;
  uint32_t sh;

  if (osInfo.kernel.state != osKernelReady) {
    return osError;
  }

  /* Thread startup (Idle and Timer Thread) */
  if (!libThreadStartup()) {
    return osError;
  }

  /* Setup SVC and PendSV System Service Calls */
  sh = SystemIsrInit();
  osInfo.base_priority = (osConfig.max_api_interrupt_priority << sh) & 0x000000FFU;

  /* Setup RTOS Tick */
  osSysTickInit(osConfig.tick_freq);

  /* Switch to Ready Thread with highest Priority */
  thread = libThreadHighestPrioGet();
  if (thread == NULL) {
    return osError;
  }
  libThreadSwitch(thread);

  if ((osConfig.flags & osConfigPrivilegedMode) != 0U) {
    // Privileged Thread mode & PSP
    __set_CONTROL(0x02U);
  } else {
    // Unprivileged Thread mode & PSP
    __set_CONTROL(0x03U);
  }

  osInfo.kernel.state = osKernelRunning;

  return (osOK);
}

/*******************************************************************************
 *  function implementations (scope: module-exported)
 ******************************************************************************/

/**
 * @fn          osStatus_t osKernelInitialize(void)
 * @brief       Initialize the RTOS Kernel.
 * @return      status code that indicates the execution status of the function.
 */
osStatus_t osKernelInitialize(void)
{
  osStatus_t status;

  if (IsIrqMode() || IsIrqMasked()) {
    status = osErrorISR;
  }
  else {
    status = (osStatus_t)svc_0((uint32_t)KernelInitialize);
  }

  return (status);
}

/**
 * @fn          osStatus_t osKernelStart(void)
 * @brief       Start the RTOS Kernel scheduler.
 * @return      status code that indicates the execution status of the function.
 */
osStatus_t osKernelStart(void)
{
  osStatus_t status;

  if (IsIrqMode() || IsIrqMasked()) {
    status = osErrorISR;
  }
  else {
    status = (osStatus_t)svc_0((uint32_t)KernelStart);
  }

  return (status);
}

uint32_t osGetTickCount(void)
{
  return osInfo.kernel.tick;
}

/*------------------------------ End of file ---------------------------------*/

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
 * Software delay routines.
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

static uint32_t loops_per_jiffy;

/*******************************************************************************
 *  function prototypes (scope: module-local)
 ******************************************************************************/

/*******************************************************************************
 *  function implementations (scope: module-local)
 ******************************************************************************/

#pragma Otime
#pragma O2

/*-----------------------------------------------------------------------------*
  Название :  __delay
  Описание :  Вызывает необходимое количество пустых циклов.
  Параметры:  loops - количество пустых циклов.
  Результат:  Нет.
*-----------------------------------------------------------------------------*/
__attribute__((noinline)) static void __delay(unsigned long loops)
{
  while (loops--);
}

/*******************************************************************************
 *  function implementations (scope: module-exported)
 ******************************************************************************/

/*-----------------------------------------------------------------------------*
  Название :  calibrate_delay
  Описание :  Функция калибровки временных задержек. Записывает в переменную
              loops_per_jiffy количество пустых циклов за один тик ОС.
  Параметры:  Нет
  Результат:  Нет
*-----------------------------------------------------------------------------*/
void calibrate_delay(void)
{
  unsigned long ticks, loopbit;

  loops_per_jiffy = (1<<10);

  while (loops_per_jiffy <<= 1) {
    /* wait for "start of" clock tick */
    ticks = knlInfo.jiffies;
    while (ticks == knlInfo.jiffies);
    ticks = knlInfo.jiffies;
    __delay(loops_per_jiffy);
    ticks = knlInfo.jiffies - ticks;
    if (ticks)
      break;
  }

  /* Do a binary approximation to get loops_per_jiffy set to equal one clock
    (up to lps_precision bits) */
  loops_per_jiffy >>= 1;
  loopbit = loops_per_jiffy;
  while (loopbit >>= 1) {
    loops_per_jiffy |= loopbit;
    ticks = knlInfo.jiffies;
    while (ticks == knlInfo.jiffies);
    ticks = knlInfo.jiffies;
    __delay(loops_per_jiffy);
    if (knlInfo.jiffies != ticks) /* longer than 1 tick */
      loops_per_jiffy &= ~loopbit;
  }
}

/*-----------------------------------------------------------------------------*
  Название :  tn_mdelay
  Описание :  Вызывает задержку на указанное количество миллисекунд.
  Параметры:  ms - количество миллисекунд
  Результат:  Нет.
*-----------------------------------------------------------------------------*/
void tn_mdelay(unsigned long ms)
{
  unsigned long StartTime, delta;

  StartTime = knlInfo.jiffies;

  do
    delta = knlInfo.jiffies - StartTime;
  while (delta < ms);
}

/*-----------------------------------------------------------------------------*
  Название :  tn_udelay
  Описание :  Вызывает задержку на указанное количество микросекунд.
  Параметры:  usecs - количество микросекунд
  Результат:  Нет.
*-----------------------------------------------------------------------------*/
void tn_udelay(unsigned long usecs)
{
  unsigned long long loops;

  loops = (unsigned long long)usecs * 0x000010C7 * knlInfo.HZ * loops_per_jiffy;
  __delay(loops >> 32);
}

/*------------------------------ End of file ---------------------------------*/

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
 * Software delay routines.
 *
 */

/*******************************************************************************
 *  includes
 ******************************************************************************/

#include "tn_timer.h"
#include "tn_delay.h"

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

static unsigned long loops_per_jiffy;

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
    ticks = jiffies;
    while (ticks == jiffies);
    ticks = jiffies;
    __delay(loops_per_jiffy);
    ticks = jiffies - ticks;
    if (ticks)
      break;
  }

  /* Do a binary approximation to get loops_per_jiffy set to equal one clock
    (up to lps_precision bits) */
  loops_per_jiffy >>= 1;
  loopbit = loops_per_jiffy;
  while (loopbit >>= 1) {
    loops_per_jiffy |= loopbit;
    ticks = jiffies;
    while (ticks == jiffies);
    ticks = jiffies;
    __delay(loops_per_jiffy);
    if (jiffies != ticks) /* longer than 1 tick */
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

  StartTime = jiffies;

  do
    delta = jiffies - StartTime;
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

  loops = (unsigned long long)usecs * 0x000010C7 * HZ * loops_per_jiffy;
  __delay(loops >> 32);
}

/*------------------------------ End of file ---------------------------------*/

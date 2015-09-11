/******************** Copyright (c) 2011. All rights reserved ******************
  File Name  : tn_delay.c
  Author     : Koshkin Sergey
  Version    : 2.7
  Date       : 26/07/2011
  Description: Файл содержит функции для формирования миллисекундных и 
               микросекундных задержек.

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

*******************************************************************************/

#include "../include/tn_timer.h"

#pragma Otime
#pragma O2

unsigned long loops_per_jiffy;

void __delay(unsigned long loops) __attribute__((noinline));

/*-----------------------------------------------------------------------------*
  Название :  __delay
  Описание :  Вызывает необходимое количество пустых циклов.
  Параметры:  loops - количество пустых циклов.
  Результат:  Нет.
*-----------------------------------------------------------------------------*/
void __delay(unsigned long loops)
{
  while (loops--);
}

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

/*------------------------------ Конец файла ---------------------------------*/

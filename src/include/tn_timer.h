/******************** Copyright (c) 2011-2012. All rights reserved *************
  File Name  : tn_timer.h
  Author     : Koshkin Sergey
  Version    : 2.7
  Date       : 14/12/2012
  Description: TNKernel real-time kernel

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

#ifndef _TN_TIMER_H_
#define _TN_TIMER_H_

#include "tn_port.h"

#define ALARM_STOP      0U
#define ALARM_START     1U

#define CYCLIC_STOP     0U
#define CYCLIC_START    1U

typedef unsigned long TIME;

extern volatile TIME    jiffies;
extern unsigned long    os_period;
extern TN_TCB           timer_task;
extern unsigned short   tslice_ticks[TN_NUM_PRIORITY];
extern unsigned long    HZ;               // Частота системного таймера.
extern unsigned int     timer_task_stack[];
extern const unsigned int timer_stack_size;

void create_timer_task(void);
void timer_insert(TMEB *event, TIME time, CBACK callback, void *arg);
void timer_delete(TMEB *event);

#endif  // _TN_TIMER_H_

/*------------------------------ Конец файла ---------------------------------*/

/*

 TNKernel real-time kernel

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

#ifndef TN_TASKS_H_
#define TN_TASKS_H_

/*******************************************************************************
 *  includes
 ******************************************************************************/

#include "tn.h"
#include "tn_port.h"

/*******************************************************************************
 *  defines and macros (scope: module-local)
 ******************************************************************************/

/*******************************************************************************
 *  typedefs and structures (scope: module-local)
 ******************************************************************************/

/*******************************************************************************
 *  exported variables
 ******************************************************************************/

extern TN_TCB *tn_next_task_to_run;
extern unsigned int tn_ready_to_run_bmp;
extern CDLL_QUEUE tn_ready_list[TN_NUM_PRIORITY];

/*******************************************************************************
 *  exported function prototypes
 ******************************************************************************/
extern bool task_wait_complete(TN_TCB *task);
extern void task_curr_to_wait_action(CDLL_QUEUE *wait_que, int wait_reason,
																		 unsigned long timeout);
extern void change_running_task_priority(TN_TCB *task, int new_priority);
extern void set_current_priority(TN_TCB *task, int priority);
extern void task_to_runnable(TN_TCB *task);
extern void task_wait_delete(CDLL_QUEUE *que);

#endif /* TN_TASKS_H_ */

/* ----------------------------- End of file ---------------------------------*/

/******************** Copyright (c) 2013. All rights reserved ******************
 * File Name  :	tn_tasks.h
 * Author     :	Сергей
 * Version    :	1.00
 * Date       :	30.09.2013
 * Description:	TNKernel
 *
 * This software is supplied "AS IS" without warranties of any kind.
 *
 ******************************************************************************/

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

extern CDLL_QUEUE tn_ready_list[TN_NUM_PRIORITY];

/*******************************************************************************
 *  exported function prototypes
 ******************************************************************************/
extern int task_wait_complete(TN_TCB *task);
extern void task_curr_to_wait_action(CDLL_QUEUE *wait_que, int wait_reason,
																		 unsigned long timeout);
extern int change_running_task_priority(TN_TCB *task, int new_priority);
extern void set_current_priority(TN_TCB *task, int priority);
extern void task_to_runnable(TN_TCB *task);
extern void task_to_non_runnable(TN_TCB *task);
extern void task_wait_delete(CDLL_QUEUE *que);

#endif /* TN_TASKS_H_ */

/* ----------------------------- End of file ---------------------------------*/

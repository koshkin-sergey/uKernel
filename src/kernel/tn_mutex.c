/*

 TNKernel real-time kernel

 Copyright © 2004, 2010 Yuri Tiomkin
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

#include "tn_tasks.h"
#include "tn_utils.h"

#ifdef USE_MUTEXES

//----------------------------------------------------------------------------
//   Routines
//----------------------------------------------------------------------------

/*-----------------------------------------------------------------------------*
 * Название : find_max_blocked_priority
 * Описание :
 * Параметры:
 * Результат:
 *----------------------------------------------------------------------------*/
int find_max_blocked_priority(TN_MUTEX *mutex, int ref_priority)
{
	int priority;
	CDLL_QUEUE *curr_que;
	TN_TCB *task;

	priority = ref_priority;
	curr_que = mutex->wait_queue.next;
	while (curr_que != &(mutex->wait_queue)) {
		task = get_task_by_tsk_queue(curr_que);
		if (task->priority < priority) //--  task priority is higher
			priority = task->priority;

		curr_que = curr_que->next;
	}

	return priority;
}

/*-----------------------------------------------------------------------------*
 * Название : do_unlock_mutex
 * Описание :
 * Параметры:
 * Результат:
 *----------------------------------------------------------------------------*/
int do_unlock_mutex(TN_MUTEX *mutex)
{
	CDLL_QUEUE *curr_que;
	TN_MUTEX *tmp_mutex;
	TN_TCB *task;
	int pr;

	//-- Delete curr mutex from task's locked mutexes queue

	queue_remove_entry(&(mutex->mutex_queue));
	pr = tn_curr_run_task->base_priority;

	//---- No more mutexes, locked by the our task
	if (!is_queue_empty(&(tn_curr_run_task->mutex_queue))) {
		curr_que = tn_curr_run_task->mutex_queue.next;
		while (curr_que != &(tn_curr_run_task->mutex_queue)) {
			tmp_mutex = get_mutex_by_mutex_queque(curr_que);

			if (tmp_mutex->attr & TN_MUTEX_ATTR_CEILING) {
				if (tmp_mutex->ceil_priority < pr)
					pr = tmp_mutex->ceil_priority;
			}
			else {
				pr = find_max_blocked_priority(tmp_mutex, pr);
			}

			curr_que = curr_que->next;
		}
	}

	//-- Restore original priority
	if (pr != tn_curr_run_task->priority)
		change_running_task_priority(tn_curr_run_task, pr);

	//-- Check for the task(s) that want to lock the mutex
	if (is_queue_empty(&(mutex->wait_queue))) {
		mutex->holder = NULL;
		return true;
	}

	//--- Now lock the mutex by the first task in the mutex queue
	curr_que = queue_remove_head(&(mutex->wait_queue));
	task = get_task_by_tsk_queue(curr_que);
	mutex->holder = task;
	if (mutex->attr & TN_MUTEX_ATTR_RECURSIVE)
		mutex->cnt++;

	if ((mutex->attr & TN_MUTEX_ATTR_CEILING)
		&& (task->priority > mutex->ceil_priority))
		task->priority = mutex->ceil_priority;

	task_wait_complete(task);
	queue_add_tail(&(task->mutex_queue), &(mutex->mutex_queue));

	return true;
}

/*
 The ceiling protocol in ver 2.6 is more "lightweight" in comparison
 to previous versions.
 The code of ceiling protocol is derived from Vyacheslav Ovsiyenko version
 */

// L. Sha, R. Rajkumar, J. Lehoczky, Priority Inheritance Protocols: An Approach
// to Real-Time Synchronization, IEEE Transactions on Computers, Vol.39, No.9, 1990
/*-----------------------------------------------------------------------------*
 * Название : tn_mutex_create
 * Описание :	Создает мьютекс
 * Параметры:	mutex	-	Указатель на инициализируемую структуру TN_MUTEX
 * 										(дескриптор мьютекса)
 * 						attribute	-	Атрибуты создаваемого мьютекса.
 *												Возможно сочетание следующих значений:
 *												TN_MUTEX_ATTR_CEILING	-	Используется протокол
 *																								увеличения приоритета для
 *																								исключения инверсии приоритета
 *																								и взаимной блокировки.
 *												TN_MUTEX_ATTR_INHERIT	-	Используется протокол
 *																								наследования приоритета для
 *																								исключения инверсии приоритета.
 *												TN_MUTEX_ATTR_RECURSIVE	-	Разрешен рекурсивный захват
 *																									мьютекса, задачей, которая
 *																									уже захватила мьютекс.
 *						ceil_priority	-	Максимальный приоритет из всех задач, которые
 *														могут владеть мютексом. Параметр игнорируется,
 *														если attribute = TN_MUTEX_ATTR_INHERIT
 * Результат: Возвращает TERR_NO_ERR если выполнено без ошибок, в противном
 * 						случае TERR_WRONG_PARAM
 *----------------------------------------------------------------------------*/
int tn_mutex_create(TN_MUTEX * mutex, int attribute, int ceil_priority)
{
#if TN_CHECK_PARAM
	if (mutex == NULL)
		return TERR_WRONG_PARAM;
	if (mutex->id_mutex != 0) //-- no recreation
		return TERR_WRONG_PARAM;
	if ((attribute & TN_MUTEX_ATTR_CEILING)
		&& (ceil_priority < 1 || ceil_priority > TN_NUM_PRIORITY - 2))
		return TERR_WRONG_PARAM;
#endif

	queue_reset(&(mutex->wait_queue));
	queue_reset(&(mutex->mutex_queue));

	mutex->attr = attribute;
	mutex->holder = NULL;
	mutex->ceil_priority = ceil_priority;
	mutex->cnt = 0;
	mutex->id_mutex = TN_ID_MUTEX;

	return TERR_NO_ERR;
}

//----------------------------------------------------------------------------
int tn_mutex_delete(TN_MUTEX *mutex)
{
#if TN_CHECK_PARAM
	if (mutex == NULL)
		return TERR_WRONG_PARAM;
	if (mutex->id_mutex != TN_ID_MUTEX)
		return TERR_NOEXS;
#endif

	BEGIN_CRITICAL_SECTION

	if (mutex->holder != NULL && mutex->holder != tn_curr_run_task) {
		END_DISABLE_INTERRUPT
		return TERR_ILUSE;
	}

	//-- Remove all tasks(if any) from mutex's wait queue
	task_wait_delete(&mutex->wait_queue);

	if (mutex->holder != NULL) {  //-- If the mutex is locked
		do_unlock_mutex(mutex);
		queue_reset(&(mutex->mutex_queue));
	}
	mutex->id_mutex = 0; // Mutex not exists now

	END_CRITICAL_SECTION

	return TERR_NO_ERR;
}

//----------------------------------------------------------------------------
int tn_mutex_lock(TN_MUTEX *mutex, unsigned long timeout)
{
	int rc = TERR_NO_ERR;

#if TN_CHECK_PARAM
	if (mutex == NULL)
		return TERR_WRONG_PARAM;
	if (mutex->id_mutex != TN_ID_MUTEX)
		return TERR_NOEXS;
#endif

	BEGIN_CRITICAL_SECTION

	if (tn_curr_run_task == mutex->holder) {
		if (mutex->attr & TN_MUTEX_ATTR_RECURSIVE) {
			/*Recursive locking enabled*/
			mutex->cnt++;
			END_DISABLE_INTERRUPT
			return rc;
		}
		else {
			/*Recursive locking not enabled*/
			END_DISABLE_INTERRUPT
			return TERR_ILUSE;
		}
	}

	if (mutex->attr & TN_MUTEX_ATTR_CEILING) {
		if (tn_curr_run_task->base_priority < mutex->ceil_priority) { //-- base pri of task higher
			END_DISABLE_INTERRUPT
			return TERR_ILUSE;
		}

		if (mutex->holder == NULL) {  //-- mutex not locked
			mutex->holder = tn_curr_run_task;

			if (mutex->attr & TN_MUTEX_ATTR_RECURSIVE)
				mutex->cnt++;

			//-- Add mutex to task's locked mutexes queue
			queue_add_tail(&(tn_curr_run_task->mutex_queue), &(mutex->mutex_queue));
			//-- Ceiling protocol
			if (tn_curr_run_task->priority > mutex->ceil_priority)
				change_running_task_priority(tn_curr_run_task, mutex->ceil_priority);
		}
		else { //-- the mutex is already locked
			if (timeout == TN_POLLING)
				rc = TERR_TIMEOUT;
			else {
				tn_curr_run_task->wercd = &rc;
				//--- Task -> to the mutex wait queue
				task_curr_to_wait_action(&(mutex->wait_queue), TSK_WAIT_REASON_MUTEX_C,
																 timeout);
			}
		}
	}
	else {
		if (mutex->holder == NULL) {  //-- mutex not locked
			mutex->holder = tn_curr_run_task;

			if (mutex->attr & TN_MUTEX_ATTR_RECURSIVE)
				mutex->cnt++;

			queue_add_tail(&(tn_curr_run_task->mutex_queue), &(mutex->mutex_queue));
		}
		else {  //-- the mutex is already locked
			if (timeout == TN_POLLING)
				rc = TERR_TIMEOUT;
			else {
				//-- Base priority inheritance protocol
				//-- if run_task curr priority higher holder's curr priority
				if (tn_curr_run_task->priority < mutex->holder->priority)
					set_current_priority(mutex->holder, tn_curr_run_task->priority);
				tn_curr_run_task->wercd = &rc;
				task_curr_to_wait_action(&(mutex->wait_queue), TSK_WAIT_REASON_MUTEX_I,
																 timeout);
			}
		}
	}

	END_CRITICAL_SECTION
	return rc;
}

//----------------------------------------------------------------------------
int tn_mutex_unlock(TN_MUTEX *mutex)
{
#if TN_CHECK_PARAM
	if (mutex == NULL)
		return TERR_WRONG_PARAM;
	if (mutex->id_mutex != TN_ID_MUTEX)
		return TERR_NOEXS;
#endif

	BEGIN_CRITICAL_SECTION

	//-- Unlocking is enabled only for the owner and already locked mutex
	if (tn_curr_run_task != mutex->holder) {
		END_DISABLE_INTERRUPT
		return TERR_ILUSE;
	}

	if (mutex->attr & TN_MUTEX_ATTR_RECURSIVE)
		mutex->cnt--;

	if (mutex->cnt == 0)
		do_unlock_mutex(mutex);

	END_CRITICAL_SECTION

	return TERR_NO_ERR;
}

#endif //-- USE_MUTEXES
//----------------------------------------------------------------------------

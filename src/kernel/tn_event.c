/*******************************************************************************
 *
 * TNKernel real-time kernel
 *
 * Copyright © 2004, 2013 Yuri Tiomkin
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
 * Kernel system routines.
 *
 */

/*******************************************************************************
 *  includes
 ******************************************************************************/

#include "tn_tasks.h"
#include "tn_utils.h"

#ifdef  USE_EVENTS

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

/*******************************************************************************
 *  function implementations (scope: module-local)
 ******************************************************************************/

/*-----------------------------------------------------------------------------*
 * Название : scan_event_waitqueue
 * Описание : Проверяет очередь ожидания.
 * Параметры: evf	- Указатель на существующую структуру TN_EVENT.
 * Результат: Возвращает true если в очереди ожидания была найдена ожидающая задача
 * 						и она была успешно разбужена, в противном случае возвращает false.
 *----------------------------------------------------------------------------*/
static bool scan_event_waitqueue(TN_EVENT *evf)
{
	CDLL_QUEUE * que;
	TN_TCB * task;
	int fCond;

	que = evf->wait_queue.next;
  /*	checking ALL of the tasks waiting on the event.
			for the event with attr TN_EVENT_ATTR_SINGLE the only one task
			may be in the queue */
	while (que != &evf->wait_queue) {
		task = get_task_by_tsk_queue(que);
		que = que->next;

		if (task->winfo.event.mode & TN_EVENT_WCOND_OR)
			fCond = ((evf->pattern & task->winfo.event.pattern) != 0);
		else
			fCond = ((evf->pattern & task->winfo.event.pattern)
				== task->winfo.event.pattern);

		if (fCond) {
			queue_remove_entry(&task->task_queue);
			*task->winfo.event.flags_pattern = evf->pattern;
			return task_wait_complete(task);
		}
	}

	return false;
}

/*******************************************************************************
 *  function implementations (scope: module-exported)
 ******************************************************************************/

/*-----------------------------------------------------------------------------*
 * Название : tn_event_create
 * Описание : Создает флаг события.
 * Параметры: evf	-	Указатель на инициализируемую структуру TN_EVENT
 * 						attr	-	Атрибуты создаваемого флага.
 * 										Возможно сочетание следующих определений:
 * 										TN_EVENT_ATTR_CLR	-	Выполнять автоматическую очистку флага
 * 																				после его обработки. Возможно
 * 																				применение только совместно с атрибутом
 * 																				TN_EVENT_ATTR_SINGLE.
 * 										TN_EVENT_ATTR_SINGLE	- Использование флага только в одной
 * 																						задаче. Исрользование в нескольких
 * 																						задачах не допускается.
 * 										TN_EVENT_ATTR_MULTI	-	Использование флага возможно в
 * 																					нескольких задачах.
 * 										Атрибуты TN_EVENT_ATTR_SINGLE и TN_EVENT_ATTR_MULTI
 * 										взаимно исключающие. Не допускается использовать их
 * 										одновременно, но также не допускается вообще не указывать
 * 										ни один из этих атрибутов.
 * 						pattern	- Начальное битовое поле по которому идет определение
 * 											установки флага. Обычно должно быть равно 0.
 * Результат: Возвращает TERR_NO_ERR если выполнено без ошибок, в противном
 * 						случае TERR_WRONG_PARAM
 *----------------------------------------------------------------------------*/
int tn_event_create(TN_EVENT *evf, int attr, unsigned int pattern)
{
	if (evf == NULL)
		return TERR_WRONG_PARAM;
	if (evf->id_event == TN_ID_EVENT
		|| (((attr & TN_EVENT_ATTR_SINGLE) == 0)
			&& ((attr & TN_EVENT_ATTR_MULTI) == 0)))
		return TERR_WRONG_PARAM;
	if ((attr & TN_EVENT_ATTR_CLR) && ((attr & TN_EVENT_ATTR_SINGLE) == 0))
		return TERR_WRONG_PARAM;

	queue_reset(&evf->wait_queue);
	evf->pattern = pattern;
	evf->attr = attr;

	evf->id_event = TN_ID_EVENT;

	return TERR_NO_ERR;
}

/*-----------------------------------------------------------------------------*
 * Название : tn_event_delete
 * Описание : Удаляет флаг события.
 * Параметры: evf	- Указатель на существующую структуру TN_EVENT.
 * Результат: Возвращает один из вариантов:
 *							TERR_NO_ERR - функция выполнена без ошибок;
 *							TERR_WRONG_PARAM  - некорректно заданы параметры;
 *							TERR_NOEXS	-	флаг события не существует;
 *----------------------------------------------------------------------------*/
int tn_event_delete(TN_EVENT *evf)
{
#if TN_CHECK_PARAM
	if (evf == NULL)
		return TERR_WRONG_PARAM;
	if (evf->id_event != TN_ID_EVENT)
		return TERR_NOEXS;
#endif

	BEGIN_CRITICAL_SECTION

	task_wait_delete(&evf->wait_queue);

	evf->id_event = 0; // Event not exists now

	END_CRITICAL_SECTION

	return TERR_NO_ERR;
}

/*-----------------------------------------------------------------------------*
 * Название : tn_event_wait
 * Описание : Ожидает установки флага события в течении заданного интервала
 * 						времени.
 * Параметры: evf	- Указатель на существующую структуру TN_EVENT.
 * 						wait_pattern	- Ожидаемая комбинация битов. Не может быть равно 0.
 * 						wait_mode	- Режим ожидания.
 * 												Возможно одно из определений:
 * 													TN_EVENT_WCOND_OR	- Ожидается установка любого бита
 * 																							из ожидаемых.
 * 													TN_EVENT_WCOND_AND	-	Ожидается установка всех битов
 * 																								из ожидаемых.
 * 						p_flags_pattern	-	Указатель на переменную, в которую будет записано
 * 															значение комбинации битов по окончании ожидания.
 * 						timeout	-	Время ожидания установки флагов событий.
 * Результат: Возвращает один из вариантов:
 *							TERR_NO_ERR - функция выполнена без ошибок;
 *							TERR_WRONG_PARAM  - некорректно заданы параметры;
 *							TERR_NOEXS	-	флаг события не существует;
 *							TERR_ILUSE	-	флаг был создан с атрибутом TN_EVENT_ATTR_SINGLE,
 *														а его пытается использовать более одной задачи.
 *							TERR_TIMEOUT	-	Время ожидания истекло.
 *----------------------------------------------------------------------------*/
int tn_event_wait(TN_EVENT *evf, unsigned int wait_pattern, int wait_mode,
									unsigned int *p_flags_pattern, unsigned long timeout)
{
	int rc;
	int fCond;

#if TN_CHECK_PARAM
	if (evf == NULL || wait_pattern == 0 || p_flags_pattern == NULL)
		return TERR_WRONG_PARAM;
	if (evf->id_event != TN_ID_EVENT)
		return TERR_NOEXS;
#endif

	BEGIN_CRITICAL_SECTION

	//-- If event attr is TN_EVENT_ATTR_SINGLE and another task already
	//-- in event wait queue - return ERROR without checking release condition
	if ((evf->attr & TN_EVENT_ATTR_SINGLE) && !is_queue_empty(&evf->wait_queue)) {
		rc = TERR_ILUSE;
	}
	else {
		//-- Check release condition
		if (wait_mode & TN_EVENT_WCOND_OR) //-- any setted bit is enough for release condition
			fCond = ((evf->pattern & wait_pattern) != 0);
		else
			//-- TN_EVENT_WCOND_AND is default mode
			fCond = ((evf->pattern & wait_pattern) == wait_pattern);

		if (fCond) {
			*p_flags_pattern = evf->pattern;
			if (evf->attr & TN_EVENT_ATTR_CLR)
				evf->pattern &= ~wait_pattern;
			rc = TERR_NO_ERR;
		}
		else {
			if (timeout == TN_POLLING) {
				rc = TERR_TIMEOUT;
			}
			else {
				tn_curr_run_task->winfo.event.mode = wait_mode;
				tn_curr_run_task->winfo.event.pattern = wait_pattern;
				tn_curr_run_task->winfo.event.flags_pattern = p_flags_pattern;
				tn_curr_run_task->wercd = &rc;
				task_curr_to_wait_action(&evf->wait_queue, TSK_WAIT_REASON_EVENT,
																 timeout);
			}
		}
	}

	END_CRITICAL_SECTION

	return rc;
}

/*-----------------------------------------------------------------------------*
 * Название : tn_event_set
 * Описание : Устанавливает флаги событий.
 * Параметры: evf	- Указатель на существующую структуру TN_EVENT.
 * 						pattern	-	Комбинация битов. 1 в разряде соответствует
 * 											устанавливаемому флагу. Не может быть равен 0.
 * Результат: Возвращает один из вариантов:
 *							TERR_NO_ERR - функция выполнена без ошибок;
 *							TERR_WRONG_PARAM  - некорректно заданы параметры;
 *							TERR_NOEXS	-	флаг события не существует;
 *----------------------------------------------------------------------------*/
int tn_event_set(TN_EVENT *evf, unsigned int pattern)
{
#if TN_CHECK_PARAM
	if (evf == NULL || pattern == 0)
		return TERR_WRONG_PARAM;
	if (evf->id_event != TN_ID_EVENT)
		return TERR_NOEXS;
#endif

	BEGIN_CRITICAL_SECTION

	evf->pattern |= pattern;

	if (scan_event_waitqueue(evf)) { //-- There are task(s) that waiting state is complete
		if (evf->attr & TN_EVENT_ATTR_CLR)
			evf->pattern &= ~pattern;
	}

	END_CRITICAL_SECTION

	return TERR_NO_ERR;
}

/*-----------------------------------------------------------------------------*
 * Название : tn_event_clear
 * Описание : Очищает флаг события.
 * Параметры: evf	- Указатель на существующую структуру TN_EVENT.
 * 						pattern	-	Комбинация битов. 1 в разряде соответствует
 * 											очищаемому флагу. Не может быть равен 0.
 * Результат: Возвращает один из вариантов:
 *							TERR_NO_ERR - функция выполнена без ошибок;
 *							TERR_WRONG_PARAM  - некорректно заданы параметры;
 *							TERR_NOEXS	-	флаг события не существует;
 *----------------------------------------------------------------------------*/
int tn_event_clear(TN_EVENT *evf, unsigned int pattern)
{
#if TN_CHECK_PARAM
	if (evf == NULL || pattern == 0)
		return TERR_WRONG_PARAM;
	if (evf->id_event != TN_ID_EVENT)
		return TERR_NOEXS;
#endif

	BEGIN_DISABLE_INTERRUPT

	evf->pattern &= ~pattern;

	END_DISABLE_INTERRUPT
	return TERR_NO_ERR;
}

#endif   //#ifdef  USE_EVENTS

/* ----------------------------- End of file ---------------------------------*/


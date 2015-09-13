/*

 TNKernel real-time kernel

 Copyright � 2004, 2010 Yuri Tiomkin
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
 * �������� : scan_event_waitqueue
 * �������� : ��������� ������� ��������.
 * ���������: evf	- ��������� �� ������������ ��������� TN_EVENT.
 * ���������: ���������� 1 ���� � ������� �������� ���� ������� ��������� ������
 * 						� ��� ���� ������� ���������, � ��������� ������ ���������� 0.
 *----------------------------------------------------------------------------*/
static int scan_event_waitqueue(TN_EVENT *evf)
{
	CDLL_QUEUE * que;
	TN_TCB * task;
	int fCond, rc = 0;

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
			if (task_wait_complete(task))
				rc = 1;
		}
	}

	return rc;
}

/*******************************************************************************
 *  function implementations (scope: module-exported)
 ******************************************************************************/

/*-----------------------------------------------------------------------------*
 * �������� : tn_event_create
 * �������� : ������� ���� �������.
 * ���������: evf	-	��������� �� ���������������� ��������� TN_EVENT
 * 						attr	-	�������� ������������ �����.
 * 										�������� ��������� ��������� �����������:
 * 										TN_EVENT_ATTR_CLR	-	��������� �������������� ������� �����
 * 																				����� ��� ���������. ��������
 * 																				���������� ������ ��������� � ���������
 * 																				TN_EVENT_ATTR_SINGLE.
 * 										TN_EVENT_ATTR_SINGLE	- ������������� ����� ������ � �����
 * 																						������. ������������� � ����������
 * 																						������� �� �����������.
 * 										TN_EVENT_ATTR_MULTI	-	������������� ����� �������� �
 * 																					���������� �������.
 * 										�������� TN_EVENT_ATTR_SINGLE � TN_EVENT_ATTR_MULTI
 * 										������� �����������. �� ����������� ������������ ��
 * 										������������, �� ����� �� ����������� ������ �� ���������
 * 										�� ���� �� ���� ���������.
 * 						pattern	- ��������� ������� ���� �� �������� ���� �����������
 * 											��������� �����. ������ ������ ���� ����� 0.
 * ���������: ���������� TERR_NO_ERR ���� ��������� ��� ������, � ���������
 * 						������ TERR_WRONG_PARAM
 *----------------------------------------------------------------------------*/
int tn_event_create(TN_EVENT *evf, int attr, unsigned int pattern)
{
	if (!evf)
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
 * �������� : tn_event_delete
 * �������� : ������� ���� �������.
 * ���������: evf	- ��������� �� ������������ ��������� TN_EVENT.
 * ���������: ���������� ���� �� ���������:
 *							TERR_NO_ERR - ������� ��������� ��� ������;
 *							TERR_WRONG_PARAM  - ����������� ������ ���������;
 *							TERR_NOEXS	-	���� ������� �� ����������;
 *----------------------------------------------------------------------------*/
int tn_event_delete(TN_EVENT *evf)
{
#if TN_CHECK_PARAM
	if (!evf)
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
 * �������� : tn_event_wait
 * �������� : ������� ��������� ����� ������� � ������� ��������� ���������
 * 						�������.
 * ���������: evf	- ��������� �� ������������ ��������� TN_EVENT.
 * 						wait_pattern	- ��������� ���������� �����. �� ����� ���� ����� 0.
 * 						wait_mode	- ����� ��������.
 * 												�������� ���� �� �����������:
 * 													TN_EVENT_WCOND_OR	- ��������� ��������� ������ ����
 * 																							�� ���������.
 * 													TN_EVENT_WCOND_AND	-	��������� ��������� ���� �����
 * 																								�� ���������.
 * 						p_flags_pattern	-	��������� �� ����������, � ������� ����� ��������
 * 															�������� ���������� ����� �� ��������� ��������.
 * 						timeout	-	����� �������� ��������� ������ �������.
 * ���������: ���������� ���� �� ���������:
 *							TERR_NO_ERR - ������� ��������� ��� ������;
 *							TERR_WRONG_PARAM  - ����������� ������ ���������;
 *							TERR_NOEXS	-	���� ������� �� ����������;
 *							TERR_ILUSE	-	���� ��� ������ � ��������� TN_EVENT_ATTR_SINGLE,
 *														� ��� �������� ������������ ����� ����� ������.
 *							TERR_TIMEOUT	-	����� �������� �������.
 *----------------------------------------------------------------------------*/
int tn_event_wait(TN_EVENT *evf, unsigned int wait_pattern, int wait_mode,
									unsigned int *p_flags_pattern, unsigned long timeout)
{
	int rc;
	int fCond;

#if TN_CHECK_PARAM
	if (!evf || !wait_pattern || !p_flags_pattern)
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
 * �������� : tn_event_set
 * �������� : ������������� ����� �������.
 * ���������: evf	- ��������� �� ������������ ��������� TN_EVENT.
 * 						pattern	-	���������� �����. 1 � ������� �������������
 * 											���������������� �����. �� ����� ���� ����� 0.
 * ���������: ���������� ���� �� ���������:
 *							TERR_NO_ERR - ������� ��������� ��� ������;
 *							TERR_WRONG_PARAM  - ����������� ������ ���������;
 *							TERR_NOEXS	-	���� ������� �� ����������;
 *----------------------------------------------------------------------------*/
int tn_event_set(TN_EVENT *evf, unsigned int pattern)
{
#if TN_CHECK_PARAM
	if (!evf || !pattern)
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
 * �������� : tn_event_clear
 * �������� : ������� ���� �������.
 * ���������: evf	- ��������� �� ������������ ��������� TN_EVENT.
 * 						pattern	-	���������� �����. 1 � ������� �������������
 * 											���������� �����. �� ����� ���� ����� 0.
 * ���������: ���������� ���� �� ���������:
 *							TERR_NO_ERR - ������� ��������� ��� ������;
 *							TERR_WRONG_PARAM  - ����������� ������ ���������;
 *							TERR_NOEXS	-	���� ������� �� ����������;
 *----------------------------------------------------------------------------*/
int tn_event_clear(TN_EVENT *evf, unsigned int pattern)
{
#if TN_CHECK_PARAM
	if (!evf || !pattern)
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


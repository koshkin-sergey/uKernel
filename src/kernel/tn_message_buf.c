/******************** Copyright (c) 2013. All rights reserved ******************
 * File Name  :	tn_message_buf.c
 * Author     :	������
 * Version    :	1.00
 * Date       :	30.09.2013
 * Description:	TNKernel
 *
 * This software is supplied "AS IS" without warranties of any kind.
 *
 ******************************************************************************/

/*******************************************************************************
 *  includes
 ******************************************************************************/
#include "tn_tasks.h"
#include "tn_utils.h"

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
 * �������� : do_mbf_send
 * �������� : �������� ������ � ����� ��������� �� ������������� ��������
 * 						�������.
 * ���������: mbf	- ���������� ������ ���������.
 *						msg - ��������� �� ������.
 *						timeout - �����, � ������� �������� ������ ������ ���� ��������
 *											� �����.
 *						send_to_first - ����, �����������, ��� ���������� ��������� ������
 *														� ������ ������.
 * ���������: ���������� ���� �� ���������:
 *							TERR_NO_ERR - ������� ��������� ��� ������;
 *							TERR_WRONG_PARAM  - ����������� ������ ���������;
 *							TERR_NOEXS	-	����� �� ����������;
 *							TERR_TIMEOUT	-	�������� �������� �������� �������;
 *----------------------------------------------------------------------------*/
static int do_mbf_send(TN_MBF *mbf, void *msg, unsigned long timeout,
											 bool send_to_first)
{
	int rc = TERR_NO_ERR;
	CDLL_QUEUE *que;
	TN_TCB *task;

#if TN_CHECK_PARAM
	if (mbf == NULL)
		return TERR_WRONG_PARAM;
	if (mbf->id_mbf != TN_ID_MESSAGEBUF)
		return TERR_NOEXS;
#endif

	BEGIN_CRITICAL_SECTION

	//-- there are task(s) in the data queue's wait_receive list

	if (!is_queue_empty(&mbf->recv_queue)) {
		que = queue_remove_head(&mbf->recv_queue);
		task = get_task_by_tsk_queue(que);
		tn_memcpy(task->winfo.rmbf.msg, msg, mbf->msz);
		task_wait_complete(task);
	}
	/* the data queue's  wait_receive list is empty */
	else {
		rc = mbf_fifo_write(mbf, msg, send_to_first);
		//-- No free entries in the data queue
		if (rc != TERR_NO_ERR) {
			if (timeout == TN_POLLING) {
				rc = TERR_TIMEOUT;
			}
			else {
				tn_curr_run_task->wercd = &rc;
				tn_curr_run_task->winfo.smbf.msg = msg;
				tn_curr_run_task->winfo.smbf.send_to_first = send_to_first;
				task_curr_to_wait_action(&mbf->send_queue, TSK_WAIT_REASON_MBF_WSEND,
																 timeout);
			}
		}
	}

	END_CRITICAL_SECTION
	return rc;
}

/*******************************************************************************
 *  function implementations (scope: module-exported)
 ******************************************************************************/

/*-----------------------------------------------------------------------------*
 * �������� : tn_mbf_create
 * �������� : ������� ����� ���������.
 * ���������: mbf	-	��������� �� ������������ ��������� TN_MBF.
 *						buf	- ��������� �� ���������� ��� ����� ��������� ������� ������.
 *									����� ���� ����� NULL.
 *						bufsz	- ������ ������ ��������� � ������. ����� ���� ����� 0,
 *										����� ������ �������� ����� ����� � ���������� ������.
 *						msz	-	������ ��������� � ������. ������ ���� ������ ����.
 * ���������: ���������� ���� �� ���������:
 *							TERR_NO_ERR - ������� ��������� ��� ������;
 *							TERR_WRONG_PARAM  - ����������� ������ ���������;
 *----------------------------------------------------------------------------*/
int tn_mbf_create(TN_MBF *mbf, void *buf, int bufsz, int msz)
{
#if TN_CHECK_PARAM
	if (mbf == NULL)
		return TERR_WRONG_PARAM;
	if (bufsz < 0 || msz <= 0 || mbf->id_mbf == TN_ID_MESSAGEBUF)
		return TERR_WRONG_PARAM;
#endif

	BEGIN_CRITICAL_SECTION

	queue_reset(&mbf->send_queue);
	queue_reset(&mbf->recv_queue);

	mbf->buf = buf;
	mbf->msz = msz;
	mbf->num_entries = bufsz / msz;
	mbf->cnt = mbf->head = mbf->tail = 0;

	mbf->id_mbf = TN_ID_MESSAGEBUF;

	END_CRITICAL_SECTION

	return TERR_NO_ERR;
}

/*-----------------------------------------------------------------------------*
 * �������� : tn_mbf_delete
 * �������� : ������� ����� ���������.
 * ���������: mbf	- ��������� �� ������������ ��������� TN_MBF.
 * ���������: ���������� ���� �� ���������:
 *							TERR_NO_ERR - ������� ��������� ��� ������;
 *							TERR_WRONG_PARAM  - ����������� ������ ���������;
 *							TERR_NOEXS	-	����� ��������� �� ����������;
 *----------------------------------------------------------------------------*/
int tn_mbf_delete(TN_MBF *mbf)
{
#if TN_CHECK_PARAM
	if (mbf == NULL)
		return TERR_WRONG_PARAM;
	if (mbf->id_mbf == 0)
		return TERR_NOEXS;
#endif

	BEGIN_CRITICAL_SECTION

	task_wait_delete(&mbf->send_queue);
	task_wait_delete(&mbf->recv_queue);

	mbf->id_mbf = 0;

	END_CRITICAL_SECTION

	return TERR_NO_ERR;
}

/*-----------------------------------------------------------------------------*
 * �������� : tn_mbf_send
 * �������� : �������� ������ � ����� ������ ��������� �� �������������
 * 						�������� �������.
 * ���������: mbf	- ���������� ������ ���������.
 *						msg - ��������� �� ������.
 *						timeout - �����, � ������� �������� ������ ������ ���� ��������
 *											� �����.
 * ���������: ���������� ���� �� ���������:
 *							TERR_NO_ERR - ������� ��������� ��� ������;
 *							TERR_WRONG_PARAM  - ����������� ������ ���������;
 *							TERR_NOEXS	-	����� �� ����������;
 *							TERR_TIMEOUT	-	�������� �������� �������� �������;
 *----------------------------------------------------------------------------*/
int tn_mbf_send(TN_MBF *mbf, void *msg, unsigned long timeout)
{
	return do_mbf_send(mbf, msg, timeout, false);
}

/*-----------------------------------------------------------------------------*
 * �������� : tn_mbf_send_first
 * �������� : �������� ������ � ������ ������ ��������� �� �������������
 * 						�������� �������.
 * ���������: mbf	- ���������� ������ ���������.
 *						msg - ��������� �� ������.
 *						timeout - �����, � ������� �������� ������ ������ ���� ��������
 *											� �����.
 * ���������: ���������� ���� �� ���������:
 *							TERR_NO_ERR - ������� ��������� ��� ������;
 *							TERR_WRONG_PARAM  - ����������� ������ ���������;
 *							TERR_NOEXS	-	����� �� ����������;
 *							TERR_TIMEOUT	-	�������� �������� �������� �������;
 *----------------------------------------------------------------------------*/
int tn_mbf_send_first(TN_MBF *mbf, void *msg, unsigned long timeout)
{
	return do_mbf_send(mbf, msg, timeout, true);
}

/*-----------------------------------------------------------------------------*
 * �������� : tn_mbf_receive
 * �������� : ��������� ���� ������� ������ �� ������ ������ ���������
 * 						�� ������������� �������� �������.
 * ���������: mbf	- ���������� ������ ���������.
 * 						msg - ��������� �� �����, ���� ����� ������� ������.
 * 						timeout - �����, � ������� �������� ������ ������ ���� �������.
 * ���������: ���������� ���� �� ���������:
 *							TERR_NO_ERR - ������� ��������� ��� ������;
 *							TERR_WRONG_PARAM  - ����������� ������ ���������;
 *							TERR_NOEXS	-	����� �� ����������;
 *							TERR_TIMEOUT	-	�������� �������� �������� �������;
 *----------------------------------------------------------------------------*/
int tn_mbf_receive(TN_MBF *mbf, void *msg, unsigned long timeout)
{
	int rc; //-- return code
	CDLL_QUEUE *que;
	TN_TCB *task;

#if TN_CHECK_PARAM
	if (mbf == NULL || msg == NULL)
		return TERR_WRONG_PARAM;
	if (mbf->id_mbf != TN_ID_MESSAGEBUF)
		return TERR_NOEXS;
#endif

	BEGIN_CRITICAL_SECTION

	rc = mbf_fifo_read(mbf, msg);
	if (rc == TERR_NO_ERR) {  //-- There was entry(s) in data queue
		if (!is_queue_empty(&mbf->send_queue)) {
			que = queue_remove_head(&mbf->send_queue);
			task = get_task_by_tsk_queue(que);
			mbf_fifo_write(mbf, task->winfo.smbf.msg, task->winfo.smbf.send_to_first);
			task_wait_complete(task);
		}
	}
	else {  //-- data FIFO is empty
		if (!is_queue_empty(&mbf->send_queue)) {
			que = queue_remove_head(&mbf->send_queue);
			task = get_task_by_tsk_queue(que);
			tn_memcpy(msg, task->winfo.smbf.msg, mbf->msz);
			task_wait_complete(task);
			rc = TERR_NO_ERR;
		}
		else {  //-- wait_send_list is empty
			if (timeout == TN_POLLING) {
				rc = TERR_TIMEOUT;
			}
			else {
				tn_curr_run_task->wercd = &rc;
				tn_curr_run_task->winfo.rmbf.msg = msg;
				task_curr_to_wait_action(&mbf->recv_queue, TSK_WAIT_REASON_MBF_WRECEIVE,
																 timeout);
			}
		}
	}

	END_CRITICAL_SECTION

	return rc;
}

/*-----------------------------------------------------------------------------*
 * �������� : tn_mbf_flush
 * �������� : ���������� ����� ���������.
 * ���������: mbf	- ���������� ������ ���������.
 * ���������: ���������� ���� �� ���������:
 *							TERR_NO_ERR - ������� ��������� ��� ������;
 *							TERR_WRONG_PARAM  - ����������� ������ ���������;
 *							TERR_NOEXS  - ����� �� ��� ������.
 *----------------------------------------------------------------------------*/
int tn_mbf_flush(TN_MBF *mbf)
{
#if TN_CHECK_PARAM
	if (mbf == NULL)
		return TERR_WRONG_PARAM;
	if (mbf->id_mbf != TN_ID_MESSAGEBUF)
		return TERR_NOEXS;
#endif

	BEGIN_CRITICAL_SECTION

	mbf->cnt = mbf->tail = mbf->head = 0;

	END_CRITICAL_SECTION

	return TERR_NO_ERR;
}

/*-----------------------------------------------------------------------------*
 * �������� : tn_mbf_empty
 * �������� : ��������� ����� ��������� �� �������.
 * ���������: mbf	- ���������� ������ ���������.
 * ���������: ���������� ���� �� ���������:
 *							TERR_TRUE - ����� ��������� ����;
 *							TERR_NO_ERR - � ������ ������ ����;
 *							TERR_WRONG_PARAM  - ����������� ������ ���������;
 *							TERR_NOEXS  - ����� �� ��� ������.
 *----------------------------------------------------------------------------*/
int tn_mbf_empty(TN_MBF *mbf)
{
	int rc;

#if TN_CHECK_PARAM
	if (mbf == NULL)
		return TERR_WRONG_PARAM;
	if (mbf->id_mbf != TN_ID_MESSAGEBUF)
		return TERR_NOEXS;
#endif

	BEGIN_CRITICAL_SECTION

	if (mbf->cnt == 0)
		rc = TERR_TRUE;
	else
		rc = TERR_NO_ERR;

	END_CRITICAL_SECTION

	return rc;
}

/*-----------------------------------------------------------------------------*
 * �������� : tn_mbf_full
 * �������� : ��������� ����� ��������� �� ������ ����������.
 * ���������: mbf	- ���������� ������ ���������.
 * ���������: ���������� ���� �� ���������:
 *            	TERR_TRUE - ����� ��������� ������;
 *              TERR_NO_ERR - ����� ��������� �� ������;
 *              TERR_WRONG_PARAM  - ����������� ������ ���������;
 *              TERR_NOEXS  - ����� ��������� �� ��� ������.
 *----------------------------------------------------------------------------*/
int tn_mbf_full(TN_MBF *mbf)
{
	int rc;

#if TN_CHECK_PARAM
	if (mbf == NULL)
		return TERR_WRONG_PARAM;
	if (mbf->id_mbf != TN_ID_MESSAGEBUF)
		return TERR_NOEXS;
#endif

	BEGIN_CRITICAL_SECTION

	if (mbf->cnt == mbf->num_entries)
		rc = TERR_TRUE;
	else
		rc = TERR_NO_ERR;

	END_CRITICAL_SECTION

	return rc;
}

/*-----------------------------------------------------------------------------*
 * �������� : tn_mbf_cnt
 * �������� : ������� ���������� ���������� ��������� � ������ ���������.
 * ���������: mbf	- ���������� ������ ���������.
 *						cnt - ��������� �� ������ ������, � ������� ����� ����������
 *									���������� ���������.
 * ���������: ���������� ���� �� ���������:
 *							TERR_NO_ERR - ������� ��������� ��� ������;
 *							TERR_WRONG_PARAM  - ����������� ������ ���������;
 *							TERR_NOEXS  - ����� ��������� �� ��� ������.
 *----------------------------------------------------------------------------*/
int tn_mbf_cnt(TN_MBF *mbf, int *cnt)
{
#if TN_CHECK_PARAM
	if (mbf == NULL || cnt == NULL)
		return TERR_WRONG_PARAM;
	if (mbf->id_mbf != TN_ID_MESSAGEBUF)
		return TERR_NOEXS;
#endif

	BEGIN_CRITICAL_SECTION

	*cnt = mbf->cnt;

	END_CRITICAL_SECTION

	return TERR_NO_ERR;
}

/* ----------------------------- End of file ---------------------------------*/

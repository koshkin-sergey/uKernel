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

/* ver 2.7  */

#include "tn.h"
#include "tn_utils.h"

//----------------------------------------------------------------------------
//      Circular double-linked list queue
//----------------------------------------------------------------------------

#if (USE_INLINE_CDLL == 0)
/*-----------------------------------------------------------------------------*
 * Название : queue_reset
 * Описание : Сбрасывает двунаправленный список
 * Параметры: que - Указатель на список
 * Результат: Нет
 *-----------------------------------------------------------------------------*/
void queue_reset(CDLL_QUEUE *que)
{
	que->prev = que->next = que;
}

/*-----------------------------------------------------------------------------*
 * Название : is_queue_empty
 * Описание : Проверяет очередь на пустоту
 * Параметры: que - Указатель на очередь
 * Результат: Возвращает TRUE если очередь пуста, в противном случае FALSE
 *-----------------------------------------------------------------------------*/
BOOL is_queue_empty(CDLL_QUEUE *que)
{
	return ((que->next == que) ? TRUE : FALSE);
}

/*-----------------------------------------------------------------------------*
 * Название : queue_add_head
 * Описание :
 * Параметры:
 * Результат:
 *-----------------------------------------------------------------------------*/
void queue_add_head(CDLL_QUEUE * que, CDLL_QUEUE * entry)
{
	//--  Insert an entry at the head of the queue.

	entry->next = que->next;
	entry->prev = que;
	entry->next->prev = entry;
	que->next = entry;
}

/*-----------------------------------------------------------------------------*
 * Название : queue_add_tail
 * Описание :
 * Параметры:
 * Результат:
 *-----------------------------------------------------------------------------*/
void queue_add_tail(CDLL_QUEUE * que, CDLL_QUEUE * entry)
{
	//-- Insert an entry at the tail of the queue.

	entry->next = que;
	entry->prev = que->prev;
	entry->prev->next = entry;
	que->prev = entry;
}

/*-----------------------------------------------------------------------------*
 * Название : queue_remove_head
 * Описание :
 * Параметры:
 * Результат:
 *----------------------------------------------------------------------------*/
CDLL_QUEUE * queue_remove_head(CDLL_QUEUE * que)
{
	//-- Remove and return an entry at the head of the queue.

	CDLL_QUEUE * entry;

	if(que == NULL || que->next == que)
	return (CDLL_QUEUE *) 0;

	entry = que->next;
	entry->next->prev = que;
	que->next = entry->next;
	return entry;
}

/*-----------------------------------------------------------------------------*
 * Название : queue_remove_tail
 * Описание :
 * Параметры:
 * Результат:
 *----------------------------------------------------------------------------*/
CDLL_QUEUE * queue_remove_tail(CDLL_QUEUE * que)
{
	//-- Remove and return an entry at the tail of the queue.

	CDLL_QUEUE * entry;

	if(que->prev == que)
	return (CDLL_QUEUE *) 0;

	entry = que->prev;
	entry->prev->next = que;
	que->prev = entry->prev;
	return entry;
}

/*-----------------------------------------------------------------------------*
 * Название : queue_remove_entry
 * Описание :
 * Параметры:
 * Результат:
 *----------------------------------------------------------------------------*/
void queue_remove_entry(CDLL_QUEUE *entry)
{
	//--  Remove an entry from the queue.
	if (!is_queue_empty(entry)) {
		entry->prev->next = entry->next;
		entry->next->prev = entry->prev;
		queue_reset(entry);
	}
}

#endif

//----------------------------------------------------------------------------
BOOL queue_contains_entry(CDLL_QUEUE * que, CDLL_QUEUE * entry)
{
	//-- The entry in the queue ???

	CDLL_QUEUE * curr_que;

	curr_que = que->next;
	while (curr_que != que) {
		if (curr_que == entry)
			return TRUE;   //-- Found

		curr_que = curr_que->next;
	}

	return FALSE;
}

//---------------------------------------------------------------------------
//    Data queue storage FIFO processing
//---------------------------------------------------------------------------

/*-----------------------------------------------------------------------------*
 * Название : dque_fifo_write
 * Описание : Записывает данные в циклический буфер
 * Параметры: dque  - Дескриптор очереди данных.
 *						data_ptr  - Указатель на данные.
 *						send_to_first - Флаг, указывающий, что необходимо поместить данные
 *														в начало очереди.
 * Результат: Возвращает один из вариантов:
 *							TERR_NO_ERR - функция выполнена без ошибок;
 *							TERR_WRONG_PARAM  - некорректно заданы параметры;
 *							TERR_OUT_OF_MEM - Емкость очереди данных равна нулю.
 *-----------------------------------------------------------------------------*/
int dque_fifo_write(TN_DQUE *dque, void *data_ptr, BOOL send_to_first)
{
#if TN_CHECK_PARAM
	if (dque == NULL)
		return TERR_WRONG_PARAM;
	if (dque->num_entries <= 0)
		return TERR_OUT_OF_MEM;
#endif

	if (dque->cnt == dque->num_entries)
		return TERR_OVERFLOW;  //--  full

	if (send_to_first) {
		if (dque->tail_cnt == 0)
			dque->tail_cnt = dque->num_entries - 1;
		else
			dque->tail_cnt--;

		dque->data_fifo[dque->tail_cnt] = data_ptr;
	}
	else {
		dque->data_fifo[dque->header_cnt] = data_ptr;
		dque->header_cnt++;
		if (dque->header_cnt >= dque->num_entries)
			dque->header_cnt = 0;
	}

	dque->cnt++;

	return TERR_NO_ERR;
}

/*-----------------------------------------------------------------------------*
 * Название : dque_fifo_read
 * Описание : Читает данные из очереди.
 * Параметры: dque  - Дескриптор очереди данных.
 *						data_ptr  - Указатель на место в памяти куда будут считаны данные.
 * Результат: Возвращает один из вариантов:
 *							TERR_NO_ERR - функция выполнена без ошибок;
 *							TERR_WRONG_PARAM  - некорректно заданы параметры;
 *							TERR_OUT_OF_MEM - Емкость очереди данных равна нулю.
 *-----------------------------------------------------------------------------*/
int dque_fifo_read(TN_DQUE *dque, void **data_ptr)
{
#if TN_CHECK_PARAM
	if (dque == NULL || data_ptr == NULL)
		return TERR_WRONG_PARAM;
	if (dque->num_entries <= 0)
		return TERR_OUT_OF_MEM;
#endif

	if (dque->cnt == 0)
		return TERR_UNDERFLOW; //-- empty

	*data_ptr = dque->data_fifo[dque->tail_cnt];
	dque->cnt--;
	dque->tail_cnt++;
	if (dque->tail_cnt >= dque->num_entries)
		dque->tail_cnt = 0;

	return TERR_NO_ERR;
}

/*-----------------------------------------------------------------------------*
 * Название : mbf_fifo_write
 * Описание : Записывает данные в циклический буфер
 * Параметры: mbf	- Дескриптор буфера сообщений.
 *						msg - Указатель на данные.
 *						send_to_first - Флаг, указывающий, что необходимо поместить данные
 *														в начало буфера.
 * Результат: Возвращает один из вариантов:
 *							TERR_NO_ERR - функция выполнена без ошибок;
 *							TERR_WRONG_PARAM  - некорректно заданы параметры;
 *							TERR_OUT_OF_MEM - Емкость буфера равна нулю.
 *-----------------------------------------------------------------------------*/
int mbf_fifo_write(TN_MBF *mbf, void *msg, BOOL send_to_first)
{
	int bufsz, msz;

#if TN_CHECK_PARAM
	if (mbf == NULL)
		return TERR_WRONG_PARAM;
	if (mbf->num_entries <= 0)
		return TERR_OUT_OF_MEM;
#endif

	if (mbf->cnt == mbf->num_entries)
		return TERR_OVERFLOW;  //--  full

	msz = mbf->msz;
	bufsz = mbf->num_entries * msz;

	if (send_to_first) {
		if (mbf->tail == 0)
			mbf->tail = bufsz - msz;
		else
			mbf->tail -= msz;

		tn_memcpy(&mbf->buf[mbf->tail], msg, msz);
	}
	else {
		tn_memcpy(&mbf->buf[mbf->head], msg, msz);
		mbf->head += msz;
		if (mbf->head >= bufsz)
			mbf->head = 0;
	}

	mbf->cnt++;

	return TERR_NO_ERR;
}

/*-----------------------------------------------------------------------------*
 * Название : mbf_fifo_read
 * Описание : Читает данные из буфера сообщений.
 * Параметры: mbf	- Дескриптор буфера сообщений.
 *						msg - Указатель на место в памяти куда будут считаны данные.
 * Результат: Возвращает один из вариантов:
 *							TERR_NO_ERR - функция выполнена без ошибок;
 *							TERR_WRONG_PARAM  - некорректно заданы параметры;
 *							TERR_OUT_OF_MEM - Емкость буфера равна нулю.
 *-----------------------------------------------------------------------------*/
int mbf_fifo_read(TN_MBF *mbf, void *msg)
{
	int bufsz, msz;

#if TN_CHECK_PARAM
	if (mbf == NULL || msg == NULL)
		return TERR_WRONG_PARAM;
	if (mbf->num_entries <= 0)
		return TERR_OUT_OF_MEM;
#endif

	if (mbf->cnt == 0)
		return TERR_UNDERFLOW; //-- empty

	msz = mbf->msz;
	bufsz = mbf->num_entries * msz;

	tn_memcpy(msg, &mbf->buf[mbf->tail], msz);
	mbf->cnt--;
	mbf->tail += msz;
	if (mbf->tail >= bufsz)
		mbf->tail = 0;

	return TERR_NO_ERR;
}

/*------------------------------ Конец файла ---------------------------------*/

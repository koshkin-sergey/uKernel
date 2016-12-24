/*******************************************************************************
 *
 * TNKernel real-time kernel
 *
 * Copyright © 2004, 2013 Yuri Tiomkin
 * Copyright © 2013-2016 Sergey Koshkin <koshkin.sergey@gmail.com>
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

#ifndef _TN_UTILS_H_
#define _TN_UTILS_H_

/*******************************************************************************
 *  includes
 ******************************************************************************/

#include <stddef.h>
#include "tn.h"

/*******************************************************************************
 *  defines and macros (scope: module-local)
 ******************************************************************************/

/*******************************************************************************
 *  typedefs and structures (scope: module-local)
 ******************************************************************************/

/*******************************************************************************
 *  exported variables
 ******************************************************************************/

/*******************************************************************************
 *  exported function prototypes
 ******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#if (USE_INLINE_CDLL == 1)

/*-----------------------------------------------------------------------------*
 * Название : queue_reset
 * Описание : Сбрасывает двунаправленный список
 * Параметры: que - Указатель на список
 * Результат: Нет
 *-----------------------------------------------------------------------------*/
INLINE_FORCED void queue_reset(CDLL_QUEUE *que)
{
	que->prev = que->next = que;
}

/*-----------------------------------------------------------------------------*
 * Название : is_queue_empty
 * Описание : Проверяет очередь на пустоту
 * Параметры: que - Указатель на очередь
 * Результат: Возвращает true если очередь пуста, в противном случае false
 *-----------------------------------------------------------------------------*/
INLINE_FORCED bool is_queue_empty(CDLL_QUEUE *que)
{
	return ((que->next == que) ? true : false);
}

/*-----------------------------------------------------------------------------*
 * Название : queue_add_head
 * Описание :
 * Параметры:
 * Результат:
 *-----------------------------------------------------------------------------*/
INLINE_FORCED void queue_add_head(CDLL_QUEUE * que, CDLL_QUEUE * entry)
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
INLINE_FORCED void queue_add_tail(CDLL_QUEUE * que, CDLL_QUEUE * entry)
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
INLINE_FORCED CDLL_QUEUE * queue_remove_head(CDLL_QUEUE * que)
{
	//-- Remove and return an entry at the head of the queue.

	CDLL_QUEUE * entry;

	if (que == NULL || que->next == que)
		return (CDLL_QUEUE *)0;

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
INLINE_FORCED CDLL_QUEUE * queue_remove_tail(CDLL_QUEUE * que)
{
	//-- Remove and return an entry at the tail of the queue.

	CDLL_QUEUE * entry;

	if (que->prev == que)
		return (CDLL_QUEUE *)0;

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
INLINE_FORCED void queue_remove_entry(CDLL_QUEUE *entry)
{
	//--  Remove an entry from the queue.
	if (!is_queue_empty(entry)) {
		entry->prev->next = entry->next;
		entry->next->prev = entry->prev;
		queue_reset(entry);
	}
}

#else

extern void queue_reset(CDLL_QUEUE *que);
extern bool is_queue_empty(CDLL_QUEUE *que);
extern void queue_add_head(CDLL_QUEUE *que, CDLL_QUEUE *entry);
extern void queue_add_tail(CDLL_QUEUE *que, CDLL_QUEUE *entry);
extern CDLL_QUEUE *queue_remove_head(CDLL_QUEUE *que);
extern CDLL_QUEUE *queue_remove_tail(CDLL_QUEUE *que);
extern void queue_remove_entry(CDLL_QUEUE *entry);

#endif

#ifdef __cplusplus
}
#endif

#endif  // _TN_UTILS_H_

/*------------------------------ End of file ---------------------------------*/

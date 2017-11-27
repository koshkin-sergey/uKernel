/*
 * Copyright (C) 2011-2017 Sergey Koshkin <koshkin.sergey@gmail.com>
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Project: uKernel real-time kernel
 */

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

#include "knl_lib.h"

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

/*******************************************************************************
 *  function implementations (scope: module-exported)
 ******************************************************************************/

__FORCEINLINE void QueueReset(CDLL_QUEUE *que)
{
  que->prev = que->next = que;
}

__FORCEINLINE bool isQueueEmpty(CDLL_QUEUE *que)
{
  return ((que->next == que) ? true : false);
}

/**
 * @fn          void QueueAddHead(CDLL_QUEUE *que, CDLL_QUEUE *entry)
 * @brief       Inserts an entry at the head of the queue.
 * @param[out]  que     Pointer to the queue
 * @param[out]  entry   Pointer to an entry
 */
void QueueAddHead(CDLL_QUEUE *que, CDLL_QUEUE *entry)
{
  entry->next = que->next;
  entry->prev = que;
  entry->next->prev = entry;
  que->next = entry;
}

/**
 * @fn          void QueueAddTail(CDLL_QUEUE *que, CDLL_QUEUE *entry)
 * @brief       Inserts an entry at the tail of the queue.
 * @param[out]  que     Pointer to the queue
 * @param[out]  entry   Pointer to an entry
 */
void QueueAddTail(CDLL_QUEUE *que, CDLL_QUEUE *entry)
{
  entry->next = que;
  entry->prev = que->prev;
  entry->prev->next = entry;
  que->prev = entry;
}

/**
 * @fn          CDLL_QUEUE* QueueRemoveHead(CDLL_QUEUE *que)
 * @brief       Remove and return an entry at the head of the queue.
 * @param[out]  que   Pointer to the queue
 * @return      Returns a pointer to an entry at the head of the queue
 */
CDLL_QUEUE* QueueRemoveHead(CDLL_QUEUE *que)
{
  CDLL_QUEUE *entry = que->next;

  entry->next->prev = que;
  que->next = entry->next;

  return entry;
}

/**
 * @fn          CDLL_QUEUE* QueueRemoveTail(CDLL_QUEUE *que)
 * @brief       Remove and return an entry at the tail of the queue.
 * @param[out]  que   Pointer to the queue
 * @return      Returns a pointer to an entry at the tail of the queue
 */
CDLL_QUEUE* QueueRemoveTail(CDLL_QUEUE *que)
{
  CDLL_QUEUE *entry = que->prev;

  entry->prev->next = que;
  que->prev = entry->prev;

  return entry;
}

/**
 * @fn          void QueueRemoveEntry(CDLL_QUEUE *entry)
 * @brief       Removes an entry from the queue.
 * @param[out]  entry   Pointer to an entry of the queue
 */
void QueueRemoveEntry(CDLL_QUEUE *entry)
{
  if (!isQueueEmpty(entry)) {
    entry->prev->next = entry->next;
    entry->next->prev = entry->prev;
    QueueReset(entry);
  }
}

/*------------------------------ End of file ---------------------------------*/

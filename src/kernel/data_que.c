/*
 * Copyright (C) 2017 Sergey Koshkin <koshkin.sergey@gmail.com>
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

/*-----------------------------------------------------------------------------*
 * Название : dque_fifo_write
 * Описание : Записывает данные в циклический буфер
 * Параметры: dque  - Дескриптор очереди данных.
 *            data_ptr  - Указатель на данные.
 *            send_to_first - Флаг, указывающий, что необходимо поместить данные
 *                            в начало очереди.
 * Результат: Возвращает один из вариантов:
 *              TERR_NO_ERR - функция выполнена без ошибок;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_OUT_OF_MEM - Емкость очереди данных равна нулю.
 *-----------------------------------------------------------------------------*/
static
osError_t dque_fifo_write(TN_DQUE *dque, void *data_ptr, bool send_to_first)
{
  if (dque->num_entries <= 0)
    return TERR_OUT_OF_MEM;

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
 *            data_ptr  - Указатель на место в памяти куда будут считаны данные.
 * Результат: Возвращает один из вариантов:
 *              TERR_NO_ERR - функция выполнена без ошибок;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_OUT_OF_MEM - Емкость очереди данных равна нулю.
 *-----------------------------------------------------------------------------*/
static
osError_t dque_fifo_read(TN_DQUE *dque, void **data_ptr)
{
  if (dque->num_entries <= 0)
    return TERR_OUT_OF_MEM;

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
 * Название : do_queue_send
 * Описание : Помещает данные в очередь за установленный интервал времени.
 * Параметры: dque  - Дескриптор очереди данных.
 *            data_ptr  - Указатель на данные.
 *            timeout - Время, в течении которого данные должны быть помещены
 *                      в очередь.
 *            send_to_first - Флаг, указывающий, что необходимо поместить данные
 *                            в начало очереди.
 * Результат: Возвращает один из вариантов:
 *              TERR_NO_ERR - функция выполнена без ошибок;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_NOEXS  - очередь не существует;
 *              TERR_TIMEOUT  - Превышен заданный интервал времени;
 *----------------------------------------------------------------------------*/
static osError_t do_queue_send(TN_DQUE *dque, void *data_ptr, unsigned long timeout,
                         bool send_to_first)
{
  osError_t rc = TERR_NO_ERR;
  CDLL_QUEUE *que;
  osTask_t *task;

  if (dque == NULL)
    return TERR_WRONG_PARAM;
  if (dque->id != ID_DATAQUEUE)
    return TERR_NOEXS;

  BEGIN_CRITICAL_SECTION

  //-- there are task(s) in the data queue's wait_receive list

  if (!isQueueEmpty(&dque->wait_receive_list)) {
    que = QueueRemoveHead(&dque->wait_receive_list);
    task = GetTaskByQueue(que);
    *task->wait_info.rdque.data_elem = data_ptr;
    ThreadWaitComplete(task);
  }
  /* the data queue's  wait_receive list is empty */
  else {
    rc = dque_fifo_write(dque, data_ptr, send_to_first);
    //-- No free entries in the data queue
    if (rc != TERR_NO_ERR) {
      if (timeout == 0U) {
        rc = TERR_TIMEOUT;
      }
      else {
        task = TaskGetCurrent();
        task->wait_info.sdque.data_elem = data_ptr;  //-- Store data_ptr
        task->wait_info.sdque.send_to_first = send_to_first;
        TaskWaitEnter(task, &(dque->wait_send_list), WAIT_REASON_DQUE_WSEND, timeout);
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
 * Название : tn_queue_create
 * Описание : Создает очередь данных. Поле id структуры TN_DQUE предварительно
 *            должно быть установлено в 0.
 * Параметры: dque  - Указатель на существующую структуру TN_DQUE.
 *            data_fifo - Указатель на существующий массив void *.
 *                        Может быть равен 0.
 *            num_entries - Емкость очереди. Может быть равна 0, тогда задачи
 *                          общаются через эту очередь в синхронном режиме.
 * Результат: Возвращает один из вариантов:
 *              TERR_NO_ERR - функция выполнена без ошибок;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *----------------------------------------------------------------------------*/
osError_t tn_queue_create(TN_DQUE *dque, void **data_fifo, int num_entries)
{
  if (dque == NULL)
    return TERR_WRONG_PARAM;
  if (num_entries < 0 || dque->id == ID_DATAQUEUE)
    return TERR_WRONG_PARAM;

  BEGIN_CRITICAL_SECTION

  QueueReset(&(dque->wait_send_list));
  QueueReset(&(dque->wait_receive_list));

  dque->data_fifo = data_fifo;
  dque->num_entries = num_entries;
  if (dque->data_fifo == NULL)
    dque->num_entries = 0;

  dque->cnt = 0;
  dque->tail_cnt = 0;
  dque->header_cnt = 0;

  dque->id = ID_DATAQUEUE;

  END_CRITICAL_SECTION

  return TERR_NO_ERR;
}

/*-----------------------------------------------------------------------------*
 * Название : tn_queue_delete
 * Описание : Удаляет очередь данных.
 * Параметры: dque  - Указатель на существующую структуру TN_DQUE.
 * Результат: Возвращает один из вариантов:
 *              TERR_NO_ERR - функция выполнена без ошибок;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_NOEXS  -   очередь не существует;
 *----------------------------------------------------------------------------*/
osError_t tn_queue_delete(TN_DQUE *dque)
{
  if (dque == NULL)
    return TERR_WRONG_PARAM;
  if (dque->id != ID_DATAQUEUE)
    return TERR_NOEXS;

  BEGIN_CRITICAL_SECTION

  TaskWaitDelete(&dque->wait_send_list);
  TaskWaitDelete(&dque->wait_receive_list);

  dque->id = ID_INVALID; // Data queue not exists now

  END_CRITICAL_SECTION

  return TERR_NO_ERR;
}

/*-----------------------------------------------------------------------------*
 * Название : tn_queue_send
 * Описание : Помещает данные в конец очереди за установленный интервал времени.
 * Параметры: dque  - Дескриптор очереди данных.
 *            data_ptr  - Указатель на данные.
 *            timeout - Время, в течении которого данные должны быть помещены
 *                                          в очередь.
 * Результат: Возвращает один из вариантов:
 *              TERR_NO_ERR - функция выполнена без ошибок;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_NOEXS  -   очередь не существует;
 *              TERR_TIMEOUT    -   Превышен заданный интервал времени;
 *----------------------------------------------------------------------------*/
osError_t tn_queue_send(TN_DQUE *dque, void *data_ptr, unsigned long timeout)
{
  return do_queue_send(dque, data_ptr, timeout, false);
}

/*-----------------------------------------------------------------------------*
 * Название : tn_queue_send_first
 * Описание : Помещает данные в начало очереди за установленный интервал времени.
 * Параметры: dque  - Дескриптор очереди данных.
 *            data_ptr  - Указатель на данные.
 *            timeout - Время, в течении которого данные должны быть помещены
 *                                          в очередь.
 * Результат: Возвращает один из вариантов:
 *              TERR_NO_ERR - функция выполнена без ошибок;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_NOEXS  -   очередь не существует;
 *              TERR_TIMEOUT    -   Превышен заданный интервал времени;
 *----------------------------------------------------------------------------*/
osError_t tn_queue_send_first(TN_DQUE *dque, void *data_ptr, unsigned long timeout)
{
  return do_queue_send(dque, data_ptr, timeout, true);
}

/*-----------------------------------------------------------------------------*
 * Название : tn_queue_receive
 * Описание : Считывает один элемент данных из начала очереди за установленный
 *                      интервал времени.
 * Параметры: dque  - Дескриптор очереди данных.
 *            data_ptr  - Указатель на буфер, куда будут считаны данные.
 *            timeout - Время, в течении которого данные должны быть считаны.
 * Результат: Возвращает один из вариантов:
 *              TERR_NO_ERR - функция выполнена без ошибок;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_NOEXS  -   очередь не существует;
 *              TERR_TIMEOUT    -   Превышен заданный интервал времени;
 *----------------------------------------------------------------------------*/
osError_t tn_queue_receive(TN_DQUE *dque, void **data_ptr, unsigned long timeout)
{
  osError_t rc; //-- return code
  CDLL_QUEUE *que;
  osTask_t *task;

  if (dque == NULL || data_ptr == NULL)
    return TERR_WRONG_PARAM;
  if (dque->id != ID_DATAQUEUE)
    return TERR_NOEXS;

  BEGIN_CRITICAL_SECTION

  rc = dque_fifo_read(dque, data_ptr);
  if (rc == TERR_NO_ERR) {  //-- There was entry(s) in data queue
    if (!isQueueEmpty(&dque->wait_send_list)) {
      que = QueueRemoveHead(&dque->wait_send_list);
      task = GetTaskByQueue(que);
      dque_fifo_write(dque, task->wait_info.sdque.data_elem,
          task->wait_info.sdque.send_to_first);
      ThreadWaitComplete(task);
    }
  }
  else {  //-- data FIFO is empty
    if (!isQueueEmpty(&dque->wait_send_list)) {
      que = QueueRemoveHead(&dque->wait_send_list);
      task = GetTaskByQueue(que);
      *data_ptr = task->wait_info.sdque.data_elem; //-- Return to caller
      ThreadWaitComplete(task);
      rc = TERR_NO_ERR;
    }
    else {  //-- wait_send_list is empty
      if (timeout == 0U) {
        rc = TERR_TIMEOUT;
      }
      else {
        task = TaskGetCurrent();
        task->wait_info.rdque.data_elem = data_ptr;
        TaskWaitEnter(task, &(dque->wait_receive_list), WAIT_REASON_DQUE_WRECEIVE, timeout);
      }
    }
  }

  END_CRITICAL_SECTION

  return rc;
}

/*-----------------------------------------------------------------------------*
 * Название : tn_queue_flush
 * Описание : Сбрасывает очередь данных.
 * Параметры: dque  - Указатель на очередь данных.
 * Результат: Возвращает один из вариантов:
 *              TERR_NO_ERR - функция выполнена без ошибок;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_NOEXS  - очередь данных не была создана.
 *----------------------------------------------------------------------------*/
osError_t tn_queue_flush(TN_DQUE *dque)
{
  if (dque == NULL)
    return TERR_WRONG_PARAM;
  if (dque->id != ID_DATAQUEUE)
    return TERR_NOEXS;

  BEGIN_DISABLE_INTERRUPT

  dque->cnt = 0;
  dque->tail_cnt = 0;
  dque->header_cnt = 0;

  END_DISABLE_INTERRUPT

  return TERR_NO_ERR;
}

/*-----------------------------------------------------------------------------*
 * Название : tn_queue_empty
 * Описание : Проверяет очередь данных на пустоту.
 * Параметры: dque  - Указатель на очередь данных.
 * Результат: Возвращает один из вариантов:
 *              TERR_TRUE - очередь данных пуста;
 *              TERR_NO_ERR - в очереди данные есть;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_NOEXS  - очередь данных не была создана.
 *----------------------------------------------------------------------------*/
osError_t tn_queue_empty(TN_DQUE *dque)
{
  osError_t rc;

  if (dque == NULL)
    return TERR_WRONG_PARAM;
  if (dque->id != ID_DATAQUEUE)
    return TERR_NOEXS;

  BEGIN_DISABLE_INTERRUPT

  if (dque->cnt == 0)
    rc = TERR_TRUE;
  else
    rc = TERR_NO_ERR;

  END_DISABLE_INTERRUPT

  return rc;
}

/*-----------------------------------------------------------------------------*
 * Название : tn_queue_full
 * Описание : Проверяет очередь данных на полное заполнение.
 * Параметры: dque  - Указатель на очередь данных.
 * Результат: Возвращает один из вариантов:
 *              TERR_TRUE - очередь данных полная;
 *              TERR_NO_ERR - очередь данных не полная;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_NOEXS  - очередь данных не была создана.
 *----------------------------------------------------------------------------*/
osError_t tn_queue_full(TN_DQUE *dque)
{
  osError_t rc;

  if (dque == NULL)
    return TERR_WRONG_PARAM;
  if (dque->id != ID_DATAQUEUE)
    return TERR_NOEXS;

  BEGIN_DISABLE_INTERRUPT

  if (dque->cnt == dque->num_entries)
    rc = TERR_TRUE;
  else
    rc = TERR_NO_ERR;

  END_DISABLE_INTERRUPT

  return rc;
}

/*-----------------------------------------------------------------------------*
 * Название : tn_queue_cnt
 * Описание : Функция возвращает количество элементов в очереди данных.
 * Параметры: dque  - Дескриптор очереди данных.
 *            cnt - Указатель на ячейку памяти, в которую будет возвращено
 *                  количество элементов.
 * Результат: Возвращает один из вариантов:
 *              TERR_NO_ERR - функция выполнена без ошибок;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_NOEXS  - очередь данных не была создана.
 *----------------------------------------------------------------------------*/
osError_t tn_queue_cnt(TN_DQUE *dque, int *cnt)
{
  if (dque == NULL || cnt == NULL)
    return TERR_WRONG_PARAM;
  if (dque->id != ID_DATAQUEUE)
    return TERR_NOEXS;

  BEGIN_DISABLE_INTERRUPT

  *cnt = dque->cnt;

  END_DISABLE_INTERRUPT

  return TERR_NO_ERR;
}

/*------------------------------ End of file ---------------------------------*/

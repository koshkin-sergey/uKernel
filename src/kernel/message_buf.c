/*
 * Copyright (C) 2013-2017 Sergey Koshkin <koshkin.sergey@gmail.com>
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

/**
 * @file
 *
 * Kernel system routines.
 *
 */

/*******************************************************************************
 *  includes
 ******************************************************************************/

#include <string.h>
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

__svc_indirect(0)
osError_t svcMessageQueueNew(osError_t (*)(osMessageQueue_t*, void*, uint32_t, uint32_t), osMessageQueue_t*, void*, uint32_t, uint32_t);
__svc_indirect(0)
osError_t svcMessageQueueDelete(osError_t (*)(osMessageQueue_t*), osMessageQueue_t*);
__svc_indirect(0)
osError_t svcMessageQueuePut(osError_t (*)(osMessageQueue_t*, void*, osMsgPriority_t, osTime_t), osMessageQueue_t*, void*, osMsgPriority_t, osTime_t);
__svc_indirect(0)
osError_t svcMessageQueueGet(osError_t (*)(osMessageQueue_t*, void*, osTime_t), osMessageQueue_t*, void*, osTime_t);

/*******************************************************************************
 *  function implementations (scope: module-local)
 ******************************************************************************/

/*-----------------------------------------------------------------------------*
 * Название : mbf_fifo_write
 * Описание : Записывает данные в циклический буфер
 * Параметры: mbf - Дескриптор буфера сообщений.
 *            msg - Указатель на данные.
 *            msg_pri - Флаг, указывающий, что необходимо поместить данные
 *                            в начало буфера.
 * Результат: Возвращает один из вариантов:
 *              TERR_NO_ERR - функция выполнена без ошибок;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_OUT_OF_MEM - Емкость буфера равна нулю.
 *-----------------------------------------------------------------------------*/
static
osError_t mbf_fifo_write(osMessageQueue_t *mbf, void *msg, osMsgPriority_t msg_pri)
{
  int bufsz, msz;

  if (mbf->num_entries == 0)
    return TERR_OUT_OF_MEM;

  if (mbf->cnt == mbf->num_entries)
    return TERR_OVERFLOW;  //--  full

  msz = mbf->msz;
  bufsz = mbf->num_entries * msz;

  if (msg_pri == osMsgPriorityHigh) {
    if (mbf->tail == 0)
      mbf->tail = bufsz - msz;
    else
      mbf->tail -= msz;

    memcpy(&mbf->buf[mbf->tail], msg, msz);
  }
  else {
    memcpy(&mbf->buf[mbf->head], msg, msz);
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
 * Параметры: mbf - Дескриптор буфера сообщений.
 *            msg - Указатель на место в памяти куда будут считаны данные.
 * Результат: Возвращает один из вариантов:
 *              TERR_NO_ERR - функция выполнена без ошибок;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_OUT_OF_MEM - Емкость буфера равна нулю.
 *-----------------------------------------------------------------------------*/
static
osError_t mbf_fifo_read(osMessageQueue_t *mbf, void *msg)
{
  int bufsz, msz;

  if (mbf->num_entries == 0)
    return TERR_OUT_OF_MEM;

  if (mbf->cnt == 0)
    return TERR_UNDERFLOW; //-- empty

  msz = mbf->msz;
  bufsz = mbf->num_entries * msz;

  memcpy(msg, &mbf->buf[mbf->tail], msz);
  mbf->cnt--;
  mbf->tail += msz;
  if (mbf->tail >= bufsz)
    mbf->tail = 0;

  return TERR_NO_ERR;
}

static
osError_t MessageQueueNew(osMessageQueue_t *mq, void *buf, uint32_t bufsz, uint32_t msz)
{
  if ( mq->id == ID_MESSAG_QUEUE)
    return TERR_WRONG_PARAM;

  QueueReset(&mq->send_queue);
  QueueReset(&mq->recv_queue);

  mq->buf = buf;
  mq->msz = msz;
  mq->num_entries = bufsz / msz;
  mq->cnt = mq->head = mq->tail = 0;
  mq->id = ID_MESSAG_QUEUE;

  return TERR_NO_ERR;
}

static
osError_t MessageQueueDelete(osMessageQueue_t *mq)
{
  if (mq->id != ID_MESSAG_QUEUE)
    return TERR_NOEXS;

  ThreadWaitDelete(&mq->send_queue);
  ThreadWaitDelete(&mq->recv_queue);
  mq->id = ID_INVALID;

  return TERR_NO_ERR;
}

static
osError_t MessageQueuePut(osMessageQueue_t *mq, void *msg, osMsgPriority_t msg_pri, osTime_t timeout)
{
  osError_t rc;
  osTask_t *task;

  if (mq->id != ID_MESSAG_QUEUE)
    return TERR_NOEXS;

  if (!isQueueEmpty(&mq->recv_queue)) {
    /* There are task(s) in the data queue's wait_receive list */
    task = GetTaskByQueue(QueueRemoveHead(&mq->recv_queue));
    memcpy(task->wait_info.rmbf.msg, msg, mq->msz);
    ThreadWaitComplete(task);
    return TERR_NO_ERR;
  }

  rc = mbf_fifo_write(mq, msg, msg_pri);
  if (rc != TERR_NO_ERR) {
    /* No free entries in the data queue */
    if (timeout == 0U) {
      return TERR_TIMEOUT;
    }

    task = TaskGetCurrent();
    task->wait_rc = &rc;
    task->wait_info.smbf.msg = msg;
    task->wait_info.smbf.msg_pri = msg_pri;
    ThreadToWaitAction(task, &mq->send_queue, WAIT_REASON_MBF_WSEND, timeout);
  }

  return rc;
}

static
osError_t MessageQueueGet(osMessageQueue_t *mq, void *msg, osTime_t timeout)
{
  osError_t rc;
  osTask_t *task;

  if (mq->id != ID_MESSAG_QUEUE)
    return TERR_NOEXS;

  rc = mbf_fifo_read(mq, msg);

  if (!isQueueEmpty(&mq->send_queue)) {
    task = GetTaskByQueue(QueueRemoveHead(&mq->send_queue));
    if (rc == TERR_NO_ERR)
      mbf_fifo_write(mq, task->wait_info.smbf.msg, task->wait_info.smbf.msg_pri);
    else
      memcpy(msg, task->wait_info.smbf.msg, mq->msz);
    ThreadWaitComplete(task);
    return TERR_NO_ERR;
  }

  if (rc != TERR_NO_ERR) {
    if (timeout == 0U) {
      rc = TERR_TIMEOUT;
    }
    else {
      task = TaskGetCurrent();
      task->wait_rc = &rc;
      task->wait_info.rmbf.msg = msg;
      ThreadToWaitAction(task, &mq->recv_queue, WAIT_REASON_MBF_WRECEIVE, timeout);
    }
  }

  return rc;
}

/*******************************************************************************
 *  function implementations (scope: module-exported)
 ******************************************************************************/

/*-----------------------------------------------------------------------------*
 * Название : osMessageQueueNew
 * Описание : Создает буфер сообщений.
 * Параметры: mbf - Указатель на существующую структуру osMessageQueue_t.
 *            buf - Указатель на выделенную под буфер сообщений область памяти.
 *                  Может быть равен NULL.
 *            bufsz - Размер буфера сообщений в байтах. Может быть равен 0,
 *                    тогда задачи общаются через буфер в синхронном режиме.
 *            msz - Размер сообщения в байтах. Должен быть больше нуля.
 * Результат: Возвращает один из вариантов:
 *              TERR_NO_ERR - функция выполнена без ошибок;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_OUT_OF_MEM - Ошибка установки размера буфера.
 *----------------------------------------------------------------------------*/
osError_t osMessageQueueNew(osMessageQueue_t *mq, void *buf, uint32_t bufsz, uint32_t msz)
{
  if (mq == NULL || msz == 0U)
    return TERR_WRONG_PARAM;
  if (IS_IRQ_MODE() || IS_IRQ_MASKED())
    return TERR_ISR;

  return svcMessageQueueNew(MessageQueueNew, mq, buf, bufsz, msz);
}

/*-----------------------------------------------------------------------------*
 * Название : osMessageQueueDelete
 * Описание : Удаляет буфер сообщений.
 * Параметры: mbf - Указатель на существующую структуру osMessageQueue_t.
 * Результат: Возвращает один из вариантов:
 *              TERR_NO_ERR - функция выполнена без ошибок;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_NOEXS  - буфер сообщений не существует;
 *----------------------------------------------------------------------------*/
osError_t osMessageQueueDelete(osMessageQueue_t *mq)
{
  if (mq == NULL)
    return TERR_WRONG_PARAM;
  if (IS_IRQ_MODE() || IS_IRQ_MASKED())
    return TERR_ISR;

  return svcMessageQueueDelete(MessageQueueDelete, mq);
}

osError_t osMessageQueuePut(osMessageQueue_t *mq, void *msg, osMsgPriority_t msg_pri, osTime_t timeout)
{
  if (mq == NULL)
    return TERR_WRONG_PARAM;

  if (IS_IRQ_MODE() || IS_IRQ_MASKED()) {
    if (timeout != 0U)
      return TERR_WRONG_PARAM;

    return MessageQueuePut(mq, msg, msg_pri, timeout);
  }
  else {
    return svcMessageQueuePut(MessageQueuePut, mq, msg, msg_pri, timeout);
  }
}

osError_t osMessageQueueGet(osMessageQueue_t *mq, void *msg, osTime_t timeout)
{
  if (mq == NULL || msg == NULL)
    return TERR_WRONG_PARAM;

  if (IS_IRQ_MODE() || IS_IRQ_MASKED()) {
    if (timeout != 0U)
      return TERR_WRONG_PARAM;

    return MessageQueueGet(mq, msg, timeout);
  }
  else {
    return svcMessageQueueGet(MessageQueueGet, mq, msg, timeout);
  }
}

/*-----------------------------------------------------------------------------*
 * Название : tn_mbf_flush
 * Описание : Сбрасывает буфер сообщений.
 * Параметры: mbf - Дескриптор буфера сообщений.
 * Результат: Возвращает один из вариантов:
 *              TERR_NO_ERR - функция выполнена без ошибок;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_NOEXS  - буфер не был создан.
 *----------------------------------------------------------------------------*/
osError_t tn_mbf_flush(osMessageQueue_t *mbf)
{
  if (mbf == NULL)
    return TERR_WRONG_PARAM;
  if (mbf->id != ID_MESSAG_QUEUE)
    return TERR_NOEXS;

  BEGIN_CRITICAL_SECTION

  mbf->cnt = mbf->tail = mbf->head = 0;

  END_CRITICAL_SECTION

  return TERR_NO_ERR;
}

/*-----------------------------------------------------------------------------*
 * Название : tn_mbf_empty
 * Описание : Проверяет буфер сообщений на пустоту.
 * Параметры: mbf - Дескриптор буфера сообщений.
 * Результат: Возвращает один из вариантов:
 *              TERR_TRUE - буфер сообщений пуст;
 *              TERR_NO_ERR - в буфере данные есть;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_NOEXS  - буфер не был создан.
 *----------------------------------------------------------------------------*/
osError_t tn_mbf_empty(osMessageQueue_t *mbf)
{
  osError_t rc;

  if (mbf == NULL)
    return TERR_WRONG_PARAM;
  if (mbf->id != ID_MESSAG_QUEUE)
    return TERR_NOEXS;

  BEGIN_CRITICAL_SECTION

  if (mbf->cnt == 0)
    rc = TERR_TRUE;
  else
    rc = TERR_NO_ERR;

  END_CRITICAL_SECTION

  return rc;
}

/*-----------------------------------------------------------------------------*
 * Название : tn_mbf_full
 * Описание : Проверяет буфер сообщений на полное заполнение.
 * Параметры: mbf - Дескриптор буфера сообщений.
 * Результат: Возвращает один из вариантов:
 *              TERR_TRUE - буфер сообщений полный;
 *              TERR_NO_ERR - буфер сообщений не полный;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_NOEXS  - буфер сообщений не был создан.
 *----------------------------------------------------------------------------*/
osError_t tn_mbf_full(osMessageQueue_t *mbf)
{
  osError_t rc;

  if (mbf == NULL)
    return TERR_WRONG_PARAM;
  if (mbf->id != ID_MESSAG_QUEUE)
    return TERR_NOEXS;

  BEGIN_CRITICAL_SECTION

  if (mbf->cnt == mbf->num_entries)
    rc = TERR_TRUE;
  else
    rc = TERR_NO_ERR;

  END_CRITICAL_SECTION

  return rc;
}

/*-----------------------------------------------------------------------------*
 * Название : tn_mbf_cnt
 * Описание : Функция возвращает количество элементов в буфере сообщений.
 * Параметры: mbf - Дескриптор буфера сообщений.
 *            cnt - Указатель на ячейку памяти, в которую будет возвращено
 *                  количество элементов.
 * Результат: Возвращает один из вариантов:
 *              TERR_NO_ERR - функция выполнена без ошибок;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_NOEXS  - буфер сообщений не был создан.
 *----------------------------------------------------------------------------*/
osError_t tn_mbf_cnt(osMessageQueue_t *mbf, int *cnt)
{
  if (mbf == NULL || cnt == NULL)
    return TERR_WRONG_PARAM;
  if (mbf->id != ID_MESSAG_QUEUE)
    return TERR_NOEXS;

  BEGIN_CRITICAL_SECTION

  *cnt = mbf->cnt;

  END_CRITICAL_SECTION

  return TERR_NO_ERR;
}

/* ----------------------------- End of file ---------------------------------*/

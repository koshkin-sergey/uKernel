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
#include "arch.h"
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

SVC_CALL
osError_t svcMessageQueueNew(osError_t (*)(osMessageQueue_t*, void*, uint32_t, uint32_t), osMessageQueue_t*, void*, uint32_t, uint32_t);
SVC_CALL
osError_t svcMessageQueueDelete(osError_t (*)(osMessageQueue_t*), osMessageQueue_t*);
SVC_CALL
osError_t svcMessageQueuePut(osError_t (*)(osMessageQueue_t*, const void*, osMsgPriority_t, osTime_t), osMessageQueue_t*, const void*, osMsgPriority_t, osTime_t);
SVC_CALL
osError_t svcMessageQueueGet(osError_t (*)(osMessageQueue_t*, void*, osTime_t), osMessageQueue_t*, void*, osTime_t);
SVC_CALL
uint32_t svcMessageQueueGetMsgSize(uint32_t (*)(osMessageQueue_t*), osMessageQueue_t*);
SVC_CALL
uint32_t svcMessageQueueGetCapacity(uint32_t (*)(osMessageQueue_t*), osMessageQueue_t*);
SVC_CALL
uint32_t svcMessageQueueGetCount(uint32_t (*)(osMessageQueue_t*), osMessageQueue_t*);
SVC_CALL
uint32_t svcMessageQueueGetSpace(uint32_t (*)(osMessageQueue_t*), osMessageQueue_t*);
SVC_CALL
osError_t svcMessageQueueReset(osError_t (*)(osMessageQueue_t*), osMessageQueue_t*);

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
osError_t mbf_fifo_write(osMessageQueue_t *mbf, const void *msg, osMsgPriority_t msg_pri)
{
  int bufsz, msz;

  if (mbf->num_entries == 0)
    return TERR_OUT_OF_MEM;

  if (mbf->cnt == mbf->num_entries)
    return TERR_OVERFLOW;  //--  full

  msz = mbf->msg_size;
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

  msz = mbf->msg_size;
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
  if ( mq->id == ID_MESSAGE_QUEUE)
    return TERR_WRONG_PARAM;

  QueueReset(&mq->send_queue);
  QueueReset(&mq->recv_queue);

  mq->buf = buf;
  mq->msg_size = msz;
  mq->num_entries = bufsz / msz;
  mq->cnt = mq->head = mq->tail = 0;
  mq->id = ID_MESSAGE_QUEUE;

  return TERR_NO_ERR;
}

static
osError_t MessageQueueDelete(osMessageQueue_t *mq)
{
  if (mq->id != ID_MESSAGE_QUEUE)
    return TERR_NOEXS;

  TaskWaitDelete(&mq->send_queue);
  TaskWaitDelete(&mq->recv_queue);
  mq->id = ID_INVALID;

  return TERR_NO_ERR;
}

static
osError_t MessageQueuePut(osMessageQueue_t *mq, const void *msg, osMsgPriority_t msg_pri, osTime_t timeout)
{
  osTask_t *task;

  if (mq->id != ID_MESSAGE_QUEUE)
    return TERR_NOEXS;

  BEGIN_CRITICAL_SECTION

  if (!isQueueEmpty(&mq->recv_queue)) {
    /* There are task(s) in the data queue's wait_receive list */
    task = GetTaskByQueue(QueueRemoveHead(&mq->recv_queue));
    memcpy(task->wait_info.rmque.msg, msg, mq->msg_size);
    TaskWaitComplete(task, (uint32_t)TERR_NO_ERR);
    END_CRITICAL_SECTION
    return TERR_NO_ERR;
  }

  if (mbf_fifo_write(mq, msg, msg_pri) == TERR_NO_ERR) {
    END_CRITICAL_SECTION
    return TERR_NO_ERR;
  }

  /* No free entries in the data queue */
  if (timeout == 0U) {
    END_CRITICAL_SECTION
    return TERR_TIMEOUT;
  }

  task = TaskGetCurrent();
  task->wait_info.smque.msg = msg;
  task->wait_info.smque.msg_pri = msg_pri;
  TaskWaitEnter(task, &mq->send_queue, WAIT_REASON_MQUE_WSEND, timeout);

  END_CRITICAL_SECTION
  return TERR_WAIT;
}

static
osError_t MessageQueueGet(osMessageQueue_t *mq, void *msg, osTime_t timeout)
{
  osError_t rc;
  osTask_t *task;

  if (mq->id != ID_MESSAGE_QUEUE)
    return TERR_NOEXS;

  BEGIN_CRITICAL_SECTION

  rc = mbf_fifo_read(mq, msg);

  if (!isQueueEmpty(&mq->send_queue)) {
    task = GetTaskByQueue(QueueRemoveHead(&mq->send_queue));
    if (rc == TERR_NO_ERR)
      mbf_fifo_write(mq, task->wait_info.smque.msg, task->wait_info.smque.msg_pri);
    else
      memcpy(msg, task->wait_info.smque.msg, mq->msg_size);
    TaskWaitComplete(task, (uint32_t)TERR_NO_ERR);
    END_CRITICAL_SECTION
    return TERR_NO_ERR;
  }

  if (rc != TERR_NO_ERR) {
    if (timeout == 0U) {
      rc = TERR_TIMEOUT;
    }
    else {
      task = TaskGetCurrent();
      task->wait_info.rmque.msg = msg;
      TaskWaitEnter(task, &mq->recv_queue, WAIT_REASON_MQUE_WRECEIVE, timeout);
      rc = TERR_WAIT;
    }
  }

  END_CRITICAL_SECTION
  return rc;
}

static
uint32_t MessageQueueGetMsgSize(osMessageQueue_t *mq)
{
  if (mq->id != ID_MESSAGE_QUEUE)
    return 0U;

  return mq->msg_size;
}

static
uint32_t MessageQueueGetCapacity(osMessageQueue_t *mq)
{
  if (mq->id != ID_MESSAGE_QUEUE)
    return 0U;

  return mq->num_entries;
}

static
uint32_t MessageQueueGetCount(osMessageQueue_t *mq)
{
  if (mq->id != ID_MESSAGE_QUEUE)
    return 0U;

  return mq->cnt;
}

static
uint32_t MessageQueueGetSpace(osMessageQueue_t *mq)
{
  if (mq->id != ID_MESSAGE_QUEUE)
    return 0U;

  BEGIN_CRITICAL_SECTION

  uint32_t ret = mq->num_entries - mq->cnt;

  END_CRITICAL_SECTION

  return ret;
}

static
osError_t MessageQueueReset(osMessageQueue_t *mq)
{
  if (mq->id != ID_MESSAGE_QUEUE)
    return TERR_NOEXS;

  BEGIN_CRITICAL_SECTION

  mq->cnt = mq->tail = mq->head = 0;

  END_CRITICAL_SECTION

  return TERR_NO_ERR;
}

/*******************************************************************************
 *  function implementations (scope: module-exported)
 ******************************************************************************/

/**
 * @fn          osError_t osMessageQueueNew(osMessageQueue_t *mq, void *buf, uint32_t bufsz, uint32_t msz)
 * @brief       Creates and initializes a message queue object
 * @param[out]  mq      Pointer to the osMessageQueue_t structure
 * @param[out]  buf     Pointer to buffer for message
 * @param[in]   bufsz   Buffer size in bytes
 * @param[in]   msz     Maximum message size in bytes
 * @return      TERR_NO_ERR       The message queue object has been created
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_NOEXS        Object is not a Message Queue or non-existent
 *              TERR_ISR          Cannot be called from interrupt service routines
 */
osError_t osMessageQueueNew(osMessageQueue_t *mq, void *buf, uint32_t bufsz, uint32_t msz)
{
  if (mq == NULL || msz == 0U)
    return TERR_WRONG_PARAM;
  if (IsIrqMode() || IsIrqMasked())
    return TERR_ISR;

  return svcMessageQueueNew(MessageQueueNew, mq, buf, bufsz, msz);
}

/**
 * @fn          osError_t osMessageQueueDelete(osMessageQueue_t *mq)
 * @brief       Deletes a message queue object
 * @param[out]  mq  Pointer to the osMessageQueue_t structure
 * @return      TERR_NO_ERR       The message queue object has been deleted
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_NOEXS        Object is not a Message Queue or non-existent
 *              TERR_ISR          Cannot be called from interrupt service routines
 */
osError_t osMessageQueueDelete(osMessageQueue_t *mq)
{
  if (mq == NULL)
    return TERR_WRONG_PARAM;
  if (IsIrqMode() || IsIrqMasked())
    return TERR_ISR;

  return svcMessageQueueDelete(MessageQueueDelete, mq);
}

/**
 * @fn          osError_t osMessageQueuePut(osMessageQueue_t *mq, const void *msg, osMsgPriority_t msg_pri, osTime_t timeout)
 * @brief       Puts the message into the the message queue
 * @param[out]  mq        Pointer to the osMessageQueue_t structure
 * @param[in]   msg       Pointer to buffer with message to put into a queue
 * @param[in]   msg_pri   Message priority
 * @param[in]   timeout   Timeout Value or 0 in case of no time-out
 * @return      TERR_NO_ERR       The message has been put into the queue
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_NOEXS        Object is not a Message Queue or non-existent
 *              TERR_TIMEOUT      The message could not be put into the queue in the given time
 */
osError_t osMessageQueuePut(osMessageQueue_t *mq, const void *msg, osMsgPriority_t msg_pri, osTime_t timeout)
{
  if (mq == NULL)
    return TERR_WRONG_PARAM;

  if (IsIrqMode() || IsIrqMasked()) {
    if (timeout != 0U)
      return TERR_WRONG_PARAM;

    return MessageQueuePut(mq, msg, msg_pri, timeout);
  }
  else {
    osError_t ret_val = svcMessageQueuePut(MessageQueuePut, mq, msg, msg_pri, timeout);

    if (ret_val == TERR_WAIT)
      return (osError_t)TaskGetCurrent()->wait_info.ret_val;

    return ret_val;
  }
}

/**
 * @fn          osError_t osMessageQueueGet(osMessageQueue_t *mq, void *msg, osTime_t timeout)
 * @brief       Retrieves a message from the message queue and saves it to the buffer
 * @param[out]  mq        Pointer to the osMessageQueue_t structure
 * @param[out]  msg       Pointer to buffer for message to get from a queue
 * @param[in]   timeout   Timeout Value or 0 in case of no time-out
 * @return      TERR_NO_ERR       The message has been retrieved from the queue
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_NOEXS        Object is not a Message Queue or non-existent
 *              TERR_TIMEOUT      The message could not be retrieved from the queue in the given time
 */
osError_t osMessageQueueGet(osMessageQueue_t *mq, void *msg, osTime_t timeout)
{
  if (mq == NULL || msg == NULL)
    return TERR_WRONG_PARAM;

  if (IsIrqMode() || IsIrqMasked()) {
    if (timeout != 0U)
      return TERR_WRONG_PARAM;

    return MessageQueueGet(mq, msg, timeout);
  }
  else {
    osError_t ret_val = svcMessageQueueGet(MessageQueueGet, mq, msg, timeout);

    if (ret_val == TERR_WAIT)
      return (osError_t)TaskGetCurrent()->wait_info.ret_val;

    return ret_val;
  }
}

/**
 * @fn          uint32_t osMessageQueueGetMsgSize(osMessageQueue_t *mq)
 * @brief       Returns the maximum message size in bytes for the message queue
 * @param[out]  mq  Pointer to the osMessageQueue_t structure
 * @return      Maximum message size in bytes or 0 in case of an error
 */
uint32_t osMessageQueueGetMsgSize(osMessageQueue_t *mq)
{
  if (mq == NULL)
    return 0U;

  if (IsIrqMode() || IsIrqMasked()) {
    return MessageQueueGetMsgSize(mq);
  }
  else {
    return svcMessageQueueGetMsgSize(MessageQueueGetMsgSize, mq);
  }
}

/**
 * @fn          uint32_t osMessageQueueGetCapacity(osMessageQueue_t *mq)
 * @brief       Returns the maximum number of messages in the message queue
 * @param[out]  mq  Pointer to the osMessageQueue_t structure
 * @return      Maximum number of messages or 0 in case of an error
 */
uint32_t osMessageQueueGetCapacity(osMessageQueue_t *mq)
{
  if (mq == NULL)
    return 0U;

  if (IsIrqMode() || IsIrqMasked()) {
    return MessageQueueGetCapacity(mq);
  }
  else {
    return svcMessageQueueGetCapacity(MessageQueueGetCapacity, mq);
  }
}

/**
 * @fn          uint32_t osMessageQueueGetCount(osMessageQueue_t *mq)
 * @brief       Returns the number of queued messages in the message queue
 * @param[out]  mq  Pointer to the osMessageQueue_t structure
 * @return      Number of queued messages or 0 in case of an error
 */
uint32_t osMessageQueueGetCount(osMessageQueue_t *mq)
{
  if (mq == NULL)
    return 0U;

  if (IsIrqMode() || IsIrqMasked()) {
    return MessageQueueGetCount(mq);
  }
  else {
    return svcMessageQueueGetCount(MessageQueueGetCount, mq);
  }
}

/**
 * @fn          uint32_t osMessageQueueGetSpace(osMessageQueue_t *mq)
 * @brief       Returns the number available slots for messages in the message queue
 * @param[out]  mq  Pointer to the osMessageQueue_t structure
 * @return      Number of available slots for messages or 0 in case of an error
 */
uint32_t osMessageQueueGetSpace(osMessageQueue_t *mq)
{
  if (mq == NULL)
    return 0U;

  if (IsIrqMode() || IsIrqMasked()) {
    return MessageQueueGetSpace(mq);
  }
  else {
    return svcMessageQueueGetSpace(MessageQueueGetSpace, mq);
  }
}

/**
 * @fn          osError_t osMessageQueueReset(osMessageQueue_t *mq)
 * @brief       Resets the message queue
 * @param[out]  mq  Pointer to the osMessageQueue_t structure
 * @return      TERR_NO_ERR       The message queue has been reset
 *              TERR_WRONG_PARAM  Input parameter(s) has a wrong value
 *              TERR_NOEXS        Object is not a Message Queue or non-existent
 *              TERR_ISR          Cannot be called from interrupt service routines
 */
osError_t osMessageQueueReset(osMessageQueue_t *mq)
{
  if (mq == NULL)
    return TERR_WRONG_PARAM;
  if (IsIrqMode() || IsIrqMasked())
    return TERR_ISR;

  return svcMessageQueueReset(MessageQueueReset, mq);
}

/* ----------------------------- End of file ---------------------------------*/

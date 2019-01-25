/*
 * Copyright (C) 2013-2019 Sergey Koshkin <koshkin.sergey@gmail.com>
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
static osError_t mbf_fifo_write(osMessageQueue_t *mbf, const void *msg, osMsgPriority_t msg_pri)
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
static osError_t mbf_fifo_read(osMessageQueue_t *mbf, void *msg)
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

static osError_t MessageQueueNew(osMessageQueue_t *mq, void *buf, uint32_t bufsz, uint32_t msz)
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

const char *MessageQueueGetName(osMessageQueueId_t mq_id)
{
  return (NULL);
}

static osError_t MessageQueuePut(osMessageQueue_t *mq, const void *msg, osMsgPriority_t msg_pri, uint32_t timeout)
{
  osThread_t *task;

  if (mq->id != ID_MESSAGE_QUEUE)
    return TERR_NOEXS;

  BEGIN_CRITICAL_SECTION

  if (!isQueueEmpty(&mq->recv_queue)) {
    /* There are task(s) in the data queue's wait_receive list */
    task = GetTaskByQueue(QueueRemoveHead(&mq->recv_queue));
    memcpy(task->wait_info.rmque.msg, msg, mq->msg_size);
    _ThreadWaitExit(task, (uint32_t)TERR_NO_ERR);
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

  task = ThreadGetRunning();
  task->wait_info.smque.msg = msg;
  task->wait_info.smque.msg_pri = msg_pri;
  _ThreadWaitEnter(task, &mq->send_queue, timeout);

  END_CRITICAL_SECTION
  return TERR_WAIT;
}

static osError_t MessageQueueGet(osMessageQueue_t *mq, void *msg, uint32_t timeout)
{
  osError_t rc;
  osThread_t *task;

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
    _ThreadWaitExit(task, (uint32_t)TERR_NO_ERR);
    END_CRITICAL_SECTION
    return TERR_NO_ERR;
  }

  if (rc != TERR_NO_ERR) {
    if (timeout == 0U) {
      rc = TERR_TIMEOUT;
    }
    else {
      task = ThreadGetRunning();
      task->wait_info.rmque.msg = msg;
      _ThreadWaitEnter(task, &mq->recv_queue, timeout);
      rc = TERR_WAIT;
    }
  }

  END_CRITICAL_SECTION
  return rc;
}

static uint32_t MessageQueueGetCapacity(osMessageQueue_t *mq)
{
  if (mq->id != ID_MESSAGE_QUEUE)
    return 0U;

  return mq->num_entries;
}

static uint32_t MessageQueueGetMsgSize(osMessageQueue_t *mq)
{
  if (mq->id != ID_MESSAGE_QUEUE)
    return 0U;

  return mq->msg_size;
}

static uint32_t MessageQueueGetCount(osMessageQueue_t *mq)
{
  if (mq->id != ID_MESSAGE_QUEUE)
    return 0U;

  return mq->cnt;
}

static uint32_t MessageQueueGetSpace(osMessageQueue_t *mq)
{
  if (mq->id != ID_MESSAGE_QUEUE)
    return 0U;

  BEGIN_CRITICAL_SECTION

  uint32_t ret = mq->num_entries - mq->cnt;

  END_CRITICAL_SECTION

  return ret;
}

static osError_t MessageQueueReset(osMessageQueue_t *mq)
{
  if (mq->id != ID_MESSAGE_QUEUE)
    return TERR_NOEXS;

  BEGIN_CRITICAL_SECTION

  mq->cnt = mq->tail = mq->head = 0;

  END_CRITICAL_SECTION

  return TERR_NO_ERR;
}

static osError_t MessageQueueDelete(osMessageQueue_t *mq)
{
  if (mq->id != ID_MESSAGE_QUEUE)
    return TERR_NOEXS;

  _ThreadWaitDelete(&mq->send_queue);
  _ThreadWaitDelete(&mq->recv_queue);
  mq->id = ID_INVALID;

  return TERR_NO_ERR;
}

/*******************************************************************************
 *  Public API
 ******************************************************************************/

/**
 * @fn          osMessageQueueId_t osMessageQueueNew(uint32_t msg_count, uint32_t msg_size, const osMessageQueueAttr_t *attr)
 * @brief       Create and Initialize a Message Queue object.
 * @param[in]   msg_count   maximum number of messages in queue.
 * @param[in]   msg_size    maximum message size in bytes.
 * @param[in]   attr        message queue attributes.
 * @return      message queue ID for reference by other functions or NULL in case of error.
 */
osMessageQueueId_t osMessageQueueNew(uint32_t msg_count, uint32_t msg_size, const osMessageQueueAttr_t *attr)
{
  osMessageQueueId_t mq_id;

  if (IsIrqMode() || IsIrqMasked()) {
    mq_id = NULL;
  }
  else {
    mq_id = (osMessageQueueId_t)svc_3(msg_count, msg_size, (uint32_t)attr, (uint32_t)MessageQueueNew);
  }

  return (mq_id);
}

/**
 * @fn          const char *osMessageQueueGetName(osMessageQueueId_t mq_id)
 * @brief       Get name of a Message Queue object.
 * @param[in]   mq_id   message queue ID obtained by \ref osMessageQueueNew.
 * @return      name as null-terminated string or NULL in case of an error.
 */
const char *osMessageQueueGetName(osMessageQueueId_t mq_id)
{
  const char *name;

  if (IsIrqMode() || IsIrqMasked()) {
    name = NULL;
  }
  else {
    name = (const char *)svc_1((uint32_t)mq_id, (uint32_t)MessageQueueGetName);
  }

  return (name);
}

/**
 * @fn          osStatus_t osMessageQueuePut(osMessageQueueId_t mq_id, const void *msg_ptr, uint8_t msg_prio, uint32_t timeout)
 * @brief       Put a Message into a Queue or timeout if Queue is full.
 * @param[in]   mq_id     message queue ID obtained by \ref osMessageQueueNew.
 * @param[in]   msg_ptr   pointer to buffer with message to put into a queue.
 * @param[in]   msg_prio  message priority.
 * @param[in]   timeout   \ref CMSIS_RTOS_TimeOutValue or 0 in case of no time-out.
 * @return      status code that indicates the execution status of the function.
 */
osStatus_t osMessageQueuePut(osMessageQueueId_t mq_id, const void *msg_ptr, uint8_t msg_prio, uint32_t timeout)
{
  osStatus_t status;

  if (IsIrqMode() || IsIrqMasked()) {
    if (timeout != 0U) {
      status = osErrorParameter;
    }
    else {
      status = MessageQueuePut(mq_id, msg_ptr, msg_prio, timeout);
    }
  }
  else {
    status = (osStatus_t)svc_4((uint32_t)mq_id, (uint32_t)msg_ptr, msg_prio, timeout, (uint32_t)MessageQueuePut);
    if (status == osThreadWait) {
      status = (osStatus_t)ThreadGetRunning()->wait_info.ret_val;
    }
  }

  return (status);
}

/**
 * @fn          osStatus_t osMessageQueueGet(osMessageQueueId_t mq_id, void *msg_ptr, uint8_t *msg_prio, uint32_t timeout)
 * @brief       Get a Message from a Queue or timeout if Queue is empty.
 * @param[in]   mq_id     message queue ID obtained by \ref osMessageQueueNew.
 * @param[out]  msg_ptr   pointer to buffer for message to get from a queue.
 * @param[out]  msg_prio  pointer to buffer for message priority or NULL.
 * @param[in]   timeout   \ref CMSIS_RTOS_TimeOutValue or 0 in case of no time-out.
 * @return      status code that indicates the execution status of the function.
 */
osStatus_t osMessageQueueGet(osMessageQueueId_t mq_id, void *msg_ptr, uint8_t *msg_prio, uint32_t timeout)
{
  osStatus_t status;

  if (IsIrqMode() || IsIrqMasked()) {
    if (timeout != 0U) {
      status = osErrorParameter;
    }
    else {
      status = MessageQueueGet(mq_id, msg_ptr, msg_prio, timeout);
    }
  }
  else {
    status = (osStatus_t)svc_4((uint32_t)mq_id, (uint32_t)msg_ptr, (uint32_t)msg_prio, timeout, (uint32_t)MessageQueueGet);
    if (status == osThreadWait) {
      status = (osStatus_t)ThreadGetRunning()->wait_info.ret_val;
    }
  }

  return (status);
}

/**
 * @fn          uint32_t osMessageQueueGetCapacity(osMessageQueueId_t mq_id)
 * @brief       Get maximum number of messages in a Message Queue.
 * @param[in]   mq_id     message queue ID obtained by \ref osMessageQueueNew.
 * @return      maximum number of messages or 0 in case of an error.
 */
uint32_t osMessageQueueGetCapacity(osMessageQueueId_t mq_id)
{
  uint32_t capacity;

  if (IsIrqMode() || IsIrqMasked()) {
    capacity = MessageQueueGetCapacity(mq_id);
  }
  else {
    capacity = svc_1((uint32_t)mq_id, (uint32_t)MessageQueueGetCapacity);
  }

  return (capacity);
}

/**
 * @fn          uint32_t osMessageQueueGetMsgSize(osMessageQueueId_t mq_id)
 * @brief       Get maximum message size in bytes.
 * @param[in]   mq_id     message queue ID obtained by \ref osMessageQueueNew.
 * @return      maximum message size in bytes or 0 in case of an error.
 */
uint32_t osMessageQueueGetMsgSize(osMessageQueueId_t mq_id)
{
  uint32_t msg_size;

  if (IsIrqMode() || IsIrqMasked()) {
    msg_size = MessageQueueGetMsgSize(mq_id);
  }
  else {
    msg_size = svc_1((uint32_t)mq_id, (uint32_t)MessageQueueGetMsgSize);
  }

  return (msg_size);
}

/**
 * @fn          uint32_t osMessageQueueGetCount(osMessageQueueId_t mq_id)
 * @brief       Get number of queued messages in a Message Queue.
 * @param[in]   mq_id     message queue ID obtained by \ref osMessageQueueNew.
 * @return      number of queued messages or 0 in case of an error.
 */
uint32_t osMessageQueueGetCount(osMessageQueueId_t mq_id)
{
  uint32_t count;

  if (IsIrqMode() || IsIrqMasked()) {
    count = MessageQueueGetCount(mq_id);
  }
  else {
    count = svc_1((uint32_t)mq_id, (uint32_t)MessageQueueGetCount);
  }

  return (count);
}

/**
 * @fn          uint32_t osMessageQueueGetSpace(osMessageQueueId_t mq_id)
 * @brief       Get number of available slots for messages in a Message Queue.
 * @param[in]   mq_id     message queue ID obtained by \ref osMessageQueueNew.
 * @return      number of available slots for messages or 0 in case of an error.
 */
uint32_t osMessageQueueGetSpace(osMessageQueueId_t mq_id)
{
  uint32_t space;

  if (IsIrqMode() || IsIrqMasked()) {
    space = MessageQueueGetSpace(mq_id);
  }
  else {
    space = svc_1((uint32_t)mq_id, (uint32_t)MessageQueueGetSpace);
  }

  return (space);
}

/**
 * @fn          osStatus_t osMessageQueueReset(osMessageQueueId_t mq_id)
 * @brief       Reset a Message Queue to initial empty state.
 * @param[in]   mq_id     message queue ID obtained by \ref osMessageQueueNew.
 * @return      status code that indicates the execution status of the function.
 */
osStatus_t osMessageQueueReset(osMessageQueueId_t mq_id)
{
  osStatus_t status;

  if (IsIrqMode() || IsIrqMasked()) {
    status = osErrorISR;
  }
  else {
    status = (osStatus_t)svc_1((uint32_t)mq_id, (uint32_t)MessageQueueReset);
  }

  return (status);
}

/**
 * @fn          osStatus_t osMessageQueueDelete(osMessageQueueId_t mq_id)
 * @brief       Delete a Message Queue object.
 * @param[in]   mq_id     message queue ID obtained by \ref osMessageQueueNew.
 * @return      status code that indicates the execution status of the function.
 */
osStatus_t osMessageQueueDelete(osMessageQueueId_t mq_id)
{
  osStatus_t status;

  if (IsIrqMode() || IsIrqMasked()) {
    status = osErrorISR;
  }
  else {
    status = (osStatus_t)svc_1((uint32_t)mq_id, (uint32_t)MessageQueueDelete);
  }

  return (status);
}

/* ----------------------------- End of file ---------------------------------*/

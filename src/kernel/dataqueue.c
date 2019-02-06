/*
 * Copyright (C) 2019 Sergey Koshkin <koshkin.sergey@gmail.com>
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

static osMessage_t *MessagePut(osMessageQueue_t *mq, const void *msg_ptr, uint8_t msg_prio)
{
  queue_t     *que;
  osMessage_t *msg;

  /* Try to allocate memory */
  msg = _MemoryPoolAlloc(&mq->mp_info);
  if (msg != NULL) {
    /* Copy Message */
    memcpy(&msg[1], msg_ptr, mq->msg_size);
    msg->id = ID_MESSAGE;
    msg->flags = 0U;
    msg->priority = msg_prio;
    /* Put Message into Queue */
    que = &mq->msg_queue;
    if (msg_prio != 0U) {
      for (que = que->next; que != &mq->msg_queue; que = que->next) {
        if (GetMessageByQueue(que)->priority < msg_prio) {
          break;
        }
      }
    }
    QueueAddTail(que, &msg->msg_que);
    mq->msg_count++;
  }

  return (msg);
}

static osMessage_t *MessageGet(osMessageQueue_t *mq, void *msg_ptr, uint8_t *msg_prio)
{
  queue_t     *que;
  osMessage_t *msg;

  que = &mq->msg_queue;

  if (!isQueueEmpty(que)) {
    msg = GetMessageByQueue(QueueRemoveHead(que));
    /* Copy Message */
    memcpy(msg_ptr, &msg[1], mq->msg_size);
    if (msg_prio != NULL) {
      *msg_prio = msg->priority;
    }
    /* Free memory */
    msg->id = ID_INVALID;
    _MemoryPoolFree(&mq->mp_info, msg);
    mq->msg_count--;
  }
  else {
    msg = NULL;
  }

  return (msg);
}

static osDataQueueId_t DataQueueNew(uint32_t data_count, uint32_t data_size, const osDataQueueAttr_t *attr)
{
  osMessageQueue_t *mq;
  void             *mq_mem;
  uint32_t          mq_size;
  uint32_t          block_size;

  /* Check parameters */
  if ((msg_count == 0U) || (msg_size  == 0U) || (attr == NULL)) {
    return (NULL);
  }

  mq      = attr->cb_mem;
  mq_mem  = attr->mq_mem;
  mq_size = attr->mq_size;
  block_size = ((msg_size + 3U) & ~3UL) + sizeof(osMessage_t);

  /* Check parameters */
  if (((__CLZ(msg_count) + __CLZ(block_size)) < 32U) ||
      (mq == NULL) || (((uint32_t)mq & 3U) != 0U) || (attr->cb_size < sizeof(osMessageQueue_t)) ||
      (mq_mem == NULL) || (((uint32_t)mq_mem & 3U) != 0U) || (mq_size < (msg_count * block_size))) {
    return (NULL);
  }

  /* Initialize control block */
  mq->id = ID_MESSAGE_QUEUE;
  mq->flags = 0U;
  mq->name = attr->name;
  mq->msg_size = msg_size;
  mq->msg_count = 0U;
  QueueReset(&mq->wait_put_queue);
  QueueReset(&mq->wait_get_queue);
  QueueReset(&mq->msg_queue);
  _MemoryPoolInit(msg_count, block_size, mq_mem, &mq->mp_info);

  return (mq);
}

static const char *DataQueueGetName(osDataQueueId_t dq_id)
{
  osDataQueue_t *dq = dq_id;

  /* Check parameters */
  if ((dq == NULL) || (dq->id != ID_DATA_QUEUE)) {
    return (NULL);
  }

  return (dq->name);
}

static osStatus_t DataQueuePut(osDataQueueId_t dq_id, const void *data_ptr, uint32_t timeout)
{
  osDataQueue_t    *dq = dq_id;
  osMessage_t      *msg;
  osThread_t       *thread;
  winfo_msgque_t   *winfo;
  osStatus_t        status;

  /* Check parameters */
  if ((dq == NULL) || (dq->id != ID_DATA_QUEUE) || (data_ptr == NULL)) {
    return (osErrorParameter);
  }

  BEGIN_CRITICAL_SECTION

  /* Check if Thread is waiting to receive a Message */
  if (!isQueueEmpty(&dq->wait_get_queue)) {
    /* Wakeup waiting Thread with highest Priority */
    thread = GetThreadByQueue(QueueRemoveHead(&dq->wait_get_queue));
    _ThreadWaitExit(thread, (uint32_t)osOK);
    winfo = &thread->winfo.msgque;
    memcpy((void *)winfo->msg, data_ptr, dq->msg_size);
    if ((uint8_t *)winfo->msg_prio != NULL) {
      *((uint8_t *)winfo->msg_prio) = msg_prio;
    }
    status = osOK;
  }
  else {
    /* Try to put Message into Queue */
    msg = MessagePut(dq, data_ptr, msg_prio);
    if (msg != NULL) {
      status = osOK;
    }
    else {
      /* No memory available */
      if (timeout != 0U) {
        /* Suspend current Thread */
        thread = ThreadGetRunning();
        thread->winfo.ret_val = (uint32_t)osErrorTimeout;
        winfo = &thread->winfo.msgque;
        winfo->msg      = (uint32_t)data_ptr;
        winfo->msg_prio = (uint32_t)msg_prio;
        _ThreadWaitEnter(thread, &dq->wait_put_queue, timeout);
        status = osThreadWait;
      }
      else {
        status = osErrorResource;
      }
    }
  }

  END_CRITICAL_SECTION

  return (status);
}

static osStatus_t DataQueueGet(osDataQueueId_t dq_id, void *data_ptr, uint32_t timeout)
{
  osDataQueue_t    *dq = dq_id;
  osMessage_t      *msg;
  osThread_t       *thread;
  winfo_msgque_t   *winfo;
  osStatus_t        status;

  /* Check parameters */
  if ((dq == NULL) || (dq->id != ID_DATA_QUEUE) || (data_ptr == NULL)) {
    return (osErrorParameter);
  }

  BEGIN_CRITICAL_SECTION

  /* Get Message from Queue */
  msg = MessageGet(mq, data_ptr, msg_prio);

  if (msg != NULL) {
    /* Check if Thread is waiting to send a Message */
    if (!isQueueEmpty(&mq->wait_put_queue)) {
      /* Get waiting Thread with highest Priority */
      thread = GetThreadByQueue(mq->wait_put_queue.next);
      winfo = &thread->winfo.msgque;
      /* Try to put Message into Queue */
      msg = MessagePut(mq, (const void *)winfo->msg, (uint8_t)winfo->msg_prio);
      if (msg != NULL) {
        /* Wakeup waiting Thread with highest Priority */
        _ThreadWaitExit(thread, (uint32_t)osOK);
      }
    }
    status = osOK;
  }
  else {
    /* No Message available */
    if (timeout != 0U) {
      /* Suspend current Thread */
      thread = ThreadGetRunning();
      thread->winfo.ret_val = (uint32_t)osErrorTimeout;
      winfo = &thread->winfo.msgque;
      winfo->msg      = (uint32_t)data_ptr;
      winfo->msg_prio = (uint32_t)msg_prio;
      _ThreadWaitEnter(thread, &dq->wait_get_queue, timeout);
      status = osThreadWait;
    }
    else {
      status = osErrorResource;
    }
  }

  END_CRITICAL_SECTION

  return (status);
}

static uint32_t DataQueueGetCapacity(osDataQueueId_t dq_id)
{
  osDataQueue_t *dq = dq_id;

  /* Check parameters */
  if ((dq == NULL) || (dq->id != ID_DATA_QUEUE)) {
    return (0U);
  }

  return (dq->mp_info.max_blocks);
}

static uint32_t DataQueueGetMsgSize(osDataQueueId_t dq_id)
{
  osDataQueue_t *dq = dq_id;

  /* Check parameters */
  if ((dq == NULL) || (dq->id != ID_DATA_QUEUE)) {
    return (0U);
  }

  return (dq->data_size);
}

static uint32_t DataQueueGetCount(osDataQueueId_t dq_id)
{
  osDataQueue_t *dq = dq_id;

  /* Check parameters */
  if ((dq == NULL) || (dq->id != ID_DATA_QUEUE)) {
    return (0U);
  }

  return (dq->data_count);
}

static uint32_t DataQueueGetSpace(osDataQueueId_t dq_id)
{
  osDataQueue_t *dq = dq_id;

  /* Check parameters */
  if ((dq == NULL) || (dq->id != ID_DATA_QUEUE)) {
    return (0U);
  }

  return (dq->mp_info.max_blocks - dq->data_count);
}

static osStatus_t DataQueueReset(osDataQueueId_t dq_id)
{
  osDataQueue_t    *dq = dq_id;
  queue_t          *que;
  osMessage_t      *msg;
  osThread_t       *thread;
  winfo_msgque_t   *winfo;

  /* Check parameters */
  if ((dq == NULL) || (dq->id != ID_DATA_QUEUE)) {
    return (osErrorParameter);
  }

  BEGIN_CRITICAL_SECTION

  /* Remove Messages from Queue */
  dq->msg_count = 0U;
  QueueReset(&dq->msg_queue);
  _MemoryPoolReset(&dq->mp_info);

  /* Check if Threads are waiting to send Messages */
  for (que = dq->wait_put_queue.next; que != &dq->wait_put_queue; que = que->next) {
    /* Get waiting Thread with highest Priority */
    thread = GetThreadByQueue(que);
    winfo = &thread->winfo.msgque;
    /* Try to put Message into Queue */
    msg = MessagePut(dq, (const void *)winfo->msg, (uint8_t)winfo->msg_prio);
    if (msg == NULL) {
      break;
    }
    /* Wakeup waiting Thread with highest Priority */
    _ThreadWaitExit(thread, (uint32_t)osOK);
  }

  END_CRITICAL_SECTION

  return (osOK);
}

static osStatus_t DataQueueDelete(osDataQueueId_t dq_id)
{
  osDataQueue_t *dq = dq_id;

  /* Check parameters */
  if ((dq == NULL) || (dq->id != ID_DATA_QUEUE)) {
    return (osErrorParameter);
  }

  /* Unblock waiting threads */
  _ThreadWaitDelete(&dq->wait_put_queue);
  _ThreadWaitDelete(&dq->wait_get_queue);

  /* Mark object as invalid */
  dq->id = ID_INVALID;

  return (osOK);
}

/*******************************************************************************
 *  Public API
 ******************************************************************************/

/**
 * @fn          osDataQueueId_t osDataQueueNew(uint32_t data_count, uint32_t data_size, const osDataQueueAttr_t *attr)
 * @brief       Create and Initialize a Data Queue object.
 * @param[in]   data_count  maximum number of data in queue.
 * @param[in]   data_size   maximum data size in bytes.
 * @param[in]   attr        data queue attributes.
 * @return      data queue ID for reference by other functions or NULL in case of error.
 */
osDataQueueId_t osDataQueueNew(uint32_t data_count, uint32_t data_size, const osDataQueueAttr_t *attr)
{
  osDataQueueId_t dq_id;

  if (IsIrqMode() || IsIrqMasked()) {
    dq_id = NULL;
  }
  else {
    dq_id = (osDataQueueId_t)svc_3(data_count, data_size, (uint32_t)attr, (uint32_t)DataQueueNew);
  }

  return (dq_id);
}

/**
 * @fn          const char *osDataQueueGetName(osDataQueueId_t dq_id)
 * @brief       Get name of a Data Queue object.
 * @param[in]   dq_id   data queue ID obtained by \ref osDataQueueNew.
 * @return      name as null-terminated string or NULL in case of an error.
 */
const char *osDataQueueGetName(osDataQueueId_t dq_id)
{
  const char *name;

  if (IsIrqMode() || IsIrqMasked()) {
    name = NULL;
  }
  else {
    name = (const char *)svc_1((uint32_t)dq_id, (uint32_t)DataQueueGetName);
  }

  return (name);
}

/**
 * @fn          osStatus_t osDataQueuePut(osDataQueueId_t dq_id, const void *data_ptr, uint32_t timeout)
 * @brief       Put a Data into a Queue or timeout if Queue is full.
 * @param[in]   dq_id     data queue ID obtained by \ref osDataQueueNew.
 * @param[in]   data_ptr  pointer to buffer with data to put into a queue.
 * @param[in]   timeout   \ref CMSIS_RTOS_TimeOutValue or 0 in case of no time-out.
 * @return      status code that indicates the execution status of the function.
 */
osStatus_t osDataQueuePut(osDataQueueId_t dq_id, const void *data_ptr, uint32_t timeout)
{
  osStatus_t status;

  if (IsIrqMode() || IsIrqMasked()) {
    if (timeout != 0U) {
      status = osErrorParameter;
    }
    else {
      status = DataQueuePut(dq_id, data_ptr, timeout);
    }
  }
  else {
    status = (osStatus_t)svc_3((uint32_t)dq_id, (uint32_t)data_ptr, timeout, (uint32_t)DataQueuePut);
    if (status == osThreadWait) {
      status = (osStatus_t)ThreadGetRunning()->winfo.ret_val;
    }
  }

  return (status);
}

/**
 * @fn          osStatus_t osDataQueueGet(osDataQueueId_t dq_id, void *data_ptr, uint32_t timeout)
 * @brief       Get a Data from a Queue or timeout if Queue is empty.
 * @param[in]   dq_id     data queue ID obtained by \ref osDataQueueNew.
 * @param[out]  data_ptr  pointer to buffer for data to get from a queue.
 * @param[in]   timeout   \ref CMSIS_RTOS_TimeOutValue or 0 in case of no time-out.
 * @return      status code that indicates the execution status of the function.
 */
osStatus_t osDataQueueGet(osDataQueueId_t dq_id, void *data_ptr, uint32_t timeout)
{
  osStatus_t status;

  if (IsIrqMode() || IsIrqMasked()) {
    if (timeout != 0U) {
      status = osErrorParameter;
    }
    else {
      status = DataQueueGet(dq_id, data_ptr, timeout);
    }
  }
  else {
    status = (osStatus_t)svc_3((uint32_t)dq_id, (uint32_t)data_ptr, timeout, (uint32_t)DataQueueGet);
    if (status == osThreadWait) {
      status = (osStatus_t)ThreadGetRunning()->winfo.ret_val;
    }
  }

  return (status);
}

/**
 * @fn          uint32_t osDataQueueGetCapacity(osDataQueueId_t dq_id)
 * @brief       Get maximum number of data in a Data Queue.
 * @param[in]   dq_id   data queue ID obtained by \ref osDataQueueNew.
 * @return      maximum number of data or 0 in case of an error.
 */
uint32_t osDataQueueGetCapacity(osDataQueueId_t dq_id)
{
  uint32_t capacity;

  if (IsIrqMode() || IsIrqMasked()) {
    capacity = DataQueueGetCapacity(dq_id);
  }
  else {
    capacity = svc_1((uint32_t)dq_id, (uint32_t)DataQueueGetCapacity);
  }

  return (capacity);
}

/**
 * @fn          uint32_t osDataQueueGetDataSize(osDataQueueId_t dq_id)
 * @brief       Get maximum data size in bytes.
 * @param[in]   dq_id   data queue ID obtained by \ref osDataQueueNew.
 * @return      maximum data size in bytes or 0 in case of an error.
 */
uint32_t osDataQueueGetDataSize(osDataQueueId_t dq_id)
{
  uint32_t data_size;

  if (IsIrqMode() || IsIrqMasked()) {
    data_size = DataQueueGetMsgSize(dq_id);
  }
  else {
    data_size = svc_1((uint32_t)dq_id, (uint32_t)DataQueueGetMsgSize);
  }

  return (data_size);
}

/**
 * @fn          uint32_t osDataQueueGetCount(osDataQueueId_t dq_id)
 * @brief       Get number of queued data in a Data Queue.
 * @param[in]   dq_id   data queue ID obtained by \ref osDataQueueNew.
 * @return      number of queued data or 0 in case of an error.
 */
uint32_t osDataQueueGetCount(osDataQueueId_t dq_id)
{
  uint32_t count;

  if (IsIrqMode() || IsIrqMasked()) {
    count = DataQueueGetCount(dq_id);
  }
  else {
    count = svc_1((uint32_t)dq_id, (uint32_t)DataQueueGetCount);
  }

  return (count);
}

/**
 * @fn          uint32_t osDataQueueGetSpace(osDataQueueId_t dq_id)
 * @brief       Get number of available slots for data in a Data Queue.
 * @param[in]   dq_id   data queue ID obtained by \ref osDataQueueNew.
 * @return      number of available slots for data or 0 in case of an error.
 */
uint32_t osDataQueueGetSpace(osDataQueueId_t dq_id)
{
  uint32_t space;

  if (IsIrqMode() || IsIrqMasked()) {
    space = DataQueueGetSpace(dq_id);
  }
  else {
    space = svc_1((uint32_t)dq_id, (uint32_t)DataQueueGetSpace);
  }

  return (space);
}

/**
 * @fn          osStatus_t osDataQueueReset(osDataQueueId_t dq_id)
 * @brief       Reset a Data Queue to initial empty state.
 * @param[in]   dq_id   data queue ID obtained by \ref osDataQueueNew.
 * @return      status code that indicates the execution status of the function.
 */
osStatus_t osDataQueueReset(osDataQueueId_t dq_id)
{
  osStatus_t status;

  if (IsIrqMode() || IsIrqMasked()) {
    status = osErrorISR;
  }
  else {
    status = (osStatus_t)svc_1((uint32_t)dq_id, (uint32_t)DataQueueReset);
  }

  return (status);
}

/**
 * @fn          osStatus_t osDataQueueDelete(osDataQueueId_t dq_id)
 * @brief       Delete a Data Queue object.
 * @param[in]   dq_id   data queue ID obtained by \ref osDataQueueNew.
 * @return      status code that indicates the execution status of the function.
 */
osStatus_t osDataQueueDelete(osDataQueueId_t dq_id)
{
  osStatus_t status;

  if (IsIrqMode() || IsIrqMasked()) {
    status = osErrorISR;
  }
  else {
    status = (osStatus_t)svc_1((uint32_t)dq_id, (uint32_t)DataQueueDelete);
  }

  return (status);
}

/* ----------------------------- End of file ---------------------------------*/

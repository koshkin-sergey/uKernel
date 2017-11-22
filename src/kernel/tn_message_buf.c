/*******************************************************************************
 *
 * TNKernel real-time kernel
 *
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
 * Название : mbf_fifo_write
 * Описание : Записывает данные в циклический буфер
 * Параметры: mbf - Дескриптор буфера сообщений.
 *            msg - Указатель на данные.
 *            send_to_first - Флаг, указывающий, что необходимо поместить данные
 *                            в начало буфера.
 * Результат: Возвращает один из вариантов:
 *              TERR_NO_ERR - функция выполнена без ошибок;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_OUT_OF_MEM - Емкость буфера равна нулю.
 *-----------------------------------------------------------------------------*/
static
osError_t mbf_fifo_write(TN_MBF *mbf, void *msg, bool send_to_first)
{
  int bufsz, msz;

  if (mbf->num_entries == 0)
    return TERR_OUT_OF_MEM;

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
 * Параметры: mbf - Дескриптор буфера сообщений.
 *            msg - Указатель на место в памяти куда будут считаны данные.
 * Результат: Возвращает один из вариантов:
 *              TERR_NO_ERR - функция выполнена без ошибок;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_OUT_OF_MEM - Емкость буфера равна нулю.
 *-----------------------------------------------------------------------------*/
static
osError_t mbf_fifo_read(TN_MBF *mbf, void *msg)
{
  int bufsz, msz;

  if (mbf->num_entries == 0)
    return TERR_OUT_OF_MEM;

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

/*-----------------------------------------------------------------------------*
 * Название : do_mbf_send
 * Описание : Помещает данные в буфер сообщений за установленный интервал
 *            времени.
 * Параметры: mbf - Дескриптор буфера сообщений.
 *            msg - Указатель на данные.
 *            timeout - Время, в течении которого данные должны быть помещены
 *                      в буфер.
 *            send_to_first - Флаг, указывающий, что необходимо поместить данные
 *                            в начало буфера.
 * Результат: Возвращает один из вариантов:
 *              TERR_NO_ERR - функция выполнена без ошибок;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_NOEXS  - буфер не существует;
 *              TERR_TIMEOUT  - Превышен заданный интервал времени;
 *----------------------------------------------------------------------------*/
static osError_t do_mbf_send(TN_MBF *mbf, void *msg, unsigned long timeout,
                       bool send_to_first)
{
  osError_t rc = TERR_NO_ERR;
  CDLL_QUEUE *que;
  TN_TCB *task;

  if (mbf == NULL)
    return TERR_WRONG_PARAM;
  if (mbf->id_mbf != TN_ID_MESSAGEBUF)
    return TERR_NOEXS;

  BEGIN_CRITICAL_SECTION

  //-- there are task(s) in the data queue's wait_receive list

  if (!isQueueEmpty(&mbf->recv_queue)) {
    que = QueueRemoveHead(&mbf->recv_queue);
    task = get_task_by_tsk_queue(que);
    tn_memcpy(task->winfo.rmbf.msg, msg, mbf->msz);
    ThreadWaitComplete(task);
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
        task = ThreadGetCurrent();
        task->wercd = &rc;
        task->winfo.smbf.msg = msg;
        task->winfo.smbf.send_to_first = send_to_first;
        ThreadToWaitAction(task, &mbf->send_queue, TSK_WAIT_REASON_MBF_WSEND,
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
 * Название : tn_mbf_create
 * Описание : Создает буфер сообщений.
 * Параметры: mbf - Указатель на существующую структуру TN_MBF.
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
osError_t tn_mbf_create(TN_MBF *mbf, void *buf, int bufsz, int msz)
{
  if (mbf == NULL)
    return TERR_WRONG_PARAM;
  if (bufsz < 0 || msz <= 0 || mbf->id_mbf == TN_ID_MESSAGEBUF)
    return TERR_WRONG_PARAM;

  BEGIN_CRITICAL_SECTION

  QueueReset(&mbf->send_queue);
  QueueReset(&mbf->recv_queue);

  mbf->buf = buf;
  mbf->msz = msz;
  mbf->num_entries = bufsz / msz;
  mbf->cnt = mbf->head = mbf->tail = 0;

  mbf->id_mbf = TN_ID_MESSAGEBUF;

  END_CRITICAL_SECTION

  return TERR_NO_ERR;
}

/*-----------------------------------------------------------------------------*
 * Название : tn_mbf_delete
 * Описание : Удаляет буфер сообщений.
 * Параметры: mbf - Указатель на существующую структуру TN_MBF.
 * Результат: Возвращает один из вариантов:
 *              TERR_NO_ERR - функция выполнена без ошибок;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_NOEXS  - буфер сообщений не существует;
 *----------------------------------------------------------------------------*/
osError_t tn_mbf_delete(TN_MBF *mbf)
{
  if (mbf == NULL)
    return TERR_WRONG_PARAM;
  if (mbf->id_mbf == 0)
    return TERR_NOEXS;

  BEGIN_CRITICAL_SECTION

  ThreadWaitDelete(&mbf->send_queue);
  ThreadWaitDelete(&mbf->recv_queue);

  mbf->id_mbf = 0;

  END_CRITICAL_SECTION

  return TERR_NO_ERR;
}

/*-----------------------------------------------------------------------------*
 * Название : tn_mbf_send
 * Описание : Помещает данные в конец буфера сообщений за установленный
 *            интервал времени.
 * Параметры: mbf - Дескриптор буфера сообщений.
 *            msg - Указатель на данные.
 *            timeout - Время, в течении которого данные должны быть помещены
 *                      в буфер.
 * Результат: Возвращает один из вариантов:
 *              TERR_NO_ERR - функция выполнена без ошибок;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_NOEXS  - буфер не существует;
 *              TERR_TIMEOUT  - Превышен заданный интервал времени;
 *----------------------------------------------------------------------------*/
osError_t tn_mbf_send(TN_MBF *mbf, void *msg, unsigned long timeout)
{
  return do_mbf_send(mbf, msg, timeout, false);
}

/*-----------------------------------------------------------------------------*
 * Название : tn_mbf_send_first
 * Описание : Помещает данные в начало буфера сообщений за установленный
 *            интервал времени.
 * Параметры: mbf - Дескриптор буфера сообщений.
 *            msg - Указатель на данные.
 *            timeout - Время, в течении которого данные должны быть помещены
 *                      в буфер.
 * Результат: Возвращает один из вариантов:
 *              TERR_NO_ERR - функция выполнена без ошибок;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_NOEXS  - буфер не существует;
 *              TERR_TIMEOUT  - Превышен заданный интервал времени;
 *----------------------------------------------------------------------------*/
osError_t tn_mbf_send_first(TN_MBF *mbf, void *msg, unsigned long timeout)
{
  return do_mbf_send(mbf, msg, timeout, true);
}

/*-----------------------------------------------------------------------------*
 * Название : tn_mbf_receive
 * Описание : Считывает один элемент данных из начала буфера сообщений
 *            за установленный интервал времени.
 * Параметры: mbf - Дескриптор буфера сообщений.
 *            msg - Указатель на буфер, куда будут считаны данные.
 *            timeout - Время, в течении которого данные должны быть считаны.
 * Результат: Возвращает один из вариантов:
 *              TERR_NO_ERR - функция выполнена без ошибок;
 *              TERR_WRONG_PARAM  - некорректно заданы параметры;
 *              TERR_NOEXS  - буфер не существует;
 *              TERR_TIMEOUT  - Превышен заданный интервал времени;
 *----------------------------------------------------------------------------*/
osError_t tn_mbf_receive(TN_MBF *mbf, void *msg, unsigned long timeout)
{
  osError_t rc; //-- return code
  CDLL_QUEUE *que;
  TN_TCB *task;

  if (mbf == NULL || msg == NULL)
    return TERR_WRONG_PARAM;
  if (mbf->id_mbf != TN_ID_MESSAGEBUF)
    return TERR_NOEXS;

  BEGIN_CRITICAL_SECTION

  rc = mbf_fifo_read(mbf, msg);
  if (rc == TERR_NO_ERR) {  //-- There was entry(s) in data queue
    if (!isQueueEmpty(&mbf->send_queue)) {
      que = QueueRemoveHead(&mbf->send_queue);
      task = get_task_by_tsk_queue(que);
      mbf_fifo_write(mbf, task->winfo.smbf.msg, task->winfo.smbf.send_to_first);
      ThreadWaitComplete(task);
    }
  }
  else {  //-- data FIFO is empty
    if (!isQueueEmpty(&mbf->send_queue)) {
      que = QueueRemoveHead(&mbf->send_queue);
      task = get_task_by_tsk_queue(que);
      tn_memcpy(msg, task->winfo.smbf.msg, mbf->msz);
      ThreadWaitComplete(task);
      rc = TERR_NO_ERR;
    }
    else {  //-- wait_send_list is empty
      if (timeout == TN_POLLING) {
        rc = TERR_TIMEOUT;
      }
      else {
        task = ThreadGetCurrent();
        task->wercd = &rc;
        task->winfo.rmbf.msg = msg;
        ThreadToWaitAction(task, &mbf->recv_queue, TSK_WAIT_REASON_MBF_WRECEIVE,
                            timeout);
      }
    }
  }

  END_CRITICAL_SECTION

  return rc;
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
osError_t tn_mbf_flush(TN_MBF *mbf)
{
  if (mbf == NULL)
    return TERR_WRONG_PARAM;
  if (mbf->id_mbf != TN_ID_MESSAGEBUF)
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
osError_t tn_mbf_empty(TN_MBF *mbf)
{
  osError_t rc;

  if (mbf == NULL)
    return TERR_WRONG_PARAM;
  if (mbf->id_mbf != TN_ID_MESSAGEBUF)
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
osError_t tn_mbf_full(TN_MBF *mbf)
{
  osError_t rc;

  if (mbf == NULL)
    return TERR_WRONG_PARAM;
  if (mbf->id_mbf != TN_ID_MESSAGEBUF)
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
osError_t tn_mbf_cnt(TN_MBF *mbf, int *cnt)
{
  if (mbf == NULL || cnt == NULL)
    return TERR_WRONG_PARAM;
  if (mbf->id_mbf != TN_ID_MESSAGEBUF)
    return TERR_NOEXS;

  BEGIN_CRITICAL_SECTION

  *cnt = mbf->cnt;

  END_CRITICAL_SECTION

  return TERR_NO_ERR;
}

/* ----------------------------- End of file ---------------------------------*/

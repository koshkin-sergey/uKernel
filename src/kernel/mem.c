/*
 * Copyright (C) 2017-2019 Sergey Koshkin <koshkin.sergey@gmail.com>
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

//----------------------------------------------------------------------------
static void* fm_get(TN_FMP *fmp)
{
  void *p_tmp;

  if (fmp->fblkcnt > 0) {
    p_tmp = fmp->free_list;
    fmp->free_list = *(void **)fmp->free_list;   //-- ptr - to new free list
    fmp->fblkcnt--;

    return p_tmp;
  }

  return NULL;
}

//----------------------------------------------------------------------------
static int fm_put(TN_FMP *fmp, void *mem)
{
  if (fmp->fblkcnt < fmp->num_blocks) {
    *(void **)mem  = fmp->free_list;   //-- insert block into free block list
    fmp->free_list = mem;
    fmp->fblkcnt++;

    return TERR_NO_ERR;
  }

  return TERR_OVERFLOW;
}

/*******************************************************************************
 *  function implementations (scope: module-exported)
 ******************************************************************************/

//----------------------------------------------------------------------------
//  Structure's field fmp->id_id_fmp have to be set to 0
//----------------------------------------------------------------------------
osError_t tn_fmem_create(TN_FMP *fmp, void *start_addr, unsigned int block_size, int num_blocks)
{
  void **p_tmp;
  unsigned char *p_block;
  unsigned long i,j;

  if (fmp == NULL)
    return TERR_WRONG_PARAM;
  if (fmp->id == ID_FSMEMORYPOOL)
    return TERR_WRONG_PARAM;

  if (start_addr == NULL || num_blocks < 2 || block_size < sizeof(int)) {
    fmp->fblkcnt = 0;
    fmp->num_blocks = 0;
    fmp->id = ID_INVALID;
    fmp->free_list = NULL;
    return TERR_WRONG_PARAM;
  }

  QueueReset(&fmp->wait_queue);

  //-- Prepare addr/block aligment
  i = ((unsigned long)start_addr + (TN_ALIG -1)) & (~(TN_ALIG-1));
  fmp->start_addr  = (void*)i;
  fmp->block_size = (block_size + (TN_ALIG -1)) & (~(TN_ALIG-1));

  i = (unsigned long)start_addr + block_size * num_blocks;
  j = (unsigned long)fmp->start_addr + fmp->block_size * num_blocks;

  fmp->num_blocks = num_blocks;

  while (j > i) { //-- Get actual num_blocks
    j -= fmp->block_size;
    fmp->num_blocks--;
  }

  if (fmp->num_blocks < 2) {
    fmp->fblkcnt    = 0;
    fmp->num_blocks = 0;
    fmp->free_list  = NULL;
    return TERR_WRONG_PARAM;
  }

  //-- Set blocks ptrs for allocation -------
  p_tmp = (void **)fmp->start_addr;
  p_block  = (unsigned char *)fmp->start_addr + fmp->block_size;
  for (i = 0; i < (fmp->num_blocks - 1); i++) {
    *p_tmp  = (void *)p_block;  //-- contents of cell = addr of next block
    p_tmp   = (void **)p_block;
    p_block += fmp->block_size;
  }
  *p_tmp = NULL;          //-- Last memory block first cell contents -  NULL

  fmp->free_list = fmp->start_addr;
  fmp->fblkcnt   = fmp->num_blocks;

  fmp->id = ID_FSMEMORYPOOL;

  return TERR_NO_ERR;
}

//----------------------------------------------------------------------------
osError_t tn_fmem_delete(TN_FMP *fmp)
{
  if (fmp == NULL)
    return TERR_WRONG_PARAM;
  if (fmp->id != ID_FSMEMORYPOOL)
    return TERR_NOEXS;

  BEGIN_CRITICAL_SECTION

  _ThreadWaitDelete(&fmp->wait_queue);

  fmp->id = ID_INVALID;   //-- Fixed-size memory pool not exists now

  END_CRITICAL_SECTION

  return TERR_NO_ERR;
}

//----------------------------------------------------------------------------
osError_t tn_fmem_get(TN_FMP *fmp, void **p_data, unsigned long timeout)
{
  osError_t rc = TERR_NO_ERR;
  void * ptr;
  osThread_t *task;

  if (fmp == NULL || p_data == NULL)
    return  TERR_WRONG_PARAM;
  if (fmp->id != ID_FSMEMORYPOOL)
    return TERR_NOEXS;

  BEGIN_CRITICAL_SECTION

  ptr = fm_get(fmp);
  if (ptr != NULL)  //-- Get memory
    *p_data = ptr;
  else {
    if (timeout == 0U)
      rc = TERR_TIMEOUT;
    else {
      task = ThreadGetRunning();
      _ThreadWaitEnter(task, &fmp->wait_queue, timeout);

      END_CRITICAL_SECTION

      //-- When returns to this point, in the 'data_elem' have to be valid value
      *p_data = task->wait_info.fmem.data_elem; //-- Return to caller
    }
  }

  END_CRITICAL_SECTION

  return rc;
}

//----------------------------------------------------------------------------
osError_t tn_fmem_release(TN_FMP *fmp,void *p_data)
{
  queue_t * que;
  osThread_t * task;

  if (fmp == NULL || p_data == NULL)
    return  TERR_WRONG_PARAM;
  if (fmp->id != ID_FSMEMORYPOOL)
    return TERR_NOEXS;

  BEGIN_CRITICAL_SECTION

  if (!isQueueEmpty(&fmp->wait_queue)) {
    que = QueueRemoveHead(&fmp->wait_queue);
    task = GetTaskByQueue(que);
    task->wait_info.fmem.data_elem = p_data;
    _ThreadWaitExit(task, (uint32_t)TERR_NO_ERR);
  }
  else
    fm_put(fmp,p_data);

  END_CRITICAL_SECTION

  return  TERR_NO_ERR;
}

/*------------------------------ End of file ---------------------------------*/

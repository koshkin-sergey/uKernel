/*******************************************************************************
 *
 * TNKernel real-time kernel
 *
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

#ifndef TN_PORT_ARM_H_
#define TN_PORT_ARM_H_

/*******************************************************************************
 *  includes
 ******************************************************************************/

/*******************************************************************************
 *  defines and macros (scope: module-local)
 ******************************************************************************/

#define USE_ASM_FFS

/* - Interrupt processing - processor specific -------------------------------*/
#define BEGIN_CRITICAL_SECTION  int tn_save_status_reg = tn_cpu_save_sr();
#define END_CRITICAL_SECTION    tn_cpu_restore_sr(tn_save_status_reg);  \
                                if (!tn_inside_irq())                   \
                                  tn_switch_context();

#define BEGIN_DISABLE_INTERRUPT int tn_save_status_reg = tn_cpu_save_sr();
#define END_DISABLE_INTERRUPT   tn_cpu_restore_sr(tn_save_status_reg);

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
extern "C"  {
#endif

extern void tn_switch_context(void);
extern void tn_cpu_irq_handler(void);
extern void tn_cpu_irq_isr(void);
extern void tn_cpu_fiq_isr(void);
extern int ffs_asm(unsigned int val);
extern unsigned int* tn_stack_init(void *task_func, unsigned int *stack_start, void *param);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* TN_PORT_ARM_H_ */

/* ----------------------------- End of file ---------------------------------*/

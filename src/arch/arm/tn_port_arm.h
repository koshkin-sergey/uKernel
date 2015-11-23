/*
 * Copyright (C) 2015 Sergey Koshkin <koshkin.sergey@gmail.com>
 * All rights reserved
 *
 * File Name  :	tn_port_arm.h
 * Description:	tnkernel
 */

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

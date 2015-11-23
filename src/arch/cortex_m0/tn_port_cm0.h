/*
 * Copyright (C) 2015 Sergey Koshkin <koshkin.sergey@gmail.com>
 * All rights reserved
 *
 * File Name  :	tn_port_cm0.h
 * Description:	tnkernel
 */

#ifndef TN_PORT_CM0_H_
#define TN_PORT_CM0_H_

/*******************************************************************************
 *  includes
 ******************************************************************************/

/*******************************************************************************
 *  defines and macros (scope: module-local)
 ******************************************************************************/

/* - Assembler functions prototypes ------------------------------------------*/
#define  tn_switch_context()

/* - Interrupt processing - processor specific -------------------------------*/
#define BEGIN_DISABLE_INTERRUPT int tn_save_status_reg = tn_cpu_save_sr();
#define END_DISABLE_INTERRUPT   tn_cpu_restore_sr(tn_save_status_reg);

#define BEGIN_CRITICAL_SECTION  BEGIN_DISABLE_INTERRUPT
#define END_CRITICAL_SECTION    END_DISABLE_INTERRUPT

/*******************************************************************************
 *  typedefs and structures (scope: module-local)
 ******************************************************************************/

/*******************************************************************************
 *  exported variables
 ******************************************************************************/

/*******************************************************************************
 *  exported function prototypes
 ******************************************************************************/

extern unsigned int* tn_stack_init(void *task_func, unsigned int *stack_start, void *param);

#endif /* TN_PORT_CM0_H_ */

/* ----------------------------- End of file ---------------------------------*/

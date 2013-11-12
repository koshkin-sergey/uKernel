/******************** Copyright (c) 2013. All rights reserved ******************
 * File Name  :	tn_winfo.h
 * Author     :	Сергей
 * Version    :	1.00
 * Date       :	30.09.2013
 * Description:	TNKernel
 *
 * This software is supplied "AS IS" without warranties of any kind.
 *
 ******************************************************************************/

#ifndef TN_WINFO_H_
#define TN_WINFO_H_

/*******************************************************************************
 *  includes
 ******************************************************************************/
#include "tn_typedef.h"

/*******************************************************************************
 *  defines and macros (scope: module-local)
 ******************************************************************************/

/*******************************************************************************
 *  typedefs and structures (scope: module-local)
 ******************************************************************************/
typedef struct {
	void **data_elem;
} WINFO_RDQUE;

typedef struct {
	void *data_elem;
	BOOL send_to_first;
} WINFO_SDQUE;

typedef struct {
	void *data_elem;
} WINFO_FMEM;

/*
 * Message buffer receive/send wait (TTW_RMBF, TTW_SMBF)
 */
typedef struct {
	void *msg; /* Address that has a received message */
} WINFO_RMBF;

typedef struct {
	void *msg; /* Send message head address */
	BOOL send_to_first;
} WINFO_SMBF;

typedef struct {
	unsigned int pattern;		// Event wait pattern
	int mode;       				// Event wait mode:  _AND or _OR
	unsigned int *flags_pattern;
} WINFO_EVENT;

/*
 * Definition of wait information in task control block
 */
typedef union {
	WINFO_RDQUE rdque;
	WINFO_SDQUE sdque;
	WINFO_RMBF rmbf;
	WINFO_SMBF smbf;
	WINFO_FMEM fmem;
	WINFO_EVENT event;
} WINFO;

/*******************************************************************************
 *  exported variables
 ******************************************************************************/

/*******************************************************************************
 *  exported function prototypes
 ******************************************************************************/

#endif /* TN_WINFO_H_ */

/* ----------------------------- End of file ---------------------------------*/

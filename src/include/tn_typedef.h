/******************** Copyright (c) 2013. All rights reserved ******************
 * File Name  :	tn_typedef.h
 * Author     :	Сергей
 * Version    :	1.00
 * Date       :	01.10.2013
 * Description:	TNKernel
 *
 * This software is supplied "AS IS" without warranties of any kind.
 *
 ******************************************************************************/

#ifndef TN_TYPEDEF_H_
#define TN_TYPEDEF_H_

/*******************************************************************************
 *  includes
 ******************************************************************************/

/*******************************************************************************
 *  defines and macros (scope: module-local)
 ******************************************************************************/
#ifndef BOOL
  #ifndef __cplusplus /* In C++, 'bool', 'true' and 'false' and keywords */
    #define BOOL    _Bool
    #define TRUE    1
    #define FALSE   0
  #else
    #ifdef __GNUC__
      /* GNU C++ supports direct inclusion of stdbool.h to provide C99
         compatibility by defining _Bool */
      #define _Bool bool
    #endif
  #endif
#endif

#ifdef NULL
#undef NULL
#endif
#define NULL                  ((void*)0)

/*******************************************************************************
 *  typedefs and structures (scope: module-local)
 ******************************************************************************/

/*******************************************************************************
 *  exported variables
 ******************************************************************************/

/*******************************************************************************
 *  exported function prototypes
 ******************************************************************************/


#endif /* TN_TYPEDEF_H_ */

/* ----------------------------- End of file ---------------------------------*/

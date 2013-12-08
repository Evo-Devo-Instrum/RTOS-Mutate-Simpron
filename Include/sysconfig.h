/******************************************************************************
Filename    : sysconfig.h
Author      : pry 
Date        : 22/07/2012
Description : The Configuration Of The Operating System Is Logged Here.
              These "#define"s Are Not Included In The "defines.h".
******************************************************************************/

/* Includes ******************************************************************/
#include "MCS51_registers.h"
#include "MCS51_ints.h"
#include "MCS51_typedefs.h"
#include "MCS51_defines.h"
#include "MCS51_externs.h"
/* End Includes **************************************************************/

/* Preprocessor Control ******************************************************/
#ifndef _SYSCONFIG_H_
#define _SYSCONFIG_H_

/* Basic Configuration *******************************************************/
/* Keywords */               
#define EXTERN                      extern
/* Configuration choice */
#define TRUE                        1
#define FALSE                       0
/* Basic types */
#define DEFINE_BASIC_TYPES		    FALSE
/* End Basic Configuration ***************************************************/                                                     

/* Kernel Configuration ******************************************************/
/* Stacks */
#define KERNEL_STACK_SIZE           30
#define APP_STACK_1_SIZE            10
#define APP_STACK_2_SIZE            10

/* Threads/Tasks */
#define MAX_THREADS                 3                 
#define MAX_STACK_DEP               10                         
/* End Kernel Configuration **************************************************/

/* Memory Management Configuration *******************************************/
/* Memory */
#define ENABLE_MEMM      	        TRUE
#define DMEM_SIZE			        800
#define DMEM_PAGES                  40
/* End Memory Manegement Configuration ***************************************/

/* _SYSCONFIG_H_ */
#endif
/* End Preprocessor Control **************************************************/

/* End Of File ***************************************************************/

/* Copyright (C) 2011-2013 Evo-Devo Instrum. All rights reserved *************/

/****************************************************************************
 *
 * Copyright (c) 2001-2014
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 *---------------------------------------------------------------------------
 *
 * Description: NVM layout declarations SerialAPIPlus (host application part)
 *
 * Author:   Erik Friis Harck
 *
 * Last Changed By:  $Author: tro $
 * Revision:         $Revision: 23236 $
 * Last Changed:     $Date: 2012-08-17 14:04:49 +0200 (fr, 17 aug 2012) $
 *
 ****************************************************************************/
#ifndef _NVMHOST_H_
#define _NVMHOST_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include "ZW_nvm_descriptor.h"

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

/* NVM allocation definitions */

#if defined(ZW_CONTROLLER) || defined(ZW_SLAVE_32)
/* NVM is 16KB, 32KB or even more (you decide the size of your SPI EEPROM or FLASH chip) */
/* Use only a reasonable amount of it for host application */
#define NVM_SERIALAPI_HOST_SIZE 2048
#else
/* For routing slaves the total number of NVM data bytes available is 254 Bytes */
#define NVM_SERIALAPI_HOST_SIZE 16

#endif

/* Note from Keil knowledgebase: http://www.keil.com/support/docs/901.htm   */
/* "The order is not necessarily taken from the variable declarations, but  */
/* the first use of the variables."                                         */
/* Therefore, when using #pragma ORDER to order variables, declare them in  */
/* the order they should be in a collection. And none of them may be        */
/* declared or known in any way from other header files.                    */

/* NVM layout SerialAPIPlus (host application part) */
#ifndef slave_routing
extern t_NvmModuleSize far nvmHostApplicationSize;
#endif

extern BYTE far EEOFFSET_HOST_OFFSET_START_far[NVM_SERIALAPI_HOST_SIZE];

#ifndef slave_routing
extern t_nvmModuleDescriptor far nvmHostApplicationDescriptor;
#endif

/* The starting address of the segment ?FD?NVMHOST (to be used as a constant as (WORD)&_FD_NVMHOST_S_) */
extern unsigned char _FD_NVMHOST_S_;
/* The length of the segment ?FD?NVMHOST in bytes (to be used as a constant as (WORD)&_FD_NVMHOST_L_) */
extern unsigned char _FD_NVMHOST_L_;

#endif /* _NVMHOST_H_ */


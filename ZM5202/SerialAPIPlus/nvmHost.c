/****************************************************************************
 *
 * Copyright (c) 2001-2014
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 *---------------------------------------------------------------------------
 *
 * Description: NVM layout definitions SerialAPIPlus (host application part)
 *
 * Author:  Erik Friis Harck
 *
 * Last Changed By:  $Author: efh $
 * Revision:         $Revision: 11509 $
 * Last Changed:     $Date: 2012-03-05 10:45:45 +0200 (Wed, 5 Mar 2012) $
 *
 ****************************************************************************/
#pragma USERCLASS(CONST=NVM)

/* Make sure compiler won't shuffle around with these variables,            */
/* as there are external dependencies.                                      */
/* Note from Keil C51 manual:                                               */
/* Variables with memory type, initilalization, and without initilalization */
/* have all different tables. Therefore only variables with the same        */
/* attributes are kept within order.                                        */
/* Note from Keil knowledgebase: http://www.keil.com/support/docs/901.htm   */
/* "The order is not necessarily taken from the variable declarations, but  */
/* the first use of the variables."                                         */
/* Therefore, when using #pragma ORDER to order variables, declare them in  */
/* the order they should be in a collection. And none of them may be        */
/* declared or known in any way from other header files.                    */
#pragma ORDER

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include "ZW_basis_api.h"
#include "nvmHost.h"
#include "ZW_nvm_descriptor.h"

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

/************************************/
/*      NVM variable definition     */
/************************************/

/*--------------------------------------------------------------------------*/
/* NVM layout SerialAPIPlus (host application part) (as in t_nvmModule)     */
/* (begin)                                                                  */

#ifndef slave_routing
/* Offset from &nvmModule where nvmModuleDescriptor structure is placed */
t_NvmModuleSize far nvmHostApplicationSize = (t_NvmModuleSize)&_FD_NVMHOST_L_;
#endif

/* Host application non volatile variables */
BYTE far EEOFFSET_HOST_OFFSET_START_far[NVM_SERIALAPI_HOST_SIZE];

#ifndef slave_routing
/* NVM module descriptor for module. Located at the end of NVM module.      */
/* During the initialization phase, the NVM still contains the NVM contents */
/* from the old version of the firmware.                                    */
t_nvmModuleDescriptor far nvmHostApplicationDescriptor =
{
  (t_NvmModuleSize)&_FD_NVMHOST_L_,     /* t_NvmModuleSize wNvmModuleSize   */
  NVM_MODULE_TYPE_HOST_APPLICATION,     /* eNvmModuleType bNvmModuleType    */
  (WORD)&_APP_VERSION_                  /* WORD wNvmModuleVersion           */
};

/* NVM layout SerialAPIPlus (as in t_nvmModule) (end)                       */
/*--------------------------------------------------------------------------*/

/* NVM module update descriptor for this new version of firmware. Located   */
/* in code space.                                                           */
const t_nvmModuleUpdate code nvmHostApplicationUpdate =
{
  (p_nvmModule)&nvmHostApplicationSize, /* nvmModulePtr                     */
  /* nvmHostApplicationDescriptor is the first new NVM variable since devkit_6_5x_branch */
  (t_NvmModuleSize)((WORD)&nvmHostApplicationDescriptor), /* wNvmModuleSizeOld */
  {
    (t_NvmModuleSize)&_FD_NVMHOST_L_,   /* t_NvmModuleSize wNvmModuleSize   */
    NVM_MODULE_TYPE_HOST_APPLICATION,   /* eNvmModuleType bNvmModuleType    */
    (WORD)&_APP_VERSION_                /* WORD wNvmModuleVersion           */
  }
};
#endif


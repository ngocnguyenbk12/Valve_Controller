/****************************************************************************
 *
 * Copyright (c) 2001-2014
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 *---------------------------------------------------------------------------
 *
 * Description: NVM layout definitions SerialAPIPlus (embedded application part)
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
#include "eeprom.h"
#include <ZW_nvm_descriptor.h>

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

/************************************/
/*      NVM variable definition     */
/************************************/

/*--------------------------------------------------------------------------*/
/* NVM layout SerialAPIPlus (embedded application part) (as in t_nvmModule) */
/* (begin)                                                                  */

#ifndef slave_routing
/* Offset from &nvmModule where nvmModuleDescriptor structure is placed */
t_NvmModuleSize far nvmApplicationSize = (t_NvmModuleSize)&_FD_EEPROM_L_;
#endif

/* NVM variables for your application                                       */
BYTE far EEOFFSET_MAGIC_far;
#ifdef ZW_SLAVE
BYTE far EEOFFSET_LISTENING_far;
BYTE far EEOFFSET_GENERIC_far;
BYTE far EEOFFSET_SPECIFIC_far;
BYTE far EEOFFSET_CMDCLASS_LEN_far;
#else
BYTE far EEOFFSET_CMDCLASS_LEN_far;
#endif
BYTE far EEOFFSET_CMDCLASS_far[APPL_NODEPARM_MAX];
BYTE far EEOFFSET_WATCHDOG_STARTED_far;
#ifdef ZW_SECURITY_PROTOCOL
BYTE far EEOFFSET_CMDCLASS_UNINCLUDED_LEN_far;
BYTE far EEOFFSET_CMDCLASS_UNINCLUDED_far[APPL_NODEPARM_MAX];
BYTE far EEOFFSET_CMDCLASS_INCLUDED_UNSECURE_LEN_far;
BYTE far EEOFFSET_CMDCLASS_INCLUDED_UNSECURE_far[APPL_NODEPARM_MAX];
BYTE far EEOFFSET_CMDCLASS_INCLUDED_SECURE_LEN_far;
BYTE far EEOFFSET_CMDCLASS_INCLUDED_SECURE_far[APPL_NODEPARM_MAX];
#endif /* ZW_SECURITY_PROTOCOL */
#ifndef slave_routing
BYTE far EEOFFSET_POWERLEVEL_NORMAL_far[POWERLEVEL_CHANNELS];
BYTE far EEOFFSET_POWERLEVEL_LOW_far[POWERLEVEL_CHANNELS];
#endif
#ifdef ZW_SMARTSTART_ENABLED
BYTE far EEOFFSET_MODULE_POWER_MODE_EXTINT_ENABLE_far;
BYTE far EEOFFSET_MODULE_POWER_MODE_far;
BYTE far EEOFFSET_MODULE_POWER_MODE_WUT_TIMEOUT_far[sizeof(DWORD)];
#endif

#ifndef slave_routing
/* NVM module descriptor for module. Located at the end of NVM module.      */
/* During the initialization phase, the NVM still contains the NVM contents */
/* from the old version of the firmware.                                    */
t_nvmModuleDescriptor far nvmApplicationDescriptor =
{
  (t_NvmModuleSize)&_FD_EEPROM_L_,      /* t_NvmModuleSize wNvmModuleSize   */
  NVM_MODULE_TYPE_APPLICATION,          /* eNvmModuleType bNvmModuleType    */
  (WORD)&_APP_VERSION_                  /* WORD wNvmModuleVersion           */
};

/* NVM layout SerialAPIPlus (as in t_nvmModule) (end)                       */
/*--------------------------------------------------------------------------*/

/* NVM module update descriptor for this new version of firmware. Located   */
/* in code space.                                                           */
const t_nvmModuleUpdate code nvmApplicationUpdate =
{
  (p_nvmModule)&nvmApplicationSize,     /* nvmModulePtr                     */
  /* nvmApplicationDescriptor is the first new NVM variable since devkit_6_5x_branch */
  (t_NvmModuleSize)((WORD)&nvmApplicationDescriptor),  /* wNvmModuleSizeOld */
  {
    (t_NvmModuleSize)&_FD_EEPROM_L_,    /* t_NvmModuleSize wNvmModuleSize   */
    NVM_MODULE_TYPE_APPLICATION,        /* eNvmModuleType bNvmModuleType    */
    (WORD)&_APP_VERSION_                /* WORD wNvmModuleVersion           */
  }
};
#endif


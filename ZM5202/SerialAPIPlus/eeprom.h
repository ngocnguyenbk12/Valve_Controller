/****************************************************************************
 *
 * Copyright (c) 2001-2014
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 *---------------------------------------------------------------------------
 *
 * Description: NVM layout declarations SerialAPIPlus (embedded application part)
 *
 * Author:   Erik Friis Harck
 *
 * Last Changed By:  $Author: tro $
 * Revision:         $Revision: 23236 $
 * Last Changed:     $Date: 2012-08-17 14:04:49 +0200 (fr, 17 aug 2012) $
 *
 ****************************************************************************/
#ifndef _EEPROM_H_
#define _EEPROM_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include "serialappl.h"
#include <ZW_nvm_descriptor.h>

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/
#define POWERLEVEL_CHANNELS   3

/* NVM allocation declarations */

/* Note from Keil knowledgebase: http://www.keil.com/support/docs/901.htm   */
/* "The order is not necessarily taken from the variable declarations, but  */
/* the first use of the variables."                                         */
/* Therefore, when using #pragma ORDER to order variables, declare them in  */
/* the order they should be in a collection. And none of them may be        */
/* declared or known in any way from other header files.                    */

#ifndef slave_routing
/* NVM layout SerialAPIPlus (embedded application part) */
extern t_NvmModuleSize far nvmApplicationSize;
#endif
extern BYTE far EEOFFSET_MAGIC_far;
#ifdef ZW_SLAVE
extern BYTE far EEOFFSET_LISTENING_far;
extern BYTE far EEOFFSET_GENERIC_far;
extern BYTE far EEOFFSET_SPECIFIC_far;
extern BYTE far EEOFFSET_CMDCLASS_LEN_far;
#else
extern BYTE far EEOFFSET_CMDCLASS_LEN_far;
#endif
extern BYTE far EEOFFSET_CMDCLASS_far[];
extern BYTE far EEOFFSET_WATCHDOG_STARTED_far;
#ifdef ZW_SECURITY_PROTOCOL
extern BYTE far EEOFFSET_CMDCLASS_UNINCLUDED_LEN_far;
extern BYTE far EEOFFSET_CMDCLASS_UNINCLUDED_far[];
extern BYTE far EEOFFSET_CMDCLASS_INCLUDED_UNSECURE_LEN_far;
extern BYTE far EEOFFSET_CMDCLASS_INCLUDED_UNSECURE_far[];
extern BYTE far EEOFFSET_CMDCLASS_INCLUDED_SECURE_LEN_far;
extern BYTE far EEOFFSET_CMDCLASS_INCLUDED_SECURE_far[];
#endif  /* ZW_SECURITY_PROTOCOL */
#ifndef slave_routing
extern BYTE far EEOFFSET_POWERLEVEL_NORMAL_far[];
extern BYTE far EEOFFSET_POWERLEVEL_LOW_far[];
#endif
#ifdef ZW_SMARTSTART_ENABLED
extern BYTE far EEOFFSET_MODULE_POWER_MODE_EXTINT_ENABLE_far;
extern BYTE far EEOFFSET_MODULE_POWER_MODE_far;
extern BYTE far EEOFFSET_MODULE_POWER_MODE_WUT_TIMEOUT_far[sizeof(DWORD)];
#endif

#ifndef slave_routing
extern t_nvmModuleDescriptor far nvmApplicationDescriptor;
#endif

/* The starting address of the segment ?FD?EEPROM (to be used as a constant as (WORD)&_FD_EEPROM_S_) */
extern unsigned char _FD_EEPROM_S_;
/* The length of the segment ?FD?EEPROM in bytes (to be used as a constant as (WORD)&_FD_EEPROM_L_) */
extern unsigned char _FD_EEPROM_L_;

#define MAGIC_VALUE       0x42

#endif /* _EEPROM_H_ */


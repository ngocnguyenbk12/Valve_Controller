/**
 * @file
 * Header file for nvm_util.c. This module implements functions used in for application data
 * migration after firmware update. Module should only be used for migration from SDK 6.5x and
 * 6.6x. There is no reason to use this module if target is build on SDK 6.7x or later.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _NVM_UTIL_H_
#define _NVM_UTIL_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_basis_api.h>

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/
#define EEPROM_MAGIC_BYTE_VALUE     0x56 // Magic value used in SDK 6.51 and 6.60 when security is supported.

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/


/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/



/**
 * @brief NvmInit
 *
 * Initialization of application nvm area.
 * This function will ensure the layout of application area of the nvm are 
 * according to the design stated in the firmware.
 *
 * @param[in] nvmStatus NVM_STATUS_INITIALIZE will fill the application nvm area with default data.
 *                      NVM_STATUS_VALID will do nothing.
 *                      NVM_STATUS_UPGRADED will migrate data from previous firmware to match the 
 *                      newly installed firmware.
 */
void
NvmInit(
  ZW_NVM_STATUS nvmStatus
);

#endif /* _NVM_UTIL_H_ */


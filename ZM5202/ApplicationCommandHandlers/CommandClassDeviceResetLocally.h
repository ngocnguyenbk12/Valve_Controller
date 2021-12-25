/**
 * @file
 * Handler for Command Class Device Reset Locally.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _COMMAND_CLASS_DEVICE_RESET_LOCALLY_H_
#define _COMMAND_CLASS_DEVICE_RESET_LOCALLY_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include <CommandClass.h>

/****************************************************************************/
/*                       PUBLIC TYPES and DEFINITIONS                       */
/****************************************************************************/

/**
 * Returns the version of this CC.
 */
#define CommandClassDeviceResetLocallyVersionGet() DEVICE_RESET_LOCALLY_VERSION

/**
 * For backwards compatibility.
 */
#define handleCommandClassDeviceResetLocally(a,b) CC_DeviceResetLocally_notification_tx(a,b)

/****************************************************************************/
/*                            PUBLIC FUNCTIONS                              */
/****************************************************************************/

/**
 * Transmits a Device Reset Locally Notification.
 * @param[in] pProfile Pointer to AGI profile.
 * @param[in] pCallback Callback function pointer. Use the callback call to reset the node.
 * This function callback MUST be implemented.
 */
void CC_DeviceResetLocally_notification_tx(
  agi_profile_t * pProfile,
  VOID_CALLBACKFUNC(pCallback)(transmission_result_t * pTransmissionResult));

#endif

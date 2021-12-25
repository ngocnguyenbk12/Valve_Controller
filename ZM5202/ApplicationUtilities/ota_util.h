/**
 * @file
 * Header file for ota_util.c. This module implements functions used in combination with command
 * class firmware update.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _OTA_UTIL_H_
#define _OTA_UTIL_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include <CommandClassFirmwareUpdate.h>

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

/**
 * The OTA status is used to tell the application the result of the firmware update.
 */
typedef enum
{
  OTA_STATUS_DONE = 0,
  OTA_STATUS_ABORT = 1,
  OTA_STATUS_TIMEOUT = 2
} OTA_STATUS;

/**
 * Defines for WaitTime field used in commmand = FIRMWARE_UPDATE_MD_STATUS_REPORT.
 * The WaitTime field MUST report the time that is needed before the receiving
 * node again becomes available for communication after the transfer of an image.
 * The unit is the second.
 * This changed to 30 sec. for protocol to extract image etc. Please see TO# 07591.
 */
#ifndef WAITTIME_FWU_SUCCESS
#define WAITTIME_FWU_SUCCESS 30
#endif
#ifndef WAITTIME_FWU_FAIL
#define WAITTIME_FWU_FAIL 2
#endif
/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/


/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/



/**
 * @brief OtaInit
 * Initialization of Firmware Update module "OTA" has  3 input parameters pOtaStart, pOtaExtWrite
 * and pOtaFinish.
 * Input parameters pOtaStart and pOtaFinish is used to inform Application of
 * the status of firmware update and give application possibility to control
 * start of firmware update. It is possible to not call OtaInit and the process
 * run without the application with standard parameters for txOption. Input parameter
 * pOtaExtWrite is used to update host firmware.
 *
 * @param[in] pOtaStart function pointer is called when firmware update is ready to
 * start. As input parameters are firmware-Id of the firmware to be upgraded and CRC of the firmware.
 *  pOtaStart can be set to NULL and OTA process is started automatically.
 * @param[in] pOtaExtWrite function pointer is called when fw-data is received for extern host.
 * Write process end with following call pOtaExtWrite( NULL, 0) to application.
 * Can be initialized to NULL if firmware only is for the device.
 * @param[in] pOtaFinish function pointer is called when the firmware update proces
 * i finish. As input parameter is status of the process of type OTA_STATUS.
 * If set to NULL, ota_util module will reboot when process is finish.
 * @return 1 if NVM is supported else 0.
 */
BYTE
OtaInit(
  BOOL (CODE *pOtaStart)(WORD fwId, WORD CRC),
  VOID_CALLBACKFUNC(pOtaExtWrite)( BYTE *pData, BYTE dataLen),
  VOID_CALLBACKFUNC(pOtaFinish)(BYTE val));


/**
 * @brief OtaHostFWU_WriteFinish
 * Host call function when finish reading incoming frame. Ota start to get
 * next frame.
 */
void
OtaHostFWU_WriteFinish(void);


/**
 * @brief OtaHostFWU_Status
 * Application call this function when firmware update process is finish. Status
 * of the process is send to the controller.
 * @param[in] userReboot Tell user to reboot host on successfull firmware update.
 * @param[in] status of the process. TRUE - successfull, FALSE - process failed.
 */
void
OtaHostFWU_Status( BOOL userReboot, BOOL status );

/**
 * Function to query if an OTA firmware update is currently in progress.
 * @return TRUE if an update is in progress. Else FALSE.
 */
BOOL
Ota_UpdateIsInProgress(void);

#endif /* _OTA_UTIL_H_ */


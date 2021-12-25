/**
 * @file
 * Handler for Command Class Basic.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _COMMAND_CLASS_BASIC_H_
#define _COMMAND_CLASS_BASIC_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include <ZW_classcmd.h>
#include <ZW_TransportEndpoint.h>
#include <agi.h>

/**
 * Returns the version of this CC.
 */
#define CommandClassBasicVersionGet() BASIC_VERSION_V2

/**
 * @brief handleBasicSetCommand
 * Application code. Incoming command class Set call to set value in application endpoint
 * @param[in] val parmeter dependent of the application device class
 * @param[in] endpoint is the destination endpoint
 */
extern void handleBasicSetCommand(
  BYTE val,
  BYTE endpoint
);


/**
 * @brief getAppBasicReport
 * Application code. Incoming command class Report call to set value in application endpoint
 * @param[in] endpoint is the destination endpoint
 * @return get application value to send in a report
 */
extern BYTE getAppBasicReport( BYTE endpoint );

/**
 * @brief getAppBasicReportTarget
 * Return the value of an ongoing transition or the most recent transition.
 * @param[in] endpoint is the destination endpoint
 * @return target value.
 */
extern BYTE getAppBasicReportTarget( BYTE endpoint );

/**
 * @brief Return the time needed to reach the Target Value at the actual transition rate.
 * @details Duration encoded as follow:
 * Duration      Description
 *  0x00           0 seconds. Already at the Target Value.
 *  0x01-0x7F      1 second (0x01) to 127 seconds (0x7F) in 1 second resolution.
 *  0x80-0xFD      1 minute (0x80) to 126 minutes (0xFD) in 1 minute resolution.
 *  0xFE           Unknown duration
 *  0xFF           Reserved
 * @param[in] endpoint is the destination endpoint
 * @return duration time.
 */
BYTE getAppBasicReportDuration(BYTE endpoint);

/**
 * @brief Handler for CC Basic.
 * @param[in] rxOpt receive options of type RECEIVE_OPTIONS_TYPE_EX
 * @param[in] pCmd Payload from the received frame
 * @param[in] cmdLength number of command bytes including the command
 * @return receive frame status.
*/
received_frame_status_t handleCommandClassBasic(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  BYTE cmdLength);


/**
 * @brief Send unsolicited command class Basic report
 * @param[in] pProfile pointer to AGI profile
 * @param[in] sourceEndpoint source endpoint
 * @param[in] bValue Basic Report value
 * @param[out] pCbFunc callback function pointer returning status on job. Can be initialized to NULL.
 * @return status of the job of type JOB_STATUS
 */
JOB_STATUS CmdClassBasicReportSend(
  AGI_PROFILE* pProfile,
  BYTE sourceEndpoint,
  BYTE bValue,
  VOID_CALLBACKFUNC(pCbFunc)(TRANSMISSION_RESULT * pTransmissionResult));

/**
 * @brief Sends an unsolicited Basic Set command.
 * @param[in] pProfile Pointer to an AGI profile defined in the config_app.h of the application.
 * @param[in] sourceEndpoint Source endpoint if the sending device use endpoints. Otherwise 0.
 * @param[in] bValue Value as defined in the specification of the command.
 * @param[out] pCbFunc Callback function pointer giving a result of the transmission.
 * @return Status on whether the transmission could be initiated.
 */
//@ [CmdClassBasicSetSend_ID]
JOB_STATUS CmdClassBasicSetSend(
  AGI_PROFILE* pProfile,
  BYTE sourceEndpoint,
  BYTE bValue,
  VOID_CALLBACKFUNC(pCbFunc)(TRANSMISSION_RESULT * pTransmissionResult));
//@ [CmdClassBasicSetSend_ID]

#endif

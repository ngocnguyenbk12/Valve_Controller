/**
 * @file
 * Handler for Command Class Firmware Update.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 *
 * @brief Current version do not support FIRMWARE_UPDATE_ACTIVATION_SET_V4 why
 * FIRMWARE_UPDATE_ACTIVATION_STATUS_REPORT_V4 return status ERROR_ACTIVATION_FIRMWARE.
 * Customers who need this feature can modify command class source and header
 * files for the specific purpose.
 */

#ifndef _COMMANDCLASSFIRMWAREUPDATE_H_
#define _COMMANDCLASSFIRMWAREUPDATE_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <CommandClass.h>
#include <ZW_TransportEndpoint.h>
#include <ZW_classcmd.h>

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

/**
 * Returns the version of this CC.
 */
#define CommandClassFirmwareUpdateMdVersionGet() FIRMWARE_UPDATE_MD_VERSION_V4

/**
 * Information struct for firmware update.
 */
typedef struct
{
  uint16_t manufacturerId;
  uint16_t firmwareId;
  uint16_t checksum;
}
FW_UPDATE_GET;


typedef enum
{
  INVALID_COMBINATION = 0x00,
  ERROR_ACTIVATION_FIRMWARE = 0x01,
  FWU_SUCCESS = 0xFF
} e_firmware_update_activation_status_report_update_status;
/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/


/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

/**
 * @brief handleCommandClassFWUpdate
 * @param[in] *rxOpt Frame header info.
 * @param[in] *pCmd Payload from the received frame, the union should be used to access
 * the fields.
 * @param[in] cmdLength IN Number of command bytes including the command.
 * @return receive frame status.
 */
received_frame_status_t handleCommandClassFWUpdate(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  BYTE cmdLength
);



/**
 * @brief handleCmdClassFirmwareUpdateMdReport
 * Application function to handle incomming frame Firmware update  MD Report
 * @param crc16Result
 * @param firmwareUpdateReportNumber
 * @param properties
 * @param pData
 * @param fw_actualFrameSize
 */
extern void
handleCmdClassFirmwareUpdateMdReport( WORD crc16Result,
                                      WORD firmwareUpdateReportNumber,
                                      BYTE properties,
                                      BYTE* pData,
                                      BYTE fw_actualFrameSize);



/**
 * @brief Send a Md status report
 * @param[in] rxOpt receive options
 * @param[in] status Values used for Firmware Update Md Status Report command
 * FIRMWARE_UPDATE_MD_STATUS_REPORT_UNABLE_TO_RECEIVE_WITHOUT_CHECKSUM_ERROR_V3     0x00
 * FIRMWARE_UPDATE_MD_STATUS_REPORT_UNABLE_TO_RECEIVE_V3                            0x01
 * FIRMWARE_UPDATE_MD_STATUS_REPORT_SUCCESSFULLY_STORED_V3                          0xFE
 * FIRMWARE_UPDATE_MD_STATUS_REPORT_SUCCESSFULLY_V3                                 0xFF
 * @param[in] waitTime field MUST report the time that is needed before the receiving
 * node again becomes available for communication after the transfer of an image. The unit is
 * the second. The value 0 (zero) indicates that the node is already available again. The value
 * 0 (zero) MUST be returned when the Status field carries the values 0x00, 0x01 and 0xFE.
 * The value 0xFFFF is reserved for future use and MUST NOT be returned.
 * @param[out] pCbFunc function pointer returning status on the job.
 * @return JOB_STATUS..
 */
JOB_STATUS
CmdClassFirmwareUpdateMdStatusReport(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  BYTE status,
  WORD waitTime ,
  VOID_CALLBACKFUNC(pCbFunc)(TRANSMISSION_RESULT * pTransmissionResult));


/**
 * @brief Send command Firmware update  MD Get
 * @param[in] rxOpt receive options
 * @param[in] firmwareUpdateReportNumber current frame number.
 * @return JOB_STATUS
 */
JOB_STATUS
CmdClassFirmwareUpdateMdGet( RECEIVE_OPTIONS_TYPE_EX *rxOpt, WORD firmwareUpdateReportNumber );


/**
 * @brief Remote request for firmware update
 * @return TRUE: we are ready to firmware update. FALSE: reject it.
 */
extern BOOL
RemoteReqAuthentication();


/**
 * @brief Authentication fw to update.
 * @param[in] rxOpt receive options of type RECEIVE_OPTIONS_TYPE_EX
 * @param[in] fwTarget is firmware target to update.
 * @param[in] fragmetSize size of incoming fw-frame.
 * @param[in] pData pointer to firmware information of type FW_UPDATE_GET
 * @param[out] pStatus pointer of including values FIRMWARE_UPDATE_MD_REQUEST_REPORT_INVALID_COMBINATION_V3,
 * FIRMWARE_UPDATE_MD_REQUEST_REPORT_REQUIRES_AUTHENTICATION_V3 or
 * FIRMWARE_UPDATE_MD_REQUEST_REPORT_VALID_COMBINATION_V3
 */
extern void
handleCmdClassFirmwareUpdateMdReqGet(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  BYTE fwTarget,
  WORD fragmetSize,
  FW_UPDATE_GET* pData,
  BYTE* pStatus);


/**
 * @brief ZCB_CmdClassFwUpdateMdReqReport
 * Callback function receive status on Send data FIRMWARE_UPDATE_MD_REQUEST_REPORT_V3
 * @param txStatus : TRANSMIT_COMPLETE_OK, TRANSMIT_COMPLETE_NO_ACK, TRANSMIT_COMPLETE_FAIL...
 * @return description..
 */
extern void
ZCB_CmdClassFwUpdateMdReqReport(BYTE txStatus);

/**
 * Returns the maximum supported fragment size.
 *
 * The maximum fragment Size field MUST report the maximum fragment size that a device is able to
 * receive at a time. A sending device MAY send shorter fragments. The fragment size actually used
 * is indicated in the Firmware Update Meta Data Request Get Command and confirmed in the Firmware
 * Update Meta Data Request Report Command.
 * @return Maximum fragment size.
 */
extern uint16_t handleCommandClassFirmwareUpdateMaxFragmentSize();

/**
 * @brief handleFirmWareIdGet
 * This function called by the framework to get firmware Id of target n (0 => is device FW ID)
 * @param[in] n the target index (0,1..N-1)
 * @return target n firmware ID
 */
extern WORD
handleFirmWareIdGet(BYTE n);

#endif /* _COMMANDCLASSFIRMWAREUPDATE_H_*/


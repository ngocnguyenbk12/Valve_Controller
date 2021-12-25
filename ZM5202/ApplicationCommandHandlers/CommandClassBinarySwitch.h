/**
 * @file
 * Handler for Command Class Binary Switch.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _COMMAND_CLASS_BINARY_SWITCH_H_
#define _COMMAND_CLASS_BINARY_SWITCH_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include <ZW_sysdefs.h>
#include <ZW_pindefs.h>
#include <ZW_evaldefs.h>
#include <ZW_classcmd.h>
#include <CommandClass.h>
#include <ZW_TransportEndpoint.h>
#include <agi.h>

/**
 * Returns the version of this CC.
 */
#define CommandClassBinarySwitchVersionGet() SWITCH_BINARY_VERSION

/**
 * The value can be either 0x00 (off/disable) or 0xFF (on/enable). The values from
 * 1 to 99 (0x01 to 0x63) MUST be mapped to 0xFF upon receipt of the Command in the
 * device. All other values are reserved and MUST be ignored by the receiving device.
 */
typedef enum
{
  CMD_CLASS_BIN_OFF = SWITCH_BINARY_SET_OFF_DISABLE_V2,
  CMD_CLASS_BIN_ON  = SWITCH_BINARY_SET_ON_ENABLE_V2
}
CMD_CLASS_BIN_SW_VAL;

/**
 * Application code. Incoming command class Get call to read value from application endpoint
 * @param[in] endpoint is endpoint destination for the Get job
 * @return value
 */
extern uint8_t handleAppltBinarySwitchGet(uint8_t endpoint);

/**
 * @brief Application code. Incoming command class Set call to set value in
 * application endpoint.
 * @param[in] val parmeter of type  CMD_CLASS_BIN_SW_VAL
 * @param[in] endpoint is the destination endpoint
 */
extern void handleApplBinarySwitchSet(
  CMD_CLASS_BIN_SW_VAL val,
  uint8_t endpoint);

/**
 * Handler for command class Binary Switch.
 * @param[in] rxOpt receive options of type RECEIVE_OPTIONS_TYPE_EX
 * @param[in] pCmd Payload from the received frame
 * @param[in] cmdLength Number of command bytes including the command
 * @return receive frame status.
 */
received_frame_status_t handleCommandClassBinarySwitch(
  RECEIVE_OPTIONS_TYPE_EX * rxOpt,
  ZW_APPLICATION_TX_BUFFER * pCmd,
  uint8_t cmdLength);

/**
 * Check value is correct for current class and call application handleApplBinarySwitchSet function
 * @param[in] val is value to validate
 * @param[in] endpoint is the destination endpoint
 */
void CommandClassBinarySwitchSupportSet(
  uint8_t val,
  uint8_t endpoint);

/**
 * Send a unsolicited command class Binary Switch report.
 * @param[in] pProfile pointer to AGI profile
 * @param[in] sourceEndpoint source endpoint
 * @param[in] bValue report value of enum type CMD_CLASS_BIN_SW_VAL
 * @param[out] pCbFunc callback funtion returning status destination node receive job.
 * @return status on the job.
 */
JOB_STATUS CmdClassBinarySwitchReportSendUnsolicited(
  AGI_PROFILE* pProfile,
  uint8_t sourceEndpoint,
  CMD_CLASS_BIN_SW_VAL bValue,
  VOID_CALLBACKFUNC(pCbFunc)(TRANSMISSION_RESULT * pTransmissionResult));

#endif /* _COMMAND_CLASS_BINARY_SWITCH_H_ */

/**
 * @file
 * Handler for Command Class Battery.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _COMMAND_CLASS_BATTERY_H_
#define _COMMAND_CLASS_BATTERY_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include <ZW_sysdefs.h>
#include <ZW_pindefs.h>
#include <ZW_evaldefs.h>
#include <ZW_classcmd.h>
#include <CommandClass.h>
#include <agi.h>
#include <battery_monitor.h>

/**
 * Returns the version of this CC.
 */
#define CommandClassBatteryVersionGet() BATTERY_VERSION

/**
 * @brief handleCommandClassBattery
 * Handler for command class Battery
 * @param[in] rxOpt receive options of type RECEIVE_OPTIONS_TYPE_EX
 * @param[in] pCmd Payload from the received frame
 * @param[in] cmdLength Number of command bytes including the command
 * @return receive frame status.
 */
received_frame_status_t handleCommandClassBattery(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  BYTE cmdLength);


/**
 * @brief CmdClassBatteryReport
 * Send unsolicited battery report
 * @param[in] pProfile pointer to AGI profile
 * @param[in] sourceEndpoint source endpoint
 * @param[in] bBattLevel report value of type Battery report value
 * @param[out] pCbFunc callback funtion returning status destination node receive job.
 * @return status on protocol exuted the job.
 */
JOB_STATUS CmdClassBatteryReport(
  AGI_PROFILE* pProfile,
  BYTE sourceEndpoint,
  BYTE bBattLevel,
  VOID_CALLBACKFUNC(pCbFunc)(TRANSMISSION_RESULT * pTransmissionResult));


/**
 * @brief BatterySensorRead
 *  Required function for the Battery Command class. This funciton read the
 *  Battery voltage from the battery Voltage sensor HW
 * @param[out] battLvl read battery level
 * @return if battery has change battery level state (ex. state high to low)
 */
extern BOOL BatterySensorRead(BATT_LEVEL *battLvl);

#endif

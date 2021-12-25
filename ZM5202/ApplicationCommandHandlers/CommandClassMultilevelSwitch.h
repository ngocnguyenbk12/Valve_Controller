/**
 * @file
 * Handler for Command Class Multilevel Switch.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _COMMAND_CLASS_MULTILEVEL_SWITCH_H_
#define _COMMAND_CLASS_MULTILEVEL_SWITCH_H_

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

/**
 * Returns the version of this CC.
 */
#define CommandClassMultilevelSwitchVersionGet() CommandClassMultiLevelSwitchVersionGet()

/**
 * For backwards compatibility with upper case L in Multilevel.
 */
#define CommandClassMultiLevelSwitchVersionGet() SWITCH_MULTILEVEL_VERSION_V4

/**
 * Multi-level switch type
 */
typedef enum
{
  MULTILEVEL_SWITCH_OFF_ON  = 0x01,	/**< (Direction/Endpoint A) value 0x00 = = off, (Direction/Endpoint A) value 0x63/0xFF = on)*/                              //!< MULTILEVEL_SWITCH_OFF_ON
  MULTILEVEL_SWITCH_DOWN_UP = 0x02,	/**< (Direction/Endpoint A) value 0x00 = = down, (Direction/Endpoint A) value 0x63/0xFF = up)*/                             //!< MULTILEVEL_SWITCH_DOWN_UP
  MULTILEVEL_SWITCH_CLOSE_OPEN = 0x03,	/** (Direction/Endpoint A) value 0x00 = = close, (Direction/Endpoint A) value 0x63/0xFF = open)*/                        //!< MULTILEVEL_SWITCH_CLOSE_OPEN
  MULTILEVEL_SWITCH_COUNTER_CLOCKWISE =0x04,	/**< (Direction/Endpoint A) value 0x00 = = counter-clockwise, (Direction/Endpoint A) value 0x63/0xFF = clockwise)*///!< MULTILEVEL_SWITCH_COUNTER_CLOCKWISE
  MULTILEVEL_SWITCH_LEFT_RIGHT = 0x05,	/**< (Direction/Endpoint A) value 0x00 = left, (Direction/Endpoint A) value 0x63/0xFF = right)*/                         //!< MULTILEVEL_SWITCH_LEFT_RIGHT
  MULTILEVEL_SWITCH_REVERSE_FORWARD = 0x06,	/** (Direction/Endpoint A) value 0x00 = reverse, (Direction/Endpoint A) value 0x63/0xFF = forward)*/                //!< MULTILEVEL_SWITCH_REVERSE_FORWARD
  MULTILEVEL_SWITCH_PULL_PUSH = 0x07	/**< (Direction/Endpoint A) value 0x00 = pull, (Direction/Endpoint A) value 0x63/0xFF = push)*/                            //!< MULTILEVEL_SWITCH_PULL_PUSH
} MULTILEVEL_SWITCH_TYPE;

/**
 * Enumeration for "Start Level Change" command.
 */
typedef enum
{
  CCMLS_PRIMARY_SWITCH_UP,              //!< CCMLS_PRIMARY_SWITCH_UP
  CCMLS_PRIMARY_SWITCH_DOWN,            //!< CCMLS_PRIMARY_SWITCH_DOWN
  CCMLS_PRIMARY_SWITCH_RESERVED,        //!< CCMLS_PRIMARY_SWITCH_RESERVED
  CCMLS_PRIMARY_SWITCH_NO_UP_DOWN_MOTION//!< CCMLS_PRIMARY_SWITCH_NO_UP_DOWN_MOTION
}
CCMLS_PRIMARY_SWITCH_T;

/**
 * Enumeration for "Start Level Change" command.
 */
typedef enum
{
  CCMLS_IGNORE_START_LEVEL_FALSE,//!< CCMLS_IGNORE_START_LEVEL_FALSE
  CCMLS_IGNORE_START_LEVEL_TRUE  //!< CCMLS_IGNORE_START_LEVEL_TRUE
}
CCMLS_IGNORE_START_LEVEL_T;

/**
 * Enumeration for "Start Level Change" command.
 */
typedef enum
{
  CCMLS_SECONDARY_SWITCH_INCREMENT,//!< CCMLS_SECONDARY_SWITCH_INCREMENT
  CCMLS_SECONDARY_SWITCH_DECREMENT,//!< CCMLS_SECONDARY_SWITCH_DECREMENT
  CCMLS_SECONDARY_SWITCH_RESERVED, //!< CCMLS_SECONDARY_SWITCH_RESERVED
  CCMLS_SECONDARY_SWITCH_NO_INC_DEC//!< CCMLS_SECONDARY_SWITCH_NO_INC_DEC
}
CCMLS_SECONDARY_SWITCH_T;

/**
 * Set the Multilevel Switch to the specified value/level.
 *
 * This command is called by the framework when the HW needs to be adjusted to a new target value.
 *
 * The application must call \ref CC_MultilevelSwitch_SetValue to set a new target value.
 *
 * @param[in] bLevel The target level of the multilevel switch. The valid range is from 0 to 99.
 *        The device may not support all the 100 level. If the HW support less levels than 100
 *        then the HW levels should be distributed uniformly over the entire range.
 *        The mapping of command values to hardware levels MUST be monotonous, i.e. a higher value
 *        MUST be mapped to either the same or a higher hardware level.
 * @param[in] endpoint is the destination endpoint
 * @return none
 */
extern void CC_MultilevelSwitch_Set_handler(uint8_t bLevel, uint8_t endpoint);

#define CommandClassMultiLevelSwitchSet(a, b) CC_MultilevelSwitch_Set_handler(a, b)

/**
 * Returns the current value for a given endpoint.
 * @param[in] endpoint Given endpoint.
 * @return Current value.
 */
extern uint8_t CC_MultilevelSwitch_GetCurrentValue_handler(uint8_t endpoint);

/**
 * For backwards compatibility.
 */
#define CommandClassMultiLevelSwitchGet(a) CC_MultilevelSwitch_GetCurrentValue_handler(a)

/**
 * @brief Handler for CC Multilevel switch.
 * @param[in] rxOpt pointer receive options of type RECEIVE_OPTIONS_TYPE_EX
 * @param[in] pCmd pointer Payload from the received frame
 * @param[in] cmdLength Number of command bytes including the command
 * @return receive frame status.
 */
received_frame_status_t handleCommandClassMultiLevelSwitch(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  uint8_t cmdLength);


/**
 * @brief CommandClassMultiLevelSwitchPrimaryTypeGet
 * Get the supported primary switch type used by the HW.
 * @param[in] endpoint is the destination endpoint
 * @return switch type
 */
extern MULTILEVEL_SWITCH_TYPE
CommandClassMultiLevelSwitchPrimaryTypeGet(uint8_t endpoint);

/**
 * @brief ZCB_CommandClassMultiLevelSwitchSupportSet
 * Set the multilevel switch to the target level
 * @param[in] bTargetlevel the target level of the multilevel switch. The valid range is from 0 to 99.
 * @param[in] bDuration duration level.
 * @param[in] endpoint is the destination endpoint
 */
void
ZCB_CommandClassMultiLevelSwitchSupportSet(
  uint8_t bTargetlevel,
  uint8_t bDuration,
  uint8_t endpoint );

/**
 * @brief Initiates the transmission of a "Multilevel Switch Start Level Change"
 * command.
 * @param[in] pProfile pointer to AGI profile
 * @param[in] sourceEndpoint source endpoint
 * @param[out] pCbFunc Callback function to be called when transmission is done/failed.
 * @param[in] primarySwitch Controls the primary device functionality.
 * @param[in] fIgnoreStartLevel Ignore start level.
 * @param[in] secondarySwitch Controls the secondary device functionality.
 * @param[in] primarySwitchStartLevel Start level for the primary device functionality.
 * @param[in] duration The duration from lowest to highest value.
 * @param[in] secondarySwitchStepSize Step size for secondary device functionality.
 * @return Status of the job.
 */
JOB_STATUS
CmdClassMultiLevelSwitchStartLevelChange(
  AGI_PROFILE* pProfile,
  uint8_t sourceEndpoint,
  VOID_CALLBACKFUNC(pCbFunc)(TRANSMISSION_RESULT * pTransmissionResult),
  CCMLS_PRIMARY_SWITCH_T primarySwitch,
  CCMLS_IGNORE_START_LEVEL_T fIgnoreStartLevel,
  CCMLS_SECONDARY_SWITCH_T secondarySwitch,
  uint8_t primarySwitchStartLevel,
  uint8_t duration,
  uint8_t secondarySwitchStepSize);

/**
 * @brief Initiates the transmission of a "Multilevel Switch Stop Level Change"
 * command.
 * @param[in] pProfile pointer to AGI profile
 * @param[in] sourceEndpoint source endpoint
 * @param[out] pCbFunc Callback function to be called when transmission is done/failed.
 * @return Status of the job.
 */
JOB_STATUS
CmdClassMultiLevelSwitchStopLevelChange(
  AGI_PROFILE* pProfile,
  uint8_t sourceEndpoint,
  VOID_CALLBACKFUNC(pCbFunc)(TRANSMISSION_RESULT * pTransmissionResult));

/**
 * @brief Initiates the transmission of a "Multilevel Switch Set" command.
 * @param[in] pProfile pointer to AGI profile
 * @param[in] sourceEndpoint source endpoint
 * @param[out] pCbFunc Callback function to be called when transmission is done/failed.
 * @param[in] value Multilevel value.
 * @param[in] duration The duration from current value to the new given value.
 * @return Status of the job.
 */
JOB_STATUS
CmdClassMultiLevelSwitchSetTransmit(
  AGI_PROFILE* pProfile,
  uint8_t sourceEndpoint,
  VOID_CALLBACKFUNC(pCbFunc)(TRANSMISSION_RESULT * pTransmissionResult),
  uint8_t value,
  uint8_t duration);

/**
 * Returns the factory default dimming duration used when handling Multilevel Switch Set or Start
 * Level Change commands.
 *
 * NOTICE: This function must be implemented by the application.
 *
 * Please see Table 7 in SDS13781 for information on how the duration is encoded.
 * @param[in] boIsSetCmd If set to TRUE, the function must return the dimming duration used for
 * the Multilevel Switch Set command.
 * If set to FALSE, the function must return the dimming duration used for the Multilevel Switch
 * Start Level Change command.
 * These values can be identical.
 * @param[in] endpoint Endpoint destination.
 * @return The endpoint's default dimming duration.
 */
extern uint8_t GetFactoryDefaultDimmingDuration(BOOL boIsSetCmd, uint8_t endpoint);

/**
 * Handles Multilevel Switch Set commands.
 * @param bTargetlevel Target level/value.
 * @param bDuration Duration of the change.
 * @param endpoint Endpoint to which the change must apply.
 * @return TRUE if the endpoint is a valid Multilevel Switch endpoint, FALSE otherwise.
 */
extern BOOL CC_MLS_Set_handler(uint8_t bTargetlevel, uint8_t bDuration, uint8_t endpoint);

#endif


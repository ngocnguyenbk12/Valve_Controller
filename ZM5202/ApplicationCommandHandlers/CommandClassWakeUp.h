/**
 * @file
 * Handler for Command Class Wake Up.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _COMMANDCLASSWAKEUP_H_
#define _COMMANDCLASSWAKEUP_H_

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
#define CC_WakeUp_getVersion() WAKE_UP_VERSION_V2

/**
 * Wakeup parameter types
 */
typedef enum _WAKEUP_PAR_
{
  WAKEUP_PAR_SLEEP_STEP,
  WAKEUP_PAR_MIN_SLEEP_TIME,
  WAKEUP_PAR_MAX_SLEEP_TIME,
  WAKEUP_PAR_DEFAULT_SLEEP_TIME,
  WAKEUP_PAR_COUNT
}
wakeup_parameter_t;

/**
 * For backwards compatibility.
 */
#define CmdClassWakeupVersionGet() CC_WakeUp_getVersion()
#define CmdClassWakeupVersion() CmdClassWakeupVersionGet()
#define HandleCommandClassWakeUp(a, b, c) CC_WakeUp_handler(a, b, c)
#define WakeUpNotification(a) CC_WakeUp_notification_tx(a)

typedef wakeup_parameter_t WAKEUP_PAR;


/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

// Nothing here.

/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

/**
 * @brief Transmits a Wake Up Notification command and handles callback.
 * @param[in] pCallback Pointer to function which will be called on Wake Up Notification ACK/NACK.
 */
void CC_WakeUp_notification_tx(VOID_CALLBACKFUNC(pCallback)(TRANSMISSION_RESULT * pTransmissionResult));

/**
 * @brief Transmits a Wake Up Notification command.
 * @param pCallback Pointer to callback function to be called upon transmission.
 * @return Status of the job.
 */
JOB_STATUS CmdClassWakeupNotification(
    VOID_CALLBACKFUNC(pCallback)(TRANSMISSION_RESULT * pTransmissionResult));

/**
 * @brief Sets up the parameters defined in wakeup_parameter_t. The values are given in seconds.
 * @param[in] type Wake up parameter.
 * @param[in] time Time in seconds.
 */
void SetWakeUpConfiguration(
    wakeup_parameter_t type,
    uint32_t time);

/**
 * @brief Handler for Wake Up CC.
 * @param[in] rxOpt Receive options.
 * @param[in] pCmd Payload from the received frame.
 * @param[in] cmdLength Length of the given payload.
 * @return receive frame status.
 */
received_frame_status_t CC_WakeUp_handler(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  uint8_t cmdLength);

/**
 * @brief Resets the saved node ID in NVM.
 */
void CmdClassWakeUpNotificationMemorySetDefault(void);

/**
 * @brief Handler for Wake Up Interval Get Command.
 * @return The current wake up interval.
 */
extern uint32_t handleWakeUpIntervalGet(void);

/**
 * @brief Set default sleep interval for device.
 * @param[in] sleep period for device to sleep.
 */
extern void SetDefaultBatteryConfiguration(uint32_t sleep);

#endif /* _COMMANDCLASSWAKEUP_H_ */

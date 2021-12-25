/**
 * @file
 * Implements functions for power management of battery nodes.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */
#ifndef _ZAF_PM_H_
#define _ZAF_PM_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include <CommandClassWakeUp.h>
#include <ZW_typedefs.h>
#include <ZW_basis_api.h>

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

/**
 * PowerDownTimeout determines the number of seconds the sensor is kept alive
 * between powerdowns. The default is one second, which is probably too little
 * if you are routing in your network. ZWave+ defined: go to sleep > 10 SDS11846-2.doc
 */
#define DEFAULT_POWERDOWNTIMEOUT    11

/**
 * KeepAliveTimeout determines the number of seconds the sensor is kept alive
 * when the button is activated for more than KEEPALIVEACTIVATEPERIOD seconds.
 * This can be used when installing the sensor in a network. Default keepalive
 * is 30 seconds.
 */
#define DEFAULT_KEEPALIVETIMEOUT   30

/**
 * Press and hold button for this period of time to enter keepalive mode
 * Default is 3 seconds.
 */
#define DEFAULT_KEEPALIVEACTIVATETIMEOUT  3

/**
 * WAKEUPCOUNT holds the number of times WUT has been activated. The value is stored
 * in EEPROM and is used to determine when to send a Wakeup Information frame.
 * Default is 5 which means that when the sensor has been woken 5 times it will send
 * a Wakeup Information frame.
 */
#define DEFAULT_WAKEUPCOUNT 5

/**
 * Seconds in minutes
 */
#define SECONDS_IN_MINUTE    (DWORD)60

/**
 * Seconds in hours
 */
#define SECONDS_IN_HOUR      (DWORD)(60 * SECONDS_IN_MINUTE)

/**
 * Seconds in day
 */
#define SECONDS_IN_DAY       (DWORD)(24 * SECONDS_IN_HOUR)

/**
 * FLIRS device TX-option macro
 */
#define FLIRS_DEVICE_OPTIONS_MASK_MODIFY(deviceOptionsMask) \
  deviceOptionsMask = (deviceOptionsMask & ~(APPLICATION_NODEINFO_LISTENING)) \
    | APPLICATION_NODEINFO_NOT_LISTENING | APPLICATION_FREQ_LISTENING_MODE_1000ms

/**
 * Converts input given in seconds.
 */
#define ZAF_PM_INPUT_SECONDS(seconds) (seconds * 100)

/**
 * Half second define.
 */
#define ZAF_PM_HALF_SECOND 50

/**
 * Two second define.
 */
#define ZAF_PM_TWO_SECONDS ZAF_PM_INPUT_SECONDS(2)

/**
 * Ten second define.
 */
#define ZAF_PM_TEN_SECONDS ZAF_PM_INPUT_SECONDS(10)

/**
 * Learn mode power down timeout
 */
#define ZAF_PM_LEARNMODE_TIMEOUT ZAF_PM_INPUT_SECONDS(25)

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

// Nothing here.

/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

/**
 * @brief Initialization of battery module
 * @param[in] wakeUpReason is ApplicationInitHW(..) wake up reason.
 */
void ZAF_pm_Init(wakeup_reason_t wakeUpReason);

/**
 * Keeps the device awake for at least the given number of milliseconds.
 *
 * The resolution is 10 ms.
 *
 * The protocol can keep the device awake even longer.
 * @param time The time to keep the device awake given in milliseconds.
 */
void ZAF_pm_KeepAwake(uint16_t time);

/**
 * Keeps the device awake for an automatically detected amount of time.
 *
 * This covers listening and non-listening nodes while also taking into account the CC Wake Up.
 *
 * The following table shows stay-awake timings in seconds.
 *
 * Wake Up CC \ node type | Listening | Non-listening
 * --------------------------------------------------
 *                 Active |    10     |      10
 * --------------------------------------------------
 *               Inactive |     2     |       0
 * --------------------------------------------------
 */
void ZAF_pm_KeepAwakeAuto(void);

/**
 * Cancels the keep-awake timer.
 */
void ZAF_pm_KeepAwakeCancel(void);

/**
 * Returns whether the keep-alive timer is running or not.
 * @return TRUE if the timer is running, FALSE otherwise.
 */
BOOL ZAF_pm_IsActive(void);

/*************************************************************************************************/
/*                           CC Wake Up specific functions                                       */
/*************************************************************************************************/

/**
 * Returns whether the CC Wake Up is active.
 *
 * It's inactive when the device wakes up, but becomes active if the application transmits a
 * CC Wake Up Notification. As soon as the controller transmits a CC Wake Up No More Information,
 * it is deactivated.
 * @return TRUE if CC Wake Up is active, FALSE otherwise.
 */
BOOL ZAF_pm_WakeUpIsActive(void);

/**
 * Activates CC Wake Up.
 */
void ZAF_pm_ActivateCCWakeUp(void);

/**
 * Deactivates CC Wake Up.
 */
void ZAF_pm_DeactivateCCWakeUp(void);

#endif /* _ZAF_PM_H_ */


/**
 * @file
 * Implements functions that make it easier to support Battery monitor.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _BATTERY_MONITOR_H_
#define _BATTERY_MONITOR_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include <ZW_basis_api.h>

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

/**
 * Battery levels
 */
typedef enum _BATT_LEVELS_
{
  BATT_DEAD_LEV = 0xff,
  BATT_LOW_LEV  = 0x00,
  BATT_HIGH_LEV = 0x10,
  BATT_FULL_LEV = 0x64
} BATT_LEVEL;


/**
 * Battery states
 */
typedef enum _ST_BATT_ {
  ST_BATT_FULL,
  ST_BATT_HIGH,
  ST_BATT_LOW,
  ST_BATT_DEAD
} ST_BATT;

/**
 * For backwards compatibility.
 */
//#define SendBattReport(a) bm_

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

/**
 * Returns whether it's time to send a battery report or not.
 *
 * @return TRUE if it's time, FALSE otherwise.
 */
BOOL TimeToSendBattReport(void);


/**
 * Transmits a battery level report.
 *
 * @param pCallback Pointer to a function which will be called on ACK/NACK.
 * @return Status of the job.
 */
job_status_t
SendBattReport(VOID_CALLBACKFUNC(pCallback)(TRANSMISSION_RESULT * pTransmissionResult));


/**
 * Initializes the battery monitor.
 *
 * @param wakeUpReason received from ApplicationInitHW()
 */
void InitBatteryMonitor(wakeup_reason_t wakeUpReason);


/**
 * @brief SetLowBattReport
 * Reactivate Low battery report.
 */
void ActivateBattNotificationTrigger();


/**
 * @brief BatteryMonitorState
 * Get battery monitor state
 * @return Battry monitor state of type ST_BATT
 */
ST_BATT BatteryMonitorState(void);

#endif /* _BATTERY_MONITOR_H_ */

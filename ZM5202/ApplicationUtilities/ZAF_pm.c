/**
 * @file
 * Handling of power functionality and helper module for the Wake Up CC.
 *
 * Implements functions that make is easier to support Battery Operated Nodes.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include <ZW_typedefs.h>
#include <ZW_stdint.h>
#include <ZW_power_api.h>
#include <eeprom.h>
#include <ZAF_pm.h>
#include <ZW_uart_api.h>
#include <CommandClassPowerLevel.h>
#include "misc.h"
#include <ZW_timer_api.h>
#include <ZW_basis_api.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

#ifdef ZW_DEBUG_BATT
#define ZW_DEBUG_BATT_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_BATT_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_BATT_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_BATT_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_BATT_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_BATT_SEND_BYTE(data)
#define ZW_DEBUG_BATT_SEND_STR(STR)
#define ZW_DEBUG_BATT_SEND_NUM(data)
#define ZW_DEBUG_BATT_SEND_WORD_NUM(data)
#define ZW_DEBUG_BATT_SEND_NL()
#endif

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

/**
 * Handle for the timer which keeps the device awake.
 */
static bTimerHandle_t timerHandle = 0xFF;

/**
 * The time at which the device goes to sleep if the protocol does not keep it awake.
 */
static uint16_t currentDeadline = 0;

/**
 * Holds a value telling whether the CC Wake Up is active or not.
 */
static BOOL wakeUpCCactive = FALSE;

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

void ZCB_timer_callback(void);

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

void ZAF_pm_KeepAwakeCancel(void)
{
  ZW_TimerLongCancel(timerHandle);
  timerHandle = 0xFF;
  ZW_DEBUG_BATT_SEND_NL();
  ZW_DEBUG_BATT_SEND_STR("KAC");
}

/**
 * This function will be called when the stay awake timer triggers. It then clears the timer handle
 * which is used to check whether a stay awake timer is active.
 */
PCB(ZCB_timer_callback)(void)
{
  timerHandle = 0xFF;
  ZW_DEBUG_BATT_SEND_NL();
  ZW_DEBUG_BATT_SEND_STR("TC");
  ZW_DEBUG_BATT_SEND_WORD_NUM(getTickTime());
}

void ZAF_pm_KeepAwake(uint16_t time)
{
  ZW_DEBUG_BATT_SEND_STR("**\r\nZAF_pm_KeepAwake");
  ZW_DEBUG_BATT_SEND_WORD_NUM(time);
  ZW_DEBUG_BATT_SEND_NL();

  if (0 == time)
  {
    return;
  }

  if (0xFF == timerHandle)
  {
    /*
     * The timer is not active.
     */
    timerHandle = ZW_TimerLongStart(ZCB_timer_callback, (uint32_t)(time * 10), TIMER_ONE_TIME);
    currentDeadline = getTickTime() + time;
    ZW_DEBUG_BATT_SEND_NL();
    ZW_DEBUG_BATT_SEND_STR("TN");
    ZW_DEBUG_BATT_SEND_WORD_NUM(getTickTime());
  }
  else if ((getTickTime() + time) > currentDeadline)
  {
    /*
     * The timer is active and we must check whether the new delay leads to a new deadline. The new
     * delay is only valid if the deadline is after the current deadline.
     */
    ZW_TimerLongCancel(timerHandle);
    timerHandle = ZW_TimerLongStart(ZCB_timer_callback, (uint32_t)time * 10, TIMER_ONE_TIME);
    currentDeadline = getTickTime() + time;
    ZW_DEBUG_BATT_SEND_NL();
    ZW_DEBUG_BATT_SEND_STR("TE");
    ZW_DEBUG_BATT_SEND_WORD_NUM(time * 10);
  }
}

void ZAF_pm_KeepAwakeAuto(void)
{
  uint16_t time = 0;
#ifdef FLIRS
    time = ZAF_PM_TWO_SECONDS;
#endif
  if (TRUE == wakeUpCCactive)
  {
    time = ZAF_PM_TEN_SECONDS;
  }
  ZAF_pm_KeepAwake(time);
}

void ZAF_pm_Init(wakeup_reason_t wakeUpReason)
{
  ZW_DEBUG_BATT_SEND_STR("BattI");
  ZW_DEBUG_BATT_SEND_NUM(wakeUpReason);
  ZW_DEBUG_BATT_SEND_NL();

#ifndef __C51__
  timerHandle = 0xFF;
#endif

#ifdef FLIRS
    ZW_Power_Management_Init(0, ZW_INT_MASK_EXT1);
#else
    ZW_Power_Management_Init(handleWakeUpIntervalGet(), ZW_INT_MASK_EXT1);
#endif

  if (SW_WAKEUP_SENSOR == wakeUpReason)
  {
    /*
     * If the reason is SW_WAKEUP_SENSOR, it means that we woke up from a beam (being a FLiRS node).
     * We must then stay awake for a minimum of 2 seconds (Z-Wave Plus Role Type Specification -
     * SDS11846, section 4.8.4, version 16, 07-04-2016).
     */
    ZAF_pm_KeepAwake(ZAF_PM_TWO_SECONDS);
  }
}

void SetDefaultBatteryConfiguration(uint32_t sleep)
{
  ZW_Power_Management_Init(sleep, ZW_INT_MASK_EXT1);
  ZW_MemoryPutBuffer((WORD)&EEOFFSET_SLEEP_PERIOD_far, (BYTE_P)&sleep, sizeof(uint32_t));
}

uint32_t handleWakeUpIntervalGet(void)
{
  uint32_t sleep;
  MemoryGetBuffer((WORD)&EEOFFSET_SLEEP_PERIOD_far, (BYTE_P)&sleep, sizeof(uint32_t));
  return sleep;
}

BOOL ZAF_pm_IsActive(void)
{
  return (0xFF != timerHandle);
}

BOOL ZAF_pm_WakeUpIsActive(void)
{
  return wakeUpCCactive;
}

void ZAF_pm_ActivateCCWakeUp(void)
{
  ZW_DEBUG_BATT_SEND_STR("**\r\nZAF_pm_ActivateCCWakeUp");
  ZW_DEBUG_BATT_SEND_NL();

  wakeUpCCactive = TRUE;
  ZAF_pm_KeepAwakeAuto();
}

void ZAF_pm_DeactivateCCWakeUp(void)
{
  ZW_DEBUG_BATT_SEND_STR("**\r\nZAF_pm_DeactivateCCWakeUp");
  ZW_DEBUG_BATT_SEND_NL();
  wakeUpCCactive = FALSE;
}

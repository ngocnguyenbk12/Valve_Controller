/***************************************************************************
*
* Copyright (c) 2001-2013
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: Implements functions that make is easier to support
*              Battery Operated Nodes
*
* Author: Thomas Roll
*
* Last Changed By: $Author: tro $
* Revision: $Revision: 0.00 $
* Last Changed: $Date: 2013/06/25 14:38:36 $
*
****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_sysdefs.h>
#include <ZW_tx_mutex.h>
/* Enhanced Slave - needed for battery operation (RTC timer) on 100 series */
/* 200 Series have WUT */
#ifdef ZW_SLAVE_32
#include <ZW_slave_32_api.h>
#else
#ifdef  ZW_SLAVE
#include <ZW_slave_api.h>
#endif
#endif
#include <ZW_adcdriv_api.h>

/* Allows data storage of application data even after reset */
#include <ZW_non_zero.h>
#include <battery_monitor.h>
#include <association_plus.h>
#include <ZW_uart_api.h>
#include <CommandClassBattery.h>
#include "misc.h"
#include <ZW_basis_api.h>


/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/
extern XBYTE lowBattReportAcked; /*Defined in battery_non_zero_vars.c*/
extern XBYTE st_battery;         /*Defined in battery_non_zero_vars.c*/
static BYTE lowBattReportOnceAwake = FALSE;
/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
#ifdef ZW_DEBUG_BATT_MON
#define ZW_DEBUG_BATT_MON_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_BATT_MON_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_BATT_MON_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_BATT_MON_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_BATT_MON_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_BATT_MON_SEND_BYTE(data)
#define ZW_DEBUG_BATT_MON_SEND_STR(STR)
#define ZW_DEBUG_BATT_MON_SEND_NUM(data)
#define ZW_DEBUG_BATT_MON_SEND_WORD_NUM(data)
#define ZW_DEBUG_BATT_MON_SEND_NL()
#endif

typedef enum _EV_CHECK_BATT_
{
  EV_CHECK_BATT_FULL,
  EV_CHECK_BATT_HIGH,
  EV_CHECK_BATT_LOW,
  EV_CHECK_BATT_DEAD
} EV_BATT;

/* Calculation of battery levels*
#define ADC_RES      256
#define ADC_REF_MV   1175  //Hardcoded reference value trimmet from ZDB3502
#define BATT_FULL_MV  3600
#define BATT_LOW_MV  2750
#define BATT_DEAD_MV  2650
#define ADC_RESOLUTION ADC_8_BIT
#define ADC_STEPS ((ADC_RESOLUTION == ADC_8_BIT) ? (256) : (4096))
*/
#define BATT_HIGH_ADC_THRES       83  //((ADC_REF_MV * ADC_STEPS)/BATT_FULL_MV);
#define BATT_LOW_ADC_THRES       109  //((ADC_REF_MV * ADC_STEPS)/BATT_LOW_MV);
#define BATT_DEAD_ADC_THRES      113  //((ADC_REF_MV * ADC_STEPS)/BATT_DEAD_MV);
#define HYS_UPPER_THRESS           5
/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/
BATT_LEVEL m_battLevel;


/****************************************************************************/
/*                           PRIVATE FUNCTIONS                             */
/****************************************************************************/
BOOL BatteryMonitorStateMan(EV_BATT ev_batt);

/*==============================   initBatterySensor   ============================
**
**  Function:  Required function for the Battery Command clas
**
**  This funciton inialize the Battery Voltage sensor HW
**  Side effects: None
**
**--------------------------------------------------------------------------*/
void initBatterySensor()
{
  /*initalise the the adc in battery monitor mode, note that the reference , and input are ignored in the battery monitor mode*/
  /*The adc can be initaised any where in the application but in this sample it done in here this we will only use */
  /*in battery monitor mode*/
  ZW_ADC_init(ADC_BATT_SINGLE_MODE, ADC_REF_U_VDD, ADC_REF_L_VSS, 0);
  ZW_ADC_resolution_set(ADC_8_BIT); /*we use 8 bit resolution since it faster and use less code space*/
}

/*==============================   BatterySensorRead   ============================
**
**  Function:  Required function for the Battery Command class
**
**  This funciton read the Battery voltage from the battery Voltage sensor HW
**  Side effects: None
**
**--------------------------------------------------------------------------*/

BOOL
BatterySensorRead(BATT_LEVEL *battLvl )
{
  BOOL stateChange = FALSE;
  /* Check battery state*/
  switch(st_battery)
  {
    case ST_BATT_FULL:
      if (battLvl){
        *battLvl = BATT_FULL_LEV;
      }
      /*Check battery level is under high level*/
      if(TRUE == BatteryMonitorStateMan(EV_CHECK_BATT_HIGH))
      {
          if (battLvl){
            *battLvl = BATT_HIGH_LEV;
          }
        /*Check battery level is under high level*/
        if(TRUE == BatteryMonitorStateMan(EV_CHECK_BATT_LOW))
        {
          if (battLvl){
            *battLvl = BATT_LOW_LEV;
          }
          /*Check also if battery is under dead level*/
          if(BatteryMonitorStateMan(EV_CHECK_BATT_DEAD))
          {
            if (battLvl){
              *battLvl = BATT_DEAD_LEV;
            }
          }
        }
        stateChange = TRUE;
      }
      break;
    case ST_BATT_HIGH:
      if (battLvl){
        *battLvl = BATT_HIGH_LEV;
      }
      /*Check battery level is under low level*/
      if(TRUE == BatteryMonitorStateMan(EV_CHECK_BATT_LOW))
      {
        if (battLvl){
          *battLvl = BATT_LOW_LEV;
        }
        /*Check also if battery is under dead level*/
        if(BatteryMonitorStateMan(EV_CHECK_BATT_DEAD))
        {
          if (battLvl){
            *battLvl = BATT_DEAD_LEV;
          }
        }
        stateChange = TRUE;
      }
      /*Check battery is charing*/
      if(TRUE == BatteryMonitorStateMan(EV_CHECK_BATT_FULL))
      {
        if (battLvl){
          *battLvl = BATT_FULL_LEV;
        }
        stateChange = TRUE;
      }
      break;
    case ST_BATT_LOW:
      if (battLvl){
        *battLvl = BATT_LOW_LEV;
      }
      /*Check battery level is under DEAD level*/
      if(TRUE == BatteryMonitorStateMan(EV_CHECK_BATT_DEAD))
      {
        if (battLvl){
          *battLvl = BATT_DEAD_LEV;
        }
        stateChange =  TRUE;
      }
      /*Check battery is charing*/
      if(TRUE == BatteryMonitorStateMan(EV_CHECK_BATT_FULL))
      {
        if (battLvl){
          *battLvl = BATT_FULL_LEV;
        }
        stateChange = TRUE;
      }
      break;
    case ST_BATT_DEAD:
      if (battLvl){
        *battLvl = BATT_DEAD_LEV;
      }
      /*Check battry is charing*/
      if(TRUE == BatteryMonitorStateMan(EV_CHECK_BATT_HIGH))
      {
        if (battLvl){
          *battLvl = BATT_HIGH_LEV;
        }
        stateChange = TRUE;
      }
      /*Check battery is charing*/
      if(TRUE == BatteryMonitorStateMan(EV_CHECK_BATT_FULL))
      {
        if (battLvl){
          *battLvl = BATT_FULL_LEV;
        }
        stateChange = TRUE;
      }
      break;
  }
  if(TRUE == stateChange)
  {
    /* Reset flag because state is changed*/
    lowBattReportOnceAwake = FALSE;

    ActivateBattNotificationTrigger();
  }
  return stateChange;
}

BOOL TimeToSendBattReport(void)
{
  ZW_DEBUG_BATT_MON_SEND_STR("\r\nTimeToSendBattReport");
  ZW_DEBUG_BATT_MON_SEND_NL();
  if (0 == GetMyNodeID())
  {
    return FALSE;
  }
  BatterySensorRead(&m_battLevel);
  /*we send the report frame only one time on every wakeup
    and when no ACK received from CSC node for a prevouis report frame
    and the batt level is low*/
  if (!lowBattReportAcked && !lowBattReportOnceAwake)
  {
    return TRUE;
  }
  return FALSE;
}

JOB_STATUS SendBattReport(VOID_CALLBACKFUNC(pCallback)(TRANSMISSION_RESULT * pTransmissionResult))
{
  AGI_PROFILE lifelineProfile = {
      ASSOCIATION_GROUP_INFO_REPORT_PROFILE_GENERAL_NA_V2,
      ASSOCIATION_GROUP_INFO_REPORT_PROFILE_GENERAL_LIFELINE
  };
  JOB_STATUS status;

  ZW_DEBUG_BATT_MON_SEND_STR("\r\nSendBattReport");
  ZW_DEBUG_BATT_MON_SEND_NL();

  /*we send the report frame only one time on every wakeup
    and when no ACK received from CSC node for a prevouis report frame
    and the batt level is low*/
  status= CmdClassBatteryReport(
                             &lifelineProfile,
                             ENDPOINT_ROOT,
                             m_battLevel,
                             pCallback);
  if(JOB_STATUS_SUCCESS == status)
  {
    ZW_DEBUG_BATT_MON_SEND_NL();
    ZW_DEBUG_BATT_MON_SEND_BYTE('s');
    lowBattReportAcked = TRUE;
  }
  /* We have tried to send battery report. set flag */
  lowBattReportOnceAwake = TRUE;
  ZW_DEBUG_BATT_MON_SEND_STR(" level ");
  ZW_DEBUG_BATT_MON_SEND_NUM(m_battLevel);
  ZW_DEBUG_BATT_MON_SEND_NL();
  return status;
}
/*============================ SetLowBattReport ===============================
** Function description
** Set status if Lowbatt report should be active og deactive. FALSE is deactive
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
ActivateBattNotificationTrigger(void)
{
  lowBattReportAcked = FALSE;
}
/*============================ InitBatteryMonitor ===============================
** Function description
** Init Battery module
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
InitBatteryMonitor(wakeup_reason_t wakeUpReason)
{
  lowBattReportOnceAwake = FALSE;

  /*Init application battery HW*/
  initBatterySensor();

  if ((SW_WAKEUP_RESET == wakeUpReason) || (SW_WAKEUP_POR == wakeUpReason))
  {
    ZW_DEBUG_BATT_MON_SEND_STR("IBattMon RES! ");
    ZW_DEBUG_BATT_MON_SEND_NUM(wakeUpReason);
    ZW_DEBUG_BATT_MON_SEND_NL();
    st_battery = ST_BATT_FULL;
    lowBattReportAcked = FALSE;
  }
}




/*============================ SetADC ===============================
** Function description
** Setup ADC treshold and treshold mode
**
** Side effects:
**
**-------------------------------------------------------------------------*/
BOOL
SetADC( BYTE threshold, BYTE thresholdMode)
{
  BOOL treshold_changed = FALSE;
  ZW_DEBUG_BATT_MON_SEND_STR("SetADC treshold ");
  ZW_DEBUG_BATT_MON_SEND_NUM(threshold);
  ZW_DEBUG_BATT_MON_SEND_STR(" mode ");
  ZW_DEBUG_BATT_MON_SEND_NUM(thresholdMode);
  ZW_DEBUG_BATT_MON_SEND_NL();
  /*set the threshold mode so that the ADC is trigggered if the conversion value is higher than the threshold*/
  /*The higher the adc value the lower the battery level*/
  ZW_ADC_threshold_mode_set(thresholdMode);
  /*The battery low level voltage should always be higher than the POR limit which has max value of 2.22 volte */
  /*In our sample we set the low level threshold to 2.5*/
  ZW_ADC_threshold_set(threshold);
  ZW_ADC_power_enable(TRUE);
  ZW_ADC_enable(TRUE);
  while ( ZW_ADC_result_get() == ADC_NOT_FINISHED );

  if (ZW_ADC_is_fired())
  {
    treshold_changed = TRUE;
  }

  ZW_ADC_enable(FALSE);
  ZW_ADC_power_enable(FALSE);
  return treshold_changed;
}


/*============================ BatteryStateMan ===============================
** Function description
** Battery Monitor state event machine.
**
** return true if state is changed.
**
**-------------------------------------------------------------------------*/
BOOL
BatteryMonitorStateMan(EV_BATT ev_batt)
{
  BOOL changeState = FALSE;
  ZW_DEBUG_BATT_MON_SEND_STR("BMSM ST ");
  ZW_DEBUG_BATT_MON_SEND_NUM(st_battery);
  ZW_DEBUG_BATT_MON_SEND_STR(" EV ");
  ZW_DEBUG_BATT_MON_SEND_NUM(ev_batt);
  ZW_DEBUG_BATT_MON_SEND_NL();
  switch(st_battery)
  {
    case ST_BATT_FULL:
      if(EV_CHECK_BATT_HIGH == ev_batt)
      {
        if(TRUE == SetADC(BATT_HIGH_ADC_THRES, ADC_THRES_UPPER))
        {
          st_battery = ST_BATT_HIGH;
          changeState = TRUE;
        }
      }
      else if(EV_CHECK_BATT_LOW == ev_batt)
      {
        if(TRUE == SetADC(BATT_LOW_ADC_THRES, ADC_THRES_UPPER))
        {
          st_battery = ST_BATT_LOW;
          changeState = TRUE;
        }
      }
      else if(EV_CHECK_BATT_DEAD == ev_batt)
      {
        if(TRUE == SetADC(BATT_DEAD_ADC_THRES, ADC_THRES_UPPER))
        {
          st_battery = ST_BATT_DEAD;
          changeState = TRUE;
        }
      }
      break;
    case ST_BATT_HIGH:
      if(EV_CHECK_BATT_FULL == ev_batt)
      {
        /*Check charging*/
        if(TRUE == SetADC(BATT_HIGH_ADC_THRES - HYS_UPPER_THRESS, ADC_THRES_LOWER))
        {
          st_battery = ST_BATT_FULL;
          changeState = TRUE;
        }
      }

      if(EV_CHECK_BATT_LOW == ev_batt)
      {
        if(TRUE == SetADC(BATT_LOW_ADC_THRES, ADC_THRES_UPPER))
        {
          st_battery = ST_BATT_LOW;
          changeState = TRUE;
        }
      }
      else if(EV_CHECK_BATT_DEAD == ev_batt)
      {
        if(TRUE == SetADC(BATT_DEAD_ADC_THRES, ADC_THRES_UPPER))
        {
          st_battery = ST_BATT_DEAD;
          changeState = TRUE;
        }
      }
      break;
    case ST_BATT_LOW:
      if(EV_CHECK_BATT_FULL == ev_batt)
      {
        /*Check charging*/
        if(TRUE == SetADC(BATT_HIGH_ADC_THRES , ADC_THRES_LOWER))
        {
          st_battery = ST_BATT_FULL;
          changeState = TRUE;
        }
      }
      if(EV_CHECK_BATT_HIGH == ev_batt)
      {
        /*Check charging*/
        if(TRUE == SetADC(BATT_LOW_ADC_THRES - HYS_UPPER_THRESS, ADC_THRES_LOWER))
        {
          st_battery = ST_BATT_HIGH;
          changeState = TRUE;
        }
      }
      else if(EV_CHECK_BATT_DEAD == ev_batt)
      {
        if(TRUE == SetADC(BATT_DEAD_ADC_THRES, ADC_THRES_UPPER))
        {
          st_battery = ST_BATT_DEAD;
          changeState = TRUE;
        }
      }
      break;
    case ST_BATT_DEAD:
      if(EV_CHECK_BATT_HIGH == ev_batt)
      {
        /*Only battery level hihger than BATT_LOW_ADC_THRES bring device out of ST_BATT_DEAD!*/
        if(TRUE == SetADC(BATT_LOW_ADC_THRES - HYS_UPPER_THRESS, ADC_THRES_LOWER))
        {
          st_battery = ST_BATT_HIGH;
          changeState = TRUE;
        }
      }
      if(EV_CHECK_BATT_FULL == ev_batt)
      {
        /*Check charging*/
        if(TRUE == SetADC(BATT_HIGH_ADC_THRES, ADC_THRES_LOWER))
        {
          st_battery = ST_BATT_FULL;
          changeState = TRUE;
        }
      }
      break;
  }

  if( TRUE == changeState)
  {
    ZW_DEBUG_BATT_MON_SEND_STR("new state ");
    ZW_DEBUG_BATT_MON_SEND_NUM(st_battery);
    ZW_DEBUG_BATT_MON_SEND_NL();
  }

  return changeState;
}



/*============================ BatteryMonitorState ===============================
** Function description
** This function...
**
** Side effects:
**
**-------------------------------------------------------------------------*/
ST_BATT
BatteryMonitorState(void)
{
  return st_battery;
}


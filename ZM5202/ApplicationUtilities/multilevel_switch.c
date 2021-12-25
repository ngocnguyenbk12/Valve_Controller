/***************************************************************************
*
* Copyright (c) 2001-2011
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: Multilevel switch utilites source file
*
* Author: Samer Seoud
*
* Last Changed By:  $Author:  $
* Revision:         $Revision:  $
* Last Changed:     $Date:  $
*
****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_basis_api.h>
#include <ZW_tx_mutex.h>
#include <ZW_TransportLayer.h>
#include <ZW_uart_api.h>
#include "config_app.h"
#include <CommandClassMultilevelSwitch.h>
#include <misc.h>
#include <endpoint_lookup.h>
#include <multilevel_switch.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

#ifdef ZW_DEBUG_MULTISWITCH
#define ZW_DEBUG_MULTISWITCH_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_MULTISWITCH_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_MULTISWITCH_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_MULTISWITCH_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_MULTISWITCH_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_MULTISWITCH_SEND_BYTE(data)
#define ZW_DEBUG_MULTISWITCH_SEND_STR(STR)
#define ZW_DEBUG_MULTISWITCH_SEND_NUM(data)
#define ZW_DEBUG_MULTISWITCH_SEND_WORD_NUM(data)
#define ZW_DEBUG_MULTISWITCH_SEND_NL()
#endif

#define SWITCH_ENDPOINT_IDX  endpointidx

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

#define SWITCH_DIMMING_UP   0x01
#define SWITCH_IS_DIMMING   0x02
#define SWITCH_IS_ON        0x04

typedef struct __MultiLvlSwitch_
{
  BYTE bOnStateSwitchLevel;  /*save the on state level value when we set the HW to off*/
  BYTE bCurrentSwitchLevel;  /*hold the current switch level value*/
  BYTE bTargetSwitchLevel;   /*hold the value we want to set the switch to when we are changing*/
  DWORD lTicksCountReload;
  DWORD lTicksCount;
  BYTE switchFlag;
}_MultiLvlSwitch;


static _MultiLvlSwitch MultiLvlSwitch[SWITCH_MULTI_ENDPOINTS];
static BYTE bMultiLevelSwTimerHandle;
static ENDPOINT_LOOKUP multiLevelEpLookup;

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/
void ZCB_SwitchLevelHandler(void);

PCB(ZCB_SwitchLevelHandler)(void)
{
  BYTE i;
  BOOL boAllTimersOff = TRUE;
  for (i = 0; i < GetEndPointCount(&multiLevelEpLookup); i++)
  {
    if (!(MultiLvlSwitch[i].switchFlag & SWITCH_IS_DIMMING))
      continue;
    if (!--MultiLvlSwitch[i].lTicksCount)
    {
      (MultiLvlSwitch[i].switchFlag & SWITCH_DIMMING_UP)? MultiLvlSwitch[i].bCurrentSwitchLevel++: MultiLvlSwitch[i].bCurrentSwitchLevel--;
      CommandClassMultiLevelSwitchSet(MultiLvlSwitch[i].bCurrentSwitchLevel, FindEndPointID(&multiLevelEpLookup,i));
      if (MultiLvlSwitch[i].bCurrentSwitchLevel == MultiLvlSwitch[i].bTargetSwitchLevel)
      {
        MultiLvlSwitch[i].switchFlag &= ~SWITCH_IS_DIMMING;
        MultiLvlSwitch[i].bCurrentSwitchLevel =  CommandClassMultiLevelSwitchGet( FindEndPointID(&multiLevelEpLookup,i) );
        if (MultiLvlSwitch[i].bCurrentSwitchLevel)
          MultiLvlSwitch[i].bOnStateSwitchLevel = MultiLvlSwitch[i].bCurrentSwitchLevel;
      }
      else
      {
        MultiLvlSwitch[i].lTicksCount =  MultiLvlSwitch[i].lTicksCountReload;
        boAllTimersOff = FALSE;
      }
    }
    else
    {
      boAllTimersOff = FALSE;
    }
  }
  if (boAllTimersOff)
  {
    TimerCancel(bMultiLevelSwTimerHandle);
    bMultiLevelSwTimerHandle = 0;
  }
}

/**
 * Starts a timer that does the level changing.
 * @param bEndPointIdx
 * @param bDuration
 * @param blevels
 */
static void StartLevelChange(BYTE bEndPointIdx, BYTE bDuration, BYTE blevels)
{
  BYTE endpointID = FindEndPointID(&multiLevelEpLookup,bEndPointIdx);

  if ((bDuration == 0) || (bDuration == 0xFF)) /*set the level instantly*/
                                               /*0xff is used to indicate factory default duration whic is 0 in our case.*/
  {
    CommandClassMultiLevelSwitchSet( MultiLvlSwitch[bEndPointIdx].bTargetSwitchLevel, endpointID );
    MultiLvlSwitch[bEndPointIdx].bCurrentSwitchLevel = CommandClassMultiLevelSwitchGet(endpointID);
    if (MultiLvlSwitch[bEndPointIdx].bCurrentSwitchLevel)
    {
      MultiLvlSwitch[bEndPointIdx].bOnStateSwitchLevel = MultiLvlSwitch[bEndPointIdx].bCurrentSwitchLevel;
    }
  }
  else
  {
    if (bDuration> 0x7F) /*duration is in minutes*/
    {
      MultiLvlSwitch[bEndPointIdx].lTicksCountReload = bDuration - 0x7F;
      /*convert the minutes to 10ms units */
      MultiLvlSwitch[bEndPointIdx].lTicksCountReload *= 6000; /*convert the minutes to 10ms units*/
    }
    else
    {/*duration in seconds*/
      MultiLvlSwitch[bEndPointIdx].lTicksCountReload = bDuration;
      MultiLvlSwitch[bEndPointIdx].lTicksCountReload *= 100; /*convert the seconds to 10ms units*/
    }
    /*calculate the number of 10ms ticks between each level change*/
    MultiLvlSwitch[bEndPointIdx].lTicksCountReload /= blevels;
    MultiLvlSwitch[bEndPointIdx].lTicksCount = MultiLvlSwitch[bEndPointIdx].lTicksCountReload; /* lTicksCount is used to count down */
    MultiLvlSwitch[bEndPointIdx].switchFlag |= SWITCH_IS_DIMMING;

    if (!bMultiLevelSwTimerHandle)
      bMultiLevelSwTimerHandle = TimerStart(ZCB_SwitchLevelHandler, 1,  TIMER_FOREVER);
  }
}

BYTE GetTargetLevel(uint8_t endpoint)
{
  BYTE endpointidx = 0;
  endpointidx = FindEndPointIndex(&multiLevelEpLookup,endpoint);
  if (endpointidx == 0xFF)
    return 0;

  if (MultiLvlSwitch[SWITCH_ENDPOINT_IDX].switchFlag & SWITCH_IS_DIMMING )
  {
    return MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bTargetSwitchLevel;
  }
  else
  {
    return MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bCurrentSwitchLevel;
  }
}

BYTE GetCurrentDuration(uint8_t endpoint)
{
  DWORD tmpDuration;
  BYTE tmpLvl;
//#ifdef ENDPOINT_SUPPORT
  BYTE endpointidx = 0;
  endpointidx = FindEndPointIndex(&multiLevelEpLookup,endpoint);
  if (0xFF == endpointidx)
  {
    ZW_DEBUG_MULTISWITCH_SEND_BYTE('a');
    return 0xFE;
  }

  if (MultiLvlSwitch[SWITCH_ENDPOINT_IDX].switchFlag & SWITCH_IS_DIMMING )
  {
    if (MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bTargetSwitchLevel > MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bCurrentSwitchLevel)
    {
      tmpLvl = MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bTargetSwitchLevel - MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bCurrentSwitchLevel;
    }
    else
    {
      tmpLvl = MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bCurrentSwitchLevel - MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bTargetSwitchLevel;
    }
    /*convert duration from 10ms ticks per level to seconds*/
    /*lTicksCountReload hold the number of 10ms ticks per level*/
    /*tmpLvl hild the number of remaining levels to reach the target.*/
    tmpDuration = (MultiLvlSwitch[SWITCH_ENDPOINT_IDX].lTicksCountReload * tmpLvl)/100 ;
    if (tmpDuration > 127)
    {
      tmpDuration /= 60; /*convert to minuts*/
      tmpDuration += 127; /*add offset according to command class specification*/
    }
    return (BYTE) tmpDuration;
  }
  else
  {
    return 0;
  }
}

void StopSwitchDimming( BYTE endpoint)
{
  BYTE endpointidx = 0;

  endpointidx = FindEndPointIndex(&multiLevelEpLookup,endpoint);
  if (endpointidx == 0xFF)
    return;

  if (MultiLvlSwitch[SWITCH_ENDPOINT_IDX].switchFlag & SWITCH_IS_DIMMING)
  {
    MultiLvlSwitch[SWITCH_ENDPOINT_IDX].switchFlag &= ~SWITCH_IS_DIMMING;
    MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bCurrentSwitchLevel = CommandClassMultiLevelSwitchGet(endpoint);
    if (MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bCurrentSwitchLevel)
      MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bOnStateSwitchLevel = MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bCurrentSwitchLevel;
  }
}

void HandleStartChangeCmd( BYTE bStartLevel,
                     BOOL boIgnoreStartLvl,
                     BOOL boDimUp,
                     BYTE bDimmingDuration,
                     BYTE endpoint )
{
  BYTE endpointidx = 0;
  endpointidx = FindEndPointIndex(&multiLevelEpLookup,endpoint);
  if (endpointidx == 0xFF)
    return;

      /*primary switch Up/Down bit field value are Up*/
  if (boDimUp)
  {
    MultiLvlSwitch[SWITCH_ENDPOINT_IDX].switchFlag |= SWITCH_DIMMING_UP;
    MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bTargetSwitchLevel = 99;
  }
  else
  {
    MultiLvlSwitch[SWITCH_ENDPOINT_IDX].switchFlag &= ~SWITCH_DIMMING_UP; /*we assume the up/down flag is down*/
    MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bTargetSwitchLevel = 0;
  }

  if (!boIgnoreStartLvl)
  {
    if (bStartLevel == 0xFF) /*On state*/
    {/*if we are in off state set the target level to the most recent non-zero level value*/
      bStartLevel = MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bOnStateSwitchLevel;
    }
    else if (bStartLevel == 0x00)
    {/*set off state */
      if (MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bCurrentSwitchLevel)  /*we are in off state then save the current level */
        MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bOnStateSwitchLevel = MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bCurrentSwitchLevel;
    }

  CommandClassMultiLevelSwitchSet( bStartLevel, endpoint);
  }
  MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bCurrentSwitchLevel = CommandClassMultiLevelSwitchGet(endpoint);

  if (MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bTargetSwitchLevel != MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bCurrentSwitchLevel)
  {
     StartLevelChange(SWITCH_ENDPOINT_IDX, bDimmingDuration, 100);
  }
}

BOOL CC_MLS_Set_handler(uint8_t bTargetlevel, uint8_t bDuration, uint8_t endpoint)
{
  if (0xFF == FindEndPointIndex(&multiLevelEpLookup, endpoint))
  {
    return FALSE;
  }
  CC_MultilevelSwitch_SetValue(bTargetlevel, bDuration, endpoint);
  return TRUE;
}

void CC_MultilevelSwitch_SetValue( BYTE bTargetlevel,
             BYTE bDuration,
             BYTE endpoint )
{
  BYTE levels;

  BYTE endpointidx = 0;
  endpointidx = FindEndPointIndex(&multiLevelEpLookup, endpoint);
  if (endpointidx == 0xFF)
  {
    return;
  }

  StopSwitchDimming(endpoint);

  MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bTargetSwitchLevel = bTargetlevel;
  MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bCurrentSwitchLevel = CC_MultilevelSwitch_GetCurrentValue_handler(endpoint);

  if (MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bTargetSwitchLevel == 0xFF) /*On state*/
  {/*if we are in off state set the target level to the most recent non-zero level value*/
    if (!MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bCurrentSwitchLevel)
    {
      if(MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bOnStateSwitchLevel)
      {
        MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bTargetSwitchLevel = MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bOnStateSwitchLevel;
      }
      else
      {
        MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bTargetSwitchLevel = 0x63; /*last level was 0. Set it to full level*/
      }
    }
    else
    {
      return; /*we are already on then ignore the on command*/
    }
  }
  else if (MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bTargetSwitchLevel == 0x00)
  {/*set off state */

    if (MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bCurrentSwitchLevel)  /*we are in off state then save the current level */
    {
      MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bOnStateSwitchLevel = MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bCurrentSwitchLevel;
    }
    else
    {
      return; /*we are already off then ignore the off command*/
    }
  }

  if (MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bCurrentSwitchLevel < MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bTargetSwitchLevel)
  {
    MultiLvlSwitch[SWITCH_ENDPOINT_IDX].switchFlag |= SWITCH_DIMMING_UP;
    levels = MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bTargetSwitchLevel - MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bCurrentSwitchLevel;

  }
  else
  {
    MultiLvlSwitch[SWITCH_ENDPOINT_IDX].switchFlag &= ~SWITCH_DIMMING_UP;
    levels = MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bCurrentSwitchLevel - MultiLvlSwitch[SWITCH_ENDPOINT_IDX].bTargetSwitchLevel;
  }
  StartLevelChange(SWITCH_ENDPOINT_IDX, bDuration, levels);

}

BOOL SetSwitchHwLevel(BYTE bInitHwLevel, BYTE endpoint )
{
  BYTE i;
  if (endpoint)
  {
    i =  FindEndPointIndex(&multiLevelEpLookup,endpoint);
    if ( i != 0xFF)
    {
      MultiLvlSwitch[i].bCurrentSwitchLevel = bInitHwLevel;
      MultiLvlSwitch[i].bOnStateSwitchLevel = bInitHwLevel;
      return TRUE;
    }
  }
  return FALSE;
}

void MultiLevelSwitchInit(uint8_t bEndPointCount, uint8_t const * const pEndPointList)
{
  BYTE i;
  multiLevelEpLookup.bEndPointsCount = bEndPointCount;
  multiLevelEpLookup.pEndPointList = (uint8_t *)pEndPointList;
  for (i = 0; i < bEndPointCount; i++)
  {
    MultiLvlSwitch[i].bCurrentSwitchLevel = 0;
    MultiLvlSwitch[i].bOnStateSwitchLevel = 0;
    MultiLvlSwitch[i].lTicksCount = 0;
    MultiLvlSwitch[i].switchFlag = 0;
  }
  bMultiLevelSwTimerHandle = 0;
}


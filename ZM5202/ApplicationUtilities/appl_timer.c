/**
 * @file appl_timer.c
 * @brief Timer service functions that handle delayed functions calls.
 * @details The time resolution is 1 second.
 * Features:
 *  1. Timer Start.
 *  2. Timer Stop.
 *  3. Timer Restart.
 *  4. Get time passed since start.
 *  5. Runs once, many times or forever.
 *
 * Copyright (c) 2001-2015
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
* Last Changed By: $Author: tro $
* Revision: $Revision: 0.00 $
* Last Changed: $Date: 2013/06/25 14:38:36 $
 *
 * @date 25/06/2013
 * @author Samer Seoud
 */

#include "config_lib.h"

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_basis_api.h>
#include <appl_timer_api.h>
#include <misc.h>

#ifdef ZW_DEBUG_APPL_TIMER
#include <ZW_uart_api.h>
#define ZW_DEBUG_APPL_TIMER_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_APPL_TIMER_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_APPL_TIMER_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_APPL_TIMER_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_APPL_TIMER_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_APPL_TIMER_SEND_BYTE(data)
#define ZW_DEBUG_APPL_TIMER_SEND_STR(STR)
#define ZW_DEBUG_APPL_TIMER_SEND_NUM(data)
#define ZW_DEBUG_APPL_TIMER_SEND_WORD_NUM(data)
#define ZW_DEBUG_APPL_TIMER_SEND_NL()
#endif


/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/
typedef struct _ZWAVEPLUS_APPL_TIMER_ELEMENT_
{
  DWORD  startTime;
  DWORD  tickCount;
  DWORD  timeoutValue;
  VOID_CALLBACKFUNC(func)(void);
  BYTE  repeats;
} ZWAVEPLUS_APPL_TIMER_ELEMENT;

static BOOL singleTon = FALSE;

static ZWAVEPLUS_APPL_TIMER_ELEMENT applTimerArray[APPL_TIMER_MAX];
static DWORD applTickTime;  /* global counter that is incremented every 10 msec */
/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/
void ZCB_ApplTimerAction(void);

PCB(ZCB_ApplTimerAction)(void)
{
  BYTE timerHandle;
  applTickTime++;
  ZW_DEBUG_APPL_TIMER_SEND_BYTE('.');
  /* walk through the timer array */
  for (timerHandle = 0; timerHandle < APPL_TIMER_MAX; timerHandle++)
  {
    if (applTimerArray[timerHandle].tickCount)
    {
      if (--applTimerArray[timerHandle].tickCount == 0) /* count change from 1 to 0 */
      {
        if (applTimerArray[timerHandle].repeats)
        { /* repeat timer action */
          applTimerArray[timerHandle].tickCount = applTimerArray[timerHandle].timeoutValue;
          if (applTimerArray[timerHandle].repeats != APPL_TIMER_FOREVER)
          { /* repeat limit */
            applTimerArray[timerHandle].repeats--;
          }
        }

        if (NON_NULL( applTimerArray[timerHandle].func ))
        {
          /* call the timeout function */
          applTimerArray[timerHandle].func();
        }
      }
    }
  }
}

/*===========================   ApplTimerGetTime   ==========================
**    Get the time passed since the start of the application timer
**    It return the time (in secs) passed since the slow timer is created
**    The time is identified with the timer handle
**--------------------------------------------------------------------------*/
DWORD /*RETURN: if it is less than 0xFFFFFFFF then it is the number of seconds passed since the timer was*/
      /*        created or restarted. If equal 0xFFFFFFFF then the timer handle is invalid*/
ApplTimerGetTime(BYTE btimerHandle)
{
  ZW_DEBUG_APPL_TIMER_SEND_BYTE('G');
  ZW_DEBUG_APPL_TIMER_SEND_NUM(btimerHandle);
  btimerHandle--;  /* Index 0...n */
  if ((btimerHandle < APPL_TIMER_MAX) && (applTimerArray[btimerHandle].timeoutValue))
  { /* valid timer element number */
    return applTickTime - applTimerArray[btimerHandle].startTime;
  }
  return (DWORD)-1;


}


/*================================   ApplTimerRestart  ===========================
**    Set the specified timer back to the initial value.
**    The timer start time will be sat´to the current value of the slow timer subsystem time
**--------------------------------------------------------------------------*/
BYTE                      /*RET TRUE if timer restarted   */
ApplTimerRestart(
  BYTE btimerHandle)       /* IN Timer number to restart   */
{
  ZW_DEBUG_APPL_TIMER_SEND_BYTE('R');
  ZW_DEBUG_APPL_TIMER_SEND_NUM(btimerHandle);
  btimerHandle--;  /* Index 0...n */
  if ((btimerHandle < APPL_TIMER_MAX) && (applTimerArray[btimerHandle].timeoutValue))
  { /* valid timer element number */
    applTimerArray[btimerHandle].tickCount = applTimerArray[btimerHandle].timeoutValue;
    applTimerArray[btimerHandle].startTime = applTickTime;
    return TRUE;
  }
  return(FALSE);
}


/*===============================   ApplTimerStop  ===========================
**    Stop the specified timer.
**    and set timerhandle to 0
**--------------------------------------------------------------------------*/
void
ApplTimerStop(
  BYTE *pbTimerHandle)
{
  ZW_DEBUG_APPL_TIMER_SEND_BYTE('C');
  ZW_DEBUG_APPL_TIMER_SEND_NUM(*pbTimerHandle);
  --*pbTimerHandle;  /* Index 0...n */
  if (*pbTimerHandle < APPL_TIMER_MAX)
  {
    /* valid timer element number */
    applTimerArray[*pbTimerHandle].startTime = 0;
    applTimerArray[*pbTimerHandle].tickCount = 0;
    applTimerArray[*pbTimerHandle].timeoutValue = 0; /* stop the timer */
  }
  *pbTimerHandle = 0;
}


/*============================   ApplTimerStart   =========================
** Creat a slow timer instance
**
**--------------------------------------------------------------------------*/
BYTE                        /* Returns 1 to APPL_TIMER_MAX the timer handle created, 0 no timer is created */
ApplTimerStart(
  VOID_CALLBACKFUNC(func)(void), /*IN  Timeout function address          */
  DWORD ltimerTicks,          /*IN  Timeout value (in seconds)  */
  BYTE brepeats)             /*IN  Number of function calls (-1: forever)  */
{
  BYTE timerHandle;

  ZW_DEBUG_APPL_TIMER_SEND_STR("ApplTimerStart");
  ZW_DEBUG_APPL_TIMER_SEND_WORD_NUM(ltimerTicks);
  ZW_DEBUG_APPL_TIMER_SEND_BYTE('.');
  ZW_DEBUG_APPL_TIMER_SEND_NUM(brepeats);
  ZW_DEBUG_APPL_TIMER_SEND_NL();

  for (timerHandle = 0; timerHandle < APPL_TIMER_MAX; timerHandle++)
  {
    /* find first free timer element */
    if (applTimerArray[timerHandle].tickCount == 0)
    {
      if (!ltimerTicks)
      {
        ltimerTicks++; /* min 1 sec. */
      }
      if (brepeats && (brepeats != APPL_TIMER_FOREVER))
      {
        brepeats--; /*brepeats = 0 then timer runs 1 timer, brepeats = 1 then timer run 2 times */
      }
      /* create new active timer element */
      applTimerArray[timerHandle].startTime = applTickTime;
      applTimerArray[timerHandle].timeoutValue = ltimerTicks;
      applTimerArray[timerHandle].tickCount = ltimerTicks;

      ZW_DEBUG_APPL_TIMER_SEND_BYTE('T');
      ZW_DEBUG_APPL_TIMER_SEND_NUM((BYTE)(ltimerTicks>>24));
      ZW_DEBUG_APPL_TIMER_SEND_NUM((BYTE)(ltimerTicks>>16));
      ZW_DEBUG_APPL_TIMER_SEND_NUM((BYTE)(ltimerTicks>>8));
      ZW_DEBUG_APPL_TIMER_SEND_NUM((BYTE)ltimerTicks);

      applTimerArray[timerHandle].repeats = brepeats;
      applTimerArray[timerHandle].func = func;

      return (timerHandle + 1);
    }
  }
  ZW_DEBUG_APPL_TIMER_SEND_BYTE('S');
  return (0);
}

BOOL
ApplTimerInit(void)
{
  if(FALSE == singleTon)
  {
    BYTE tmp;

    ZW_DEBUG_APPL_TIMER_SEND_STR("ApplTimerInit");
    ZW_DEBUG_APPL_TIMER_SEND_NL();

    singleTon = TRUE;
    for (tmp = 0; tmp < APPL_TIMER_MAX; tmp++)
    {
      applTimerArray[tmp].startTime = 0;
      applTimerArray[tmp].tickCount = 0;
      applTimerArray[tmp].timeoutValue = 0;
      applTimerArray[tmp].func = NULL;
      applTimerArray[tmp].repeats = 0;
    }
    applTickTime = 0;

    if (TimerStart(ZCB_ApplTimerAction, 100, 0xFF) == 0xFF){
      return FALSE;
    }
  }
  return TRUE;
}

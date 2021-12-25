/*******************************  ZW_EV_SCHEDULER.C  *******************************
 *           #######
 *           ##  ##
 *           #  ##    ####   #####    #####  ##  ##   #####
 *             ##    ##  ##  ##  ##  ##      ##  ##  ##
 *            ##  #  ######  ##  ##   ####   ##  ##   ####
 *           ##  ##  ##      ##  ##      ##   #####      ##
 *          #######   ####   ##  ##  #####       ##  #####
 *                                           #####
 *          Z-Wave, the wireless language.
 *
 *              Copyright (c) 2001
 *              Zensys A/S
 *              Denmark
 *
 *              All Rights Reserved
 *
 *    This source file is subject to the terms and conditions of the
 *    Zensys Software License Agreement which restricts the manner
 *    in which it may be used.
 *
 *---------------------------------------------------------------------------
 *
 * Description: Z-Wave event scheduler module
 *
 * Author:   Johann Sigfredsson
 *
 * Last Changed By:  $Author: tro $
 * Revision:         $Revision: 1.2 $
 * Last Changed:     $Date: 2008/03/18 11:51:46 $
 *
 ****************************************************************************/
/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include <ZW_ev_scheduler.h>
#include <ZW_typedefs.h>
#include <ZW_timer_api.h>

#include <ZW_uart_api.h>
#include <ZW_debug_api.h>
#include <misc.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
#define AREA_SIZE 3


typedef struct _eventQueueElement_
{
  BYTE handle;
  VOID_CALLBACKFUNC(pSchedApp)(void);
  struct _eventQueueElement_* pNext;
} eventQueueElement;


typedef struct _eventScheduler
{
  BYTE bEventTimerHandle;
  eventQueueElement* pRoot;
  BYTE area [AREA_SIZE] [1 + sizeof (eventQueueElement)];
} eventScheduler;


/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

eventScheduler myEventScheduler = {0, NULL};
eventQueueElement* pElementCurrent;
BOOL bEventTick = FALSE;

/****************************************************************************/
/*                            Functions used in library                     */
/****************************************************************************/
void *GetArea( void);
void ReleaseArea( void* ptr);
void ZCB_EventTicker(void);
void EventTimerStart(void);
void EventTimerStop(void);
BYTE FindHandle(eventQueueElement* pElement, VOID_CALLBACKFUNC(pSchedApp)(void));
BYTE NewHandle(eventQueueElement* pRoot);
void AddSchedApp(eventQueueElement* pElement, eventQueueElement* pNew);
eventQueueElement** FindSchedApp(eventQueueElement** ppElement, BYTE handle);


/*===============================   GetArea   ==============================
**    Allocate block of memory
**    [ret]      : Return pointer to allocated memory. No free memory if
**                 return pointer is NULL!
**--------------------------------------------------------------------------*/
void *GetArea( void)
{
  BYTE i;

  for (i = 0; i < AREA_SIZE; i++)
  {
    if (0 == myEventScheduler.area[i][0])
    {
      myEventScheduler.area[i][0] = 1;
      return (void*) &myEventScheduler.area[i][1];
    }
  }
  return NULL;

}


/*===============================   ReleaseArea   ============================
**    Free allocated memory.
**    [in/out] ptr: Pointer to allocated memory.Pointer is set to NULL.
**    [ret]       : None
**--------------------------------------------------------------------------*/
void ReleaseArea( void* ptr)
{

  if( NON_NULL( ptr ))
  {
    BYTE* ptrTemp = (BYTE*)ptr; /* free myEventScheduler.area[x][0]*/
    *(--ptrTemp) = 0;
    //*(ptr - 1) = 0;
  }
}



/*================================== ZCB_EventTicker =============================
**  Event Scheduler timer tick.
**  Sets bEventTick = TRUE if not bEventTick != FALSE
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
PCB(ZCB_EventTicker)(void)
{
  if (!bEventTick)
  {
    bEventTick = TRUE;
    pElementCurrent = myEventScheduler.pRoot;
  }
}


/*================================ EventTimerStart ===========================
**  Function
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
void
EventTimerStart(void)
{
  if (myEventScheduler.bEventTimerHandle)
  {
    EventTimerStop();
  }
  /* Start Event Timer, so that EventTimerTicker gets called every 20ms */
  myEventScheduler.bEventTimerHandle = TimerStart( ZCB_EventTicker, 2, TIMER_FOREVER);
}


/*================================== EventTimerStop ==========================
**  Function
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
void
EventTimerStop(void)
{
  if(TRUE == TimerCancel(myEventScheduler.bEventTimerHandle))
  {
    myEventScheduler.bEventTimerHandle = 0xff;
  }

}



/*================================== FindApp ==================================
**  Function search after application pointer and return handle. If not present
**  return 0.
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
BYTE     /*return handle*/
FindHandle(
  eventQueueElement* pElement,
  VOID_CALLBACKFUNC(pSchedApp)(void))
{
  while (pElement->pSchedApp != pSchedApp)
  {
    if ((pElement->pNext == NULL) || (pElement->pSchedApp == NULL))
    {
      return 0;
    }
    else
    {
      pElement = pElement->pNext;
    }
  }
  return pElement->handle;
}


/*================================== NewHandle ==============================
**  Function find a handle. Return 0 if no handle free!
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
BYTE
NewHandle(
  eventQueueElement* pRoot)
{
  BYTE handle = pRoot->handle + 1;
  eventQueueElement* pElement = pRoot;

  do
  {
    while (handle !=  pElement->handle)
    {
      if (IS_NULL( pElement->pNext ))
      {
        return handle;
      }
      else
      {
        pElement = pElement->pNext;
      }
    }
    pElement = pRoot;
  } while (0xff != ++handle);

  return 0;
}


/*================================== AddSchedApp ============================
**  Function
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
void
AddSchedApp(
  eventQueueElement* pElement, eventQueueElement* pNew)
{
  while (NULL != pElement->pNext)
  {
    pElement = pElement->pNext;
  }
  pElement->pNext = pNew;
}


/*================================ RemoveSchedApp ============================
**  Function
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
eventQueueElement**
FindSchedApp(
  eventQueueElement** ppElement, BYTE handle)
{
  while ((*ppElement)->handle != handle)
  {
    if ((*ppElement)->pNext == NULL)
    {
      return NULL;
    }
    *ppElement = (*ppElement)->pNext;
  }

  return ppElement;
}


/*================================== EventSchedulerAdd =======================
**  Function to add application call to scheduler
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
BYTE   /* event handle. failing if 0*/
EventSchedulerAdd(
  VOID_CALLBACKFUNC(pSchedApp)(void))
{
  BYTE i;
  BYTE handle = 0;

  //ZW_DEBUG_SEND_STR("EventSchedulerAdd:");


  /*First time*/
  if (IS_NULL( myEventScheduler.pRoot ))
  {
    //ZW_DEBUG_SEND_STR("root");
    /*first time. Clear all*/
    for (i = 0; i < AREA_SIZE; i++)
    {
      myEventScheduler.area[i][0] = 0;
    }
    //eventQueueElement* pElement = malloc(sizeof(eventQueueElement));
    myEventScheduler.pRoot = GetArea();
    if (NON_NULL( myEventScheduler.pRoot ))
    {
      myEventScheduler.pRoot->handle = 1;
      handle = myEventScheduler.pRoot->handle;
      myEventScheduler.pRoot->pSchedApp = pSchedApp;
      myEventScheduler.pRoot->pNext = NULL;
      //myEventScheduler.pRoot = pElement;
      myEventScheduler.bEventTimerHandle = 0xff;
      EventTimerStart();
    }
  }
  else
  {
    /*Search if ScedApp is in list*/
    handle = FindHandle(myEventScheduler.pRoot, pSchedApp);
    if (0 == handle)
    {
      //eventQueueElement* pElement = malloc( sizeof(eventQueueElement) );
      eventQueueElement* pElement = GetArea( );
      if (NON_NULL( pElement ))
      {
        handle = NewHandle(myEventScheduler.pRoot);
        if (0 != handle)
        {
          pElement->handle = handle;
          pElement->pSchedApp = pSchedApp;
          pElement->pNext = NULL;

          AddSchedApp(myEventScheduler.pRoot, pElement);
        }
        else
        {
          //free(pElement);
          ReleaseArea(pElement);

        }
      }
    }
  }
  //ZW_DEBUG_SEND_STR(":H:");
  //ZW_DEBUG_SEND_BYTE(handle);
  //ZW_DEBUG_SEND_NL();
  return handle;
}


/*================================== EventSchedulerRemove ====================
**  Function to remove application call from scheduler
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
BOOL
EventSchedulerRemove(
  BYTE* pHandle)
{
  //ZW_DEBUG_SEND_STR("EventSchedulerRemove H:");
  //ZW_DEBUG_SEND_BYTE(*pHandle);
  //ZW_DEBUG_SEND_NL();
  //eventQueueElement** pElement = FindSchedApp(&myEventScheduler.pRoot, *pHandle);
  if (IS_NULL( myEventScheduler.pRoot ))
  {
    /* No more Events */
    EventTimerStop();
  }
  else
  {
    eventQueueElement* pElement = myEventScheduler.pRoot;
    if (*pHandle == myEventScheduler.pRoot->handle)
    {
      /* We are root! */
      if (IS_NULL( myEventScheduler.pRoot->pNext ))
      {
        /* No more EventHandlers */
        EventTimerStop();
      }
      myEventScheduler.pRoot = myEventScheduler.pRoot->pNext;
      ReleaseArea(pElement);
      *pHandle = 0;
      return TRUE;
    }
    else
    {
      eventQueueElement* pPostelement;
      do
      {
        if (IS_NULL( pElement->pNext ))
        {
          /* handle is not present! */
          return FALSE;
        }
        else
        {
          pPostelement = pElement;
          pElement = pElement->pNext;
        }
      }
      while (*pHandle != pElement->handle);
      /*handle found!*/
      pPostelement->pNext = pElement->pNext;
      ReleaseArea(pElement);
      *pHandle = 0;
      return TRUE;
    }
  }
  return FALSE;
}


/*================================== ZCB_EventScheduler ==========================
**  Scheduler engine.
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
PCB(ZCB_eventScheduler)(
  void)
{
  if (bEventTick)
  {
    pElementCurrent->pSchedApp();
    pElementCurrent = pElementCurrent->pNext;
    if (IS_NULL( pElementCurrent ))
    {
      bEventTick = FALSE;
    }
  }
}

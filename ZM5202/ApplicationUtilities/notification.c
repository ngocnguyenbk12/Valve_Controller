/**
 * @file notification.c
 * @brief Command Class Notification helper module.
 * @author Thomas Roll
 * @copyright Copyright (c) 2001-2017
 * Sigma Designs, Inc.
 * All Rights Reserved
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <notification.h>
#include <CommandClassNotification.h>
#include "config_app.h"
#include "eeprom.h"
#include <ZW_uart_api.h>
#include <ZW_mem_api.h>
/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
#ifdef ZW_DEBUG_NOTIFICATION
#define ZW_DEBUG_NOTIFICATION_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_NOTIFICATION_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_NOTIFICATION_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_NOTIFICATION_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_NOTIFICATION_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_NOTIFICATION_SEND_BYTE(data)
#define ZW_DEBUG_NOTIFICATION_SEND_STR(STR)
#define ZW_DEBUG_NOTIFICATION_SEND_NUM(data)
#define ZW_DEBUG_NOTIFICATION_SEND_WORD_NUM(data)
#define ZW_DEBUG_NOTIFICATION_SEND_NL()
#endif

typedef struct _NOTIFICATION_
{
  AGI_PROFILE agiProfile;
  uint8_t ep;
  NOTIFICATION_TYPE type;
  uint8_t* pSuppportedEvents;
  uint8_t suppportedEventsLen;
  uint8_t event;
  uint8_t* pEvPar;
  uint8_t evParLen;
  uint8_t stateless : 1;
  uint8_t trigged   : 1;
  uint8_t noUsed    : 6;
} NOTIFICATION;

typedef struct _MY_NOTIFICATION_
{
  uint8_t lastActionGrp;
  NOTIFICATION grp[MAX_NOTIFICATIONS];
} MY_NOTIFICATION;



/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

MY_NOTIFICATION myNotification;

BOOL notificationBurglerUnknownEvent = FALSE;
uint8_t notificationBurglerSequenceNbr = 0;

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

static BOOL ValidateNotificationType(NOTIFICATION_TYPE notificationType, uint8_t endpoint);

void InitNotification(void)
{
  uint8_t i = 0;
  notificationBurglerUnknownEvent = FALSE;

  for(i = 0; i< MAX_NOTIFICATIONS; i++)
  {
    myNotification.grp[i].agiProfile.profile_MS = 0;
    myNotification.grp[i].agiProfile.profile_LS = 0;
    myNotification.grp[i].ep = 0xff;
    myNotification.grp[i].pSuppportedEvents = NULL;
    myNotification.grp[i].suppportedEventsLen = 0;
    myNotification.grp[i].type = NOTIFICATION_TYPE_NONE;
    myNotification.grp[i].event = 0;
    myNotification.grp[i].stateless = 0;
    myNotification.grp[i].pEvPar = NULL;
    myNotification.grp[i].evParLen = 0;
    myNotification.grp[i].trigged = 0;
  }
  myNotification.lastActionGrp = 0;

}

void DefaultNotifactionStatus(NOTIFICATION_STATUS status)
{
  uint8_t i;
  for(i = 0; i < MAX_NOTIFICATIONS; i++)
  {
    MemoryPutByte((WORD)&EEOFFSET_alarmStatus_far[i], (BYTE)status);
  }
}

BOOL AddNotification(
    AGI_PROFILE * pAgiProfile,
    NOTIFICATION_TYPE type,
    uint8_t * pSuppportedEvents,
    uint8_t suppportedEventsLen,
    BOOL stateless,
    uint8_t endpoint)
{
  uint8_t i;
  /*Find free slot*/
  for(i = 0; i< MAX_NOTIFICATIONS; i++)
  {
    if( 0 == myNotification.grp[i].type)
    {
      /*Event configuration*/
      myNotification.lastActionGrp = i;
      myNotification.grp[i].agiProfile.profile_MS = pAgiProfile->profile_MS;
      myNotification.grp[i].agiProfile.profile_LS = pAgiProfile->profile_LS;
      myNotification.grp[i].ep = endpoint;
      myNotification.grp[i].type = type;
      myNotification.grp[i].pSuppportedEvents = pSuppportedEvents;
      myNotification.grp[i].suppportedEventsLen = suppportedEventsLen;
      myNotification.grp[i].stateless = (stateless == FALSE) ? 0 : 1;

      /*Event state*/
      myNotification.grp[i].event = 0;
      myNotification.grp[i].pEvPar = NULL;
      myNotification.grp[i].evParLen = 0;
      myNotification.grp[i].trigged = 0;

      return TRUE;
    }
  }
  return FALSE;
}

uint8_t GetGroupNotificationType(uint8_t* pNotificationType, uint8_t endpoint)
{
  uint8_t i = 0;

  ZW_DEBUG_NOTIFICATION_SEND_STR("\r\nGetGroupNotificationType ");
  ZW_DEBUG_NOTIFICATION_SEND_NUM(*pNotificationType);

  if(0xFF == *pNotificationType)
  {
    /*Check last action is ready*/
    if(0xff == myNotification.lastActionGrp)
    {
      /* no last action, take first*/
      myNotification.lastActionGrp = 0;
    }

    if(endpoint == 0)
    {
      *pNotificationType =  myNotification.grp[myNotification.lastActionGrp].type;
    }
    else
    {
      if(myNotification.grp[myNotification.lastActionGrp].ep == endpoint)
      {
        *pNotificationType =  myNotification.grp[myNotification.lastActionGrp].type;
      }
      else{
        /*find notification out from end-point*/
        for(i = 0; i< MAX_NOTIFICATIONS; i++)
        {
          if(myNotification.grp[i].ep == endpoint)
          {
            *pNotificationType =  myNotification.grp[i].type;
          }
        }
      }
    }
  }

  for(i = 0; i< MAX_NOTIFICATIONS; i++)
  {
    ZW_DEBUG_NOTIFICATION_SEND_NUM(myNotification.grp[i].type);
    ZW_DEBUG_NOTIFICATION_SEND_NUM(*myNotification.grp[i].pSuppportedEvents);
    ZW_DEBUG_NOTIFICATION_SEND_BYTE(' ');
    if((myNotification.grp[i].type == *pNotificationType) && (myNotification.grp[i].ep == endpoint))
    {
      ZW_DEBUG_NOTIFICATION_SEND_STR("ID ");
      ZW_DEBUG_NOTIFICATION_SEND_NUM(i);
      return i;
    }
  }
  return 0xff;
}

BOOL FindNotificationEndpoint(
    NOTIFICATION_TYPE notificationType,
    uint8_t * pEndpoint)
{
  ZW_DEBUG_NOTIFICATION_SEND_STR("\r\nFindNotificationEndpoint ");
  ZW_DEBUG_NOTIFICATION_SEND_NUM(notificationType);
  ZW_DEBUG_NOTIFICATION_SEND_STR(" EP ");
  ZW_DEBUG_NOTIFICATION_SEND_NUM(*pEndpoint);
  ZW_DEBUG_NOTIFICATION_SEND_NL();

  if (FALSE == ValidateNotificationType(notificationType , *pEndpoint ) || (0 == *pEndpoint))
  {
    if(0 == *pEndpoint)
    {
      if(0xFF == notificationType )
      {
        if(0xFF != myNotification.lastActionGrp)
        {
          *pEndpoint = myNotification.grp[myNotification.lastActionGrp].ep;
        }
        else{
          *pEndpoint = myNotification.grp[0].ep;
        }
        return TRUE;
      }
      else
      {
        uint8_t i;

        if(0xFF != myNotification.lastActionGrp)
        {
          if(myNotification.grp[myNotification.lastActionGrp].type == notificationType)
          {
            *pEndpoint = myNotification.grp[myNotification.lastActionGrp].ep;
            return TRUE;
          }
        }

        for(i = 0; i< MAX_NOTIFICATIONS; i++)
        {
          if((myNotification.grp[i].type == notificationType) && (0xff != myNotification.grp[i].ep))
          {
            *pEndpoint = myNotification.grp[i].ep;
            return TRUE;
          }
        }
      }
    }
  }
  else{
    return TRUE;
  }
  return FALSE;
}

BOOL handleAppNotificationSet(
    NOTIFICATION_TYPE notificationType,
    NOTIFICATION_STATUS_SET notificationStatus,
    uint8_t endpoint)
{
  BYTE endpoint_temp =  endpoint;

  if (0xFF == notificationType)
  {
    // Return if notification type is 0xFF. Requirement CC:0071.03.06.11.002.
    return FALSE;
  }

  /*Validate endpoint! Change endpoint if it is not valid and root-endpoint*/
  if (FALSE == FindNotificationEndpoint(notificationType, &endpoint_temp))
  {
    /* Not valid endpoint!*/
    return FALSE;
  }

  if ((0x00 != notificationStatus) && (0xff != notificationStatus))
  {
    return FALSE;
  }

  ZW_DEBUG_NOTIFICATION_SEND_BYTE('i');
  ZW_DEBUG_NOTIFICATION_SEND_NUM((uint8_t)notificationStatus);
  MemoryPutByte((WORD)&(EEOFFSET_alarmStatus_far[GetGroupNotificationType((uint8_t*)&notificationType, endpoint_temp )]), (BYTE)notificationStatus);

  return TRUE;
}

void handleCmdClassNotificationEventSupportedReport(
    uint8_t notificationType,
    uint8_t * pNbrBitMask,
    uint8_t * pBitMaskArray,
    uint8_t endpoint)
{
  if( TRUE == FindNotificationEndpoint(notificationType, &endpoint) )
  {
    uint8_t temp_notificationType = notificationType;
    uint8_t grpNo = GetGroupNotificationType(&temp_notificationType, endpoint );
    uint8_t i;

    if(temp_notificationType != notificationType)
    {
      grpNo = 0xff;
    }

    *pNbrBitMask = 0;
    if(0xff == grpNo)
    {
      return;
    }
    for( i = 0; i < myNotification.grp[grpNo].suppportedEventsLen; i++)
    {
      if( *pNbrBitMask < myNotification.grp[grpNo].pSuppportedEvents[i] )
      {
        *pNbrBitMask = myNotification.grp[grpNo].pSuppportedEvents[i];
      }

      pBitMaskArray[myNotification.grp[grpNo].pSuppportedEvents[i]/8] |= (0x01<< (myNotification.grp[grpNo].pSuppportedEvents[i]%8));
    }
    /*calc number of bitmask bytes*/
    *pNbrBitMask = (*pNbrBitMask / 8) + 1;
  }
  else{
    /*Only support Unkown event why bit maks is 0*/
    *pNbrBitMask = 0;
  }
}

NOTIFICATION_STATUS CmdClassNotificationGetNotificationStatus(
    uint8_t notificationType,
    uint8_t endpoint)
{
  NOTIFICATION_STATUS status = NOTIFICATION_STATUS_UNSOLICIT_DEACTIVATED;
  uint8_t grp = GetGroupNotificationType( &notificationType, endpoint );

  if(0xff != grp)
  {
    uint8_t notification_status = MemoryGetByte((WORD)&EEOFFSET_alarmStatus_far[grp]);
    ZW_DEBUG_NOTIFICATION_SEND_BYTE('o');
    ZW_DEBUG_NOTIFICATION_SEND_NUM(notification_status);
    if(notification_status)
    {
      status = NOTIFICATION_STATUS_UNSOLICIT_ACTIVED;
    }
  }
  ZW_DEBUG_NOTIFICATION_SEND_STR("\r\nCCStatus");
  ZW_DEBUG_NOTIFICATION_SEND_NUM(status);
  ZW_DEBUG_NOTIFICATION_SEND_NL();

  return status;
}

BOOL CmdClassNotificationGetNotificationEvent(
    uint8_t * pNotificationType,
    uint8_t * pNotificationEvent,
    uint8_t * pEventPar,
    uint8_t * pEvNbrs,
    uint8_t endpoint)
{
  uint8_t i = 0;
  uint8_t grpNo = GetGroupNotificationType(pNotificationType, endpoint );
  *pEventPar = 0;
  *pEvNbrs = 0;
  if(0xff == grpNo)
  {
    return FALSE;
  }

  ZW_DEBUG_NOTIFICATION_SEND_STR("GetNotificationEvent");
  ZW_DEBUG_NOTIFICATION_SEND_NUM(*pNotificationType);
  ZW_DEBUG_NOTIFICATION_SEND_NUM(*pNotificationEvent);
  /*check valid type*/
  if(TRUE == ValidateNotificationType(*pNotificationType, endpoint ))
  {
    ZW_DEBUG_NOTIFICATION_SEND_NUM(myNotification.grp[grpNo].event);

    /*check valid event*/
    ZW_DEBUG_NOTIFICATION_SEND_STR(" valid event ");
    for(i = 0; i < myNotification.grp[grpNo].suppportedEventsLen; i++)
    {
      ZW_DEBUG_NOTIFICATION_SEND_NUM(myNotification.grp[grpNo].pSuppportedEvents[i]);


      if((*pNotificationEvent == myNotification.grp[grpNo].pSuppportedEvents[i]) && (0x00 != *pNotificationEvent))
      {
        /* Found correct supported event*/
        if(*pNotificationEvent == myNotification.grp[grpNo].event)
        {
          /* Event in queue*/
          *pEvNbrs = myNotification.grp[grpNo].evParLen;
          for(i = 0; i < myNotification.grp[grpNo].evParLen; i++)
          {
            pEventPar[i] = myNotification.grp[grpNo].pEvPar[i];
          }
          return TRUE;
        }
        else
        {
          /*No event in queue*/
          *pNotificationEvent = 0x00;
          *pNotificationEvent = 0;
          *pEventPar = 0;
          return TRUE;
        }
      }
    }

    if( *pNotificationEvent == 0x00)
    {
      /*Check for any event in queue*/
      ZW_DEBUG_NOTIFICATION_SEND_BYTE('!');
      ZW_DEBUG_NOTIFICATION_SEND_BYTE( myNotification.lastActionGrp);
      if( 0xff != myNotification.lastActionGrp)
      {
        uint8_t i = 0;
        *pNotificationEvent = myNotification.grp[grpNo].event;
        *pEvNbrs = myNotification.grp[grpNo].evParLen;
        for(i = 0; i < myNotification.grp[grpNo].evParLen; i++)
        {
          pEventPar[i] = myNotification.grp[grpNo].pEvPar[i];
        }
        //myNotification.lastActionGrp = 0xff;// empty last action

        return TRUE;
      }
      else
      {
        ZW_DEBUG_NOTIFICATION_SEND_BYTE('%');
        /*No event in queue*/
        *pNotificationEvent = 0x00;
        return TRUE;
      }
    }

    /* Event is not supported!*/
    ZW_DEBUG_NOTIFICATION_SEND_BYTE('-');
    *pNotificationEvent = 0xFE;
    *pEventPar = 0;
    //ZW_DEBUG_NOTIFICATION_SEND_NL();
    return TRUE;
  }
  return FALSE;
}

void NotificationEventTrigger(
    AGI_PROFILE * pAgiProfile,
    uint8_t type,
    uint8_t notificationEvent,
    uint8_t * pEvPar,
    uint8_t evParLen,
    uint8_t sourceEndpoint)
{
  uint8_t i;
  for(i = 0; i< MAX_NOTIFICATIONS; i++)
  {
    if( myNotification.grp[i].agiProfile.profile_MS == pAgiProfile->profile_MS &&
        myNotification.grp[i].agiProfile.profile_LS  == pAgiProfile->profile_LS &&
        myNotification.grp[i].type == type &&
        myNotification.grp[i].ep == sourceEndpoint
      )
    {
      ZW_DEBUG_NOTIFICATION_SEND_STR("\r\nNTrig");
      ZW_DEBUG_NOTIFICATION_SEND_NUM(myNotification.grp[i].type);
      ZW_DEBUG_NOTIFICATION_SEND_NUM( myNotification.grp[i].ep);
      ZW_DEBUG_NOTIFICATION_SEND_NL();
      myNotification.lastActionGrp = i;
      myNotification.grp[i].event = notificationEvent;
      myNotification.grp[i].pEvPar = pEvPar;
      myNotification.grp[i].evParLen = evParLen;
      myNotification.grp[i].trigged = 1;
      i = MAX_NOTIFICATIONS;
    }
  }
}

JOB_STATUS UnsolicitedNotificationAction(
  AGI_PROFILE* pProfile,
  uint8_t sourceEndpoint,
  VOID_CALLBACKFUNC(pCallback)(TRANSMISSION_RESULT * pTransmissionResult))
{
  if (myNotification.lastActionGrp >= MAX_NOTIFICATIONS)
  {
    return JOB_STATUS_BUSY;
  }

  if ((myNotification.grp[myNotification.lastActionGrp].agiProfile.profile_MS == pProfile->profile_MS) &&
      (myNotification.grp[myNotification.lastActionGrp].agiProfile.profile_LS == pProfile->profile_LS) &&
      (myNotification.grp[myNotification.lastActionGrp].trigged  == 1) &&
      (NOTIFICATION_STATUS_UNSOLICIT_ACTIVED ==
      CmdClassNotificationGetNotificationStatus(
          myNotification.grp[myNotification.lastActionGrp].type,
          myNotification.grp[myNotification.lastActionGrp].ep))
      )
  {
    ZW_DEBUG_NOTIFICATION_SEND_STR("\r\nUnAction");
    ZW_DEBUG_NOTIFICATION_SEND_NUM( myNotification.lastActionGrp);
    ZW_DEBUG_NOTIFICATION_SEND_NUM( myNotification.grp[myNotification.lastActionGrp].trigged);
    ZW_DEBUG_NOTIFICATION_SEND_NL();
    ZW_DEBUG_NOTIFICATION_SEND_NL();
    ZW_DEBUG_NOTIFICATION_SEND_BYTE('a');
    return CmdClassNotificationReport( pProfile,
                                   sourceEndpoint,
                                   myNotification.grp[myNotification.lastActionGrp].type,
                                   myNotification.grp[myNotification.lastActionGrp].event,
                                   pCallback);
  }
  myNotification.grp[myNotification.lastActionGrp].trigged = 0;
  ZW_DEBUG_NOTIFICATION_SEND_NL();
  ZW_DEBUG_NOTIFICATION_SEND_BYTE('b');
  return JOB_STATUS_BUSY;
}

void ClearLastNotificationAction(AGI_PROFILE* pAgiProfile, uint8_t sourceEndpoint)
{
  if (myNotification.lastActionGrp < MAX_NOTIFICATIONS)
  {
    if( myNotification.grp[myNotification.lastActionGrp].agiProfile.profile_MS == pAgiProfile->profile_MS &&
        myNotification.grp[myNotification.lastActionGrp].agiProfile.profile_LS  == pAgiProfile->profile_LS &&
        myNotification.grp[myNotification.lastActionGrp].ep == sourceEndpoint)
    {
      myNotification.grp[myNotification.lastActionGrp].event = 0;
      myNotification.grp[myNotification.lastActionGrp].pEvPar = NULL;
      myNotification.grp[myNotification.lastActionGrp].evParLen = 0;
      myNotification.grp[myNotification.lastActionGrp].trigged = 0;
    }
  }
  myNotification.lastActionGrp = 0xff;
}

void handleCmdClassNotificationSupportedReport(
    uint8_t * pNbrBitMask,
    uint8_t * pBitMaskArray,
    uint8_t endpoint)
{
  uint8_t i = 0;
  *pNbrBitMask = 0;

  if(0 == endpoint )   /* find all notification types for device*/
  {
    for(i = 0; i< MAX_NOTIFICATIONS; i++)
    {
      if( myNotification.grp[i].type != NOTIFICATION_TYPE_NONE)
      {
        /* Find max number of bit masks*/
        if(*pNbrBitMask < ((myNotification.grp[i].type / 8) + 1))
        {
          *pNbrBitMask = (myNotification.grp[i].type / 8) + 1;
        }
        /* Add Bit in bit-mask byte (myNotification.grp[i].type / 8)*/
        *(pBitMaskArray + (myNotification.grp[i].type / 8)) |= (1 << ((myNotification.grp[i].type) % 8));
      }
    }
  }
  else  /* find all notification types for endpoint*/
  {
    for(i = 0; i< MAX_NOTIFICATIONS; i++)
    {
      if( (myNotification.grp[i].type != NOTIFICATION_TYPE_NONE) &&
          (myNotification.grp[i].ep == endpoint))
      {
        /* Find max number of bit masks*/
        if(*pNbrBitMask < ((myNotification.grp[i].type / 8) + 1))
        {
          *pNbrBitMask = (myNotification.grp[i].type / 8) + 1;
        }
        /* Add Bit in bit-mask byte (myNotification.grp[i].type / 8)*/
        *(pBitMaskArray + (myNotification.grp[i].type / 8)) |= (1 << ((myNotification.grp[i].type) % 8));
      }
    }
  }
}

/**
 * @brief Validates whether a given notification type is set for a given endpoint.
 * @param notificationType Notification type.
 * @param endpoint Endpoint number
 * @return TRUE if notification type is set for endpoint, FALSE otherwise.
 */
static BOOL ValidateNotificationType(NOTIFICATION_TYPE notificationType, uint8_t endpoint)
{
  uint8_t i = 0;

  if( 0xFF == notificationType)
  {
    for(i = 0; i< MAX_NOTIFICATIONS; i++)
    {
      if(myNotification.grp[i].ep == endpoint)
      {
        return TRUE;
      }
    }
    return FALSE;
  }

  for(i = 0; i< MAX_NOTIFICATIONS; i++)
  {
    if(myNotification.grp[i].type == notificationType && myNotification.grp[i].ep == endpoint)
    {
      return TRUE;
    }
  }
  return FALSE;
}


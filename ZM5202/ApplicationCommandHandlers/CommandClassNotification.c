/**
 * @file
 * Handler for Command Class Notification.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_basis_api.h>
#include <ZW_TransportLayer.h>
#include <ZW_tx_mutex.h>

#include "config_app.h"
#include <CommandClassNotification.h>
#include <association_plus.h>
#include <misc.h>
#include <ZW_TransportMulticast.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

#ifdef ZW_DEBUG_CC_NOTIFICATION
#include <ZW_uart_api.h>
#define ZW_DEBUG_CCN_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_CCN_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_CCN_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_CCN_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_CCN_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_CCN_SEND_BYTE(data)
#define ZW_DEBUG_CCN_SEND_STR(STR)
#define ZW_DEBUG_CCN_SEND_NUM(data)
#define ZW_DEBUG_CCN_SEND_WORD_NUM(data)
#define ZW_DEBUG_CCN_SEND_NL()
#endif

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

received_frame_status_t handleCommandClassNotification(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  BYTE cmdLength)
{
  BYTE size;
  UNUSED(cmdLength);
  ZW_DEBUG_CCN_SEND_STR("CmdClassAlarm ");
  ZW_DEBUG_CCN_SEND_NUM(pCmd->ZW_Common.cmd);
  ZW_DEBUG_CCN_SEND_NL();
  switch (pCmd->ZW_Common.cmd)
  {
    case NOTIFICATION_SET_V4:
      if (TRUE != handleAppNotificationSet(
          pCmd->ZW_NotificationSetV4Frame.notificationType,
          pCmd->ZW_NotificationSetV4Frame.notificationStatus,
          rxOpt->destNode.endpoint))
      {
        return RECEIVED_FRAME_STATUS_FAIL;
      }
      break;

    case NOTIFICATION_GET_V4:
      if(FALSE == Check_not_legal_response_job(rxOpt))
      {
        ZW_APPLICATION_TX_BUFFER *pTxBuf = GetResponseBuffer();
        /*Check pTxBuf is free*/
        if( NON_NULL( pTxBuf ) )
        {
          BYTE tempEndpoint =  rxOpt->destNode.endpoint;
          TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx;
          RxToTxOptions(rxOpt, &pTxOptionsEx);

          pTxBuf->ZW_NotificationReport1byteV4Frame.notificationType = pCmd->ZW_NotificationGetV4Frame.notificationType;
          if(3 == cmdLength)
          {
            ZW_DEBUG_CCN_SEND_STR("ZW_ALARM_GET_V1_FRAME");
            pTxBuf->ZW_NotificationReport1byteV4Frame.notificationType = 0xFF;

          }

          ZW_DEBUG_CCN_SEND_STR("ZW_ALARM_GET_V4_FRAME");
          /*Validate endpoint! Change endpoint if it is not valid and root-endpoint*/
          if( FALSE == FindNotificationEndpoint(pTxBuf->ZW_NotificationReport1byteV4Frame.notificationType, &tempEndpoint))
          {
            /* Not valid endpoint!*/
            /*Job failed, free transmit-buffer pTxBuf by clearing mutex */
            FreeResponseBuffer();
            return RECEIVED_FRAME_STATUS_FAIL;
          }

          pTxBuf->ZW_NotificationReport1byteV4Frame.cmdClass = COMMAND_CLASS_NOTIFICATION_V4;
          pTxBuf->ZW_NotificationReport1byteV4Frame.cmd = NOTIFICATION_REPORT_V4;
          pTxBuf->ZW_NotificationReport1byteV4Frame.v1AlarmType = 0; /*must be set to 0*/
          pTxBuf->ZW_NotificationReport1byteV4Frame.v1AlarmLevel = 0; /*must be set to 0*/
          pTxBuf->ZW_NotificationReport1byteV4Frame.reserved = 0; /*must be set to 0*/
          pTxBuf->ZW_NotificationReport1byteV4Frame.notificationStatus =
            CmdClassNotificationGetNotificationStatus( pTxBuf->ZW_NotificationReport1byteV4Frame.notificationType, tempEndpoint );
          pTxBuf->ZW_NotificationReport1byteV4Frame.properties1 = 0;
          pTxBuf->ZW_NotificationReport1byteV4Frame.eventParameter1 = 0;
          pTxBuf->ZW_NotificationReport1byteV4Frame.mevent = 0;
          if(3 == cmdLength)
          {
            pTxBuf->ZW_NotificationReport1byteV4Frame.notificationType = 0;
            size = sizeof(ZW_NOTIFICATION_REPORT_1BYTE_V4_FRAME) - 2; //Remove event-parameter and sequence number
          }
          else if(5 > cmdLength) /*CC  V2*/
          {
            ZW_DEBUG_CCN_SEND_STR("CC V2");
            size = 0;
          }
          else{
            pTxBuf->ZW_NotificationReport1byteV4Frame.mevent = pCmd->ZW_NotificationGetV4Frame.mevent;
            size = 0;
          }


          if( 0xff == pTxBuf->ZW_NotificationReport1byteV4Frame.notificationType && 0x00 == pTxBuf->ZW_NotificationReport1byteV4Frame.mevent)
          {
            /* In response to a Notification Get (Notification Type = 0xFF) , a responding device MUST return
               a pending notification from its internal list (Pull mode). We also do it for Push mode.*/

            BYTE grp = GetGroupNotificationType(&pTxBuf->ZW_NotificationReport1byteV4Frame.notificationType,tempEndpoint);
            if(0xff == grp)
            {
              //ZW_DEBUG_CCN_SEND_STR(" STATUS 0xFE");
              //pTxBuf->ZW_NotificationReport1byteV4Frame.notificationStatus = 0xFE;
              //size = sizeof(ZW_NOTIFICATION_REPORT_1BYTE_V4_FRAME) - 5;
              /*We do not support alarmType!*/
              /*Job failed, free transmit-buffer pTxBuf by clearing mutex */
              FreeResponseBuffer();
              return RECEIVED_FRAME_STATUS_FAIL;
            }
          }

          if(0 == size)
          {
            if(TRUE == CmdClassNotificationGetNotificationEvent( &pTxBuf->ZW_NotificationReport1byteV4Frame.notificationType,
                                                            &pTxBuf->ZW_NotificationReport1byteV4Frame.mevent,
                                                            &(pTxBuf->ZW_NotificationReport1byteV4Frame.eventParameter1),
                                                            &(pTxBuf->ZW_NotificationReport1byteV4Frame.properties1),
                                                            tempEndpoint))
            {
              size = (sizeof(ZW_NOTIFICATION_REPORT_1BYTE_V4_FRAME) - sizeof(BYTE) +
                  (pTxBuf->ZW_NotificationReport1byteV4Frame.properties1 & ALARM_TYPE_SUPPORTED_REPORT_PROPERTIES1_NUMBER_OF_BIT_MASKS_MASK_V2)) -
                  sizeof(BYTE); /* Removed sequence number*/
            }
            else{
              /*We do not support alarmType!*/
              /*Job failed, free transmit-buffer pTxBuf by clearing mutex */
              FreeResponseBuffer();
              return RECEIVED_FRAME_STATUS_FAIL;
            }
          }

          if (size)
          {
            if(ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
                (BYTE *)pTxBuf,
                size,
                pTxOptionsEx,
                ZCB_ResponseJobStatus))
            {
              /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
              FreeResponseBuffer();
            }
            return RECEIVED_FRAME_STATUS_SUCCESS;
          }
          else
          {
            /*Size 0, free transmit-buffer pTxBuf by cleaing mutex */
            FreeResponseBuffer();
          }
        }
      }
      return RECEIVED_FRAME_STATUS_FAIL;
      break;

    case NOTIFICATION_SUPPORTED_GET_V4:
      if(FALSE == Check_not_legal_response_job(rxOpt))
      {
        ZW_APPLICATION_TX_BUFFER *pTxBuf = GetResponseBuffer();
        /*Check pTxBuf is free*/
        if( NON_NULL( pTxBuf ) )
        {
          TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx;
          RxToTxOptions(rxOpt, &pTxOptionsEx);
          pTxBuf->ZW_NotificationSupportedReport1byteV4Frame.cmdClass = COMMAND_CLASS_NOTIFICATION_V4;
          pTxBuf->ZW_NotificationSupportedReport1byteV4Frame.cmd = NOTIFICATION_SUPPORTED_REPORT_V4;
          handleCmdClassNotificationSupportedReport(&(pTxBuf->ZW_NotificationSupportedReport1byteV4Frame.properties1),
                                               &(pTxBuf->ZW_NotificationSupportedReport1byteV4Frame.bitMask1),
                                               rxOpt->destNode.endpoint);

          /*
           * Requirement CC:0071.03.08.11.001:
           * MSb of properties1 tells whether a node supports Alarm CC V1. This implementation of
           * Notification CC does not support Alarm CC V1, hence this field is set to zero.
           */
          pTxBuf->ZW_NotificationSupportedReport1byteV4Frame.properties1 &= 0x7F;

          if(ZW_TX_IN_PROGRESS != Transport_SendResponseEP((BYTE *)pTxBuf,
                                           sizeof(ZW_NOTIFICATION_SUPPORTED_REPORT_1BYTE_V4_FRAME) - 1 +
                                            pTxBuf->ZW_NotificationSupportedReport1byteV4Frame.properties1,
                                           pTxOptionsEx, ZCB_ResponseJobStatus))
          {
            /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
            FreeResponseBuffer();
          }
          return RECEIVED_FRAME_STATUS_SUCCESS;
        }
      }
      return RECEIVED_FRAME_STATUS_FAIL;
      break;

    case EVENT_SUPPORTED_GET_V4:
      if(FALSE == Check_not_legal_response_job(rxOpt))
      {
        ZW_APPLICATION_TX_BUFFER *pTxBuf = GetResponseBuffer();
        /*Check pTxBuf is free*/
        if( NON_NULL( pTxBuf ) )
        {
          TRANSMIT_OPTIONS_TYPE_SINGLE_EX* txOptionsEx;
          RxToTxOptions(rxOpt, &txOptionsEx);
          pTxBuf->ZW_EventSupportedReport1byteV4Frame.cmdClass = COMMAND_CLASS_NOTIFICATION_V4;
          pTxBuf->ZW_EventSupportedReport1byteV4Frame.cmd = EVENT_SUPPORTED_REPORT_V4;
          pTxBuf->ZW_EventSupportedReport1byteV4Frame.notificationType = pCmd->ZW_EventSupportedGetV4Frame.notificationType;
          pTxBuf->ZW_EventSupportedReport1byteV4Frame.properties1 = 0;
          handleCmdClassNotificationEventSupportedReport(
                              pTxBuf->ZW_EventSupportedReport1byteV4Frame.notificationType,
                              &(pTxBuf->ZW_EventSupportedReport1byteV4Frame.properties1),
                              &(pTxBuf->ZW_EventSupportedReport1byteV4Frame.bitMask1),
                              rxOpt->destNode.endpoint);

          pTxBuf->ZW_EventSupportedReport1byteV4Frame.properties1 &= 0x7F;

          if(ZW_TX_IN_PROGRESS != Transport_SendResponseEP( (BYTE *)pTxBuf,
                                           sizeof(ZW_EVENT_SUPPORTED_REPORT_1BYTE_V4_FRAME) - 1 +
                                           (pTxBuf->ZW_EventSupportedReport1byteV4Frame.properties1 & 0x1F), /*remove reserved bits*/
                                           txOptionsEx, ZCB_ResponseJobStatus))
          {
            /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
            FreeResponseBuffer();
          }
          return RECEIVED_FRAME_STATUS_SUCCESS;
        }
      }
      return RECEIVED_FRAME_STATUS_FAIL;
      break;

    default:
      return RECEIVED_FRAME_STATUS_NO_SUPPORT;
      break;
  }
  return RECEIVED_FRAME_STATUS_SUCCESS;
}

JOB_STATUS CmdClassNotificationReport(
  AGI_PROFILE* pProfile,
  BYTE sourceEndpoint,
  BYTE notificationType,
  BYTE notificationEvent,
  VOID_CALLBACKFUNC(pCallback)(TRANSMISSION_RESULT * pTransmissionResult))
{
  ZW_APPLICATION_TX_BUFFER *pTxBuf = GetRequestBuffer(pCallback);
 if( IS_NULL( pTxBuf ) )
  {
    /*Ongoing job is active.. just stop current job*/
    return JOB_STATUS_BUSY;
  }
  else
  {
    TRANSMIT_OPTIONS_TYPE_EX* pTxOptionsEx = NULL;

    pTxBuf->ZW_NotificationReport1byteV4Frame.cmdClass = COMMAND_CLASS_NOTIFICATION_V4;
    pTxBuf->ZW_NotificationReport1byteV4Frame.cmd = NOTIFICATION_REPORT_V4;
    pTxBuf->ZW_NotificationReport1byteV4Frame.v1AlarmType = 0;
    pTxBuf->ZW_NotificationReport1byteV4Frame.v1AlarmLevel = 0;
    pTxBuf->ZW_NotificationReport1byteV4Frame.reserved = 0;
    pTxBuf->ZW_NotificationReport1byteV4Frame.notificationStatus = NOTIFICATION_STATUS_UNSOLICIT_ACTIVED;
    pTxBuf->ZW_NotificationReport1byteV4Frame.notificationType = notificationType;
    pTxBuf->ZW_NotificationReport1byteV4Frame.mevent = notificationEvent;

    CmdClassNotificationGetNotificationEvent( &(pTxBuf->ZW_NotificationReport1byteV4Frame.notificationType),
                                              &(pTxBuf->ZW_NotificationReport1byteV4Frame.mevent),
                                              &(pTxBuf->ZW_NotificationReport1byteV4Frame.eventParameter1),
                                              &(pTxBuf->ZW_NotificationReport1byteV4Frame.properties1),
                                              sourceEndpoint);
    pTxBuf->ZW_NotificationReport1byteV4Frame.properties1 &= ALARM_TYPE_SUPPORTED_REPORT_PROPERTIES1_NUMBER_OF_BIT_MASKS_MASK_V2; /*remove sequence number and reserved*/


    /*Get transmit options (node list)*/
    pTxOptionsEx = ReqNodeList( pProfile,
                          (CMD_CLASS_GRP*) &(pTxBuf->ZW_Common.cmdClass),
                          sourceEndpoint);
    if( IS_NULL( pTxOptionsEx ) )
    {
      /*Job failed, free transmit-buffer pTxBuf by cleaning mutex */
      FreeRequestBuffer();
      return JOB_STATUS_NO_DESTINATIONS;
    }

    if (ZW_TX_IN_PROGRESS != ZW_TransportMulticast_SendRequest(
      (BYTE *)pTxBuf,
      (sizeof(ZW_NOTIFICATION_REPORT_1BYTE_V4_FRAME) - sizeof(BYTE) +
        pTxBuf->ZW_NotificationReport1byteV4Frame.properties1) - sizeof(BYTE),
      TRUE, // Use Supervision encapsulation
      pTxOptionsEx,
      ZCB_RequestJobStatus))
    {
      /*Job failed, free transmit-buffer pTxBuf by cleaning mutex */
      FreeRequestBuffer();
     return JOB_STATUS_BUSY;
    }
  }
  return JOB_STATUS_SUCCESS;
}

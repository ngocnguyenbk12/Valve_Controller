/**
 * @file
 * Handler for Command Class Central Scene.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_basis_api.h>
#include <ZW_tx_mutex.h>
#include <ZW_TransportLayer.h>
#include <ZW_uart_api.h>
#include "config_app.h"
#include <CommandClassCentralScene.h>
#include <misc.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
#ifdef ZW_DEBUG_CENTRAL_SCENE
#define ZW_DEBUG_CENTRAL_SCENE_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_CENTRAL_SCENE_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_CENTRAL_SCENE_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_CENTRAL_SCENE_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_CENTRAL_SCENE_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_CENTRAL_SCENE_SEND_BYTE(data)
#define ZW_DEBUG_CENTRAL_SCENE_SEND_STR(STR)
#define ZW_DEBUG_CENTRAL_SCENE_SEND_NUM(data)
#define ZW_DEBUG_CENTRAL_SCENE_SEND_WORD_NUM(data)
#define ZW_DEBUG_CENTRAL_SCENE_SEND_NL()
#endif

received_frame_status_t
handleCommandClassCentralScene(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  BYTE cmdLength)
{
  ZW_APPLICATION_TX_BUFFER * pTxBuf;
  TRANSMIT_OPTIONS_TYPE_SINGLE_EX * pTxOptionsEx;
  uint8_t numberOfBytesWithKeyAttributes;
  UNUSED(cmdLength);

  switch (pCmd->ZW_Common.cmd)
  {
    case CENTRAL_SCENE_SUPPORTED_GET_V2:
      if (FALSE == Check_not_legal_response_job(rxOpt))
      {

        pTxBuf = GetResponseBuffer();
        /*Check pTxBuf is free*/
        if (NON_NULL(pTxBuf))
        {
          RxToTxOptions(rxOpt, &pTxOptionsEx);

          numberOfBytesWithKeyAttributes = getAppCentralSceneReportData(
            &(pTxBuf->ZW_CentralSceneSupportedReport1byteV3Frame));

          pTxBuf->ZW_CentralSceneSupportedReport1byteV3Frame.cmdClass = COMMAND_CLASS_CENTRAL_SCENE_V3;
          pTxBuf->ZW_CentralSceneSupportedReport1byteV3Frame.cmd = CENTRAL_SCENE_SUPPORTED_REPORT_V3;
          pTxBuf->ZW_CentralSceneSupportedReport1byteV3Frame.properties1 &= ~0xF8;

          // Forced to always support slow refresh.
          pTxBuf->ZW_CentralSceneSupportedReport1byteV3Frame.properties1 |= 0x80;

          if (ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
            (BYTE *)pTxBuf,
            sizeof(ZW_CENTRAL_SCENE_SUPPORTED_REPORT_1BYTE_V3_FRAME) - 4 + numberOfBytesWithKeyAttributes,
           pTxOptionsEx,
            ZCB_ResponseJobStatus))
          {
            /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
            FreeResponseBuffer();
          }
          return RECEIVED_FRAME_STATUS_SUCCESS;
        }
      }
      return RECEIVED_FRAME_STATUS_FAIL;
      break;

    case CENTRAL_SCENE_CONFIGURATION_GET_V3:
      if (FALSE == Check_not_legal_response_job(rxOpt))
      {

        pTxBuf = GetResponseBuffer();
        /*Check pTxBuf is free*/
        if (NON_NULL(pTxBuf))
        {
          RxToTxOptions(rxOpt, &pTxOptionsEx);

          pTxBuf->ZW_CentralSceneConfigurationReportV3Frame.cmdClass = COMMAND_CLASS_CENTRAL_SCENE_V3;
          pTxBuf->ZW_CentralSceneConfigurationReportV3Frame.cmd = CENTRAL_SCENE_CONFIGURATION_REPORT_V3;
          {
            central_scene_configuration_t configuration;
            getAppCentralSceneConfiguration(&configuration);
            pTxBuf->ZW_CentralSceneConfigurationReportV3Frame.properties1 = (configuration.slowRefresh << 7);
          }

          if (ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
            (BYTE *)pTxBuf,
            sizeof(ZW_CENTRAL_SCENE_CONFIGURATION_REPORT_V3_FRAME),
            pTxOptionsEx,
            ZCB_ResponseJobStatus))
          {
            /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
            FreeResponseBuffer();
          }
          return RECEIVED_FRAME_STATUS_SUCCESS;
        }
      }
      return RECEIVED_FRAME_STATUS_FAIL;
      break;

    case CENTRAL_SCENE_CONFIGURATION_SET_V3:
      {
        central_scene_configuration_t configuration;
        configuration.slowRefresh = (pCmd->ZW_CentralSceneConfigurationSetV3Frame.properties1 >> 7);
        setAppCentralSceneConfiguration(&configuration);
      }
      return RECEIVED_FRAME_STATUS_SUCCESS;
      break;
  }
  return RECEIVED_FRAME_STATUS_NO_SUPPORT;
}

JOB_STATUS
CommandClassCentralSceneNotificationTransmit(
                                             AGI_PROFILE* pProfile,
                                             BYTE sourceEndpoint,
                                             BYTE keyAttribute,
                                             BYTE sceneNumber,
                                             VOID_CALLBACKFUNC(pCbFunc) (TRANSMISSION_RESULT * pTransmissionResult))
{
  static BYTE sequenceNumber = 0;
  TRANSMIT_OPTIONS_TYPE_EX* pTxOptionsEx = NULL;
  central_scene_configuration_t configuration;
  ZW_APPLICATION_TX_BUFFER *pTxBuf = GetRequestBuffer(pCbFunc);

  if( IS_NULL( pTxBuf ) )
  {
    /*Ongoing job is active.. just stop current job*/
    return JOB_STATUS_BUSY;
  }

  pTxBuf->ZW_CentralSceneNotificationV3Frame.cmdClass = COMMAND_CLASS_CENTRAL_SCENE_V3;
  pTxBuf->ZW_CentralSceneNotificationV3Frame.cmd = CENTRAL_SCENE_NOTIFICATION_V3;
  pTxBuf->ZW_CentralSceneNotificationV3Frame.sequenceNumber = sequenceNumber++;

  getAppCentralSceneConfiguration(&configuration);
  pTxBuf->ZW_CentralSceneNotificationV3Frame.properties1 = (configuration.slowRefresh << 7);
  pTxBuf->ZW_CentralSceneNotificationV3Frame.properties1 |= (keyAttribute & 0x87);
  pTxBuf->ZW_CentralSceneNotificationV3Frame.sceneNumber = sceneNumber;

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

  if(ZW_TX_IN_PROGRESS != ZW_TransportMulticast_SendRequest(
    (BYTE *)pTxBuf,
    sizeof(ZW_CENTRAL_SCENE_NOTIFICATION_V2_FRAME),
    TRUE, // Enable Supervision
    pTxOptionsEx,
    ZCB_RequestJobStatus))
  {
    /*Job failed, free transmit-buffer pTxBuf by cleaning mutex */
    FreeRequestBuffer();
    return JOB_STATUS_BUSY;
  }

  return JOB_STATUS_SUCCESS;
}

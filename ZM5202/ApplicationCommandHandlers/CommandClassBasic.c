/**
 * @file
 * Handler for Command Class Basic.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_basis_api.h>
#include <ZW_tx_mutex.h>
#include <ZW_TransportLayer.h>
#include <ZW_TransportEndpoint.h>
#include <ZW_TransportMulticast.h>
#include <ZW_transport_api.h>
#include "config_app.h"
#include <CommandClassBasic.h>
#include "misc.h"
#include <CommandClass.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

received_frame_status_t handleCommandClassBasic(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  BYTE cmdLength)
{
  UNUSED(cmdLength);
  switch (pCmd->ZW_Common.cmd)
  {
      //Must be ignored to avoid unintentional operation. Cannot be mapped to another command class.
    case BASIC_SET:
      if ((0x63 < pCmd->ZW_BasicSetV2Frame.value) && (0xFF != pCmd->ZW_BasicSetV2Frame.value))
      {
        return RECEIVED_FRAME_STATUS_FAIL;
      }
      handleBasicSetCommand(pCmd->ZW_BasicSetFrame.value, rxOpt->destNode.endpoint);
      return RECEIVED_FRAME_STATUS_SUCCESS;
      break;

    case BASIC_GET:
      if(FALSE == Check_not_legal_response_job(rxOpt))
      {
        ZW_APPLICATION_TX_BUFFER *pTxBuf = GetResponseBuffer();

        /*Check pTxBuf is free*/
        if( NON_NULL( pTxBuf ) )
        {
          TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx;
          RxToTxOptions(rxOpt, &pTxOptionsEx);
          /* Controller wants the sensor level */
          pTxBuf->ZW_BasicReportFrame.cmdClass = COMMAND_CLASS_BASIC;
          pTxBuf->ZW_BasicReportFrame.cmd = BASIC_REPORT;

          pTxBuf->ZW_BasicReportV2Frame.currentValue =  getAppBasicReport(pTxOptionsEx->sourceEndpoint);
          pTxBuf->ZW_BasicReportV2Frame.targetValue =  getAppBasicReportTarget(pTxOptionsEx->sourceEndpoint);
          pTxBuf->ZW_BasicReportV2Frame.duration =  getAppBasicReportDuration(pTxOptionsEx->sourceEndpoint);
          if(ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
              (BYTE *)pTxBuf,
              sizeof(ZW_BASIC_REPORT_V2_FRAME),
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
    default:
      return RECEIVED_FRAME_STATUS_NO_SUPPORT;
  }
  return RECEIVED_FRAME_STATUS_SUCCESS;
}

JOB_STATUS
CmdClassBasicReportSend(
  AGI_PROFILE* pProfile,
  BYTE sourceEndpoint,
  BYTE bValue,
  VOID_CALLBACKFUNC(pCbFunc)(TRANSMISSION_RESULT * pTransmissionResult))
{
  CMD_CLASS_GRP cmdGrp = {COMMAND_CLASS_BASIC, BASIC_REPORT};

  return cc_engine_multicast_request(
      pProfile,
      sourceEndpoint,
      &cmdGrp,
      &bValue,
      1,
      TRUE,
      pCbFunc);
}


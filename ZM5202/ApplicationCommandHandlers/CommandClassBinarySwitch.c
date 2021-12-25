/***************************************************************************
*
* Copyright (c) 2001-2011
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: Binary switch Command Class cource file
*
* Author:
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
#include <ZW_TransportMulticast.h>
#include "config_app.h"
#include <CommandClassBinarySwitch.h>
#include <misc.h>

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

received_frame_status_t
handleCommandClassBinarySwitch(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  BYTE cmdLength)
{
  UNUSED(cmdLength);

  switch (pCmd->ZW_Common.cmd)
  {
    case SWITCH_BINARY_GET:
      if(FALSE == Check_not_legal_response_job(rxOpt))
      {
        ZW_APPLICATION_TX_BUFFER *pTxBuf = GetResponseBuffer();
        /*Check pTxBuf is free*/
        if( NON_NULL( pTxBuf ) )
        {
          TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx;
          RxToTxOptions(rxOpt, &pTxOptionsEx);
          pTxBuf->ZW_SwitchBinaryReportFrame.cmdClass = COMMAND_CLASS_SWITCH_BINARY;
          pTxBuf->ZW_SwitchBinaryReportFrame.cmd = SWITCH_BINARY_REPORT;
          pTxBuf->ZW_SwitchBinaryReportFrame.value = handleAppltBinarySwitchGet(rxOpt->destNode.endpoint);

          if(ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
              (BYTE *)pTxBuf,
              sizeof(ZW_SWITCH_BINARY_REPORT_FRAME),
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

    case SWITCH_BINARY_SET:
      if ((0x63 < pCmd->ZW_SwitchBinarySetFrame.switchValue) &&
          (0xFF != pCmd->ZW_SwitchBinarySetFrame.switchValue))
      {
        return RECEIVED_FRAME_STATUS_FAIL;
      }
      CommandClassBinarySwitchSupportSet(pCmd->ZW_SwitchBinarySetFrame.switchValue, rxOpt->destNode.endpoint );
      return RECEIVED_FRAME_STATUS_SUCCESS;
      break;
  }
  return RECEIVED_FRAME_STATUS_NO_SUPPORT;
}

void CommandClassBinarySwitchSupportSet(
    BYTE val,
    BYTE endpoint)
{
  if (val == 0)
  {
    handleApplBinarySwitchSet(CMD_CLASS_BIN_OFF,  endpoint );
  }

  else if ((val < 0x64) ||
           (val == 0xff))
  {
    handleApplBinarySwitchSet(CMD_CLASS_BIN_ON, endpoint );
  }
}

JOB_STATUS CmdClassBinarySwitchReportSendUnsolicited(
  AGI_PROFILE* pProfile,
  BYTE sourceEndpoint,
  CMD_CLASS_BIN_SW_VAL bValue,
  VOID_CALLBACKFUNC(pCbFunc)(TRANSMISSION_RESULT * pTransmissionResult))
{
  CMD_CLASS_GRP cmdGrp = {COMMAND_CLASS_SWITCH_BINARY, SWITCH_BINARY_REPORT};

  return cc_engine_multicast_request(
      pProfile,
      sourceEndpoint,
      &cmdGrp,
      (uint8_t *)&bValue,
      1,
      TRUE,
      pCbFunc);
}

#include "config_app.h"
#include <ZW_basis_api.h>
#include <ZW_TransportLayer.h>
#include <CommandClassSimpleAv.h>
#include <ZW_tx_mutex.h>

static BYTE seqNo;

received_frame_status_t
handleCommandClassSimpleAv(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  BYTE cmdLength)
{
  ZW_APPLICATION_TX_BUFFER * pTxBuf;

  UNUSED(cmdLength);

  if(TRUE == Check_not_legal_response_job(rxOpt))
  {
    /*Get/Report do not support endpoint bit-addressing */
    return RECEIVED_FRAME_STATUS_FAIL;
  }

  switch (pCmd->ZW_Common.cmd)
  {
      //Must be ignored to avoid unintentional operation. Cannot be mapped to another command class.
    case SIMPLE_AV_CONTROL_GET:
      pTxBuf = GetResponseBuffer();
        /*Check pTxBuf is free*/
      if( NON_NULL( pTxBuf ) )
      {
        /* Controller wants the sensor level */
          TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx;
          RxToTxOptions(rxOpt, &pTxOptionsEx);
        pTxBuf->ZW_SimpleAvControlReportFrame.cmdClass = COMMAND_CLASS_SIMPLE_AV_CONTROL;
        pTxBuf->ZW_SimpleAvControlReportFrame.cmd = SIMPLE_AV_CONTROL_REPORT;
        pTxBuf->ZW_SimpleAvControlReportFrame.numberOfReports =  getApplSimpleAvReports();
        if(ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
            (BYTE *)pTxBuf,
            sizeof(ZW_SIMPLE_AV_CONTROL_GET_FRAME),
            pTxOptionsEx,
            ZCB_ResponseJobStatus))
        {
            /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
          FreeResponseBuffer();
        }
        return RECEIVED_FRAME_STATUS_SUCCESS;
      }
      return RECEIVED_FRAME_STATUS_FAIL;
      break;

    case SIMPLE_AV_CONTROL_SUPPORTED_GET:
      pTxBuf = GetResponseBuffer();
      /*Check pTxBuf is free*/
      if( NON_NULL( pTxBuf ) )
      {
        /* Controller wants the sensor level */
        BYTE len;
        TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx;
        RxToTxOptions(rxOpt, &pTxOptionsEx);
        len = getApplSimpleAvSupported(pCmd->ZW_SimpleAvControlSupportedReport4byteFrame.reportNo,
                                      &pTxBuf->ZW_SimpleAvControlSupportedReport1byteFrame.bitMask1);
        pTxBuf->ZW_SimpleAvControlSupportedReport1byteFrame.cmdClass = COMMAND_CLASS_SIMPLE_AV_CONTROL;
        pTxBuf->ZW_SimpleAvControlSupportedReport1byteFrame.cmd = SIMPLE_AV_CONTROL_SUPPORTED_REPORT;
        pTxBuf->ZW_SimpleAvControlSupportedReport1byteFrame.reportNo = pCmd->ZW_SimpleAvControlSupportedReport4byteFrame.reportNo;
        if(ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
            (BYTE *)pTxBuf,
            sizeof(ZW_SIMPLE_AV_CONTROL_SUPPORTED_REPORT_1BYTE_FRAME) - 1 +len,
            pTxOptionsEx,
            ZCB_ResponseJobStatus))
        {
          /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
          FreeResponseBuffer();
        }
        return RECEIVED_FRAME_STATUS_SUCCESS;
      }
      return RECEIVED_FRAME_STATUS_FAIL;
      break;

    default:
      break;
  }
  return RECEIVED_FRAME_STATUS_NO_SUPPORT;
}

JOB_STATUS
CmdClassSimpleAvSet(
  TRANSMIT_OPTIONS_TYPE_SINGLE_EX* pTxOptionsEx,
  WORD bCommand,
  BYTE bKeyAttrib,
  VOID_CALLBACKFUNC(pCbFunc)(TRANSMISSION_RESULT * pTransmissionResult))
{

  ZW_APPLICATION_TX_BUFFER *pTxBuf = GetRequestBuffer(pCbFunc);
 if( IS_NULL( pTxBuf ) )
  {
    /*Ongoing job is active.. just stop current job*/
    return JOB_STATUS_BUSY;
  }
  pTxBuf->ZW_SimpleAvControlSet1byteFrame.cmdClass = COMMAND_CLASS_SIMPLE_AV_CONTROL;
  pTxBuf->ZW_SimpleAvControlSet1byteFrame.cmd = SIMPLE_AV_CONTROL_SET;
  pTxBuf->ZW_SimpleAvControlSet1byteFrame.sequenceNumber = seqNo++;
  pTxBuf->ZW_SimpleAvControlSet1byteFrame.properties1 = 0x07 & bKeyAttrib;
  pTxBuf->ZW_SimpleAvControlSet1byteFrame.itemId1 = 0;
  pTxBuf->ZW_SimpleAvControlSet1byteFrame.itemId2 = 0;
  pTxBuf->ZW_SimpleAvControlSet1byteFrame.variantgroup1.command1 = (BYTE)((bCommand&0xff00)>>8);   //Command MSB
  pTxBuf->ZW_SimpleAvControlSet1byteFrame.variantgroup1.command2 = (BYTE)(bCommand&0xff);        //Command LSB


  if(ZW_TX_IN_PROGRESS != Transport_SendRequestEP(
        (BYTE*)pTxBuf,
        sizeof(ZW_SIMPLE_AV_CONTROL_SET_1BYTE_FRAME),
        pTxOptionsEx,
        ZCB_RequestJobStatus))
  {
    FreeRequestBuffer();
    return JOB_STATUS_BUSY;
  }
  return JOB_STATUS_SUCCESS;
}

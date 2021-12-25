/***************************************************************************
*
* Copyright (c) 2001-2011
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: Command class security supported Get functionality
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
#include <CommandClassSecurity.h>
#include <misc.h>
#include <ZW_uart_api.h>

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



/*==============================   handleCommandClassBinarySwitch  ============
**
**  Function:  handler for Binary Switch Info CC
**
**  Side effects: None
**
**--------------------------------------------------------------------------*/
received_frame_status_t
handleCommandClassSecurity(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt, /* IN receive options of type RECEIVE_OPTIONS_TYPE_EX  */
  ZW_APPLICATION_TX_BUFFER *pCmd, /* IN  Payload from the received frame */
  BYTE cmdLength)               /* IN Number of command bytes including the command */
{
  BYTE length = 0;
  ZW_APPLICATION_TX_BUFFER *pTxBuf = NULL;
  TRANSMIT_OPTIONS_TYPE_SINGLE_EX *txOptionsEx = NULL;
  received_frame_status_t frame_status = RECEIVED_FRAME_STATUS_FAIL;
  UNUSED(cmdLength);

  if(TRUE == Check_not_legal_response_job(rxOpt))
  {
    /*Get/Report do not support endpoint bit-addressing */
    return RECEIVED_FRAME_STATUS_FAIL;
  }

  if ((SECURITY_COMMANDS_SUPPORTED_GET != pCmd->ZW_Common.cmd) &&
      (SECURITY_2_COMMANDS_SUPPORTED_GET != pCmd->ZW_Common.cmd))
  {
    return RECEIVED_FRAME_STATUS_NO_SUPPORT;
  }

  pTxBuf = GetResponseBuffer();
  /*Check pTxBuf is free*/
  if( NON_NULL( pTxBuf ) )
  {
    BYTE* pPayload = NULL;
    security_key_t secureLevel;
    CMD_CLASS_LIST *pCmdClassList  = NULL;
    RxToTxOptions(rxOpt, &txOptionsEx);



    if ((SECURITY_COMMANDS_SUPPORTED_GET == pCmd->ZW_Common.cmd) && (COMMAND_CLASS_SECURITY == pCmd->ZW_Common.cmdClass))
    {
      frame_status = RECEIVED_FRAME_STATUS_SUCCESS;
      pTxBuf->ZW_SecurityCommandsSupportedReport1byteFrame.cmdClass = COMMAND_CLASS_SECURITY;
      pTxBuf->ZW_SecurityCommandsSupportedReport1byteFrame.cmd = SECURITY_COMMANDS_SUPPORTED_REPORT;
      pTxBuf->ZW_SecurityCommandsSupportedReport1byteFrame.reportsToFollow = 0;
      length = sizeof(pTxBuf->ZW_SecurityCommandsSupportedReport1byteFrame) - 3;
      secureLevel = SECURITY_KEY_S0;
      pPayload = &pTxBuf->ZW_SecurityCommandsSupportedReport1byteFrame.commandClassSupport1;
    }
    else if ((SECURITY_2_COMMANDS_SUPPORTED_GET == pCmd->ZW_Common.cmd) && (COMMAND_CLASS_SECURITY_2 == pCmd->ZW_Common.cmdClass))
    {
      frame_status = RECEIVED_FRAME_STATUS_SUCCESS;
      pTxBuf->ZW_SecurityCommandsSupportedReport1byteFrame.cmdClass = COMMAND_CLASS_SECURITY_2;
      pTxBuf->ZW_SecurityCommandsSupportedReport1byteFrame.cmd = SECURITY_2_COMMANDS_SUPPORTED_REPORT;
      length = 2; /*SECURITY_2_COMMANDS_SUPPORTED_REPORT = 2*/
      secureLevel = rxOpt->securityKey;//SECURITY_KEY_S2_UNAUTHENTICATED;
      pPayload = &pTxBuf->ZW_SecurityCommandsSupportedReport1byteFrame.reportsToFollow;
    }
    else
    {
      /*Job failed, free transmit-buffer pTxBuf by cleaning mutex */
      FreeResponseBuffer();
      return RECEIVED_FRAME_STATUS_NO_SUPPORT;
    }

    pCmdClassList = GetCommandClassList((0 != GetNodeId()), secureLevel, rxOpt->destNode.endpoint);
    if(NULL != pCmdClassList)
    {
      length += pCmdClassList->size;
      memcpy(pPayload, pCmdClassList->pList, pCmdClassList->size);
    }

    if(ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
        (BYTE *)pTxBuf,
        length,
        txOptionsEx,
        ZCB_ResponseJobStatus))
    {
      /*Job failed, free transmit-buffer pTxBuf by cleaning mutex */
      FreeResponseBuffer();
    }
  }
  return frame_status;
}

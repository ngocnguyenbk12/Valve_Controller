/**
 * @file
 * Handler for Command Class Multi Channel.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include <ZW_classcmd.h>
#include <ZW_cmd_class_list.h>
#include <ZW_tx_mutex.h>
#include <ZW_mem_api.h>
#include <ZW_TransportSecProtocol.h>
#include <CommandClassMultiChan.h>


/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

// Add debug defines here if necessary. Remember UART api.

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

// Nothing here.

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

// Nothing here.

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

static void CmdClassMultiChannelEncapsulateCmd(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_MULTI_CHANNEL_CMD_ENCAP_V2_FRAME *pCmd,
  BYTE cmdLength);

/****************************************************************************/
/*                              FUNCTIONS                                   */
/****************************************************************************/

received_frame_status_t MultiChanCommandHandler(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  BYTE cmdLength)
{
  ZW_APPLICATION_TX_BUFFER * pTxBuf;
  TRANSMIT_OPTIONS_TYPE_SINGLE_EX * pTxOptionsEx;

  switch(pCmd->ZW_Common.cmd)
  {
    case MULTI_CHANNEL_END_POINT_GET_V4:
      if (TRUE == Check_not_legal_response_job(rxOpt))
      {
        // None of the following commands support endpoint bit addressing.
        return RECEIVED_FRAME_STATUS_FAIL;
      }

      pTxBuf = GetResponseBuffer();

      if (IS_NULL(pTxBuf))
      {
        return RECEIVED_FRAME_STATUS_FAIL;
      }

      RxToTxOptions(rxOpt, &pTxOptionsEx);

      pTxBuf->ZW_MultiChannelEndPointReportV4Frame.cmdClass      = COMMAND_CLASS_MULTI_CHANNEL_V4;
      pTxBuf->ZW_MultiChannelEndPointReportV4Frame.cmd           = MULTI_CHANNEL_END_POINT_REPORT_V4;
      GetMultiChannelEndPointFunctionality((EP_FUNCTIONALITY_DATA*) &pTxBuf->ZW_MultiChannelEndPointReportV4Frame.properties1);

      if(ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
          (BYTE *)pTxBuf,
          sizeof(ZW_MULTI_CHANNEL_END_POINT_REPORT_V4_FRAME),
          pTxOptionsEx,
          ZCB_ResponseJobStatus))
      {
        /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
        FreeResponseBuffer();
      }
      break;

    case MULTI_CHANNEL_CAPABILITY_GET_V4:
      if (TRUE == Check_not_legal_response_job(rxOpt))
      {
        // None of the following commands support endpoint bit addressing.
        return RECEIVED_FRAME_STATUS_FAIL;
      }
      pTxBuf = GetResponseBuffer();
      if (IS_NULL(pTxBuf))
      {
        return RECEIVED_FRAME_STATUS_FAIL;
      }
      if ((0 == (pCmd->ZW_MultiChannelCapabilityGetV4Frame.properties1 & 0x7F)) ||
          (NUMBER_OF_ENDPOINTS < (pCmd->ZW_MultiChannelCapabilityGetV4Frame.properties1 & 0x7F)))
      {
        FreeResponseBuffer();
        return RECEIVED_FRAME_STATUS_FAIL;
      }

      {
        ZW_MULTI_CHANNEL_CAPABILITY_GET_V4_FRAME * pCmdCap = (ZW_MULTI_CHANNEL_CAPABILITY_GET_V4_FRAME *)pCmd;
        DEVICE_CLASS* pDevClass = GetEndpointDeviceClass(pCmdCap->properties1 & 0x7F);
        CMD_CLASS_LIST* pCmdClassList = GetCommandClassList((0 != GetNodeId()), SECURITY_KEY_NONE, (pCmdCap->properties1 & 0x7F));
        uint8_t ccListSize = 0;

        RxToTxOptions(rxOpt, &pTxOptionsEx);

        pTxBuf->ZW_MultiChannelCapabilityReport4byteV4Frame.cmdClass    = COMMAND_CLASS_MULTI_CHANNEL_V4;
        pTxBuf->ZW_MultiChannelCapabilityReport4byteV4Frame.cmd         = MULTI_CHANNEL_CAPABILITY_REPORT_V4;
        pTxBuf->ZW_MultiChannelCapabilityReport4byteV4Frame.properties1 = (pCmdCap->properties1 & 0x7f);
        if (IS_NULL(pCmdClassList) || IS_NULL(pCmdClassList->pList) || (0 == pCmdClassList->size) || IS_NULL(pDevClass) || (0 == (pCmdCap->properties1 & 0x7f)))
        {
          FreeResponseBuffer();
          return RECEIVED_FRAME_STATUS_FAIL;
        }
        else
        {
          pTxBuf->ZW_MultiChannelCapabilityReport4byteV4Frame.genericDeviceClass  = pDevClass->genericDeviceClass;
          pTxBuf->ZW_MultiChannelCapabilityReport4byteV4Frame.specificDeviceClass = pDevClass->specificDeviceClass;
          memcpy( &(pTxBuf->ZW_MultiChannelCapabilityReport4byteV4Frame.commandClass1), pCmdClassList->pList, pCmdClassList->size);
          ccListSize = pCmdClassList->size;
        }

        if (ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
            (BYTE *)pTxBuf,
            sizeof(ZW_MULTI_CHANNEL_CAPABILITY_REPORT_1BYTE_V4_FRAME) + ccListSize - 1,
            pTxOptionsEx,
            ZCB_ResponseJobStatus))
        {
          /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
          FreeResponseBuffer();
        }
      }
      return RECEIVED_FRAME_STATUS_SUCCESS;
      break;

    case MULTI_CHANNEL_END_POINT_FIND_V4:
      if (TRUE == Check_not_legal_response_job(rxOpt))
      {
        return RECEIVED_FRAME_STATUS_FAIL;
      }

      pTxBuf = GetResponseBuffer();

      if (IS_NULL(pTxBuf))
      {
        return RECEIVED_FRAME_STATUS_FAIL;
      }

      RxToTxOptions(rxOpt, &pTxOptionsEx);

      {
        BYTE bufLen = 0;
        ZW_MULTI_CHANNEL_END_POINT_FIND_V4_FRAME* pcmdEpfind = (ZW_MULTI_CHANNEL_END_POINT_FIND_V4_FRAME *)pCmd;

        pTxBuf->ZW_MultiChannelEndPointFindReport1byteV4Frame.cmdClass = COMMAND_CLASS_MULTI_CHANNEL_V4;
        pTxBuf->ZW_MultiChannelEndPointFindReport1byteV4Frame.cmd = MULTI_CHANNEL_END_POINT_FIND_REPORT_V4;
        pTxBuf->ZW_MultiChannelEndPointFindReport1byteV4Frame.reportsToFollow = 0;
        pTxBuf->ZW_MultiChannelEndPointFindReport1byteV4Frame.genericDeviceClass = pcmdEpfind->genericDeviceClass;
        pTxBuf->ZW_MultiChannelEndPointFindReport1byteV4Frame.specificDeviceClass = pcmdEpfind->specificDeviceClass;
        bufLen = FindEndPoints(pcmdEpfind->genericDeviceClass,
                               pcmdEpfind->specificDeviceClass,
                               &pTxBuf->ZW_MultiChannelEndPointFindReport1byteV4Frame.variantgroup1.properties1);

        if(ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
            (BYTE *)pTxBuf,
            sizeof(ZW_MULTI_CHANNEL_END_POINT_FIND_REPORT_1BYTE_V4_FRAME) + bufLen - 1 ,
            pTxOptionsEx,
            ZCB_ResponseJobStatus))
        {
          /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
          FreeResponseBuffer();
        }
      }
      return RECEIVED_FRAME_STATUS_SUCCESS;
      break;
    case MULTI_CHANNEL_CMD_ENCAP_V4:
      CmdClassMultiChannelEncapsulateCmd(rxOpt,(ZW_MULTI_CHANNEL_CMD_ENCAP_V2_FRAME*) pCmd, cmdLength);
      return RECEIVED_FRAME_STATUS_SUCCESS;
      break;
    case MULTI_CHANNEL_AGGREGATED_MEMBERS_GET_V4:
      if (TRUE == Check_not_legal_response_job(rxOpt))
      {
        return RECEIVED_FRAME_STATUS_FAIL;
      }

      pTxBuf = GetResponseBuffer();

      if (IS_NULL(pTxBuf))
      {
        return RECEIVED_FRAME_STATUS_FAIL;
      }

      RxToTxOptions(rxOpt, &pTxOptionsEx);

      {
        ZW_MULTI_CHANNEL_AGGREGATED_MEMBERS_GET_V4_FRAME* pcmdEpAgg = (ZW_MULTI_CHANNEL_AGGREGATED_MEMBERS_GET_V4_FRAME *)pCmd;
        pTxBuf->ZW_MultiChannelAggregatedMembersReport1byteV4Frame.cmdClass = COMMAND_CLASS_MULTI_CHANNEL_V4;
        pTxBuf->ZW_MultiChannelAggregatedMembersReport1byteV4Frame.cmd = MULTI_CHANNEL_AGGREGATED_MEMBERS_REPORT_V4;
        pTxBuf->ZW_MultiChannelAggregatedMembersReport1byteV4Frame.properties1 = pcmdEpAgg->properties1;
        pTxBuf->ZW_MultiChannelAggregatedMembersReport1byteV4Frame.numberOfBitMasks =
          ReadAggregatedEndpointGroup( pcmdEpAgg->properties1,
                                       &pTxBuf->ZW_MultiChannelAggregatedMembersReport1byteV4Frame.aggregatedMembersBitMask1);

        if(ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
            (BYTE *)pTxBuf,
            sizeof(ZW_MULTI_CHANNEL_AGGREGATED_MEMBERS_REPORT_1BYTE_V4_FRAME) + pTxBuf->ZW_MultiChannelAggregatedMembersReport1byteV4Frame.numberOfBitMasks - 1 ,
            pTxOptionsEx,
            ZCB_ResponseJobStatus))
        {
          /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
          FreeResponseBuffer();
        }
      }
      return RECEIVED_FRAME_STATUS_SUCCESS;
      break;

  }
  return RECEIVED_FRAME_STATUS_NO_SUPPORT;
}

BOOL ZAF_CC_MultiChannel_IsCCSupported(
  RECEIVE_OPTIONS_TYPE_EX * pRxOpt,
  ZW_APPLICATION_TX_BUFFER * pCmd)
{
  if (0 == pRxOpt->destNode.endpoint)
  {
    return (TransportCmdClassSupported(pCmd->ZW_Common.cmdClass, pCmd->ZW_Common.cmd, pRxOpt->securityKey));
  }
  else
  {
    CMD_CLASS_LIST* pNonSec = GetEndpointcmdClassList(FALSE, pRxOpt->destNode.endpoint);
    CMD_CLASS_LIST* pSec    = GetEndpointcmdClassList(TRUE, pRxOpt->destNode.endpoint);
    if ((NULL != pNonSec) && (NULL != pSec))
    {
      return (CmdClassSupported(pRxOpt->securityKey,
                                pCmd->ZW_Common.cmdClass,
                                pCmd->ZW_Common.cmd,
                                pSec->pList, pSec->size,
                                pNonSec->pList, pNonSec->size));
    }
  }
  return FALSE;
}

/**
 * Extracts the content of a multichannel frame and call the application command handler.
 * @param rxOpt Receive options.
 * @param pCmd Given command.
 * @param cmdLength Length of the command.
 */
static void CmdClassMultiChannelEncapsulateCmd(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_MULTI_CHANNEL_CMD_ENCAP_V2_FRAME *pCmd,
  BYTE cmdLength)
{
  BYTE bitNo;

  //Setup source node endpoint
  rxOpt->sourceNode.endpoint = pCmd->properties1;

  if( 0x00 == (pCmd->properties2 & 0x80))
  {
    /*Non bit addressing */
    rxOpt->destNode.endpoint = pCmd->properties2 & 0x7f;
  }
  else
  {
    /*Bit addressing. Get is not legal why adding  RECEIVE_STATUS_TYPE_MULTI to rxStatus */
    rxOpt->rxStatus |= RECEIVE_STATUS_TYPE_MULTI;
    /* Need to extract endpoint bits. Just reset it*/
    rxOpt->destNode.endpoint = 0;
  }

  bitNo = 0;

  do /*BitAdress = 0 -> run only through once*/
  {
    /*BitAdress = 0 -> Destination address 1-127 */
    /*BitAdress = 1 -> Destination address is mask for End-point 1-7 */
    if(0 == (pCmd->properties2 & 0x80))/**End-point address**/
    {
      bitNo = 7;
    }
    else /**End-point bit mask address**/
    {
      while(0 == ((pCmd->properties2 & 0x7f) & (1<<bitNo)))/* Search through bitfied endpoint*/
      {
        bitNo++;
        /*validate bitNo is legal endpoint*/
        if((7 == bitNo) || NULL == GetEndpointcmdClassList(FALSE, bitNo)){
          return;
        }
      }
      rxOpt->destNode.endpoint = bitNo + 1;/*endpoint = bit x + 1*/
      rxOpt->destNode.BitAddress = 1; /*Tell Command Class that it is bit-addres. Get do not support bit-address!!*/
    }

    if (TRUE == ZAF_CC_MultiChannel_IsCCSupported(rxOpt, (ZW_APPLICATION_TX_BUFFER*)&pCmd->encapFrame))
    {
      /* Command class supported */
      Transport_ApplicationCommandHandlerEx(rxOpt, (ZW_APPLICATION_TX_BUFFER*)&pCmd->encapFrame, cmdLength - 4);
    }
    if (0 == rxOpt->destNode.endpoint)
    {
      return; /* No endpoint addressing! just return */
    }
  }while(7 > ++bitNo);
}

void CmdClassMultiChannelEncapsulate(
  BYTE **ppData,
  BYTE *dataLength,
  TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx)
{
  ZW_APPLICATION_TX_BUFFER *pTxBuf;
  BYTE sizeCmdFrameHeader;

  if (IS_NULL(*ppData)
   || ((0 == pTxOptionsEx->sourceEndpoint)
   && (0 == pTxOptionsEx->pDestNode->node.endpoint)))
  {
    return;
  }

  sizeCmdFrameHeader = sizeof(ZW_MULTI_CHANNEL_CMD_ENCAP_V2_FRAME) - sizeof(ALL_EXCEPT_ENCAP);
  *ppData -= sizeCmdFrameHeader;
  pTxBuf = (ZW_APPLICATION_TX_BUFFER *)*ppData;


  *dataLength += sizeCmdFrameHeader;

  pTxBuf->ZW_MultiChannelCmdEncapV2Frame.cmdClass = COMMAND_CLASS_MULTI_CHANNEL_V4;
  pTxBuf->ZW_MultiChannelCmdEncapV2Frame.cmd = MULTI_CHANNEL_CMD_ENCAP_V4;
  pTxBuf->ZW_MultiChannelCmdEncapV2Frame.properties1 = pTxOptionsEx->sourceEndpoint;
  pTxBuf->ZW_MultiChannelCmdEncapV2Frame.properties2 = (pTxOptionsEx->pDestNode->node.endpoint | (pTxOptionsEx->pDestNode->node.BitAddress << 0x07));
}

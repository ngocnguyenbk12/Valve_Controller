/***************************************************************************
*
* Copyright (c) 2001-2014
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: Some nice descriptive description.
*
* Author: Thomas Roll
*
* Last Changed By: $Author: tro $
* Revision: $Revision: 0.00 $
* Last Changed: $Date: 2014/08/21 13:28:13 $
*
****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include <ZW_mem_api.h>
#include <ZW_transport_api.h>
#include <CommandClassMultiChan.h>
#include <ZW_uart_api.h>
#include <ZW_TransportLayer.h>
#include <misc.h>
#include <ZW_tx_mutex.h>
#include <ZW_TransportEndpoint.h>
#ifndef NON_BATT
#include <ZAF_pm.h>
#endif

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

#ifdef TRANSPORT_ENDPOINT_DEBUG
#define TED_(x) x
#else
#define TED_(x)
#endif
#define _TED_(x) /*outcommon debug print*/

VOID_CALLBACKFUNC(pCallbackRequest)(TRANSMISSION_RESULT * pTransmissionResult);
VOID_CALLBACKFUNC(pCallbackResponse)(uint8_t status);

void ZCB_RequestCompleted(BYTE txStatus, TX_STATUS_TYPE* extendedTxStatus);
void ZCB_ResponseCompleted(BYTE txStatus, TX_STATUS_TYPE* extendedTxStatus);

typedef struct _CTRANSPORT_ENDPOINT_
{
  uint8_t flag_supervision_encap;
  EP_FUNCTIONALITY_DATA* pFunctionality;
  BYTE sizeList;
  EP_NIF* pList;
  AGGREGATED_GROUP_BIT_MASK* pMaskArray;
  BYTE sizeAggGroupArray;
  CMD_CLASS_LIST emptyList;
} CTRANSPORT_ENDPOINT;
/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/
static CTRANSPORT_ENDPOINT myTransportEndpoint =
{
  FALSE,
  (EP_FUNCTIONALITY_DATA*)NULL,
  0,
  (EP_NIF*)NULL,
  (AGGREGATED_GROUP_BIT_MASK*)NULL,
  0,
  {(BYTE*)NULL, 0}};
/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/



/*============================ Transport_AddEndpointSupport ================
** Function description
** This function...
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
Transport_AddEndpointSupport( EP_FUNCTIONALITY_DATA* pFunctionality, EP_NIF* pList, BYTE sizeList)
{
  myTransportEndpoint.pFunctionality = pFunctionality;
  myTransportEndpoint.pList = pList;
  myTransportEndpoint.sizeList = sizeList;
  myTransportEndpoint.pMaskArray = NULL;
  myTransportEndpoint.sizeAggGroupArray = 0;
  myTransportEndpoint.flag_supervision_encap = FALSE;
}


/*============================ Transport_SetupAggregationGroups =============
** Function description
** This function...
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
Transport_SetupAggregationGroups(AGGREGATED_GROUP_BIT_MASK* pMaskArray, BYTE sizeArray)
{
  if(myTransportEndpoint.pFunctionality->bits.nbrAggregatedEndpoints == sizeArray)
  {
    myTransportEndpoint.pMaskArray = pMaskArray;
    myTransportEndpoint.sizeAggGroupArray = sizeArray;
  }
}

/*============================ GetMultiChannelEndPointFunctionality ===============================
** Function description
** Read Node endpoint functionality
**
** return pointer to functionality of type EP_FUNCTIONALITY.
**
**-------------------------------------------------------------------------*/
void
GetMultiChannelEndPointFunctionality(EP_FUNCTIONALITY_DATA* pFunc)
{
  if(NON_NULL( myTransportEndpoint.pFunctionality ))
  {
    pFunc->bDdata[0] = myTransportEndpoint.pFunctionality->bits.resZero |
                     (myTransportEndpoint.pFunctionality->bits.identical << 6) |
                     (myTransportEndpoint.pFunctionality->bits.dynamic << 7);
    pFunc->bDdata[1] = (((myTransportEndpoint.pFunctionality->bits.nbrIndividualEndpoints |
                     (myTransportEndpoint.pFunctionality->bits.resIndZeorBit << 7))));
    pFunc->bDdata[2] = (((myTransportEndpoint.pFunctionality->bits.nbrAggregatedEndpoints |
                     (myTransportEndpoint.pFunctionality->bits.resAggZeorBit << 7))));
  }

}


BYTE eKeyToBitKey[] =
{
  0x00, SECURITY_KEY_S2_UNAUTHENTICATED_BIT, SECURITY_KEY_S2_AUTHENTICATED_BIT, SECURITY_KEY_S2_ACCESS_BIT, SECURITY_KEY_S0_BIT
};

BYTE ConvertEnumKeytoBitKey(enum SECURITY_KEY eKey)
{
  return eKeyToBitKey[eKey];
}

/*============================ GetEndpointDeviceClass =======================
** Function description
** This function...
**
**-------------------------------------------------------------------------*/
DEVICE_CLASS*
GetEndpointDeviceClass( BYTE endpoint)
{
  static DEVICE_CLASS devClass;
  ZW_DEBUG_SEND_NL();
  ZW_DEBUG_SEND_STR("GetEndpointDeviceClass ");
  ZW_DEBUG_SEND_NUM(endpoint);

  if( (endpoint > myTransportEndpoint.sizeList) || (0 == endpoint) )
  {
    return NULL;
  }

  devClass.genericDeviceClass = myTransportEndpoint.pList[endpoint - 1].genericDeviceClass;
  devClass.specificDeviceClass = myTransportEndpoint.pList[endpoint - 1].specificDeviceClass;

  return &devClass;
}


/*============================ GetEndpointcmdClassList ======================
**-------------------------------------------------------------------------*/
CMD_CLASS_LIST*
GetEndpointcmdClassList( BOOL secList, BYTE endpoint)
{
  if( (endpoint > myTransportEndpoint.sizeList) || (0 == endpoint) )
  {
    return NULL;
  }

  if(SECURITY_KEY_NONE == GetHighestSecureLevel(ZW_GetSecurityKeys()))
  {
    /*Non-secure included*/
    if(FALSE == secList){
      return &(myTransportEndpoint.pList[endpoint - 1].CmdClass3List.unsecList);
    }
    else {
      return &(myTransportEndpoint.emptyList);
    }
  }
  else{
    /*Secure included*/
    if(FALSE == secList){
      return &(myTransportEndpoint.pList[endpoint - 1].CmdClass3List.sec.unsecList);
    }
    else {
      return &(myTransportEndpoint.pList[endpoint - 1].CmdClass3List.sec.secList);
    }
  }
  return NULL;
}


/*============================ FindNextEndPoint ===============================
** Function description
** Search for next specified set of generic and specific
** device class in End Points.
** param genDeviceClass is endpoint generic device class
** param specDeviceClass is endpoint specific device class
** return endpoint. 0 if no endpoint.
**-------------------------------------------------------------------------*/
BYTE
FindEndPoints( BYTE genDeviceClass, BYTE specDeviceClass, BYTE* pEp)
{
  BYTE i;
  BYTE len =0;
  TED_( ZW_DEBUG_SEND_NL();)
  TED_( ZW_DEBUG_SEND_STR("TED: FindNextEndPoint ");)
  TED_( ZW_DEBUG_SEND_NUM(genDeviceClass);)
  TED_( ZW_DEBUG_SEND_NUM(specDeviceClass);)

  for(i = 0; i < myTransportEndpoint.sizeList; i++)
  {
    if( ((0xFF == genDeviceClass) && (0xFF == specDeviceClass) ) ||
        ((genDeviceClass == myTransportEndpoint.pList[i].genericDeviceClass) &&
         ((specDeviceClass == myTransportEndpoint.pList[i].specificDeviceClass) ||
         (specDeviceClass == 0xFF)))
      )
    {
      pEp[len++] = i + 1; /* Add one endpoint is from 1 to 127*/
      TED_( ZW_DEBUG_SEND_STR("ep ");)
      TED_( ZW_DEBUG_SEND_NUM(*(pEp + len - 1));)
    }
    TED_( ZW_DEBUG_SEND_BYTE('.');)
  }
  if( 0 == len)
  {
    /* deliver an endpoint list with EP1 = 0 */
    len = 1;
    pEp[0] = 0;
  }
  TED_( ZW_DEBUG_SEND_STR(" len=");)
  TED_( ZW_DEBUG_SEND_NUM(len);)
  TED_( ZW_DEBUG_SEND_NL();)
  return len;
}


/*============================ GetAggregatedEndpointGroup ===================
** Function description
** Read members of an aggregated endpoint
**
** Side effects:
** Return number of Aggregated Members Bit Mask bytes
**-------------------------------------------------------------------------*/
BYTE
ReadAggregatedEndpointGroup( BYTE aggregatedEndpoint, BYTE* pAggBitMask)
{
  BYTE aggBitMaskSize = 0;
  BYTE i = 0;
  /* Find aggregated Endpoint */

  for(i = 0; i < myTransportEndpoint.sizeAggGroupArray; i++)
  {
    if(aggregatedEndpoint == myTransportEndpoint.pMaskArray[i].aggregatedEndpoint)
    {
      /* Copy data bit mask*/
      memcpy(pAggBitMask, myTransportEndpoint.pMaskArray[i].pBitMask, myTransportEndpoint.pMaskArray[i].len);
      aggBitMaskSize = myTransportEndpoint.pMaskArray[i].len;
      break;
    }
  }
  return aggBitMaskSize;
}

enum ZW_SENDDATA_EX_RETURN_CODES
Transport_SendRequestEP(
  BYTE *pData,
  BYTE  dataLength,
  TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx,
  VOID_CALLBACKFUNC(pCallback)(TRANSMISSION_RESULT * pTransmissionResult))
{
  static TRANSMIT_OPTIONS_TYPE sTxOptions;
  // Check for multi channel
  if(FALSE == pTxOptionsEx->pDestNode->nodeInfo.BitMultiChannelEncap)
  {
    pTxOptionsEx->sourceEndpoint = 0;
  }

  /*
   * Save the function pointer to a variable in order to call it in
   * ZCB_RequestCompleted.
   */
  pCallbackRequest = pCallback;

  CmdClassMultiChannelEncapsulate(&pData,
                                  &dataLength,
                                  pTxOptionsEx);

  /**
   *  Build transmit options
   */
  sTxOptions.destNode = pTxOptionsEx->pDestNode->node.nodeId;
  sTxOptions.txOptions = pTxOptionsEx->txOptions;
  sTxOptions.txSecOptions = (pTxOptionsEx->txSecOptions | S2_TXOPTION_VERIFY_DELIVERY); /**<Always set verify options on request job*/
  sTxOptions.securityKey = pTxOptionsEx->pDestNode->nodeInfo.security;

  return ZW_SendDataEx(pData,
                       dataLength,
                       &sTxOptions,
                       ZCB_RequestCompleted);
}


enum ZW_SENDDATA_EX_RETURN_CODES
Transport_SendResponseEP(
  BYTE *pData,
  BYTE  dataLength,
  TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx,
  VOID_CALLBACKFUNC(pCallback)(BYTE bStatus))
{
  static TRANSMIT_OPTIONS_TYPE sTxOptions;

  pCallbackResponse = pCallback;

  if (IS_NULL(pTxOptionsEx))
  {
    return ZW_TX_FAILED;
  }

  /**
   *  Build transmit options
   */
  sTxOptions.destNode = pTxOptionsEx->pDestNode->node.nodeId;
  sTxOptions.txOptions = pTxOptionsEx->txOptions;
  sTxOptions.txSecOptions = pTxOptionsEx->txSecOptions;
  sTxOptions.securityKey = pTxOptionsEx->pDestNode->nodeInfo.security;

  CmdClassMultiChannelEncapsulate(&pData,
                                  &dataLength,
                                  pTxOptionsEx);

  return ZW_SendDataEx(pData,
                       dataLength,
                       &sTxOptions,
                       ZCB_ResponseCompleted);
}

void Transport_ApplicationCommandHandler(
    ZW_APPLICATION_TX_BUFFER * pCmd,
    BYTE cmdLength,
    RECEIVE_OPTIONS_TYPE * rxOpt)
{
  RECEIVE_OPTIONS_TYPE_EX rxOptEx;
  rxOptEx.rxStatus = rxOpt->rxStatus;
  rxOptEx.securityKey = rxOpt->securityKey;
  rxOptEx.sourceNode.nodeId = rxOpt->sourceNode;
  rxOptEx.sourceNode.endpoint = 0;
  rxOptEx.destNode.endpoint = 0;
  rxOptEx.destNode.BitAddress = 0;
  if( COMMAND_CLASS_MULTI_CHANNEL_V3 == pCmd->ZW_Common.cmdClass)
  {
    MultiChanCommandHandler(&rxOptEx, pCmd, cmdLength);
  }
  else
  {
    Transport_ApplicationCommandHandlerEx(&rxOptEx, pCmd, cmdLength);
  }
}

void
RxToTxOptions( RECEIVE_OPTIONS_TYPE_EX *rxopt,     /* IN  receive options to convert */
               TRANSMIT_OPTIONS_TYPE_SINGLE_EX** txopt)   /* OUT converted transmit options */
{
  static TRANSMIT_OPTIONS_TYPE_SINGLE_EX txOptionsEx;
  static MULTICHAN_NODE_ID destNode;
  txOptionsEx.pDestNode = &destNode;
  *txopt = &txOptionsEx;

  destNode.node.nodeId = rxopt->sourceNode.nodeId;
  destNode.node.endpoint = rxopt->sourceNode.endpoint;
  destNode.nodeInfo.BitMultiChannelEncap = (rxopt->sourceNode.endpoint == 0) ? 0 : 1;
  destNode.nodeInfo.security = rxopt->securityKey;


  txOptionsEx.txOptions = TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE | ZWAVE_PLUS_TX_OPTIONS;
  if (rxopt->rxStatus & RECEIVE_STATUS_LOW_POWER)
  {
      txOptionsEx.txOptions |= TRANSMIT_OPTION_LOW_POWER;
  }
  txOptionsEx.sourceEndpoint = rxopt->destNode.endpoint;
  txOptionsEx.txSecOptions = 0;
  txOptionsEx.pDestNode->nodeInfo.security = rxopt->securityKey;
}

BOOL
Check_not_legal_response_job(RECEIVE_OPTIONS_TYPE_EX *rxOpt) /*rxOpt pointer of type RECEIVE_OPTIONS_TYPE_EX */
{
  BOOL status = FALSE;

  /*
   * Check
   * 1: Get/Report do not support endpoint bit-addressing
   * 2: This check is to avoid that a response to a request is sent if the
   * request is not singlecast.
   * 3: Get command must no support Supervision encapsulation (CC:006C.01.00.13.002)
   */
  if((0 != rxOpt->destNode.BitAddress) ||
     (0 != (rxOpt->rxStatus & (RECEIVE_STATUS_TYPE_BROAD | RECEIVE_STATUS_TYPE_MULTI))) ||
     (TRUE == myTransportEndpoint.flag_supervision_encap))
  {

    status = TRUE;
  }
  myTransportEndpoint.flag_supervision_encap = FALSE;
  return status;
}

/**
 * This function is used as an abstraction layer for the lower layers just to
 * parse a status. This layer will parse a pointer of the type
 * TRANSMISSION_RESULT including the given status.
 */
PCB(ZCB_RequestCompleted)(BYTE txStatus, TX_STATUS_TYPE* extendedTxStatus)
{
  TRANSMISSION_RESULT transmissionResult;
  UNUSED(extendedTxStatus);

  transmissionResult.nodeId = 0;
  transmissionResult.status = txStatus;
  transmissionResult.isFinished = TRANSMISSION_RESULT_FINISHED;
  if (NON_NULL(pCallbackRequest))
  {
    pCallbackRequest(&transmissionResult);
  }
#ifndef NON_BATT
  ZAF_pm_KeepAwakeAuto();
#endif
}

PCB(ZCB_ResponseCompleted)(BYTE txStatus, TX_STATUS_TYPE* extendedTxStatus)
{
  UNUSED(extendedTxStatus);
  if (NON_NULL(pCallbackResponse))
  {
    pCallbackResponse(txStatus);
  }
#ifndef NON_BATT
  ZAF_pm_KeepAwakeAuto();
#endif
}


void
SetFlagSupervisionEncap(BOOL flag)
{
  myTransportEndpoint.flag_supervision_encap = flag;
}


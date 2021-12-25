/**
 * @file ZW_TransportMulticast.c
 * @author Christian Salmony Olsen
 * @date 09/09/2015
 * @brief Handles multicast frames in the Z-Wave Framework.
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include <ZW_stdint.h>
#include <ZW_typedefs.h>
#include <ZW_uart_api.h>
#include <ZW_nodemask_api.h>
#include <ZW_TransportEndpoint.h>
#include <misc.h>
#include <ZW_tx_mutex.h>
#include <ZW_TransportMulticast.h>
#include <ZW_transport_api.h>
#include <ZW_TransportSecProtocol.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

#ifdef ZW_DEBUG_TRANSPORT_MULTICAST
#define ZW_DEBUG__SEND_BYTE(data)     ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG__SEND_STR(STR)       ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG__SEND_NUM(data)      ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG__SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG__SEND_NL()           ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG__SEND_BYTE(data)
#define ZW_DEBUG__SEND_STR(STR)
#define ZW_DEBUG__SEND_NUM(data)
#define ZW_DEBUG__SEND_WORD_NUM(data)
#define ZW_DEBUG__SEND_NL()
#endif

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

VOID_CALLBACKFUNC(p_callback_hold)(TRANSMISSION_RESULT * pTransmissionResult);

static uint8_t * p_data_hold;
static uint8_t data_length_hold;

static TRANSMIT_OPTIONS_TYPE_EX * p_nodelist_hold;

static BOOL gotSupervision = FALSE;

static uint8_t singlecast_node_count;

static uint8_t fSupervisionEnableHold;

static uint8_t secKeys;

static uint8_t txSecOptions = 0;

static uint8_t node_mask[MAX_NODEMASK_LENGTH];
/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

#ifndef __C51__
// This is included for the sake of unit test.
extern void ZW_NodeMaskClear(BYTE_P pMask, BYTE bLength);
extern void ZW_NodeMaskSetBit(BYTE_P pMask, BYTE bNodeID);
enum ZW_SENDDATA_EX_RETURN_CODES ZW_SendDataMultiEx(
    BYTE *pData,
    BYTE dataLength,
    TRANSMIT_MULTI_OPTIONS_TYPE *pTxOptionsMultiEx,
    VOID_CALLBACKFUNC(completedFunc)(BYTE));
extern BYTE ZW_SendDataMulti(
    BYTE *pNodeIDList,
    BYTE *pData,
    BYTE  dataLength,
    BYTE  txOptions,
    VOID_CALLBACKFUNC(completedFunc)(BYTE));
#endif

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

void ZCB_callback_wrapper(uint8_t status);
void ZCB_multicast_callback(TRANSMISSION_RESULT * pTransmissionResult);

enum ZW_SENDDATA_EX_RETURN_CODES
ZW_TransportMulticast_SendRequest(const uint8_t * const p_data,
                                  uint8_t data_length,
                                  uint8_t fSupervisionEnable,
                                  TRANSMIT_OPTIONS_TYPE_EX * p_nodelist,
                                  VOID_CALLBACKFUNC(p_callback)(TRANSMISSION_RESULT * pTransmissionResult))
{
  uint8_t node_count;
  uint8_t s2_node_count;
  uint8_t nonsecure_node_count;

  if (IS_NULL(p_nodelist) || 0 == p_nodelist->list_length || IS_NULL(p_data) || 0 == data_length)
  {
    return ZW_TX_FAILED;
  }

  p_callback_hold = p_callback;
  p_nodelist_hold = p_nodelist;
  p_data_hold = (uint8_t *)p_data;
  data_length_hold = data_length;

  if (TRUE != RequestBufferSetPayloadLength((ZW_APPLICATION_TX_BUFFER *)p_data, data_length))
  {
    // Something is fucked up.
    return ZW_TX_FAILED;
  }

  // Set the supervision hold variable in order to use it for singlecast
  fSupervisionEnableHold = fSupervisionEnable;

  // Get the active security keys
  secKeys = ZW_GetSecurityKeys();

  ZW_DEBUG__SEND_NL();
  ZW_DEBUG__SEND_STR("Sec: ");
  ZW_DEBUG__SEND_NUM(secKeys);

  // Use supervision if security scheme is 2 and supervision is enabled.
  if (TRUE != RequestBufferSupervisionPayloadActivate((ZW_APPLICATION_TX_BUFFER**)&p_data_hold, &data_length_hold, ((0 != (SECURITY_KEY_S2_MASK & secKeys)) && fSupervisionEnableHold)))
  {
    // Something is wrong.
    return ZW_TX_FAILED;
  }

  ZW_NodeMaskClear(node_mask, sizeof(node_mask));

  s2_node_count = 0;
  nonsecure_node_count = 0;
  for (node_count = 0; node_count < p_nodelist->list_length; node_count++)
  {

    //Do not support end-points in multicast!
    if(0 == p_nodelist->pList[node_count].node.endpoint)
    {
      if (SECURITY_KEY_S2_UNAUTHENTICATED == p_nodelist->pList[node_count].nodeInfo.security ||
          SECURITY_KEY_S2_AUTHENTICATED   == p_nodelist->pList[node_count].nodeInfo.security ||
          SECURITY_KEY_S2_ACCESS          == p_nodelist->pList[node_count].nodeInfo.security)
      {
        // The node is S2.
        s2_node_count++;
      }
      else if (SECURITY_KEY_NONE == p_nodelist->pList[node_count].nodeInfo.security)
      {
        // The node is non-secure.
        nonsecure_node_count++;
        ZW_NodeMaskSetBit(node_mask, p_nodelist->pList[node_count].node.nodeId);
      }

      if (1 < s2_node_count)
      {
        // We got two S2 nodes => Skip the rest and go to S2 multicast.
        break;
      }
    }
  }

  singlecast_node_count = 0;
  txSecOptions = 0;

  if (1 < s2_node_count)
  {
    // We got more than one s2 node => Transmit S2 multicast.
    TRANSMIT_MULTI_OPTIONS_TYPE txOptionsMultiEx;

    txOptionsMultiEx.groupID = p_nodelist->S2_groupID;
    txOptionsMultiEx.bSrcNode = 0xFF;
    txOptionsMultiEx.txOptions = 0;
    txOptionsMultiEx.securityKey = GetHighestSecureLevel(ZW_GetSecurityKeys());
    txSecOptions = S2_TXOPTION_VERIFY_DELIVERY | S2_TXOPTION_SINGLECAST_FOLLOWUP;

    ZW_DEBUG_SEND_STR("\r\nMulti dest grp ID: ");
    ZW_DEBUG_SEND_NUM(txOptionsMultiEx.groupID);
    ZW_DEBUG_SEND_NL();

    return (ZW_SendDataMultiEx((BYTE *)p_data, data_length, &txOptionsMultiEx, ZCB_callback_wrapper));
  }
  else if (1 < nonsecure_node_count)
  {
    if (!ZW_SendDataMulti(node_mask, (BYTE *)p_data, data_length, 0, ZCB_callback_wrapper))
    {
      return ZW_TX_FAILED;
    }
    else
    {
      return ZW_TX_IN_PROGRESS;
    }
  }
  else
  {
    /*
     * If there's only one non-endpoint node, we skip the multicast and call
     * the multicast callback directly. This callback will initiate singlecast
     * transmission. The argument can be ignored.
     */
    ZCB_multicast_callback(NULL);
    return ZW_TX_IN_PROGRESS;
  }
}

PCB(ZCB_callback_wrapper)(uint8_t status)
{
  UNUSED(status);
  ZCB_multicast_callback(NULL);
}

/**
 * This function will be called from two sources:
 * 1. when multicast is done OR when there's no need for multicast.
 * 2. when a transmission is done.
 *
 * It will initiate transmission of singlecast frames for each of the nodes
 * included in the multicast.
 */
PCB(ZCB_multicast_callback)(TRANSMISSION_RESULT * pTransmissionResult)
{
  TRANSMISSION_RESULT_FINISH_STATUS isFinished = TRANSMISSION_RESULT_NOT_FINISHED;
  static TRANSMISSION_RESULT transmissionResult;
  enum ZW_SENDDATA_EX_RETURN_CODES txResult;

  ZW_DEBUG__SEND_NL();
  ZW_DEBUG__SEND_BYTE('c');

  if (singlecast_node_count > 0)
  {
    /*
     * When singlecast_node_count is higher than zero, it means that the call
     * to this function is a callback when transmission is done or failed.
     */

    // Check whether to set the finish flag.
    if (singlecast_node_count == p_nodelist_hold->list_length)
    {
      isFinished = TRANSMISSION_RESULT_FINISHED;
      ZW_DEBUG__SEND_NL();
      ZW_DEBUG__SEND_STR("Transmission done!");
    }
    else
    {
      isFinished = TRANSMISSION_RESULT_NOT_FINISHED;
    }

    transmissionResult.nodeId = pTransmissionResult->nodeId;
    transmissionResult.isFinished = isFinished;
    transmissionResult.status = pTransmissionResult->status;

    if (NON_NULL(p_callback_hold))
    {
      p_callback_hold(&transmissionResult); // TODO: What about endpoints...?
    }
  }

  if (singlecast_node_count < p_nodelist_hold->list_length)
  {
    TRANSMIT_OPTIONS_TYPE_SINGLE_EX txOptions;

    txOptions.txOptions = p_nodelist_hold->txOptions;
    txOptions.txSecOptions = txSecOptions;
    if(0 == singlecast_node_count && (txSecOptions & S2_TXOPTION_SINGLECAST_FOLLOWUP) )
    {
      txOptions.txSecOptions |= S2_TXOPTION_FIRST_SINGLECAST_FOLLOWUP;
    }

    txOptions.sourceEndpoint = p_nodelist_hold->sourceEndpoint;
    txOptions.pDestNode = &p_nodelist_hold->pList[singlecast_node_count];
    transmissionResult.nodeId = txOptions.pDestNode->node.nodeId;

    /*
    ZW_DEBUG__SEND_NL();
    ZW_DEBUG__SEND_NL();
    ZW_DEBUG__SEND_STR("Node C/ID: ");
    ZW_DEBUG__SEND_NUM(singlecast_node_count);
    ZW_DEBUG__SEND_BYTE('/');
    ZW_DEBUG__SEND_NUM((BYTE)(txOptions.pDestNode->node.nodeId));

    ZW_DEBUG__SEND_NL();
    ZW_DEBUG__SEND_STR("Sec scheme: ");
    ZW_DEBUG__SEND_NUM(txOptions.pDestNode->nodeInfo.security);
    ZW_DEBUG__SEND_NUM(ZW_GetSecurityKeys());
    */

    if (TRUE != RequestBufferSupervisionPayloadActivate((ZW_APPLICATION_TX_BUFFER**)&p_data_hold, &data_length_hold, ((0 != (SECURITY_KEY_S2_MASK & secKeys)) && fSupervisionEnableHold && (0 == txOptions.pDestNode->node.BitAddress))))
    {
      transmissionResult.nodeId = 0;
      transmissionResult.status = ZW_TX_FAILED;
      transmissionResult.isFinished = TRANSMISSION_RESULT_FINISHED;

      if (NON_NULL(p_callback_hold))
      {
        p_callback_hold(&transmissionResult);
      }
      return;
    }

    txResult = Transport_SendRequestEP(
                  p_data_hold,
                  data_length_hold,
                  &txOptions,
                  ZCB_multicast_callback);

    if (ZW_TX_IN_PROGRESS != txResult)
    {
      ZW_DEBUG__SEND_NL();
      ZW_DEBUG__SEND_STR("Error: ");
      ZW_DEBUG__SEND_NUM(txResult);
      transmissionResult.nodeId = 0;
      transmissionResult.status = TRANSMIT_COMPLETE_FAIL;
      transmissionResult.isFinished = TRANSMISSION_RESULT_FINISHED;

      if (NON_NULL(p_callback_hold))
      {
        p_callback_hold(&transmissionResult);
      }
      return;
    }
    singlecast_node_count++;
  }
}

void ZW_TransportMulticast_clearTimeout(void)
{
  gotSupervision = TRUE;
  ZCB_multicast_callback(0);
}

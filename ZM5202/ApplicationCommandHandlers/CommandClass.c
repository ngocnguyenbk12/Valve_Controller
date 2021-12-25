/**
 * @file
 * Common types and definitions for all command classes.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

#include <ZW_typedefs.h>
#include <ZW_TransportMulticast.h>
#include <CommandClass.h>
#include <ZW_mem_api.h>
#include <agi.h>

#define MAX_PAYLOAD 150

typedef struct _ZW_ENGINE_FRAME_
{
  CMD_CLASS_GRP grp;
   BYTE payload[MAX_PAYLOAD];
} ZW_ENGINE_FRAME;



JOB_STATUS cc_engine_multicast_request(
    AGI_PROFILE* pProfile,
    uint8_t endpoint,
    CMD_CLASS_GRP *pcmdGrp,
    uint8_t* pPayload,
    uint8_t size,
    uint8_t fSupervisionEnable,
    VOID_CALLBACKFUNC(pCbFunc)(TRANSMISSION_RESULT * pTransmissionResult))
{
  uint8_t frame_len = sizeof(CMD_CLASS_GRP);
  ZW_ENGINE_FRAME * pTxBuf = (ZW_ENGINE_FRAME *)GetRequestBuffer(pCbFunc);
 if( IS_NULL( pTxBuf ) )
  {
    /*Ongoing job is active.. just stop current job*/
    return JOB_STATUS_BUSY;
  }
  else
  {
    TRANSMIT_OPTIONS_TYPE_EX* pTxOptionsEx = NULL;

    pTxBuf->grp.cmdClass = pcmdGrp->cmdClass;
    pTxBuf->grp.cmd = pcmdGrp->cmd;

    if( 0 != size )
    {
      memcpy(pTxBuf->payload, pPayload, size);
      frame_len += size;
    }

    /*Get transmit options (node list)*/
    pTxOptionsEx = ReqNodeList( pProfile,
                          &(pTxBuf->grp),
                          endpoint);

    if( IS_NULL( pTxOptionsEx ) )
    {
      /*Job failed, free transmit-buffer pTxBuf by cleaning mutex */
      FreeRequestBuffer();
      return JOB_STATUS_NO_DESTINATIONS;
    }

    if(ZW_TX_IN_PROGRESS != ZW_TransportMulticast_SendRequest(
      (BYTE *)pTxBuf,
      frame_len,
      fSupervisionEnable,
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



#if 0

JOB_STATUS cc_engine_singlecast_request(
    TRANSMIT_OPTIONS_TYPE_SINGLE_EX* pTxOptionsEx,
    CMD_CLASS_GRP *pcmdGrp,
    uint8_t* pPayload,
    uint8_t size,
    VOID_CALLBACKFUNC(pCbFunc)(TRANSMISSION_RESULT * pTransmissionResult))
{
  uint8_t frame_len = sizeof(CMD_CLASS_GRP);

  ZW_ENGINE_FRAME *pTxBuf = GetRequestBuffer(pCbFunc);
  if (IS_NULL( pTxBuf ))
  {
    /*Ongoing job is active.. just stop current job*/
    return JOB_STATUS_BUSY;
  }

  pTxBuf->grp.cmdClass = pcmdGrp->cmdClass;
  pTxBuf->grp.cmd = pcmdGrp->cmd;

  if( 0 != size )
  {
    memcpy(pTxBuf->payload, pPayload, size);
    frame_len += size;
  }

  if(ZW_TX_IN_PROGRESS != Transport_SendRequestEP(
      (BYTE *)pTxBuf,
      sizeof(ZW_SUPERVISION_REPORT_FRAME),
      pTxOptionsEx,
      ZCB_RequestJobStatus))
  {
    /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
     FreeRequestBuffer();
     return JOB_STATUS_BUSY;
  }
  return JOB_STATUS_SUCCESS;
}
#endif


/**
 * @file
 * Handler for Command Class Supervision.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_non_zero.h>
#include <ZW_stdint.h>
#include <ZW_TransportEndpoint.h>
#include <ZW_TransportSecProtocol.h>
#include "config_app.h"
#include <CommandClassMultiChan.h>
#include <CommandClassSupervision.h>
#include <ZW_uart_api.h>
#include <ZW_TransportMulticast.h>
#include <misc.h>
#include <ZW_tx_mutex.h>
#include <ZW_mem_api.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

#ifdef ZW_DEBUG_CC_SUPERVISION
#define ZW_DEBUG_CC_SUPERVISION_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_CC_SUPERVISION_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_CC_SUPERVISION_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_CC_SUPERVISION_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_CC_SUPERVISION_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_CC_SUPERVISION_SEND_BYTE(data)
#define ZW_DEBUG_CC_SUPERVISION_SEND_STR(STR)
#define ZW_DEBUG_CC_SUPERVISION_SEND_NUM(data)
#define ZW_DEBUG_CC_SUPERVISION_SEND_WORD_NUM(data)
#define ZW_DEBUG_CC_SUPERVISION_SEND_NL()
#endif

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

#if defined(BATTERY) || defined(BATTERY__CC_WAKEUP)
extern uint8_t xdata m_sessionId;
extern uint8_t xdata previously_receive_session_id;
extern uint8_t xdata previously_rxStatus;
extern MULTICHAN_DEST_NODE_ID xdata previously_received_destination;
#else
static uint8_t m_sessionId = 0;
static uint8_t previously_receive_session_id = 0;
static uint8_t previously_rxStatus = 0;
static MULTICHAN_DEST_NODE_ID previously_received_destination = {0, 0, 0};
#endif

static uint8_t m_CommandLength = 0;

static cc_supervision_status_updates_t m_status_updates = CC_SUPERVISION_STATUS_UPDATES_NOT_SUPPORTED;
VOID_CALLBACKFUNC(m_pGetReceivedHandler)(SUPERVISION_GET_RECEIVED_HANDLER_ARGS * pArgs) = NULL;
VOID_CALLBACKFUNC(m_pReportReceivedHandler)(cc_supervision_status_t status, uint8_t duration) = NULL;

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

void
CommandClassSupervisionInit( cc_supervision_status_updates_t status_updates,
        VOID_CALLBACKFUNC(pGetReceivedHandler)(SUPERVISION_GET_RECEIVED_HANDLER_ARGS * pArgs),
        VOID_CALLBACKFUNC(pReportReceivedHandler)(cc_supervision_status_t status, uint8_t duration))
{
  m_status_updates = status_updates;
  m_pGetReceivedHandler = pGetReceivedHandler;
  m_pReportReceivedHandler = pReportReceivedHandler;
}

/**
 * Returns whether the command must be handled or not based on Node ID and session ID.
 * @param nodeID Destination Node ID including endpoint.
 * @param sessionID Received session ID.
 * @return Returns TRUE if the command must be handled. FALSE otherwise.
 */
static BOOL mustCommandBeHandled(MULTICHAN_DEST_NODE_ID nodeID, uint8_t sessionID)
{
  ZW_DEBUG_CC_SUPERVISION_SEND_NL();
  ZW_DEBUG_CC_SUPERVISION_SEND_NUM(previously_receive_session_id);
  ZW_DEBUG_CC_SUPERVISION_SEND_NUM(sessionID);
  if (previously_receive_session_id != sessionID)
  {
    return TRUE;
  }
  if (0 != memcmp((uint8_t *)&nodeID, (uint8_t *)&previously_received_destination, sizeof(MULTICHAN_DEST_NODE_ID)))
  {
    return TRUE;
  }
  return FALSE;
}

received_frame_status_t handleCommandClassSupervision(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  uint8_t cmdLength)
{

  UNUSED(cmdLength);

  switch (pCmd->ZW_Common.cmd)
  {
    case SUPERVISION_GET:
    {
      /**
       * SUPERVISION_GET handle:
       * 1. Single-cast:
       *    a. Transport_ApplicationCommandHandlerEx() is called by checking previously_receive_session_id.
       *    b. single-cast trigger supervision_report is send back.
       *
       * 2. Multi-cast:
       *    a. If multi-cast is received (rxStatus includes flag RECEIVE_STATUS_TYPE_MULTI).
       *    b. Transport_ApplicationCommandHandlerEx() is called
       *    c. Do not send supervision_report.
       *
       * 3. Multi-cast single-cast follow up:
       *    a. Transport_ApplicationCommandHandlerEx is discarded on single-cast by checking previously_receive_session_id.
       *    b. single-cast trigger supervision_report is send back.
       *
       * 4. Single-cast CC multichannel bit-adr.:
       *    CommandClassMultiChan handle bit addressing by calling each endpoint with the payload.
       *    a. If Single-cast CC multichannel bit-adr. (rxStatus includes flag RECEIVE_STATUS_TYPE_MULTI).
       *    b. Transport_ApplicationCommandHandlerEx() must be called every time. Check previously_received_destination
       *       differ from EXTRACT_SESSION_ID(pCmd->ZW_SupervisionGetFrame.sessionid)
       *    c. Do not send supervision_report.
       */
      uint8_t properties1;
      /*
       * status need to be static to handle multi-cast single-cast follow up.
       * Multi-cast get status Transport_ApplicationCommandHandlerEx() and sing-cast send Supervision report.
       */
      static cc_supervision_status_t status = CC_SUPERVISION_STATUS_NOT_SUPPORTED;
      uint8_t duration;
      TRANSMIT_OPTIONS_TYPE_SINGLE_EX * pTxOptions;

      SetFlagSupervisionEncap(TRUE);

      if(previously_receive_session_id != CC_SUPERVISION_EXTRACT_SESSION_ID(pCmd->ZW_SupervisionGetFrame.properties1))
      {
        /*
         * Reset status session id is changed.
         */
        ZW_DEBUG_CC_SUPERVISION_SEND_BYTE('a');
        status = CC_SUPERVISION_STATUS_NOT_SUPPORTED;
      }
      /* ZF-378, ZF-379 fix - Make sure encapsulated CmdClass are supported (including possible endpoints) using current keyclass */
      if (TRUE == ZAF_CC_MultiChannel_IsCCSupported(rxOpt, (ZW_APPLICATION_TX_BUFFER *)(((uint8_t *)pCmd) + sizeof(ZW_SUPERVISION_GET_FRAME))))
      {
        if (TRUE == mustCommandBeHandled(rxOpt->destNode, CC_SUPERVISION_EXTRACT_SESSION_ID(pCmd->ZW_SupervisionGetFrame.properties1)))
        {
          ZW_DEBUG_CC_SUPERVISION_SEND_BYTE('b');
          status = (cc_supervision_status_t)Transport_ApplicationCommandHandlerEx(
                  rxOpt,
                  (ZW_APPLICATION_TX_BUFFER *)(((uint8_t *)pCmd) + sizeof(ZW_SUPERVISION_GET_FRAME)),
                  (pCmd->ZW_SupervisionGetFrame.encapsulatedCommandLength));
        }
      }
      else
      {
        status = CC_SUPERVISION_STATUS_NOT_SUPPORTED;
      }
      /*
       * Deactivate FlagSupervisionEncap because frame is extracted. (ZF_235)
       */
      SetFlagSupervisionEncap(FALSE);

      /*
       * Check whether the frame was sent using multicast or broadcast. In that case we MUST NOT
       * respond to it.
       * RECEIVE_STATUS_TYPE_MULTI applies in two scenarios: non-securely and Multichannel endpoint
       * bit addressing frames.
       * RECEIVE_STATUS_TYPE_BROAD applies in the case of S2 multicast frames.
       */
      if ((rxOpt->rxStatus & RECEIVE_STATUS_TYPE_MULTI) ||
          (rxOpt->rxStatus & RECEIVE_STATUS_TYPE_BROAD))
      {
        ZW_DEBUG_CC_SUPERVISION_SEND_BYTE('c');
        if (FALSE == mustCommandBeHandled(rxOpt->destNode, CC_SUPERVISION_EXTRACT_SESSION_ID(pCmd->ZW_SupervisionGetFrame.properties1)))
        {
          return RECEIVED_FRAME_STATUS_FAIL;
        }

        /*update previously session-id and reset previously_received_destination [node-id + endpoint]*/
        previously_receive_session_id = CC_SUPERVISION_EXTRACT_SESSION_ID(pCmd->ZW_SupervisionGetFrame.properties1);
        memcpy((uint8_t*)&previously_received_destination, (uint8_t*)&rxOpt->destNode, sizeof(MULTICHAN_DEST_NODE_ID));
        previously_rxStatus = rxOpt->rxStatus;

        return RECEIVED_FRAME_STATUS_SUCCESS;
      }
      else
      {
        /*
         * In this case the frame is sent using singlecast.
         *
         * We cannot respond to a singlecast in the following scenarios:
         * - Session ID is unchanged from last singlecast
         */
          if ((FALSE == mustCommandBeHandled(rxOpt->destNode, CC_SUPERVISION_EXTRACT_SESSION_ID(pCmd->ZW_SupervisionGetFrame.properties1))) &&
              (0 == previously_rxStatus))
        {
          ZW_DEBUG_CC_SUPERVISION_SEND_BYTE('e');
          return RECEIVED_FRAME_STATUS_FAIL;
        }
      }
      /*update previously session-id and reset previously_received_destination [node-id + endpoint]*/
      previously_receive_session_id = CC_SUPERVISION_EXTRACT_SESSION_ID(pCmd->ZW_SupervisionGetFrame.properties1);
      memcpy((uint8_t*)&previously_received_destination, (uint8_t*)&rxOpt->destNode, sizeof(MULTICHAN_DEST_NODE_ID));
      previously_rxStatus = 0;

      duration = 0;
      properties1 = CC_SUPERVISION_EXTRACT_SESSION_ID(pCmd->ZW_SupervisionGetFrame.properties1);
      properties1 |= CC_SUPERVISION_ADD_MORE_STATUS_UPDATE(CC_SUPERVISION_MORE_STATUS_UPDATES_THIS_IS_LAST);
      if ((CC_SUPERVISION_STATUS_FAIL != status) &&
          (CC_SUPERVISION_STATUS_NOT_SUPPORTED != status) &&
          NON_NULL(m_pGetReceivedHandler))
      {
        // Call the assigned function.
        SUPERVISION_GET_RECEIVED_HANDLER_ARGS args;

        args.cmdClass = *(((uint8_t *)pCmd) + sizeof(ZW_SUPERVISION_GET_FRAME));
        args.cmd      = *(((uint8_t *)pCmd) + sizeof(ZW_SUPERVISION_GET_FRAME) + 1);
        args.properties1 = pCmd->ZW_SupervisionGetFrame.properties1;
        args.rxOpt = rxOpt;
        args.status = status;
        args.duration = 0;

        m_pGetReceivedHandler(&args);

        status = args.status;
        duration = args.duration;
        properties1 = args.properties1;
      }

      // When we have gotten the information, we can send a Supervision report.
      RxToTxOptions(rxOpt, &pTxOptions);
      CmdClassSupervisionReportSend(
              pTxOptions,
              properties1,
              status,
              duration);
    }
    return RECEIVED_FRAME_STATUS_SUCCESS;
    break;

    case SUPERVISION_REPORT:
      if (((m_sessionId - 1) & 0x3F) == pCmd->ZW_SupervisionReportFrame.properties1)
      {
        // The received session ID matches the one we sent.
        if (NON_NULL(m_pReportReceivedHandler))
        {
          m_pReportReceivedHandler(
                  pCmd->ZW_SupervisionReportFrame.status,
                  pCmd->ZW_SupervisionReportFrame.duration);
        }

        ZW_TransportMulticast_clearTimeout();
      }
      return RECEIVED_FRAME_STATUS_SUCCESS;
      break;
  }
  return RECEIVED_FRAME_STATUS_NO_SUPPORT;
}

JOB_STATUS CmdClassSupervisionReportSend(
  TRANSMIT_OPTIONS_TYPE_SINGLE_EX* pTxOptionsEx,
  uint8_t properties,
  cc_supervision_status_t status,
  uint8_t duration)
{
  ZW_APPLICATION_TX_BUFFER *pTxBuf = GetResponseBuffer();
  if (IS_NULL( pTxBuf ))
  {
    /*Ongoing job is active.. just stop current job*/
    return JOB_STATUS_BUSY;
  }

  pTxBuf->ZW_SupervisionReportFrame.cmdClass = COMMAND_CLASS_SUPERVISION;
  pTxBuf->ZW_SupervisionReportFrame.cmd = SUPERVISION_REPORT;
  pTxBuf->ZW_SupervisionReportFrame.properties1 = properties;
  pTxBuf->ZW_SupervisionReportFrame.status = status;
  pTxBuf->ZW_SupervisionReportFrame.duration = duration;

  if(ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
      (uint8_t *)pTxBuf,
      sizeof(ZW_SUPERVISION_REPORT_FRAME),
      pTxOptionsEx,
      ZCB_ResponseJobStatus))
  {
    /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
     FreeResponseBuffer();
     return JOB_STATUS_BUSY;
  }
  return JOB_STATUS_SUCCESS;
}

void CommandClassSupervisionGetAdd(ZW_SUPERVISION_GET_FRAME* pbuf)
{
  m_CommandLength = 0;
  CommandClassSupervisionGetWrite(pbuf);
  m_sessionId = (m_sessionId + 1) & 0x3F; /* increment m_sessionId, wrap around in range 0..0x3F */
}

void CommandClassSupervisionGetWrite(ZW_SUPERVISION_GET_FRAME* pbuf)
{
  pbuf->cmdClass =  COMMAND_CLASS_SUPERVISION;
  pbuf->cmd = SUPERVISION_GET;
  pbuf->properties1 = CC_SUPERVISION_ADD_SESSION_ID(m_sessionId);
  pbuf->properties1 |= CC_SUPERVISION_ADD_STATUS_UPDATE(m_status_updates);
  pbuf->encapsulatedCommandLength = m_CommandLength;
}

void CommandClassSupervisionGetSetPayloadLength(ZW_SUPERVISION_GET_FRAME* pbuf, uint8_t payLoadlen)
{
  pbuf->encapsulatedCommandLength = payLoadlen;
  m_CommandLength = payLoadlen;
}

uint8_t CommandClassSupervisionGetGetPayloadLength(ZW_SUPERVISION_GET_FRAME* pbuf)
{
  return pbuf->encapsulatedCommandLength;
}

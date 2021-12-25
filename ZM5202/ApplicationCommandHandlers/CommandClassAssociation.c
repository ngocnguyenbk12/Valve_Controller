/**
 * @file
 * Handler for Command Class Association.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include <ZW_basis_api.h>
#include <ZW_tx_mutex.h>
#include <ZW_TransportLayer.h>
#include <association_plus.h>
#include <CommandClassAssociation.h>
#include <misc.h>
#include <ZW_uart_api.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

#ifdef ZW_DEBUG_CMD_ASSOCIATION
#define ZW_DEBUG_CMD_ASSOCIATION_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_CMD_ASSOCIATION_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_CMD_ASSOCIATION_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_CMD_ASSOCIATION_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_CMD_ASSOCIATION_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_CMD_ASSOCIATION_SEND_BYTE(data)
#define ZW_DEBUG_CMD_ASSOCIATION_SEND_STR(STR)
#define ZW_DEBUG_CMD_ASSOCIATION_SEND_NUM(data)
#define ZW_DEBUG_CMD_ASSOCIATION_SEND_WORD_NUM(data)
#define ZW_DEBUG_CMD_ASSOCIATION_SEND_NL()
#endif

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

received_frame_status_t
handleCommandClassAssociation(
    RECEIVE_OPTIONS_TYPE_EX *rxOpt,
    ZW_APPLICATION_TX_BUFFER *pCmd,
    BYTE cmdLength
)
{
  ZW_APPLICATION_TX_BUFFER * pTxBuf;
  TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx;
  uint8_t txResult;

  switch (pCmd->ZW_Common.cmd)
  {
    case ASSOCIATION_GET_V2:
      {
        uint8_t outgoingFrameLength;

        if (TRUE == Check_not_legal_response_job(rxOpt))
        {
          // Get/Report do not support endpoint bit addressing.
          return RECEIVED_FRAME_STATUS_FAIL;
        }

        pTxBuf = GetResponseBuffer();
        if (IS_NULL(pTxBuf))
        {
          return RECEIVED_FRAME_STATUS_FAIL;
        }

        AssociationGet(
            rxOpt->destNode.endpoint,
            (uint8_t *)&pCmd->ZW_AssociationGetFrame.cmdClass,
            (uint8_t *)pTxBuf,
            &outgoingFrameLength);

        RxToTxOptions(rxOpt, &pTxOptionsEx);

        // Transmit the stuff.
        txResult = Transport_SendResponseEP(
            (BYTE *)pTxBuf,
            outgoingFrameLength,
            pTxOptionsEx,
            ZCB_ResponseJobStatus);
        if (ZW_TX_IN_PROGRESS != txResult)
        {
          FreeResponseBuffer();
          return RECEIVED_FRAME_STATUS_FAIL;
        }
        return RECEIVED_FRAME_STATUS_SUCCESS;
      }
      break;

    case ASSOCIATION_SET_V2:
      if (TRUE != handleAssociationSet(
          rxOpt->destNode.endpoint,
          (ZW_MULTI_CHANNEL_ASSOCIATION_SET_1BYTE_V2_FRAME*)pCmd,
          cmdLength))
      {
        return RECEIVED_FRAME_STATUS_FAIL;
      }
      return RECEIVED_FRAME_STATUS_SUCCESS;
      break;

    case ASSOCIATION_REMOVE_V2:
      if (3 > cmdLength)
      {
        return RECEIVED_FRAME_STATUS_FAIL;
      }
      if (TRUE != AssociationRemove(
          pCmd->ZW_AssociationRemove1byteV2Frame.groupingIdentifier,
          rxOpt->destNode.endpoint,
          (ZW_MULTI_CHANNEL_ASSOCIATION_REMOVE_1BYTE_V2_FRAME*)pCmd,
          cmdLength))
      {
        return RECEIVED_FRAME_STATUS_FAIL;
      }
      return RECEIVED_FRAME_STATUS_SUCCESS;
      break;

    case ASSOCIATION_GROUPINGS_GET_V2:
        ZW_DEBUG_CMD_ASSOCIATION_SEND_STR("ASSOCIATION_GROUPINGS_GET_V2");
        ZW_DEBUG_CMD_ASSOCIATION_SEND_NL();

        if(FALSE == Check_not_legal_response_job(rxOpt))
        {
          ZW_APPLICATION_TX_BUFFER *pTxBuf = GetResponseBuffer();

          /*Check pTxBuf is free*/
          if( NON_NULL( pTxBuf ) )
          {
            TRANSMIT_OPTIONS_TYPE_SINGLE_EX *txOptionsEx;
            RxToTxOptions(rxOpt, &txOptionsEx);
            pTxBuf->ZW_AssociationGroupingsReportV2Frame.cmdClass = COMMAND_CLASS_ASSOCIATION;
            pTxBuf->ZW_AssociationGroupingsReportV2Frame.cmd = ASSOCIATION_GROUPINGS_REPORT_V2;
            pTxBuf->ZW_AssociationGroupingsReportV2Frame.supportedGroupings = handleGetMaxAssociationGroups(rxOpt->destNode.endpoint);
            if(ZW_TX_IN_PROGRESS != Transport_SendResponseEP( (BYTE *)pTxBuf,
                                              sizeof(pTxBuf->ZW_AssociationGroupingsReportV2Frame),
                                              txOptionsEx,
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
    case ASSOCIATION_SPECIFIC_GROUP_GET_V2:
        ZW_DEBUG_CMD_ASSOCIATION_SEND_STR("ASSOCIATION_GROUPINGS_GET_V2");
        ZW_DEBUG_CMD_ASSOCIATION_SEND_NL();

        if(FALSE== Check_not_legal_response_job(rxOpt))
        {
          ZW_APPLICATION_TX_BUFFER *pTxBuf = GetResponseBuffer();

          /*Check pTxBuf is free*/
          if( NON_NULL( pTxBuf ) )
          {
            TRANSMIT_OPTIONS_TYPE_SINGLE_EX *txOptionsEx;
            RxToTxOptions(rxOpt, &txOptionsEx);
            pTxBuf->ZW_AssociationSpecificGroupReportV2Frame.cmdClass = COMMAND_CLASS_ASSOCIATION;
            pTxBuf->ZW_AssociationSpecificGroupReportV2Frame.cmd = ASSOCIATION_SPECIFIC_GROUP_REPORT_V2;
            pTxBuf->ZW_AssociationSpecificGroupReportV2Frame.group = ApplicationGetLastActiveGroupId();
            if(ZW_TX_IN_PROGRESS != Transport_SendResponseEP( (BYTE *)pTxBuf,
                                              sizeof(pTxBuf->ZW_AssociationSpecificGroupReportV2Frame),
                                              txOptionsEx,
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
  }
  return RECEIVED_FRAME_STATUS_NO_SUPPORT;
}

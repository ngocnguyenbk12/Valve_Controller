/**
 * @file
 * Handler for Command Class Association Group Info.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include <ZW_basis_api.h>
#include <ZW_TransportLayer.h>
#include "config_app.h"
#include <CommandClassAssociationGroupInfo.h>
#include <misc.h>
#include <ZW_tx_mutex.h>
#include <ZW_uart_api.h>
#include <ZW_mem_api.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

#ifdef ZW_DEBUG_CCAGI
#define ZW_DEBUG_CCAGI_SEND_BYTE(data)      ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_CCAGI_SEND_STR(STR)        ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_CCAGI_SEND_NUM(data)       ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_CCAGI_SEND_WORD_NUM(data)  ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_CCAGI_SEND_NL()            ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_CCAGI_SEND_BYTE(data)
#define ZW_DEBUG_CCAGI_SEND_STR(STR)
#define ZW_DEBUG_CCAGI_SEND_NUM(data)
#define ZW_DEBUG_CCAGI_SEND_WORD_NUM(data)
#define ZW_DEBUG_CCAGI_SEND_NL()
#endif

#define REPORT_ONE_GROUP 1
#define REPORT_ALL_GROUPS 2

typedef enum
{
  REPORT_STATUS_INACTIVE,
  REPORT_STATUS_ONE_GROUP,
  REPORT_STATUS_ALL_GROUPS
}
report_status_t;

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

static uint8_t currentGroupId;
static uint8_t associationGroupInfoGetEndpoint = 0;
static report_status_t reportStatus = REPORT_STATUS_INACTIVE;
static RECEIVE_OPTIONS_TYPE_EX rxOptions;

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

void ZCB_AGIReport(uint8_t txStatus);
void ZCB_AGIReportSendTimer(void);

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

static BOOL SendAssoGroupInfoReport(void)
{
  ZW_APPLICATION_TX_BUFFER *pTxBuf = NULL;
  TRANSMIT_OPTIONS_TYPE_SINGLE_EX *txOptionsEx;

  pTxBuf = GetResponseBufferCb(ZCB_AGIReport);

  /*Check pTxBuf is free*/
  if (IS_NULL( pTxBuf ))
  {
    return FALSE;
  }

  memset((uint8_t *)pTxBuf, 0, sizeof(ZW_ASSOCIATION_GROUP_INFO_REPORT_1BYTE_FRAME));

  RxToTxOptions(&rxOptions, &txOptionsEx);

  pTxBuf->ZW_AssociationGroupInfoReport1byteFrame.cmdClass = COMMAND_CLASS_ASSOCIATION_GRP_INFO;
  pTxBuf->ZW_AssociationGroupInfoReport1byteFrame.cmd      = ASSOCIATION_GROUP_INFO_REPORT;
  // If thelist mode bit is set in the get frame it should be also set in the report frame.
  pTxBuf->ZW_AssociationGroupInfoReport1byteFrame.properties1 = ((REPORT_STATUS_ALL_GROUPS == reportStatus) ? ASSOCIATION_GROUP_INFO_REPORT_PROPERTIES1_LIST_MODE_BIT_MASK : 0x00) & 0xFF; // we send one report per group
  pTxBuf->ZW_AssociationGroupInfoReport1byteFrame.properties1 |= 0x01; // One report pr. group.

  CC_AGI_groupInfoGet_handler(
      currentGroupId,
      rxOptions.destNode.endpoint,
      &pTxBuf->ZW_AssociationGroupInfoReport1byteFrame.variantgroup1);

  if (ZW_TX_IN_PROGRESS != Transport_SendResponseEP( (uint8_t *)pTxBuf,
              sizeof(ZW_ASSOCIATION_GROUP_INFO_REPORT_1BYTE_FRAME),
              txOptionsEx,
              ZCB_ResponseJobStatus))
  {
    /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
    FreeResponseBuffer();
    return FALSE;
  }
  return TRUE;
}


/*
Use this timer to delay the sending of the next AGI report after the mutex is released
Since we cannot get a new tx buffer in the call back because the mutex is reserved
*/
PCB(ZCB_AGIReportSendTimer)(void)
{
  SendAssoGroupInfoReport();
}


/*The AGI report call back we will send a report per association group
  if we seed to send AGI fro all the groups*/
PCB(ZCB_AGIReport)(uint8_t txStatus)
{
  UNUSED(txStatus);
  if (reportStatus == REPORT_STATUS_ALL_GROUPS)
  {
    if (currentGroupId++ < CC_AGI_groupCount_handler(associationGroupInfoGetEndpoint))
    {
      TimerStart(ZCB_AGIReportSendTimer, 1, TIMER_ONE_TIME);
      return;
    }
  }
  reportStatus = REPORT_STATUS_INACTIVE;
}

received_frame_status_t
CC_AGI_handler(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  uint8_t cmdLength)
{
  uint8_t length;
  uint8_t groupID;
  TRANSMIT_OPTIONS_TYPE_SINGLE_EX *txOptionsEx;
  uint8_t groupNameLength;
  ZW_APPLICATION_TX_BUFFER *pTxBuf = NULL;

  UNUSED(cmdLength);

  if (TRUE == Check_not_legal_response_job(rxOpt))
  {
    // Get/Report do not support bit addressing.
    return RECEIVED_FRAME_STATUS_FAIL;
  }

  switch (pCmd->ZW_Common.cmd)
    {
      case ASSOCIATION_GROUP_NAME_GET:
        pTxBuf = GetResponseBuffer();
        if (IS_NULL(pTxBuf))
        {
          // The buffer is not free :(
          return RECEIVED_FRAME_STATUS_FAIL;
        }

        if (3 != cmdLength)
        {
          FreeResponseBuffer();
          return RECEIVED_FRAME_STATUS_FAIL;
        }

        pTxBuf->ZW_AssociationGroupNameReport1byteFrame.cmdClass = COMMAND_CLASS_ASSOCIATION_GRP_INFO;
        pTxBuf->ZW_AssociationGroupNameReport1byteFrame.cmd      = ASSOCIATION_GROUP_NAME_REPORT;

        groupID = pCmd->ZW_AssociationGroupNameGetFrame.groupingIdentifier;
        pTxBuf->ZW_AssociationGroupNameReport1byteFrame.groupingIdentifier = groupID;

        groupNameLength = CC_AGI_groupNameGet_handler(
            (char *)&(pTxBuf->ZW_AssociationGroupNameReport1byteFrame.name1),
            groupID,
            rxOpt->destNode.endpoint);

        pTxBuf->ZW_AssociationGroupNameReport1byteFrame.lengthOfName = groupNameLength;

        RxToTxOptions(rxOpt, &txOptionsEx);
        if (ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
            (uint8_t *)pTxBuf,
            sizeof(ZW_ASSOCIATION_GROUP_NAME_REPORT_1BYTE_FRAME) - sizeof(uint8_t) + groupNameLength,
            txOptionsEx,
            ZCB_ResponseJobStatus))
        {
          /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
          FreeResponseBuffer();
        }
        return RECEIVED_FRAME_STATUS_SUCCESS;
        break;

    case ASSOCIATION_GROUP_INFO_GET:
      // Ignore command if we are already transmitting.
      if (REPORT_STATUS_INACTIVE != reportStatus)
      {
        return RECEIVED_FRAME_STATUS_FAIL;
      }
      if (pCmd->ZW_AssociationGroupInfoGetFrame.properties1 &
          ASSOCIATION_GROUP_INFO_GET_PROPERTIES1_LIST_MODE_BIT_MASK)
      {
        /*if list mode is one then ignore groupid and report information about all the asscoication group
         one group at a time*/
         reportStatus = REPORT_STATUS_ALL_GROUPS;
         currentGroupId = 1;
         associationGroupInfoGetEndpoint = rxOpt->destNode.endpoint;
      }
      else
      {
        /*if list mode is zero and group id is not then report the association group info for the specific group*/
        reportStatus = REPORT_STATUS_ONE_GROUP;
        currentGroupId = pCmd->ZW_AssociationGroupInfoGetFrame.groupingIdentifier;
        associationGroupInfoGetEndpoint = rxOpt->destNode.endpoint;
      }

      if (REPORT_STATUS_INACTIVE != reportStatus)
      {
        memcpy((uint8_t*)&rxOptions, (uint8_t*)rxOpt, sizeof(RECEIVE_OPTIONS_TYPE_EX));
        if (TRUE != SendAssoGroupInfoReport())
        {
          reportStatus = REPORT_STATUS_INACTIVE;
          return RECEIVED_FRAME_STATUS_FAIL;
        }
        else
        {
          return RECEIVED_FRAME_STATUS_SUCCESS;
        }
      }
      return RECEIVED_FRAME_STATUS_FAIL;
      break;
    case ASSOCIATION_GROUP_COMMAND_LIST_GET:
      pTxBuf = GetResponseBuffer();
      if (IS_NULL(pTxBuf))
      {
        // The buffer is not free :(
        return RECEIVED_FRAME_STATUS_FAIL;
      }

      groupID = pCmd->ZW_AssociationGroupCommandListGetFrame.groupingIdentifier;

      ZAF_CC_AGI_CorrectGroupIdIfInvalid(rxOpt->destNode.endpoint, &groupID);

      length = GetApplGroupCommandListSize(groupID, rxOpt->destNode.endpoint);
      if (0 == length)
      {
        FreeResponseBuffer();
        return RECEIVED_FRAME_STATUS_FAIL;
      }

      RxToTxOptions(rxOpt, &txOptionsEx);
      pTxBuf->ZW_AssociationGroupCommandListReport1byteFrame.cmdClass = COMMAND_CLASS_ASSOCIATION_GRP_INFO;
      pTxBuf->ZW_AssociationGroupCommandListReport1byteFrame.cmd      = ASSOCIATION_GROUP_COMMAND_LIST_REPORT;
      pTxBuf->ZW_AssociationGroupCommandListReport1byteFrame.groupingIdentifier = groupID;
      pTxBuf->ZW_AssociationGroupCommandListReport1byteFrame.listLength = length;
      GetApplGroupCommandList(&pTxBuf->ZW_AssociationGroupCommandListReport1byteFrame.command1, groupID, rxOpt->destNode.endpoint);

      if(ZW_TX_IN_PROGRESS != Transport_SendResponseEP( (uint8_t *)pTxBuf,
                  sizeof(ZW_ASSOCIATION_GROUP_COMMAND_LIST_REPORT_1BYTE_FRAME)
                  - sizeof(uint8_t)
                  + length,
                  txOptionsEx,
                  ZCB_ResponseJobStatus))
      {
        /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
        FreeResponseBuffer();
        return RECEIVED_FRAME_STATUS_FAIL;
      }
      break;
    default:
      return RECEIVED_FRAME_STATUS_NO_SUPPORT;
      break;
  }
  return RECEIVED_FRAME_STATUS_SUCCESS;
}

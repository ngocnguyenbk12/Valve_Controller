/**
 * @file
 * Handler for Command Class User Code.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_basis_api.h>
#include <ZW_tx_mutex.h>
#include <ZW_TransportLayer.h>

#include "config_app.h"
#include <CommandClassUserCode.h>
#include <misc.h>


/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
#ifdef ZW_DEBUG_USERCODE
#include <ZW_uart_api.h>
#define ZW_DEBUG_USERCODE_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_USERCODE_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_USERCODE_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_USERCODE_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_USERCODE_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_USERCODE_SEND_BYTE(data)
#define ZW_DEBUG_USERCODE_SEND_STR(STR)
#define ZW_DEBUG_USERCODE_SEND_NUM(data)
#define ZW_DEBUG_USERCODE_SEND_WORD_NUM(data)
#define ZW_DEBUG_USERCODE_SEND_NL()
#endif

typedef struct
{
  uint8_t userIdentifier;
  uint8_t userIdStatus;
  uint8_t userCode[10];
}
user_code_report_t;

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/



/*============================ handleCommandClassUserCode ===================
** Function description
** Handle incoming command class User Code frames version 1.
**
** Side effects: none
**
**-------------------------------------------------------------------------*/
received_frame_status_t
handleCommandClassUserCode(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt, /* IN receive options of type RECEIVE_OPTIONS_TYPE_EX  */
  ZW_APPLICATION_TX_BUFFER *pCmd, /* IN  Payload from the received frame */
  uint8_t cmdLength)               /* IN Number of command bytes including the command */
{
  ZW_APPLICATION_TX_BUFFER * pTxBuf;
  switch (pCmd->ZW_Common.cmd)
  {
    case USER_CODE_GET:
      if(TRUE == Check_not_legal_response_job(rxOpt))
      {
        /*Get/Report do not support endpoint bit-addressing */
        return RECEIVED_FRAME_STATUS_FAIL;
      }
      pTxBuf = GetResponseBuffer();
      /*Check pTxBuf is free*/
      if( NON_NULL( pTxBuf ) )
      {
        uint8_t len = 0;
        uint8_t maxNumberOfUsers = handleCommandClassUserCodeUsersNumberReport(rxOpt->destNode.endpoint);
        TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx;
        RxToTxOptions(rxOpt, &pTxOptionsEx);

        pTxBuf->ZW_UserCodeReport1byteFrame.cmdClass = COMMAND_CLASS_USER_CODE;
        pTxBuf->ZW_UserCodeReport1byteFrame.cmd = USER_CODE_REPORT;
        pTxBuf->ZW_UserCodeReport1byteFrame.userIdentifier = pCmd->ZW_UserCodeGetFrame.userIdentifier;

        if ((pCmd->ZW_UserCodeGetFrame.userIdentifier > 0) &&
            (pCmd->ZW_UserCodeGetFrame.userIdentifier <= maxNumberOfUsers))
        {
          handleCommandClassUserCodeIdGet(
              pCmd->ZW_UserCodeGetFrame.userIdentifier,
              (user_id_status_t*)&(pTxBuf->ZW_UserCodeReport1byteFrame.userIdStatus),
              rxOpt->destNode.endpoint);

          if(FALSE == handleCommandClassUserCodeReport(
              pCmd->ZW_UserCodeGetFrame.userIdentifier,
                                          &(pTxBuf->ZW_UserCodeReport1byteFrame.userCode1),
                                          &len,
                                          rxOpt->destNode.endpoint))
          {
            /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
            FreeResponseBuffer();
            return RECEIVED_FRAME_STATUS_FAIL; /*failing*/
          }
        }
        else
        {
          pTxBuf->ZW_UserCodeReport1byteFrame.userIdStatus = USER_ID_NO_STATUS;
        }
        if(ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
                    (uint8_t *)pTxBuf,
                    sizeof(ZW_USER_CODE_REPORT_1BYTE_FRAME) + len - 1,
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

    case USER_CODE_SET:
      {
        BOOL status = TRUE;
        uint8_t i;
        uint8_t user_code_length = cmdLength - 4;

        if (!((pCmd->ZW_UserCodeSet1byteFrame.userIdentifier <= handleCommandClassUserCodeUsersNumberReport(rxOpt->destNode.endpoint)) &&
            ((USERCODE_MIN_LEN <= user_code_length) && (USERCODE_MAX_LEN >= user_code_length)))
          )
        {
          return RECEIVED_FRAME_STATUS_FAIL;
        }

        switch (pCmd->ZW_UserCodeSet1byteFrame.userIdStatus)
        {
          case USER_ID_AVAILBLE:
            for (i = 0; i < 4; i++)
            {
              *(&pCmd->ZW_UserCodeSet1byteFrame.userCode1 + i) = 0x00;
            }
            user_code_length = 4;
            break;

          case USER_ID_OCCUPIED:
          case USER_ID_RESERVED:
            // Validate user code are digits
            for(i = 0; i < user_code_length; i++)
            {
              if( ((0x30 > (uint8_t)*(&pCmd->ZW_UserCodeSet1byteFrame.userCode1 + i)) || (0x39 < (uint8_t)*(&pCmd->ZW_UserCodeSet1byteFrame.userCode1 + i))))
              {
                status = FALSE;
                break;
              }
            }
            break;

          default:
            return RECEIVED_FRAME_STATUS_FAIL;
            break;
        }

        if (TRUE == status)
        {
          if (0 == pCmd->ZW_UserCodeSet1byteFrame.userIdentifier)
          {
            uint8_t max_users = handleCommandClassUserCodeUsersNumberReport(rxOpt->destNode.endpoint);
            for (i = 1; i <= max_users; i++)
            {
              if (TRUE != handleCommandClassUserCodeSet(
                  i,
                  pCmd->ZW_UserCodeSet1byteFrame.userIdStatus,
                  &(pCmd->ZW_UserCodeSet1byteFrame.userCode1),
                  user_code_length,
                  rxOpt->destNode.endpoint))
              {
                return RECEIVED_FRAME_STATUS_FAIL;
              }
            }
            return RECEIVED_FRAME_STATUS_SUCCESS;
          }
          else
          {
            if (TRUE == handleCommandClassUserCodeSet( pCmd->ZW_UserCodeSet1byteFrame.userIdentifier,
                                           pCmd->ZW_UserCodeSet1byteFrame.userIdStatus,
                                           &(pCmd->ZW_UserCodeSet1byteFrame.userCode1),
                                           user_code_length,
                                           rxOpt->destNode.endpoint))
            {
              return RECEIVED_FRAME_STATUS_SUCCESS;
            }
          }
        }
        return RECEIVED_FRAME_STATUS_FAIL;
      }
      break;

    case USERS_NUMBER_GET:
      if(FALSE == Check_not_legal_response_job(rxOpt))
      {
        ZW_APPLICATION_TX_BUFFER *pTxBuf = GetResponseBuffer();
        /*Check pTxBuf is free*/
        if( NON_NULL( pTxBuf ) )
        {
          TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx;
          RxToTxOptions(rxOpt, &pTxOptionsEx);
          pTxBuf->ZW_UsersNumberReportFrame.cmdClass = COMMAND_CLASS_USER_CODE;
          pTxBuf->ZW_UsersNumberReportFrame.cmd = USERS_NUMBER_REPORT;
          pTxBuf->ZW_UsersNumberReportFrame.supportedUsers =
            handleCommandClassUserCodeUsersNumberReport( rxOpt->destNode.endpoint );

          if(ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
                        (uint8_t *)pTxBuf,
                        sizeof(ZW_USERS_NUMBER_REPORT_FRAME),
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
  }
  return RECEIVED_FRAME_STATUS_NO_SUPPORT;
}

JOB_STATUS CmdClassUserCodeSupportReport(
  AGI_PROFILE* pProfile,
  uint8_t sourceEndpoint,
  uint8_t userIdentifier,
  uint8_t userIdStatus,
  uint8_t* pUserCode,
  uint8_t userCodeLen,
  VOID_CALLBACKFUNC(pCallback)(TRANSMISSION_RESULT * pTransmissionResult))
{
  CMD_CLASS_GRP cmdGrp = {COMMAND_CLASS_USER_CODE, USER_CODE_REPORT};
  user_code_report_t user_code_report;

  if ((0 == userIdentifier) || IS_NULL(pUserCode) ||
      (userCodeLen > USERCODE_MAX_LEN) || (userCodeLen < USERCODE_MIN_LEN) ||
      (2 < userIdStatus))
  {
    return JOB_STATUS_BUSY;
  }

  user_code_report.userIdentifier = userIdentifier;
  user_code_report.userIdStatus = userIdStatus;
  memcpy(user_code_report.userCode, pUserCode, userCodeLen);

  return cc_engine_multicast_request(
      pProfile,
      sourceEndpoint,
      &cmdGrp,
      (uint8_t*)&user_code_report,
      sizeof(user_code_report_t) + userCodeLen - USERCODE_MAX_LEN,
      FALSE,
      pCallback);
}

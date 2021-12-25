/***************************************************************************
*
* Copyright (c) 2001-2011
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: ZWave+ Info Command Class source file
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
#include <ZW_typedefs.h>
#include "config_app.h"
#include <CommandClassZWavePlusInfo.h>
#include <misc.h>
#include <ZW_uart_api.h>
#include <ZW_plus_version.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

#ifdef ZW_DEBUG_ZWAVE_INFO
#define ZW_DEBUG_ZWAVE_INFO_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_ZWAVE_INFO_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_ZWAVE_INFO_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_ZWAVE_INFO_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_ZWAVE_INFO_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_ZWAVE_INFO_SEND_BYTE(data)
#define ZW_DEBUG_ZWAVE_INFO_SEND_STR(STR)
#define ZW_DEBUG_ZWAVE_INFO_SEND_NUM(data)
#define ZW_DEBUG_ZWAVE_INFO_SEND_WORD_NUM(data)
#define ZW_DEBUG_ZWAVE_INFO_SEND_NL()
#endif

typedef struct _CICON_
{
  ST_ENDPOINT_ICONS* pEpIcon;
  BYTE nbrOfElements;
} CICON;

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

CICON myIcon = {NULL, 0};

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

// Nothing here.

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

void
CommandClassZWavePlusInfoInit(ST_ENDPOINT_ICONS* pEpIcon, BYTE nbrOfElements)
{
  myIcon.pEpIcon = pEpIcon;
  myIcon.nbrOfElements = nbrOfElements;
}

received_frame_status_t
handleCommandClassZWavePlusInfo(
    RECEIVE_OPTIONS_TYPE_EX *rxOpt,
    ZW_APPLICATION_TX_BUFFER *pCmd,
    BYTE cmdLength
)
{
  UNUSED(cmdLength);
  if (pCmd->ZW_Common.cmd == ZWAVEPLUS_INFO_GET)
  {
    /*Check pTxBuf is free*/
    if(FALSE == Check_not_legal_response_job(rxOpt))
    {
      ZW_APPLICATION_TX_BUFFER * pTxBuf = GetResponseBuffer();

      if( NON_NULL( pTxBuf ) )
      {
        TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx;
        RxToTxOptions(rxOpt, &pTxOptionsEx);
        pTxBuf->ZW_ZwaveplusInfoReportV2Frame.cmdClass = COMMAND_CLASS_ZWAVEPLUS_INFO;
        pTxBuf->ZW_ZwaveplusInfoReportV2Frame.cmd = ZWAVEPLUS_INFO_REPORT;
        pTxBuf->ZW_ZwaveplusInfoReportV2Frame.zWaveVersion = ZW_PLUS_VERSION;
        pTxBuf->ZW_ZwaveplusInfoReportV2Frame.roleType = APP_ROLE_TYPE;
        pTxBuf->ZW_ZwaveplusInfoReportV2Frame.nodeType = APP_NODE_TYPE;


        if((myIcon.nbrOfElements >= rxOpt->destNode.endpoint) && (0 != rxOpt->destNode.endpoint) && (NON_NULL(myIcon.pEpIcon)) )
        {
          pTxBuf->ZW_ZwaveplusInfoReportV2Frame.installerIconType1 = (myIcon.pEpIcon[rxOpt->destNode.endpoint-1].installerIconType >> 8);
          pTxBuf->ZW_ZwaveplusInfoReportV2Frame.installerIconType2 = (myIcon.pEpIcon[rxOpt->destNode.endpoint-1].installerIconType & 0xff);
          pTxBuf->ZW_ZwaveplusInfoReportV2Frame.userIconType1 = (myIcon.pEpIcon[rxOpt->destNode.endpoint-1].userIconType >> 8);
          pTxBuf->ZW_ZwaveplusInfoReportV2Frame.userIconType2 = (myIcon.pEpIcon[rxOpt->destNode.endpoint-1].userIconType & 0xff);
        }
        else
        {
          pTxBuf->ZW_ZwaveplusInfoReportV2Frame.installerIconType1 = (APP_ICON_TYPE >> 8);
          pTxBuf->ZW_ZwaveplusInfoReportV2Frame.installerIconType2 = (APP_ICON_TYPE & 0xff);
          pTxBuf->ZW_ZwaveplusInfoReportV2Frame.userIconType1 = (APP_USER_ICON_TYPE >> 8);
          pTxBuf->ZW_ZwaveplusInfoReportV2Frame.userIconType2 = (APP_USER_ICON_TYPE & 0xff);
        }

        {
          uint8_t txResult;

          txResult = Transport_SendResponseEP(
                        (BYTE *)pTxBuf,
                        sizeof(pTxBuf->ZW_ZwaveplusInfoReportV2Frame),
                        pTxOptionsEx,
                        ZCB_ResponseJobStatus);

          if (ZW_TX_IN_PROGRESS != txResult)
          {
            FreeResponseBuffer();
          }
        }
        return RECEIVED_FRAME_STATUS_SUCCESS;
      }  
    }
    return RECEIVED_FRAME_STATUS_FAIL;
  }
  return RECEIVED_FRAME_STATUS_NO_SUPPORT;
}

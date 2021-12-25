/**
 * @file
 * Handler for Command Class Multilevel Switch.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_basis_api.h>
#include <ZW_tx_mutex.h>
#include <ZW_TransportLayer.h>
#include <ZW_uart_api.h>
#include "config_app.h"
#include <CommandClassMultilevelSwitch.h>
#include <multilevel_switch.h>
#include <misc.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
#ifdef ZW_DEBUG_MULTISWITCH
#define ZW_DEBUG_MULTISWITCH_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_MULTISWITCH_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_MULTISWITCH_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_MULTISWITCH_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_MULTISWITCH_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_MULTISWITCH_SEND_BYTE(data)
#define ZW_DEBUG_MULTISWITCH_SEND_STR(STR)
#define ZW_DEBUG_MULTISWITCH_SEND_NUM(data)
#define ZW_DEBUG_MULTISWITCH_SEND_WORD_NUM(data)
#define ZW_DEBUG_MULTISWITCH_SEND_NL()
#endif

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

received_frame_status_t handleCommandClassMultiLevelSwitch(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  BYTE cmdLength)
{
  switch (pCmd->ZW_Common.cmd)
  {
    case SWITCH_MULTILEVEL_GET:
      if(FALSE == Check_not_legal_response_job(rxOpt))
      {
        ZW_APPLICATION_TX_BUFFER *pTxBuf =  GetResponseBuffer();
        /*Check pTxBuf is free*/
        if( NON_NULL( pTxBuf ) )
        {
          TRANSMIT_OPTIONS_TYPE_SINGLE_EX* pTxOptionsEx;
          RxToTxOptions(rxOpt, &pTxOptionsEx);
          pTxBuf->ZW_SwitchMultilevelReportV4Frame.cmdClass     = COMMAND_CLASS_SWITCH_MULTILEVEL;
          pTxBuf->ZW_SwitchMultilevelReportV4Frame.cmd          = SWITCH_MULTILEVEL_REPORT;
          pTxBuf->ZW_SwitchMultilevelReportV4Frame.currentValue = CC_MultilevelSwitch_GetCurrentValue_handler(rxOpt->destNode.endpoint);
          pTxBuf->ZW_SwitchMultilevelReportV4Frame.targetValue  = GetTargetLevel(rxOpt->destNode.endpoint);
          pTxBuf->ZW_SwitchMultilevelReportV4Frame.duration     = GetCurrentDuration(rxOpt->destNode.endpoint);

          if (ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
              (BYTE *)pTxBuf,
              sizeof(ZW_SWITCH_MULTILEVEL_REPORT_V4_FRAME),
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

    case SWITCH_MULTILEVEL_SET:
      if ((100 <= pCmd->ZW_SwitchMultilevelSetV3Frame.value) &&
          (255 != pCmd->ZW_SwitchMultilevelSetV3Frame.value))
      {
        return RECEIVED_FRAME_STATUS_FAIL;
      }
      if ((3 == cmdLength) || (0xFF == pCmd->ZW_SwitchMultilevelSetV3Frame.dimmingDuration))
      {
        /*
         * Get the application's default dimming duration in two cases:
         * - If the length is 3, the CC version is 1. Hence, duration is not supported.
         * - If the desired dimming duration is 0xFF (see Table 7, SDS13781-4).
         */
        pCmd->ZW_SwitchMultilevelSetV3Frame.dimmingDuration = GetFactoryDefaultDimmingDuration( TRUE, rxOpt->destNode.endpoint );
      }

      if (TRUE != CC_MLS_Set_handler(
          pCmd->ZW_SwitchMultilevelSetV3Frame.value,
          pCmd->ZW_SwitchMultilevelSetV3Frame.dimmingDuration,
          rxOpt->destNode.endpoint))
      {
        return RECEIVED_FRAME_STATUS_FAIL;
      }
      break;

    case SWITCH_MULTILEVEL_START_LEVEL_CHANGE:
      {
        BOOL boDimUp = FALSE;
        BOOL boIgnoreLevel;
        boIgnoreLevel = (pCmd->ZW_SwitchMultilevelStartLevelChangeFrame.level & SWITCH_MULTILEVEL_START_LEVEL_CHANGE_LEVEL_IGNORE_START_LEVEL_BIT_MASK);
        if ((!boIgnoreLevel) &&
            (pCmd->ZW_SwitchMultilevelStartLevelChangeFrame.startLevel > 99) &&
            (pCmd->ZW_SwitchMultilevelStartLevelChangeFrame.startLevel != 0xFF))
        {
          return RECEIVED_FRAME_STATUS_SUCCESS;
        }
        StopSwitchDimming(rxOpt->destNode.endpoint);
        if (cmdLength == 6)
        {
          if (!(pCmd->ZW_SwitchMultilevelStartLevelChangeV3Frame.properties1&
              SWITCH_MULTILEVEL_START_LEVEL_CHANGE_PROPERTIES1_UP_DOWN_MASK_V3))
          {
            /*primary switch Up/Down bit field value are Up*/
            boDimUp = TRUE;
          }
          else if  ((pCmd->ZW_SwitchMultilevelStartLevelChangeV3Frame.properties1 &
              SWITCH_MULTILEVEL_START_LEVEL_CHANGE_PROPERTIES1_UP_DOWN_MASK_V3) > 0x40)
          {
            /*We should ignore the frame if the  up/down primary switch bit field value is either reserved or no up/down motion*/
            return RECEIVED_FRAME_STATUS_SUCCESS;
          }
        }
        else
        {
          if (cmdLength == 4) /*version 1*/
          {
             /*dimmingDuration is 1 sec when handling start level change version 1*/
            pCmd->ZW_SwitchMultilevelStartLevelChangeV3Frame.dimmingDuration = GetFactoryDefaultDimmingDuration( FALSE, rxOpt->destNode.endpoint );
          }
          if (!(pCmd->ZW_SwitchMultilevelStartLevelChangeFrame.level & SWITCH_MULTILEVEL_START_LEVEL_CHANGE_LEVEL_UP_DOWN_BIT_MASK))
            boDimUp = TRUE;
        }
        if (pCmd->ZW_SwitchMultilevelStartLevelChangeV3Frame.dimmingDuration == 0xFF)
        {
          /*use the factory default dimming duration if the value is 0xFF*/
          pCmd->ZW_SwitchMultilevelStartLevelChangeV3Frame.dimmingDuration =  GetFactoryDefaultDimmingDuration( FALSE, rxOpt->destNode.endpoint );
        }

        HandleStartChangeCmd(pCmd->ZW_SwitchMultilevelStartLevelChangeFrame.startLevel,
                             boIgnoreLevel,
                             boDimUp,
                             pCmd->ZW_SwitchMultilevelStartLevelChangeV3Frame.dimmingDuration,
                             rxOpt->destNode.endpoint );
      }
      return RECEIVED_FRAME_STATUS_SUCCESS;
      break;

    case SWITCH_MULTILEVEL_STOP_LEVEL_CHANGE:
      StopSwitchDimming( rxOpt->destNode.endpoint );
      return RECEIVED_FRAME_STATUS_SUCCESS;
      break;
    case SWITCH_MULTILEVEL_SUPPORTED_GET_V3:
      if(FALSE == Check_not_legal_response_job(rxOpt))
      {
        ZW_APPLICATION_TX_BUFFER *pTxBuf =  GetResponseBuffer();
        /*Check pTxBuf is free*/
        if( NON_NULL( pTxBuf ) )
        {
          TRANSMIT_OPTIONS_TYPE_SINGLE_EX* pTxOptionsEx;
          RxToTxOptions(rxOpt, &pTxOptionsEx);

          pTxBuf->ZW_SwitchMultilevelSupportedReportV3Frame.cmdClass = COMMAND_CLASS_SWITCH_MULTILEVEL;
          pTxBuf->ZW_SwitchMultilevelSupportedReportV3Frame.cmd = SWITCH_MULTILEVEL_SUPPORTED_REPORT_V3;
          pTxBuf->ZW_SwitchMultilevelSupportedReportV3Frame.properties1 = CommandClassMultiLevelSwitchPrimaryTypeGet( rxOpt->destNode.endpoint );
          pTxBuf->ZW_SwitchMultilevelSupportedReportV3Frame.properties2 = 0x00; /*The secondary switch type is deprectaed, thus the value should be not support (0x02)*/

          if(ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
              (BYTE *)pTxBuf,
              sizeof(ZW_SWITCH_MULTILEVEL_SUPPORTED_REPORT_V3_FRAME),
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

    default:
      return RECEIVED_FRAME_STATUS_NO_SUPPORT;
      break;
  }
  return RECEIVED_FRAME_STATUS_SUCCESS;
}


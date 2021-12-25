/***************************************************************************
*
* Copyright (c) 2001-2011
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: Multilevel switch Command Class source file
*
* Author: Samer Seoud
* Author: Thomas Roll
* Author: Christian Salmony Olsen
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


typedef struct _multi_level_switch_set_t_
{
  uint8_t value;
  uint8_t dimmingDuration;
}multi_level_switch_set_t;


/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/
CMD_CLASS_GRP cmdGrp;

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

JOB_STATUS
CmdClassMultiLevelSwitchStartLevelChange(
  AGI_PROFILE* pProfile,
  BYTE sourceEndpoint,
  VOID_CALLBACKFUNC(pCbFunc)(TRANSMISSION_RESULT * pTransmissionResult),
  CCMLS_PRIMARY_SWITCH_T primarySwitch,
  CCMLS_IGNORE_START_LEVEL_T fIgnoreStartLevel,
  CCMLS_SECONDARY_SWITCH_T secondarySwitch,
  BYTE primarySwitchStartLevel,
  BYTE duration,
  BYTE secondarySwitchStepSize)
{
  ZW_APPLICATION_TX_BUFFER *pTxBuf = GetRequestBuffer(pCbFunc);
 if( IS_NULL( pTxBuf ) )
  {
    /*Ongoing job is active.. just stop current job*/
    return JOB_STATUS_BUSY;
  }
  else
  {
    TRANSMIT_OPTIONS_TYPE_EX* pTxOptionsEx = NULL;

    // ZW_classcmd.h:19423 -> ZW_SWITCH_MULTILEVEL_START_LEVEL_CHANGE_V4_FRAME
    pTxBuf->ZW_SwitchMultilevelStartLevelChangeV4Frame.cmdClass = COMMAND_CLASS_SWITCH_MULTILEVEL_V4;
    pTxBuf->ZW_SwitchMultilevelStartLevelChangeV4Frame.cmd = SWITCH_MULTILEVEL_START_LEVEL_CHANGE_V4;
    pTxBuf->ZW_SwitchMultilevelStartLevelChangeV4Frame.properties1 = 0;
    pTxBuf->ZW_SwitchMultilevelStartLevelChangeV4Frame.properties1 = (primarySwitch << 6) | (fIgnoreStartLevel << 5) | (secondarySwitch << 3);
    pTxBuf->ZW_SwitchMultilevelStartLevelChangeV4Frame.startLevel = primarySwitchStartLevel;
    pTxBuf->ZW_SwitchMultilevelStartLevelChangeV4Frame.dimmingDuration = duration;
    pTxBuf->ZW_SwitchMultilevelStartLevelChangeV4Frame.stepSize = secondarySwitchStepSize;

    /*Get transmit options (node list)*/
    pTxOptionsEx = ReqNodeList( pProfile,
                          (CMD_CLASS_GRP*) &(pTxBuf->ZW_Common.cmdClass),
                          sourceEndpoint);
    if( IS_NULL( pTxOptionsEx ) )
    {
      /*Job failed, free transmit-buffer pTxBuf by cleaning mutex */
      FreeRequestBuffer();
      return JOB_STATUS_NO_DESTINATIONS;
    }

    if(ZW_TX_IN_PROGRESS != ZW_TransportMulticast_SendRequest(
      (BYTE *)pTxBuf,
      sizeof(ZW_SWITCH_MULTILEVEL_START_LEVEL_CHANGE_V4_FRAME),
      TRUE, // Enable Supervision
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


JOB_STATUS
CmdClassMultiLevelSwitchStopLevelChange(
  AGI_PROFILE* pProfile,
  BYTE sourceEndpoint,
  VOID_CALLBACKFUNC(pCbFunc)(TRANSMISSION_RESULT * pTransmissionResult))
{
  cmdGrp.cmdClass = COMMAND_CLASS_SWITCH_MULTILEVEL_V4;
  cmdGrp.cmd = SWITCH_MULTILEVEL_STOP_LEVEL_CHANGE_V4;

  return cc_engine_multicast_request(
      pProfile,
      sourceEndpoint,
      &cmdGrp,
      NULL,
      0,
      TRUE,
      pCbFunc);
}


JOB_STATUS
CmdClassMultiLevelSwitchSetTransmit(
  AGI_PROFILE* pProfile,
  BYTE sourceEndpoint,
  VOID_CALLBACKFUNC(pCbFunc)(TRANSMISSION_RESULT * pTransmissionResult),
  BYTE value,
  BYTE duration)
{
  multi_level_switch_set_t multi_level_switch_set;
  multi_level_switch_set.value = value;
  multi_level_switch_set.dimmingDuration = duration;

  cmdGrp.cmdClass = COMMAND_CLASS_SWITCH_MULTILEVEL_V4;
  cmdGrp.cmd = SWITCH_MULTILEVEL_SET_V4;


  return cc_engine_multicast_request(
      pProfile,
      sourceEndpoint,
      &cmdGrp,
      (uint8_t*)&multi_level_switch_set,
      sizeof(multi_level_switch_set_t),
      TRUE,
      pCbFunc);

}


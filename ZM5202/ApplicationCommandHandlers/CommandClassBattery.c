/***************************************************************************
*
* Copyright (c) 2001-2011
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: Battery Command Class source file
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

#include "config_app.h"
#include <CommandClassBattery.h>
#include <ZW_adcdriv_api.h>
#include <ZW_timer_api.h>
#include "misc.h"
#include <ZW_TransportMulticast.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

#define UNS_BATT_TIMEOUT   30

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/


/*==============================   handleCommandClassBattery  ============
**
**  Function:  handler for Battery CC
**
**  Side effects: None
**
**--------------------------------------------------------------------------*/
received_frame_status_t
handleCommandClassBattery(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt, /* IN receive options of type RECEIVE_OPTIONS_TYPE_EX  */
  ZW_APPLICATION_TX_BUFFER *pCmd, /* IN  Payload from the received frame */
  BYTE cmdLength)               /* IN Number of command bytes including the command */
{
  UNUSED(cmdLength);
  if (pCmd->ZW_Common.cmd == BATTERY_GET)
  {
    ZW_APPLICATION_TX_BUFFER *pTxBuf = NULL;

    if (TRUE == Check_not_legal_response_job(rxOpt))
    {
      // None of the following commands support endpoint bit addressing.
      return RECEIVED_FRAME_STATUS_FAIL;
    }
    pTxBuf = GetResponseBuffer();
    /*Check pTxBuf is free*/
    if( NON_NULL( pTxBuf ) )
    {
      TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx;
      RxToTxOptions(rxOpt, &pTxOptionsEx);
      pTxBuf->ZW_BatteryReportFrame.cmdClass = COMMAND_CLASS_BATTERY;
      pTxBuf->ZW_BatteryReportFrame.cmd = BATTERY_REPORT;
      BatterySensorRead((BATT_LEVEL *)&(pTxBuf->ZW_BatteryReportFrame.batteryLevel));
      if(ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
          (BYTE *)pTxBuf,
          sizeof(ZW_BATTERY_REPORT_FRAME),
          pTxOptionsEx,
          ZCB_ResponseJobStatus))
      {
        /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
        FreeResponseBuffer();
      }
      return RECEIVED_FRAME_STATUS_SUCCESS;
    }
    return RECEIVED_FRAME_STATUS_FAIL;
  }
  return RECEIVED_FRAME_STATUS_NO_SUPPORT;
}

/*================= CmdClassBatteryReport =======================
** Function description
** This function...
**
** Side effects:
**
**-------------------------------------------------------------------------*/
JOB_STATUS
CmdClassBatteryReport(
  AGI_PROFILE* pProfile,
  BYTE sourceEndpoint,
  BYTE bBattLevel,
  VOID_CALLBACKFUNC(pCbFunc)(TRANSMISSION_RESULT * pTransmissionResult))
{
  CMD_CLASS_GRP cmdGrp = {COMMAND_CLASS_BATTERY, BATTERY_REPORT};

  return cc_engine_multicast_request(
      pProfile,
      sourceEndpoint,
      &cmdGrp,
      &bBattLevel,
      1,
      FALSE,
      pCbFunc);
}

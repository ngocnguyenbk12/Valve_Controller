/***************************************************************************
*
* Copyright (c) 2001-2013
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: The Door Lock Command Class used to secure/unsecure a lock
*              type as well as setting the configuration of an advanced
*              Z-Wave™ door lock device. Version 2 enable Door Lock
*              Operation Report Command to report a Door Lock Mode that the
*              door/lock state is unknown, i.e. bolt is not fully
*              retracted/engaged.
*
*        Door Lock Command Class, version 1-2.
*        Z-Wave command Class Specification SDS11060.doc
*
* Author: Thomas Roll
*
* Last Changed By: $Author: tro $
* Revision: $Revision: 0.00 $
* Last Changed: $Date: 2013/05/02 09:38:04 $
*
****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_basis_api.h>
#include <ZW_TransportLayer.h>
#include "config_app.h"
#include <CommandClassDoorLock.h>
#include <misc.h>


/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

received_frame_status_t handleCommandClassDoorLock(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  BYTE cmdLength)
{
  UNUSED(cmdLength);
  switch (pCmd->ZW_Common.cmd)
  {

    case DOOR_LOCK_OPERATION_SET_V2:
      if (TRUE != handleCommandClassDoorLockOperationSet(pCmd->ZW_DoorLockOperationSetV2Frame.doorLockMode))
      {
        return RECEIVED_FRAME_STATUS_FAIL;
      }
      break;

    case DOOR_LOCK_OPERATION_GET_V2:
      if(FALSE == Check_not_legal_response_job(rxOpt))
      {
        ZW_APPLICATION_TX_BUFFER *pTxBuf = GetResponseBuffer();
        /*Check pTxBuf is free*/
        if( NON_NULL( pTxBuf ) )
        {
          TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx;
          RxToTxOptions(rxOpt, &pTxOptionsEx);
          pTxBuf->ZW_DoorLockOperationReportV2Frame.cmdClass = COMMAND_CLASS_DOOR_LOCK_V2;
          pTxBuf->ZW_DoorLockOperationReportV2Frame.cmd = DOOR_LOCK_OPERATION_REPORT_V2;
          handleCommandClassDoorLockOperationReport((CMD_CLASS_DOOR_LOCK_OPERATION_REPORT *)&(pTxBuf->ZW_DoorLockOperationReportV2Frame.doorLockMode));
          if(ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
              (BYTE *)pTxBuf,
              sizeof(ZW_DOOR_LOCK_OPERATION_REPORT_V2_FRAME),
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

    case DOOR_LOCK_CONFIGURATION_SET_V2:
      if (TRUE != handleCommandClassDoorLockConfigurationSet(
          (cc_door_lock_configuration_t *)&(pCmd->ZW_DoorLockConfigurationSetV2Frame.operationType)))
      {
        return RECEIVED_FRAME_STATUS_FAIL;
      }
      break;

    case DOOR_LOCK_CONFIGURATION_GET_V2:
      if(FALSE == Check_not_legal_response_job(rxOpt))
      {
        ZW_APPLICATION_TX_BUFFER *pTxBuf = GetResponseBuffer();
        /*Check pTxBuf is free*/
        if( NON_NULL( pTxBuf ) )
        {
          TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx;
          RxToTxOptions(rxOpt, &pTxOptionsEx);
          pTxBuf->ZW_DoorLockConfigurationReportV2Frame.cmdClass = COMMAND_CLASS_DOOR_LOCK_V2;
          pTxBuf->ZW_DoorLockConfigurationReportV2Frame.cmd = DOOR_LOCK_CONFIGURATION_REPORT_V2;
          handleCommandClassDoorLockConfigurationReport((CMD_CLASS_DOOR_LOCK_CONFIGURATION *)&(pTxBuf->ZW_DoorLockConfigurationReportV2Frame.operationType));
          if(ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
              (BYTE *)pTxBuf,
              sizeof(ZW_DOOR_LOCK_CONFIGURATION_REPORT_V2_FRAME),
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

JOB_STATUS CmdClassDoorLockOperationSupportReport(
  agi_profile_t * pProfile,
  BYTE sourceEndpoint,
  CMD_CLASS_DOOR_LOCK_OPERATION_REPORT* pData,
  VOID_CALLBACKFUNC(pCallback)(TRANSMISSION_RESULT * pTransmissionResult))
{
  CMD_CLASS_GRP cmdGrp = {COMMAND_CLASS_DOOR_LOCK_V2, DOOR_LOCK_OPERATION_REPORT_V2};

  return cc_engine_multicast_request(
      pProfile,
      sourceEndpoint,
      &cmdGrp,
      (uint8_t*)pData,
      sizeof(CMD_CLASS_DOOR_LOCK_OPERATION_REPORT),
      TRUE,
      pCallback);
}

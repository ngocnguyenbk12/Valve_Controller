/***************************************************************************
*
* Copyright (c) 2001-2013
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: Some nice descriptive description.
*
* Author: Thomas Roll
*
* Last Changed By: $Author: tro $
* Revision: $Revision: 0.00 $
* Last Changed: $Date: 2013/07/04 13:51:13 $
*
****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_basis_api.h>
#include <ZW_TransportLayer.h>
#include <misc.h>
#include "config_app.h"
#include <ota_util.h>
#include <ZW_tx_mutex.h>
#include <ZW_uart_api.h>
#include <ZW_crc.h>
#include <ZW_firmware_descriptor.h>
#include <ZW_firmware_bootloader_defs.h>
#include <ZW_firmware_update_nvm_api.h>
#include <CommandClassFirmwareUpdate.h>
#include <CommandClassVersion.h>
#include <ZW_typedefs.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

#ifdef ZW_DEBUG_FIRMWARE_UPDATE
#ifdef ZW_DEBUG_UART0
#define ZW_DEBUG_FIRMWARE_UPDATE_INIT(baud)       ZW_UART0_init(baud, TRUE, FALSE)
#define ZW_DEBUG_FIRMWARE_UPDATE_SEND_BYTE(bData) ZW_UART0_tx_send_byte(bData)
#define ZW_DEBUG_FIRMWARE_UPDATE_SEND_NUM(bData)  ZW_UART0_tx_send_num(bData)
#define ZW_DEBUG_FIRMWARE_UPDATE_SEND_WORD_NUM(bData)  ZW_UART0_tx_send_w_num(bData)
#define ZW_DEBUG_FIRMWARE_UPDATE_SEND_NL()        ZW_UART0_tx_send_nl()
#define ZW_DEBUG_FIRMWARE_UPDATE_SEND_STR(STR)    ZW_UART0_tx_send_str(STR)
#define ZW_DEBUG_FIRMWARE_UPDATE_TX_STATUS()      ZW_UART0_tx_active_get()
#else
#ifdef ZW_DEBUG_UART1
#define ZW_DEBUG_FIRMWARE_UPDATE_INIT(baud)       ZW_UART1_init(baud, TRUE, FALSE)
#define ZW_DEBUG_FIRMWARE_UPDATE_SEND_BYTE(bData) ZW_UART1_tx_send_byte(bData)
#define ZW_DEBUG_FIRMWARE_UPDATE_SEND_NUM(bData)  ZW_UART1_tx_send_num(bData)
#define ZW_DEBUG_FIRMWARE_UPDATE_SEND_WORD_NUM(bData)  ZW_UART1_tx_send_w_num(bData)
#define ZW_DEBUG_FIRMWARE_UPDATE_SEND_NL()        ZW_UART1_tx_send_nl()
#define ZW_DEBUG_FIRMWARE_UPDATE_SEND_STR(STR)    ZW_UART1_tx_send_str(STR)
#define ZW_DEBUG_FIRMWARE_UPDATE_TX_STATUS()      ZW_UART1_tx_active_get()
#else
#define ZW_DEBUG_FIRMWARE_UPDATE_INIT(baud)       ZW_UART_init(baud, TRUE, FALSE)
#define ZW_DEBUG_FIRMWARE_UPDATE_SEND_BYTE(bData) ZW_UART_tx_send_byte(bData)
#define ZW_DEBUG_FIRMWARE_UPDATE_SEND_NUM(bData)  ZW_UART_tx_send_num(bData)
#define ZW_DEBUG_FIRMWARE_UPDATE_SEND_WORD_NUM(bData)  ZW_UART_tx_send_w_num(bData)
#define ZW_DEBUG_FIRMWARE_UPDATE_SEND_NL()        ZW_UART_tx_send_nl()
#define ZW_DEBUG_FIRMWARE_UPDATE_SEND_STR(STR)    ZW_UART_tx_send_str(STR)
#define ZW_DEBUG_FIRMWARE_UPDATE_TX_STATUS()      ZW_UART_tx_active_get()
#endif
#endif
#else
#define ZW_DEBUG_FIRMWARE_UPDATE_INIT(baud)
#define ZW_DEBUG_FIRMWARE_UPDATE_SEND_BYTE(bData)
#define ZW_DEBUG_FIRMWARE_UPDATE_SEND_NUM(bData)
#define ZW_DEBUG_FIRMWARE_UPDATE_SEND_WORD_NUM(bData)
#define ZW_DEBUG_FIRMWARE_UPDATE_SEND_NL()
#define ZW_DEBUG_FIRMWARE_UPDATE_SEND_STR(STR)
#define ZW_DEBUG_FIRMWARE_UPDATE_TX_STATUS()
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

received_frame_status_t
handleCommandClassFWUpdate(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  BYTE cmdLength)
{
  if (TRUE == Check_not_legal_response_job(rxOpt))
  {
    // None of the following commands support endpoint bit addressing.
    return RECEIVED_FRAME_STATUS_FAIL;
  }

  switch (pCmd->ZW_Common.cmd)
  {
    case FIRMWARE_MD_GET_V4:
    {
      ZW_APPLICATION_TX_BUFFER* pTxBuf = GetResponseBuffer();
      if( NON_NULL( pTxBuf ) )
      {
        uint8_t i;
        uint8_t * pData;
        TRANSMIT_OPTIONS_TYPE_SINGLE_EX * pTxOptionsEx;
        uint16_t fragment_size = handleCommandClassFirmwareUpdateMaxFragmentSize();

        RxToTxOptions(rxOpt, &pTxOptionsEx);

        pTxBuf->ZW_FirmwareMdReport1byteV4Frame.cmdClass = COMMAND_CLASS_FIRMWARE_UPDATE_MD_V4;
        pTxBuf->ZW_FirmwareMdReport1byteV4Frame.cmd = FIRMWARE_MD_REPORT_V4;
        pTxBuf->ZW_FirmwareMdReport1byteV4Frame.manufacturerId1 = firmwareDescriptor.manufacturerID >> 8;
        pTxBuf->ZW_FirmwareMdReport1byteV4Frame.manufacturerId2 = firmwareDescriptor.manufacturerID & 0xFF;
        pTxBuf->ZW_FirmwareMdReport1byteV4Frame.firmware0Id1 = (BYTE)(handleFirmWareIdGet(0) >> 8);
        pTxBuf->ZW_FirmwareMdReport1byteV4Frame.firmware0Id2 = (BYTE)(handleFirmWareIdGet(0) & 0xff);
        pTxBuf->ZW_FirmwareMdReport1byteV4Frame.firmware0Checksum1 = firmwareDescriptor.checksum >> 8;
        pTxBuf->ZW_FirmwareMdReport1byteV4Frame.firmware0Checksum2 = firmwareDescriptor.checksum & 0xFF;
        pTxBuf->ZW_FirmwareMdReport1byteV4Frame.firmwareUpgradable = FIRMWARE_UPGRADABLE;
        pTxBuf->ZW_FirmwareMdReport1byteV4Frame.numberOfFirmwareTargets = handleNbrFirmwareVersions() - 1;
        pTxBuf->ZW_FirmwareMdReport1byteV4Frame.maxFragmentSize1 = (fragment_size >> 8);
        pTxBuf->ZW_FirmwareMdReport1byteV4Frame.maxFragmentSize2 = (fragment_size & 0xFF);

        pData = (uint8_t *)pTxBuf;

        for (i = 1; i < handleNbrFirmwareVersions(); i++)
        {
          *(pData + 10 + (2 * i)) = (uint8_t)(handleFirmWareIdGet(i) >> 8);
          *(pData + 10 + (2 * i) + 1) = (uint8_t)(handleFirmWareIdGet(i) & 0xff);
        }

        if ( ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
               (BYTE *)pTxBuf,
               10 + (2 * i),
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
    case FIRMWARE_UPDATE_MD_REPORT_V4:
      {
        WORD crc16Result = ZW_CheckCrc16(0x1D0F, &(pCmd->ZW_Common.cmdClass), cmdLength);
        WORD  firmwareUpdateReportNumber = ((WORD)(pCmd->ZW_FirmwareUpdateMdReport1byteV4Frame.properties1 &
                                      FIRMWARE_UPDATE_MD_REPORT_PROPERTIES1_REPORT_NUMBER_1_MASK_V4) << 8) +
                                     (WORD)(pCmd->ZW_FirmwareUpdateMdReport1byteV4Frame.reportNumber2);
        BYTE fw_actualFrameSize =  cmdLength -
                                  /* Calculate length of actual data1 field */
                                  (sizeof(pCmd->ZW_FirmwareUpdateMdReport1byteV4Frame.cmdClass) +
                                   sizeof(pCmd->ZW_FirmwareUpdateMdReport1byteV4Frame.cmd) +
                                   sizeof(pCmd->ZW_FirmwareUpdateMdReport1byteV4Frame.properties1) +
                                   sizeof(pCmd->ZW_FirmwareUpdateMdReport1byteV4Frame.reportNumber2) +
                                   sizeof(pCmd->ZW_FirmwareUpdateMdReport1byteV4Frame.checksum1) +
                                   sizeof(pCmd->ZW_FirmwareUpdateMdReport1byteV4Frame.checksum2));


        handleCmdClassFirmwareUpdateMdReport(crc16Result,
                                             firmwareUpdateReportNumber,
                                             pCmd->ZW_FirmwareUpdateMdReport1byteV4Frame.properties1,
                                             &(pCmd->ZW_FirmwareUpdateMdReport1byteV4Frame.data1),
                                             fw_actualFrameSize);


      }
      return RECEIVED_FRAME_STATUS_SUCCESS;
      break;
    case FIRMWARE_UPDATE_MD_REQUEST_GET_V4:
      {
        ZW_APPLICATION_TX_BUFFER* pTxBuf = GetResponseBufferCb(ZCB_CmdClassFwUpdateMdReqReport);
        ZW_DEBUG_FIRMWARE_UPDATE_SEND_BYTE('a');
        /*Check pTxBuf is free*/
        if( NON_NULL( pTxBuf ) )
        {
          BYTE fwTarget = 0;
          WORD fragmentSize = 0;

          TRANSMIT_OPTIONS_TYPE_SINGLE_EX * pTxOptionsEx;
          ZW_DEBUG_FIRMWARE_UPDATE_SEND_BYTE('b');
          RxToTxOptions(rxOpt, &pTxOptionsEx);

          /*Get max size for V4 commands*/
          fragmentSize = handleCommandClassFirmwareUpdateMaxFragmentSize();

          /*Check V4 commands*/
          if( (sizeof(ZW_FIRMWARE_UPDATE_MD_REQUEST_GET_V3_FRAME) == cmdLength) ||
              (sizeof(ZW_FIRMWARE_UPDATE_MD_REQUEST_GET_V4_FRAME) == cmdLength ))
          {
            ZW_DEBUG_FIRMWARE_UPDATE_SEND_BYTE('c');
            fwTarget = pCmd->ZW_FirmwareUpdateMdRequestGetV4Frame.firmwareTarget;
            fragmentSize =  (((WORD)pCmd->ZW_FirmwareUpdateMdRequestGetV4Frame.fragmentSize1) << 8);
            fragmentSize +=  (((WORD)pCmd->ZW_FirmwareUpdateMdRequestGetV4Frame.fragmentSize2) & 0xff);
          }

          pTxBuf->ZW_FirmwareUpdateMdRequestReportV4Frame.cmdClass = COMMAND_CLASS_FIRMWARE_UPDATE_MD_V4;
          pTxBuf->ZW_FirmwareUpdateMdRequestReportV4Frame.cmd = FIRMWARE_UPDATE_MD_REQUEST_REPORT_V4;

          if( fwTarget < handleNbrFirmwareVersions())
          {
            uint8_t * pData = (uint8_t *)&(pCmd->ZW_FirmwareUpdateMdRequestGetV4Frame.manufacturerId1);
            FW_UPDATE_GET fwUpdateData;
            ZW_DEBUG_FIRMWARE_UPDATE_SEND_BYTE('d');

            fwUpdateData.manufacturerId = (((uint16_t)*(pData + 0)) << 8) | *(pData + 1);
            fwUpdateData.firmwareId     = (((uint16_t)*(pData + 2)) << 8) | *(pData + 3);
            fwUpdateData.checksum       = (((uint16_t)*(pData + 4)) << 8) | *(pData + 5);

            handleCmdClassFirmwareUpdateMdReqGet(
                rxOpt,
                fwTarget,
                fragmentSize,
                &fwUpdateData,
                &(pTxBuf->ZW_FirmwareUpdateMdRequestReportV4Frame.status));
          }
          else
          {
            ZW_DEBUG_FIRMWARE_UPDATE_SEND_BYTE('e');
            /*wrong target!!*/
            pTxBuf->ZW_FirmwareUpdateMdRequestReportV4Frame.status = FIRMWARE_UPDATE_MD_REQUEST_REPORT_NOT_UPGRADABLE_V4;
          }

          if (ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
                  (BYTE *)pTxBuf,
                  sizeof(ZW_FIRMWARE_UPDATE_MD_REQUEST_REPORT_V4_FRAME),
                  pTxOptionsEx,
                  ZCB_ResponseJobStatus))
          {
            ZW_DEBUG_FIRMWARE_UPDATE_SEND_BYTE('f');
            FreeResponseBuffer();
            ZCB_CmdClassFwUpdateMdReqReport(TRANSMIT_COMPLETE_FAIL);
          }
          return RECEIVED_FRAME_STATUS_SUCCESS;
        }
      }
      return RECEIVED_FRAME_STATUS_FAIL;
      break;

    case FIRMWARE_UPDATE_ACTIVATION_SET_V4:
      if(FALSE == Check_not_legal_response_job(rxOpt) &&
         (cmdLength == sizeof(ZW_FIRMWARE_UPDATE_ACTIVATION_SET_V4_FRAME)))
      {
        ZW_APPLICATION_TX_BUFFER *pTxBuf = GetResponseBuffer();

        /*Check pTxBuf is free*/
        if( NON_NULL( pTxBuf ) )
        {
          TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx;
          RxToTxOptions(rxOpt, &pTxOptionsEx);

          memcpy(
              (uint8_t*)&pTxBuf->ZW_FirmwareUpdateActivationStatusReportV4Frame,
              (uint8_t*)&pCmd->ZW_FirmwareUpdateActivationSetV4Frame,
              sizeof(ZW_FIRMWARE_UPDATE_ACTIVATION_SET_V4_FRAME));

          pTxBuf->ZW_FirmwareUpdateActivationStatusReportV4Frame.cmdClass = COMMAND_CLASS_FIRMWARE_UPDATE_MD_V4;
          pTxBuf->ZW_FirmwareUpdateActivationStatusReportV4Frame.cmd = FIRMWARE_UPDATE_ACTIVATION_STATUS_REPORT_V4;
          pTxBuf->ZW_FirmwareUpdateActivationStatusReportV4Frame.firmwareUpdateStatus = ERROR_ACTIVATION_FIRMWARE;

          if(ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
              (BYTE *)pTxBuf,
              sizeof(ZW_FIRMWARE_UPDATE_ACTIVATION_STATUS_REPORT_V4_FRAME),
              pTxOptionsEx,
              ZCB_ResponseJobStatus))
          {
            /*Job failed, free transmit-buffer pTxBuf by clearing mutex */
            FreeResponseBuffer();
          }
          return RECEIVED_FRAME_STATUS_SUCCESS;
        }
      }
      return RECEIVED_FRAME_STATUS_FAIL;
      break;
    default:
      break;
  }
  return RECEIVED_FRAME_STATUS_NO_SUPPORT;
}

/*============================ CmdClassFirmwareUpdateMdStatusReport ===============================
** Function description
** This function...
** Values used for Firmware Update Md Status Report command
**
** Side effects:
**
**-------------------------------------------------------------------------*/
JOB_STATUS
CmdClassFirmwareUpdateMdStatusReport(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  BYTE status,
  WORD waitTime,
  VOID_CALLBACKFUNC(pCbFunc)(TRANSMISSION_RESULT * pTransmissionResult))
{
  ZW_APPLICATION_TX_BUFFER* pTxBuf = GetRequestBuffer(pCbFunc);
  TRANSMIT_OPTIONS_TYPE_SINGLE_EX* pTxOptionsEx = NULL;
  RxToTxOptions(rxOpt, &pTxOptionsEx);
  /* Send status, when finished */
  if (pTxBuf != NULL)
  {
    pTxBuf->ZW_FirmwareUpdateMdStatusReportV4Frame.cmdClass = COMMAND_CLASS_FIRMWARE_UPDATE_MD_V4;
    pTxBuf->ZW_FirmwareUpdateMdStatusReportV4Frame.cmd = FIRMWARE_UPDATE_MD_STATUS_REPORT_V4;
    pTxBuf->ZW_FirmwareUpdateMdStatusReportV4Frame.status = status;
    pTxBuf->ZW_FirmwareUpdateMdStatusReportV4Frame.waittime1 = (waitTime >> 8);
    pTxBuf->ZW_FirmwareUpdateMdStatusReportV4Frame.waittime2 = (waitTime & 0xff);
    if(ZW_TX_IN_PROGRESS != Transport_SendRequestEP(
        (BYTE *)pTxBuf,
        sizeof(ZW_FIRMWARE_UPDATE_MD_STATUS_REPORT_V4_FRAME),
        pTxOptionsEx,
        ZCB_RequestJobStatus))
    {
      TRANSMISSION_RESULT result = {0xff, TRANSMIT_COMPLETE_FAIL, TRANSMISSION_RESULT_FINISHED};
      pCbFunc(&result);
      FreeRequestBuffer();
      return JOB_STATUS_BUSY;
    }
     return JOB_STATUS_SUCCESS;
  }
  return JOB_STATUS_BUSY;
}



/*============================ CmdClassFirmwareUpdateMdGet ==================
** Function description
** Send command Firmware update  MD Get
**
** Side effects:
**
**-------------------------------------------------------------------------*/
JOB_STATUS
CmdClassFirmwareUpdateMdGet(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  WORD firmwareUpdateReportNumber)
{
  ZW_APPLICATION_TX_BUFFER* pTxBuf = GetRequestBuffer(NULL);
  TRANSMIT_OPTIONS_TYPE_SINGLE_EX* pTxOptionsEx = NULL;
  RxToTxOptions(rxOpt, &pTxOptionsEx);

  ZW_DEBUG_FIRMWARE_UPDATE_SEND_STR("CmdClassFirmwareUpdateMdGet");
  ZW_DEBUG_FIRMWARE_UPDATE_SEND_WORD_NUM(firmwareUpdateReportNumber);
  if (pTxBuf != NULL)
  {
  /* Ask for the next report */
    pTxBuf->ZW_FirmwareUpdateMdGetV4Frame.cmdClass = COMMAND_CLASS_FIRMWARE_UPDATE_MD_V4;
    pTxBuf->ZW_FirmwareUpdateMdGetV4Frame.cmd = FIRMWARE_UPDATE_MD_GET_V4;
    pTxBuf->ZW_FirmwareUpdateMdGetV4Frame.numberOfReports = 1;
    pTxBuf->ZW_FirmwareUpdateMdGetV4Frame.properties1 = firmwareUpdateReportNumber >> 8;
    pTxBuf->ZW_FirmwareUpdateMdGetV4Frame.reportNumber2 = firmwareUpdateReportNumber & 0xFF;
    if(ZW_TX_IN_PROGRESS != Transport_SendRequestEP(
        (BYTE *)pTxBuf,
        sizeof(ZW_FIRMWARE_UPDATE_MD_GET_V4_FRAME),
        pTxOptionsEx,
        ZCB_RequestJobStatus))
    {
      FreeRequestBuffer();
      return JOB_STATUS_BUSY;
    }
    return JOB_STATUS_SUCCESS;
  }
  return JOB_STATUS_BUSY;


}

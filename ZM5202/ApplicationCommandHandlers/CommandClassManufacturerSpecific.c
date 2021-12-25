/**
 * @file
 * Handler for Command Class Manufacturer Specific.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_TransportLayer.h>
#include <ZW_tx_mutex.h>
#include "config_app.h"
#include <CommandClassManufacturerSpecific.h>
#include <ZW_mem_api.h>
#include <ZW_nvr_api.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

static uint8_t manSpecificDeviceID[MAN_DEVICE_ID_SIZE];

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

received_frame_status_t
handleCommandClassManufacturerSpecific(
    RECEIVE_OPTIONS_TYPE_EX *rxOpt,
    ZW_APPLICATION_TX_BUFFER *pCmd,
    BYTE cmdLength
)
{
  ZW_APPLICATION_TX_BUFFER *pTxBuf;
  TRANSMIT_OPTIONS_TYPE_SINGLE_EX *txOptionsEx;
  UNUSED(cmdLength);

  if (TRUE == Check_not_legal_response_job(rxOpt))
  {
    // None of the following commands support endpoint bit addressing.
    return RECEIVED_FRAME_STATUS_FAIL;
  }

  switch(pCmd->ZW_Common.cmd)
  {
    case MANUFACTURER_SPECIFIC_GET_V2:
      pTxBuf = GetResponseBuffer();
      if (IS_NULL(pTxBuf))
      {
        return RECEIVED_FRAME_STATUS_FAIL;
      }
      RxToTxOptions(rxOpt, &txOptionsEx);
      pTxBuf->ZW_ManufacturerSpecificReportV2Frame.cmdClass = COMMAND_CLASS_MANUFACTURER_SPECIFIC_V2;
      pTxBuf->ZW_ManufacturerSpecificReportV2Frame.cmd = MANUFACTURER_SPECIFIC_REPORT_V2;
      pTxBuf->ZW_ManufacturerSpecificReportV2Frame.manufacturerId1 = (APP_MANUFACTURER_ID >> 8);
      pTxBuf->ZW_ManufacturerSpecificReportV2Frame.manufacturerId2 = (APP_MANUFACTURER_ID & 0xFF);
      pTxBuf->ZW_ManufacturerSpecificReportV2Frame.productTypeId1 = (APP_PRODUCT_TYPE_ID >> 8);
      pTxBuf->ZW_ManufacturerSpecificReportV2Frame.productTypeId2 = (APP_PRODUCT_TYPE_ID & 0xFF);
      pTxBuf->ZW_ManufacturerSpecificReportV2Frame.productId1 = (APP_PRODUCT_ID>>8);
      pTxBuf->ZW_ManufacturerSpecificReportV2Frame.productId2 = (APP_PRODUCT_ID & 0xFF);

      if (ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
          (BYTE *)pTxBuf,
          sizeof(ZW_MANUFACTURER_SPECIFIC_REPORT_V2_FRAME),
          txOptionsEx,
          ZCB_ResponseJobStatus))
      {
        /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
        FreeResponseBuffer();
      }
      return RECEIVED_FRAME_STATUS_SUCCESS;
      break;
    case DEVICE_SPECIFIC_GET_V2:
      pTxBuf = GetResponseBuffer();
      /*Check pTxBuf is free*/
      if (IS_NULL(pTxBuf))
      {
        return RECEIVED_FRAME_STATUS_FAIL;
      }

      RxToTxOptions(rxOpt, &txOptionsEx);
      pTxBuf->ZW_DeviceSpecificReport1byteV2Frame.cmdClass = COMMAND_CLASS_MANUFACTURER_SPECIFIC_V2;
      pTxBuf->ZW_DeviceSpecificReport1byteV2Frame.cmd = DEVICE_SPECIFIC_REPORT_V2;
      pTxBuf->ZW_DeviceSpecificReport1byteV2Frame.properties1 = APP_DEVICE_ID_TYPE;
      pTxBuf->ZW_DeviceSpecificReport1byteV2Frame.properties2 = (APP_DEVICE_ID_FORMAT << 5) | (MAN_DEVICE_ID_SIZE & 0x1F);
      memcpy(&(pTxBuf->ZW_DeviceSpecificReport1byteV2Frame.deviceIdData1), manSpecificDeviceID, MAN_DEVICE_ID_SIZE);

      if(ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
          (BYTE *)pTxBuf,
          sizeof(ZW_DEVICE_SPECIFIC_REPORT_1BYTE_V2_FRAME) +
            (pTxBuf->ZW_DeviceSpecificReport1byteV2Frame.properties2 & 0x1F) - 1, /*Drag out length field 0x1F*/
          txOptionsEx,
          ZCB_ResponseJobStatus))
      {
        /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
        FreeResponseBuffer();
      }
      return RECEIVED_FRAME_STATUS_SUCCESS;
      break;
  }
  return RECEIVED_FRAME_STATUS_NO_SUPPORT;
}

void
ManufacturerSpecificDeviceIDInit(void)
{
 /*Read UUID from NVR*/
  ZW_NVRGetValue(offsetof(NVR_FLASH_STRUCT, abUUID), MAN_DEVICE_ID_SIZE, manSpecificDeviceID);
}

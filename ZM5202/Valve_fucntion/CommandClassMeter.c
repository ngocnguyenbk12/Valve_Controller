/***************************************************************************
*
* Copyright (c) 2001-2011
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: COmmand Class All Switch Handle
*
* Author:
*
* Last Changed By:  $Author:  NgocNguyen
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
#include <ZW_TransportMulticast.h>
#include "config_app.h"
#include <CommandClassMeter.h>
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

received_frame_status_t
handleCommandClassMeter(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  BYTE cmdLength)
{
  UNUSED(cmdLength);

  switch (pCmd->ZW_Common.cmd)
  {
	case METER_RESET_V2:
		if (FALSE == Check_not_legal_response_job(rxOpt))
      {
		
       ZW_APPLICATION_TX_BUFFER *pTxBuf = GetResponseBuffer();
        /*Check pTxBuf is free*/
        if (NON_NULL(pTxBuf))
        {
          MeterReset();
          return RECEIVED_FRAME_STATUS_SUCCESS;
        }
      }
      return RECEIVED_FRAME_STATUS_FAIL;
      break;
	  
	
//	case METER_GET_V2:
//		if (FALSE == Check_not_legal_response_job(rxOpt))
//      {
//		
//       ZW_APPLICATION_TX_BUFFER *pTxBuf = GetResponseBuffer();
//        /*Check pTxBuf is free*/
//        if (NON_NULL(pTxBuf))
//        {
//			TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx;
//			RxToTxOptions(rxOpt, &pTxOptionsEx);
//			
//          pTxBuf->ZW_MeterReport1byteV2Frame.cmdClass = COMMAND_CLASS_METER;
//          pTxBuf->ZW_MeterReport1byteV2Frame.cmd = METER_REPORT;
//       //   pTxBuf->ZW_MeterReport1byteV2Frame.meterType = METER_REPORT_WATER_METER;
//				pTxBuf->ZW_MeterReport1byteV2Frame.properties1 = 0x09;
//				pTxBuf->ZW_MeterReport1byteV2Frame.meterValue1 = getMeter() ;
//				pTxBuf->ZW_MeterReport1byteV2Frame.deltaTime1 = 0x00;
//				pTxBuf->ZW_MeterReport1byteV2Frame.deltaTime2 = 0x00;
//				pTxBuf->ZW_MeterReport1byteV2Frame.previousMeterValue1 = 0x00;
//          
//          if (ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
//            (BYTE *)pTxBuf,
//            sizeof(ZW_METER_REPORT_1BYTE_FRAME),
//           pTxOptionsEx,
//            ZCB_ResponseJobStatus))
//          {
//            /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
//            FreeResponseBuffer();
//          }
//          return RECEIVED_FRAME_STATUS_SUCCESS;
//        }
//      }
//      return RECEIVED_FRAME_STATUS_FAIL;
//      break;
//	
    case METER_GET:
      if (FALSE == Check_not_legal_response_job(rxOpt))
      {
		
       ZW_APPLICATION_TX_BUFFER *pTxBuf = GetResponseBuffer();
        /*Check pTxBuf is free*/
        if (NON_NULL(pTxBuf))
        {
			TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx;
			RxToTxOptions(rxOpt, &pTxOptionsEx);
			
          pTxBuf->ZW_MeterReport1byteFrame.cmdClass = COMMAND_CLASS_METER;
          pTxBuf->ZW_MeterReport1byteFrame.cmd = METER_REPORT;
          pTxBuf->ZW_MeterReport1byteFrame.meterType = METER_REPORT_WATER_METER;
				pTxBuf->ZW_MeterReport1byteFrame.properties1 = 0x09;
				pTxBuf->ZW_MeterReport1byteFrame.meterValue1 = getMeter() ;
          
          if (ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
            (BYTE *)pTxBuf,
            sizeof(ZW_METER_REPORT_1BYTE_FRAME),
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
	/*  
	case METER_SUPPORTED_GET:
		if (FALSE == Check_not_legal_response_job(rxOpt))
		  {
			
		   ZW_APPLICATION_TX_BUFFER *pTxBuf = GetResponseBuffer();
			if (NON_NULL(pTxBuf))
			{
				TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx;
				RxToTxOptions(rxOpt, &pTxOptionsEx);
				
			  pTxBuf->ZW_MeterSupportedReportV2Frame.cmdClass = COMMAND_CLASS_METER;
			  pTxBuf->ZW_MeterSupportedReportV2Frame.cmd = METER_REPORT;
//			  pTxBuf->ZW_MeterSupportedReportV2Frame.meterType = METER_REPORT_WATER_METER;
					pTxBuf->ZW_MeterSupportedReportV2Frame.properties1 = METER_REPORT_WATER_METER;
					pTxBuf->ZW_MeterSupportedReportV2Frame.properties2 = getMeter() ;
			  
			  if (ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
				(BYTE *)pTxBuf,
				sizeof(ZW_METER_REPORT_1BYTE_FRAME),
			   pTxOptionsEx,
				ZCB_ResponseJobStatus))
			  {

				FreeResponseBuffer();
			  }
			  return RECEIVED_FRAME_STATUS_SUCCESS;
			}
		  }
		  return RECEIVED_FRAME_STATUS_FAIL;
		  break;
		*/
	  
    
  }
  return RECEIVED_FRAME_STATUS_NO_SUPPORT;
}


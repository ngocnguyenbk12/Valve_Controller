/**
 * @file
 * Handler for Command Class Configuration
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
#include <CommandClassConfiguration.h>
#include <misc.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

BYTE bvalue ;


received_frame_status_t
handleCommandClassConfiguration(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  BYTE cmdLength)
{
  
  UNUSED(cmdLength);

  switch (pCmd->ZW_Common.cmd)
  {
	 case CONFIGURATION_SET:
      {
        setConfiguration(pCmd->ZW_ConfigurationSet1byteFrame.parameterNumber,
						pCmd->ZW_ConfigurationSet1byteFrame.configurationValue1);
      }
      return RECEIVED_FRAME_STATUS_SUCCESS;
      break;
	  
    case CONFIGURATION_GET:
      if (FALSE == Check_not_legal_response_job(rxOpt))
      {
		
       ZW_APPLICATION_TX_BUFFER *pTxBuf = GetResponseBuffer();
        /*Check pTxBuf is free*/
        if (NON_NULL(pTxBuf))
        {
			TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx;
			RxToTxOptions(rxOpt, &pTxOptionsEx);
			
          pTxBuf->ZW_ConfigurationReport1byteFrame.cmdClass = COMMAND_CLASS_CONFIGURATION;
          pTxBuf->ZW_ConfigurationReport1byteFrame.cmd = CONFIGURATION_REPORT;
          pTxBuf->ZW_ConfigurationReport1byteFrame.parameterNumber = pCmd->ZW_ConfigurationSet1byteFrame.parameterNumber;
				pTxBuf->ZW_ConfigurationReport1byteFrame.level = 0x01;
				pTxBuf->ZW_ConfigurationReport1byteFrame.configurationValue1 = getConfiguration(pCmd->ZW_ConfigurationSet1byteFrame.parameterNumber);
          
          if (ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
            (BYTE *)pTxBuf,
            sizeof(ZW_CONFIGURATION_REPORT_1BYTE_FRAME),
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


JOB_STATUS
CommandClassConfigurationReport(
	AGI_PROFILE* pProfile,
  BYTE sourceEndpoint,
  BYTE bvalue,
  VOID_CALLBACKFUNC(pCallback)(TRANSMISSION_RESULT * pTransmissionResult))
{
	CMD_CLASS_GRP cmdGrp = {COMMAND_CLASS_CONFIGURATION, CONFIGURATION_REPORT};
  return cc_engine_multicast_request(
      pProfile,
      sourceEndpoint,
      &cmdGrp,
      &bvalue,
      1,
      TRUE,
      pCallback);
}

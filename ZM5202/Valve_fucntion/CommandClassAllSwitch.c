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
#include <CommandClassAllSwitch.h>
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
handleCommandClassAllSwitch(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  BYTE cmdLength)
{
  UNUSED(cmdLength);

  switch (pCmd->ZW_Common.cmd)
  {
    case SWITCH_ALL_ON:
      CommandClassBinarySwitchSupportSet(CMD_CLASS_BIN_ON, ENDPOINT_1 );
      return RECEIVED_FRAME_STATUS_SUCCESS;
      break;	  
	case SWITCH_ALL_OFF:
		CommandClassBinarySwitchSupportSet(CMD_CLASS_BIN_OFF, ENDPOINT_1 );
	  return RECEIVED_FRAME_STATUS_SUCCESS;
	  break;
  }
  return RECEIVED_FRAME_STATUS_NO_SUPPORT;
}


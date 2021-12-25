/***************************************************************************
*
* Copyright (c) 2001-2011
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: Command Class All Switch Handle 
*
* Author:
*
* Last Changed By:  $Author:  NgocNguyen
* Revision:         $Revision:  $
* Last Changed:     $Date:  $
*


/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include <ZW_sysdefs.h>
#include <ZW_pindefs.h>
#include <ZW_evaldefs.h>
#include <ZW_classcmd.h>
#include <CommandClass.h>
#include <ZW_TransportEndpoint.h>
#include <CommandClassBinarySwitch.h>
#include <agi.h>

#define CommandClassBinarySwitchVersionGet() SWITCH_BINARY_VERSION

/**
 * The value can be either 0x00 (off/disable) or 0xFF (on/enable). The values from
 * 1 to 99 (0x01 to 0x63) MUST be mapped to 0xFF upon receipt of the Command in the
 * device. All other values are reserved and MUST be ignored by the receiving device.
 */

received_frame_status_t
handleCommandClassAllSwitch(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  BYTE cmdLength);
  
void CommandClassAllSwitchOn(
  uint8_t val,
  uint8_t endpoint);
  

void CommandClassAllSwitchOff(
  uint8_t val,
  uint8_t endpoint);
  
  
extern void handleApplAllSwitchSet(
  CMD_CLASS_BIN_SW_VAL val,
  uint8_t endpoint);  

  

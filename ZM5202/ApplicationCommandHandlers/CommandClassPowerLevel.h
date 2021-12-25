/**
 * @file
 * Handler for Command Class Powerlevel.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _COMMAND_CLASS_POWERLEVEL_H_
#define _COMMAND_CLASS_POWERLEVEL_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include <ZW_classcmd.h>
#include <ZW_TransportEndpoint.h>

/**
 * Returns the version of this CC.
 */
#define CC_Powerlevel_getVersion() POWERLEVEL_VERSION

/**
 * For backwards compatibility.
 */
#define CommandClassPowerLevelVersionGet()    CC_Powerlevel_getVersion()
#define handleCommandClassPowerLevel(a, b, c) CC_Powerlevel_handler(a, b, c)
#define CommandClassPowerLevelIsInProgress()  CC_Powerlevel_isInProgress()

/*
 * TODO: Merge the following two functions (loadStatusPowerLevel & loadInitStatusPowerLevel) into
 * one init function if we're not saving anything in NVM.
 */
/**
 * @brief loadStatusPowerLevel
 * Load parameters from NVM
 */
void loadStatusPowerLevel(void);

 /**
 * @brief loadInitStatusPowerLevel
 * loads initial power level status from nvram
 */
void loadInitStatusPowerLevel(void);

/**
 * @brief Handler for Powerlevel CC.
 * @param[in] rxOpt Pointer to receive options.
 * @param[in] pCmd Pointer to payload from the received frame
 * @param[in] cmdLength Length of the received command given in bytes.
 * @return receive frame status.
 */
received_frame_status_t CC_Powerlevel_handler(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  uint8_t cmdLength);

/**
 * Returns whether a powerlevel test is in progress.
 * @return TRUE if in progress, FALSE otherwise.
 */
BOOL CC_Powerlevel_isInProgress(void);

#endif

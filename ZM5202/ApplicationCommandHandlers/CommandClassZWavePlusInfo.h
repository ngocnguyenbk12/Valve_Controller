/**
 * @file
 * Handler for Command Class Z-Wave Plus Info.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _COMMAND_CLASS_ZWAVE_PLUS_INFO_H_
#define _COMMAND_CLASS_ZWAVE_PLUS_INFO_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include <ZW_sysdefs.h>
#include <ZW_pindefs.h>
#include <ZW_evaldefs.h>
#include <ZW_classcmd.h>
#include <CommandClass.h>

/**
 * Returns the version of this CC.
 */
#define CommandClassZWavePlusVersionGet() ZWAVEPLUS_INFO_VERSION_V2
#define CommandClassZWavePlusVersion() CommandClassZWavePlusVersionGet()

/**
 * Endpoint Icons data struct
 */
typedef struct _ENDPOINT_ICONS_
{
  uint16_t installerIconType;
  uint16_t userIconType;
} ST_ENDPOINT_ICONS;

/**
 * @brief Init Command class Z-Wave Plus Info
 * @param[in] pEpIcon pointer to device incon types
 * @param[in] nbrOfElements number of elements
 * @return description..
 */
void CommandClassZWavePlusInfoInit(ST_ENDPOINT_ICONS* pEpIcon, uint8_t nbrOfElements);

/**
 * @brief Handler for Z-Wave Plus Info CC.
 * @param[in] rxOpt Pointer to receive options.
 * @param[in] pCmd Pointer to payload from the received frame
 * @param[in] cmdLength Length of the received command given in bytes.
 * @return receive frame status.
 */
received_frame_status_t handleCommandClassZWavePlusInfo(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  uint8_t cmdLength);

#endif

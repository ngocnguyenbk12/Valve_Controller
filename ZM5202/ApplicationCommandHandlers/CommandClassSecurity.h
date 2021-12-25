/**
 * @file
 * Handler for Command Class Security & Command Class Security 2.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

#ifndef PRODUCTPLUS_APPLICATIONCOMMANDHANDLERS_COMMANDCLASSSECURITY_H_
#define PRODUCTPLUS_APPLICATIONCOMMANDHANDLERS_COMMANDCLASSSECURITY_H_
#include <ZW_classcmd.h>

/**
 * @brief handleCommandClassSecurity
 * Handler for command class security, command SECURITY_COMMANDS_SUPPORTED_GET
 * @param[in] rxOpt IN receive options of type RECEIVE_OPTIONS_TYPE_EX
 * @param[in] pCmd IN  Payload from the received frame
 * @param[in] cmdLength IN Number of command bytes including the command
 * @return receive frame status.
 */
received_frame_status_t
handleCommandClassSecurity(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  BYTE cmdLength);


#endif /* PRODUCTPLUS_APPLICATIONCOMMANDHANDLERS_COMMANDCLASSSECURITY_H_ */

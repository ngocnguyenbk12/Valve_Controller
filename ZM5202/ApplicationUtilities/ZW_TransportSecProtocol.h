/**
 * @file
 * Implements functions for transporting frames over the secure protocol Z-Wave Network.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _TRANSPORT_SEC_PROTOCOL_H_
#define _TRANSPORT_SEC_PROTOCOL_H_


/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_basis_api.h>
#include <ZW_transport_api.h>
#include <ZW_TransportEndpoint.h>


#define NETWORK_KEY_LENGTH  16

/****************************************************************************/
/*                       PUBLIC TYPES and DEFINITIONS                       */
/****************************************************************************/


typedef struct _eeoffs_network_security_struct_
{
#ifdef ZW_CONTROLLER
  uint8_t      EEOFFS_SECURITY_SCHEME_field;
#endif
  uint8_t      EEOFFS_NETWORK_SECURITY_field;
  uint8_t      EEOFFS_NETWORK_KEY_START_field[NETWORK_KEY_LENGTH];
//new eeprom variables add only before this magic byte variable (and don't forget to change offset of magic byte!!!)
  uint8_t      EEOFFS_MAGIC_BYTE_field;
  uint8_t      EEOFFS_NETWORK_SECURITY_RESERVED_field;  /* Deprecated field used to live here - is it okay to recycle it? */
} EEOFFS_NETWORK_SECURITY_STRUCT;


/****************************************************************************/
/*                              IMPORTED DATA                               */
/****************************************************************************/


/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/**
 * Structure holding information about node lists, device option mask and node type.
 */
typedef struct
{
  uint8_t *cmdClassListNonSecure;            /**< Nonsecure-list of supported command classes, when node communicate by this transport */
  uint8_t cmdClassListNonSecureCount;        /**< Count of elements in supported command classes Nonsecure-list */
  uint8_t *cmdClassListNonSecureIncludedSecure; /**< Nonsecure-list of supported command classes, when node communicate by this transport */
  uint8_t cmdClassListNonSecureIncludedSecureCount;  /**< Count of elements in supported command classes Nonsecure-list */
  uint8_t *cmdClassListSecure;               /**< Secure-list of supported command classes, when node communicate by this transport */
  uint8_t cmdClassListSecureCount;           /**< Count of elements in supported command classes Secure-list */
  uint8_t deviceOptionsMask; /**< See ZW_basic_api.h for ApplicationNodeInformation field deviceOptionMask */
  APPL_NODE_TYPE nodeType;
}
APP_NODE_INFORMATION;


/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/
/* params used by ApplicationNodeInformation */
#ifdef slave_routing
#define APPL_NODEPARM_MAX       20
#else
#define APPL_NODEPARM_MAX       35
#endif


/****************************************************************************/
/*                           IMPORTED FUNCTIONS                             */
/****************************************************************************/

/**
 * Handles commands.
 * @param[in] pCmd Pointer to command.
 * @param[in] cmdLength Command length.
 * @param[in] rxOpt Receive options.
 */
extern void Transport_ApplicationCommandHandler(
    ZW_APPLICATION_TX_BUFFER *pCmd,
    uint8_t cmdLength,
    RECEIVE_OPTIONS_TYPE *rxOpt);

/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

/**
 * This function is not used for anything at the moment, but was added for possible future
 * needs. It does nothing and the call for this function in ApplicationInitHW can be omitted
 * if desired.
 * @param[in] bStatus Status of hardware initialization.
 * @return Returns TRUE always.
 */
BOOL Transport_OnApplicationInitHW(uint8_t bStatus);

/**
 * Initializes the framework transport layer. It must be called at the end of
 * ApplicationInitSW() in the application.
 * @param[in] pAppNode Pointer to a struct holding information about the node.
 * @return TRUE if transport layer is initialized, FALSE otherwise.
 */
uint8_t Transport_OnApplicationInitSW(
    APP_NODE_INFORMATION * pAppNode);

/**
 * Resets the transport layer.
 */
void Transport_SetDefault(void);

/**
 * Must be called upon learn mode completion.
 * @param[in] nodeID The node ID obtained from learn mode.
 * @return
 */
uint8_t Transport_OnLearnCompleted(uint8_t nodeID);

/**
 * Returns whether a command class is supported on a given security level.
 * @param[in] commandClass Command class to test for.
 * @param[in] command Command to test for.
 * @param[in] eKey Security level to test upon.
 * @return TRUE if the command class is supported, FALSE otherwise.
 */
BOOL TransportCmdClassSupported(uint8_t commandClass,
                                uint8_t command,
                                enum SECURITY_KEY eKey);

/**
 * Get highest secure level
 * @param protocolSecBits secure bits
 * @return secure level
 */
enum SECURITY_KEY GetHighestSecureLevel(uint8_t protocolSecBits);

/**
 * Returns the node ID.
 * @return Node ID.
 */
uint8_t GetNodeId(void);

/**
 * Get command class list from device
 * @param[in] included boolean value for node is included
 * @param[in] eKey security key of type security_key_t
 * @param[in] endpoint 0 - root, 1 - X endpoint number
 */
CMD_CLASS_LIST * GetCommandClassList(
    BOOL included,
    security_key_t eKey,
    uint8_t endpoint);

/**
 * Kicks the keep awake timer.
 */
void ZW_TSP_timer_kick(void);

#endif /*_TRANSPORT_SEC_PROTOCOL_H_*/

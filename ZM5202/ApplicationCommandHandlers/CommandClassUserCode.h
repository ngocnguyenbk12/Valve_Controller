/**
 * @file
 * Handler for Command Class User Code.
 *
 * The purpose of the User Code Command Class is to supply a enabled
 * Door Lock Device with a command class to manage user codes.
 *
 * User Identifier (8 bits).
 * -------------------------
 * The User Identifier used to recognise the user identity. The User Identifier
 * values MUST be a sequence starting from 1. This field can be ignored in case
 * the node only supports one User Code. Setting the User Identifier to 0 will
 * address all User Identifiers available in the device.
 *
 * USER_CODE1, USER_CODEn.
 * -----------------------
 * These fields contain the user code. Minimum code length is 4 and maximum 10
 * ASCII digits. The number of data fields transmitted can be determined from
 * the length field returned by the ApplicationCommandHandler. The user code
 * fields MUST be initialize to 0x00 0x00 0x00 0x00 (4 bytes) when User ID
 * Status is equal to 0x00.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _COMMANDCLASSUSERCODE_H_
#define _COMMANDCLASSUSERCODE_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <CommandClass.h>
#include <agi.h>

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

/**
 * Returns the version of this CC.
 */
#define CommandClassUserCodeVersionGet() USER_CODE_VERSION

/**
 * User ID Status.
 * ---------------
 * The User ID Status field indicates the state of the User Identifier. All
 * other values not mentioned in below list are reserved for future
 * implementation.
 * Hex | Description
 * ----|---------------------------
 *  00 | Available (not set)
 *  01 | Occupied
 *  02 | Reserved by administrator
 *  FE | Status not available
 */
typedef enum
{
  USER_ID_AVAILBLE = 0x00, /**< Available (not set)*/
  USER_ID_OCCUPIED = 0x01, /**< Occupied*/
  USER_ID_RESERVED = 0x02, /**< Reserved by administrator*/
  USER_ID_NO_STATUS = 0xFE /**<	Status not available*/
}
user_id_status_t;
typedef user_id_status_t USER_ID_STATUS;

/**
 * Minimum length of a user code as defined in SDS12652.
 */
#define USERCODE_MIN_LEN 4

/**
 * Maximum length of a user code as defined in SDS12652.
 */
#define USERCODE_MAX_LEN 10

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

// Nothing here.

/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

/**
 * @brief The User Code Set Command used to set a User Code in the device.
 * @param[in] identifier User Identifier.
 * @param[in] id user Id status.
 * @param[in] pUserCode pointer to UserCode data.
 * @param[in] len UserCode data.
 * @param[in] endpoint is the destination endpoint
 * @return Returns TRUE if the user code was set, FALSE otherwise.
 */
extern BOOL
handleCommandClassUserCodeSet(
  uint8_t identifier,
  user_id_status_t id,
  uint8_t* pUserCode,
  uint8_t len,
  uint8_t endpoint);

/**
 * @brief The User Code Get ID.
 * @param[in] identifier User Identifier.
 * @param[in] pId pointer to return Id.
 * @param[in] endpoint is the destination endpoint
 * @return status valid boolean.
 */
extern BOOL handleCommandClassUserCodeIdGet(
  uint8_t identifier,
  user_id_status_t * pId,
  uint8_t endpoint);


/**
 * @brief The User Code Report Command can be used by e.g. a door lock device to send a
 * report either unsolicited or requested by the User Code Get Command.
 * @param[in] identifier User Identifier.
 * @param[out] pUserCode pointer to UserCode data.
 * @param[out] pLen length UserCode data.
 * @param[in] endpoint is the destination endpoint
 * @return status valid boolean.
 */
extern BOOL handleCommandClassUserCodeReport(
  uint8_t identifier,
  uint8_t* pUserCode,
  uint8_t *pLen,
  uint8_t endpoint);


/**
 * @brief The Users Number Report Command used to report the maximum number of USER CODES
 * the given node supports. The Users Number Report Command can be send requested
 * by the Users Number Get Command.
 * @param[in] endpoint is the destination endpoint
 * @return maximum number of USER CODES.
 */
extern uint8_t handleCommandClassUserCodeUsersNumberReport( uint8_t endpoint );

/**
 * @brief Handler for command class User Code
 * @param[in] rxOpt receive options of type RECEIVE_OPTIONS_TYPE_EX
 * @param[in] pCmd Payload from the received frame
 * @param cmdLength IN Number of command bytes including the command
 * @return receive frame status.
 */
received_frame_status_t handleCommandClassUserCode(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  uint8_t cmdLength);


/**
 * @brief Send a Command Class User code support report
 * @param[in] pProfile pointer to AGI profile
 * @param[in] sourceEndpoint source endpoint
 * @param[in] userIdentifier user identifier
 * @param[in] userIdStatus user Id status
 * @param[in] pUserCode user code
 * @param[in] userCodeLen length of user code
 * @param[out] pCallback callback function returning status destination node receive job.
 * @return status on the job.
 */
JOB_STATUS
CmdClassUserCodeSupportReport(
  AGI_PROFILE* pProfile,
  uint8_t sourceEndpoint,
  uint8_t userIdentifier,
  uint8_t userIdStatus,
  uint8_t* pUserCode,
  uint8_t userCodeLen,
  VOID_CALLBACKFUNC(pCallback)(TRANSMISSION_RESULT * pTransmissionResult));

#endif /* _COMMANDCLASSUSERCODE_H_ */


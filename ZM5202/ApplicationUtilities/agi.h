/**
 * @file
 * Helper module for Command Class Association Group Information.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _AGI_H_
#define _AGI_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include <ZW_typedefs.h>
#include <ZW_TransportEndpoint.h>
#include <CommandClass.h>

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

/**
 * Enmum type NODE_LIST_STATUS is used for return status on API call AGI_NodeIdListGetNext.
 * Enum types from NODE_LIST_STATUS_SUCCESS to NODE_LIST_STATUS_ERROR_LIST deliver status
 * on the call and after NODE_LIST_STATUS_ERROR_LIST deliver an error identifiers pointing
 * to a problem in application AGI/association configuarion.
 */
typedef enum
{
  NODE_LIST_STATUS_SUCCESS = 0,
  NODE_LIST_STATUS_NO_MORE_NODES,
  NODE_LIST_STATUS_ASSOCIATION_LIST_EMPTY,
  NODE_LIST_STATUS_ERROR_LIST,              /**< enum values higher than this is error identifiers*/
  NODE_LIST_STATUS_ERR_NO_TABLE_ENDPOINT,
  NODE_LIST_STATUS_ERR_UNKNOWN_PROFILE,
  NODE_LIST_STATUS_ERR_ENDPOINT_OUT_OF_RANGE,
  NODE_LIST_STATUS_ERR_GROUP_NBR_NOT_LEGAL,
  NODE_LIST_STATUS_ERR_LIFELINE_PROFILE_NOT_SUPPORTED,
  NODE_LIST_STATUS_ERR_LIFELINE_SUPPORT_NOT_CC_BASIC,
  NODE_LIST_STATUS_ERR_PROFILE_LIFELINE_ONLY_SUPPORT_IN_GRP_1,
}
NODE_LIST_STATUS;

/**
 * NODE_LIST is used to control current AGI profile job.
 */
typedef struct _NODE_LIST_
{
  uint8_t sourceEndpoint;       /**< Active endpoint handling AGI profile job */
  MULTICHAN_NODE_ID* pNodeList; /**< pointer to the node list for AGI profile group */
  uint8_t len;                     /**< Length of the node list */
}
NODE_LIST;

/**
 * Structure for an AGI group including profile, one command class group and group name
 */
typedef struct _AGI_GROUP_
{
  AGI_PROFILE profile;
  CMD_CLASS_GRP cmdGrp; /**< AGI Profile cmd class group*/
  char groupName[42]; /**< AGI Profile group-name UTF-8 format*/
}
AGI_GROUP;

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

// Nothing here.

/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

/**
 * Initializes all AGI parameters. This MUST be called before calling other AGI functions.
 */
//@ [AGI_Init]
void CC_AGI_Init(void);
//@ [AGI_Init]

/**
 * For backwards compatibility.
 */
#define AGI_Init() CC_AGI_Init()

/**
 * Setup AGI Lifeline command classes and commands for each endpoint.
 * @param[in] pCmdGrpList Pointer to an array of command class and command pairs.
 * @param[in] listSize The number of command class and command pairs for the Lifeline (array size).
 * @param[in] endpoint The endpoint for which the array applies.
 */
//@ [AGI_LifeLineGroupSetup]
void CC_AGI_LifeLineGroupSetup(
    cc_group_t const * const pCmdGrpList,
    uint8_t listSize,
    uint8_t endpoint);
//@ [AGI_LifeLineGroupSetup]

/**
 * For backwards compatibility.
 *
 * Since Lifeline groups of both root device and endpoints must be named "Lifeline", the text is
 * hardcoded in this module. Hence, there's no need for an input parameter for the group name.
 */
#define AGI_LifeLineGroupSetup(a, b, c, d) CC_AGI_LifeLineGroupSetup(a, b, d);

 /**
 * Sets up the AGI table of groups for a given endpoint (or root device).
 * @param[in] pTable[] is AGI table for one endpoint.
 * @param[in] tableSize is number of groups in table.
 * @param[in] endpoint device endpoint number for the AGI table. Endpoint 0 and 1 will be
 * handle as the same number!
 */
//@ [AGI_ResourceGroupSetup]
void CC_AGI_ResourceGroupSetup(
    AGI_GROUP const * const pTable,
    uint8_t tableSize,
    uint8_t endpoint);
//@ [AGI_ResourceGroupSetup]

/**
 * For backwards compatibility.
 */
#define AGI_ResourceGroupSetup(a, b, c) CC_AGI_ResourceGroupSetup(a, b, c)

/**
 * Request node list is used by Command classes modules to extract the association nodelist for a
 * request call.
 * @param[in] pProfile is a pointer to AGI profile.
 * @param[in] pCurrentCmdGrp is a pointer to command class group
 * @param[in] sourceEndpoint device source endpoint
 * @return transmit option pointer of type TRANSMIT_OPTIONS_TYPE_EX. Return NULL if something vent
 * wrong.
 */
TRANSMIT_OPTIONS_TYPE_EX * ReqNodeList(
    AGI_PROFILE * pProfile,
    CMD_CLASS_GRP * pCurrentCmdGrp,
    uint8_t sourceEndpoint);

#endif /* _AGI_H_ */

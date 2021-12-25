/**
 * @file
 * Helper module for Command Class Association and Command Class Multi Channel Association.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _ASSOCIATION_PLUS_H_
#define _ASSOCIATION_PLUS_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include "config_app.h"
#include <agi.h>

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

/**
 * Size of an association group
 */
#define ASSOCIATION_SIZE (MAX_ASSOCIATION_GROUPS * MAX_ASSOCIATION_IN_GROUP)

/**
 * Group id numbers
 */
typedef enum _ASSOCIATION_GROUP_ID_
{
  ASS_GRP_ID_1 = 1,
  ASS_GRP_ID_2,
  ASS_GRP_ID_3,
  ASS_GRP_ID_4,
  ASS_GRP_ID_5,
  ASS_GRP_ID_6,
  ASS_GRP_ID_7,
  ASS_GRP_ID_8,
  ASS_GRP_ID_9
}
ASSOCIATION_GROUP_ID;

/**
 * Structure for mapping root groups to endpoint groups
 */
typedef struct _ENDPOINT_ASSOCIATION_MAPPING_
{
  BYTE rootGrpId;
  BYTE endpoint;
  BYTE endpointGrpId;
}
ASSOCIATION_ROOT_GROUP_MAPPING;

typedef struct _EEOFFS_NVM_TRANSPORT_CAPABILITIES_STRUCT_
{
  BYTE security             : 4; /**< bit 0-3 of type security_key_t: 0-NON_KEY,1-S2_UNAUTHENTICATED,
                                      2-S2_AUTHENTICATED, 3-S2_ACCESS, 4-S0 (security_key_t)*/
  BYTE unused                 : 1; /**< bit 4 */
  BYTE BitMultiChannelEncap   : 1; /**< bit 5 */
  BYTE unused2                : 1; /**< bit 6 */
  BYTE unused3                : 1; /**< bit 7 */
}
EEOFFSET_TRANSPORT_CAPABILITIES_STRUCT;

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

// Nothing here.

/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

/**
 * @brief Initializes the Association module. This function must be called by devices not using
 * endpoints.
 * @param[in] forceClearMem If TRUE, the association NVM will be cleared.
 */
void
AssociationInit(BOOL forceClearMem);

/**
 * @brief Initializes the Association module. This function must be called by devices using
 * endpoints.
 * @param[in] forceClearMem If TRUE, the association NVM will be cleared.
 * @param[in] pMapping is used for backwards compatibility to non-Multi Channel
 * devices. The mapping is used to configure the Root Device advertises the
 * association groups on behalf of Endpoints.
 * @param[in] nbrGrp is number of groups in pMapping list.
 */
void
AssociationInitEndpointSupport(
  BOOL forceClearMem,
  ASSOCIATION_ROOT_GROUP_MAPPING* pMapping,
  BYTE nbrGrp);

/**
 * @brief handleAssociationGetnodeList
 * Deliver group number node list
 * @param[in] groupIden Group ID.
 * @param[in] ep Endpoint.
 * @param[out] ppList is out double-pointer of type MULTICHAN_NODE_ID deliver node list
 * @param[out] pListLen length of list
 * @return enum type NODE_LIST_STATUS
 */
NODE_LIST_STATUS
handleAssociationGetnodeList(
  BYTE groupIden,
  BYTE ep,
  MULTICHAN_NODE_ID** ppList,
  BYTE* pListLen);

/**
 * @brief Removes all nodes or given node(s) from all groups or a given group.
 * @details See Association CC and Multi Channel Association CC for details.
 * @param[in] groupIden A given group ID.
 * @param[in] ep A given endpoint.
 * @param[in] pCmd Pointer to the command containing the node IDs to remove.
 * @param[in] cmdLength Length of the command.
 */
BOOL AssociationRemove(
  BYTE groupIden,
  BYTE ep,
  ZW_MULTI_CHANNEL_ASSOCIATION_REMOVE_1BYTE_V2_FRAME* pCmd,
  BYTE cmdLength);

/**
 * @brief Returns the number of association groups for a given endpoint.
 * @param[in] endpoint A given endpoint where 0 is the root device.
 * @return Number of association groups.
 */
BYTE
handleGetMaxAssociationGroups(BYTE endpoint);

/**
 * @brief Handles an incoming (Multi Channel) Association Get command and composes a (Multi Channel)
 * Association Report.
 * @param[in] endpoint The endpoint from which the associated nodes must be read.
 * @param[in] incomingFrame The incoming frame including CC and command.
 * @param[out] outgoingFrame The composed frame ready for transmission.
 * @param[out] outgoingFrameLength The total length of the outgoing frame.
 */
void
AssociationGet(
    uint8_t endpoint,
    uint8_t * incomingFrame,
    uint8_t * outgoingFrame,
    uint8_t * outgoingFrameLength);

/**
 * @brief Associates a given node in the given group for a given endpoint.
 * @details The endpoint argument specifies the local endpoint for which the association is made.
 * E.g. if this device would be a Wall Controller/Switch with 4 endpoints, one for each switch, and
 * an association was to be made for endpoint 1. The associated node would receive something when
 * button 1 is pressed (if of course button one represents endpoint 1).
 * @param groupID ID of the group in which the association must be made.
 * @param endpoint Endpoint for which the association must be made.
 * @param pNode Pointer to a node with info about node ID, endpoint, etc.
 * @param multiChannelAssociation Specifies whether the associated node ID includes an endpoint or
 * not.
 * @return TRUE if association is added, FALSE otherwise.
 */
BOOL AssociationAddNode(
    BYTE groupID,
    BYTE endpoint,
    MULTICHAN_DEST_NODE_ID* pNode,
    BOOL multiChannelAssociation);

#endif /* _ASSOCIATION_PLUS_H_ */

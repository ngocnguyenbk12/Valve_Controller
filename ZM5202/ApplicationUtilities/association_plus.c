/**
 * @file
 * Helper module for Command Class Association and Command Class Multi Channel Association.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include <ZW_basis_api.h>
#include <ZW_uart_api.h>
#include <association_plus.h>
#include <ZW_TransportLayer.h>
#include <CommandClassAssociation.h>
#include <CommandClassAssociationGroupInfo.h>
#include <eeprom.h>
#include <ZW_classcmd.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

#ifdef ZW_DEBUG_ASSOCIATION
#define ZW_DEBUG_ASSOCIATION_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_ASSOCIATION_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_ASSOCIATION_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_ASSOCIATION_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_ASSOCIATION_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_ASSOCIATION_SEND_BYTE(data)
#define ZW_DEBUG_ASSOCIATION_SEND_STR(STR)
#define ZW_DEBUG_ASSOCIATION_SEND_NUM(data)
#define ZW_DEBUG_ASSOCIATION_SEND_WORD_NUM(data)
#define ZW_DEBUG_ASSOCIATION_SEND_NL()
#endif

typedef enum _NVM_ACTION_
{
  READ_DATA,
  WRITE_DATA
}
NVM_ACTION;

typedef struct _ASSOCIATION_GROUP_
{
  MULTICHAN_NODE_ID subGrp[MAX_ASSOCIATION_IN_GROUP];
}
ASSOCIATION_GROUP;

typedef struct _ASSOCIATION_NODE_LIST_
{
  uint8_t * pNodeId; /*IN pointer to list of nodes*/
  uint8_t noOfNodes;  /*IN number of nodes in List*/
  MULTICHAN_DEST_NODE_ID * pMulChanNodeId; /*IN pointer to list of multi channel nodes*/
  uint8_t noOfMulchanNodes;  /*IN number of  multi channel nodes in List*/
}
ASSOCIATION_NODE_LIST;

#define MAGIC_VALUE (0x42)

#define OFFSET_CLASSCMD                       0x00
#define OFFSET_CMD                            0x01
#define OFFSET_PARAM_1                        0x02
#define OFFSET_PARAM_2                        0x03
#define OFFSET_PARAM_3                        0x04
#define OFFSET_PARAM_4                        0x05

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

/* Default values */

ASSOCIATION_GROUP groups[NUMBER_OF_ENDPOINTS + 1][MAX_ASSOCIATION_GROUPS];

BYTE groupReportSourceNode;

BYTE indx;

ASSOCIATION_ROOT_GROUP_MAPPING* pGroupMappingTable = NULL;
BYTE numberOfGroupMappingEntries = 0;

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

// Nothing here.

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

static BOOL handleCheckgroupIden(BYTE* pGroupIden, BYTE ep);
static void ExtractCmdClassNodeList(
    ASSOCIATION_NODE_LIST* plist,
    ZW_MULTI_CHANNEL_ASSOCIATION_SET_1BYTE_V2_FRAME* pCmd,
    BYTE cmdLength,
    BYTE* pMultiChannelAssociation);
static BOOL AssGroupMappingLookUp(BYTE* pEndpoint, BYTE* pGroupID);
static void AssociationEndpointClearAll(void);
static void AssociationClearAll(void);
static void AssociationStoreAll(void);
static void NVM_Action(NVM_ACTION action);
static void
RemoveAssociationsFromGroup(
    uint8_t cmdClass,
    uint8_t ep,
    uint8_t groupId,
    ASSOCIATION_NODE_LIST * pListOfNodes);

/**
 * @brief Reorders nodes in a given group.
 * @param groupIden Given group ID.
 * @param ep Given endpoint.
 * @param emptyIndx Location in index which must be filled up.
 */
static void
ReorderGroupAfterRemove(
  BYTE groupIden,      /*the group index to reorder*/
  BYTE ep,             /* endpoint*/
  BYTE emptyIndx)      /*the index of the empty field*/
{
  BYTE move;

  for(move = emptyIndx+1; move < MAX_ASSOCIATION_IN_GROUP; move++)
  {
    memcpy((BYTE*)&groups[ep][groupIden].subGrp[move-1], (BYTE*)&groups[ep][groupIden].subGrp[move], sizeof(MULTICHAN_NODE_ID));

    memset((BYTE*)&groups[ep][groupIden].subGrp[move], 0, sizeof(MULTICHAN_NODE_ID));
  }
}

NODE_LIST_STATUS
handleAssociationGetnodeList(
  BYTE groupIden,
  BYTE ep,
  MULTICHAN_NODE_ID** ppList,
  BYTE* pListLen)
{
  BYTE indx;

  if((NUMBER_OF_ENDPOINTS + 1) < ep )
  {
    return NODE_LIST_STATUS_ERR_ENDPOINT_OUT_OF_RANGE;
  }
  /*Check group number*/
  if(FALSE == handleCheckgroupIden(&groupIden, ep))
  {
    return NODE_LIST_STATUS_ERR_GROUP_NBR_NOT_LEGAL; /*not legal number*/
  }
  AssGroupMappingLookUp(&ep, &groupIden);

  *ppList = &groups[ep][groupIden - 1].subGrp[0];
  *pListLen = 0; // Assume zero associations unless finding some in the loop below.

  for (indx = MAX_ASSOCIATION_IN_GROUP; indx > 0; indx--)
  {
    if(0 < groups[ep][groupIden - 1].subGrp[indx - 1].node.nodeId)
    {
      *pListLen = indx; /*number of nodes in list*/
      break;  /* break out of loop*/
    }
  }
  if(0 == *pListLen)
  {
    return NODE_LIST_STATUS_ASSOCIATION_LIST_EMPTY;
  }
  return NODE_LIST_STATUS_SUCCESS;
}

/**
 * @brief Checks whether a given group ID is valid.
 * @param pGroupIden The group ID to check.
 * @param endpoint The endpoint to which the group belongs.
 * @return TRUE if group ID is valid, FALSE otherwise.
 */
static BOOL
handleCheckgroupIden(BYTE* pGroupIden, BYTE endpoint)
{
  if ((CC_AGI_groupCount_handler(endpoint) <= (*pGroupIden - 1)) || (0 == *pGroupIden))
  {
    return FALSE; /*not legal number*/
  }
  return TRUE;
}

BYTE
handleGetMaxNodesInGroup(
  BYTE groupIden,
  BYTE ep)
{
  if ((1 == groupIden) && (0 != ep))
  {
    return 0;
  }
  return MAX_ASSOCIATION_IN_GROUP;
}

BYTE
handleGetMaxAssociationGroups(BYTE endpoint)
{
  return CC_AGI_groupCount_handler(endpoint);
}

BOOL
AssociationAddNode(
    BYTE groupID,
    BYTE endpoint,
    MULTICHAN_DEST_NODE_ID* pNode,
    BOOL multiChannelAssociation)
{
  AssGroupMappingLookUp(&endpoint, &groupID);

  for (indx = 0; indx < handleGetMaxNodesInGroup(groupID, endpoint) /*MAX_ASSOCIATION_IN_GROUP*/; indx++)
  {
    if (((groups[endpoint][groupID - 1].subGrp[indx].node.nodeId == pNode->nodeId) &&
         (groups[endpoint][groupID - 1].subGrp[indx].node.endpoint == pNode->endpoint)&&
         (groups[endpoint][groupID - 1].subGrp[indx].node.BitAddress == pNode->BitAddress)) ||
        (0 == groups[endpoint][groupID - 1].subGrp[indx].node.nodeId))
    {
      memcpy((BYTE*)&groups[endpoint][groupID - 1].subGrp[indx].node, (BYTE*)pNode, sizeof(MULTICHAN_DEST_NODE_ID));
      groups[endpoint][groupID - 1].subGrp[indx].nodeInfo.BitMultiChannelEncap = multiChannelAssociation;
      groups[endpoint][groupID - 1].subGrp[indx].nodeInfo.security = GetHighestSecureLevel(ZW_GetSecurityKeys());
      return TRUE;
    }
  }

  return FALSE;
}

BOOL AssociationRemove(
  uint8_t groupId,
  uint8_t ep,
  ZW_MULTI_CHANNEL_ASSOCIATION_REMOVE_1BYTE_V2_FRAME* pCmd,
  uint8_t cmdLength)
{
  uint8_t i;
  uint8_t j;
  uint8_t * pCmdByteWise = (uint8_t *)pCmd;
  ASSOCIATION_NODE_LIST list;
  uint8_t maxNumberOfGroups;

  /*Only setup lifeline for rootdevice*/
  if ((NUMBER_OF_ENDPOINTS + 1) < ep || ((1 == groupId) && (0 < ep)))
  {
    return FALSE;
  }

  maxNumberOfGroups = CC_AGI_groupCount_handler(ep);

  if ((0 < groupId) && (maxNumberOfGroups >= groupId))
  {
    AssGroupMappingLookUp(&ep, &groupId);

    if ((3 == cmdLength) || ((4 == cmdLength) && (0x00 == *(pCmdByteWise + 3))))
    {
      /*
       * The command is either [Class, Command, GroupID] or [Class, Command, GroupID, Marker].
       * In either case, we delete all nodes in the given group.
       */
      for (i = 0; i < MAX_ASSOCIATION_IN_GROUP; i++)
      {
        /*
         * The node can only be deleted according to the following truth table.
         * CCA   = Command Class Association Remove Command
         * CCMCA = Command Class Multi Channel Association Remove Command
         * MC    = Multi Channel associated node
         * T     = TRUE
         * F     = FALSE
         *      | CCA | CCMCA |
         * --------------------
         * MC=T |  V  |   V   |
         * --------------------
         * MC=F |  %  |   V   |
         * --------------------
         */
        if (!((COMMAND_CLASS_ASSOCIATION == *pCmdByteWise) && (TRUE == groups[ep][groupId-1].subGrp[i].nodeInfo.BitMultiChannelEncap)))
        {
          memset((uint8_t*)&groups[ep][groupId-1].subGrp[i], 0, sizeof(MULTICHAN_NODE_ID));
        }
      }
    }
    else
    {
      // If this is the case, the command must be [Class, Command, GroupID, i3, i4, i5, ...].
      ExtractCmdClassNodeList(&list,(ZW_MULTI_CHANNEL_ASSOCIATION_SET_1BYTE_V2_FRAME*) pCmd, cmdLength, NULL);
      RemoveAssociationsFromGroup(*pCmdByteWise, ep, groupId, &list);
    }
    AssociationStoreAll();
  }
  else if (0 == groupId)
  {
    /*
     * When the group ID equals zero, it is desired to remove all nodes from all groups or given
     * nodes from all groups.
     * If the length is larger than 3, it means that only given nodes must be removed from all
     * groups.
     */
    ExtractCmdClassNodeList(&list,(ZW_MULTI_CHANNEL_ASSOCIATION_SET_1BYTE_V2_FRAME*) pCmd, cmdLength, NULL);

    for (ep = 0; ep < NUMBER_OF_ENDPOINTS + 1; ep++)
    {
      maxNumberOfGroups = CC_AGI_groupCount_handler(ep);
      for (j = 1; j <= maxNumberOfGroups; j++)
      {
        RemoveAssociationsFromGroup(*pCmdByteWise, ep, j, &list);
      }
    }
    AssociationStoreAll();
  }
  else
  {
    return FALSE;
  }
  return TRUE;
}

/**
 * @brief Removes given associations or all associations from a given group.
 * @param groupId ID of the group from where the associations must be removed.
 * @param listOfNodes List of nodes/associations that must be removed.
 */
static void
RemoveAssociationsFromGroup(
    uint8_t cmdClass,
    uint8_t ep,
    uint8_t groupId,
    ASSOCIATION_NODE_LIST * pListOfNodes)
{
  uint8_t i;
  uint8_t numberOfNodes;

  numberOfNodes = ((0 == pListOfNodes->noOfNodes) ? 1 : pListOfNodes->noOfNodes);

  // Remove all Node ID Associations in the given list.
  for (i = 0; i < numberOfNodes; i++)
  {
    for (indx = 0; indx < MAX_ASSOCIATION_IN_GROUP; indx++)
    {
      if ((FALSE == groups[ep][groupId-1].subGrp[indx].nodeInfo.BitMultiChannelEncap) &&
          ((0 == pListOfNodes->noOfNodes && 0 == pListOfNodes->noOfMulchanNodes) || groups[ep][groupId-1].subGrp[indx].node.nodeId == pListOfNodes->pNodeId[i]))
      {
        memset((uint8_t*)&groups[ep][groupId-1].subGrp[indx], 0, sizeof(MULTICHAN_NODE_ID));
        if (0 < pListOfNodes->noOfNodes)
        {
          ReorderGroupAfterRemove(groupId-1, ep, indx);
          break;
        }
      }
    }
  }

  if (COMMAND_CLASS_ASSOCIATION == cmdClass)
  {
    return;
  }

  numberOfNodes = ((0 == pListOfNodes->noOfMulchanNodes) ? 1 : pListOfNodes->noOfMulchanNodes);

  // Remove all Endpoint Node ID Associations in the given list.
  for (i = 0; i < numberOfNodes; i++)
  {
    for (indx = 0; indx < MAX_ASSOCIATION_IN_GROUP; indx++)
    {
      if ((TRUE == groups[ep][groupId-1].subGrp[indx].nodeInfo.BitMultiChannelEncap) &&
          ((0 == pListOfNodes->noOfNodes && 0 == pListOfNodes->noOfMulchanNodes) ||
              ((groups[ep][groupId-1].subGrp[indx].node.nodeId == pListOfNodes->pMulChanNodeId[i].nodeId) &&
                  (groups[ep][groupId-1].subGrp[indx].node.endpoint == pListOfNodes->pMulChanNodeId[i].endpoint) &&
                  (groups[ep][groupId-1].subGrp[indx].node.BitAddress == pListOfNodes->pMulChanNodeId[i].BitAddress))))
      {
        memset((uint8_t*)&groups[ep][groupId-1].subGrp[indx], 0, sizeof(MULTICHAN_NODE_ID));
        if (0 < pListOfNodes->noOfMulchanNodes)
        {
          ReorderGroupAfterRemove(groupId-1, ep, indx);
          break;
        }
      }
    }
  }
}

/**
 * @brief Reads/Writes association data to the NVM.
 * @param action The action to take.
 */
static void
NVM_Action(NVM_ACTION action)
{
  BYTE i,j,k;
  BYTE transportCap;

  ZW_DEBUG_ASSOCIATION_SEND_NL();
  ZW_DEBUG_ASSOCIATION_SEND_STR("NVM_Action groups");
  if(READ_DATA == action){    ZW_DEBUG_ASSOCIATION_SEND_STR(" read ");}
  else{ ZW_DEBUG_ASSOCIATION_SEND_STR(" write ");}


  for(i = 0; i < MAX_ASSOCIATION_GROUPS; i++)
  {
    ZW_DEBUG_ASSOCIATION_SEND_NL();
    ZW_DEBUG_ASSOCIATION_SEND_STR("groupeId ");
    ZW_DEBUG_ASSOCIATION_SEND_NUM(i + 1);
    ZW_DEBUG_ASSOCIATION_SEND_NL();
    for(j = 0; j < NUMBER_OF_ENDPOINTS + 1; j++)
    {
      ZW_DEBUG_ASSOCIATION_SEND_STR("[ep");
      ZW_DEBUG_ASSOCIATION_SEND_NUM(j);
      ZW_DEBUG_ASSOCIATION_SEND_STR("] ");
      for(k = 0; k < MAX_ASSOCIATION_IN_GROUP; k++)
      {
        if(READ_DATA == action)
        {
          //MemoryGetByte((WORD)&nvmApplDescriptor.EEOFFSET_ASSOCIATION_MAGIC_far)
          groups[j][i].subGrp[k].node.nodeId =   MemoryGetByte((WORD)&EEOFFSET_ASSOCIATION_START_far[MAX_ASSOCIATION_IN_GROUP*(NUMBER_OF_ENDPOINTS + 1)*i + MAX_ASSOCIATION_IN_GROUP*j + k ]);
          {
            BYTE ep = MemoryGetByte((WORD)&EEOFFSET_ASSOCIATION_ENDPOINT_START_far[MAX_ASSOCIATION_IN_GROUP*(NUMBER_OF_ENDPOINTS + 1)*i + MAX_ASSOCIATION_IN_GROUP*j + k ]);
            groups[j][i].subGrp[k].node.endpoint = 0x7f & ep;
            groups[j][i].subGrp[k].node.BitAddress = ((0x80 & ep) == 0) ? 0 : 1;
          }

          transportCap = MemoryGetByte((WORD)&EEOFFSET_TRANSPORT_CAPABILITIES_START_far[MAX_ASSOCIATION_IN_GROUP*(NUMBER_OF_ENDPOINTS + 1)*i + MAX_ASSOCIATION_IN_GROUP*j + k ]);
          groups[j][i].subGrp[k].nodeInfo.BitMultiChannelEncap = (*((EEOFFSET_TRANSPORT_CAPABILITIES_STRUCT*)&transportCap)).BitMultiChannelEncap;
          groups[j][i].subGrp[k].nodeInfo.security =  (*((EEOFFSET_TRANSPORT_CAPABILITIES_STRUCT*)&transportCap)).security;
          ZW_DEBUG_ASSOCIATION_SEND_NUM( groups[j][i].subGrp[k].node.nodeId );
          ZW_DEBUG_ASSOCIATION_SEND_BYTE('.');
          ZW_DEBUG_ASSOCIATION_SEND_NUM( groups[j][i].subGrp[k].node.endpoint );
          ZW_DEBUG_ASSOCIATION_SEND_BYTE(' ');
        }
        else
        {
          MemoryPutByte((WORD)&EEOFFSET_ASSOCIATION_START_far[MAX_ASSOCIATION_IN_GROUP*(NUMBER_OF_ENDPOINTS + 1)*i + MAX_ASSOCIATION_IN_GROUP*j + k ],
                        groups[j][i].subGrp[k].node.nodeId);
          {
            BYTE node = groups[j][i].subGrp[k].node.endpoint | (groups[j][i].subGrp[k].node.BitAddress << 7);
            MemoryPutByte((WORD)&EEOFFSET_ASSOCIATION_ENDPOINT_START_far[MAX_ASSOCIATION_IN_GROUP*(NUMBER_OF_ENDPOINTS + 1)*i + MAX_ASSOCIATION_IN_GROUP*j + k ],
                        node);
          }
          /**
           * To be independent of nodeInfo bit-structure is each bit-group saved separately. This cost in NVM size!!!
           */
          transportCap = 0;
          (*((EEOFFSET_TRANSPORT_CAPABILITIES_STRUCT*)&transportCap)).BitMultiChannelEncap = groups[j][i].subGrp[k].nodeInfo.BitMultiChannelEncap;
          (*((EEOFFSET_TRANSPORT_CAPABILITIES_STRUCT*)&transportCap)).security             = groups[j][i].subGrp[k].nodeInfo.security;

          MemoryPutByte((WORD)&EEOFFSET_TRANSPORT_CAPABILITIES_START_far[MAX_ASSOCIATION_IN_GROUP*(NUMBER_OF_ENDPOINTS + 1)*i + MAX_ASSOCIATION_IN_GROUP*j + k ], transportCap);

          ZW_DEBUG_ASSOCIATION_SEND_NUM( groups[j][i].subGrp[k].node.nodeId );
          ZW_DEBUG_ASSOCIATION_SEND_BYTE('.');
          ZW_DEBUG_ASSOCIATION_SEND_NUM( groups[j][i].subGrp[k].node.endpoint );
          ZW_DEBUG_ASSOCIATION_SEND_BYTE(' ');
        }
      }
      ZW_DEBUG_ASSOCIATION_SEND_NL();
    }
  }
}

/**
 * @brief Stores all associations in the NVM.
 */
static void
AssociationStoreAll(void)
{
  NVM_Action(WRITE_DATA);
}

/**
 * @brief Clears all associations from the NVM.
 */
static void
AssociationClearAll(void)
{
  MemoryPutBuffer((WORD)&EEOFFSET_ASSOCIATION_START_far,
                  NULL, // Write zeros.
                  MAX_ASSOCIATION_GROUPS * (NUMBER_OF_ENDPOINTS + 1) * MAX_ASSOCIATION_IN_GROUP,
                  NULL);
  MemoryPutByte((WORD)&EEOFFSET_ASSOCIATION_MAGIC_far, MAGIC_VALUE); /* Now ASSOCIATION should be OK */
  MemoryPutByte((WORD)&EEOFFSET_ASSOCIATION_ENDPOINT_MAGIC_far, MAGIC_VALUE); /* Now ASSOCIATION EP should be OK */
  MemoryPutByte((WORD)&EEOFFSET_MAGIC_SDK_6_70_ASSOCIATION_SECURE_far, MAGIC_VALUE);/* Now ASSOCIATION secure level should be OK */
  AssociationEndpointClearAll();
}

/**
 * @brief Clears all endpoint association related data from the NVM.
 */
static void
AssociationEndpointClearAll(void)
{
  MemoryPutBuffer((WORD)&EEOFFSET_ASSOCIATION_ENDPOINT_START_far,
                  NULL,
                  MAX_ASSOCIATION_GROUPS*(NUMBER_OF_ENDPOINTS + 1)*MAX_ASSOCIATION_IN_GROUP,
                  NULL);
  MemoryPutBuffer((WORD)&EEOFFSET_TRANSPORT_CAPABILITIES_START_far,
                  NULL,
                  MAX_ASSOCIATION_GROUPS*(NUMBER_OF_ENDPOINTS + 1)*MAX_ASSOCIATION_IN_GROUP,
                  NULL);
  MemoryPutByte((WORD)&EEOFFSET_ASSOCIATION_ENDPOINT_MAGIC_far, MAGIC_VALUE); /* Now ASSOCIATION EP should be OK */
}

void
AssociationInit(BOOL forceClearMem)
{
  if ((MemoryGetByte((WORD)&EEOFFSET_ASSOCIATION_MAGIC_far) != MAGIC_VALUE) ||
      (TRUE == forceClearMem))
  {
    /* Clear it */
    AssociationClearAll();
  }
  else if ((MemoryGetByte((WORD)&EEOFFSET_ASSOCIATION_ENDPOINT_MAGIC_far) != MAGIC_VALUE) ||
      (TRUE == forceClearMem))
  {
    /* Clear it */
    AssociationEndpointClearAll();
  }
  NVM_Action(READ_DATA);
}

BOOL handleAssociationSet(
    BYTE ep,
    ZW_MULTI_CHANNEL_ASSOCIATION_SET_1BYTE_V2_FRAME* pCmd,
    BYTE cmdLength)
{
  BYTE i = 0;
  ASSOCIATION_NODE_LIST list;
  BYTE multiChannelAssociation = FALSE;

  // Set up lifeline for root device only.
  if (((NUMBER_OF_ENDPOINTS + 1) < ep || ((1 == pCmd->groupingIdentifier) && (0 < ep) )) ||
     (0 == pCmd->groupingIdentifier) || (CC_AGI_groupCount_handler(ep) < pCmd->groupingIdentifier))
  {
    return FALSE;
  }

  ExtractCmdClassNodeList(&list, pCmd, cmdLength, &multiChannelAssociation);

  if ((list.noOfNodes + list.noOfMulchanNodes) > handleGetMaxNodesInGroup(pCmd->groupingIdentifier, ep))
  {
    return FALSE;
  }

  for(i = 0; i < list.noOfNodes; i++)
  {
    MULTICHAN_DEST_NODE_ID node;
    node.nodeId = *(list.pNodeId + i);
    node.endpoint = 0;
    node.BitAddress = 0;
    if(FALSE == AssociationAddNode( pCmd->groupingIdentifier, ep, &node, FALSE ))
    {
      return FALSE;
    }
  }

  for(i = 0; i < list.noOfMulchanNodes; i++)
  {
    if(FALSE == AssociationAddNode( pCmd->groupingIdentifier, ep, &list.pMulChanNodeId[i], multiChannelAssociation ))
    {
      return FALSE;
    }
  }
  AssociationStoreAll();
  return TRUE;
}

/**
 * @brief Extracts nodes and endpoints from a given (Multi Channel) Association frame.
 * @param[out] plist
 * @param[in] pCmd
 * @param[in] cmdLength
 * @param[out] pMultiChannelAssociation
 */
static void
ExtractCmdClassNodeList(
    ASSOCIATION_NODE_LIST* plist,
    ZW_MULTI_CHANNEL_ASSOCIATION_SET_1BYTE_V2_FRAME* pCmd,
    BYTE cmdLength,
    BYTE* pMultiChannelAssociation)
{
  uint8_t i = 0;
  uint8_t * pNodeId = NULL;

  plist->pNodeId = &pCmd->nodeId1;
  plist->noOfNodes  = 0;
  plist->pMulChanNodeId = NULL;
  plist->noOfMulchanNodes = 0;

  if (3 >= cmdLength)
  {
    /*
     * If the length is less than or equal to three, it means that it's a get or a remove. In the
     * first case we shouldn't end up here. In the second case, we must return, since the remove
     * command should remove all nodes.
     */
    plist->pNodeId = NULL;
    return;
  }

  cmdLength -= OFFSET_PARAM_2; /*calc length on node-Id's*/

  for (i = 0; i < cmdLength; i++)
  {
    pNodeId = plist->pNodeId + i;
    if (MULTI_CHANNEL_ASSOCIATION_SET_MARKER_V2 == *pNodeId)
    {
      if (MULTI_CHANNEL_ASSOCIATION_SET_MARKER_V2 == *(pNodeId + 1))
      {
        /* 2 MARKERS!! error en incomming frame!*/
        return;
      }
      plist->noOfMulchanNodes = (cmdLength - (i + 1)) / 2;
      if (0 != plist->noOfMulchanNodes)
      {
        if (NON_NULL(pMultiChannelAssociation))
        {
          *pMultiChannelAssociation = TRUE;
        }
        plist->pMulChanNodeId = (MULTICHAN_DEST_NODE_ID*)(pNodeId + 1); /*Point after the marker*/
        i = cmdLength;
      }
    }
    else
    {
      plist->noOfNodes = i + 1;
    }
  }
}

void
AssociationInitEndpointSupport(
  BOOL forceClearMem,
  ASSOCIATION_ROOT_GROUP_MAPPING* pMapping,
  BYTE nbrGrp)
{
  pGroupMappingTable = pMapping;
  numberOfGroupMappingEntries = nbrGrp;
  AssociationInit(forceClearMem);
}

/**
 * @brief Searches through the group mapping table and updates the variables given by the pointers.
 * @param[in/out] pEndpoint Pointer to an endpoint.
 * @param[in/out] pGroupID Pointer to a group ID.
 * @return TRUE if the mapping was found, FALSE otherwise.
 */
static BOOL
AssGroupMappingLookUp(
    BYTE* pEndpoint,
    BYTE* pGroupID)
{
  if ((0 == *pEndpoint) && (1 != *pGroupID) && NON_NULL(pGroupMappingTable))
  {
    /*Use Group mapping to get endpoint proupIden*/
    BYTE i;
    for (i = 0; i < numberOfGroupMappingEntries; i++)
    {
      if (*pGroupID == pGroupMappingTable[i].rootGrpId)
      {
        *pEndpoint = pGroupMappingTable[i].endpoint;
        *pGroupID = pGroupMappingTable[i].endpointGrpId;
        return TRUE;
      }
    }
  }
  return FALSE;
}

void
AssociationGet(
    uint8_t endpoint,
    uint8_t * incomingFrame,
    uint8_t * outgoingFrame,
    uint8_t * outgoingFrameLength)
{
  uint8_t nodeCount;
  uint8_t nodeCountMax;
  uint8_t nodeCountNoEndpoint;
  uint8_t nodeFieldCount;
  uint8_t mappedEndpoint;
  uint8_t mappedGroupID;
  MULTICHAN_NODE_ID * pCurrentNode;

  if ((*(incomingFrame + 2) > CC_AGI_groupCount_handler(endpoint)) || (0 == *(incomingFrame + 2)))
  {
    // If the group is invalid, we return group 1
    *(incomingFrame + 2) = 1;
  }

  mappedEndpoint = endpoint;
  mappedGroupID = *(incomingFrame + 2);
  if (0 == endpoint)
  {
    AssGroupMappingLookUp(&mappedEndpoint, &mappedGroupID);
  }

  nodeCountMax = handleGetMaxNodesInGroup(*(incomingFrame + 2), endpoint);

  *outgoingFrame = *incomingFrame; // Set the command class.

  *(outgoingFrame + 2) = *(incomingFrame + 2); // The group
  *(outgoingFrame + 3) = nodeCountMax;
  *(outgoingFrame + 4) = 0; // Number of reports to follow.

  // Add node IDs without endpoints if any.
  nodeCountNoEndpoint = 0;
  for (nodeCount = 0; nodeCount < nodeCountMax; nodeCount++)
  {
    pCurrentNode = &(groups[mappedEndpoint][mappedGroupID - 1].subGrp[nodeCount]);
    if (FALSE == pCurrentNode->nodeInfo.BitMultiChannelEncap &&
        0 < pCurrentNode->node.nodeId)
    {
      // No endpoints in the association
      *(outgoingFrame + 5 + nodeCountNoEndpoint) = groups[mappedEndpoint][mappedGroupID - 1].subGrp[nodeCount].node.nodeId;
      nodeCountNoEndpoint++;
    }
  }

  switch (*(incomingFrame)) // Check command class.
  {
  case COMMAND_CLASS_ASSOCIATION:
    *(outgoingFrame + 1) = ASSOCIATION_REPORT_V2; // The response command.
    break;
  case COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V3:
    *(outgoingFrame + 1) = MULTI_CHANNEL_ASSOCIATION_REPORT_V3; // The response command.

    // Add endpoint nodes if any.
    nodeFieldCount = 0;
    for (nodeCount = 0; nodeCount < nodeCountMax; nodeCount++)
    {
      pCurrentNode = &(groups[mappedEndpoint][mappedGroupID - 1].subGrp[nodeCount]);
      if (TRUE == pCurrentNode->nodeInfo.BitMultiChannelEncap &&
          0 < pCurrentNode->node.nodeId)
      {
        // The association contains endpoints.
        *(outgoingFrame + 6 + nodeCountNoEndpoint + nodeFieldCount++) = *((uint8_t *)pCurrentNode);
        *(outgoingFrame + 6 + nodeCountNoEndpoint + nodeFieldCount++) = *(((uint8_t *)pCurrentNode) + 1);
      }
    }

    if (nodeFieldCount)
    {
      *(outgoingFrame + 5 + nodeCountNoEndpoint) = MULTI_CHANNEL_ASSOCIATION_REPORT_MARKER_V3;
      *outgoingFrameLength = sizeof(ZW_MULTI_CHANNEL_ASSOCIATION_REPORT_1BYTE_V3_FRAME) - 3 + nodeCountNoEndpoint + nodeFieldCount;

      // We return if there are endpoint associations.
      return;
    }
    break;
  default:
    // We should never get here, but if we do it means that we got an invalid command class.
    // Set the length to zero.
    *outgoingFrameLength = 0;
    break;
  }

  // If there are no endpoint associations we end up here.
  *outgoingFrameLength = sizeof(ZW_ASSOCIATION_REPORT_1BYTE_FRAME) - 1 + nodeCountNoEndpoint;
}

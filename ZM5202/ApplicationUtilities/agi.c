/**
 * @file
 * Helper module for Command Class Association Group Information.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include "config_app.h"
#include <ZW_mem_api.h>
#include <ZW_string.h>
#include <association_plus.h>
#include <agi.h>
#include <misc.h>
#include <ZW_uart_api.h>
#include <CommandClassAssociationGroupInfo.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

#ifdef ZW_DEBUG_AGI
#define ZW_DEBUG_AGI_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_AGI_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_AGI_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_AGI_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_AGI_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_AGI_SEND_BYTE(data)
#define ZW_DEBUG_AGI_SEND_STR(STR)
#define ZW_DEBUG_AGI_SEND_NUM(data)
#define ZW_DEBUG_AGI_SEND_WORD_NUM(data)
#define ZW_DEBUG_AGI_SEND_NL()
#endif

#define AGI_STRING_LEN 42

typedef struct _AGI_LIFELINE_
{
  char grpName[AGI_STRING_LEN];
  CMD_CLASS_GRP* pCmdGrpList;
  uint8_t listSize;
} AGI_LIFELINE;

typedef struct _AGI_TABLE_EP_
{
  AGI_GROUP* pTable;
  uint8_t tableSize;
} AGI_TABLE_EP;

typedef struct _AGI_TABLE_
{
  AGI_LIFELINE lifeLineEndpoint[NUMBER_OF_ENDPOINTS + 1];
  AGI_TABLE_EP tableEndpoint[NUMBER_OF_ENDPOINTS + 1];
} AGI_TABLE;

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

static AGI_TABLE myAgi;
static uint8_t m_lastActiveGroupId = 1;

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

// Nothing here.

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

uint8_t i;

void
CC_AGI_Init(void)
{
  const char lifelineText[] = "Lifeline";

  m_lastActiveGroupId = 1;

  memset((uint8_t *)&myAgi, 0x00, sizeof(myAgi));
  for (i = NUMBER_OF_ENDPOINTS + 1; i > 0; i--)
  {
    memcpy(
           (uint8_t *)myAgi.lifeLineEndpoint[i - 1].grpName,
           (uint8_t *)lifelineText,
           ZW_strlen((uint8_t *)lifelineText));
  }
}

void CC_AGI_LifeLineGroupSetup(
    cc_group_t const * const pCmdGrpList,
    uint8_t listSize,
    uint8_t endpoint)
{
  if (NUMBER_OF_ENDPOINTS < endpoint)
  {
    return; // Invalid endpoint => return.
  }

  myAgi.lifeLineEndpoint[endpoint].pCmdGrpList = (cc_group_t *)pCmdGrpList;
  myAgi.lifeLineEndpoint[endpoint].listSize = listSize;
}

void CC_AGI_ResourceGroupSetup(AGI_GROUP const * const pTable, uint8_t tableSize, uint8_t endpoint)
{
  if (IS_NULL(pTable))
  {
    tableSize = 0;
  }

  if (NUMBER_OF_ENDPOINTS >= endpoint)
  {
    myAgi.tableEndpoint[endpoint].tableSize = tableSize;
    myAgi.tableEndpoint[endpoint].pTable = (AGI_GROUP *)pTable;
  }
}

uint8_t
CC_AGI_groupNameGet_handler(
                 char * pGroupName,
                 uint8_t groupId,
                 uint8_t endpoint)
{
  uint8_t nameLength = 0;
  char * pSourceGroupName;
  const char errorText[] = "Invalid group";

  if ((IS_NULL(pGroupName)) || (NUMBER_OF_ENDPOINTS < endpoint))
  {
    return nameLength;
  }

  // tableSize does not include Lifeline. Hence, +1.
  if ((groupId > (myAgi.tableEndpoint[endpoint].tableSize + 1)) || (0 == groupId))
  {
    pSourceGroupName = (char *)errorText;
  }
  else
  {
    if (1 == groupId)
    {
      pSourceGroupName = myAgi.lifeLineEndpoint[endpoint].grpName;
    }
    else
    {
      /*
       * myAgi.tableEndpoint[endpoint].pTable represents all groups not being lifeline groups.
       * Hence, the lowest possible group is 2 since lifeline is group 1.
       * the first index in array = given group - 2.
       */
      pSourceGroupName = myAgi.tableEndpoint[endpoint].pTable[groupId - 2].groupName;
    }
  }
  nameLength = ZW_strlen((uint8_t *)pSourceGroupName);
  memcpy(
         (uint8_t * )pGroupName,
         (uint8_t * )pSourceGroupName,
         nameLength);

  return nameLength;
}

uint8_t CC_AGI_groupCount_handler(uint8_t endpoint)
{
  if (NUMBER_OF_ENDPOINTS < endpoint)
  {
    return 0; /** Error!!*/
  }
  return 1 + myAgi.tableEndpoint[endpoint].tableSize; /* Lifeline group + grouptable size.*/
}

void CC_AGI_groupInfoGet_handler(
    uint8_t groupId,
    uint8_t endpoint,
    VG_ASSOCIATION_GROUP_INFO_REPORT_VG* report)
{
  ZAF_CC_AGI_CorrectGroupIdIfInvalid(endpoint, &groupId);

  report->groupingIdentifier = groupId;

  if (1 == groupId) {
    report->profile1 = ASSOCIATION_GROUP_INFO_REPORT_PROFILE_GENERAL; /* MSB */
    report->profile2 = ASSOCIATION_GROUP_INFO_REPORT_PROFILE_GENERAL_LIFELINE; /* LSB */
  } else {
    report->profile1 = myAgi.tableEndpoint[endpoint].pTable[groupId - 2].profile.profile_MS;
    report->profile2 = myAgi.tableEndpoint[endpoint].pTable[groupId - 2].profile.profile_LS;
  }
}

void GetApplGroupCommandList(
                             uint8_t * pGroupList,
                             uint8_t groupId,
                             uint8_t endpoint)
{
  if (NUMBER_OF_ENDPOINTS < endpoint)
  {
    return; /** Error!!*/
  }

  if (1 == groupId)
  {
    memcpy(pGroupList, (BYTE_P )myAgi.lifeLineEndpoint[endpoint].pCmdGrpList,
           myAgi.lifeLineEndpoint[endpoint].listSize * sizeof(CMD_CLASS_GRP));
  }
  else
  {
    memcpy(pGroupList, (BYTE_P )&myAgi.tableEndpoint[endpoint].pTable[groupId - 2].cmdGrp,
           sizeof(CMD_CLASS_GRP));
  }
}

uint8_t GetApplGroupCommandListSize(
                                    uint8_t groupId,
                                    uint8_t endpoint)
{
  uint8_t size = 0;

  if (NUMBER_OF_ENDPOINTS < endpoint)
  {
    return 0; /** Error!!*/
  }

  if (1 == groupId)
  {
    size = myAgi.lifeLineEndpoint[endpoint].listSize * sizeof(CMD_CLASS_GRP);
  }
  else
  {
    size = sizeof(CMD_CLASS_GRP);
  }
  return size;
}

uint8_t
ApplicationGetLastActiveGroupId(void)
{
  return m_lastActiveGroupId;
}

/**
 * SearchCmdClass
 */
BOOL
SearchCmdClass(CMD_CLASS_GRP cmdGrp, CMD_CLASS_GRP* pCmdGrpList, uint8_t listSize)
{
  for (; listSize > 0; listSize--) {
    if (cmdGrp.cmdClass == pCmdGrpList[listSize - 1].cmdClass) {
      return TRUE;
    }
  }
  return FALSE;
}

/**
 * SearchCmdClass
 */
uint8_t
SearchProfile(AGI_PROFILE profile, CMD_CLASS_GRP cmdGrp, AGI_TABLE_EP* pAgiTable)
{
#define grpId i
  for (grpId = pAgiTable->tableSize; grpId > 0; grpId--)
  {
    /*Find profile*/
    if ((profile.profile_MS == pAgiTable->pTable[grpId - 1].profile.profile_MS) &&
        (profile.profile_LS == pAgiTable->pTable[grpId - 1].profile.profile_LS))
    {
      if ( TRUE == SearchCmdClass(cmdGrp, &pAgiTable->pTable[grpId - 1].cmdGrp, 1))
      {
        return grpId - 1;
      }
    }
  }
  return 0xFF;
#undef grpId
}

TRANSMIT_OPTIONS_TYPE_EX * ReqNodeList(
                                       AGI_PROFILE* pProfile,
                                       CMD_CLASS_GRP* pCurrentCmdGrp,
                                       uint8_t sourceEndpoint)
{
  static MULTICHAN_NODE_ID nodeListCombined[MAX_ASSOCIATION_IN_GROUP * 2]; /**< handle 2 node lists: lifeline + one list more */
  MULTICHAN_NODE_ID * pNodeList = &nodeListCombined[0];
  MULTICHAN_NODE_ID * pAssociationData = NULL;
  uint8_t associationLength;
  uint8_t grpId = 0;
  static TRANSMIT_OPTIONS_TYPE_EX reqTxOpt;
  reqTxOpt.sourceEndpoint = sourceEndpoint;
  reqTxOpt.txOptions = ZWAVE_PLUS_TX_OPTIONS;
  reqTxOpt.pList = &nodeListCombined[0];
  reqTxOpt.list_length = 0;

  if (NUMBER_OF_ENDPOINTS < sourceEndpoint)
  {
    return NULL; /** Error!!*/
  }

  // Try to find the command class and command pair in the Lifeline AGI
  if (TRUE == SearchCmdClass(*pCurrentCmdGrp,
                             myAgi.lifeLineEndpoint[sourceEndpoint].pCmdGrpList,
                             myAgi.lifeLineEndpoint[sourceEndpoint].listSize))
  {
    // If we found the pair, get a pointer to the association data and a length.

    // Lifeline is group 1 and endpoints have no lifeline. Hence, always fetch from endpoint 0.
    associationLength = 0;
    handleAssociationGetnodeList(1, 0, &pAssociationData, &associationLength);
    if (0 < associationLength) {
      // More than one association. Copy to the combined list.
      memcpy((uint8_t *)pNodeList, (uint8_t *)pAssociationData, associationLength * sizeof(MULTICHAN_NODE_ID));

      // Update the TX options
      reqTxOpt.list_length += associationLength;

      // Update the combined list pointer
      pNodeList += associationLength; // Increase by <associationLength> x items

      m_lastActiveGroupId = 1;
    }
  }

  grpId = SearchProfile(*pProfile, *pCurrentCmdGrp, &myAgi.tableEndpoint[sourceEndpoint]);

  if (0xFF != grpId)
  {
    // Found the group that matches profile and the command class/command pair.

    // Fetch associations from that group. Re-use pAssociationData & associationLength.
    associationLength = 0;
    handleAssociationGetnodeList(grpId + 2,
                                 sourceEndpoint,
                                 &pAssociationData,
                                 &associationLength);
    if (0 < associationLength) {
      // More than one association. Copy to the combined list.
      memcpy((uint8_t *)pNodeList, (uint8_t *)pAssociationData, associationLength * sizeof(MULTICHAN_NODE_ID));

      // Update the TX options
      reqTxOpt.list_length += associationLength;

      m_lastActiveGroupId = grpId + 2;
    }
  }

  if (0 < reqTxOpt.list_length) {
    reqTxOpt.S2_groupID = m_lastActiveGroupId + (sourceEndpoint << 4);
    return &reqTxOpt;
  }
  return NULL;
}

void ZAF_CC_AGI_CorrectGroupIdIfInvalid(uint8_t endpoint, uint8_t * pGroupId)
{
  if ((0 == *pGroupId) || (*pGroupId >= myAgi.tableEndpoint[endpoint].tableSize + 2))
  {
    *pGroupId = 1;
  }
}

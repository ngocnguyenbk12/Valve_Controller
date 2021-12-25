/***************************************************************************
*
* Copyright (c) 2001-2013
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: Implements functions for transporting frames over the
*              native Z-Wave Network
*
* Author: Thomas Roll
*
* Last Changed By: $Author: tro $
* Revision: $Revision: 0.00 $
* Last Changed: $Date: 2013/08/26 10:47:09 $
*
****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include <ZW_TransportSecProtocol.h>
#include <ZW_cmd_class_list.h>
#include <ZW_transport_api.h>
#include <ZW_mem_api.h>
#include <ZW_uart_api.h>
#include "config_app.h"
#include <ZW_tx_mutex.h>
#include <misc.h>
#ifndef NON_BATT
#include <ZAF_pm.h>
#endif

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
#ifdef ZW_DEBUG_SECURITY
#define ZW_DEBUG_SECURITY_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_SECURITY_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_SECURITY_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_SECURITY_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_SECURITY_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_SECURITY_SEND_BYTE(data)
#define ZW_DEBUG_SECURITY_SEND_STR(STR)
#define ZW_DEBUG_SECURITY_SEND_NUM(data)
#define ZW_DEBUG_SECURITY_SEND_WORD_NUM(data)
#define ZW_DEBUG_SECURITY_SEND_NL()
#endif


#ifdef ZW_DEBUG_NATIVE
#define ZW_DEBUG_NATIVE_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_NATIVE_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_NATIVE_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_NATIVE_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_NATIVE_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_NATIVE_SEND_BYTE(data)
#define ZW_DEBUG_NATIVE_SEND_STR(STR)
#define ZW_DEBUG_NATIVE_SEND_NUM(data)
#define ZW_DEBUG_NATIVE_SEND_WORD_NUM(data)
#define ZW_DEBUG_NATIVE_SEND_NL()
#endif


#ifndef SEC_2_POWERDOWNTIMEOUT
#define SEC_2_POWERDOWNTIMEOUT 20
#endif

#ifndef MSEC_200_POWERDOWNTIMEOUT
#define MSEC_200_POWERDOWNTIMEOUT 2
#endif

#ifndef SEC_10_POWERDOWNTIMEOUT
#define SEC_10_POWERDOWNTIMEOUT 100
#endif

typedef struct _ST_TRANSPORT_NODE_INFORMATION_
{
  union
  {
    APP_NODE_INFORMATION *pNifs;
    CMD_CLASS_LIST_3_LIST *pCmdLists;
  }u;
  CMD_CLASS_LIST activeNonsecureList;
} TRANSPORT_NODE_INFORMATION;


/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/
#ifdef ZW_CONTROLLER
BYTE SECURITY_KEY = 0;
#endif


static TRANSPORT_NODE_INFORMATION m_AppInfo =
{
  {(APP_NODE_INFORMATION*)NULL},
  {(BYTE*)NULL, 0} /**> CMD_CLASS_LIST data*/
};


#define TSEC_CMD_HANDLING_DEFAULT   0
#define TSEC_CMD_HANDLING_UNSECURE  1
#define TSEC_CMD_HANDLING_SECURE    2
#define TSEC_CMD_HANDLING_SECURE2   3

#define INVALID_NODE_ID 0xFF

static XBYTE cmd_class_buffer[APPL_NODEPARM_MAX];

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/
static void SetupActiveNIF(BYTE nodeId);

/**
 * See description for function prototype in ZW_basis_api.h.
 */
void
ApplicationNodeInformation(
  BYTE *deviceOptionsMask,
  APPL_NODE_TYPE *nodeType,
  BYTE **nodeParm,
  BYTE *parmLength)
{
  CMD_CLASS_LIST* pCmdClassList = GetCommandClassList((0 != GetNodeId()), SECURITY_KEY_NONE, 0);


  *deviceOptionsMask  = m_AppInfo.u.pNifs->deviceOptionsMask;  //APPLICATION_NODEINFO_LISTENING | APPLICATION_NODEINFO_OPTIONAL_FUNCTIONALITY;
  nodeType->generic   = m_AppInfo.u.pNifs->nodeType.generic;  /* Generic device class */
  nodeType->specific  = m_AppInfo.u.pNifs->nodeType.specific; /* Specific device class */
  *nodeParm = pCmdClassList->pList;
  *parmLength = pCmdClassList->size;
}

BOOL Transport_OnApplicationInitHW(uint8_t bStatus)
{
  UNUSED(bStatus);
  return TRUE;
}

BYTE
Transport_OnApplicationInitSW(
  APP_NODE_INFORMATION* pAppNode)
{
  m_AppInfo.u.pNifs = pAppNode;

  SetupActiveNIF(GetNodeId());

  ZW_DEBUG_SECURITY_SEND_STR("SecAppInit ");
  ZW_DEBUG_SECURITY_SEND_NL();

#ifndef SERIAL_API_APP
  mutex_init();
#endif

  return TRUE;
}

enum SECURITY_KEY
GetHighestSecureLevel(BYTE protocolSecBits)
{
  if(SECURITY_KEY_S2_ACCESS_BIT & protocolSecBits)
  {
    return SECURITY_KEY_S2_ACCESS;
  }
  else if(SECURITY_KEY_S2_AUTHENTICATED_BIT & protocolSecBits)
  {
    return SECURITY_KEY_S2_AUTHENTICATED;
  }
  else if(SECURITY_KEY_S2_UNAUTHENTICATED_BIT & protocolSecBits)
  {
    return SECURITY_KEY_S2_UNAUTHENTICATED;
  }
  else if(SECURITY_KEY_S0_BIT & protocolSecBits)
  {
    return SECURITY_KEY_S0;
  }

  return SECURITY_KEY_NONE;
}



CMD_CLASS_LIST*
GetCommandClassList(
    BOOL included,
    security_key_t eKey,
    BYTE endpoint)
{
  static CMD_CLASS_LIST cmd_class_list;
  static CMD_CLASS_LIST* pCmdClassList = NULL;
  ZW_DEBUG_SECURITY_SEND_STR("\r\nCommandsSuppported(");
  ZW_DEBUG_SECURITY_SEND_NUM(included);
  ZW_DEBUG_SECURITY_SEND_NUM(eKey);
  ZW_DEBUG_SECURITY_SEND_NUM(endpoint);
  ZW_DEBUG_SECURITY_SEND_STR(")\r\n");
  if (TRUE == included)
  {

    if (SECURITY_KEY_NONE == eKey)
    {
      BYTE keys = ZW_GetSecurityKeys();

      /*Check non secure command class list*/
      if(0 == endpoint)
      {
        if(SECURITY_KEY_NONE_MASK == keys)
        {
          /*Non-secure included, non-secure cmd class list*/
          pCmdClassList =  &(m_AppInfo.u.pCmdLists->unsecList);
        }
        else{
          /*secure included, non-secure cmd class list*/
          pCmdClassList =  &(m_AppInfo.u.pCmdLists->sec.unsecList);
        }
      }
#ifndef SERIAL_API_APP

      else
      {
        pCmdClassList = GetEndpointcmdClassList(FALSE, endpoint);
      }
#endif
      //Copy cmd class list into buffer and only add active CC_SECURITY + CC_SECURITY_2
      /*
      cmd_class_list.pList = cmd_class_buffer;
      cmd_class_list.size = 0;


      for(i = 0; i < pCmdClassList->size; i++)
      {
        if( ( (COMMAND_CLASS_SECURITY != *(pCmdClassList->pList + i)) &&
              (COMMAND_CLASS_SECURITY_2 != *(pCmdClassList->pList + i))
            ) ||
            (
              (((COMMAND_CLASS_SECURITY == *(pCmdClassList->pList + i)) && (SECURITY_KEY_S0_BIT & keys)) ||
              ((COMMAND_CLASS_SECURITY_2 == *(pCmdClassList->pList + i)) && (SECURITY_KEY_S2_MASK & keys)))
            )
          )
        {
          //Only add security CC if device support it
          cmd_class_buffer[cmd_class_list.size] = *(pCmdClassList->pList + i);
          cmd_class_list.size++;
        }
      }
      pCmdClassList = &cmd_class_list;
      */
    }
    else
    {
      /*Check secure command class list*/

      /*If eKey not is supported, return NULL pointer!!*/
      if(eKey == GetHighestSecureLevel( ZW_GetSecurityKeys()) )
      {
        if(0 == endpoint)
        {
          pCmdClassList = &(m_AppInfo.u.pCmdLists->sec.secList);
        }
  #ifndef SERIAL_API_APP
        else
        {
          pCmdClassList = GetEndpointcmdClassList(TRUE, endpoint);
        }
  #endif
        /*Remove marker and commands*/
        if( (SECURITY_KEY_S2_UNAUTHENTICATED == eKey) ||
            (SECURITY_KEY_S2_AUTHENTICATED == eKey) ||
            (SECURITY_KEY_S2_ACCESS == eKey)
          )
        {
          cmd_class_list.pList = cmd_class_buffer;
          cmd_class_list.size = 0;
          while(((BYTE)*(pCmdClassList->pList + cmd_class_list.size) != COMMAND_CLASS_MARK) &&
                (cmd_class_list.size < pCmdClassList->size))
          {
            cmd_class_buffer[cmd_class_list.size] = *(pCmdClassList->pList + cmd_class_list.size);
            cmd_class_list.size++;
          }
          pCmdClassList = &cmd_class_list;
        }
      }
      else
      {
        /*not included. Deliver empty list*/
        cmd_class_list.pList = NULL;
        cmd_class_list.size = 0;
        pCmdClassList = &cmd_class_list;
      }
    }
  }
  else
  {
    /*Not included!*/
    if(0 == endpoint)
    {
      if (SECURITY_KEY_NONE == eKey)
      {
        pCmdClassList = &(m_AppInfo.u.pCmdLists->unsecList);
      }
      else
      {
        /*not included. Deliver empty list*/
        cmd_class_list.pList = NULL;
        cmd_class_list.size = 0;
        pCmdClassList = &cmd_class_list;
      }
    }
#ifndef SERIAL_API_APP
    else
    {
      pCmdClassList = GetEndpointcmdClassList(FALSE, endpoint);
    }
#endif
  }

  return pCmdClassList;
}


void
ApplicationSecureCommandsSupported(
    enum SECURITY_KEY eKey, /* IN Security Scheme to report on */
    BYTE **pCmdClasses,   /* OUT Cmd classes supported by endpoint */
    BYTE *pLength)        /* OUT Length of pCmdClasses, 0 if endpoint does not exist */
{
  CMD_CLASS_LIST *pCmdClassList  = NULL;
  pCmdClassList = GetCommandClassList((0 != GetNodeId()), eKey, 0);
  *pCmdClasses = pCmdClassList->pList;
  *pLength = pCmdClassList->size;
}


void
Transport_SetDefault()
{
}


BYTE GetNodeId(void)
{
  BYTE nodeId;
  MemoryGetID(NULL, &nodeId);
  return nodeId;
}


uint8_t Transport_OnLearnCompleted(uint8_t nodeID)
{
  SetupActiveNIF(nodeID);
#ifndef NON_BATT
  ZAF_pm_KeepAwakeAuto();
#endif

  return TRUE;
}


void
ApplicationCommandHandler(
  ZW_APPLICATION_TX_BUFFER *pCmd, /* IN Payload from the received frame, */
                                  /*    the command is the very first byte */
  BYTE cmdLength,                 /* IN Number of command bytes including the command */
  RECEIVE_OPTIONS_TYPE *rxOpt)    /* rxOpt struct to use (may be partially filled out if
                                     parsing encapsulated command */
{
  ZW_DEBUG_SECURITY_SEND_STR("\r\nAppCmdH ");
  ZW_DEBUG_SECURITY_SEND_BYTE(' ');
  ZW_DEBUG_SECURITY_SEND_NUM(rxOpt->securityKey);
  ZW_DEBUG_SECURITY_SEND_BYTE(' ');
  ZW_DEBUG_SECURITY_SEND_NUM(pCmd->ZW_Common.cmdClass);
  ZW_DEBUG_SECURITY_SEND_NUM(pCmd->ZW_Common.cmd);
  ZW_DEBUG_SECURITY_SEND_STR("\r\nm_AppInfo size");
  ZW_DEBUG_SECURITY_SEND_NUM(m_AppInfo.u.pNifs->cmdClassListSecureCount);
  ZW_DEBUG_SECURITY_SEND_NUM(m_AppInfo.activeNonsecureList.size);
  ZW_DEBUG_SECURITY_SEND_NL();

#ifndef ACCEPT_ALL_CMD_CLASSES
  /* Check if cmd Class are supported in current mode (unsecure or secure) */
  if (TRUE == CmdClassSupported(rxOpt->securityKey,
                                pCmd->ZW_Common.cmdClass,
                                pCmd->ZW_Common.cmd,
                                m_AppInfo.u.pNifs->cmdClassListSecure,
                                m_AppInfo.u.pNifs->cmdClassListSecureCount,
                                m_AppInfo.activeNonsecureList.pList,
                                m_AppInfo.activeNonsecureList.size))
#endif /* ACCEPT_ALL_CMD_CLASSES */
  {
		Transport_ApplicationCommandHandler(pCmd, cmdLength, rxOpt);
#ifndef NON_BATT
		ZAF_pm_KeepAwakeAuto();
#endif
  }
#ifndef ACCEPT_ALL_CMD_CLASSES
  else
  {
    ZW_DEBUG_SECURITY_SEND_NL();
    ZW_DEBUG_SECURITY_SEND_STR("CmdCl not supported :(");
    ZW_DEBUG_SECURITY_SEND_NL();
  }
#endif /* ACCEPT_ALL_CMD_CLASSES */
}


BOOL
TransportCmdClassSupported(uint8_t commandClass,
                           uint8_t command,
                           enum SECURITY_KEY eKey)
{
  return CmdClassSupported(eKey,
                           commandClass,
                           command,
                           m_AppInfo.u.pNifs->cmdClassListSecure,
                           m_AppInfo.u.pNifs->cmdClassListSecureCount,
                           m_AppInfo.activeNonsecureList.pList,
                           m_AppInfo.activeNonsecureList.size);
}

/**
 * @brief Sets up the active NIF.
 * @param nodeId
 */
static void
SetupActiveNIF(BYTE nodeId)
{
  if(0 == nodeId || (SECURITY_KEY_NONE == GetHighestSecureLevel(ZW_GetSecurityKeys())) )
  {
    m_AppInfo.activeNonsecureList = m_AppInfo.u.pCmdLists->unsecList;
  }
  else
  {
    m_AppInfo.activeNonsecureList = m_AppInfo.u.pCmdLists->sec.unsecList;
  }
}

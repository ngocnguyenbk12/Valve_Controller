/***************************************************************************
*
* Copyright (c) 2001-2013
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: functionality to check if a cmdClass is present in NodeInfo.
*
* Author: Thomas Roll
*
* Last Changed By: $Author: tro $
* Revision: $Revision: 0.00 $
* Last Changed: $Date: 2013/05/14 15:28:37 $
*
****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include <ZW_classcmd.h>
#include <ZW_cmd_class_list.h>
#include <ZW_TransportSecProtocol.h>


/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
#ifdef ZW_DEBUG_CMDCLIST
#include <ZW_uart_api.h>
#define ZW_DEBUG_CMDCLIST_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_CMDCLIST_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_CMDCLIST_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_CMDCLIST_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_CMDCLIST_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_CMDCLIST_SEND_BYTE(data)
#define ZW_DEBUG_CMDCLIST_SEND_STR(STR)
#define ZW_DEBUG_CMDCLIST_SEND_NUM(data)
#define ZW_DEBUG_CMDCLIST_SEND_WORD_NUM(data)
#define ZW_DEBUG_CMDCLIST_SEND_NL()
#endif


/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/
//#define TSEC_CMD_HANDLING_SECURE    1

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

BOOL
CheckCmdClass(BYTE cmdClass,
              BYTE *pList,
              BYTE len)
{
  BYTE i;
  for( i = 0; i < len; i++)
  {
    if(*(pList+i) == cmdClass)
    {
      ZW_DEBUG_CMDCLIST_SEND_STR("CheckCmdC success ");
      ZW_DEBUG_CMDCLIST_SEND_NUM(cmdClass);
      ZW_DEBUG_CMDCLIST_SEND_NL();
      return TRUE;
    }
  }
  ZW_DEBUG_CMDCLIST_SEND_STR("CheckCmdClass fail cmd: ");
  ZW_DEBUG_CMDCLIST_SEND_NUM(cmdClass);
  ZW_DEBUG_CMDCLIST_SEND_NUM(len);
  ZW_DEBUG_CMDCLIST_SEND_NL();
  return FALSE;
}

BOOL
CmdClassSupported(security_key_t eKey,
                  BYTE commandClass,
                  BYTE command,
                  BYTE* pSecurelist,
                  BYTE securelistLen,
                  BYTE* pNonSecurelist,
                  BYTE nonSecurelistLen)
{
  security_key_t device_higest_secure_level = GetHighestSecureLevel(ZW_GetSecurityKeys());
  /*Only allow secure command list if node is securely included.*/
  /*Check if cmd Class are supported in current mode (non-secure or secure)*/
  if((SECURITY_KEY_NONE != device_higest_secure_level) && (eKey == device_higest_secure_level) && NON_NULL( pSecurelist ))
  {
    if ((COMMAND_CLASS_BASIC == commandClass) ||
        (TRUE == CheckCmdClass(commandClass, pSecurelist, securelistLen)))
    {
      return TRUE; /*cmd is supported!*/
    }
  }

  if ((SECURITY_KEY_S0 == device_higest_secure_level) &&     /* Security S0 */
      (SECURITY_KEY_NONE == eKey) &&                         /* non-secure input */
      (COMMAND_CLASS_MANUFACTURER_SPECIFIC == commandClass) &&
      (MANUFACTURER_SPECIFIC_GET == command) &&
      NON_NULL( pSecurelist ))                               /* Securely included */
  {
    return TRUE; /*cmd is supported!*/
  }

  if ((SECURITY_KEY_NONE == device_higest_secure_level) && (commandClass == COMMAND_CLASS_BASIC))
  {
    /* Non-secure node always support CC Basic. */
    return TRUE;
  }

  /*Check cmdClass is in non-secure list*/
  if(NON_NULL( pNonSecurelist ))
  {
    return CheckCmdClass(commandClass, pNonSecurelist, nonSecurelistLen);
  }
  return FALSE; /* Cmd not in list*/
}

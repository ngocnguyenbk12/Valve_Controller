/***************************************************************************
*
* Copyright (c) 2001-2011
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: Basic Command Class source file
*
* Author:
*
* Last Changed By:  $Author:  $
* Revision:         $Revision:  $
* Last Changed:     $Date:  $
*
****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_basis_api.h>
#include <ZW_tx_mutex.h>
#include <ZW_TransportLayer.h>
#include <ZW_TransportEndpoint.h>
#include "config_app.h"
#include <CommandClassBasic.h>
#include <CommandClass.h>
#include "misc.h"
#include <ZW_uart_api.h>
#include <agi.h>
#include <ZW_typedefs.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

#ifdef ZW_DEBUG_CC_BASIC_CONTROLLER
#define ZW_DEBUG__SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG__SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG__SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG__SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG__SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG__SEND_BYTE(data)
#define ZW_DEBUG__SEND_STR(STR)
#define ZW_DEBUG__SEND_NUM(data)
#define ZW_DEBUG__SEND_WORD_NUM(data)
#define ZW_DEBUG__SEND_NL()
#endif

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/


/*================= CmdClassBasicSetSend =======================
** Function description
** This function...
**
** Side effects:
**
**-------------------------------------------------------------------------*/
JOB_STATUS
CmdClassBasicSetSend(
  AGI_PROFILE* pProfile,
  BYTE sourceEndpoint,
  BYTE bValue,
  VOID_CALLBACKFUNC(pCbFunc)(TRANSMISSION_RESULT * pTransmissionResult))
{
  CMD_CLASS_GRP cmdGrp = {COMMAND_CLASS_BASIC, BASIC_SET};

  return cc_engine_multicast_request(
      pProfile,
      sourceEndpoint,
      &cmdGrp,
      &bValue,
      1,
      TRUE,
      pCbFunc);
}

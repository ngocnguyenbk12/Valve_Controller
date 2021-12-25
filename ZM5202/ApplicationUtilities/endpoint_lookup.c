/***************************************************************************
*
* Copyright (c) 2001-2011
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: Multilevel switch utilites source file
*
* Author: Samer Seoud
*
* Last Changed By:  $Author:  $
* Revision:         $Revision:  $
* Last Changed:     $Date:  $
*
****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include "config_app.h"
#include "endpoint_lookup.h"
#include <ZW_basis_api.h>
#include <ZW_uart_api.h>



/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
#ifdef ZW_DEBUG_MULTISWITCH
#define ZW_DEBUG_MULTISWITCH_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_MULTISWITCH_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_MULTISWITCH_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_MULTISWITCH_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_MULTISWITCH_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_MULTISWITCH_SEND_BYTE(data)
#define ZW_DEBUG_MULTISWITCH_SEND_STR(STR)
#define ZW_DEBUG_MULTISWITCH_SEND_NUM(data)
#define ZW_DEBUG_MULTISWITCH_SEND_WORD_NUM(data)
#define ZW_DEBUG_MULTISWITCH_SEND_NL()
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


BYTE
FindEndPointID(ENDPOINT_LOOKUP *pEPLookup, BYTE endpointindex)
{
  if (endpointindex < pEPLookup->bEndPointsCount)
  {
    return pEPLookup->pEndPointList[endpointindex];
  }
  return 0xFF;
}

BYTE GetEndPointCount(ENDPOINT_LOOKUP *pEPLookup)
{
  return  pEPLookup->bEndPointsCount;
}

BYTE
FindEndPointIndex(ENDPOINT_LOOKUP *pEPLookup, BYTE endpoint)
{
  BYTE i;
  if (endpoint) /*0 endpoint is mapped for the first end point*/
  {
    for (i = 0; i < pEPLookup->bEndPointsCount; i++)
    {
      if (pEPLookup->pEndPointList[i] == endpoint)
        return i;
    }
    return 0xFF;
  }
  return 0;
}

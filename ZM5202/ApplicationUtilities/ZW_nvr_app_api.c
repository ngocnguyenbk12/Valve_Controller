/***************************************************************************
*
* Copyright (c) 2001-2013
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: Some nice descriptive description.
*
* Author: Thomas Roll
*
* Last Changed By: $Author: tro $
* Revision: $Revision: 0.00 $
* Last Changed: $Date: 2013/09/16 16:00:29 $
*
****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include <ZW_flash_api.h>
#include <ZW_nvr_api.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
#ifdef ZW_DEBUG_NVR_APP
#include <ZW_uart_api.h>
#define ZW_DEBUG_NVR_APP_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_NVR_APP_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_NVR_APP_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_NVR_APP_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_NVR_APP_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_NVR_APP_SEND_BYTE(data)
#define ZW_DEBUG_NVR_APP_SEND_STR(STR)
#define ZW_DEBUG_NVR_APP_SEND_NUM(data)
#define ZW_DEBUG_NVR_APP_SEND_WORD_NUM(data)
#define ZW_DEBUG_NVR_APP_SEND_NL()
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

void
ZW_NVRGetAppValue(BYTE bOffset, BYTE bLength, BYTE *bRetBuffer)
{
  ZW_DEBUG_NVR_APP_SEND_BYTE('N');
  ZW_DEBUG_NVR_APP_SEND_BYTE('g');
  ZW_DEBUG_NVR_APP_SEND_NUM(NVR_APP_START_ADDRESS + bOffset);
  ZW_DEBUG_NVR_APP_SEND_BYTE(',');
  while (bLength)
  {
    /* NVR is valid, get value from NVR */
    *bRetBuffer = ZW_FLASH_nvr0_get(bOffset + NVR_APP_START_ADDRESS);

    ZW_DEBUG_NVR_APP_SEND_NUM(*bRetBuffer);

    bRetBuffer++;
    bLength--;
    bOffset++;
  }
  ZW_DEBUG_NVR_APP_SEND_NL();
}

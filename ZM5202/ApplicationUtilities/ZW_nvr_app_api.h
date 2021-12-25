/**
 * @file
 * API to NVR application area
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _ZW_nvr_app_api_H_
#define _ZW_nvr_app_api_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/


/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

/**
 * Data structure for NVR application area.
 * Max size is (NVR_APP_END_ADDRESS - NVR_APP_START_ADDRESS)
 */
typedef struct _NVR_APP_FLASH_STRUCT_
{
  BYTE hwVersion; /* product hardware version*/

} NVR_APP_FLASH_STRUCT;

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/


/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/


/**
 * @brief ZW_NVRGetAppValue
 *    Get a value from the NVR flash page application area. There is no CRC
 *    protection of NVR application area!
 *
 * @param bOffset is the offset in NVR area. Offset 0 is NVR_APP_START_ADDRESS
 *        address ends with = (NVR_APP_END_ADDRESS - NVR_APP_START_ADDRESS).
 * @param bLength is the length of data to be read.
 * @param bRetBuffer pointer to return data.
 */
void
ZW_NVRGetAppValue(BYTE bOffset, BYTE bLength, BYTE *bRetBuffer);



#endif /* _ZW_nvr_app_api_H_ */


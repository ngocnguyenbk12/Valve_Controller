/**
 * @file
 * Handler for Command Class Manufacturer Specific.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _COMMANDCLASSMANUFACTURERSPECIFIC_H_
#define _COMMANDCLASSMANUFACTURERSPECIFIC_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include <ZW_typedefs.h>
#include <ZW_classcmd.h>
#include <ZW_TransportEndpoint.h>

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

/**
 * Returns the version of this CC.
 */
#define CommandClassManufacturerVersionGet() MANUFACTURER_SPECIFIC_VERSION_V2

#define MAN_DEVICE_ID_SIZE (8)

/**
 * Command class manufacturer specific device Id type
 */
typedef enum _DEVICE_ID_TYPE_
{
  DEVICE_ID_TYPE_OEM = 0,
  DEVICE_ID_TYPE_SERIAL_NBR,
  DEVICE_ID_TYPE_PSEUDO_RANDOM
}
DEVICE_ID_TYPE;

/**
 * Command class manufacturer specific device Id format
 */
typedef enum _DEVICE_ID_FORMAT_
{
  DEVICE_ID_FORMAT_UTF_8 = 0,
  DEVICE_ID_FORMAT_BIN
}
DEVICE_ID_FORMAT;


/**
 * Command class manufacturer specific device Id data
 */
typedef struct _DEV_ID_DATA
{
  BYTE DevIdDataFormat: 3; /**< Type DEVICE_ID_FORMAT */
  BYTE DevIdDataLen: 5;
  BYTE* pDevIdData;
}
DEV_ID_DATA;

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

// Nothing here.

/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

/**
 * @brief Handler for the Manufacturer Specific command class.
 * @param[in] rxOpt receive options of type RECEIVE_OPTIONS_TYPE_EX
 * @param[in] pCmd Payload from the received frame, the union should be used to access
 * the fields.
 * @param[in] cmdLength Number of command bytes including the command.
 * @return receive frame status.
 */
received_frame_status_t handleCommandClassManufacturerSpecific(
    RECEIVE_OPTIONS_TYPE_EX *rxOpt,
    ZW_APPLICATION_TX_BUFFER *pCmd,
    BYTE cmdLength);

/**
 * @brief Read the Device specifict ID Data fields.
 * @param[in] deviceIdType values for the Device ID Type of enum type DEVICE_ID_TYPE
 * @param[out] pDevIdDataFormat pointer to data format of type DEVICE_ID_FORMAT
 * @param[out] pDevIdDataLen  pointer returning len of the Device ID Data fields.
 * @param[out] pDevIdData pointer to the Device ID Data fields.
 * @return boolean
 */
extern void ApplDeviceSpecificInfoGet(
    DEVICE_ID_TYPE *deviceIdType,
    DEVICE_ID_FORMAT* pDevIdDataFormat,
    BYTE* pDevIdDataLen,
    BYTE* pDevIdData);

/**
 * @brief Read the manufacturer specific unique serial number from NVR if the number from NVR all
 * 0xFF then create a random one.
 */
void ManufacturerSpecificDeviceIDInit(void);

#endif /* _COMMANDCLASSMANUFACTURERSPECIFIC_H_ */

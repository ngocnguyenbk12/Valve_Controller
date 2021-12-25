/**
 * @file
 * Declaration of Z-Wave Product IDs.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _PRODUCT_ID_ENUM_H_
#define _PRODUCT_ID_ENUM_H_

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/**
 * Standard enum Product type Id
 */
typedef enum _PRODUCT_TYPE_ID_ENUM_
{
  PRODUCT_TYPE_ID_ZWAVE_ZIP_GATEWAY = 1,
  PRODUCT_TYPE_ID_ZWAVE,
  PRODUCT_TYPE_ID_ZWAVE_PLUS
} eProductTypeID;


/**
 * Standard enum Product Plus Id's
 */
typedef enum _PRODUCT_PLUS_ID_ENUM_
{
  PRODUCT_ID_DoorLockKeyPad = 0x0001,
  PRODUCT_ID_SwitchOnOff = 0x0002,
  PRODUCT_ID_SensorPIR = 0x0003,
  PRODUCT_ID_InclusionController = 0x0004,
  PRODUCT_ID_MyProductPlus = 0x0005,
  PRODUCT_ID_SecureSensorPIR =  0x0006,
  PRODUCT_ID_SecureSwitchOnOff = 0x0007,
  PRODUCT_ID_SerialAPIPlus = 0x0008,
  PRODUCT_ID_ProdTestDUT = 0x0009,
  PRODUCT_ID_ProdTestGen = 0x000A,
  PRODUCT_ID_PowerStrip = 0x000B,
  PRODUCT_ID_WallController = 0x000C,
  PRODUCT_ID_ZIRC = 0x21ac
} eProductPlusID;

#endif /* _PRODUCT_ID_ENUM_H_ */

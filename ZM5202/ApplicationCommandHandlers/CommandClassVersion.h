/**
 * @file
 * Handler for Command Class Version.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _COMMAND_CLASS_VERSION_H_
#define _COMMAND_CLASS_VERSION_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include <ZW_sysdefs.h>
#include <ZW_pindefs.h>
#include <ZW_evaldefs.h>
#include <ZW_classcmd.h>
#include <CommandClass.h>
#include <ZW_TransportEndpoint.h>

/****************************************************************************/
/*                          TYPES and DEFINITIONS                           */
/****************************************************************************/

/**
 * Returns the version of this CC.
 */
#define CC_Version_getVersion() VERSION_VERSION_V3

/**
 * For backwards compatibility.
 */
#define CommandClassVersionVersionGet()     CC_Version_getVersion()
#define handleCommandClassVersionAppl(a)    CC_Version_getCommandClassVersion_handler(a)
#define handleCommandClassVersion(a, b, c)  CC_Version_handler(a, b, c)
#define handleNbrFirmwareVersions()         CC_Version_getNumberOfFirmwareTargets_handler()
#define handleGetFirmwareVersion(a, b)      CC_Version_GetFirmwareVersion_handler(a, b)
#define handleGetFirmwareHwVersion()        CC_Version_GetHardwareVersion()

typedef struct
{
  uint8_t major;
  uint8_t minor;
  uint8_t patch;
  uint16_t build_number;
}
version_info_t;

/****************************************************************************/
/*                                FUNCTIONS                                 */
/****************************************************************************/

/**
 * @brief Read command class version
 * @param cmdClass command class
 * @return version
 */
extern uint8_t CC_Version_getCommandClassVersion_handler(uint8_t cmdClass);

/**
 * @brief Handler for call command class modules version functions
 * @param[in] rxOpt pointer to rx options
 * @param[in] pCmd pointer to command
 * @param[in] cmdLength length of command
 * @return receive frame status.
 */
received_frame_status_t CC_Version_handler(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  uint8_t cmdLength);

/**
 * @brief Return number (N) of firmware versions.
 * @return N
 */
extern uint8_t CC_Version_getNumberOfFirmwareTargets_handler(void);

/**
 * Get firmware version of given firmware target index.
 *
 * @param[in] firmwareTargetIndex The number of firmware target [0;N-1] where N is the max number
 * of available firmware targets.
 * @param[out] pVariantgroup returns pointer to application version group number n.
 */
extern void CC_Version_GetFirmwareVersion_handler(
    uint8_t firmwareTargetIndex,
    VG_VERSION_REPORT_V2_VG* pVariantgroup);

/**
 * @brief The Hardware Version field MUST report a value which is unique to this particular
 * version of the product. It MUST be possible to uniquely determine the hardware
 * characteristics from the Hardware Version field in combination with the Manufacturer
 * ID, Product Type ID and Product ID fields of Manufacturer Specific Info Report
 * of the Manufacturer Specific Command Class.
 * This information allows a user to pick a firmware image version that is guaranteed
 * to work with this particular version of the product.
 * Note that the Hardware Version field is intended for the hardware version of the
 * entire product, not just the version of the Z-Wave radio chip
 * @return Hardware version
 */
uint8_t CC_Version_GetHardwareVersion(void);

/**
 * Sets the host interface version and build number.
 *
 * The host interface could be a Serial API application. Hence, this function is only relevant
 * in two-chip products.
 *
 * @param major Major version
 * @param minor Minor version
 * @param patch Patch version
 * @param build_number Build number
 */
void CC_Version_SetHostInterfaceVersionInfo(
    uint8_t major,
    uint8_t minor,
    uint8_t patch,
    uint16_t build_number);

/**
 * Sets the application version and build number.
 *
 * @param major Major version
 * @param minor Minor version
 * @param patch Patch version
 * @param build_number Build number
 */
void CC_Version_SetApplicationVersionInfo(
    uint8_t major,
    uint8_t minor,
    uint8_t patch,
    uint16_t build_number);

#endif /*_COMMAND_CLASS_VERSION_H_*/

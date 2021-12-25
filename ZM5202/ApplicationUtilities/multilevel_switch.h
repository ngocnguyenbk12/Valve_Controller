/**
 * @file
 * Multilevel switch helper module.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _MULTILEVEL_SWITCH_H_
#define _MULTILEVEL_SWITCH_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include <ZW_sysdefs.h>
#include <ZW_evaldefs.h>


/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/
#define ENDPOINT_NOT_VALID   0xFF


/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/


/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/


/**
 * @brief GetSwitchHWLevel
 *  Read the switch actual HW level
 * @param[in] endpoint is the destination endpoint
 * @return The current device HW level.
 *
 */
BYTE GetSwitchHWLevel( BYTE endpoint );

/**
 * @brief StopSwitchDimming
 * Stop the ongoing switch dimming if it running
 * @param[in] endpoint is the destination endpoint
 */
void StopSwitchDimming( BYTE endpoint);


/**
 * @brief HandleStartChangeCmd
 *  Handle the multilevel switch start change command
 * @param[in] bStartLevel IN the the switch level the dimming should start from.
 * @param[in] boIgnoreStartLvl IN TRUE if bStartLevel should be ignored FALSE if bStartLevel should be used.
 * @param[in] boDimUp IN TRUE if switch level should be incremented, FALSE if the level should be decremented.
 * @param[in] bDimmingDuration IN the time it should take the switch to transit from 0 to 99
 * The encoding of the value is as follow:
 * 0x00  Instantly
 * 0x01-0x7F 1 second (0x01) to 127 seconds (0x7F) in 1-second resolution.
 * 0x80-0xFE 1 minute (0x80) to 127 minutes (0xFE) in 1-minute resolution.
 * 0xFF Factory default duration.
 * @param endpoint is the destination endpoint
 */
void HandleStartChangeCmd( BYTE bStartLevel,
                     BOOL boIgnoreStartLvl,
                     BOOL boDimUp,
                     BYTE bDimmingDuration,
                     BYTE endpoint );

/**
 * @brief HandleSetCmd
 *  Handle the multilevel switch set command
 * @param[in] bTargetlevel IN the wanted switch level.
 * @param[in] bDuration IN the time it should take the switch to go from current level to bTargetlevel
 * The encoding of the value is as follow:
 * 0x00  Instantly
 * 0x01-0x7F 1 second (0x01) to 127 seconds (0x7F) in 1-second resolution.
 * 0x80-0xFE 1 minute (0x80) to 127 minutes (0xFE) in 1-minute resolution.
 * 0xFF Factory default duration.
 * @param[in] endpoint is the destination endpoint
 */
void CC_MultilevelSwitch_SetValue(uint8_t bTargetlevel, uint8_t bDuration, uint8_t endpoint);

/**
 * @brief SetSwitchHwLevel
 * Set the intial a multilevel switch hw level
 * The number of endpoints that can be initailsed are defiend by the constant SWITCH_MULTI_ENDPOINTS
 * @param[in]  bInitHwLevel IN a multilevel switch initiale HW level value.
 * @param[in]  endpoint IN multilevel switch endpoint ID, Endpoint 0 is invalid.
 * @return TRUE if the endpoint initalised correctly, FALSE if the endpoint not initalised
 */
BOOL SetSwitchHwLevel( BYTE bInitHwLevel,BYTE endpoint );

/**
 * @brief Initializes the Multilevel Switch command class by telling it which endpoints are
 * capable of handling Multilevel Switch commands.
 *
 * This function must be called once by the application.
 * @param[in] bEndPointCount Number of multilevel switch endpoints.
 * @param[in] pEndPointList Pointer to a list of endpoints.
 */
void MultiLevelSwitchInit(uint8_t bEndPointCount, uint8_t const * const pEndPointList);

/**
 * @brief GetTargetLevel
 * returns the target level of the ongoing or the most recent transision
 * @param[in] endpoint number of multilevel switch endpoints
 */
BYTE
GetTargetLevel(BYTE endpoint);

/**
 * @brief GetTargetLevel
 * Returns the duration the HW takes to reach the target level from the current level
 * The duration is zero if the target level was reached
 * @param[in] endpoint number of multilevel switch endpoints
 * @return the duration encoded as fellow 0x01-0x7F 1 second (0x01) to 127 seconds (0x7F) in 1 second resolution.
 *         0x80-0xFD 1 minute (0x80) to 126 minutes (0xFD) in 1 minute resolution.
 *         0xFE Unknown duration.
 */
BYTE
GetCurrentDuration(BYTE endpoint);

/**
 * For backwards compatiblity.
 */
#define HandleSetCmd(a, b, c) CC_MultilevelSwitch_SetValue(a, b, c)
#define ZCB_CommandClassMultiLevelSwitchSupportSet(a, b, c) CC_MultilevelSwitch_SetValue(a, b, c)

#endif /*_MULTILEVEL_SWITCH_H_*/


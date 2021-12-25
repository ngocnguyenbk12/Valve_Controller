/**
 * @file config_app.h
 * @brief Configuration file for Power Strip sample application.
 * @copyright Copyright (c) 2001-2017
 * Sigma Designs, Inc.
 * All Rights Reserved
 * @details This file contains definitions for the Z-Wave+ Framework as well for the sample app.
 *
 * NOTICE: The file name must not be changed and the two definitions APP_VERSION and APP_REVISION
 * must not be changed since they are used by the build environment.
 */
#ifndef _CONFIG_APP_H_
#define _CONFIG_APP_H_

#ifdef __C51__
#include <ZW_product_id_enum.h>
#include <commandClassManufacturerSpecific.h>
#include <agi.h>
#endif
#include <app_config_common.h>

/**
 * Defines device generic and specific types
 */
//@ [GENERIC_TYPE_ID]
#define GENERIC_TYPE  GENERIC_TYPE_SWITCH_BINARY
#define SPECIFIC_TYPE SPECIFIC_TYPE_VALVE_OPEN_CLOSE
//@ [GENERIC_TYPE_ID]

/**
 * See ZW_basic_api.h for ApplicationNodeInformation field deviceOptionMask
 */
//@ [DEVICE_OPTIONS_MASK_ID]
#define DEVICE_OPTIONS_MASK   APPLICATION_NODEINFO_LISTENING | APPLICATION_NODEINFO_OPTIONAL_FUNCTIONALITY
//@ [DEVICE_OPTIONS_MASK_ID]

/**
 * Defines used to initialize the Z-Wave Plus Info Command Class.
 */
 
 //icon: display on type column of pc controller
//@ [APP_TYPE_ID]
#define APP_ROLE_TYPE 		ZWAVEPLUS_INFO_REPORT_ROLE_TYPE_SLAVE_ALWAYS_ON
#define APP_NODE_TYPE 		ZWAVEPLUS_INFO_REPORT_NODE_TYPE_ZWAVEPLUS_NODE			

#define APP_ICON_TYPE 								ICON_TYPE_GENERIC_VALVE_OPEN_CLOSE
#define APP_USER_ICON_TYPE 						ICON_TYPE_GENERIC_VALVE_OPEN_CLOSE

#define ENDPOINT_ICONS \
 {ICON_TYPE_GENERIC_VALVE_OPEN_CLOSE, ICON_TYPE_GENERIC_VALVE_OPEN_CLOSE},	\
 {ICON_TYPE_GENERIC_VALVE_OPEN_CLOSE, ICON_TYPE_GENERIC_VALVE_OPEN_CLOSE}, \
 {ICON_TYPE_GENERIC_SENSOR_NOTIFICATION, ICON_TYPE_SPECIFIC_SENSOR_NOTIFICATION_WATER_ALARM}
//@ [APP_TYPE_ID]


/**
 * Defines used to initialize the Manufacturer Specific Command Class.
 */
#define APP_MANUFACTURER_ID     MFG_ID_SIGMA_DESIGNS	// 0x1510

#define APP_PRODUCT_TYPE_ID     PRODUCT_TYPE_ID_ZWAVE_PLUS
#define APP_PRODUCT_ID          PRODUCT_ID_SwitchOnOff

#define APP_FIRMWARE_ID         APP_PRODUCT_ID | (APP_PRODUCT_TYPE_ID << 8)

#define APP_DEVICE_ID_TYPE      DEVICE_ID_TYPE_PSEUDO_RANDOM
#define APP_DEVICE_ID_FORMAT    DEVICE_ID_FORMAT_BIN

/**
 * Defines used to initialize the Association Group Information (AGI)
 * Command Class.
 */
#define NUMBER_OF_INDIVIDUAL_ENDPOINTS    3
#define NUMBER_OF_AGGREGATED_ENDPOINTS    0
#define NUMBER_OF_ENDPOINTS         NUMBER_OF_INDIVIDUAL_ENDPOINTS + NUMBER_OF_AGGREGATED_ENDPOINTS
#define MAX_ASSOCIATION_GROUPS      4
#define MAX_ASSOCIATION_IN_GROUP    5

//@ [AGI_TABLE_ID]
#define AGITABLE_LIFELINE_GROUP \
	{COMMAND_CLASS_DEVICE_RESET_LOCALLY, DEVICE_RESET_LOCALLY_NOTIFICATION}, \
	{COMMAND_CLASS_BASIC, BASIC_REPORT},	\
	{COMMAND_CLASS_SWITCH_BINARY, SWITCH_BINARY_REPORT}
//	{COMMAND_CLASS_DEVICE_REPORT_LOCALLY, DEVICE_RESET_LOCALLY_NOTIFICATION}

#define AGITABLE_LIFELINE_GROUP_ENDPOINTS_VALVE	\
	{COMMAND_CLASS_BASIC, BASIC_REPORT},	\
	{COMMAND_CLASS_SWITCH_BINARY, SWITCH_BINARY_REPORT},\
	{COMMAND_CLASS_MULTI_CHANNEL_V3, MULTI_CHANNEL_CMD_ENCAP_V3}
	
	
#define AGITABLE_LIFELINE_GROUP_ENDPOINTS_SENSOR \
	{COMMAND_CLASS_BASIC, BASIC_REPORT},	\
	{COMMAND_CLASS_NOTIFICATION_V3, NOTIFICATION_REPORT_V3} 
	
#define ASSOCIATION_ROOT_GROUP_MAPPING_CONFIG \
  {ASS_GRP_ID_2, ENDPOINT_1, ASS_GRP_ID_2}, \
  {ASS_GRP_ID_3, ENDPOINT_2, ASS_GRP_ID_2},\
  {ASS_GRP_ID_4, ENDPOINT_3, ASS_GRP_ID_2}
	
	
	
#define  AGITABLE_ROOTDEVICE_GROUPS \
 {ASSOCIATION_GROUP_INFO_REPORT_PROFILE_NOTIFICATION, NOTIFICATION_REPORT_POWER_MANAGEMENT_V4, {COMMAND_CLASS_NOTIFICATION_V3, NOTIFICATION_REPORT_V3},"alarm EP 1"}, \
 {ASSOCIATION_GROUP_INFO_REPORT_PROFILE_NOTIFICATION, NOTIFICATION_REPORT_POWER_MANAGEMENT_V4, {COMMAND_CLASS_NOTIFICATION_V3, NOTIFICATION_REPORT_V3},"alarm EP 2"}, \
 {ASSOCIATION_GROUP_INFO_REPORT_PROFILE_NOTIFICATION, NOTIFICATION_REPORT_WATER_V3, {COMMAND_CLASS_NOTIFICATION_V3, NOTIFICATION_REPORT_V3},"alarm EP 3"}

#define  AGITABLE_ENDPOINT_1_GROUPS \
 {ASSOCIATION_GROUP_INFO_REPORT_PROFILE_NOTIFICATION, NOTIFICATION_REPORT_POWER_MANAGEMENT_V4, {COMMAND_CLASS_NOTIFICATION_V3, NOTIFICATION_REPORT_V3},"alarm EP 1"}
#define  AGITABLE_ENDPOINT_2_GROUPS \
 {ASSOCIATION_GROUP_INFO_REPORT_PROFILE_NOTIFICATION, NOTIFICATION_REPORT_POWER_MANAGEMENT_V4, {COMMAND_CLASS_NOTIFICATION_V3, NOTIFICATION_REPORT_V3},"alarm EP 2"}
#define  AGITABLE_ENDPOINT_3_GROUPS  \
 {ASSOCIATION_GROUP_INFO_REPORT_PROFILE_NOTIFICATION, NOTIFICATION_REPORT_WATER_V3, {COMMAND_CLASS_NOTIFICATION_V3, NOTIFICATION_REPORT_V3},"alarm EP 3"}

#define FIRMWARE_UPGRADABLE        0xFF  /**< 0x00 = Not upgradable, 0xFF = Upgradable*/

/**
 * Configuration for ApplicationUtilities/notification.h + .c
 */
#define MAX_NOTIFICATIONS 1
/**
 * configuration for ApplicationUtilities/multilevel_switch.h + .c
 */
//#define SWITCH_MULTI_ENDPOINTS  0


//@ [SECURITY_AUTHENTICATION_ID]
/*
 * This definition must be set in order for the application to handle CSA. It is used only in the
 * application.
 */
// #define APP_SUPPORTS_CLIENT_SIDE_AUTHENTICATION

/*
 * This definition tells the protocol whether the application uses CSA or not.
 * It can be set to one of the two following values:
 * - SECURITY_AUTHENTICATION_SSA
 * - SECURITY_AUTHENTICATION_CSA
 */
#define REQUESTED_SECURITY_AUTHENTICATION SECURITY_AUTHENTICATION_SSA
//@ [SECURITY_AUTHENTICATION_ID]

/**
 * Security keys
 */
//@ [REQUESTED_SECURITY_KEYS_ID]
#define REQUESTED_SECURITY_KEYS	SECURITY_KEY_NONE_MASK	// (SECURITY_KEY_S0_BIT | SECURITY_KEY_S2_UNAUTHENTICATED_BIT)
//@ [REQUESTED_SECURITY_KEYS_ID]

#endif /* _CONFIG_APP_H_ */

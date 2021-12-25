/**
 * @file SwitchOnOff.c
 * @copyright Copyright (c) 2001-2015
 * Sigma Designs, Inc.
 * All Rights Reserved�
 * @brief Z-Wave Switch On/Off Sample Application
 * @details This sample application is a Z-Wave slave node which has an LED (D2
 * on ZDP03A) that can be turned on or off from another Z-Wave node by sending
 * a Basic Set On or a Basic Set Off command.
 *
 * It can be included and excluded from a Z-Wave network by pressing S1 switch
 * on the ZDP03A board 3 times. S2 switch toggles LED D2. S3 switch transmits
 * a Node Information Frame (NIF).
 *
 * Last changed by: $Author: $
 * Revision:        $Revision: $
 * Last changed:    $Date: $
 *
 * @author Someone who started this sample application at some point in time.
 * @author Thomas Roll (TRO)
 * @author Christian Salmony Olsen (COLSEN)
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include "config_app.h"
#include <app_version.h>

#include <ZW_slave_api.h>
#ifdef ZW_SLAVE_32
#include <ZW_slave_32_api.h>
#else
#include <ZW_slave_routing_api.h>
#endif  /* ZW_SLAVE_32 */
#include <ZW_classcmd.h>
#include <ZW_mem_api.h>
#include <eeprom.h>
#include <ZW_uart_api.h>
#include <misc.h>
#ifdef BOOTLOADER_ENABLED
#include <ota_util.h>
#include <CommandClassFirmwareUpdate.h>
#endif
#include <nvm_util.h>
/*IO control*/
#include <io_zdp03a.h>
#include <ZW_task.h>
#include <ev_man.h>
#ifdef ZW_ISD51_DEBUG
#include "ISD51.h"
#endif
#include <association_plus.h>
#include <agi.h>
#include <CommandClass.h>
#include <CommandClassAssociation.h>
#include <CommandClassAssociationGroupInfo.h>
#include <CommandClassVersion.h>
#include <CommandClassZWavePlusInfo.h>
#include <CommandClassPowerLevel.h>
#include <CommandClassDeviceResetLocally.h>
#include <CommandClassBasic.h>
#include <CommandClassBinarySwitch.h>
#include <CommandClassSupervision.h>
#include <CommandClassMultiChan.h>
#include <CommandClassMultiChanAssociation.h>
#include <CommandClassNotification.h>
#include <zaf_version.h>
#include <gpio_driver.h>

#include <endpoint_lookup.h>
#include <conhandle.h>
#include <ZW_conbufio.h>
#include <ZW_basis_api.h>
#include <ZW_uart_api.h>
#include <ZW_SerialAPI.h>
#include <uart_command.h>
#include <ZW_spi_api.h>
#include <notification.h>
#include <CommandClassConfiguration.h>
#include <CommandClassAllSwitch.h>
#include <CommandClassMeter.h>

#include <key.h>
#include <interrupt_driver.h>
#include <Cat25256.h>



/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
/**
 * @def ZW_DEBUG_MYPRODUCT_SEND_BYTE(data)
 * Transmits a given byte to the debug port.
 * @def ZW_DEBUG_MYPRODUCT_SEND_STR(STR)
 * Transmits a given string to the debug port.
 * @def ZW_DEBUG_MYPRODUCT_SEND_NUM(data)
 * Transmits a given number to the debug port.
 * @def ZW_DEBUG_MYPRODUCT_SEND_WORD_NUM(data)
 * Transmits a given WORD number to the debug port.
 * @def ZW_DEBUG_MYPRODUCT_SEND_NL()
 * Transmits a newline to the debug port.
 */
#ifdef ZW_DEBUG_APP
#define ZW_DEBUG_MYPRODUCT_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_MYPRODUCT_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_MYPRODUCT_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_MYPRODUCT_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_MYPRODUCT_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_MYPRODUCT_SEND_BYTE(data)
#define ZW_DEBUG_MYPRODUCT_SEND_STR(STR)
#define ZW_DEBUG_MYPRODUCT_SEND_NUM(data)
#define ZW_DEBUG_MYPRODUCT_SEND_WORD_NUM(data)
#define ZW_DEBUG_MYPRODUCT_SEND_NL()
#endif

#define Sw_inex 0x11
#define led_in  0x04   // GPIO PIN 5
#define led_ex  0x36		// GPIO PIN 13
#define led_er 	0x10		// GPIO PIN 4

#define spi_cs 	0x37



/**
 * Application events for AppStateManager(..)
 */
typedef enum _EVENT_APP_
{
  EVENT_EMPTY = DEFINE_EVENT_APP_NBR,
  EVENT_APP_INIT,
  EVENT_APP_REFRESH_MMI,
  EVENT_APP_NEXT_EVENT_JOB,
  EVENT_APP_FINISH_EVENT_JOB,
  EVENT_APP_GET_NODELIST,
  EVENT_APP_SEND_OVERLOAD_NOTIFICATION,
  EVENT_APP_SMARTSTART_IN_PROGRESS,
	FACTORYNEW_RESET
} EVENT_APP;

/**
 * Application states. Function AppStateManager(..) includes the state
 * event machine.
 */
typedef enum _STATE_APP_
{
  STATE_APP_STARTUP,
  STATE_APP_IDLE,
  STATE_APP_LEARN_MODE,
  STATE_APP_WATCHDOG_RESET,
  STATE_APP_OTA,
  STATE_APP_TRANSMIT_DATA
} STATE_APP;


/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

/**
 * Unsecure node information list.
 * Be sure Command classes are not duplicated in both lists.
 * CHANGE THIS - Add all supported non-secure command classes here
 **/
static code BYTE cmdClassListNonSecureNotIncluded[] =
{
  COMMAND_CLASS_ZWAVEPLUS_INFO_V2,
  COMMAND_CLASS_SWITCH_BINARY,
	COMMAND_CLASS_CONFIGURATION,
  COMMAND_CLASS_ASSOCIATION,
  COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V3,
  COMMAND_CLASS_MULTI_CHANNEL_V4,
  COMMAND_CLASS_ASSOCIATION_GRP_INFO,
  COMMAND_CLASS_TRANSPORT_SERVICE_V2,
	COMMAND_CLASS_NOTIFICATION_V3,
  COMMAND_CLASS_VERSION_V2,
  COMMAND_CLASS_MANUFACTURER_SPECIFIC,
  COMMAND_CLASS_DEVICE_RESET_LOCALLY,
  COMMAND_CLASS_POWERLEVEL,
  COMMAND_CLASS_SECURITY,
  COMMAND_CLASS_SECURITY_2,
  COMMAND_CLASS_SUPERVISION,
	COMMAND_CLASS_CONFIGURATION,
	COMMAND_CLASS_SWITCH_ALL,
	COMMAND_CLASS_METER
#ifdef BOOTLOADER_ENABLED
  ,COMMAND_CLASS_FIRMWARE_UPDATE_MD_V2
#endif
};

/**
 * Unsecure node information list Secure included.
 * Be sure Command classes are not duplicated in both lists.
 * CHANGE THIS - Add all supported non-secure command classes here
 **/
static code BYTE cmdClassListNonSecureIncludedSecure[] =
{
  COMMAND_CLASS_ZWAVEPLUS_INFO,
  COMMAND_CLASS_SUPERVISION,
  COMMAND_CLASS_TRANSPORT_SERVICE_V2,
  COMMAND_CLASS_SECURITY,
  COMMAND_CLASS_SECURITY_2
};


/**
 * Secure node inforamtion list.
 * Be sure Command classes are not duplicated in both lists.
 * CHANGE THIS - Add all supported secure command classes here
 **/
static code BYTE cmdClassListSecure[] =
{
	COMMAND_CLASS_NOTIFICATION_V3,
  COMMAND_CLASS_VERSION,
  COMMAND_CLASS_SWITCH_BINARY,
  COMMAND_CLASS_ASSOCIATION,
  COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
  COMMAND_CLASS_ASSOCIATION_GRP_INFO,
  COMMAND_CLASS_MANUFACTURER_SPECIFIC,
  COMMAND_CLASS_DEVICE_RESET_LOCALLY,
  COMMAND_CLASS_MULTI_CHANNEL_V3,
	COMMAND_CLASS_CONFIGURATION,
	COMMAND_CLASS_SWITCH_ALL,
	COMMAND_CLASS_METER
#ifdef BOOTLOADER_ENABLED
  ,COMMAND_CLASS_FIRMWARE_UPDATE_MD_V2
#endif
};


/**
 * Structure includes application node information list's and device type.
 */
APP_NODE_INFORMATION m_AppNIF =
{
  cmdClassListNonSecureNotIncluded, sizeof(cmdClassListNonSecureNotIncluded),
  cmdClassListNonSecureIncludedSecure, sizeof(cmdClassListNonSecureIncludedSecure),
  cmdClassListSecure, sizeof(cmdClassListSecure),
  DEVICE_OPTIONS_MASK, GENERIC_TYPE, SPECIFIC_TYPE
};

static code BYTE cmdClassListNonSecureIncludedNonSecure_Endpoints[] =
{
  COMMAND_CLASS_ZWAVEPLUS_INFO,
  COMMAND_CLASS_SWITCH_BINARY,
	COMMAND_CLASS_BASIC,
  COMMAND_CLASS_ASSOCIATION,
  COMMAND_CLASS_ASSOCIATION_GRP_INFO,
  COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
	COMMAND_CLASS_MULTI_CHANNEL_V3,
	COMMAND_CLASS_CONFIGURATION,
	COMMAND_CLASS_SWITCH_ALL

};

static code BYTE cmdClassListNonSecureIncludedSecure_Endpoints[] =
{
  COMMAND_CLASS_ZWAVEPLUS_INFO,
  COMMAND_CLASS_SUPERVISION,
  COMMAND_CLASS_SECURITY,
  COMMAND_CLASS_SECURITY_2
};

static code BYTE cmdClassListSecure_Endpoints[] =
{
  COMMAND_CLASS_SWITCH_BINARY,
  COMMAND_CLASS_ASSOCIATION,
  COMMAND_CLASS_ASSOCIATION_GRP_INFO,
  COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
	COMMAND_CLASS_MULTI_CHANNEL_V3,
	COMMAND_CLASS_CONFIGURATION,
	COMMAND_CLASS_SWITCH_ALL
};

static code BYTE cmdClassListNonSecureIncludedNonSecure_Sensor[] =
{
	COMMAND_CLASS_NOTIFICATION_V3,
  COMMAND_CLASS_ZWAVEPLUS_INFO,
  COMMAND_CLASS_NOTIFICATION_V3,
	COMMAND_CLASS_SUPERVISION,
	COMMAND_CLASS_BASIC,
  COMMAND_CLASS_ASSOCIATION,
  COMMAND_CLASS_ASSOCIATION_GRP_INFO,
  COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
	COMMAND_CLASS_MULTI_CHANNEL_V3,
	COMMAND_CLASS_CONFIGURATION
};

static code BYTE cmdClassListNonSecureIncludedSecure_Sensor[] =
{
  COMMAND_CLASS_ZWAVEPLUS_INFO,
  COMMAND_CLASS_SUPERVISION,
  COMMAND_CLASS_SECURITY,
  COMMAND_CLASS_SECURITY_2
};

static code BYTE cmdClassListSecure_Sensor[] =
{
  COMMAND_CLASS_NOTIFICATION_V3,
  COMMAND_CLASS_ASSOCIATION,
  COMMAND_CLASS_ASSOCIATION_GRP_INFO,
  COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
	COMMAND_CLASS_MULTI_CHANNEL_V3,
	COMMAND_CLASS_CONFIGURATION
};


static EP_NIF endpointsNIF[NUMBER_OF_ENDPOINTS] = 
{
	/* EndPoint 1 */
	{ GENERIC_TYPE_SWITCH_BINARY, SPECIFIC_TYPE_POWER_SWITCH_BINARY,
		{
	{cmdClassListNonSecureIncludedNonSecure_Endpoints, sizeof(cmdClassListNonSecureIncludedNonSecure_Endpoints)},
	{{cmdClassListNonSecureIncludedSecure_Endpoints, sizeof(cmdClassListNonSecureIncludedSecure_Endpoints)},
	{cmdClassListSecure_Endpoints, sizeof(cmdClassListSecure_Endpoints)}}
		}
	},
	/* EndPoint 2 */
	{ GENERIC_TYPE_SENSOR_NOTIFICATION, SPECIFIC_TYPE_NOTIFICATION_SENSOR,
		{
	{cmdClassListNonSecureIncludedNonSecure_Endpoints, sizeof(cmdClassListNonSecureIncludedNonSecure_Endpoints)},
	{{cmdClassListNonSecureIncludedSecure_Endpoints, sizeof(cmdClassListNonSecureIncludedSecure_Endpoints)},
	{cmdClassListSecure_Endpoints, sizeof(cmdClassListSecure_Endpoints)}}
		}
	},
	/* Endpoitn 3 */
	{ GENERIC_TYPE_SENSOR_NOTIFICATION, SPECIFIC_TYPE_NOTIFICATION_SENSOR,
		{
	{cmdClassListNonSecureIncludedNonSecure_Sensor, sizeof(cmdClassListNonSecureIncludedNonSecure_Sensor)},
	{{cmdClassListNonSecureIncludedSecure_Sensor, sizeof(cmdClassListNonSecureIncludedSecure_Sensor)},
	{cmdClassListSecure_Sensor, sizeof(cmdClassListSecure_Sensor)}}
		}
	}
};


EP_FUNCTIONALITY_DATA endPointFunctionality =
{
  NUMBER_OF_INDIVIDUAL_ENDPOINTS,     /**< nbrIndividualEndpoints 7 bit*/
  RES_ZERO,                           /**< resIndZeorBit 1 bit*/
  NUMBER_OF_AGGREGATED_ENDPOINTS,     /**< nbrAggregatedEndpoints 7 bit*/
  RES_ZERO,                           /**< resAggZeorBit 1 bit*/
  RES_ZERO,                           /**< resZero 6 bit*/
  ENDPOINT_IDENTICAL_DEVICE_CLASS_NO,/**< identical 1 bit*/
  ENDPOINT_DYNAMIC_NO                /**< dynamic 1 bit*/
};

/**
 * AGI lifeline string
 */
const char GroupName[]   = "Lifeline";

CMD_CLASS_GRP  agiTableLifeLine[] =         {AGITABLE_LIFELINE_GROUP};
CMD_CLASS_GRP  agiTableLifeLineEndPoints[] = {AGITABLE_LIFELINE_GROUP_ENDPOINTS_VALVE};
CMD_CLASS_GRP  agiTableLifeLineEndpoints_Sensor[] = {AGITABLE_LIFELINE_GROUP_ENDPOINTS_SENSOR};
ST_ENDPOINT_ICONS ZWavePlusEndpointIcons[] = {ENDPOINT_ICONS};

AGI_GROUP agiTableRootDeviceGroups[] =        {AGITABLE_ROOTDEVICE_GROUPS};
AGI_GROUP agiTableEndpoint1Groups[] = {AGITABLE_ENDPOINT_1_GROUPS};
AGI_GROUP agiTableEndpoint2Groups[] = {AGITABLE_ENDPOINT_2_GROUPS};
AGI_GROUP agiTableEndpoint3Groups[] = {AGITABLE_ENDPOINT_3_GROUPS};


//static const AGI_PROFILE lifelineProfile = {
//	ASSOCIATION_GROUP_INFO_REPORT_PROFILE_GENERAL,
//	ASSOCIATION_GROUP_INFO_REPORT_PROFILE_GENERAL_LIFELINE
//};

//static const AGI_PROFILE lifelineProfileEndPoint1 = {
//	ASSOCIATION_GROUP_INFO_REPORT_PROFILE_CONTROL_V3,
//	ASSOCIATION_GROUP_INFO_REPORT_PROFILE_CONTROL_KEY01_V3
//};

//static const AGI_PROFILE lifelineProfileEndPoint2 = {
//	ASSOCIATION_GROUP_INFO_REPORT_PROFILE_CONTROL_V3,
//	ASSOCIATION_GROUP_INFO_REPORT_PROFILE_CONTROL_KEY02_V3
//};

static BYTE Valve_state = 0x00;
static BYTE Flow_state  = 0x00;
static BYTE Leak_state  = 0x00;
static BYTE Flow_value  = 0x00;
ASSOCIATION_ROOT_GROUP_MAPPING rootGroupMapping[] = { ASSOCIATION_ROOT_GROUP_MAPPING_CONFIG};
extern BOOL initstatus = FALSE;
KEY_EVENT_T Button_state_check;
BYTE supportedEvents = 0x00;
BYTE Config_Value ;
BYTE Stt_Value;
BYTE Meter_Value = 0x50;
BYTE Meter_config ;
BYTE Meter_scale_value;


/**
 * Application node ID
 */
BYTE myNodeID = 0;

/**
 * Application state-machine state.
 */
static STATE_APP currentState = STATE_APP_IDLE;

/**
 * Parameter is used to save wakeup reason after ApplicationInitHW(..)
 */
SW_WAKEUP wakeupReason;


/**
 * Use to tell if the host OTA required auto rebooting or not
 */
BOOL userReboot = FALSE;

#ifdef APP_SUPPORTS_CLIENT_SIDE_AUTHENTICATION
s_SecurityS2InclusionCSAPublicDSK_t sCSAResponse = { 0, 0, 0, 0};
#endif /* APP_SUPPORTS_CLIENT_SIDE_AUTHENTICATION */

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

// No exported data.

	

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

void ZCB_DeviceResetLocallyDone(TRANSMISSION_RESULT * pTransmissionResult);
void ZCB_ResetDelay(void);
STATE_APP GetAppState();
void AppStateManager( EVENT_APP event);
void ChangeState( STATE_APP newState);
#ifdef BOOTLOADER_ENABLED
void ZCB_OTAFinish(OTA_STATUS otaStatus);
BOOL ZCB_OTAStart(void);
void ZCB_OTAWrite(BYTE *pData, BYTE len);
#endif

void SetDefaultConfiguration(void);

void RefreshMMI(void);


/* **************** PRIVATE FUNCTIONS *******************/

void InexInfo(void);
void ReportState(BYTE endpoint, BYTE state);
void InexPoll(void);
void UARTPoll(void);
void LoadConfiguration(ZW_NVM_STATUS nvmStatus);
void ZCB_CommandClassSupervisionGetReceived(SUPERVISION_GET_RECEIVED_HANDLER_ARGS * pArgs);
void learnmode_reset(void);
void Blink_light(void);
P_KEY_EVENT_T GetButtonstatus(void);
void Clear_Button_status(void);
void ZCB_JobStatus(TRANSMISSION_RESULT * pTransmissionResult);

void Notificationrp(BYTE state);

void ReportState2(BYTE endpoint, BYTE state);
void ReportState3(BYTE endpoint, BYTE state);
BOOL Delay_Handle();





	
/**
 * @brief See description for function prototype in ZW_basis_api.h.
 */
void
ApplicationRfNotify(BYTE rfState)
{
  UNUSED(rfState);
}


/**
 * @brief See description for function prototype in ZW_basis_api.h.
 */
BYTE
ApplicationInitHW(SW_WAKEUP bWakeupReason)
{
  wakeupReason = bWakeupReason;

		//ZDP03A_InitHW(ZCB_eventSchedulerEventAdd);
	
	
	
	ConInit(96);
	SetPinOut(led_in);
	SetPinOut(led_ex);
	SetPinOut(led_er);
	SetPinIn(Sw_inex, TRUE);
	
	gpio_SetPin(led_in, TRUE);
	gpio_SetPin(led_ex, TRUE);
	gpio_SetPin(led_er, TRUE);
	
	
	//Cat25256_init();
			

	
//	ZW_SPI1_init(SPI_SPEED_8_MHZ|SPI_MODE_0|SPI_MSB_FIRST);
//	ZW_SPI1_enable(TRUE);
	
	
	
  Transport_OnApplicationInitHW(bWakeupReason);

  return(TRUE);
}


/**
 * @brief See description for function prototype in ZW_basis_api.h.
 */
BYTE
ApplicationInitSW(ZW_NVM_STATUS nvmStatus)
{
  BYTE application_node_type = DEVICE_OPTIONS_MASK;

  /* Init state machine*/
  currentState = STATE_APP_STARTUP;
  /* Do not reinitialize the UART if already initialized for ISD51 in ApplicationInitHW() */
	
	initstatus =	Valvekeyinit();
	ZW_SPI1_enable(TRUE);
	ZW_SPI1_init(SPI_SPEED_8_MHZ|SPI_MODE_0|SPI_MSB_FIRST);
	
	
#ifndef ZW_ISD51_DEBUG
  ZW_DEBUG_INIT(1152);
#endif



#ifdef WATCHDOG_ENABLED
  ZW_WatchDogEnable();
#endif
	
	
	ConInit(96);
	
  LoadConfiguration(nvmStatus);
	AssociationInitEndpointSupport(FALSE,
                                 rootGroupMapping,
                                 sizeof(rootGroupMapping)/sizeof(ASSOCIATION_ROOT_GROUP_MAPPING));

  /* Setup AGI group lists*/
  AGI_Init();
  AGI_LifeLineGroupSetup(agiTableLifeLine,
                         (sizeof(agiTableLifeLine)/sizeof(CMD_CLASS_GRP)),
                         NULL,
                         ENDPOINT_ROOT);
  AGI_LifeLineGroupSetup(agiTableLifeLineEndPoints,
												 (sizeof(agiTableLifeLine)/sizeof(CMD_CLASS_GRP)), 
													 NULL, ENDPOINT_1);
  AGI_LifeLineGroupSetup(agiTableLifeLineEndPoints, 
												 (sizeof(agiTableLifeLine)/sizeof(CMD_CLASS_GRP)), 
													 NULL, ENDPOINT_2);
	
	AGI_LifeLineGroupSetup(agiTableLifeLineEndPoints, 
												 (sizeof(agiTableLifeLine)/sizeof(CMD_CLASS_GRP)), 
													 NULL, ENDPOINT_3);
	

InitNotification();
  {
//    AddNotification(&(agiTableEndpoint1Groups->profile),
//                    NOTIFICATION_TYPE_POWER_MANAGEMENT,
//                    &supportedEvents,
//                    sizeof(supportedEvents),
//                    FALSE,
//                    ENDPOINT_1);

//    AddNotification(&(agiTableEndpoint2Groups->profile),
//                    NOTIFICATION_TYPE_POWER_MANAGEMENT,
//                    &supportedEvents,
//                    sizeof(supportedEvents),
//                    FALSE,
//                    ENDPOINT_2);

    AddNotification(&(agiTableEndpoint3Groups->profile),
                    NOTIFICATION_REPORT_WATER_V3,
                    &supportedEvents,
                    sizeof(supportedEvents),
                    FALSE,
                    ENDPOINT_3);
  }												 
												 
	CommandClassZWavePlusInfoInit(ZWavePlusEndpointIcons,
                                sizeof(ZWavePlusEndpointIcons)/sizeof(ST_ENDPOINT_ICONS));
  CommandClassSupervisionInit(
      CC_SUPERVISION_STATUS_UPDATES_NOT_SUPPORTED,
      ZCB_CommandClassSupervisionGetReceived,
      NULL);
												 
												 
	/* Get this sensors identification on the network */
  MemoryGetID(NULL, &myNodeID);

  /* Initialize manufactory specific module */
  ManufacturerSpecificDeviceIDInit();
												 
												 
												 
#ifdef BOOTLOADER_ENABLED
  /* Initialize OTA module */
    OtaInit( ZCB_OTAStart, NULL, ZCB_OTAFinish);
#endif

	CC_Version_SetApplicationVersionInfo(APP_VERSION, APP_REVISION, APP_VERSION_PATCH, APP_BUILD_NUMBER);

 /*
   * Initialize Event Scheduler.
   */
  ZAF_eventSchedulerInit(AppStateManager);

  Transport_OnApplicationInitSW( &m_AppNIF);
   Transport_AddEndpointSupport( &endPointFunctionality, endpointsNIF, NUMBER_OF_ENDPOINTS);
  ZCB_eventSchedulerEventAdd(EVENT_APP_INIT);

  return(application_node_type);
}


/**
 * @brief See description for function prototype in ZW_basis_api.h.
 */
void
ApplicationTestPoll(void)
{
}



/**
 * @brief See description for function prototype in ZW_basis_api.h.
 */
E_APPLICATION_STATE ApplicationPoll(E_PROTOCOL_STATE bProtocolState)
{
	static BOOL tmp = TRUE;
	static BYTE reg_stt;
	
	ZW_UART_COMMAND cmd;
  UNUSED(bProtocolState);
	
	ZCB_InterruptCallback();
	
		InexInfo();
		UARTPoll();
	
		ZW_SPI1_tx_set(0x01);
		ZW_SPI1_tx_set(0x02);
		ZW_SPI1_tx_set(0x03);
	ZW_SPI1_tx_set(0x01);
		ZW_SPI1_tx_set(0x02);
		ZW_SPI1_tx_set(0x03);
		ZW_SPI1_tx_set(0xFF);
	

  TaskApplicationPoll();

  return E_APPLICATION_STATE_ACTIVE;
}

void InexInfo(void){
	static ZW_UART_COMMAND cmd;
		Button_state_check = GetButtonstatus();
	
	if(Button_state_check == KEY_LEARNMODE){
			ChangeState(STATE_APP_IDLE);        
			ZCB_eventSchedulerEventAdd(EVENT_SYSTEM_LEARNMODE_START);
			Clear_Button_status();
			Button_state_check = KEY_IDLE;
	}
	
	if(Button_state_check == KEY_FACTORYNEW_RESET){
			ChangeState(STATE_APP_IDLE);        
			ZCB_eventSchedulerEventAdd(FACTORYNEW_RESET);
			Clear_Button_status();
			Button_state_check = KEY_IDLE;
					cmd.zw_uartcommandset.length =4;
					cmd.zw_uartcommandset.cmd = COMMAND_VALVE;
					cmd.zw_uartcommandset.type = ZW_VALVE_RESET;
					cmd.zw_uartcommandset.value1 = ZW_VALVE_RESET;
					ConTxFrame(cmd.zw_uartcommandset.cmd,cmd.zw_uartcommandset.type,&cmd.zw_uartcommandset.value1,cmd.zw_uartcommandset.length-3);	
	}
	if(Button_state_check == KEY_IDLE)
		{
			if(myNodeID){
				gpio_SetPin(led_in, FALSE);
				gpio_SetPin(led_ex, TRUE);
				gpio_SetPin(led_er, TRUE);
			}
			else
			{
				gpio_SetPin(led_in, TRUE);
				gpio_SetPin(led_ex, FALSE);
				gpio_SetPin(led_er, TRUE);
			}
		}
}
void UARTPoll(void)
{
	ZW_UART_COMMAND cmd;
  BYTE cmdLength;
	BYTE bMyNodeID;
	
	if (ConUpdate(TRUE) == conFrameReceived)
  {	
		/* Get command length*/
		cmdLength = serBuf[0];
		/* Coppy The Command from serBuf to cmd*/ 
    __ZW_memcpy(cmdLength, &cmd.zw_uartcommand.length,(serBuf+2));
		/* Check the command*/
		switch(cmd.zw_uartcommand.cmd)
    {
			case COMMAND_VALVE:
				
				if(cmd.zw_uartcommandset.type == ZW_VALVE_SET)
				{
					if(cmd.zw_uartcommandset.value1 == ZW_VALVE_ON){
						Valve_state = CMD_CLASS_BIN_OFF;   // with ha, off show on
						ReportState(ENDPOINT_1, Valve_state);
						ReportState2(ENDPOINT_2, Valve_state);
			//			ReportState3(ENDPOINT_2, Valve_state);
						
			//			ReportState(ENDPOINT_ROOT, Valve_state);
						MemoryPutByte((WORD)&Valve_state_far, Valve_state);
					}
					
					if(cmd.zw_uartcommandset.value1 == ZW_VALVE_OFF){
						
						Valve_state = CMD_CLASS_BIN_ON; // with ha, on show off
						ReportState(ENDPOINT_1, Valve_state);
						ReportState2(ENDPOINT_2, Valve_state);
			//			ReportState3(ENDPOINT_2, Valve_state);
			//			ReportState(ENDPOINT_ROOT, Valve_state);
						MemoryPutByte((WORD)&Valve_state_far, Valve_state);
					}
				}
				break;
				
			case COMMAND_LEAK:
				if(cmd.zw_uartcommandset.value1 == ZW_LEAK)
					{
						Leak_state = CMD_CLASS_BIN_OFF;
						Flow_state = CMD_CLASS_BIN_OFF;
						MemoryPutByte((WORD)&Leak_state_far, Leak_state);
						//MemoryPutByte((WORD)&Flow_state_far, Flow_state);
						ReportState(ENDPOINT_2, Leak_state);
//						ReportState(ENDPOINT_3, Flow_state);
						Notificationrp(Leak_state);
					}
					if(cmd.zw_uartcommandset.value1 == ZW_NON_LEAK){
						Leak_state = CMD_CLASS_BIN_ON;
						Flow_state = CMD_CLASS_BIN_ON;
						MemoryPutByte((WORD)&Leak_state_far, Leak_state);
						//MemoryPutByte((WORD)&Flow_state_far, Flow_state);
						ReportState(ENDPOINT_2, Leak_state);
						Notificationrp(Leak_state);
//						ReportState(ENDPOINT_3, Flow_state);
					}
				break;
			
			case COMMAND_METER:
				if(cmd.zw_uartcommandset.type == ZW_METER_REPORT){
					Meter_Value = cmd.zw_uartcommandset.value1 ;
				}
				break;
				
		}
	}
}

received_frame_status_t
Transport_ApplicationCommandHandlerEx(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  BYTE cmdLength)
{
  received_frame_status_t frame_status = RECEIVED_FRAME_STATUS_NO_SUPPORT;
  /* Call command class handlers */
  switch (pCmd->ZW_Common.cmdClass)
  {
    case COMMAND_CLASS_VERSION:
      frame_status = handleCommandClassVersion(rxOpt, pCmd, cmdLength);
      break;

#ifdef BOOTLOADER_ENABLED
    case COMMAND_CLASS_FIRMWARE_UPDATE_MD_V2:
      frame_status = handleCommandClassFWUpdate(rxOpt, pCmd, cmdLength);
      break;
#endif

    case COMMAND_CLASS_ASSOCIATION_GRP_INFO:
      frame_status = CC_AGI_handler( rxOpt, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_ASSOCIATION:
			frame_status = handleCommandClassAssociation(rxOpt, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_MANUFACTURER_SPECIFIC:
      frame_status = handleCommandClassManufacturerSpecific(rxOpt, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_ZWAVEPLUS_INFO:
      frame_status = handleCommandClassZWavePlusInfo(rxOpt, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_BASIC:
      frame_status = handleCommandClassBasic(rxOpt, pCmd, cmdLength);
      break;

		case COMMAND_CLASS_NOTIFICATION_V3:
      frame_status = handleCommandClassNotification(rxOpt, pCmd, cmdLength);
      break;
		
    case COMMAND_CLASS_SWITCH_BINARY:
      frame_status = handleCommandClassBinarySwitch(rxOpt, pCmd, cmdLength);
      break;

    case COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2:
      frame_status = handleCommandClassMultiChannelAssociation(rxOpt, pCmd, cmdLength);
      break;
		case COMMAND_CLASS_MULTI_CHANNEL_V3:
      frame_status = MultiChanCommandHandler(rxOpt, pCmd, cmdLength);
      break;
//		case COMMAND_CLASS_CENTRAL_SCENE_V3:
//			frame_status = handleCommandClassCentralScene(rxOpt, pCmd, cmdLength);
//			break;
		case COMMAND_CLASS_CONFIGURATION:
			frame_status = handleCommandClassConfiguration(rxOpt, pCmd, cmdLength);
			break;
		case COMMAND_CLASS_SWITCH_ALL:
			frame_status = handleCommandClassAllSwitch(rxOpt, pCmd, cmdLength);
      break;
		case COMMAND_CLASS_METER:
			frame_status = handleCommandClassMeter(rxOpt, pCmd, cmdLength);
			break;
  }
  return frame_status;
}



BYTE
handleCommandClassVersionAppl( BYTE cmdClass )
{
  BYTE commandClassVersion = UNKNOWN_VERSION;

  switch (cmdClass)
  {
    case COMMAND_CLASS_VERSION:
     commandClassVersion = CommandClassVersionVersionGet();
      break;

    case COMMAND_CLASS_BASIC:
     commandClassVersion =  CommandClassBasicVersionGet();
      break;
#ifdef BOOTLOADER_ENABLED
    case COMMAND_CLASS_FIRMWARE_UPDATE_MD:
      commandClassVersion = CommandClassFirmwareUpdateMdVersionGet();
      break;
#endif

    case COMMAND_CLASS_MANUFACTURER_SPECIFIC:
     commandClassVersion = CommandClassManufacturerVersionGet();
      break;

    case COMMAND_CLASS_ASSOCIATION:
     commandClassVersion = CommandClassAssociationVersionGet();
      break;

    case COMMAND_CLASS_ASSOCIATION_GRP_INFO:
     commandClassVersion = CommandClassAssociationGroupInfoVersionGet();
      break;

    case COMMAND_CLASS_DEVICE_RESET_LOCALLY:
     commandClassVersion = CommandClassDeviceResetLocallyVersionGet();
      break;

    case COMMAND_CLASS_ZWAVEPLUS_INFO:
     commandClassVersion = CommandClassZWavePlusVersion();
      break;
    case COMMAND_CLASS_SWITCH_BINARY:
     commandClassVersion = CommandClassBinarySwitchVersionGet();
      break;
		
		case COMMAND_CLASS_NOTIFICATION_V3:
      commandClassVersion = CommandClassNotificationVersionGet();
      break;
			
    case COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2:
      commandClassVersion = CmdClassMultiChannelAssociationVersion();
      break;
		
		case COMMAND_CLASS_MULTI_CHANNEL_V3:
      commandClassVersion = CmdClassMultiChannelGet();
      break;
		
    default:
     commandClassVersion = ZW_Transport_CommandClassVersionGet(cmdClass);
  }
  return commandClassVersion;
}




void
ApplicationSlaveUpdate(
  BYTE bStatus,
  BYTE bNodeID,
  BYTE* pCmd,
  BYTE bLen)
{
  UNUSED(bStatus);
  UNUSED(bNodeID);
  UNUSED(pCmd);
  UNUSED(bLen);
}


void ApplicationNetworkLearnModeCompleted(uint8_t bNodeID)
{
	
						gpio_SetPin(led_in, TRUE);
					gpio_SetPin(led_ex, TRUE);
					gpio_SetPin(led_er, TRUE);
  if(APPLICATION_NETWORK_LEARN_MODE_COMPLETED_SMART_START_IN_PROGRESS == bNodeID)
  {
    ZCB_eventSchedulerEventAdd(EVENT_APP_SMARTSTART_IN_PROGRESS);
  }
  else
  {
    if (APPLICATION_NETWORK_LEARN_MODE_COMPLETED_FAILED == bNodeID)
    {
      MemoryPutByte((WORD)&EEOFFSET_MAGIC_far, APPL_MAGIC_VALUE + 1);
      ZCB_eventSchedulerEventAdd((EVENT_APP) EVENT_SYSTEM_WATCHDOG_RESET);
    }
    else
    {
      if (APPLICATION_NETWORK_LEARN_MODE_COMPLETED_TIMEOUT == bNodeID)
      {
        /**
         * Inclusion or exclusion timed out. We need to make sure that the application triggers
         * Smartstart inclusion.
         */
        ZCB_eventSchedulerEventAdd(EVENT_APP_INIT);
        ChangeState(STATE_APP_STARTUP);
      }
      else
      {
        /*Success*/
        myNodeID = bNodeID;
        if (0 == myNodeID)
        {

          /*Clear association*/
          AssociationInit(TRUE);
          SetDefaultConfiguration();

          ZCB_eventSchedulerEventAdd(EVENT_APP_INIT);
          ChangeState(STATE_APP_STARTUP);
        }
      }
    }
    ZCB_eventSchedulerEventAdd((EVENT_APP) EVENT_SYSTEM_LEARNMODE_FINISH);
    Transport_OnLearnCompleted(bNodeID);
  }
}



BYTE
GetMyNodeID(void)
{
	return myNodeID;
}



STATE_APP
GetAppState(void)
{
  return currentState;
}


void
AppStateManager(EVENT_APP event)
{
	if(EVENT_SYSTEM_WATCHDOG_RESET == event)
  {
    ChangeState(STATE_APP_WATCHDOG_RESET);
  }


  switch(currentState)
  {
    case STATE_APP_STARTUP:

      if(EVENT_APP_INIT == event)
      {
        ZW_NetworkLearnModeStart(E_NETWORK_LEARN_MODE_INCLUSION_SMARTSTART);
      }

      ChangeState(STATE_APP_IDLE);
     break;

    case STATE_APP_IDLE:
			if(	FACTORYNEW_RESET == event){
				MemoryPutByte((WORD)&Config_far,0x00);
				MemoryPutByte((WORD)&Valve_state_far, CMD_CLASS_BIN_OFF);
				Valve_state = CMD_CLASS_BIN_OFF;
				
				MemoryPutByte((WORD)&EEOFFSET_MAGIC_far, 1 + APPL_MAGIC_VALUE);
        ZW_TIMER_START(ZCB_ResetDelay, 50, 1); // 50 * 10 = 500 ms  to be sure.


			}
      if (EVENT_APP_REFRESH_MMI == event)
      {
      }

      if(EVENT_APP_SMARTSTART_IN_PROGRESS == event)
      {
        ChangeState(STATE_APP_LEARN_MODE);
      }

      if (EVENT_SYSTEM_LEARNMODE_START == event)
      {
        
        if (myNodeID)
        {
					gpio_SetPin(led_ex, FALSE);
          ZW_NetworkLearnModeStart(E_NETWORK_LEARN_MODE_EXCLUSION_NWE);
        }
        else
        {
					gpio_SetPin(led_in, FALSE);
          ZW_NetworkLearnModeStart(E_NETWORK_LEARN_MODE_INCLUSION);
        }
        ChangeState(STATE_APP_LEARN_MODE);
      }

      if (EVENT_SYSTEM_RESET == event)
      {
        AGI_PROFILE lifelineProfile = {
                ASSOCIATION_GROUP_INFO_REPORT_PROFILE_GENERAL_NA_V2,
                ASSOCIATION_GROUP_INFO_REPORT_PROFILE_GENERAL_LIFELINE
        };
				MemoryPutByte((WORD)&EEOFFSET_MAGIC_far, 1 + APPL_MAGIC_VALUE);
//        ZW_TIMER_START(ZCB_ResetDelay, 50, 1); // 50 * 10 = 500 ms  to be sure.
        handleCommandClassDeviceResetLocally(&lifelineProfile, ZCB_DeviceResetLocallyDone);
        break;
      }
      break;

    case STATE_APP_LEARN_MODE:
      if(EVENT_APP_REFRESH_MMI == event){}
      if((EVENT_KEY_B1_PRESS == event)||(EVENT_SYSTEM_LEARNMODE_END == event))
      {
        ZW_NetworkLearnModeStart(E_NETWORK_LEARN_MODE_DISABLE);
        ZCB_eventSchedulerEventAdd(EVENT_APP_INIT);
        ChangeState(STATE_APP_STARTUP);
      }

      if(EVENT_SYSTEM_LEARNMODE_FINISH == event)
      {
        ChangeState(STATE_APP_IDLE);
      }
      break;

    case STATE_APP_WATCHDOG_RESET:
      if(EVENT_APP_REFRESH_MMI == event){}


      ZW_WatchDogEnable(); /*reset asic*/
      for (;;) {}
      break;
    case STATE_APP_OTA:
      if(EVENT_APP_REFRESH_MMI == event){}
      /*OTA state... do nothing until firmware update is finish*/
      break;

    case STATE_APP_TRANSMIT_DATA:
      if (EVENT_APP_FINISH_EVENT_JOB == event)
      {
//        ClearLastNotificationAction(&agiTableRootDeviceGroups[notificationOverLoadendpoint - 1].profile, notificationOverLoadendpoint);
        ChangeState(STATE_APP_IDLE);
      }

      if (EVENT_KEY_B6_PRESS == event)
      {
        /*
         * Pressing the B6/S6 key will toggle the overload timer. This timer
         * will transmit a notification every 30th second.
         */

      }
      break;
  }
  
}



static void
ChangeState(STATE_APP newState)
{
  ZW_DEBUG_MYPRODUCT_SEND_NL();
  ZW_DEBUG_MYPRODUCT_SEND_STR("State changed: ");
  ZW_DEBUG_MYPRODUCT_SEND_NUM(currentState);
  ZW_DEBUG_MYPRODUCT_SEND_STR(" -> ");
  ZW_DEBUG_MYPRODUCT_SEND_NUM(newState);

  currentState = newState;

  /**< Pre-action on new state is to refresh MMI */
  ZCB_eventSchedulerEventAdd(EVENT_APP_REFRESH_MMI);
}


PCB(ZCB_DeviceResetLocallyDone)(TRANSMISSION_RESULT * pTransmissionResult)
{
  if (TRANSMISSION_RESULT_FINISHED == pTransmissionResult->isFinished)
  {
    ZW_DEBUG_MYPRODUCT_SEND_NL();
    ZW_DEBUG_MYPRODUCT_SEND_STR("DRLD");

    ZCB_eventSchedulerEventAdd((EVENT_APP) EVENT_SYSTEM_WATCHDOG_RESET);
  }
}


PCB(ZCB_ResetDelay)(void)
{
  AGI_PROFILE lifelineProfile = {
      ASSOCIATION_GROUP_INFO_REPORT_PROFILE_GENERAL,
      ASSOCIATION_GROUP_INFO_REPORT_PROFILE_GENERAL_LIFELINE
  };


  handleCommandClassDeviceResetLocally(&lifelineProfile, ZCB_DeviceResetLocallyDone);

}






PCB(ZCB_OTAFinish)(OTA_STATUS otaStatus)
{
  UNUSED(otaStatus);
  /*Just reboot node to cleanup and start on new FW.*/
  ZW_WatchDogEnable(); /*reset asic*/
  while(1);
}



code const BOOL (code * ZCB_OTAStart_p)(void) = &ZCB_OTAStart;

BOOL
ZCB_OTAStart(void)
{
  BOOL  status = FALSE;
  if (STATE_APP_IDLE == GetAppState())
  {
    status = TRUE;
  }
  return status;
}

void
handleBasicSetCommand(BYTE val, BYTE endpoint )
{
  ZW_UART_COMMAND cmd;
		if(ENDPOINT_1 == endpoint){
				if(val == CMD_CLASS_BIN_OFF){
					Valve_state = CMD_CLASS_BIN_OFF;
					
			//		ReportState(ENDPOINT_ROOT, Valve_state);
					ReportState(ENDPOINT_1, Valve_state);
					
					cmd.zw_uartcommandset.length =4;
					cmd.zw_uartcommandset.cmd = COMMAND_VALVE;
					cmd.zw_uartcommandset.type = ZW_VALVE_SET;
					cmd.zw_uartcommandset.value1 = ZW_VALVE_ON;
					ConTxFrame(cmd.zw_uartcommandset.cmd,cmd.zw_uartcommandset.type,&cmd.zw_uartcommandset.value1,cmd.zw_uartcommandset.length-3);				
					}			
				
				if(val == CMD_CLASS_BIN_ON){
					Valve_state = CMD_CLASS_BIN_ON;
					
				//	ReportState(ENDPOINT_ROOT, Valve_state);
					ReportState(ENDPOINT_1, Valve_state);
					
					cmd.zw_uartcommandset.length =4;
					cmd.zw_uartcommandset.cmd = COMMAND_VALVE;
					cmd.zw_uartcommandset.type = ZW_VALVE_SET;
					cmd.zw_uartcommandset.value1 = ZW_VALVE_OFF;
					ConTxFrame(cmd.zw_uartcommandset.cmd,cmd.zw_uartcommandset.type,&cmd.zw_uartcommandset.value1,cmd.zw_uartcommandset.length-3);
					}
		}
}



BYTE
getAppBasicReport( BYTE endpoint )
{
  if(ENDPOINT_1 ==  endpoint)
  {
    return handleAppltBinarySwitchGet(endpoint);
  }
	if(ENDPOINT_2 == endpoint)
	{
		return handleAppltBinarySwitchGet(endpoint);
	}
//	if(ENDPOINT_3 == endpoint)
//	{
//		return handleAppltBinarySwitchGet(endpoint);
//	}
}



BYTE
getAppBasicReportTarget( BYTE endpoint )
{
 if(ENDPOINT_1 ==  endpoint)
  {
    return handleAppltBinarySwitchGet(endpoint);
  }
	if(ENDPOINT_2 == endpoint)
	{
		return handleAppltBinarySwitchGet(endpoint);
	}
//	if(ENDPOINT_3 == endpoint)
//	{
//		return handleAppltBinarySwitchGet(endpoint);
//	}
}



BYTE
getAppBasicReportDuration( BYTE endpoint )
{
  if(ENDPOINT_1 ==  endpoint)
  {
    return handleAppltBinarySwitchGet(endpoint);
  }
	if(ENDPOINT_2 == endpoint)
	{
		return handleAppltBinarySwitchGet(endpoint);
	}
//	if(ENDPOINT_3 == endpoint)
//	{
//		return handleAppltBinarySwitchGet(endpoint);
//	}
}

uint8_t handleNbrFirmwareVersions()
{
  return 1; /*CHANGE THIS - firmware 0 version*/
}



void
handleGetFirmwareVersion(
        BYTE bFirmwareNumber,
        VG_VERSION_REPORT_V2_VG* pVariantgroup)
{
  /*firmware 0 version and sub version*/
  if(bFirmwareNumber == 0)
  {
    pVariantgroup->firmwareVersion = APP_VERSION;
    pVariantgroup->firmwareSubVersion = APP_REVISION;
  }
  else
  {
    /*Just set it to 0 if firmware n is not present*/
    pVariantgroup->firmwareVersion = 0;
    pVariantgroup->firmwareSubVersion = 0;
  }
}


/**
 * Function return firmware Id of target n (0 => is device FW ID)
 * n read version of firmware number n (0,1..N-1)
 * @return firmaware ID.
 */
WORD
handleFirmWareIdGet( BYTE n)
{
  if(n == 0)
  {
    return APP_FIRMWARE_ID;
  }
  else if (n == 1)
  {
    return 0x1234;
  }
  return 0;
}

void setConfiguration(uint8_t Parameter,	uint8_t value )
{
	ZW_UART_COMMAND cmd;
	if(Parameter == WATER_MODE){
		if(WATER_MODE_0 == value){
					Config_Value = value;
					MemoryPutByte((WORD)&Config_far,value);
					cmd.zw_uartcommandset.length =4;
					cmd.zw_uartcommandset.cmd = COMMAND_CONFIGURATION;
					cmd.zw_uartcommandset.type = ZW_CONFIGURATION;
					cmd.zw_uartcommandset.value1 = Config_Value;
					ConTxFrame(cmd.zw_uartcommandset.cmd,cmd.zw_uartcommandset.type,
					&cmd.zw_uartcommandset.value1,cmd.zw_uartcommandset.length-3);
		}
		if(WATER_MODE_1 == value){
					Config_Value = value;
					MemoryPutByte((WORD)&Config_far,value);
					cmd.zw_uartcommandset.length =4;
					cmd.zw_uartcommandset.cmd = COMMAND_CONFIGURATION;
					cmd.zw_uartcommandset.type = ZW_CONFIGURATION;
					cmd.zw_uartcommandset.value1 = Config_Value;
					ConTxFrame(cmd.zw_uartcommandset.cmd,cmd.zw_uartcommandset.type,
					&cmd.zw_uartcommandset.value1,cmd.zw_uartcommandset.length-3);
		}
	}
	if(Parameter == METER_VALUE){
		MemoryPutByte((WORD)&Meter_config_far,value);
		Meter_config = value;
		cmd.zw_uartcommandset.length =4;
					cmd.zw_uartcommandset.cmd = COMMAND_METER;
					cmd.zw_uartcommandset.type = ZW_METER_SET;
					cmd.zw_uartcommandset.value1 = Meter_config ;
					ConTxFrame(cmd.zw_uartcommandset.cmd,cmd.zw_uartcommandset.type,
					&cmd.zw_uartcommandset.value1,cmd.zw_uartcommandset.length-3);
	}	
}



uint8_t getConfiguration(uint8_t parameterNumber){
	return Stt_Value;
//	if(parameterNumber == 1){
//		return Config_Value;
//	}
//	if(parameterNumber == 2){
//		return Meter_config;
//	}
}

BYTE getMeter(void){
	return Meter_Value;
}

void MeterReset(void){
	ZW_UART_COMMAND cmd;
	Meter_config = 0;
	MemoryPutByte((WORD)&Meter_config_far,Meter_config);
	cmd.zw_uartcommandset.length =4;
					cmd.zw_uartcommandset.cmd = COMMAND_METER;
					cmd.zw_uartcommandset.type = ZW_METER_RESET;
					cmd.zw_uartcommandset.value1 = Meter_config;
					ConTxFrame(cmd.zw_uartcommandset.cmd,cmd.zw_uartcommandset.type,
					&cmd.zw_uartcommandset.value1,cmd.zw_uartcommandset.length-3);
}

BYTE
handleAppltBinarySwitchGet(BYTE endpoint)
{
  if(ENDPOINT_1 == endpoint){
			return Valve_state;
	}
	if(ENDPOINT_2 == endpoint){
			return Leak_state;
	}
//	if(ENDPOINT_3 == endpoint){
//			return Flow_state;
//	}
}






BOOL handleAssociationSet(
    uint8_t ep,
    ZW_MULTI_CHANNEL_ASSOCIATION_SET_1BYTE_V2_FRAME* pCmd,
    uint8_t cmdLength)
{	
	ZW_MultiChannelAssociationSet1byteV2Frame CMD = &pCmd;
	
	if(ENDPOINT_1 == ep){
		handleApplBinarySwitchSet(CMD.properties1,ENDPOINT_1);
	return TRUE;
	}
}
	




	
void
handleApplBinarySwitchSet(CMD_CLASS_BIN_SW_VAL val, BYTE endpoint )
{
		ZW_UART_COMMAND cmd;
		if(ENDPOINT_1 == endpoint){
				if(val == CMD_CLASS_BIN_OFF){
					Valve_state = CMD_CLASS_BIN_OFF;
	
//					ReportState(ENDPOINT_ROOT, Valve_state);
					ReportState(ENDPOINT_1, Valve_state);
					MemoryPutByte((WORD)&Valve_state_far, Valve_state);
					
					cmd.zw_uartcommandset.length =4;
					cmd.zw_uartcommandset.cmd = COMMAND_VALVE;
					cmd.zw_uartcommandset.type = ZW_VALVE_SET;
					cmd.zw_uartcommandset.value1 = ZW_VALVE_ON;
					ConTxFrame(cmd.zw_uartcommandset.cmd,cmd.zw_uartcommandset.type,&cmd.zw_uartcommandset.value1,cmd.zw_uartcommandset.length-3);	
					
					}		
				if(val == CMD_CLASS_BIN_ON){
					Valve_state = CMD_CLASS_BIN_ON;
	
//					ReportState(ENDPOINT_ROOT, Valve_state);
					ReportState(ENDPOINT_1, Valve_state);
					MemoryPutByte((WORD)&Valve_state_far, Valve_state);
					
					cmd.zw_uartcommandset.length =4;
					cmd.zw_uartcommandset.cmd = COMMAND_VALVE;
					cmd.zw_uartcommandset.type = ZW_VALVE_SET;
					cmd.zw_uartcommandset.value1 = ZW_VALVE_OFF;
					ConTxFrame(cmd.zw_uartcommandset.cmd,cmd.zw_uartcommandset.type,&cmd.zw_uartcommandset.value1,cmd.zw_uartcommandset.length-3);
					
					}
		}
}
void
SetDefaultConfiguration(void)
{
  ZW_DEBUG_MYPRODUCT_SEND_NL();
  ZW_DEBUG_MYPRODUCT_SEND_BYTE('C');
  ZW_DEBUG_MYPRODUCT_SEND_BYTE('d');

	
  MemoryPutByte((WORD)&EEOFFSET_MAGIC_far, APPL_MAGIC_VALUE);
}



void
LoadConfiguration(ZW_NVM_STATUS nvmStatus)
{
	ZW_UART_COMMAND cmd;
  MemoryGetID( NULL, &myNodeID);
  ManufacturerSpecificDeviceIDInit();
	
	#ifdef BOOTLOADER_ENABLED
  NvmInit(nvmStatus);
#else
  UNUSED(nvmStatus);
#endif
	


		Config_Value = MemoryGetByte((WORD)&Config_far);
    Valve_state = MemoryGetByte((WORD)&Valve_state_far);
		Leak_state = MemoryGetByte((WORD)&Leak_state_far);
		Flow_state = MemoryGetByte((WORD)&Flow_state_far);
		
	  /* Initialize the NVM if needed */
  if (MemoryGetByte((WORD)&EEOFFSET_MAGIC_far) == APPL_MAGIC_VALUE)
  {
    /* Initialize PowerLevel functionality*/
		Config_Value = MemoryGetByte((WORD)&Config_far);
		Valve_state = MemoryGetByte((WORD)&Valve_state_far);
		Leak_state = MemoryGetByte((WORD)&Leak_state_far);
		Flow_state = MemoryGetByte((WORD)&Flow_state_far);
		
   // loadStatusPowerLevel();
  }
	
  else
  {
    /* Initialize transport layer NVM */
    Transport_SetDefault();
    /* Reset protocol */
    ZW_SetDefault();
    /* Initialize PowerLevel functionality.*/
    loadInitStatusPowerLevel();

    /* Force reset of associations. */
    AssociationInit(TRUE);

    ZW_MEM_PUT_BYTE((WORD)&EEOFFSET_MAGIC_far, APPL_MAGIC_VALUE);
    ZW_MEM_PUT_BYTE((WORD)&EEOFFS_SECURITY_RESERVED.EEOFFS_MAGIC_BYTE_field, EEPROM_MAGIC_BYTE_VALUE);
   // DefaultNotifactionStatus(NOTIFICATION_STATUS_UNSOLICIT_ACTIVED);
  }
	cmd.zw_uartcommandset.length =4;
					cmd.zw_uartcommandset.cmd = COMMAND_CONFIGURATION;
					cmd.zw_uartcommandset.type = ZW_CONFIGURATION;
					cmd.zw_uartcommandset.value1 = Config_Value;
					ConTxFrame(cmd.zw_uartcommandset.cmd,cmd.zw_uartcommandset.type,&cmd.zw_uartcommandset.value1,cmd.zw_uartcommandset.length-3);

				if(Valve_state == CMD_CLASS_BIN_OFF){

					ReportState(ENDPOINT_1, Valve_state);
					cmd.zw_uartcommandset.length =4;
					cmd.zw_uartcommandset.cmd = COMMAND_VALVE;
					cmd.zw_uartcommandset.type = ZW_VALVE_SET;
					cmd.zw_uartcommandset.value1 = ZW_VALVE_ON;
					ConTxFrame(cmd.zw_uartcommandset.cmd,cmd.zw_uartcommandset.type,&cmd.zw_uartcommandset.value1,cmd.zw_uartcommandset.length-3);				
					}			
				
				if(Valve_state == CMD_CLASS_BIN_ON){

					ReportState(ENDPOINT_1, Valve_state);
					
					cmd.zw_uartcommandset.length =4;
					cmd.zw_uartcommandset.cmd = COMMAND_VALVE;
					cmd.zw_uartcommandset.type = ZW_VALVE_SET;
					cmd.zw_uartcommandset.value1 = ZW_VALVE_OFF;
					ConTxFrame(cmd.zw_uartcommandset.cmd,cmd.zw_uartcommandset.type,&cmd.zw_uartcommandset.value1,cmd.zw_uartcommandset.length-3);
					}		
}







/*
void
RefreshMMI(void)
{
  if (CMD_CLASS_BIN_OFF == onOffState)
  {
    Led(ZDP03A_LED_D2,OFF);
  }
  else if (CMD_CLASS_BIN_ON == onOffState)
  {
    Led(ZDP03A_LED_D2,ON);
  }
}

*/



/*
 * @brief Called when protocol needs to inform Application about a Security Event.
 * @details If the app does not need/want the Security Event the handling can be left empty.
 *
 *    Event E_APPLICATION_SECURITY_EVENT_S2_INCLUSION_REQUEST_DSK_CSA
 *          If CSA is needed, the app must do the following when this event occures:
 *             1. Obtain user input with first 4 bytes of the S2 including node DSK
 *             2. Store the user input in a response variable of type s_SecurityS2InclusionCSAPublicDSK_t.
 *             3. Call ZW_SetSecurityS2InclusionPublicDSK_CSA(response)
 *
 */
void
ApplicationSecurityEvent(
  s_application_security_event_data_t *securityEvent)
{
  switch (securityEvent->event)
  {
#ifdef APP_SUPPORTS_CLIENT_SIDE_AUTHENTICATION
    case E_APPLICATION_SECURITY_EVENT_S2_INCLUSION_REQUEST_DSK_CSA:
      {
        ZW_SetSecurityS2InclusionPublicDSK_CSA(&sCSAResponse);
      }
      break;
#endif /* APP_SUPPORTS_CLIENT_SIDE_AUTHENTICATION */

    default:
      break;
  }
}


/**
* Set up security keys to request when joining a network.
* These are taken from the config_app.h header file.
*/
BYTE ApplicationSecureKeysRequested(void)
{
//  return REQUESTED_SECURITY_KEYS_SECURITY_KEY_NON_MASK;
}

/**
* Set up security S2 inclusion authentication to request when joining a network.
* These are taken from the config_app.h header file.
*/
BYTE ApplicationSecureAuthenticationRequested(void)
{
  return REQUESTED_SECURITY_AUTHENTICATION;
}





void ReportState(BYTE endpoint, BYTE state)
{
	ZW_APPLICATION_TX_BUFFER *pTxBuf = GetResponseBuffer(); 

	if( NON_NULL( pTxBuf ) ) 
	{
		TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx;
		TRANSMIT_OPTIONS_TYPE_SINGLE_EX txOptionsEx;
		MULTICHAN_NODE_ID destNode;
		txOptionsEx.pDestNode = &destNode;
		*(&pTxOptionsEx) = &txOptionsEx;
		
		destNode.node.nodeId = 1;								//controller_id =1
		destNode.node.endpoint = 0;					
		destNode.nodeInfo.BitMultiChannelEncap = 0;  
		destNode.nodeInfo.security = 0x00;							//security_key_none

		txOptionsEx.txOptions = TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE | ZWAVE_PLUS_TX_OPTIONS;
		txOptionsEx.sourceEndpoint = endpoint;
		txOptionsEx.txSecOptions = 0;
		txOptionsEx.pDestNode->nodeInfo.security = 0x00;
		
		pTxBuf->ZW_SwitchBinaryReportFrame.cmdClass = COMMAND_CLASS_SWITCH_BINARY;
		pTxBuf->ZW_SwitchBinaryReportFrame.cmd = SWITCH_BINARY_REPORT;
		pTxBuf->ZW_SwitchBinaryReportFrame.value = state;

		if(ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
				(BYTE *)pTxBuf,
				sizeof(ZW_SWITCH_BINARY_REPORT_FRAME),
				pTxOptionsEx,
				ZCB_ResponseJobStatus))   
		{
			/*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
			FreeResponseBuffer();
		}
	}
}

void ReportState2(BYTE endpoint, BYTE state)
{
	ZW_APPLICATION_TX_BUFFER *pTxBuf = GetResponseBuffer(); 

	if( NON_NULL( pTxBuf ) ) 
	{
		TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx;
		TRANSMIT_OPTIONS_TYPE_SINGLE_EX txOptionsEx;
		MULTICHAN_NODE_ID destNode;
		txOptionsEx.pDestNode = &destNode;
		*(&pTxOptionsEx) = &txOptionsEx;
		
		destNode.node.nodeId = 1;								//controller_id =1
		destNode.node.endpoint = 0;					
		destNode.nodeInfo.BitMultiChannelEncap = 0;  
		destNode.nodeInfo.security = 0x00;							//security_key_none

		txOptionsEx.txOptions = TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE | ZWAVE_PLUS_TX_OPTIONS;
		txOptionsEx.sourceEndpoint = endpoint;
		txOptionsEx.txSecOptions = 0;
		txOptionsEx.pDestNode->nodeInfo.security = 0x00;
		
		pTxBuf->ZW_ConfigurationReport1byteFrame.cmdClass = COMMAND_CLASS_CONFIGURATION;
		pTxBuf->ZW_ConfigurationReport1byteFrame.cmd = CONFIGURATION_REPORT;
		pTxBuf->ZW_ConfigurationReport1byteFrame.parameterNumber = endpoint;
		  pTxBuf->ZW_ConfigurationReport1byteFrame.level = 0x01;
		  pTxBuf->ZW_ConfigurationReport1byteFrame.configurationValue1 = state;

		if(ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
				(BYTE *)pTxBuf,
				sizeof(ZW_CONFIGURATION_REPORT_1BYTE_FRAME),
				pTxOptionsEx,
				ZCB_ResponseJobStatus))   
		{
			/*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
			FreeResponseBuffer();
		}
	}
}


PCB(ZCB_CommandClassSupervisionGetReceived)(SUPERVISION_GET_RECEIVED_HANDLER_ARGS * pArgs)
{
	/*
  if (SWITCH_MULTILEVEL_SET == pArgs->cmd && COMMAND_CLASS_SWITCH_MULTILEVEL_V4 == pArgs->cmdClass)
  {
    uint8_t duration = GetCurrentDuration(pArgs->rxOpt->destNode.endpoint);
    pArgs->duration = duration;
    if (0 < duration)
    {
      pArgs->status = CC_SUPERVISION_STATUS_WORKING;

      if (CC_SUPERVISION_STATUS_UPDATES_SUPPORTED == CC_SUPERVISION_EXTRACT_STATUS_UPDATE(pArgs->properties1))
      {
        // Save the data
        rxOptionSupervision = *(pArgs->rxOpt);
        sessionID = CC_SUPERVISION_EXTRACT_SESSION_ID(pArgs->properties1);

        pArgs->properties1 = CC_SUPERVISION_ADD_MORE_STATUS_UPDATE(CC_SUPERVISION_MORE_STATUS_UPDATES_REPORTS_TO_FOLLOW) | CC_SUPERVISION_ADD_SESSION_ID(sessionID);

        /*
         * Start timer that will send another Supervision report when triggered.
         *
         * Checking whether duration is higher than 127 is done because these values are
         * interpreted as minutes. Please see table 8, SDS13781-4.
         */
				 
				 
				 /*
        ZW_TimerLongStart(ZCB_SupervisionTimerCallback, ((127 < duration) ? (duration - 127) * 1000 * 60 : duration * 1000), 1);
      }
    }
    else {
      // Duration == 0. I.e. the command has already been handled and completed. Clear the "more status updates" flag
      pArgs->properties1 &= (CC_SUPERVISION_ADD_MORE_STATUS_UPDATE(CC_SUPERVISION_MORE_STATUS_UPDATES_THIS_IS_LAST) |
                             CC_SUPERVISION_COMMAND_PROPERTIES_RESERVED_BITMASK |
                             CC_SUPERVISION_COMMAND_PROPERTIES_SESSION_ID_BITMASK);
    }
  }
  else
  {

    pArgs->properties1 &= (CC_SUPERVISION_ADD_MORE_STATUS_UPDATE(CC_SUPERVISION_MORE_STATUS_UPDATES_THIS_IS_LAST) |
                           CC_SUPERVISION_COMMAND_PROPERTIES_RESERVED_BITMASK |
                           CC_SUPERVISION_COMMAND_PROPERTIES_SESSION_ID_BITMASK);
    pArgs->duration = 0;
  }
	*/
}


PCB(ZCB_JobStatus)(TRANSMISSION_RESULT * pTransmissionResult)
{
  
  if (TRANSMISSION_RESULT_FINISHED == pTransmissionResult->isFinished)
  {
    ZCB_eventSchedulerEventAdd(EVENT_APP_FINISH_EVENT_JOB);
  }
}

void Notificationrp(BYTE state){
	 JOB_STATUS jobStatus;
	
	
	
	if(state == CMD_CLASS_BIN_ON)
	{
		supportedEvents = 0x02;
	NotificationEventTrigger(&agiTableRootDeviceGroups[3 -1].profile,
        NOTIFICATION_TYPE_WATER_ALARM,
        supportedEvents,
        &supportedEvents, 1,
        ENDPOINT_3);
		
		  jobStatus = UnsolicitedNotificationAction(
      &agiTableRootDeviceGroups[3 - 1].profile,
      ENDPOINT_3,
      ZCB_JobStatus);
  //@ [NOTIFICATION_TRANSMIT]

  if (JOB_STATUS_SUCCESS != jobStatus)
  {
    TRANSMISSION_RESULT transmissionResult;

    

    transmissionResult.status = FALSE;
    transmissionResult.nodeId = 0;
    transmissionResult.isFinished = TRANSMISSION_RESULT_FINISHED;

    ZCB_JobStatus(&transmissionResult);
  }
		
	}
	else{
		
		supportedEvents = 0x05;
			NotificationEventTrigger(&agiTableRootDeviceGroups[3 -1].profile,
        NOTIFICATION_TYPE_WATER_ALARM,
        supportedEvents,
        &supportedEvents, 1,
        ENDPOINT_3);
		
		  jobStatus = UnsolicitedNotificationAction(
      &agiTableRootDeviceGroups[3 - 1].profile,
      ENDPOINT_3,
      ZCB_JobStatus);
  //@ [NOTIFICATION_TRANSMIT]

  if (JOB_STATUS_SUCCESS != jobStatus)
  {
    TRANSMISSION_RESULT transmissionResult;


    transmissionResult.status = FALSE;
    transmissionResult.nodeId = 0;
    transmissionResult.isFinished = TRANSMISSION_RESULT_FINISHED;

    ZCB_JobStatus(&transmissionResult);
  }
		
	}
	
}


void Cat25256_init(void){
	//SetPinOut(spi_cs);
	ZW_SPI1_init(SPI_SPEED_8_MHZ|SPI_MODE_0|SPI_MSB_FIRST);
	ZW_SPI1_enable(TRUE);
	//gpio_SetPin(spi_cs, TRUE);
}

BYTE Cat25256_read(BYTE address1, BYTE address2 ){
	BYTE Data;
	gpio_SetPin(spi_cs, FALSE);
	ZW_SPI1_tx_set(READ);
	ZW_SPI1_tx_set(address1);
	ZW_SPI1_tx_set(address2);
	Data = ZW_SPI1_rx_get();
	gpio_SetPin(spi_cs, TRUE);
	return Data;
}

void Cat25256_write(BYTE address1, BYTE address2, BYTE Data){
	gpio_SetPin(spi_cs, FALSE);
	ZW_SPI1_tx_set(WRITE);
	ZW_SPI1_tx_set(address1);
	ZW_SPI1_tx_set(address2);
	ZW_SPI1_tx_set(Data);
	gpio_SetPin(spi_cs,TRUE);
}

BYTE reg_status(void){
	BYTE Data;
	//gpio_SetPin(spi_cs, FALSE);
	ZW_SPI1_tx_set(0xFF);
//while(ZW_SPI1_active_get() != 0){
//}

//	ZW_TIMER_START(Delay_Handle, 1, 1);
	Data = ZW_SPI1_rx_get();
//	gpio_SetPin(spi_cs, TRUE);
	return Data;
}

void Cat25256_write_reg(BYTE reg_value){
	gpio_SetPin(spi_cs, FALSE);
	ZW_SPI1_tx_set(WRSR);
	ZW_SPI1_tx_set(reg_value);
	gpio_SetPin(spi_cs, TRUE);
}

BOOL Delay_Handle()
{
	return TRUE;

}
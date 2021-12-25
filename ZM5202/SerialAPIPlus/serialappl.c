/****************************************************************************
 *
 * Copyright (c) 2001-2013
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 *---------------------------------------------------------------------------
 *
 * Description:       Serial API implementation for Enhanced Z-Wave module
 *
 * Last Changed By:  $Author: jsi $
 * Revision:         $Revision: 30882 $
 * Last Changed:     $Date: 2015-02-04 16:03:45 +0100 (on, 04 feb 2015) $
 *
 ****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include "config_app.h"
#include <app_version.h>
#ifdef ZW_CONTROLLER
#ifdef ZW_INSTALLER
#include <ZW_controller_installer_api.h>
#else
#ifdef ZW_CONTROLLER_STATIC
#ifdef ZW_CONTROLLER_BRIDGE
#include <ZW_controller_bridge_api.h>
#else
#include <ZW_controller_static_api.h>
#endif /* ZW_CONTROLLER_BRIDGE */
#else
#include <ZW_controller_api.h>
#endif /* ZW_CONTROLLER_STATIC */
#endif /* ZW_INSTALLER */
#endif /* ZW_CONTROLLER */

#ifdef ZW_SLAVE
#ifdef ZW_SLAVE_32
#include <ZW_slave_32_api.h>
#else
#ifdef ZW_SLAVE_ROUTING
#include <ZW_slave_routing_api.h>
#else
#include <ZW_slave_api.h>
#endif  /* ZW_SLAVE_ROUTING */
#endif  /* ZW_SLAVE_32 */
#endif  /* ZW_SLAVE */


#include <ZW_pindefs.h>
#include <ZW_evaldefs.h>

#include <led_control.h>
#include <ZW_uart_api.h>
#include <ZW_SerialAPI.h>
#include <conhandle.h>
#include <ZW_power_api.h>
#include <ZW_flash_api.h>
#include <ZW_nvr_api.h>
#include <ZW_timer_api.h>

/* Include serialappl header file - containing version and */
/* SerialAPI functionality support definitions */
#include <serialappl.h>
#include <eeprom.h>
#include "nvmHost.h"
#ifdef ZW_SECURITY_PROTOCOL
#include <ZW_security_api.h>
#include <ZW_TransportLayer.h>
#endif

#if SUPPORT_ZW_AES_ECB
#include <ZW_aes_api.h>
#endif

#if SUPPORT_SERIAL_API_POWER_MANAGEMENT
#include <ZW_portpin_api.h>
#endif

#if SUPPORT_ZW_PORT_STATUS
#include <ZW_ev_scheduler.h>
#include <ZW_portpin_api.h>
#include <port_monitor.h>
#endif

#if SUPPORT_ZW_NUNIT
#include <ZW_nunit_api.h>
#endif
#ifdef ZW_ISD51_DEBUG
#include "ISD51.h"
#endif

#if defined(ZW_CONTROLLER) || defined(ZW_SLAVE_32)
#include <ZW_nvm_ext_api.h>
#endif

#if SUPPORT_ZW_FIRMWARE_UPDATE_NVM
#include <ZW_firmware_update_nvm_api.h>
#endif

#if SUPPORT_ZW_NVR_GET_APP_VALUE
#include <ZW_nvr_app_api.h>
#endif

/* Support ZW_SendData even though support has been removed from library. */
#undef SUPPORT_ZW_SEND_DATA
#define SUPPORT_ZW_SEND_DATA 1

/* Basic level definitions */
#define BASIC_ON                                        0xFF
#define BASIC_OFF                                       0x00

/* The end of HDATA class in bytes (to be used as a constant as (WORD)&_HDATA_E_) */
extern unsigned char _HDATA_E_;      /* LX51 linker will deliver this */

#define NVM_STORAGE_END ((WORD)&_HDATA_E_)

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
#ifdef JP_DK
/* Define RSSI threshold so JP can be tested in DK */
#define JP_DK_RSSI_THRESHOLD      52
#endif

/**
 *
 */
typedef struct _S_TRANSPORT_REQUESTED_SECURITY_SETTINGS_
{
  BYTE requestedSecurityKeysBits;
  e_security_s2_authentication_t requestedSecurityAuthentication;
} S_TRANSPORT_REQUESTED_SECURITY_SETTINGS;

typedef struct _S_POWERLEVELS_STRUCT_
{
  BYTE normal[POWERLEVEL_CHANNELS];
  BYTE low[POWERLEVEL_CHANNELS];
} S_POWERLEVELS;

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

#ifdef SUPPORT_SERIAL_API_GET_CAPABILITIES
const XBYTE SERIALAPI_CAPABILITIES[] = {
                                        APP_VERSION, APP_REVISION,
                                        SERIALAPI_MANUFACTURER_ID1, SERIALAPI_MANUFACTURER_ID2,
                                        SERIALAPI_MANUFACTURER_PRODUCT_TYPE1, SERIALAPI_MANUFACTURER_PRODUCT_TYPE2,
                                        SERIALAPI_MANUFACTURER_PRODUCT_ID1, SERIALAPI_MANUFACTURER_PRODUCT_ID2,
/* EFH-comment: I have split the listing of all these CAP_BM(x) into separate lines */
/* because else some kind of internal compiler overflow occurred.                   */
/* C51 COMPILER V7.50 - SN:                                                         */
/* COPYRIGHT KEIL ELEKTRONIK GmbH 1987 - 2004                                       */
/**** ERROR C190 IN LINE 1249 OF SERIALAPPL.H: '&': not an lvalue                   */
                                        CAP_BM(0),
                                        CAP_BM(1),
                                        CAP_BM(2),
                                        CAP_BM(3),
                                        CAP_BM(4),
                                        CAP_BM(5),
                                        CAP_BM(6),
                                        CAP_BM(7),
                                        CAP_BM(8),
                                        CAP_BM(9),
                                        CAP_BM(10),
                                        CAP_BM(11),
                                        CAP_BM(12),
                                        CAP_BM(13),
                                        CAP_BM(14),
                                        CAP_BM(15),
                                        CAP_BM(16),
                                        CAP_BM(17),
                                        CAP_BM(18),
                                        CAP_BM(19),
                                        CAP_BM(20),
                                        CAP_BM(21),
                                        CAP_BM(22),
                                        CAP_BM(23),
                                        CAP_BM(24),
                                        CAP_BM(25),
                                        CAP_BM(26),
                                        CAP_BM(27),
                                        CAP_BM(28),
                                        CAP_BM(29),
                                        CAP_BM(30),
                                        CAP_BM(31)
                                       };
#endif

/* State vars for ApplicationPoll */
BYTE state = stateIdle;
BYTE retry = 0;

/* Work variable */
BYTE i, j, n;

XBYTE retVal;

#ifdef ZW_ENABLE_RTC
#define RTC_TIMER_MAX 8
/* Array of RTCtimer callback functions */
VOID_CALLBACKFUNC(rtcArray[RTC_TIMER_MAX + 1])(BYTE, BYTE);
#endif

/* Should be enough */
#define BUF_SIZE_RX 168
#define BUF_SIZE_TX 168
BYTE workbuf[BUF_SIZE_RX];          /* Used for frames received from remote side */
XBYTE compl_workbuf[BUF_SIZE_TX];   /* Used for frames send to remote side. */

#if SUPPORT_ZW_SEND_DATA_MULTI || SUPPORT_ZW_SEND_DATA_MULTI_BRIDGE || SUPPORT_ZW_SEND_DATA_MULTI_EX
BYTE groupMask[MAX_NODEMASK_LENGTH]; /* Used for sending nodelist in a multicast */
#endif

/* Queue for frames transmitted to PC - callback, ApplicationCommandHandler, ApplicationControllerUpdate... */
#if defined(ZW_CONTROLLER_BRIDGE) || defined(USBVCP)
#define MAX_CALLBACK_QUEUE 3
#else
#define MAX_CALLBACK_QUEUE 4
#endif

typedef struct _callback_element_
{
  BYTE      wCmd;
  BYTE      wLen;
  BYTE      wBuf[BUF_SIZE_TX];
} CALLBACK_ELEMENT;

typedef struct _request_queue_
{
  BYTE requestOut;
  BYTE requestIn;
  BYTE requestCnt;
  CALLBACK_ELEMENT requestQueue[MAX_CALLBACK_QUEUE];
} REQUEST_QUEUE;

REQUEST_QUEUE callbackQueue;

REQUEST_QUEUE commandQueue;

BOOL bTxStatusReportEnabled;

#if SUPPORT_ADD_REMOVE_PROTECT
static BYTE addState = 0;
#endif

/* Set listening, generic, specific and nodeparm defaults */
#ifdef ZW_CONTROLLER
#ifdef ZW_CONTROLLER_STATIC
XBYTE applNodeInfo_deviceOptionsMask = APPLICATION_NODEINFO_LISTENING;
#else
XBYTE applNodeInfo_deviceOptionsMask = APPLICATION_NODEINFO_NOT_LISTENING;
#endif
#else
XBYTE applNodeInfo_deviceOptionsMask = APPLICATION_NODEINFO_LISTENING;
#endif  /* ZW_CONTROLLER */

#ifdef ZW_CONTROLLER
#ifdef ZW_CONTROLLER_STATIC
XBYTE applNodeInfo_nodeType_generic = GENERIC_TYPE_STATIC_CONTROLLER;
XBYTE applNodeInfo_nodeType_specific = SPECIFIC_TYPE_PC_CONTROLLER;
#else
XBYTE applNodeInfo_nodeType_generic = GENERIC_TYPE_GENERIC_CONTROLLER;
XBYTE applNodeInfo_nodeType_specific = SPECIFIC_TYPE_PORTABLE_REMOTE_CONTROLLER;
#endif
#else
XBYTE applNodeInfo_nodeType_generic = GENERIC_TYPE_SWITCH_MULTILEVEL;
XBYTE applNodeInfo_nodeType_specific = SPECIFIC_TYPE_NOT_USED;
#endif  /* ZW_CONTROLLER */
XBYTE applNodeInfo_parmLength = 0;
XBYTE applNodeInfo_nodeParm[APPL_NODEPARM_MAX];

#if SUPPORT_SERIAL_API_APPL_NODE_INFORMATION_CMD_CLASSES
XBYTE applNodeInfo_unincluded_parmLength = 0;
XBYTE applNodeInfo_unincluded_nodeParm[APPL_NODEPARM_MAX];
XBYTE applNodeInfo_included_unsecure_parmLength = 0;
XBYTE applNodeInfo_included_unsecure_nodeParm[APPL_NODEPARM_MAX];
XBYTE applNodeInfo_included_secure_parmLength = 0;
XBYTE applNodeInfo_included_secure_nodeParm[APPL_NODEPARM_MAX];

/**
 * Structure includes application node information list's and device type.
 */
APP_NODE_INFORMATION m_AppNIF =
{
  applNodeInfo_unincluded_nodeParm, 0,
  applNodeInfo_included_unsecure_nodeParm, 0,
  applNodeInfo_included_secure_nodeParm, 0,
  0, 0, 0
};

static S_TRANSPORT_REQUESTED_SECURITY_SETTINGS sRequestedSecuritySettings = { REQUESTED_SECURITY_KEYS,
                                                                              REQUESTED_SECURITY_AUTHENTICATION};
#endif


#ifdef ZW_CONTROLLER_BRIDGE
BYTE applSlaveNodeInfo_deviceOptionsMask = APPLICATION_NODEINFO_LISTENING;
BYTE applSlaveNodeInfo_nodeType_generic = GENERIC_TYPE_SWITCH_MULTILEVEL;
BYTE applSlaveNodeInfo_nodeType_specific = SPECIFIC_TYPE_NOT_USED;
BYTE applSlaveNodeInfo_parmLength = 0;
XBYTE applSlaveNodeInfo_nodeParm[APPL_SLAVENODEPARM_MAX];

#endif  /* ZW_CONTROLLER_BRIDGE */

#ifdef ZW_SMARTSTART_ENABLED
BYTE applPowerModeExtIntEnable;
E_SYSTEM_TYPE applPowerMode;
DWORD applPowerModeWutTimeOut;
#endif

/* params used for storing Callback function IDs returned to remote side */
#if SUPPORT_ZW_SEND_NODE_INFORMATION
BYTE funcID_ComplHandler_ZW_SendNodeInformation;
#endif
#if SUPPORT_ZW_SEND_DATA
BYTE funcID_ComplHandler_ZW_SendData;
#endif
#if SUPPORT_ZW_SEND_DATA_EX
BYTE funcID_ComplHandler_ZW_SendDataEx;
#endif
#if SUPPORT_ZW_SEND_DATA_META || SUPPORT_ZW_SEND_DATA_META_BRIDGE
BYTE funcID_ComplHandler_ZW_SendDataMeta;
#endif

#if SUPPORT_ZW_REQUEST_NEW_ROUTE_DESTINATIONS
static BYTE funcID_ComplHandler_ZW_RequestNewRouteDestinations;

#ifdef ZW_SLAVE_32
static BYTE returnRouteDests[5];
#else
static BYTE returnRouteDests[ZW_MAX_RETURN_ROUTE_DESTINATIONS];
#endif
#endif

#if SUPPORT_ZW_SET_SUC_NODE_ID
BYTE funcID_ComplHandler_ZW_SetSUCNodeID;
#endif
#if SUPPORT_ZW_SEND_DATA_MULTI || SUPPORT_ZW_SEND_DATA_MULTI_BRIDGE
BYTE funcID_ComplHandler_ZW_SendDataMulti;
#endif
#if SUPPORT_ZW_SEND_DATA_MULTI_EX
BYTE funcID_ComplHandler_ZW_SendDataMultiEx;
#endif
#if SUPPORT_ZW_SET_LEARN_NODE_STATE
BYTE funcID_ComplHandler_ZW_SetLearnNodeState;
#endif
#if SUPPORT_ZW_REQUEST_NODE_NEIGHBOR_UPDATE
BYTE funcID_ComplHandler_ZW_RequestNodeNeighborUpdate;
#endif
#if SUPPORT_ZW_SET_DEFAULT
BYTE funcID_ComplHandler_ZW_SetDefault;
#endif
#ifdef ZW_CONTROLLER
BYTE funcID_ComplHandler_ZW_NodeManagement;
BYTE nodeManagement_Func_ID;
#endif
#if SUPPORT_ZW_REPLICATION_SEND_DATA
BYTE funcID_ComplHandler_ZW_ReplicationSendData;
#endif
#if SUPPORT_ZW_ASSIGN_RETURN_ROUTE
BYTE funcID_ComplHandler_ZW_AssignReturnRoute;
#endif
#if SUPPORT_ZW_DELETE_RETURN_ROUTE
BYTE funcID_ComplHandler_ZW_DeleteReturnRoute;
#endif
#if SUPPORT_ZW_ASSIGN_PRIORITY_RETURN_ROUTE
BYTE funcID_ComplHandler_ZW_AssignPriorityReturnRoute;
#endif
#if SUPPORT_ZW_ASSIGN_PRIORITY_SUC_RETURN_ROUTE
BYTE funcID_ComplHandler_ZW_AssignPrioritySUCReturnRoute;
#endif
#if SUPPORT_ZW_REMOVE_FAILED_NODE_ID
BYTE funcID_ComplHandler_ZW_RemoveFailedNodeID;
#endif
#if SUPPORT_ZW_REPLACE_FAILED_NODE
BYTE funcID_ComplHandler_ZW_ReplaceFailedNode;
#endif
#if SUPPORT_ZW_SEND_SUC_ID
BYTE funcID_ComplHandler_ZW_SendSUC_ID;
#endif
#if SUPPORT_STORE_NODEINFO
BYTE funcID_ComplHandler_ZW_StoreNodeInfo;
#endif
#ifdef ZW_SLAVE
#if SUPPORT_ZW_SET_LEARN_MODE
BYTE funcID_ComplHandler_ZW_SetLearnMode;
#endif
#endif
#if SUPPORT_ZW_SEND_SLAVE_NODE_INFORMATION
BYTE funcID_ComplHandler_ZW_SendSlaveNodeInformation;
#endif
#if SUPPORT_ZW_SEND_SLAVE_DATA
BYTE funcID_ComplHandler_ZW_SendSlaveData;
#endif
#if SUPPORT_ZW_SET_SLAVE_LEARN_MODE
BYTE funcID_ComplHandler_ZW_SetSlaveLearnMode;
#endif
#if SUPPORT_ZW_SEND_TEST_FRAME
BYTE funcID_ComplHandler_ZW_SendTestFrame;
#endif

#ifdef ZW_CONTROLLER
BYTE funcID_ComplHandler_netWork_Management;
BYTE management_Func_ID;
#else
#ifdef ZW_SLAVE_ROUTING
BYTE funcID_ComplHandler_netWork_Management;
BYTE management_Func_ID;
#endif
#endif

#if SUPPORT_MEMORY_PUT_BUFFER
BYTE funcID_ComplHandler_MemoryPutBuffer;
#endif

#ifdef ZW_ENABLE_RTC
BYTE funcID_ComplHandler_RTCTimerCreate;
CLOCK clkTime;

RTC_TIMER timer;
#endif

BYTE appHomeId[HOMEID_LENGTH];
BYTE appNodeId;

#if SUPPORT_ZW_WATCHDOG_START | SUPPORT_ZW_WATCHDOG_STOP
BYTE bWatchdogStarted;
#endif

#ifdef ZW_CONTROLLER_SINGLE
#if SUPPORT_SERIAL_API_TEST
ZW_APPLICATION_META_TX_BUFFER txBuffer;

BYTE funcID_ComplHandler_Serial_API_Test;
XBYTE testnodemask[ZW_MAX_NODES / 8];
XBYTE testresultnodemask[ZW_MAX_NODES / 8];

BYTE testnodemasklen
#ifndef WORK_PATCH
 = 0
#endif
;
BYTE timerTestHandle = 0xff;
BYTE testCmd;
BOOL setON = TRUE;
BYTE testNodeID = 0;
WORD testDelay;
WORD testCurrentDelay;
BYTE testPayloadLen;
WORD testCount;
WORD testSuccessCount;
WORD testFailedCount;
BYTE testState = 0;
BYTE testTxOptions;
#endif
#endif

#if SUPPORT_SERIAL_API_READY

enum
{
  /* SERIAL_LINK_IDLE = Ready for incomming Serial communication, but */
  /* do not transmit anything via the serial link even if application */
  /* frames is received on the RF, which normally should be transmitted */
  /* to the HOST. */
  SERIAL_LINK_DETACHED = 0,
  /* SERIAL_LINK_CONNECTED = There exists a HOST so transmit on serial */
  /* link if needed. */
  SERIAL_LINK_CONNECTED = 1
};

BYTE serialLinkState;
#endif

/* Last system wakeup reason - is set in ApplicationInitHW */
BYTE applWakeupReason;

#if SUPPORT_SERIAL_API_POWER_MANAGEMENT

/* Undefine this as when Power Management is enabled, no IO pins should */
/* be changed in regards to the RESET IO state */
#undef ENABLE_LEDS

/* Define this if for simple test of the POWER MANAGEMENT functionality */
/* Initiates HOST Powerdown using the LEDs on the ZDP03A */
/* Enables HOST Wakeup on External using Button (INT1) on Z-Wave module */
/* Enables HOST Wakeup on Timer set to 600 seconds */
/* Enables HOST Wakeup on RF - Wait for specific frame pattern, before waking up the HOST */
/* HOST Wakeup uses the LEDs on the ZDP03A */
#undef POWER_MANAGEMENT_TEST

/* Power Management Command definitions. */
#define PM_PIN_UP_CONFIGURATION_CMD           0x01
#define PM_MODE_CONFIGURATION_CMD             0x02
#define PM_POWERUP_ZWAVE_CONFIGURATION_CMD    0x03
#define PM_POWERUP_TIMER_CONFIGURATION_CMD    0x04
#define PM_POWERUP_EXTERNAL_CONFIGURATION_CMD 0x05
#define PM_SET_POWER_MODE_CMD                 0x06
#define PM_GET_STATUS                         0x07

/* SerialAPI power management is in Idle as it has not been configured */
/* to handle power management for HOST. */
#define POWER_MODE_IDLE                       0x00
#define POWER_MODE_RUNNING_TRANSITION         0x01
#define POWER_MODE_RUNNING                    0x02
#define POWER_MODE_POWERDOWN_TRANSITION       0x03
#define POWER_MODE_POWERDOWN                  0x04
#define POWER_MODE_MAX                        POWER_MODE_POWERDOWN

BYTE powerManagementState;

#define PM_WAKEUP_REASON_NONE                 0x00
#define PM_WAKEUP_REASON_EXTERNAL             0x01
#define PM_WAKEUP_REASON_RF_ALL               0x02
#define PM_WAKEUP_REASON_RF_ALL_NO_BROADCAST  0x03
#define PM_WAKEUP_REASON_RF_MASK              0x04
#define PM_WAKEUP_REASON_TIMER_SECONDS        0x05
#define PM_WAKEUP_REASON_TIMER_MINUTES        0x06

BYTE powerManagementWakeUpReason;

#define PM_PHYSICAL_PIN_P00         0x00
#define PM_PHYSICAL_PIN_P01         0x01
#define PM_PHYSICAL_PIN_P02         0x02
#define PM_PHYSICAL_PIN_P03         0x03
#define PM_PHYSICAL_PIN_P04         0x04
#define PM_PHYSICAL_PIN_P05         0x05
#define PM_PHYSICAL_PIN_P06         0x06
#define PM_PHYSICAL_PIN_P07         0x07
#define PM_PHYSICAL_PIN_P10         0x10
#define PM_PHYSICAL_PIN_P11         0x11
#define PM_PHYSICAL_PIN_P12         0x12
#define PM_PHYSICAL_PIN_P13         0x13
#define PM_PHYSICAL_PIN_P14         0x14
#define PM_PHYSICAL_PIN_P15         0x15
#define PM_PHYSICAL_PIN_P16         0x16
#define PM_PHYSICAL_PIN_P17         0x17
#define PM_PHYSICAL_PIN_P22         0x22
#define PM_PHYSICAL_PIN_P23         0x23
#define PM_PHYSICAL_PIN_P24         0x24
#define PM_PHYSICAL_PIN_P30         0x30
#define PM_PHYSICAL_PIN_P31         0x31
#define PM_PHYSICAL_PIN_P32         0x32
#define PM_PHYSICAL_PIN_P33         0x33
#define PM_PHYSICAL_PIN_P34         0x34
#define PM_PHYSICAL_PIN_P35         0x35
#define PM_PHYSICAL_PIN_P36         0x36
#define PM_PHYSICAL_PIN_P37         0x37
#define PM_PHYSICAL_PIN_MAX         PM_PHYSICAL_PIN_P37
#define PM_PHYSICAL_PIN_UNDEFINED   0xFF

/* In ZW050x, which has a 8051 core, the Ports P0, P1, P2 and P3 are memory */
/* mapped into address 0x80, 0x90, 0xA0 and 0xB0 respectively */
#define PM_PHYSICAL_PIN_OFFSET      0x80

#define PM_IO_PIN_MAX               4

typedef struct _PM_PIN
{
  BYTE bPin;
  BYTE bEnableLevel;
  BYTE bPullUp;
} PM_PIN;

#define PM_PIN_IN_DEBOUNCE          2

BYTE poweredUpTimerHandle;
BYTE poweredUpTransition;
BOOL poweredUp;

BYTE wakeUpOnExternalTimerHandle;
BYTE wakeUpOnExternalTransition;
BOOL wakeUpOnExternal;

BOOL wakeUpOnRF;

WORD wakeUpOnTimerCount;
BYTE wakeUpOnTimerRes;
BYTE wakeUpOnTimerHandle;
BOOL wakeUpOnTimer;

/* Structure containing configuration of the optional HOST PoweredUp pin */
PM_PIN powerManagementPoweredUp;

/* Structure containing pin configuration when waking up the HOST */
PM_PIN powerManagementWakeUpPinConf[PM_IO_PIN_MAX];

/* Power Management RF Wakeup modes */
#define PM_WAKEUP_UNDEFINED         0x00
#define PM_WAKEUP_ALL               0x01
#define PM_WAKEUP_ALL_NO_BROADCAST  0x02
#define PM_WAKEUP_MASK              0x03
#define PM_WAKEUP_MODE_MAX          PM_WAKEUP_MASK

#define PM_WAKEUP_MAX_BYTES         8
#define PM_MASK_DONTCARE            0

typedef struct _PM_WAKEUP
{
  BYTE bValue;
  BYTE bMask;
} PM_WAKEUP;

PM_WAKEUP powerManagementWakeUpOnRF[PM_WAKEUP_MAX_BYTES];

BYTE powerManagementWakeUpOnRFMode;
BYTE powerManagementWakeUpOnRFCount;

/* Power Management Timer Resolution definitions */
#define PM_TIMER_UNDEFINED          0x00
#define PM_TIMER_SECONDS            0x01
#define PM_TIMER_MINUTES            0x02
#define PM_TIMER_MODE_MAX           PM_TIMER_MINUTES

BYTE powerManagementWakeUpOnTimer;
WORD powerManagementWakeUpOnTimerCount;

PM_PIN powerManagementWakeUpOnExternal;

/* Structure containing pin configuration of HOST PowerDown */
PM_PIN powerManagementPowerMode[PM_IO_PIN_MAX];

#endif

#ifdef APPL_PROD_TEST
#define PRODTEST_NR_1     1
#define PRODTEST_NR_2     2
#define PRODTEST_NR_3     3
#define PRODTEST_NR_4     4
#define PRODTEST_NR_5     5
#define PRODTEST_NR_6     6
#define PRODTEST_NR_7     7

#define POWERON_MAGIC_VALUE  0x4242
extern BYTE bProdtestState;
/* Production test pin definition */
static BYTE testRun;
#define SET_PRODUCTIONTEST_PIN  PIN_IN(P21, 1)
#define IN_PRODUCTIONTEST       (!PIN_GET(P21))
#endif /* APPL_PROD_TEST */

/* Structs for host configured powerlevels */
#ifndef slave_routing
static S_POWERLEVELS sPowerlevels;
#endif


/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

BYTE CheckPowerlevel(BYTE bPower);

#if SUPPORT_ZW_NUNIT
BYTE ZCB_GetCallbackCnt(void);
static void ZCB_NUnitCmd( BYTE cmd, XBYTE* pMessage, BYTE len );
#endif

#ifdef UZB

#if 1 // 0 - test UZB on ZDP03A, 1 - normal mode (UZB on UZB :)
#define LEDxPort      P0
#define LEDxSHADOW    P0Shadow
#define LEDxSHADOWDIR P0ShadowDIR
#define LEDxDIR       P0DIR
#define LEDxDIR_PAGE  P0DIR_PAGE
#define LEDx          4
#else // 0 - test UZB on ZDP03A, 1 - normal mode (UZB on UZB :)
#define LEDxPort      P0
#define LEDxSHADOW    P0Shadow
#define LEDxSHADOWDIR P0ShadowDIR
#define LEDxDIR       P0DIR
#define LEDxDIR_PAGE  P0DIR_PAGE
#define LEDx          7
#endif // 0 - test UZB on ZDP03A, 1 - normal mode (UZB on UZB :)

void            /*RET  Nothing                  */
set_state(
BYTE st
)
{
  if (state != st)
  {
    if (st == stateIdle)
    {
      PIN_HIGH(LEDx);
    }
    else if (state == stateIdle)
    {
      PIN_LOW(LEDx);
    }
    state = st;
  }
}


#else  // UZB

#define set_state(st) state = st

#endif    // UZB


/*===============================   Request   ================================
**    Queues request (callback) to be transmitted to remote side
**
**--------------------------------------------------------------------------*/
BOOL               /*RET  queue status (FALSE queue full)*/
Request(
  BYTE cmd,       /*IN   Command                  */
  XBYTE *pData,   /*IN   pointer to data          */
  BYTE len        /*IN   Length of data           */
)
{
#if SUPPORT_SERIAL_API_READY
  /* Only queue Request frame for HOST if SERIAL LINK has been established or we need to send the WakeUp Frame */
  if (((SERIAL_LINK_DETACHED != serialLinkState) || (wakeUpOnRF)) && (callbackQueue.requestCnt < MAX_CALLBACK_QUEUE))
#else
  if (callbackQueue.requestCnt < MAX_CALLBACK_QUEUE)
#endif
  {
    callbackQueue.requestCnt++;
    callbackQueue.requestQueue[callbackQueue.requestIn].wCmd = cmd;
    if (len > BUF_SIZE_TX)
    {
      len = BUF_SIZE_TX;
    }
    callbackQueue.requestQueue[callbackQueue.requestIn].wLen = len;
    memcpy(&callbackQueue.requestQueue[callbackQueue.requestIn].wBuf[0], pData, len);
    if (++callbackQueue.requestIn >= MAX_CALLBACK_QUEUE)
    {
      callbackQueue.requestIn = 0;
    }
    return TRUE;
  }
  return FALSE;
}



/*=========================   RequestUnsolicited   ===========================
**    Queues request (command) to be transmitted to remote side
**
**--------------------------------------------------------------------------*/
BOOL               /*RET  queue status (FALSE queue full)*/
RequestUnsolicited(
  BYTE cmd,       /*IN   Command                  */
  XBYTE *pData,   /*IN   pointer to data          */
  BYTE len        /*IN   Length of data           */
)
{
#if SUPPORT_SERIAL_API_READY
  /* Only queue Request frame for HOST if SERIAL LINK has been established or we need to send the WakeUp Frame */
  if (((SERIAL_LINK_DETACHED != serialLinkState) || (wakeUpOnRF)) && (commandQueue.requestCnt < MAX_CALLBACK_QUEUE))
#else
  if (commandQueue.requestCnt < MAX_CALLBACK_QUEUE)
#endif
  {
    commandQueue.requestCnt++;
    commandQueue.requestQueue[commandQueue.requestIn].wCmd = cmd;
    if (len > BUF_SIZE_TX)
    {
      len = BUF_SIZE_TX;
    }
    commandQueue.requestQueue[commandQueue.requestIn].wLen = len;
    memcpy(&commandQueue.requestQueue[commandQueue.requestIn].wBuf[0], pData, len);
    if (++commandQueue.requestIn >= MAX_CALLBACK_QUEUE)
    {
      commandQueue.requestIn = 0;
    }
    return TRUE;
  }
  return FALSE;
}

void
PurgeCallbackQueue(void)
{
  callbackQueue.requestOut = callbackQueue.requestIn = callbackQueue.requestCnt = 0;
}

void
PurgeCommandQueue(void)
{
  commandQueue.requestOut = commandQueue.requestIn = commandQueue.requestCnt = 0;
}

/*===============================   Respond   ===============================
**    Send immediate respons to remote side
**
**    Side effects: Sets state variable to stateTxSerial (wait for ack)
**
**--------------------------------------------------------------------------*/
void             /*RET  Nothing                 */
Respond(
  BYTE cmd,       /*IN   Command                  */
  XBYTE *pData,    /*IN   pointer to data          */
  BYTE len        /*IN   Length of data           */
)
{
  /* If there are no data; pData == NULL and len == 0 we must set the data pointer */
  /* to some dummy data. ConTxFrame interprets NULL pointer as retransmit indication */
  if (len == 0)
  {
    pData = (XBYTE *)0x7ff; /* Just something is not used anyway */
  }
  ConTxFrame(cmd, RESPONSE, pData, len);

  set_state(stateTxSerial);  /* We want ACK/NAK...*/
}


#ifdef ZW_ENABLE_RTC
/*===============================   RTCHandler  =============================
**    RTC timer callback functions. Called when RTC timer fires
**
**--------------------------------------------------------------------------*/
void            /* RET  Nothing                                 */
RTCHandler(
  BYTE funcId,    /* IN   Callback function ID                    */
  BYTE status,    /* IN   status                                  */
  BYTE parm)      /* IN   parm specified when creating RTC timer  */
{
  BYTE_IN_AR(compl_workbuf, 0) = funcId;
  BYTE_IN_AR(compl_workbuf, 1) = status;
  BYTE_IN_AR(compl_workbuf, 2) = parm;
  Request(FUNC_ID_RTC_TIMER_CALL, compl_workbuf, 3);
}


void RTCHandler1(BYTE status, BYTE parm) {RTCHandler(0, status,parm);}
void RTCHandler2(BYTE status, BYTE parm) {RTCHandler(1, status,parm);}
void RTCHandler3(BYTE status, BYTE parm) {RTCHandler(2, status,parm);}
void RTCHandler4(BYTE status, BYTE parm) {RTCHandler(3, status,parm);}
void RTCHandler5(BYTE status, BYTE parm) {RTCHandler(4, status,parm);}
void RTCHandler6(BYTE status, BYTE parm) {RTCHandler(5, status,parm);}
void RTCHandler7(BYTE status, BYTE parm) {RTCHandler(6, status,parm);}
void RTCHandler8(BYTE status, BYTE parm) {RTCHandler(7, status,parm);}
void RTCHandler9(BYTE status, BYTE parm) {RTCHandler(8, status,parm);}
void RTCHandler10(BYTE status, BYTE parm) {RTCHandler(9, status,parm);}
#endif  /* ZW_ENABLE_RTC */


#if SUPPORT_ZW_SEND_NODE_INFORMATION
code const void (code * ZCB_ComplHandler_ZW_SendNodeInformation_p)(BYTE, TX_STATUS_TYPE*) = &ZCB_ComplHandler_ZW_SendNodeInformation;
/*=====================   ComplHandler_ZW_SendNodeInformation   =============
**    Completion handler for ZW_SendNodeInformation
**
**--------------------------------------------------------------------------*/
void                             /* RET  Nothing                     */
ZCB_ComplHandler_ZW_SendNodeInformation(
  BYTE txStatus,                        /* IN   Transmit completion status  */
  TX_STATUS_TYPE *txStatusReport)
{
  UNUSED(txStatusReport);
  BYTE_IN_AR(compl_workbuf, 0) = funcID_ComplHandler_ZW_SendNodeInformation;
  BYTE_IN_AR(compl_workbuf, 1) = txStatus;
  Request(FUNC_ID_ZW_SEND_NODE_INFORMATION, compl_workbuf, 2);
}
#endif


#if SUPPORT_ZW_SEND_DATA
code const void (code * ZCB_ComplHandler_ZW_SendData_p)(BYTE, TX_STATUS_TYPE*) = &ZCB_ComplHandler_ZW_SendData;
/*======================   ComplHandler_ZW_SendData   ========================
**    Completion handler for ZW_SendData
**
**--------------------------------------------------------------------------*/
void                             /* RET  Nothing                     */
ZCB_ComplHandler_ZW_SendData(
  BYTE txStatus,
  TX_STATUS_TYPE *txStatusReport)                        /* IN   Transmit completion status  */
{
  BYTE bIdx = 0;
  BYTE_IN_AR(compl_workbuf, bIdx++) = funcID_ComplHandler_ZW_SendData;
  BYTE_IN_AR(compl_workbuf, bIdx++) = txStatus;
  if (bTxStatusReportEnabled  /* Do HOST want txStatusReport */
      && txStatusReport)      /* Check if detailed info is available from protocol */
  {
    memcpy(&compl_workbuf[bIdx], (BYTE*)txStatusReport, sizeof(TX_STATUS_TYPE));
    bIdx += sizeof(TX_STATUS_TYPE);
  }
  Request(FUNC_ID_ZW_SEND_DATA, compl_workbuf, bIdx);
}
#endif  /* SUPPORT_ZW_SEND_DATA */


#if SUPPORT_ZW_SEND_DATA_EX
code const void (code * ZCB_ComplHandler_ZW_SendDataEx_p)(BYTE, TX_STATUS_TYPE*) = &ZCB_ComplHandler_ZW_SendDataEx;
/*======================   ComplHandler_ZW_SendDataEx   ========================
**    Completion handler for ZW_SendDataEx
**
**--------------------------------------------------------------------------*/
void                             /* RET  Nothing                     */
ZCB_ComplHandler_ZW_SendDataEx(
  BYTE txStatus,
  TX_STATUS_TYPE *txStatusReport)                        /* IN   Transmit completion status  */
{
  BYTE bIdx = 0;
  BYTE_IN_AR(compl_workbuf, bIdx++) = funcID_ComplHandler_ZW_SendDataEx;
  BYTE_IN_AR(compl_workbuf, bIdx++) = txStatus;
  if (bTxStatusReportEnabled  /* Do HOST want txStatusReport */
      && txStatusReport)      /* Check if detailed info is available from protocol */
  {
    memcpy(&compl_workbuf[bIdx], (BYTE*)txStatusReport, sizeof(TX_STATUS_TYPE));
    bIdx += sizeof(TX_STATUS_TYPE);
  }
  Request(FUNC_ID_ZW_SEND_DATA_EX, compl_workbuf, bIdx);
}
#endif  /* SUPPORT_ZW_SEND_DATA_EX */


#if SUPPORT_ZW_SEND_DATA_META
code const void (code * ZCB_ComplHandler_ZW_SendDataMeta_p)(BYTE, TX_STATUS_TYPE*) = &ZCB_ComplHandler_ZW_SendDataMeta;
/*====================   ComplHandler_ZW_SendDataMeta   ======================
**    Completion handler for ZW_SendDataMeta
**
**--------------------------------------------------------------------------*/
void                             /* RET  Nothing                     */
ZCB_ComplHandler_ZW_SendDataMeta(
  BYTE txStatus,
  TX_STATUS_TYPE *txStatusReport)                        /* IN   Transmit completion status  */
{
  UNUSED(txStatusReport);
  BYTE_IN_AR(compl_workbuf, 0) = funcID_ComplHandler_ZW_SendDataMeta;
  BYTE_IN_AR(compl_workbuf, 1) = txStatus;
  Request(FUNC_ID_ZW_SEND_DATA_META, compl_workbuf, 2);
}
#endif


#if SUPPORT_ZW_SEND_DATA_BRIDGE
code const void (code * ZCB_ComplHandler_ZW_SendData_Bridge_p)(BYTE, TX_STATUS_TYPE*) = &ZCB_ComplHandler_ZW_SendData_Bridge;
/*=================   ComplHandler_ZW_SendData_Bridge   ======================
**    Completion handler for ZW_SendData_Bridge
**
**--------------------------------------------------------------------------*/
void                             /* RET  Nothing                     */
ZCB_ComplHandler_ZW_SendData_Bridge(
  BYTE txStatus,
  TX_STATUS_TYPE* txStatusReport)                        /* IN   Transmit completion status  */
{
  BYTE bIdx = 0;
  BYTE_IN_AR(compl_workbuf, bIdx++) = funcID_ComplHandler_ZW_SendData;
  BYTE_IN_AR(compl_workbuf, bIdx++) = txStatus;
  if (bTxStatusReportEnabled  /* Do HOST want txStatusReport */
      && txStatusReport)      /* Check if detailed info is available from protocol */
  {
    memcpy(&compl_workbuf[bIdx], (BYTE*)txStatusReport, sizeof(TX_STATUS_TYPE));
    bIdx += sizeof(TX_STATUS_TYPE);
  }
  Request(FUNC_ID_ZW_SEND_DATA_BRIDGE, compl_workbuf, bIdx);
}
#endif


#if SUPPORT_ZW_SEND_DATA_META_BRIDGE
code const void (code * ZCB_ComplHandler_ZW_SendDataMeta_Bridge_p)(BYTE, TX_STATUS_TYPE*) = &ZCB_ComplHandler_ZW_SendDataMeta_Bridge;
/*===============   ComplHandler_ZW_SendDataMeta_Bridge   ====================
**    Completion handler for ZW_SendDataMeta_Bridge
**
**--------------------------------------------------------------------------*/
void                             /* RET  Nothing                     */
ZCB_ComplHandler_ZW_SendDataMeta_Bridge(
  BYTE txStatus,
  TX_STATUS_TYPE *txStatusReport)                        /* IN   Transmit completion status  */
{
  UNUSED(txStatusReport);
  BYTE_IN_AR(compl_workbuf, 0) = funcID_ComplHandler_ZW_SendDataMeta;
  BYTE_IN_AR(compl_workbuf, 1) = txStatus;
  Request(FUNC_ID_ZW_SEND_DATA_META_BRIDGE, compl_workbuf, 2);
}
#endif


#if SUPPORT_ZW_SEND_DATA_MULTI_BRIDGE
code const void (code * ZCB_ComplHandler_ZW_SendDataMulti_Bridge_p)(BYTE) = &ZCB_ComplHandler_ZW_SendDataMulti_Bridge;
/*================   ComplHandler_ZW_SendDataMulti_Bridge   ==================
**    Completion handler for ZW_SendDataMulti
**
**--------------------------------------------------------------------------*/
void                             /* RET  Nothing                     */
ZCB_ComplHandler_ZW_SendDataMulti_Bridge(
  BYTE txStatus)                        /* IN   Transmit completion status  */
{
  BYTE_IN_AR(compl_workbuf, 0) = funcID_ComplHandler_ZW_SendDataMulti;
  BYTE_IN_AR(compl_workbuf, 1) = txStatus;
  Request(FUNC_ID_ZW_SEND_DATA_MULTI_BRIDGE, compl_workbuf, 2);
}
#endif


#if SUPPORT_ZW_SEND_DATA_MULTI_EX
code const void (code * ZCB_ComplHandler_ZW_SendDataMultiEx_p)(BYTE) = &ZCB_ComplHandler_ZW_SendDataMultiEx;
/*=====================   ComplHandler_ZW_SendDataMulti   ====================
**    Completion handler for ZW_SendDataMulti
**
**--------------------------------------------------------------------------*/
void
ZCB_ComplHandler_ZW_SendDataMultiEx(
  BYTE txStatus)
{
  BYTE_IN_AR(compl_workbuf, 0) = funcID_ComplHandler_ZW_SendDataMultiEx;
  BYTE_IN_AR(compl_workbuf, 1) = txStatus;
  Request(FUNC_ID_ZW_SEND_DATA_MULTI_EX, compl_workbuf, 2);
}
#endif


#if SUPPORT_STORE_NODEINFO
code const void (code * ZCB_ComplHandler_ZW_StoreNodeInfo_p)(void) = &ZCB_ComplHandler_ZW_StoreNodeInfo;
/*===============   ComplHandler_ZW_StoreNodeInfo  ===============
**    Completion handler for ZW_STORE_NODEINFO
**
**--------------------------------------------------------------------------*/
void
ZCB_ComplHandler_ZW_StoreNodeInfo(void)
{
  BYTE_IN_AR(compl_workbuf, 0) = funcID_ComplHandler_ZW_StoreNodeInfo;
  Request(FUNC_ID_STORE_NODEINFO,compl_workbuf, 1);
}
#endif /* SUPPORT_STORE_NODEINFO */


#if SUPPORT_ZW_SET_SUC_NODE_ID
code const void (code * ZCB_ComplHandler_ZW_SetSUCNodeID_p)(BYTE txStatus, TX_STATUS_TYPE *txStatusReport) = &ZCB_ComplHandler_ZW_SetSUCNodeID;
/*====================   ComplHandler_ZW_SetSUCNodeID   ======================
**    Function description
**    ZW_SUC_SET_SUCCEEDED
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ZCB_ComplHandler_ZW_SetSUCNodeID(
  BYTE txStatus,          /*IN   Completion status*/
  TX_STATUS_TYPE *txStatusReport)
{
  UNUSED(txStatusReport);
  BYTE_IN_AR(compl_workbuf, 0) = funcID_ComplHandler_ZW_SetSUCNodeID;
  BYTE_IN_AR(compl_workbuf, 1) = txStatus;
  Request(FUNC_ID_ZW_SET_SUC_NODE_ID, compl_workbuf, 2);
}
#endif


#if SUPPORT_ZW_SEND_DATA_MULTI
code const void (code * ZCB_ComplHandler_ZW_SendDataMulti_p)(BYTE) = &ZCB_ComplHandler_ZW_SendDataMulti;
/*=====================   ComplHandler_ZW_SendDataMulti   ====================
**    Completion handler for ZW_SendDataMulti
**
**--------------------------------------------------------------------------*/
void                             /* RET  Nothing                     */
ZCB_ComplHandler_ZW_SendDataMulti(
  BYTE txStatus)                        /* IN   Transmit completion status  */
{
  BYTE_IN_AR(compl_workbuf, 0) = funcID_ComplHandler_ZW_SendDataMulti;
  BYTE_IN_AR(compl_workbuf, 1) = txStatus;
  Request(FUNC_ID_ZW_SEND_DATA_MULTI, compl_workbuf, 2);
}
#endif


#if SUPPORT_ZW_REQUEST_NODE_INFO
code const void (code * ZCB_ComplHandler_ZW_RequestNodeInfo_p)(BYTE txStatus, TX_STATUS_TYPE *txStatusReport) = &ZCB_ComplHandler_ZW_RequestNodeInfo;
/*====================== ComplHandler_ZW_RequestNodeInfo =====================
**    Completion handler for ZW_RequestNodeInfo
**
**--------------------------------------------------------------------------*/
void                             /* RET  Nothing                     */
ZCB_ComplHandler_ZW_RequestNodeInfo(
  BYTE txStatus,                        /* IN   Transmit completion status  */
  TX_STATUS_TYPE *txStatusReport)
{
  /* */
  UNUSED(txStatusReport);
  if (txStatus != TRANSMIT_COMPLETE_OK)
  {
#ifdef ZW_CONTROLLER
    ApplicationControllerUpdate(UPDATE_STATE_NODE_INFO_REQ_FAILED, 0, NULL, 0);
#else
    ApplicationSlaveUpdate(UPDATE_STATE_NODE_INFO_REQ_FAILED, 0, NULL, 0);
#endif
  }
}
#endif


#if SUPPORT_ZW_SET_LEARN_NODE_STATE
code const void (code * ZCB_ComplHandler_ZW_SetLearnNodeState_p)(LEARN_INFO *learnNodeInfo) = &ZCB_ComplHandler_ZW_SetLearnNodeState;
/*==================   ZCB_ComplHandler_ZW_SetLearnNodeState   ===============
**    Completion handler for ZW_SetLearnNodeState
**
**--------------------------------------------------------------------------*/
void                         /* RET  Nothing                         */
ZCB_ComplHandler_ZW_SetLearnNodeState(
  LEARN_INFO *learnNodeInfo)
{
  BYTE_IN_AR(compl_workbuf, 0) = funcID_ComplHandler_ZW_SetLearnNodeState;
  {
    BYTE_IN_AR(compl_workbuf, 1) = (*learnNodeInfo).bStatus;
    BYTE_IN_AR(compl_workbuf, 2) = (*learnNodeInfo).bSource;
    /*  - Buffer boundary check */
    if ((*learnNodeInfo).bLen > BUF_SIZE_TX - 4)
    {
      (*learnNodeInfo).bLen = BUF_SIZE_TX - 4;
    }
    BYTE_IN_AR(compl_workbuf, 3) = (*learnNodeInfo).bLen;
    for (i = 0; i < (*learnNodeInfo).bLen; i++)
    {
      BYTE_IN_AR(compl_workbuf, 4 + i) = (*learnNodeInfo).pCmd[i];
    }
    Request(FUNC_ID_ZW_SET_LEARN_NODE_STATE, compl_workbuf, (*learnNodeInfo).bLen + 4);
  }
}
#endif


#if SUPPORT_ZW_REQUEST_NODE_NEIGHBOR_UPDATE
code const void (code * ZCB_ComplHandler_ZW_RequestNodeNeighborUpdate_p)(BYTE txStatus) = &ZCB_ComplHandler_ZW_RequestNodeNeighborUpdate;
/*===============   ComplHandler_ZW_RequestNodeNeighborUpdate  ===============
**    Completion handler for ZW_REQUEST_NODE_NEIGHBOR_UPDATE
**
**--------------------------------------------------------------------------*/
void                                 /* RET  Nothing                    */
ZCB_ComplHandler_ZW_RequestNodeNeighborUpdate(
  BYTE txStatus)                              /* IN   Transmit completion status */
{
  BYTE_IN_AR(compl_workbuf, 0) = funcID_ComplHandler_ZW_RequestNodeNeighborUpdate;
  BYTE_IN_AR(compl_workbuf, 1) = txStatus;
  Request(FUNC_ID_ZW_REQUEST_NODE_NEIGHBOR_UPDATE, compl_workbuf, 2);
}
#endif


#if SUPPORT_ZW_SET_DEFAULT
code const void (code * ZCB_ComplHandler_ZW_SetDefault_p)(void) = &ZCB_ComplHandler_ZW_SetDefault;
/*=====================   ComplHandler_ZW_SetDefault   =============
**    Completion handler for ZW_SetDefault
**
**--------------------------------------------------------------------------*/
void                            /* RET  Nothing */
ZCB_ComplHandler_ZW_SetDefault(void)    /* IN   Nothing */
{
  BYTE_IN_AR(compl_workbuf, 0) = funcID_ComplHandler_ZW_SetDefault;
  Request(FUNC_ID_ZW_SET_DEFAULT, compl_workbuf, 1);
#ifdef ZW_ENABLE_RTC
  ZW_RTC_INIT();
#endif
}
#endif  /* SUPPORT_ZW_SET_DEFAULT */


#ifdef ZW_CONTROLLER
code const void (code * ZCB_ComplHandler_ZW_NodeManagement_p)(LEARN_INFO *learnNodeInfo) = &ZCB_ComplHandler_ZW_NodeManagement;
/*=======================   ComplHandler_ZW_NodeManagement   =================
**    Completion handler for ZW_AddNodeToNetwork, ZW_RemoveNodeFromNetwork
**    ZW_CreateNewPrimary, ZW_ControllerChange and ZW_SetLearnMode for
**    controller based applications
**
**--------------------------------------------------------------------------*/
void                                /* RET  Nothing */
ZCB_ComplHandler_ZW_NodeManagement(
  LEARN_INFO *learnNodeInfo)
{
#if SUPPORT_ADD_REMOVE_PROTECT
  addState = (*learnNodeInfo).bStatus;
#endif
  BYTE_IN_AR(compl_workbuf, 0) = funcID_ComplHandler_ZW_NodeManagement;
  BYTE_IN_AR(compl_workbuf, 1) = (*learnNodeInfo).bStatus;
  BYTE_IN_AR(compl_workbuf, 2) = (*learnNodeInfo).bSource;
  /*  - Buffer boundary check */
  if ((*learnNodeInfo).bLen > BUF_SIZE_TX - 4)
  {
    (*learnNodeInfo).bLen = BUF_SIZE_TX - 4;
  }
  BYTE_IN_AR(compl_workbuf, 3) = (*learnNodeInfo).bLen;
  for (i = 0; i < (*learnNodeInfo).bLen; i++)
  {
    BYTE_IN_AR(compl_workbuf, 4 + i) = (*learnNodeInfo).pCmd[i];
  }
  Request(nodeManagement_Func_ID, compl_workbuf, (*learnNodeInfo).bLen + 4);
}


#if SUPPORT_ADD_REMOVE_PROTECT
BOOL
ZW_NodeManagementRunning()
{
  return(addState==ADD_NODE_STATUS_NODE_FOUND ||
		 addState==ADD_NODE_STATUS_ADDING_SLAVE ||
		 addState==ADD_NODE_STATUS_ADDING_CONTROLLER);
}
#endif
#endif /* ZW_CONTROLLER */


#if SUPPORT_ZW_REPLICATION_SEND_DATA
code const void (code * ZCB_ComplHandler_ZW_ReplicationSendData_p)(BYTE) = &ZCB_ComplHandler_ZW_ReplicationSendData;
/*=====================   ComplHandler_ZW_ReplicationSendData   =============
**    Completion handler for ZW_ReplicationSendData
**
**--------------------------------------------------------------------------*/
void                                    /* RET  Nothing */
ZCB_ComplHandler_ZW_ReplicationSendData(
  BYTE txStatus)                          /* IN   Transmit completion status */
{
  BYTE_IN_AR(compl_workbuf, 0) = funcID_ComplHandler_ZW_ReplicationSendData;
  BYTE_IN_AR(compl_workbuf, 1) = txStatus;
  Request(FUNC_ID_ZW_REPLICATION_SEND_DATA, compl_workbuf, 2);
}
#endif


#if SUPPORT_ZW_ASSIGN_RETURN_ROUTE
code const void (code * ZCB_ComplHandler_ZW_AssignReturnRoute_p)(BYTE bStatus, TX_STATUS_TYPE *txStatusReport) = &ZCB_ComplHandler_ZW_AssignReturnRoute;
/*=====================   ComplHandler_ZW_AssignReturnRoute   =============
**    Completion handler for ZW_AssignReturnRoute
**
**--------------------------------------------------------------------------*/
void                                    /* RET  Nothing                     */
ZCB_ComplHandler_ZW_AssignReturnRoute(
  BYTE bStatus,
  TX_STATUS_TYPE *txStatusReport)       /* IN   Transmit completion status  */
{
  BYTE bIdx = 0;
  BYTE_IN_AR(compl_workbuf, bIdx++) = funcID_ComplHandler_ZW_AssignReturnRoute;
  BYTE_IN_AR(compl_workbuf, bIdx++) = bStatus;
  if (bTxStatusReportEnabled
      && txStatusReport)  /* Check if detailed info is available from protocol */
  {
    memcpy(&compl_workbuf[bIdx], (BYTE*)txStatusReport, sizeof(TX_STATUS_TYPE));
    bIdx += sizeof(TX_STATUS_TYPE);
  }
  Request(FUNC_ID_ZW_ASSIGN_RETURN_ROUTE, compl_workbuf, bIdx);
}
#endif


#if SUPPORT_ZW_ASSIGN_PRIORITY_RETURN_ROUTE
code const void (code * ZCB_ComplHandler_ZW_AssignPriorityReturnRoute_p)(BYTE bStatus, TX_STATUS_TYPE *txStatusReport) = &ZCB_ComplHandler_ZW_AssignPriorityReturnRoute;
/*===============   ZCB_ComplHandler_ZW_AssignPriorityReturnRoute   =========
**    Completion handler for ZW_AssignPriorityReturnRoute
**
**--------------------------------------------------------------------------*/
void                                    /* RET  Nothing                     */
ZCB_ComplHandler_ZW_AssignPriorityReturnRoute(
  BYTE bStatus,                         /* IN   Transmit completion status  */
  TX_STATUS_TYPE *txStatusReport)
{
  BYTE bIdx = 0;
  BYTE_IN_AR(compl_workbuf, bIdx++) = funcID_ComplHandler_ZW_AssignPriorityReturnRoute;
  BYTE_IN_AR(compl_workbuf, bIdx++) = bStatus;
  if (bTxStatusReportEnabled
      && txStatusReport)  /* Check if detailed info is available from protocol */
  {
    memcpy(&compl_workbuf[bIdx], (BYTE*)txStatusReport, sizeof(TX_STATUS_TYPE));
    bIdx += sizeof(TX_STATUS_TYPE);
  }
  Request(FUNC_ID_ZW_ASSIGN_PRIORITY_RETURN_ROUTE, compl_workbuf, bIdx);
}
#endif


#if SUPPORT_ZW_SEND_SUC_ID
code const void (code * ZCB_ComplHandler_ZW_SendSUC_ID_p)(BYTE bStatus, TX_STATUS_TYPE *txStatusReport) = &ZCB_ComplHandler_ZW_SendSUC_ID;
void
ZCB_ComplHandler_ZW_SendSUC_ID(
  BYTE bStatus,
  TX_STATUS_TYPE *txStatusReport)
{
  UNUSED(txStatusReport);
  BYTE_IN_AR(compl_workbuf, 0) = funcID_ComplHandler_ZW_SendSUC_ID;
  BYTE_IN_AR(compl_workbuf, 1) = bStatus;
  Request(FUNC_ID_ZW_SEND_SUC_ID, compl_workbuf, 2);
}
#endif


#if SUPPORT_ZW_DELETE_RETURN_ROUTE
code const void (code * ZCB_ComplHandler_ZW_DeleteReturnRoute_p)(BYTE bStatus, TX_STATUS_TYPE *txStatusReport) = &ZCB_ComplHandler_ZW_DeleteReturnRoute;
/*=====================   ComplHandler_ZW_DeleteReturnRoute   =============
**    Completion handler for ZW_DeleteReturnRoute
**
**--------------------------------------------------------------------------*/
void                                    /* RET  Nothing                     */
ZCB_ComplHandler_ZW_DeleteReturnRoute(
  BYTE bStatus,
  TX_STATUS_TYPE *txStatusReport)       /* IN   Transmit completion status  */
{
  BYTE bIdx = 0;
  BYTE_IN_AR(compl_workbuf, bIdx++) = funcID_ComplHandler_ZW_DeleteReturnRoute;
  BYTE_IN_AR(compl_workbuf, bIdx++) = bStatus;
  if (bTxStatusReportEnabled  /* Do HOST want txStatusReport */
      && txStatusReport)      /* Check if detailed info is available from protocol */
  {
    memcpy(&compl_workbuf[bIdx], (BYTE*)txStatusReport, sizeof(TX_STATUS_TYPE));
    bIdx += sizeof(TX_STATUS_TYPE);
  }
  Request(FUNC_ID_ZW_DELETE_RETURN_ROUTE, compl_workbuf, bIdx);
}
#endif


#if SUPPORT_ZW_REMOVE_FAILED_NODE_ID
code const void (code * ZCB_ComplHandler_ZW_RemoveFailedNodeID_p)(BYTE bStatus) = &ZCB_ComplHandler_ZW_RemoveFailedNodeID;
/*=====================   ComplHandler_ZW_RemoveFailedNodeID   ==============
**    Completion handler for ZW_RemoveFailedNodeID
**
**--------------------------------------------------------------------------*/
void                                    /* RET  Nothing                     */
ZCB_ComplHandler_ZW_RemoveFailedNodeID(
  BYTE bStatus)                         /* IN   Transmit completion status  */
{
  BYTE_IN_AR(compl_workbuf, 0) = funcID_ComplHandler_ZW_RemoveFailedNodeID;
  BYTE_IN_AR(compl_workbuf, 1) = bStatus;
  Request(FUNC_ID_ZW_REMOVE_FAILED_NODE_ID, compl_workbuf, 2);
}
#endif


#if SUPPORT_ZW_REPLACE_FAILED_NODE
code const void (code * ZCB_ComplHandler_ZW_ReplaceFailedNode_p)(BYTE bStatus) = &ZCB_ComplHandler_ZW_ReplaceFailedNode;
/*=====================   ComplHandler_ZW_RemoveFailedNodeID   ==============
**    Completion handler for ZW_RemoveFailedNodeID
**
**--------------------------------------------------------------------------*/
void                                    /* RET  Nothing                     */
ZCB_ComplHandler_ZW_ReplaceFailedNode(
  BYTE bStatus)                         /* IN   Transmit completion status  */
{
  BYTE_IN_AR(compl_workbuf, 0) = funcID_ComplHandler_ZW_ReplaceFailedNode;
  BYTE_IN_AR(compl_workbuf, 1) = bStatus;
  Request(FUNC_ID_ZW_REPLACE_FAILED_NODE, compl_workbuf, 2);
}
#endif


#if defined(ZW_SLAVE_ROUTING) || defined(ZW_CONTROLLER)
code const void (code * ZCB_ComplHandler_ZW_netWork_Management_p)(BYTE bStatus, TX_STATUS_TYPE *txStatusReport) = &ZCB_ComplHandler_ZW_netWork_Management;
/*=====================   ComplHandler_ZW_netWork_Management   ===============
**    Completion handler for the network management functionality
**
**--------------------------------------------------------------------------*/
void                                    /* RET  Nothing                     */
ZCB_ComplHandler_ZW_netWork_Management(
  BYTE bStatus,                         /* IN   Transmit completion status  */
  TX_STATUS_TYPE *txStatusReport)       /* IN Detailed transmit information */
{
  BYTE bIdx = 0;
  BYTE_IN_AR(compl_workbuf, bIdx++) = funcID_ComplHandler_netWork_Management;
  BYTE_IN_AR(compl_workbuf, bIdx++) = bStatus;
  if (bTxStatusReportEnabled
      && txStatusReport)  /* Check if detailed info is available from protocol */
  {
    memcpy(&compl_workbuf[bIdx], (BYTE*)txStatusReport, sizeof(TX_STATUS_TYPE));
    bIdx += sizeof(TX_STATUS_TYPE);
  }
  Request(management_Func_ID, compl_workbuf, bIdx);
}
#endif /*ZW_SLAVE_32 ZW_CONTROLLER*/


#ifdef ZW_SLAVE
#if SUPPORT_ZW_SET_LEARN_MODE
code const void (code * ZCB_ComplHandler_ZW_SetLearnMode_p)(BYTE, BYTE) = &ZCB_ComplHandler_ZW_SetLearnMode;
/*=========================   ComplHandler_ZW_SetLearnMode   =================
**    Completion handler for ZW_SetLearnMode
**
**--------------------------------------------------------------------------*/
void                             /*RET Nothing                       */
ZCB_ComplHandler_ZW_SetLearnMode(
  BYTE bStatus,                         /*IN  ZW_SetLearnMode status        */
  BYTE bNodeID)                         /*IN  Node ID                       */
{
  BYTE_IN_AR(compl_workbuf, 0) = funcID_ComplHandler_ZW_SetLearnMode;
  BYTE_IN_AR(compl_workbuf, 1) = bStatus;
  BYTE_IN_AR(compl_workbuf, 2) = bNodeID;
  /* For safty we transmit len = 0, to indicate that no data follows */
  BYTE_IN_AR(compl_workbuf, 3) = 0;
  Request(FUNC_ID_ZW_SET_LEARN_MODE, compl_workbuf, 4);
}
#endif  /* SUPPORT_ZW_SET_LEARN_MODE */

#if SUPPORT_ZW_REQUEST_NEW_ROUTE_DESTINATIONS
#ifdef ZW_SLAVE_ROUTING
code const void (code * ZCB_ComplHandler_ZW_RequestNewRouteDestinations_p)(BYTE) = &ZCB_ComplHandler_ZW_RequestNewRouteDestinations;
/*============   ZCB_ComplHandler_ZW_RequestNewRouteDestinations   ===========
**    Completion handler for ZW_RequestNewRouteDestinations
**
**--------------------------------------------------------------------------*/
void                                    /* RET  Nothing                     */
ZCB_ComplHandler_ZW_RequestNewRouteDestinations(
  BYTE bStatus)                          /* IN  Function status */
{
  PreRequestPar2(FUNC_ID_ZW_REQUEST_NEW_ROUTE_DESTINATIONS, funcID_ComplHandler_ZW_RequestNewRouteDestinations, bStatus);
}
#endif  /* ZW_SLAVE_ROUTING */
#endif  /* SUPPORT_ZW_REQUEST_NEW_ROUTE_DESTINATIONS */

#endif  /* ZW_SLAVE */


#if SUPPORT_ZW_SET_SLAVE_LEARN_MODE
code const void (code * ZCB_ComplHandler_ZW_SetSlaveLearnMode_p)(BYTE, BYTE, BYTE) = &ZCB_ComplHandler_ZW_SetSlaveLearnMode;
/*=================   ComplHandler_ZW_SetSlaveLearnMode   ====================
**    Completion handler for ZW_SetSlaveLearnMode
**
**--------------------------------------------------------------------------*/
void                             /* RET  Nothing                     */
ZCB_ComplHandler_ZW_SetSlaveLearnMode(
  BYTE bStatus,
  BYTE orgID,
  BYTE newID)                           /*  IN  Node ID                     */
{
  BYTE_IN_AR(compl_workbuf, 0) = funcID_ComplHandler_ZW_SetSlaveLearnMode;
  BYTE_IN_AR(compl_workbuf, 1) = bStatus;
  BYTE_IN_AR(compl_workbuf, 2) = orgID;
  BYTE_IN_AR(compl_workbuf, 3) = newID;
  Request(FUNC_ID_ZW_SET_SLAVE_LEARN_MODE, compl_workbuf, 4);
}
#endif

#if SUPPORT_ZW_SEND_TEST_FRAME
code const void (code * ZCB_ComplHandler_ZW_SendTestFrame_p)(BYTE txStatus, TX_STATUS_TYPE *txStatusReport) = &ZCB_ComplHandler_ZW_SendTestFrame;
/*=================    ComplHandler_ZW_SendTestFrame    ====================
**    Completion handler for ZW_SendTestFrame
**
**--------------------------------------------------------------------------*/
void
ZCB_ComplHandler_ZW_SendTestFrame(
  BYTE txStatus,
  TX_STATUS_TYPE *txStatusReport)
{
  UNUSED(txStatusReport);
  BYTE_IN_AR(compl_workbuf, 0) = funcID_ComplHandler_ZW_SendTestFrame;
  BYTE_IN_AR(compl_workbuf, 1) = txStatus;
  Request(FUNC_ID_ZW_SEND_TEST_FRAME, compl_workbuf, 2);
}
#endif

#if SUPPORT_ZW_SEND_SLAVE_NODE_INFORMATION
code const void (code * ZCB_ComplHandler_ZW_SendSlaveNodeInformation_p)(BYTE txStatus, TX_STATUS_TYPE *txStatusReport) = &ZCB_ComplHandler_ZW_SendSlaveNodeInformation;
/*=================   ComplHandler_ZW_SendSlaveNodeInformation   ============
**    Completion handler for ZW_SendSlaveNodeInformation
**
**--------------------------------------------------------------------------*/
void                             /* RET  Nothing                     */
ZCB_ComplHandler_ZW_SendSlaveNodeInformation(
  BYTE txStatus,                        /* IN   Transmit completion status  */
  TX_STATUS_TYPE *txStatusReport)
{
  UNUSED(txStatusReport);
  BYTE_IN_AR(compl_workbuf, 0) = funcID_ComplHandler_ZW_SendSlaveNodeInformation;
  BYTE_IN_AR(compl_workbuf, 1) = txStatus;
  Request(FUNC_ID_ZW_SEND_SLAVE_NODE_INFORMATION, compl_workbuf, 2);
}
#endif


#if SUPPORT_ZW_SEND_SLAVE_DATA
/*===================   ComplHandler_ZW_SendSlaveData   ======================
**    Completion handler for ZW_SendData
**
**--------------------------------------------------------------------------*/
static void                             /* RET  Nothing                     */
ComplHandler_ZW_SendSlaveData(
  BYTE txStatus)                        /* IN   Transmit completion status  */
{
  BYTE_IN_AR(compl_workbuf, 0) = funcID_ComplHandler_ZW_SendSlaveData;
  BYTE_IN_AR(compl_workbuf, 1) = txStatus;
  Request(FUNC_ID_ZW_SEND_SLAVE_DATA, compl_workbuf, 2);
}
#endif


#if SUPPORT_MEMORY_PUT_BUFFER
code const void (code * ZCB_ComplHandler_MemoryPutBuffer_p)(void) = &ZCB_ComplHandler_MemoryPutBuffer;
/*=====================   ComplHandler_MemoryPutBuffer   =============
**    Completion handler for MemoryPutBuffer
**
**--------------------------------------------------------------------------*/
void                                /* RET  Nothing */
ZCB_ComplHandler_MemoryPutBuffer(void)  /* IN   Nothing */
{
  BYTE_IN_AR(compl_workbuf, 0) = funcID_ComplHandler_MemoryPutBuffer;
  Request(FUNC_ID_MEMORY_PUT_BUFFER, compl_workbuf, 1);
}
#endif



#if SUPPORT_ZW_NUNIT
code const BYTE (code * ZCB_ZW_NUnitCmd_p)(BYTE cmd, XBYTE* pMessage,BYTE len) = &ZCB_NUnitCmd;
/*=====================   NUnitCmd   =====================================
**    Send cmd ZW_AssertEqual to PC
**
**--------------------------------------------------------------------------*/
void            /* RET  Nothing */
ZCB_NUnitCmd( BYTE cmd, /*NUnit command*/
  XBYTE* pMessage,      /* NUnit cmd data*/
  BYTE len )           /*lenght of data*/
{
  BYTE i = 0;
  BYTE_IN_AR(compl_workbuf, 0) = cmd;
  ZW_DEBUG_SEND_STR("NUnitCmd:");
  ZW_DEBUG_SEND_NUM(compl_workbuf[0]);

  if(NON_NULL( pMessage ))
  {
    for( i = 0; i < len; i++)
    {
      BYTE_IN_AR(compl_workbuf, 1 + i) = *(pMessage+i);
      ZW_DEBUG_SEND_NUM(compl_workbuf[1+i]);

    }
  }
  Request(FUNC_ID_ZW_NUNIT_CMD, compl_workbuf, 1 + i);
  ZW_DEBUG_SEND_NL();
}

#endif /*SUPPORT_ZW_NUNIT*/



#if SUPPORT_ZW_PORT_STATUS

/*=====================   EventHandler   =====================================
**    Application EventHandler
**
**--------------------------------------------------------------------------*/
static void     /* RET  Nothing */
EventHandler(
  WORD event,       /*HI(ID_EVENT_MODULE),LOW(PORT_EVENT)*/
  XBYTE* pMessage,  /* msg pointer  */
  BYTE len)         /*lenght of data*/
{

  switch(event>>8) /*enum type: ID_EVENT_MODULE*/
  {
    case ID_EVENT_PIO_PORT:
      switch(event&0xff)
      {
        case ID_PORTIN_CHANGE: /*pMessage:(PortMonitor struct: PORT_STATUS)*/
        case ID_PORTOUT_CHANGE: /*pMessage:(PortMonitor struct: PORT_STATUS)*/

          {
            BYTE i = 0;
            BYTE_IN_AR(compl_workbuf, 0) = event&0xff;
            //ZW_DEBUG_SEND_STR("FUNC_ID_PORT_STATUS:");
            //ZW_DEBUG_SEND_NUM(compl_workbuf[0]);

            if(NON_NULL( pMessage ))
            {
              for( i = 0; i < len; i++)
              {
                BYTE_IN_AR(compl_workbuf, 1 + i) = *(pMessage+i);
                //ZW_DEBUG_SEND_NUM(compl_workbuf[1+i]);

              }
            }
            Request(FUNC_ID_IO_PORT_STATUS, compl_workbuf, 1 + i);
            //ZW_DEBUG_SEND_NL();
          }
        break;

        default:
        break;

      }
    break;

    default:
    break;
  }
}
#endif /*SUPPORT_ZW_PORT_STATUS*/

void
DoRespond(void)
{
  Respond(serFrameCmd, &retVal, 1);
}


void
DoRespond_workbuf(
  BYTE cnt
)
{
  Respond(serFrameCmd, compl_workbuf, cnt);
}

#ifdef ZW_ENABLE_RTC
/*====================   ComplHandler_RTCTimerCreate   =======================
**    Completion handler for RTCTimerCreate
**
**--------------------------------------------------------------------------*/
static void                            /* RET  Nothing   */
ComplHandler_RTCTimerCreate()   /* IN   Nothing   */
{
  BYTE_IN_AR(compl_workbuf, 0) = funcID_ComplHandler_RTCTimerCreate;
  Request(FUNC_ID_RTC_TIMER_CREATE, compl_workbuf, 1);
}


void
RespondRTCTimer(void)
{
  BYTE handle = serFrameDataPtr[0];

  BYTE_IN_AR(compl_workbuf, 0) = RTCTimerRead(&handle, &timer);
  BYTE_IN_AR(compl_workbuf, 1) = handle;
  BYTE_IN_AR(compl_workbuf, 2) = timer.status;
  BYTE_IN_AR(compl_workbuf, 3) = timer.timeOn.weekday;
  BYTE_IN_AR(compl_workbuf, 4) = timer.timeOn.hour;
  BYTE_IN_AR(compl_workbuf, 5) = timer.timeOn.minute;
  BYTE_IN_AR(compl_workbuf, 6) = timer.timeOff.weekday;
  BYTE_IN_AR(compl_workbuf, 7) = timer.timeOff.hour;
  BYTE_IN_AR(compl_workbuf, 8) = timer.timeOff.minute;
  BYTE_IN_AR(compl_workbuf, 9) = timer.repeats;
  BYTE_IN_AR(compl_workbuf, 10) = timer.parm;
  DoRespond_workbuf(11);
}
#endif


#ifdef ZW_CONTROLLER
void
SetupNodeManagement(
  BYTE funcID_offet)
{
  ZW_DEBUG_SEND_BYTE('~');
  ZW_DEBUG_SEND_NUM(serFrameCmd);
  ZW_DEBUG_SEND_BYTE('~');
  nodeManagement_Func_ID = serFrameCmd;
  funcID_ComplHandler_ZW_NodeManagement = *(serFrameDataPtr + funcID_offet);
  set_state(stateIdle);
#if SUPPORT_ADD_REMOVE_PROTECT
  addState = 0;
#endif
}
#endif


#ifdef ZW_CONTROLLER_SINGLE
#if SUPPORT_SERIAL_API_TEST
void
SendTestReport(
  BYTE txStatus
)
{
  if (funcID_ComplHandler_Serial_API_Test != 0)
  {
    BYTE_IN_AR(compl_workbuf, 0) = funcID_ComplHandler_Serial_API_Test;
    BYTE_IN_AR(compl_workbuf, 1) = testCmd;
    BYTE_IN_AR(compl_workbuf, 2) = testState;
    BYTE_IN_AR(compl_workbuf, 3) = testNodeID;
    BYTE_IN_AR(compl_workbuf, 4) = txStatus;
    BYTE_IN_AR(compl_workbuf, 5) = BYTE_GET_LOW_BYTE_IN_WORD(testCount);
    Request(FUNC_ID_SERIAL_API_TEST, compl_workbuf, 6);
  }
}


void
SendTestRoundReport(
  BYTE txStatus
)
{
  UNUSED(txStatus);
  if (funcID_ComplHandler_Serial_API_Test != 0)
  {
    BYTE_IN_AR(compl_workbuf, 0) = funcID_ComplHandler_Serial_API_Test;
    BYTE_IN_AR(compl_workbuf, 1) = testCmd;
    BYTE_IN_AR(compl_workbuf, 2) = testState;
    BYTE_IN_AR(compl_workbuf, 3) = BYTE_GET_HIGH_BYTE_IN_WORD(testCount);
    BYTE_IN_AR(compl_workbuf, 4) = BYTE_GET_LOW_BYTE_IN_WORD(testCount);
    /* Initialy we assume every node acked, so we assume no nodemask is to be send */
    i = 0;
    if (ZW_NODE_MASK_BITS_IN(testresultnodemask, testnodemasklen))
    {
      for (; i < testnodemasklen; i++)
      {
        BYTE_IN_AR(compl_workbuf, 6 + i) = testresultnodemask[i];
      }
      BYTE_IN_AR(compl_workbuf, 5) = testnodemasklen;
      i++;
    }
    Request(FUNC_ID_SERIAL_API_TEST, compl_workbuf, 5 + i);
  }
}


code const void (code * ZCB_TestDelayNextSendTimeout_p)(void) = &ZCB_TestDelayNextSendTimeout;
void
ZCB_TestDelayNextSendTimeout(void)
{
  if (--testCurrentDelay == 0)
  {
    if (timerTestHandle != 0xff)
    {
      TimerCancel(timerTestHandle);
    }
    timerTestHandle = 0xff;
    TestSend();
  }
}


code const void (code * ZCB_TestDelayTimeout_p)(void) = &ZCB_TestDelayTimeout;
void
ZCB_TestDelayTimeout(void)
{
  if (--testCurrentDelay == 0)
  {
    if (timerTestHandle != 0xff)
    {
      TimerCancel(timerTestHandle);
    }
    timerTestHandle = 0xff;
    setON = !setON;
    TestStartRound();
  }
}


BOOL
TestFindNextNode(void)
{
  do
  {
    if (ZW_NodeMaskNodeIn(testnodemask, ++testNodeID))
    {
      return TRUE;
    }
  } while (testNodeID < ZW_MAX_NODES);
  return FALSE;
}


code const void (code * ZCB_TestSendComplete_p)(BYTE bStatus, TX_STATUS_TYPE *txStatusReport) = &ZCB_TestSendComplete;
/*===========================  MetaDataSendComplete  =========================
**    Function description
**
**
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ZCB_TestSendComplete(
  BYTE bStatus,
  TX_STATUS_TYPE *txStatusReport
)
{
  UNUSED(txStatusReport);
  ZW_DEBUG_SEND_BYTE('C');
  ZW_DEBUG_SEND_NUM(bStatus);

  if (bStatus == TRANSMIT_COMPLETE_OK)
  {
    testSuccessCount++;
  }
  else
  {
    /* Set bit indicating that node failed to acknowledge */
    ZW_NODE_MASK_SET_BIT(testresultnodemask, testNodeID);
    testFailedCount++;
  }
  /* Should we transmit result (to host) after every transmit or do we send one frame after every round? */
  if (testCmd < 0x05)
  {
    /* One result frame after every transmit */
    SendTestReport(bStatus);
  }
  if (testState == POWERLEVEL_TEST_NODE_REPORT_ZW_TEST_INPROGRESS)
  {
    if (TestFindNextNode())
    {
      testCurrentDelay = testDelay;
      if (timerTestHandle != 0xff)
      {
        TimerCancel(timerTestHandle);
      }
	  timerTestHandle = 0xff;
      if (!testCurrentDelay )
      {
        testCurrentDelay++;
        ZCB_TestDelayNextSendTimeout();
      }
      else
      {
        timerTestHandle = TimerStart(ZCB_TestDelayNextSendTimeout, 1, TIMER_FOREVER);
      }
    }
    else
    {
      if (testCmd >= 0x05)
      {
        /* One result frame after every round */
        SendTestRoundReport(0);
      }
      /* No more nodes in this round - delay (if any delay to be done) before starting next round */
      if (testCount && (--testCount != 0))
      {
        testCurrentDelay = testDelay;
        if (timerTestHandle != 0xff)
        {
          TimerCancel(timerTestHandle);
        }
        timerTestHandle = 0xff;
        if (!testCurrentDelay )
        {
          testCurrentDelay++;
          ZCB_TestDelayTimeout();
        }
        else
        {
          timerTestHandle = TimerStart(ZCB_TestDelayTimeout, 1, TIMER_FOREVER);
        }
      }
      else
      {
        testState = POWERLEVEL_TEST_NODE_REPORT_ZW_TEST_SUCCES;
        SendTestReport(0);
      }
    }
  }
  else
  {
    if (timerTestHandle != 0xff)
    {
      TimerCancel(timerTestHandle);
    }
    timerTestHandle = 0xff;
    SendTestReport(0);
  }
}


/*=================================  TestSend  ===============================
**    Function description
**        Send the next data frame
**
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
TestSend(void)
{
  register BYTE payLoadLen;

  ZW_DEBUG_SEND_BYTE('N');

  if ((testCmd == 0x03) || (testCmd == 0x04) || (testCmd == 0x07) || (testCmd == 0x08))
  {
    txBuffer.ZW_BasicSetFrame.cmdClass = COMMAND_CLASS_BASIC;
    txBuffer.ZW_BasicSetFrame.cmd      = BASIC_SET;
    txBuffer.ZW_BasicSetFrame.value    = setON ? BASIC_ON : BASIC_OFF;
    payLoadLen = sizeof(txBuffer.ZW_BasicSetFrame);
  }
  else
  {
    txBuffer.ZW_ManufacturerSpecificReportFrame.cmdClass = COMMAND_CLASS_MANUFACTURER_PROPRIETARY;
    txBuffer.ZW_ManufacturerSpecificReportFrame.manufacturerId1 = 0x00;
    txBuffer.ZW_ManufacturerSpecificReportFrame.manufacturerId2 = 0x00;
    txBuffer.ZW_ManufacturerSpecificReportFrame.productId1 = 0x04;
    txBuffer.ZW_ManufacturerSpecificReportFrame.productId2 = 0x81;
    *(&(txBuffer.ZW_ManufacturerSpecificReportFrame.productId2) + 1) = 0x00;
    *(&(txBuffer.ZW_ManufacturerSpecificReportFrame.productId2) + 2) = BYTE_GET_HIGH_BYTE_IN_WORD(testCount); //data1
    *(&(txBuffer.ZW_ManufacturerSpecificReportFrame.productId2) + 3) = BYTE_GET_LOW_BYTE_IN_WORD(testCount);  //data1+1
    payLoadLen = sizeof(txBuffer.ZW_ManufacturerSpecificReportFrame);
  }
  /* Send meta data frame */
#if SUPPORT_ZW_SEND_DATA_META
  /* Do we use ZW_SendDataMeta */
  if ((testCmd & 0x01) == 0)
  {
    if (!ZW_SendDataMeta(testNodeID,
                         (BYTE*)&txBuffer,
                         (testPayloadLen > payLoadLen) ? testPayloadLen : payLoadLen,
                         testTxOptions,
                         ZCB_TestSendComplete))
    {
      ZCB_TestSendComplete(TRANSMIT_COMPLETE_NO_ACK, NULL);
    }
  }
  else
#endif
  {
    if (!ZW_SendData(testNodeID,
                     (BYTE*)&txBuffer,
                     (testPayloadLen > payLoadLen) ? testPayloadLen : payLoadLen,
                     testTxOptions,
                     ZCB_TestSendComplete))
    {
      ZCB_TestSendComplete(TRANSMIT_COMPLETE_NO_ACK, NULL);
    }
  }
}


/*============================   TestStartRound   ============================
**    Start a Test round
**
**    This is an application function example
**
**--------------------------------------------------------------------------*/
void
TestStartRound(void)
{
  register BYTE bTemp;

  ZW_NODE_MASK_CLEAR(testresultnodemask, ZW_MAX_NODES / 8);
  /* Fill the meta data frame with data */
  for (bTemp = 0; bTemp < (META_DATA_MAX_DATA_SIZE - sizeof(ZW_MANUFACTURER_SPECIFIC_REPORT_FRAME)); bTemp++)
  {
      *(&(txBuffer.ZW_ManufacturerSpecificReportFrame.productId2) + 1 + bTemp) = bTemp + 1; // was .ZW_MetaDataReportFrame.data1
  }
  testNodeID = 0;
  /* Find first node to transmit to */
  if ((testState == POWERLEVEL_TEST_NODE_REPORT_ZW_TEST_INPROGRESS) && TestFindNextNode())
  {
    /* Found a node */
    TestSend();
  }
  else
  {
    testState = POWERLEVEL_TEST_NODE_REPORT_ZW_TEST_FAILED;
    SendTestReport(0);
  }
}
#endif
#endif

void
SaveApplicationSettings(void)
{
  /* First make NV-mem app. data invalid */
//  ZW_MEM_PUT_BYTE((WORD)&EEOFFSET_MAGIC_far, 0);
#ifdef ZW_SLAVE
  ZW_MEM_PUT_BYTE((WORD)&EEOFFSET_LISTENING_far, applNodeInfo_deviceOptionsMask);
  ZW_MEM_PUT_BYTE((WORD)&EEOFFSET_GENERIC_far, applNodeInfo_nodeType_generic);
  ZW_MEM_PUT_BYTE((WORD)&EEOFFSET_SPECIFIC_far, applNodeInfo_nodeType_specific);
#endif
  /*  - Buffer boundary check */
  if (applNodeInfo_parmLength > APPL_NODEPARM_MAX)
  {
    applNodeInfo_parmLength = APPL_NODEPARM_MAX;
  }
  ZW_MEM_PUT_BYTE((WORD)&EEOFFSET_CMDCLASS_LEN_far, applNodeInfo_parmLength);
  for (i = 0; i < applNodeInfo_parmLength; i++)
  {
    ZW_MEM_PUT_BYTE((WORD)&EEOFFSET_CMDCLASS_far + i, applNodeInfo_nodeParm[i]);
  }
#if SUPPORT_ZW_WATCHDOG_START | SUPPORT_ZW_WATCHDOG_STOP
  ZW_MEM_PUT_BYTE((WORD)&EEOFFSET_WATCHDOG_STARTED_far, bWatchdogStarted);
#else
  ZW_MEM_PUT_BYTE((WORD)&EEOFFSET_WATCHDOG_STARTED_far, FALSE);
#endif
#if SUPPORT_SERIAL_API_APPL_NODE_INFORMATION_CMD_CLASSES
  /* Unincluded Supported Command Classes */
  if (applNodeInfo_unincluded_parmLength > APPL_NODEPARM_MAX)
  {
    applNodeInfo_unincluded_parmLength = APPL_NODEPARM_MAX;
  }
  ZW_MEM_PUT_BYTE((WORD)&EEOFFSET_CMDCLASS_UNINCLUDED_LEN_far, applNodeInfo_unincluded_parmLength);
  for (i = 0; i < applNodeInfo_unincluded_parmLength; i++)
  {
    ZW_MEM_PUT_BYTE((WORD)&EEOFFSET_CMDCLASS_UNINCLUDED_far + i, applNodeInfo_unincluded_nodeParm[i]);
  }
  /* Included unsecure Supported Command Classes */
  if (applNodeInfo_included_unsecure_parmLength > APPL_NODEPARM_MAX)
  {
    applNodeInfo_included_unsecure_parmLength = APPL_NODEPARM_MAX;
  }
  ZW_MEM_PUT_BYTE((WORD)&EEOFFSET_CMDCLASS_INCLUDED_UNSECURE_LEN_far, applNodeInfo_included_unsecure_parmLength);
  for (i = 0; i < applNodeInfo_included_unsecure_parmLength; i++)
  {
    ZW_MEM_PUT_BYTE((WORD)&EEOFFSET_CMDCLASS_INCLUDED_UNSECURE_far + i, applNodeInfo_included_unsecure_nodeParm[i]);
  }
  /* Included secure Supported Command Classes */
  if (applNodeInfo_included_secure_parmLength > APPL_NODEPARM_MAX)
  {
    applNodeInfo_included_secure_parmLength = APPL_NODEPARM_MAX;
  }
  ZW_MEM_PUT_BYTE((WORD)&EEOFFSET_CMDCLASS_INCLUDED_SECURE_LEN_far, applNodeInfo_included_secure_parmLength);
  for (i = 0; i < applNodeInfo_included_secure_parmLength; i++)
  {
    ZW_MEM_PUT_BYTE((WORD)&EEOFFSET_CMDCLASS_INCLUDED_SECURE_far + i, applNodeInfo_included_secure_nodeParm[i]);
  }
#endif
#ifndef slave_routing
  /* Save powerlevel settings (if any has been set) */
  for (i = 0; i < POWERLEVEL_CHANNELS; i++)
  {
    ZW_MEM_PUT_BYTE((WORD)&EEOFFSET_POWERLEVEL_NORMAL_far + i, sPowerlevels.normal[i]);
    ZW_MEM_PUT_BYTE((WORD)&EEOFFSET_POWERLEVEL_LOW_far + i, sPowerlevels.low[i]);
  }
#endif
#ifdef ZW_SMARTSTART_ENABLED
  ZW_MEM_PUT_BYTE((WORD)&EEOFFSET_MODULE_POWER_MODE_EXTINT_ENABLE_far, applPowerModeExtIntEnable);
  ZW_MEM_PUT_BYTE((WORD)&EEOFFSET_MODULE_POWER_MODE_far, applPowerMode);

  ZW_MEM_PUT_BYTE((WORD)&EEOFFSET_MODULE_POWER_MODE_WUT_TIMEOUT_far, (BYTE)((applPowerModeWutTimeOut & 0xFF000000) >> 24));
  ZW_MEM_PUT_BYTE((WORD)(&EEOFFSET_MODULE_POWER_MODE_WUT_TIMEOUT_far + 1), (BYTE)((applPowerModeWutTimeOut & 0xFF0000) >> 16));
  ZW_MEM_PUT_BYTE((WORD)(&EEOFFSET_MODULE_POWER_MODE_WUT_TIMEOUT_far + 2), (BYTE)((applPowerModeWutTimeOut & 0xFF00) >> 8));
  ZW_MEM_PUT_BYTE((WORD)(&EEOFFSET_MODULE_POWER_MODE_WUT_TIMEOUT_far + 3), (BYTE)(applPowerModeWutTimeOut & 0xFF));
#endif
  /* Now app. data is valid! */
  ZW_MEM_PUT_BYTE((WORD)&EEOFFSET_MAGIC_far, MAGIC_VALUE);
}


#if SUPPORT_SERIAL_API_POWER_MANAGEMENT
code const void (code * ZCB_powerManagementPoweredUpPinActive_p)(void) = &ZCB_powerManagementPoweredUpPinActive;
void
ZCB_powerManagementPoweredUpPinActive(void)
{
  if (poweredUp)
  {
    if (ZW_PortPinGet(powerManagementPoweredUp.bPin) == powerManagementPoweredUp.bEnableLevel)
    {
      poweredUpTransition = 0;
    }
  }
  else
  {
    if (ZW_PortPinGet(powerManagementPoweredUp.bPin) == !powerManagementPoweredUp.bEnableLevel)
    {
      poweredUpTransition = 0;
    }
  }
  if (++poweredUpTransition >= PM_PIN_IN_DEBOUNCE)
  {
    poweredUp = !poweredUp;
    poweredUpTransition = 0;
  }
}


code const void (code * ZCB_PowerManagementWakeUpOnExternalActive_p)(void) = &ZCB_PowerManagementWakeUpOnExternalActive;
void
ZCB_PowerManagementWakeUpOnExternalActive(void)
{
  if (!wakeUpOnExternal)
  {
    if (ZW_PortPinGet(powerManagementWakeUpOnExternal.bPin) == !powerManagementWakeUpOnExternal.bEnableLevel)
    {
      wakeUpOnExternalTransition = 0;
    }
  }
  if (++wakeUpOnExternalTransition >= PM_PIN_IN_DEBOUNCE)
  {
    wakeUpOnExternal = TRUE;
    ZW_TIMER_CANCEL(wakeUpOnExternalTimerHandle);
    wakeUpOnExternalTimerHandle = -1;
  }
}


code const void (code * ZCB_PowerManagementWakeUpOnTimerHandler_p)(void) = &ZCB_PowerManagementWakeUpOnTimerHandler;
void
ZCB_PowerManagementWakeUpOnTimerHandler(void)
{
  if (0 == --wakeUpOnTimerRes)
  {
    if (0 == --wakeUpOnTimerCount)
    {
      wakeUpOnTimer = TRUE;
      powerManagementWakeUpReason = (PM_TIMER_MINUTES == powerManagementWakeUpOnTimer)
                                    ? PM_WAKEUP_REASON_TIMER_MINUTES
                                    : PM_WAKEUP_REASON_TIMER_SECONDS;
      ZW_TIMER_CANCEL(wakeUpOnTimerHandle);
      wakeUpOnTimerHandle = -1;
    }
    else
    {
      wakeUpOnTimerRes = (PM_TIMER_MINUTES == powerManagementWakeUpOnTimer) ? 60 : 1;
    }
  }
}


void
PowerManagementSetPowerDown(void)
{
  for (i = 0; i < PM_IO_PIN_MAX; i++)
  {
    if (PM_PHYSICAL_PIN_UNDEFINED != powerManagementPowerMode[i].bPin)
    {
      ZW_PortPinOut(powerManagementPowerMode[i].bPin);
      ZW_PortPinSet(powerManagementPowerMode[i].bPin, powerManagementPowerMode[i].bEnableLevel);
    }
  }
  /* Wakeup on External setup */
  if (PM_PHYSICAL_PIN_UNDEFINED != powerManagementWakeUpOnExternal.bPin)
  {
    if (-1 != wakeUpOnExternalTimerHandle)
    {
      ZW_TIMER_CANCEL(wakeUpOnExternalTimerHandle);
    }
    ZW_PortPinIn(powerManagementWakeUpOnExternal.bPin);
    wakeUpOnExternal = 0;
    wakeUpOnExternalTransition = 0;
    wakeUpOnExternalTimerHandle = ZW_TIMER_START(ZCB_PowerManagementWakeUpOnExternalActive, 1, TIMER_FOREVER);
  }

  /* Wakeup on RF setup */
  if (PM_WAKEUP_UNDEFINED != powerManagementWakeUpOnRFMode)
  {
    wakeUpOnRF = FALSE;
  }

  /* Wakeup on Timer setup */
  if (PM_TIMER_UNDEFINED != powerManagementWakeUpOnTimer)
  {
    if (-1 != wakeUpOnTimerHandle)
    {
      ZW_TIMER_CANCEL(wakeUpOnTimerHandle);
    }
    wakeUpOnTimer = FALSE;
    wakeUpOnTimerHandle = ZW_TIMER_START(ZCB_PowerManagementWakeUpOnTimerHandler, TIMER_ONE_SECOND, TIMER_FOREVER);
    wakeUpOnTimerRes = (PM_TIMER_MINUTES == powerManagementWakeUpOnTimer) ? 60 : 1;
    wakeUpOnTimerCount = powerManagementWakeUpOnTimerCount;
  }
  powerManagementWakeUpReason = PM_WAKEUP_REASON_NONE;
}


void
PowerManagementSetPowerUp(void)
{
  /* WakeUp HOST */
  for (i = 0; i < PM_IO_PIN_MAX; i++)
  {
    if (PM_PHYSICAL_PIN_UNDEFINED != powerManagementWakeUpPinConf[i].bPin)
    {
      ZW_PortPinOut(powerManagementWakeUpPinConf[i].bPin);
      ZW_PortPinSet(powerManagementWakeUpPinConf[i].bPin, powerManagementWakeUpPinConf[i].bEnableLevel);
    }
  }
#ifdef POWER_MANAGEMENT_TEST
  if (wakeUpOnExternal)
  {
    ZW_PortPinSet(powerManagementWakeUpPinConf[1].bPin, 0);
  }
  if (wakeUpOnRF)
  {
    ZW_PortPinSet(powerManagementWakeUpPinConf[2].bPin, 0);
  }
  if (wakeUpOnTimer)
  {
    ZW_PortPinSet(powerManagementWakeUpPinConf[3].bPin, 0);
  }
#endif
  if (-1 != wakeUpOnTimerHandle)
  {
    ZW_TIMER_CANCEL(wakeUpOnTimerHandle);
    wakeUpOnTimerHandle = -1;
  }
  if (-1 != wakeUpOnExternalTimerHandle)
  {
    ZW_TIMER_CANCEL(wakeUpOnExternalTimerHandle);
    wakeUpOnExternalTimerHandle = -1;
  }
  wakeUpOnExternal = FALSE;
  wakeUpOnRF = FALSE;
  wakeUpOnTimer = FALSE;
}


void
PowerManagementCheck(void)
{
  switch (powerManagementState)
  {
    case POWER_MODE_RUNNING:
      break;

    case POWER_MODE_POWERDOWN_TRANSITION:
      /* Set HOST powerdown mode if either PowerUp pin is undefined or */
      /* Powerup pin is LOW */
      if ((PM_PHYSICAL_PIN_UNDEFINED == powerManagementPoweredUp.bPin)
          || !poweredUp)
      {
        PowerManagementSetPowerDown();
        powerManagementState = POWER_MODE_POWERDOWN;
      }
      break;

    case POWER_MODE_POWERDOWN:
      /* Check if we are to Wakeup HOST again */
      /* WakeUpOnExternal  */
      if (wakeUpOnExternal)
      {
        poweredUpTransition = 0;
        powerManagementWakeUpReason = PM_WAKEUP_REASON_EXTERNAL;
        powerManagementState = POWER_MODE_RUNNING_TRANSITION;
      }
      if (wakeUpOnRF)
      {
        powerManagementState = POWER_MODE_RUNNING_TRANSITION;
      }
      if (wakeUpOnTimer)
      {
        powerManagementState = POWER_MODE_RUNNING_TRANSITION;
      }
      break;

    case POWER_MODE_RUNNING_TRANSITION:
//      if ((PM_PHYSICAL_PIN_UNDEFINED == powerManagementPoweredUp.bPin)
//          || poweredUp)
      {
        PowerManagementSetPowerUp();
        powerManagementState = POWER_MODE_RUNNING;
      }
      break;

    case POWER_MODE_IDLE:
    default:
      break;
  }
}

#endif


/*===============================   ApplicationPoll   =======================
**    Application poll function, handling the receiving and transmitting
**    communication with the PC.
**
**--------------------------------------------------------------------------*/
#ifdef ZW_SMARTSTART_ENABLED
E_APPLICATION_STATE          /*RET Application active state - if TRUE, ready to powerdown */
ApplicationPoll(
  E_PROTOCOL_STATE bProtocolState) /* IN Protocol current state - If FALSE, ready to shutdown */
#else
void                  /*RET Nothing */
ApplicationPoll(void) /*IN  Nothing */
#endif
{
  BYTE conVal;
#if SUPPORT_SERIAL_API_EXT
  register BYTE in0, in1;
#endif
#if SUPPORT_ZW_GET_RANDOM
  BYTE noRndBytes;
  WORD rndWord;
#endif

#ifdef ZW_SMARTSTART_ENABLED
  bProtocolState = bProtocolState;
#endif

#ifdef ZW_ISD51_DEBUG         /* init ISD51 only when the uVision2 Debugger tries to connect */
    ISDcheck();               /* initialize uVision2 Debugger and continue program run */
#endif
  /* ApplicationPoll is controlled by a statemachine with the four states:
      stateIdle, stateFrameParse, stateTxSerial, stateCbTxSerial.

      stateIdle: If there is anything to transmit do so. -> stateCbTxSerial
                 If not, check if anything is received. -> stateFrameParse
                 If neither, stay in the state
                 Note: frames received while we are transmitting are lost
                 and must be retransmitted by PC

      stateFrameParse: Parse received frame.
                 If the request has no response -> stateIdle
                 If there is an immediate response send it. -> stateTxSerial

      stateTxSerial:  Waits for ack on responses send in stateFrameParse.
                 Retransmit frame as needed.
                 -> stateIdle

      stateCbTxSerial:  Waits for ack on requests send in stateIdle
                  (callback, ApplicationCommandHandler etc).
                 Retransmit frame as needed and remove from callbackqueue when done.
                 -> stateIdle

      The serialapi sample code uses the LEDs as indicator for state (if ENABLE_LEDS is defined)
      and the following are defined:
      LED1 ON, LED2 OFF, LED3 ON - Idle (waiting for action) - stateIdle.
      LED1 OFF, LED2 ON, LED3 OFF - A command has been received from HOST
                and this is being executed (a Response frame will also be send if needed) - stateFrameParse.
      LED1 OFF, LED2 OFF, LED3 ON - Waiting for ACK for Response frame transmitted - stateTxSerial.
      LED1 ON, LED2 ON, LED3 OFF - A Request is transmitted to HOST (next state is
                Waiting for ACK for Request - in stateIdle if any Requests in queue)
      LED1 OFF, LED2 ON, LED3 ON - Waiting for ACK for Request frame transmitted - stateCbTxSerial.

  */

#ifdef NO_UART_INTERRUPT
  /* Background handler for buffered receive/transmit - alternative to interrupt handling */
  SerialBackgroundHandler();
#endif
#ifdef ZW_ENABLE_RTC
  ZW_RTC_TIMER_ACTION();
#endif

#if SUPPORT_ZW_WATCHDOG_START || SUPPORT_ZW_WATCHDOG_STOP
  if (bWatchdogStarted)
  {
    /* If watchdog enabled - kick it... */
    ZW_WATCHDOG_KICK;
  }
#endif

#if SUPPORT_SERIAL_API_POWER_MANAGEMENT
  PowerManagementCheck();
#endif
#if SUPPORT_ZW_PORT_STATUS
  EventScheduler();
#endif

  switch (state)
  {
    case stateIdle :
    {
#if SUPPORT_SERIAL_API_READY
      /* Only empty callback queue for HOST if SERIAL LINK has been established  */
      if (callbackQueue.requestCnt && (SERIAL_LINK_DETACHED != serialLinkState))
#else
      /* Check if there is anything to transmit. If so do it */
      if (callbackQueue.requestCnt)
#endif
      {
#ifdef ENABLE_LEDS
        LedOn(1);
        LedOn(2);
        LedOff(3);
#endif
        ConTxFrame(callbackQueue.requestQueue[callbackQueue.requestOut].wCmd, REQUEST, (XBYTE*)callbackQueue.requestQueue[callbackQueue.requestOut].wBuf, callbackQueue.requestQueue[callbackQueue.requestOut].wLen);
        set_state(stateCallbackTxSerial);
        /* callbackCnt decremented when frame is acknowledged from PC - or timed out after retries */
      }
      else
      {
#if SUPPORT_SERIAL_API_READY
        /* Only empty command queue for HOST if SERIAL LINK has been established  */
        if (commandQueue.requestCnt && (SERIAL_LINK_DETACHED != serialLinkState))
#else
        /* Check if there is anything to transmit. If so do it */
        if (commandQueue.requestCnt)
#endif
        {
#ifdef ENABLE_LEDS
          LedOn(1);
          LedOn(2);
          LedOff(3);
#endif
          ConTxFrame(commandQueue.requestQueue[commandQueue.requestOut].wCmd, REQUEST, (XBYTE*)commandQueue.requestQueue[commandQueue.requestOut].wBuf, commandQueue.requestQueue[commandQueue.requestOut].wLen);
          set_state(stateCommandTxSerial);
          /* commandCnt decremented when frame is acknowledged from PC - or timed out after retries */
        }
        else
        {
#ifdef ENABLE_LEDS
          LedOn(1);
          LedOff(2);
          LedOn(3);
#endif
          /* Nothing to transmit. Check if we received anything */
          if (ConUpdate(TRUE) == conFrameReceived)
          {
#if SUPPORT_SERIAL_API_READY
            /* We have received a frame from HOST so we must assume we are connected */
            serialLinkState = SERIAL_LINK_CONNECTED;
#endif
            /* We got a frame... */
            set_state(stateFrameParse);
          }
        }
      }
    }
    break;

    case stateFrameParse :
    {
#ifdef ENABLE_LEDS
      LedOff(1);
      LedOn(2);
      LedOff(3);
#endif
      /* Parse received frame */
      switch (serFrameCmd)
      {
#if SUPPORT_FUNC_ID_CLEAR_TX_TIMERS
        case FUNC_ID_CLEAR_TX_TIMERS:
          /* HOST->ZW: */
          ZW_ClearTxTimers();
          state = stateIdle;
          break;
#endif  /* SUPPORT_FUNC_ID_CLEAR_TX_TIMERS */

#if SUPPORT_FUNC_ID_GET_TX_TIMERS
        case FUNC_ID_GET_TX_TIMERS:
          /* HOST->ZW: */
          /* ZW->HOST: channel0TxTimer_MSB | channel0TxTimer_Next_MSB | channel0TxTimer_Next_LSB | channel0TxTimerLSB | */
          /*           channel1TxTimer_MSB | channel1TxTimer_Next_MSB | channel1TxTimer_Next_LSB | channel1TxTimerLSB | */
          /*           channel2TxTimer_MSB | channel2TxTimer_Next_MSB | channel2TxTimer_Next_LSB | channel2TxTimerLSB */
          ZW_GetTxTimer(0, (DWORD_P)compl_workbuf);
          ZW_GetTxTimer(1, (DWORD_P)(compl_workbuf + sizeof(DWORD)));
          ZW_GetTxTimer(2, (DWORD_P)(compl_workbuf + (2 * sizeof(DWORD))));

          DoRespond_workbuf(3*sizeof(DWORD));

          break;
#endif  /* SUPPORT_FUNC_ID_GET_TX_TIMERS */

#if SUPPORT_ZW_CLEAR_NETWORK_STATS
        case FUNC_ID_ZW_CLEAR_NETWORK_STATS:
          /* HOST->ZW: */
          /* ZW->HOST: 0x01 */
          ZW_ClearNetworkStats();
          retVal = TRUE;
          DoRespond();
          break;
#endif  /* SUPPORT_ZW_CLEAR_NETWORK_STATS */

#if SUPPORT_ZW_GET_NETWORK_STATS
        case FUNC_ID_ZW_GET_NETWORK_STATS:
          /* HOST->ZW: */
          /* ZW->HOST: wRFTxFrames_MSB | wRFTxFrames_LSB | */
          /*           wRFTxLBTBackOffs_MSB | wRFTxLBTBackOffs_LSB | */
          /*           wRFRxFrames_MSB | wRFRxFrames_LSB */
          /*           wRFRxLRCErrors_MSB | wRFRxLRCErrors_LSB */
          /*           wRFRxCRC16Errors_MSB | wRFRxCRC16Errors_LSB */
          /*           wRFRxForeignHomeID_MSB | wRFRxForeignHomeID_LSB */
          {
            S_NETWORK_STATISTICS sNetSats;
            ZW_GetNetworkStats(&sNetSats);
            memcpy(compl_workbuf, (BYTE*)&sNetSats, sizeof(S_NETWORK_STATISTICS));
            DoRespond_workbuf(sizeof(S_NETWORK_STATISTICS));
          }
          break;
#endif  /* SUPPORT_ZW_GET_NETWORK_STATS */

#if SUPPORT_ZW_SET_RF_RECEIVE_MODE
        case FUNC_ID_ZW_SET_RF_RECEIVE_MODE:
          /* HOST->ZW: mode */
          /* ZW->HOST: retVal */
          retVal = ZW_SetRFReceiveMode(serFrameDataPtr[0]);
          DoRespond();
          break;
#endif  /* SUPPORT_ZW_SET_RF_RECEIVE_MODE */

#if SUPPORT_ZW_REQUEST_NEW_ROUTE_DESTINATIONS
        case FUNC_ID_ZW_REQUEST_NEW_ROUTE_DESTINATIONS:
          /* HOST->ZW: destList[5] | funcID */
          /* destList zero terminated if less than ZW_MAX_RETURN_ROUTE_DESTINATIONS */
          funcID_ComplHandler_ZW_RequestNewRouteDestinations = serFrameDataPtr[5];
          for (i = 0; i < 5; i++)
          {
            if (serFrameDataPtr[i])
            {
              returnRouteDests[i] = serFrameDataPtr[i];
            }
            else
            {
              break;
            }
          }
          retVal = ZW_REQUEST_NEW_ROUTE_DESTINATIONS(returnRouteDests, i,
                                                     serFrameDataPtr[5] ? ZCB_ComplHandler_ZW_RequestNewRouteDestinations : NULL);
          DoRespond();
          break;
#endif  /* SUPPORT_ZW_REQUEST_NEW_ROUTE_DESTINATIONS */

#if SUPPORT_ZW_IS_NODE_WITHIN_DIRECT_RANGE
        case FUNC_ID_ZW_IS_NODE_WITHIN_DIRECT_RANGE:
          /* HOST->ZW: bNodeID */
          /* ZW->HOST: retVal */
          retVal = ZW_IS_NODE_WITHIN_DIRECT_RANGE(serFrameDataPtr[0]);
          DoRespond();
          break;
#endif

#if SUPPORT_ZW_SEND_NODE_INFORMATION
        case FUNC_ID_ZW_SEND_NODE_INFORMATION:
          /* HOST->ZW: destNode | txOptions | funcID */
          /* ZW->HOST: retVal */
          funcID_ComplHandler_ZW_SendNodeInformation = serFrameDataPtr[2];
          retVal = ZW_SendNodeInformation(serFrameDataPtr[0], serFrameDataPtr[1],
                                          (serFrameDataPtr[2] != 0) ? ZCB_ComplHandler_ZW_SendNodeInformation : NULL);
          DoRespond();
          break;
#endif /* SUPPORT_ZW_SEND_NODE_INFORMATION */

#if SUPPORT_ZW_SECURITY_SETUP
        case FUNC_ID_ZW_SECURITY_SETUP:
          /* HOST->ZW: securityFuncID [| bDataLen | abData[bDataLen]] */
          /* ZW->HOST: securityFuncID | bretValLen | retVal[bretValLen] */
          compl_workbuf[0] = serFrameDataPtr[0];
          switch ((eSecuritySetupCmd_t)serFrameDataPtr[0])
          {
#if SUPPORT_ZW_GET_SECURITY_KEYS
            case E_SECURITY_SETUP_CMD_ZW_GET_SECURITY_KEYS:
              /* HOST->ZW: */
              /* ZW->HOST: securityKeys_bitmaskLen(1) | securityKeys_bitmask[securityKeys_bitmaskLen] */
              retVal = ZW_GetSecurityKeys();
              compl_workbuf[1] = 1;
              compl_workbuf[2] = retVal;
              DoRespond_workbuf(3);
              break;

            case E_SECURITY_SETUP_CMD_SET_SECURITY_INCLUSION_REQUESTED_KEYS:
              /* HOST->ZW: registeredSecurityKeysLen(1) | registeredSecurityKeys */
              /* ZW->HOST: retValLen(1) | retVal[retValLen] */
              retVal = FALSE;
              if (((FRAME_LENGTH_MIN + 1 + 1 + 1) == serFrameLen) &&
                  (1 == serFrameDataPtr[1]))
              {
                /* Set the requestedSecurityKeysBits requested by protocol when doing S2 inclusion */
                sRequestedSecuritySettings.requestedSecurityKeysBits = serFrameDataPtr[2];
                /* sRequestedSecuritySettings.requestedSecurityKeysBits are either the initialized */
                /* value (REQUESTED_SECURITY_KEYS) or the value set through the FUNC_ID_ZW_SECURITY_SETUP */
                /* function E_SECURITY_SETUP_CMD_SET_SECURITY_INCLUSION_REQUESTED_KEYS */
                /* As requestedSecurityKeyBits has been changed we MUST call ZW_s2_inclusion_init */
                ZW_s2_inclusion_init();
                retVal = TRUE;
              }
              compl_workbuf[1] = 1;
              compl_workbuf[2] = retVal;
              DoRespond_workbuf(3);
              break;

            case E_SECURITY_SETUP_CMD_SET_SECURITY_INCLUSION_REQUESTED_AUTHENTICATION:
              /* HOST->ZW: registeredSecurityAuthenticationLen(1) | registeredSecurityAuthentication */
              /* ZW->HOST: retValLen(1) | retVal[retValLen] */
              retVal = FALSE;
              if (((FRAME_LENGTH_MIN + 1 + 1 + 1) == serFrameLen) &&
                  (1 == serFrameDataPtr[1]))
              {
                /* Set the requestedSecurityAuthentication requested by protocol when doing S2 inclusion */
                sRequestedSecuritySettings.requestedSecurityAuthentication = serFrameDataPtr[2];
                /* sRequestedSecuritySettings.requestedSecurityAuthentication are either the initialized */
                /* value (REQUESTED_SECURITY_AUTHENTICATION) or the value set through the FUNC_ID_ZW_SECURITY_SETUP */
                /* function E_SECURITY_SETUP_CMD_SET_SECURITY_INCLUSION_REQUESTED_AUTHENTICATION */
                /* As requestedSecurityAuthentication has been changed we MUST call ZW_s2_inclusion_init */
                ZW_s2_inclusion_init();
                retVal = TRUE;
              }
              compl_workbuf[1] = 1;
              compl_workbuf[2] = retVal;
              DoRespond_workbuf(3);
              break;
#endif


#if SUPPORT_ZW_SET_SECURITY_S0_NETWORK_KEY
            case E_SECURITY_SETUP_CMD_ZW_SET_SECURITY_S0_NETWORK_KEY:
              /* HOST->ZW: networkkeyLen(16) | network_key[networkkeyLen] */
              /* ZW->HOST: retValLen(1) | retVal[retValLen] */
              retVal = FALSE;
              if ((NETWORK_KEY_LENGTH == serFrameDataPtr[1]) &&
                  ((FRAME_LENGTH_MIN + 1 + 1 + NETWORK_KEY_LENGTH) == serFrameLen))
              {
                ZW_SetSecurityS0NetworkKey(&serFrameDataPtr[2]);
                retVal = TRUE;
              }
              compl_workbuf[1] = 1;
              compl_workbuf[2] = retVal;
              DoRespond_workbuf(3);
              break;
#endif

#if SUPPORT_ZW_GET_SECURITY_S2_PUBLIC_DSK
            case E_SECURITY_SETUP_CMD_ZW_GET_SECURITY_S2_PUBLIC_DSK:
              /* HOST->ZW: */
              /* ZW->HOST: retValLen(SECURITY_KEY_S2_PUBLIC_DSK_LENGTH) | retVal[retValLen] */
              compl_workbuf[1] = SECURITY_KEY_S2_PUBLIC_DSK_LENGTH;
              ZW_GetSecurityS2PublicDSK(&compl_workbuf[2]);
              DoRespond_workbuf(2 + SECURITY_KEY_S2_PUBLIC_DSK_LENGTH);
              break;
#endif

#if SUPPORT_ZW_SET_SECURITY_S2_CRITICAL_NODE_ID
            case E_SECURITY_SETUP_CMD_ZW_SET_SECURITY_S2_CRITICAL_NODE_ID:
              /* HOST->ZW: bNodeIDLen(1) | nodeID[bNodeIDLen] */
              /* ZW->HOST: retValLen(1) | retVal[retValLen] */
              retVal = FALSE;
              if (((FRAME_LENGTH_MIN + 1 + 1 + 1) == serFrameLen) &&
                  (1 == serFrameDataPtr[1]))
              {
                ZW_SetSecurityS2CriticalNodeID(serFrameDataPtr[2]);
                retVal = TRUE;
              }
              compl_workbuf[1] = 1;
              compl_workbuf[2] = retVal;
              DoRespond_workbuf(3);
              break;
#endif

#if SUPPORT_ZW_SET_SECURITY_S2_INCLUSION_PUBLIC_DSK_CSA
            case E_SECURITY_SETUP_CMD_ZW_SET_SECURITY_S2_INCLUSION_PUBLIC_DSK_CSA:
              /* HOST->ZW: bCSA_DSKLen(SECURITY_KEY_S2_PUBLIC_CSA_DSK_LENGTH) | aCSA_DSK[bCSA_DSKLen] */
              /* ZW->HOST: retValLen(1) | retVal[retValLen] */
              {
                retVal = FALSE;
                if (((FRAME_LENGTH_MIN + 1 + 1 + SECURITY_KEY_S2_PUBLIC_CSA_DSK_LENGTH) == serFrameLen) &&
                    (SECURITY_KEY_S2_PUBLIC_CSA_DSK_LENGTH == serFrameDataPtr[1]))
                {
                  ZW_SetSecurityS2InclusionPublicDSK_CSA((s_SecurityS2InclusionCSAPublicDSK_t*)&serFrameDataPtr[2]);
                  retVal = TRUE;
                }
                compl_workbuf[1] = 1;
                compl_workbuf[2] = retVal;
                DoRespond_workbuf(3);
              }
              break;
#endif  /* SUPPORT_ZW_SET_SECURITY_S2_INCLUSION_PUBLIC_DSK_CSA */

            case E_SECURITY_SETUP_CMD_GET_SECURITY_CAPABILITIES:
              {
                /* HOST->ZW: */
                /* ZW->HOST: securitySetup_bitmaskLen | securityKeys_bitmask[securitySetup_bitmaskLen] */
                compl_workbuf[1] = 1;
              /* LSB first if more than one byte in securityKeys_bitmask[] */
                compl_workbuf[2] = 0
#if SUPPORT_ZW_GET_SECURITY_KEYS
                                   | E_SECURITY_SETUP_SUPPORT_CMD_ZW_GET_SECURITY_KEYS
                                   | E_SECURITY_SETUP_SUPPORT_CMD_SET_SECURITY_INCLUSION_REQUESTED_KEYS
                                   | E_SECURITY_SETUP_SUPPORT_CMD_SET_SECURITY_INCLUSION_REQUESTED_AUTHENTICATION
#endif
#if SUPPORT_ZW_SET_SECURITY_S0_NETWORK_KEY
                                   | E_SECURITY_SETUP_SUPPORT_CMD_ZW_SET_SECURITY_S0_NETWORK_KEY
#endif
#if SUPPORT_ZW_GET_SECURITY_S2_PUBLIC_DSK
                                   | E_SECURITY_SETUP_SUPPORT_CMD_ZW_GET_SECURITY_S2_PUBLIC_DSK
#endif
#if SUPPORT_ZW_SET_SECURITY_S2_CRITICAL_NODE_ID
                                   | E_SECURITY_SETUP_SUPPORT_CMD_ZW_SET_SECURITY_S2_CRITICAL_NODE_ID
#endif
#if SUPPORT_ZW_SET_SECURITY_S2_INCLUSION_PUBLIC_DSK_CSA
                                   | E_SECURITY_SETUP_SUPPORT_CMD_ZW_SET_SECURITY_S2_INCLUSION_PUBLIC_DSK_CSA
#endif
                                   ;
                DoRespond_workbuf(3);
              }
              break;

            default:
              {
                /* ZW->HOST: E_SECURITY_SETUP_UNKNOWN_COMMAND | retValLen | securityFuncID(called)[retValLen] */
                compl_workbuf[0] = E_SECURITY_SETUP_CMD_UNKNOWN;
                compl_workbuf[1] = 1;
                /* Return the called Unknown FUNC_ID_ZW_SECURITY_SETUP Command */
                compl_workbuf[2] = serFrameDataPtr[0];
                DoRespond_workbuf(3);
              }
              break;
          }
          break;
#endif

#if SUPPORT_ZW_SEND_DATA
        case FUNC_ID_ZW_SEND_DATA:
          /* HOST->ZW: nodeID | dataLength | pData[] | txOptions | funcID */
          /* ZW->HOST: RetVal */
          /* If RetVal == FALSE -> no callback */
          /* If RetVal == TRUE then callback returns with */
          /* ZW->HOST: txStatus | wTransmitTicksMSB | wTransmitTicksLSB | bRepeaters | rssi_values.incoming[0] | */
          /*           rssi_values.incoming[1] | rssi_values.incoming[2] | rssi_values.incoming[3] | rssi_values.incoming[4] | */
          /*           bRouteSchemeState | repeater0 | repeater1 | repeater2 | repeater3 | routespeed | */
          /*           bRouteTries | bLastFailedLink.from | bLastFailedLink.to */
          {
            BYTE dataLength = serFrameDataPtr[1];

            if (dataLength > BUF_SIZE_RX)
            {
              dataLength = BUF_SIZE_RX;
            }
            for (i = 0; i < dataLength; i++)
            {
              workbuf[i] = serFrameDataPtr[2 + i];
            }
            funcID_ComplHandler_ZW_SendData = serFrameDataPtr[3 + dataLength];
#ifndef ZW_SECURITY_PROTOCOL
            retVal = ZW_SendData(serFrameDataPtr[0], workbuf, dataLength, serFrameDataPtr[2 + dataLength],
              (serFrameDataPtr[3+dataLength] != 0) ? ZCB_ComplHandler_ZW_SendData : NULL);
#else
            {
              TRANSMIT_OPTIONS_TYPE ltTxOptions2;
              memset((BYTE*)&ltTxOptions2, 0, sizeof(ltTxOptions2));
              ltTxOptions2.destNode = serFrameDataPtr[0];
              ltTxOptions2.txOptions = serFrameDataPtr[2 + dataLength];
              retVal = (BYTE)ZW_SendDataEx(workbuf, dataLength, &ltTxOptions2,
                                          (funcID_ComplHandler_ZW_SendData != 0) ? ZCB_ComplHandler_ZW_SendData : NULL);
            }
#endif /* !ZW_SECURITY_PROTOCOL */
            DoRespond();
          }
          break;
#endif

#if SUPPORT_ZW_SEND_DATA_EX
        case FUNC_ID_ZW_SEND_DATA_EX:

          /* HOST->ZW: nodeID | dataLength | pData[] | txOptions | txSecOptions | securityKey | txOptions2 | funcID */
          /* ZW->HOST: RetVal */
          /* If "RetVal != 1" -> no callback */
          /* If "RetVal == 1" and "funcID != 0" then callback returns with */
          /* ZW->HOST: txStatus | wTransmitTicksMSB | wTransmitTicksLSB | bRepeaters | rssi_values.incoming[0] | */
          /*           rssi_values.incoming[1] | rssi_values.incoming[2] | rssi_values.incoming[3] | rssi_values.incoming[4] | */
          /*           bACKChannelNo | bLastTxChannelNo | */
          /*           bRouteSchemeState | repeater0 | repeater1 | repeater2 | repeater3 | routespeed | */
          /*           bRouteTries | bLastFailedLink.from | bLastFailedLink.to */
          {
            TRANSMIT_OPTIONS_TYPE ltTxOptions;
            BYTE dataLength;

            dataLength = serFrameDataPtr[1];
            if (dataLength > BUF_SIZE_RX)
            {
              dataLength = BUF_SIZE_RX;
            }
            for (i = 0; i < dataLength; i++)
            {
              workbuf[i] = serFrameDataPtr[2 + i];
            }
            memset((BYTE*)&ltTxOptions, 0, sizeof(ltTxOptions));
            ltTxOptions.destNode = serFrameDataPtr[0];
            ltTxOptions.txOptions = serFrameDataPtr[2 + dataLength];
            ltTxOptions.txSecOptions = serFrameDataPtr[3 + dataLength];
            ltTxOptions.securityKey = serFrameDataPtr[4 + dataLength];
            ltTxOptions.txOptions2 = serFrameDataPtr[5 + dataLength];
            funcID_ComplHandler_ZW_SendDataEx = serFrameDataPtr[6 + dataLength];
            retVal = (BYTE)ZW_SendDataEx(workbuf, dataLength, &ltTxOptions,
                                         (funcID_ComplHandler_ZW_SendDataEx != 0) ? ZCB_ComplHandler_ZW_SendDataEx : NULL);
            DoRespond();
          }
          break;
#endif

#if SUPPORT_ZW_SEND_DATA_MULTI
        case FUNC_ID_ZW_SEND_DATA_MULTI:
          /* numberNodes | pNodeIDList[] | dataLength | pData[] | txOptions | funcId */
          {
            BYTE numberNodes, dataLength, txOptions;

            retVal = 0;
            numberNodes = serFrameDataPtr[0];
            /* clear the destination node mask */
            for (i = 0; i < MAX_NODEMASK_LENGTH; i++)
            {
              groupMask[i] = 0;
            }
            /* Set the destination node mask bits */
            for (i = 0; i < numberNodes && i < MAX_GROUP_NODES; i++)
            {
              ZW_NodeMaskSetBit(groupMask, serFrameDataPtr[1+i]);
            }
            dataLength = serFrameDataPtr[1 + numberNodes];
            if (dataLength > BUF_SIZE_RX)
            {
              dataLength = BUF_SIZE_RX;
            }
            for (i = 0; i < dataLength; i++)
            {
              workbuf[i] = serFrameDataPtr[2 + numberNodes + i];
            }
            txOptions = serFrameDataPtr[2 + numberNodes + dataLength];

            funcID_ComplHandler_ZW_SendDataMulti = serFrameDataPtr[3 + numberNodes + dataLength];
            retVal = ZW_SendDataMulti(groupMask, workbuf, dataLength, txOptions,
                                      (serFrameDataPtr[3 + numberNodes + dataLength] != 0) ? ZCB_ComplHandler_ZW_SendDataMulti : NULL);
            DoRespond();
          }
          break;
#endif

#if SUPPORT_ZW_SEND_DATA_MULTI_EX
        case FUNC_ID_ZW_SEND_DATA_MULTI_EX:
          /* dataLength | pData[] | txOptions | securityKey | groupId | funcId */
          {
            TRANSMIT_MULTI_OPTIONS_TYPE ltTxOptionsMultiEx;
            BYTE dataLength;

            dataLength = serFrameDataPtr[0];
            if (dataLength > BUF_SIZE_RX)
            {
              dataLength = BUF_SIZE_RX;
            }
            for (i = 0; i < dataLength; i++)
            {
              workbuf[i] = serFrameDataPtr[1 + i];
            }
            /* txOptions */
            ltTxOptionsMultiEx.txOptions = serFrameDataPtr[1 + serFrameDataPtr[0]];
            /* securityKey */
            ltTxOptionsMultiEx.securityKey = serFrameDataPtr[2 + serFrameDataPtr[0]];
            /* groupID */
            ltTxOptionsMultiEx.groupID = serFrameDataPtr[3 + serFrameDataPtr[0]];
            /* funcID */
            funcID_ComplHandler_ZW_SendDataMultiEx = serFrameDataPtr[4 + serFrameDataPtr[0]];
            retVal = ZW_SendDataMultiEx(workbuf, dataLength, &ltTxOptionsMultiEx,
                                      (serFrameDataPtr[4 + serFrameDataPtr[0]] != 0) ? ZCB_ComplHandler_ZW_SendDataMultiEx : NULL);
            DoRespond();
          }
          break;
#endif

#if SUPPORT_ZW_SEND_DATA_ABORT
        case FUNC_ID_ZW_SEND_DATA_ABORT:
          /* */
          ZW_SendDataAbort(); /* If we are in middle of transmitting an application frame then STOP that. */
          set_state(stateIdle);
          break;
#endif

#if SUPPORT_ZW_SEND_DATA_META
        case FUNC_ID_ZW_SEND_DATA_META:
          /* nodeID | dataLength | pData[] | txOptions | funcID */
          {
            BYTE dataLength = serFrameDataPtr[1];
            if (dataLength > BUF_SIZE_RX)
            {
              dataLength = BUF_SIZE_RX;
            }
            for (i = 0; i < dataLength; i++)
            {
              workbuf[i] = serFrameDataPtr[2 + i];
            }
            funcID_ComplHandler_ZW_SendDataMeta = serFrameDataPtr[3 + dataLength];
            retVal = ZW_SEND_DATA_META(serFrameDataPtr[0], workbuf, dataLength, serFrameDataPtr[2 + dataLength],
                                       (serFrameDataPtr[3 + dataLength] != 0) ? ZCB_ComplHandler_ZW_SendDataMeta : NULL);
            DoRespond();
          }
          break;
#endif

#if SUPPORT_ZW_SEND_DATA_BRIDGE
        case FUNC_ID_ZW_SEND_DATA_BRIDGE:
          /* HOST->ZW: srcNodeID | destNodeID | dataLength | pData[] | txOptions | pRoute[4] | funcID */
          /* Devkit 6.0x pRoute[4] not used... Use [0,0,0,0] */
          {
            BYTE dataLength = serFrameDataPtr[2];
            if (dataLength > BUF_SIZE_RX)
            {
              dataLength = BUF_SIZE_RX;
            }
            for (i = 0; i < dataLength; i++)
            {
              workbuf[i] = serFrameDataPtr[3 + i];
            }
            funcID_ComplHandler_ZW_SendData = serFrameDataPtr[3 + 1 + 4 + dataLength];
            retVal = ZW_SEND_DATA_BRIDGE(serFrameDataPtr[0], serFrameDataPtr[1], workbuf, dataLength,
                                         serFrameDataPtr[3 + dataLength],
                                         (serFrameDataPtr[3 + 1 + 4 + dataLength] != 0) ? ZCB_ComplHandler_ZW_SendData_Bridge : NULL);
            DoRespond();
          }
          break;
#endif

#if SUPPORT_ZW_SEND_DATA_META_BRIDGE
        case FUNC_ID_ZW_SEND_DATA_META_BRIDGE:
          /* HOST->ZW: srcNodeID | destNodeID | dataLength | pData[] | txOptions | pRoute[4] | funcID */
          /* Devkit 6.0x pRoute[4] not used... Use [0,0,0,0] */
          {
            BYTE dataLength = serFrameDataPtr[2];
            if (dataLength > BUF_SIZE_RX)
            {
              dataLength = BUF_SIZE_RX;
            }
            for (i = 0; i < dataLength; i++)
            {
              workbuf[i] = serFrameDataPtr[3 + i];
            }
            funcID_ComplHandler_ZW_SendDataMeta = serFrameDataPtr[3 + 1 + 4 + dataLength];
            retVal = ZW_SEND_DATA_META_BRIDGE(serFrameDataPtr[0], serFrameDataPtr[1], workbuf, dataLength,
                                              serFrameDataPtr[3 + dataLength],
                                              (serFrameDataPtr[3 + 1 + 4 + dataLength] != 0) ? ZCB_ComplHandler_ZW_SendDataMeta_Bridge : NULL);
            DoRespond();
          }
          break;
#endif

#if SUPPORT_ZW_SEND_DATA_MULTI_BRIDGE
        case FUNC_ID_ZW_SEND_DATA_MULTI_BRIDGE:
          /* HOST->ZW: srcNodeID | numberNodes | pNodeIDList[] | dataLength | pData[] | txOptions | funcId */
          {
            BYTE numberNodes, dataLength, txOptions;

            numberNodes = serFrameDataPtr[1];
            /* clear the destination node mask */
            for (i = 0; i < MAX_NODEMASK_LENGTH; i++)
            {
              groupMask[i] = 0;
            }
            /* Set the destination node mask bits */
            for (i = 0; i < numberNodes && i < MAX_GROUP_NODES; i++)
            {
              ZW_NodeMaskSetBit(groupMask, serFrameDataPtr[2 + i]);
            }
            dataLength = serFrameDataPtr[2 + numberNodes];
            if (dataLength > BUF_SIZE_RX)
            {
              dataLength = BUF_SIZE_RX;
            }
            for (i = 0; i < dataLength; i++)
            {
              workbuf[i] = serFrameDataPtr[2 + 1 + numberNodes + i];
            }
            txOptions = serFrameDataPtr[2 + 1 + numberNodes + dataLength];

            funcID_ComplHandler_ZW_SendDataMulti = serFrameDataPtr[2 + 1 + 1 + numberNodes + dataLength];
            retVal = ZW_SEND_DATA_MULTI_BRIDGE(serFrameDataPtr[0], groupMask, workbuf, dataLength, txOptions,
                                               (serFrameDataPtr[2 + 1 + 1 + numberNodes + dataLength] != 0) ?
                                                 ZCB_ComplHandler_ZW_SendDataMulti_Bridge : NULL);
            DoRespond();
          }
          break;
#endif

#if SUPPORT_MEMORY_GET_ID
        case FUNC_ID_MEMORY_GET_ID:
          /*  */
          ZW_MEMORY_GET_ID(compl_workbuf, compl_workbuf + 4);
          DoRespond_workbuf(5);
          break;
#endif

#if SUPPORT_MEMORY_GET_BYTE
        case FUNC_ID_MEMORY_GET_BYTE:
          /* offset (MSB) | offset (LSB) */
          retVal = MemoryGetByte(((WORD)&EEOFFSET_HOST_OFFSET_START_far)+
                                 (((WORD)serFrameDataPtr[0] << 8) + serFrameDataPtr[1]));
          DoRespond();
          break;
#endif

#if SUPPORT_MEMORY_PUT_BYTE
        case FUNC_ID_MEMORY_PUT_BYTE:
          /* HOST->ZW:
             offset(MSB)        offset into host application NVM memory array
             offset(LSB)
             data
           */
          /* ZW->HOST:
             retVal             [retVal=0 ==> error|
                                 retVal=1 ==> OK (NVM no change)|
                                 retVal>=2 ==> OK (NVM data bytes written + 1)]
           */
          {
            WORD NVM_WorkPtr;
            NVM_WorkPtr = ((WORD)&EEOFFSET_HOST_OFFSET_START_far) +
                           (((WORD)serFrameDataPtr[0] << 8) + serFrameDataPtr[1]);
            /* Don't go beyond reserved NVM area for host application [sizeof(EEOFFSET_HOST_OFFSET_START_far)] */
            if (NVM_WorkPtr >= ((WORD)&EEOFFSET_HOST_OFFSET_START_far) + sizeof(EEOFFSET_HOST_OFFSET_START_far))
            {
              retVal = 0;
            }
            else
            {
              /* For external SPI NVM: */
              /* MemoryPutByte() does not return as it says "RET Number of bytes written +1" */
              /* instead it returns "RET FALSE if value is identical to value in EEPROM else TRUE" */
              /* i.e. it will be assumed, that the operation always succeeds */
              /* For internal FLASH/MTP NVM: */
              /* MemoryPutByte() does return as it says "RET Number of bytes written +1" */
              /* i.e. it will be assumed, that the operation always succeeds */
              retVal = (MemoryPutByte(NVM_WorkPtr, serFrameDataPtr[2]) ? 2 : 1);
            }
            DoRespond();
          }
          break;
#endif

#if SUPPORT_MEMORY_GET_BUFFER
        case FUNC_ID_MEMORY_GET_BUFFER:
          /* offset(MSB) | offset (LSB) | length */
          {
            BYTE dataLength = serFrameDataPtr[2];
            /* Make sure the length isn't larger than the available buffer size */
            if (dataLength > BUF_SIZE_TX)
            {
              dataLength = BUF_SIZE_TX;
            }
            MemoryGetBuffer(((WORD)&EEOFFSET_HOST_OFFSET_START_far)+
                            (((WORD)serFrameDataPtr[0] << 8) + serFrameDataPtr[1]), compl_workbuf, dataLength);
            DoRespond_workbuf(dataLength);
          }
          break;
#endif

#if SUPPORT_MEMORY_PUT_BUFFER
        case FUNC_ID_MEMORY_PUT_BUFFER:
          /* HOST->ZW:
             offset(MSB)        offset into host application NVM memory array
             offset(LSB)
             length(MSB)        desired length of write operation
             length(LSB)
             buffer[]           buffer
             funcID
           */
          /* ZW->HOST:
             retVal             [retVal=0 ==> error|
                                 retVal=1 ==> OK (NVM no change)|
                                 retVal>=2 ==> OK (NVM data bytes written + 1)]
           */
          {
            WORD NVM_WorkPtr;
            WORD length;
            NVM_WorkPtr = ((WORD)&EEOFFSET_HOST_OFFSET_START_far) +
                          (((WORD)serFrameDataPtr[0] << 8) + serFrameDataPtr[1]);
            length = ((WORD)(serFrameDataPtr[2] << 8)) + serFrameDataPtr[3];
            retVal = 0;
            {
              /* Don't go beyond serial buffer size */
              /* and don't go beyond reserved NVM area for host application [sizeof(EEOFFSET_HOST_OFFSET_START_far)] */
              if ((length > BUF_SIZE_RX) ||
                  ((NVM_WorkPtr + length) > ((WORD)&EEOFFSET_HOST_OFFSET_START_far) + sizeof(EEOFFSET_HOST_OFFSET_START_far))
                 )
              {
                retVal = FALSE; /* ignore request if length is larger than available buffer */
              }
              else
              {
                for (i = 0; i <length; i++)
                {
                  workbuf[i] = serFrameDataPtr[4+i];
                }
                funcID_ComplHandler_MemoryPutBuffer = serFrameDataPtr[4+length];
                retVal = ZW_MEM_PUT_BUFFER(NVM_WorkPtr,
                                           workbuf, length,
                                           (serFrameDataPtr[4 + length] != 0) ? ZCB_ComplHandler_MemoryPutBuffer : NULL);
              }
            }
            DoRespond();
          }
          break;
#endif

#if SUPPORT_NVM_BACKUP_RESTORE
        case FUNC_ID_NVM_BACKUP_RESTORE:
          /* HOST->ZW:
             operation          [open=0|read=1|write=2|close=3]
             length             desired length of read/write operation
             offset(MSB)        pointer to NVM memory
             offset(LSB)
             buffer[]           buffer only sent for operation=write
           */
          /* ZW->HOST:
             retVal             [OK=0|error=1|EOF=-1]
             length             actual length of read/written data
             offset(MSB)        pointer to NVM memory (EOF ptr for operation=open)
             offset(LSB)
             buffer[]           buffer only returned for operation=read
           */
          {
            eNVMBackupRestoreOperation NVMBackupRestoreOperationInProgress;
            WORD NVM_WorkPtr;
            BYTE dataLength;

            dataLength = 0; /* Assume nothing is read or written */
            compl_workbuf[0] = NVMBackupRestoreReturnValueOK;   /* Assume not at EOF and no ERROR */
            compl_workbuf[1] = 0;                               /* Assume no data */
            compl_workbuf[2] = 0;
            compl_workbuf[3] = 0;
            switch (serFrameDataPtr[0]) /* operation */
            {
              case NVMBackupRestoreOperationOpen: /* open */
                /* Lock everyone else out from making changes to the NVM content */
                /* Remember to have some kind of dead-mans-pedal to release lock again. */
                /* TODO */
                NVMBackupRestoreOperationInProgress = NVMBackupRestoreOperationOpen;
                NVM_WorkPtr = 0;
                /* Tell him the expected size of backup/restore */
                compl_workbuf[2] = (NVM_STORAGE_END >> 8) & 0xFF;
                compl_workbuf[3] = NVM_STORAGE_END & 0xFF;
                MemoryClearStatus();
                break;

              case NVMBackupRestoreOperationRead: /* read */
                /* Validate input */
                if ((NVMBackupRestoreOperationInProgress != NVMBackupRestoreOperationRead) &&
                    (NVMBackupRestoreOperationInProgress != NVMBackupRestoreOperationOpen))
                {
                  compl_workbuf[0] = NVMBackupRestoreReturnValueOperationMismatch;
                  break;
                }
                NVMBackupRestoreOperationInProgress = NVMBackupRestoreOperationRead;
                dataLength = serFrameDataPtr[1];                /* Requested dataLength */
                NVM_WorkPtr = (((WORD)serFrameDataPtr[2] << 8) + serFrameDataPtr[3]);
                /* Make sure that length isn't larger than the available buffer size */
                if (dataLength > BUF_SIZE_TX - 4)
                {
                  dataLength = BUF_SIZE_TX - 4;
                }
                /* Make sure that we don't go beyond valid NVM content */
                if ((NVM_WorkPtr + dataLength) > NVM_STORAGE_END)
                {
                  dataLength = NVM_STORAGE_END - NVM_WorkPtr + 1;
                  compl_workbuf[0] = NVMBackupRestoreReturnValueEOF;        /* Indicate at EOF */
                }
                compl_workbuf[1] = dataLength;
                compl_workbuf[2] = serFrameDataPtr[2];
                compl_workbuf[3] = serFrameDataPtr[3];
                MemoryGetBuffer(NVM_WorkPtr, &compl_workbuf[4], dataLength);
                if (MemoryGetStatus())
                {
                  compl_workbuf[0] = NVMBackupRestoreReturnValueOperationDisturbed;
                }
                break;

              case NVMBackupRestoreOperationWrite: /* write */
                /* Validate input */
                if ((NVMBackupRestoreOperationInProgress != NVMBackupRestoreOperationWrite) &&
                    (NVMBackupRestoreOperationInProgress != NVMBackupRestoreOperationOpen))
                {
                  compl_workbuf[0] = NVMBackupRestoreReturnValueOperationMismatch;
                  break;
                }
                NVMBackupRestoreOperationInProgress = NVMBackupRestoreOperationWrite;
                dataLength = serFrameDataPtr[1];                /* Requested dataLength */
                NVM_WorkPtr = (((WORD)serFrameDataPtr[2] << 8) + serFrameDataPtr[3]);

                if (dataLength > BUF_SIZE_RX)
                {
                  compl_workbuf[0] = TRUE; /* ERROR: ignore request if length is larger than available buffer */
                }
                else
                {
                  /* Make sure that we don't go beyond valid NVM content */
                  if ((NVM_WorkPtr + dataLength) > NVM_STORAGE_END)
                  {
                    dataLength = NVM_STORAGE_END - NVM_WorkPtr + 1;
                    compl_workbuf[0] = EOF;                       /* Indicate at EOF */
                  }
                  for (i = 0; i < dataLength; i++)
                  {
                    workbuf[i] = serFrameDataPtr[4+i];
                  }
                  retVal = ZW_MEM_PUT_BUFFER(NVM_WorkPtr, workbuf, dataLength, NULL);
                  compl_workbuf[1] = dataLength;                /* Data written */
                  compl_workbuf[2] = serFrameDataPtr[2];
                  compl_workbuf[3] = serFrameDataPtr[3];
                  dataLength = 0;                               /* No data returned */
                }
                break;

              case NVMBackupRestoreOperationClose: /* close */
                /* Unlock NVM content, so everyone else can make changes again */
                /* TODO */
                if ((NVMBackupRestoreOperationInProgress == NVMBackupRestoreOperationRead) &&
                    MemoryGetStatus())
                {
                  compl_workbuf[0] = NVMBackupRestoreReturnValueOperationDisturbed;
                }
                NVMBackupRestoreOperationInProgress = NVMBackupRestoreOperationClose;
                NVM_WorkPtr = NVM_STORAGE_END;
                break;

              default:
                break;
            }
            DoRespond_workbuf(dataLength + 4);
          }
          break;
#endif

#if SUPPORT_NVM_GET_ID
        case FUNC_ID_NVM_GET_ID:
          /* HOST->ZW: */
          /* ZW->HOST: length | manufacturerID | memoryType | memoryCapacity */
          {
            NVM_TYPE_T nvmTypeT;

            NVM_get_id(&nvmTypeT);
            i = 0;
            compl_workbuf[++i] = nvmTypeT.manufacturerID;
            compl_workbuf[++i] = nvmTypeT.memoryType;
            compl_workbuf[++i] = nvmTypeT.memoryCapacity;
            compl_workbuf[0] = i;
            DoRespond_workbuf(i + 1);
          }
          break;
#endif

#if SUPPORT_NVM_EXT_READ_LONG_BYTE
        case FUNC_ID_NVM_EXT_READ_LONG_BYTE:
          /* HOST->ZW: offset3byte(MSB) | offset3byte | offset3byte(LSB) */
          /* ZW->HOST: dataread */
          {
            retVal = FALSE;
            if ((FRAME_LENGTH_MIN + 2) < serFrameLen)
            {
              retVal = NVM_ext_read_long_byte((DWORD)(((DWORD)serFrameDataPtr[0] << 16) + ((WORD)serFrameDataPtr[1] << 8) + serFrameDataPtr[2]));
            }
            DoRespond();
          }
          break;
#endif

#if SUPPORT_NVM_EXT_WRITE_LONG_BYTE
        case FUNC_ID_NVM_EXT_WRITE_LONG_BYTE:
          /* HOST->ZW: offset3byte(MSB) | offset3byte | offset3byte(LSB) | data */
          /* ZW->HOST: writestatus */
          {
            retVal = FALSE;
            if ((FRAME_LENGTH_MIN + 3) < serFrameLen)
            {
              retVal = NVM_ext_write_long_byte((DWORD)(((DWORD)serFrameDataPtr[0] << 16) + ((WORD)serFrameDataPtr[1] << 8) + serFrameDataPtr[2]),
                                               serFrameDataPtr[3]);
            }
            DoRespond();
          }
          break;
#endif

#if SUPPORT_NVM_EXT_READ_LONG_BUFFER
        case FUNC_ID_NVM_EXT_READ_LONG_BUFFER:
          /* HOST->ZW: offset3byte(MSB) | offset3byte | offset3byte(LSB) | length2byte(MSB) | length2byte(LSB) */
          /* ZW->HOST: data[] */
          {
            WORD dataLength = 0;
            /* Ignore if frame is to short */
            if ((FRAME_LENGTH_MIN + 3 + 1) < serFrameLen)
            {
              dataLength = ((WORD)(serFrameDataPtr[3] << 8)) + serFrameDataPtr[4];
              /* Make sure the length isn't larger than the available buffer size */
              if (dataLength > BUF_SIZE_TX)
              {
                dataLength = BUF_SIZE_TX;
              }
              NVM_ext_read_long_buffer((DWORD)(((DWORD)serFrameDataPtr[0] << 16) + ((WORD)serFrameDataPtr[1] << 8) + serFrameDataPtr[2]), compl_workbuf, dataLength);
            }
            DoRespond_workbuf(dataLength);
          }
          break;
#endif

#if SUPPORT_NVM_EXT_WRITE_LONG_BUFFER
        case FUNC_ID_NVM_EXT_WRITE_LONG_BUFFER:
          /* HOST->ZW: offset3byte(MSB) | offset3byte | offset2byte(LSB) | length2byte(MSB) | length2byte(LSB) | buffer[] */
          /* ZW->HOST: retVal */
          {
            WORD length;
            retVal = FALSE;
            /* Ignore if frame has no data to write */
            if ((FRAME_LENGTH_MIN + 6) < serFrameLen)
            {
              length = ((WORD)(serFrameDataPtr[3] << 8)) + serFrameDataPtr[4];
              /* Ignore write if length exceeds specified data-array */
              if (length <= serFrameLen - FRAME_LENGTH_MIN)
              {
                /* ignore request if length is larger than available buffer */
                if (length < BUF_SIZE_RX)
                {
                  for (i = 0; i < length; i++)
                  {
                    workbuf[i] = serFrameDataPtr[5+i];
                  }
                  retVal = NVM_ext_write_long_buffer((DWORD)(((DWORD)serFrameDataPtr[0] << 16) + ((WORD)serFrameDataPtr[1] << 8) + serFrameDataPtr[2]),
                                             workbuf, length);
                }
              }
            }
            DoRespond();
          }
          break;
#endif

#if SUPPORT_ZW_FIRMWARE_UPDATE_NVM
        case FUNC_ID_ZW_FIRMWARE_UPDATE_NVM:
          /* HOST->ZW: firmwareUpdateFunction [| data[...]] */
          /* ZW->HOST: firmwareUpdateFunction [| retVal[...]] */
          {
            WORD fw_crc;
            WORD length;
            FIRMWARE_UPDATE_NVM_T bFirmwareUpdateFunction = (FIRMWARE_UPDATE_NVM_T)serFrameDataPtr[0];
            BYTE_IN_AR(compl_workbuf, 0) = bFirmwareUpdateFunction;
            switch (bFirmwareUpdateFunction)
            {
              case FIRMWARE_UPDATE_NVM_INIT:
                /* HOST->ZW: FIRMWARE_UPDATE_NVM_INIT */
                /* ZW->HOST: FIRMWARE_UPDATE_NVM_INIT | retVal */
                {
                  // BYTE /*RET NVM_FIRMWARE_UPDATE_SUPPORTED if NVM is Firmware Update compatible */
                  //      /*    NVM_FIRMWARE_UPDATE_NOT_SUPPORTED if NVM is NOT Firmware Update compatible */
                  // ZW_FirmwareUpdate_NVM_Init();

                  BYTE_IN_AR(compl_workbuf, 1) = ZW_FirmwareUpdate_NVM_Init();
                  DoRespond_workbuf(2);
                }
                break;

              case FIRMWARE_UPDATE_NVM_SET_NEW_IMAGE:
                /* HOST->ZW: FIRMWARE_UPDATE_NVM_SET_NEW_IMAGE | value */
                /* ZW->HOST: FIRMWARE_UPDATE_NVM_SET_NEW_IMAGE | retVal */
                {
                  // BYTE  /* RET  TRUE if specified bValue has been written to NVM */
                  //       /*      FALSE if either NVM is not firmware update capable or */
                  //       /*      the Firmware_NEWIMAGE value is allready set to bValue */
                  // ZW_FirmwareUpdate_NVM_Set_NEWIMAGE(
                  //   BYTE bValue);

                  BYTE_IN_AR(compl_workbuf, 1) = ZW_FirmwareUpdate_NVM_Set_NEWIMAGE(serFrameDataPtr[1]);
                  DoRespond_workbuf(2);
                }
                break;

              case FIRMWARE_UPDATE_NVM_GET_NEW_IMAGE:
                /* HOST->ZW: FIRMWARE_UPDATE_NVM_GET_NEW_IMAGE */
                /* ZW->HOST: FIRMWARE_UPDATE_NVM_GET_NEW_IMAGE | retVal */
                {
                  // BYTE  /* RET 0 if either NVM not capable or Indicator indicates NO NEW image */
                  //       /*     1 if Indicator indicates NEW image present */
                  // ZW_FirmwareUpdate_NVM_Get_NEWIMAGE();

                  BYTE_IN_AR(compl_workbuf, 1) = ZW_FirmwareUpdate_NVM_Get_NEWIMAGE();
                  DoRespond_workbuf(2);
                }
                break;

              case FIRMWARE_UPDATE_NVM_UPDATE_CRC16:
                /* HOST->ZW: FIRMWARE_UPDATE_NVM_UPDATE_CRC16 |
                             offset3byte(MSB) |
                             offset3byte |
                             offset2byte(LSB) |
                             length2byte(MSB) |
                             length2byte(LSB) |
                             seedCRC16_high |
                             seedCRC16_low */
                /* ZW->HOST: FIRMWARE_UPDATE_NVM_UPDATE_CRC16 | resCRC16_high | resCRC16_low */
                {
                  // WORD  /* RET  Resulting CRC16 value after doing CRC16 calculation */
                  //       /*      on specified block of data in extrnal NVM */
                  // ZW_firmwareUpdate_NVM_UpdateCRC16(
                  //   WORD crc,
                  //   DWORD nvmOffset,
                  //   WORD blockSize);

                  fw_crc = ZW_firmwareUpdate_NVM_UpdateCRC16((WORD)(((WORD)serFrameDataPtr[6] << 8) + serFrameDataPtr[7]),
                                                             (DWORD)(((DWORD)serFrameDataPtr[1] << 16) +
                                                                     ((WORD)serFrameDataPtr[2] << 8) +
                                                                     serFrameDataPtr[3]),
                                                             (WORD)(((WORD)(serFrameDataPtr[4] << 8)) + serFrameDataPtr[5]));
                  BYTE_IN_AR(compl_workbuf, 1) = (BYTE)(((fw_crc & 0xFF00) >> 8) & 0xFF);
                  BYTE_IN_AR(compl_workbuf, 2) = (BYTE)(fw_crc & 0xFF);
                  DoRespond_workbuf(3);
                }
                break;

              case FIRMWARE_UPDATE_NVM_IS_VALID_CRC16:
                /* HOST->ZW: FIRMWARE_UPDATE_NVM_IS_VALID_CRC16 */
                /* ZW->HOST: FIRMWARE_UPDATE_NVM_IS_VALID_CRC16 | retVal | deprecated | deprecated */
                {
                  // BOOL  /* RET  TRUE if NVM contains is a valid Z-Wave Bootloader upgradeable Firmware */
                  //       /*      FALSE if NVM do NOT contain a valid Z-Wave Bootloader upgradeable Firmware */
                  // ZW_FirmwareUpdate_NVM_isValidCRC16(void);

                  BYTE_IN_AR(compl_workbuf, 1) = (BYTE)ZW_FirmwareUpdate_NVM_isValidCRC16();
                  BYTE_IN_AR(compl_workbuf, 2) = 0;
                  BYTE_IN_AR(compl_workbuf, 3) = 0;
                  DoRespond_workbuf(4);
                }
                break;

              case FIRMWARE_UPDATE_NVM_WRITE:
                /* HOST->ZW: FIRMWARE_UPDATE_NVM_WRITE |
                             offset3byte(MSB) |
                             offset3byte |
                             offset2byte(LSB) |
                             length2byte(MSB) |
                             length2byte(LSB) |
                             buffer[] */
                /* ZW->HOST: FIRMWARE_UPDATE_NVM_WRITE | retVal */
                {
                  // BYTE  /* RET  TRUE if specified buffer has been written to NVM */
                  //       /*      FALSE if either NVM is not firmware update capable or */
                  //       /*      the sourceBuffer contents allready are present at */
                  //       /*      specified offset in external NVM */
                  // ZW_FirmwareUpdate_NVM_Write(
                  //   BYTE *sourceBuffer,
                  //   WORD fw_bufSize,
                  //   DWORD firmwareOffset);

                  /* Assume failed */
                  BYTE_IN_AR(compl_workbuf, 1) = FALSE;
                  /* Ignore if frame has no data to write */
                  if ((FRAME_LENGTH_MIN + 7) < serFrameLen)
                  {
                    length = ((WORD)(serFrameDataPtr[4] << 8)) + serFrameDataPtr[5];
                    /* Ignore write if length exceeds receive data-array length */
                    if (length <= serFrameLen - FRAME_LENGTH_MIN - 1)
                    {
                      /* ignore request if length is larger than available buffer */
                      if (length < BUF_SIZE_RX)
                      {
                        for (i = 0; i < length; i++)
                        {
                          workbuf[i] = serFrameDataPtr[6+i];
                        }
                        BYTE_IN_AR(compl_workbuf, 1) = ZW_FirmwareUpdate_NVM_Write(workbuf, length,
                                                                                   (DWORD)(((DWORD)serFrameDataPtr[1] << 16) +
                                                                                           ((WORD)serFrameDataPtr[2] << 8) +
                                                                                           serFrameDataPtr[3]));
                      }
                    }
                  }
                  DoRespond_workbuf(2);
                }
                break;

              default:
                /* ZW->HOST: FIRMWARE_UPDATE_NVM_UNKNOWN */
                DoRespond_workbuf(1);
                break;
            }
          }
          break;
#endif

#if SUPPORT_ZW_NVR_GET_APP_VALUE
        case FUNC_ID_ZW_NVR_GET_APP_VALUE:
          /* HOST->ZW: appNVROffset | length */
          /* ZW->HOST: appNVRdata[] */
          {
            BYTE dataLength = 0;
            /* Ignore if frame is to short */
            if ((FRAME_LENGTH_MIN + 1) < serFrameLen)
            {
              dataLength = serFrameDataPtr[1];
              /* Make sure the length isn't larger than the available buffer size */
              if (dataLength > BUF_SIZE_TX)
              {
                dataLength = BUF_SIZE_TX;
              }
              ZW_NVRGetAppValue(serFrameDataPtr[0], dataLength, compl_workbuf);
            }
            DoRespond_workbuf(dataLength);
          }
          break;
#endif

#if SUPPORT_ZW_NVR_GET_VALUE
        case FUNC_ID_NVR_GET_VALUE:
          /* HOST->ZW: offset | length */
          /* ZW->HOST: NVRdata[] */
          {
            BYTE dataLength = 0;
            /* Ignore if frame is to short */
            if ((FRAME_LENGTH_MIN + 1) < serFrameLen)
            {
              dataLength = serFrameDataPtr[1];
              /* Make sure the length isn't larger than the available buffer size */
              if (dataLength > BUF_SIZE_TX)
              {
                dataLength = BUF_SIZE_TX;
              }
              ZW_NVRGetValue(serFrameDataPtr[0], dataLength, compl_workbuf);
            }
            DoRespond_workbuf(dataLength);
          }
          break;
#endif

#if SUPPORT_SERIAL_API_GET_APPL_HOST_MEMORY_OFFSET
        case FUNC_ID_SERIAL_API_GET_APPL_HOST_MEMORY_OFFSET:
          retVal =
          break;
#endif

#if SUPPORT_ZW_REQUEST_NETWORK_UPDATE
        case FUNC_ID_ZW_REQUEST_NETWORK_UPDATE:
          /* funcID */
          funcID_ComplHandler_netWork_Management = serFrameDataPtr[0];
          management_Func_ID = serFrameCmd;
          retVal = ZW_REQUEST_NETWORK_UPDATE((serFrameDataPtr[0] != 0) ? ZCB_ComplHandler_ZW_netWork_Management : NULL);
          DoRespond();
          break;
#endif /* SUPPORT_ZW_REQUEST_NETWORK_UPDATE */

#if SUPPORT_ZW_ENABLE_SUC
        case FUNC_ID_ZW_ENABLE_SUC:
          /* HOST->ZW: state | capabilities */
          retVal = ZW_ENABLE_SUC(serFrameDataPtr[0], serFrameDataPtr[1]);
          DoRespond();
          break;
#endif

#if SUPPORT_ZW_REQUEST_NODE_NEIGHBOR_UPDATE
        case FUNC_ID_ZW_REQUEST_NODE_NEIGHBOR_UPDATE:
          /* nodeID | funcID */
          funcID_ComplHandler_ZW_RequestNodeNeighborUpdate = serFrameDataPtr[1];
          if (!ZW_REQUEST_NODE_NEIGHBOR_UPDATE(serFrameDataPtr[0], serFrameDataPtr[1] ? ZCB_ComplHandler_ZW_RequestNodeNeighborUpdate : NULL))
          { /* It did not kick off, no nodes to ask? */
            if (serFrameDataPtr[1])
            {
              ZCB_ComplHandler_ZW_RequestNodeNeighborUpdate(REQUEST_NEIGHBOR_UPDATE_FAILED);
            }
          }
          set_state(stateIdle);
          break;
#endif

#ifdef ZW_CONTROLLER
#if SUPPORT_ZW_GET_NEIGHBOR_COUNT
        case FUNC_ID_ZW_GET_NEIGHBOR_COUNT:
          retVal = ZW_GET_NEIGHBOR_COUNT(serFrameDataPtr[0]);
          DoRespond();
          break;
#endif
#endif /* ZW_CONTROLLER */

#ifdef ZW_CONTROLLER
#if SUPPORT_ZW_ARE_NODES_NEIGHBOURS
        case FUNC_ID_ZW_ARE_NODES_NEIGHBOURS:
          retVal = ZW_ARE_NODES_NEIGHBOURS(serFrameDataPtr[0], serFrameDataPtr[1]);
          DoRespond();
          break;
#endif
#endif /* ZW_CONTROLLER */

#if SUPPORT_ZW_SET_LEARN_NODE_STATE
        case FUNC_ID_ZW_SET_LEARN_NODE_STATE:
          /* mode | funcID */
          funcID_ComplHandler_ZW_SetLearnNodeState = serFrameDataPtr[1];
          ZW_SetLearnNodeState(serFrameDataPtr[0], (serFrameDataPtr[1] != 0) ? ZCB_ComplHandler_ZW_SetLearnNodeState : NULL);
          set_state(stateIdle);
          break;
#endif

#if SUPPORT_ZW_GET_NODE_PROTOCOL_INFO
        case FUNC_ID_ZW_GET_NODE_PROTOCOL_INFO:
          /* bNodeID */
          ZW_GET_NODE_STATE(serFrameDataPtr[0], (NODEINFO *) compl_workbuf);
          DoRespond_workbuf(6);
          break;
#endif

#if SUPPORT_ZW_SET_DEFAULT
        case FUNC_ID_ZW_SET_DEFAULT:
          /* funcID */
          funcID_ComplHandler_ZW_SetDefault = serFrameDataPtr[0];

#ifdef ZW_CONTROLLER
          ZW_SET_DEFAULT((serFrameDataPtr[0] != 0) ? ZCB_ComplHandler_ZW_SetDefault : NULL);
#endif  /* ZW_CONTROLLER */
#ifdef ZW_SLAVE
          ZW_SET_DEFAULT();
          if (serFrameDataPtr[0] != 0)
          {
            ZCB_ComplHandler_ZW_SetDefault(); //To make sure that controllers and slaves behave identically
          }
#endif  /* ZW_SLAVE */
          set_state(stateIdle);
          break;
#endif

#if SUPPORT_ZW_NEW_CONTROLLER
        case FUNC_ID_ZW_NEW_CONTROLLER:
          /* state | funcID */
          funcID_ComplHandler_ZW_NewController = serFrameDataPtr[1];
          ZW_NEW_CONTROLLER( serFrameDataPtr[0], (serFrameDataPtr[1] != 0) ? ComplHandler_ZW_NewController : NULL);
          set_state(stateIdle);
          break;
#endif

#ifdef ZW_CONTROLLER
#if SUPPORT_ADD_REMOVE_PROTECT
        case FUNC_ID_PROPRIETARY_1:
          addState = 0;
          set_state(stateIdle);
          break;
#endif
#endif

#if SUPPORT_ZW_ADD_NODE_TO_NETWORK
        case FUNC_ID_ZW_ADD_NODE_TO_NETWORK:
          /* HOST->ZW: mode | funcID */
          /* HOST->ZW: mode = 0x07 | funcID | DSK[0] | DSK[1] | DSK[2] | DSK[3] | DSK[4] | DSK[5] | DSK[6] | DSK[7] */
#if SUPPORT_ADD_REMOVE_PROTECT
          if (ZW_NodeManagementRunning())
          {
            break;
          }
#endif
          SetupNodeManagement(1);
#if SUPPORT_ZW_ADD_NODE_DSK_TO_NETWORK
          if ((serFrameDataPtr[0] & ADD_NODE_MODE_MASK) == ADD_NODE_HOME_ID)
          {
            ZW_ADD_NODE_DSK_TO_NETWORK(serFrameDataPtr[0], &serFrameDataPtr[2], (serFrameDataPtr[1] != 0) ? ZCB_ComplHandler_ZW_NodeManagement : NULL);
          }
          else
#endif
          {
            ZW_ADD_NODE_TO_NETWORK(serFrameDataPtr[0], (serFrameDataPtr[1] != 0) ? ZCB_ComplHandler_ZW_NodeManagement : NULL);
          }
          break;
#endif

#if SUPPORT_ZW_REMOVE_NODE_FROM_NETWORK
        case FUNC_ID_ZW_REMOVE_NODE_FROM_NETWORK:
          /* HOST->ZW: mode | funcID */
#if SUPPORT_ADD_REMOVE_PROTECT
          if (ZW_NodeManagementRunning())
          {
            break;
          }
#endif
          SetupNodeManagement(1);
          ZW_REMOVE_NODE_FROM_NETWORK(serFrameDataPtr[0], (serFrameDataPtr[1] != 0) ? ZCB_ComplHandler_ZW_NodeManagement : NULL);
          break;
#endif


#if SUPPORT_ZW_REMOVE_NODE_ID_FROM_NETWORK
        case FUNC_ID_ZW_REMOVE_NODE_ID_FROM_NETWORK:
          /* HOST->ZW: mode | nodeID | funcID */
#if SUPPORT_ADD_REMOVE_PROTECT
          if (ZW_NodeManagementRunning())
          {
            break;
          }
#endif
          SetupNodeManagement(2);
          ZW_REMOVE_NODE_ID_FROM_NETWORK(serFrameDataPtr[0], serFrameDataPtr[1], (serFrameDataPtr[2] != 0) ? ZCB_ComplHandler_ZW_NodeManagement : NULL);
          break;
#endif

#if SUPPORT_ZW_CREATE_NEW_PRIMARY
        case FUNC_ID_ZW_CREATE_NEW_PRIMARY:
          /* HOST->ZW: mode | funcID */
#if SUPPORT_ADD_REMOVE_PROTECT
          if (ZW_NodeManagementRunning())
          {
            break;
          }
#endif
          SetupNodeManagement(1);
          ZW_CREATE_NEW_PRIMARY_CTRL(serFrameDataPtr[0], (serFrameDataPtr[1] != 0) ? ZCB_ComplHandler_ZW_NodeManagement : NULL);
          break;
#endif

#if SUPPORT_ZW_CONTROLLER_CHANGE
        case FUNC_ID_ZW_CONTROLLER_CHANGE:
          /* HOST->ZW: mode | funcID */
#if SUPPORT_ADD_REMOVE_PROTECT
          if (ZW_NodeManagementRunning())
          {
            break;
          }
#endif
          SetupNodeManagement(1);
          ZW_CONTROLLER_CHANGE(serFrameDataPtr[0], (serFrameDataPtr[1] != 0) ? ZCB_ComplHandler_ZW_NodeManagement : NULL);
          break;
#endif

#if SUPPORT_ZW_SET_LEARN_MODE
        case FUNC_ID_ZW_SET_LEARN_MODE:
          /* HOST->ZW: mode | funcID */
          /* ZW->HOST: retVal */
          retVal = TRUE;
#ifdef ZW_CONTROLLER
          SetupNodeManagement(1);
#endif  /* ZW_CONTROLLER */
#ifdef ZW_SLAVE
          funcID_ComplHandler_ZW_SetLearnMode = serFrameDataPtr[1];
#endif
#ifdef ZW_CONTROLLER_SINGLE
          ZW_SET_LEARN_MODE(serFrameDataPtr[0], (serFrameDataPtr[1] != 0) ? ZCB_ComplHandler_ZW_NodeManagement : NULL);
#else
          if (SERIALPI_SET_LEARN_MODE_LEARN_PLUS_OFFSET > serFrameDataPtr[0])
          {
            /* Plain ZW_SetLearnMode */
            /* ZW_SET_LEARN_MODE_DISABLE          0x00 */
            /* ZW_SET_LEARN_MODE_CLASSIC          0x01 */
            /* ZW_SET_LEARN_MODE_NWI              0x02 */
            /* ZW_SET_LEARN_MODE_NWE              0x03 */
#ifdef ZW_CONTROLLER
            ZW_SET_LEARN_MODE(serFrameDataPtr[0],
                              (serFrameDataPtr[1] != 0) ?
                               ZCB_ComplHandler_ZW_NodeManagement :
                               NULL);
#else
            ZW_SET_LEARN_MODE(serFrameDataPtr[0],
                              (serFrameDataPtr[1] != 0) ?
                               ZCB_ComplHandler_ZW_SetLearnMode :
                               NULL);
#endif
          }
          else
          {
            /* HOST want to use Network Management for inclusion/exclusion */
            /* We need to substract the SERIALPI_SET_LEARN_MODE_LEARN_PLUS_OFFSET to get: */
            /* E_NETWORK_LEARN_MODE_DISABLE =  0,      Disable learn process */
            /* E_NETWORK_LEARN_MODE_INCLUSION  = 1,    Enable the learn process to do an inclusion */
            /* E_NETWORK_LEARN_MODE_EXCLUSION  = 2,    Enable the learn process to do an exclusion */
            /* E_NETWORK_LEARN_MODE_EXCLUSION_NWE = 3  Enable the learn process to do a network wide exclusion */
            /* E_NETWORK_LEARN_MODE_INCLUSION_SMARTSTART = 4 Enable the learn process to initiate SMARTSTART inclusion */
            retVal = ZW_NetworkLearnModeStart(serFrameDataPtr[0] - SERIALPI_SET_LEARN_MODE_LEARN_PLUS_OFFSET);
          }
#endif  /* ZW_CONTROLLER_SINGLE */
          DoRespond();
          break;
#endif  /* SUPPORT_ZW_SET_LEARN_MODE */

#if SUPPORT_ZW_EXPLORE_REQUEST_INCLUSION
        case FUNC_ID_ZW_EXPLORE_REQUEST_INCLUSION:
          retVal = ZW_EXPLORE_REQUEST_INCLUSION();
          DoRespond();
          break;
#endif

#if SUPPORT_ZW_EXPLORE_REQUEST_EXCLUSION
        case FUNC_ID_ZW_EXPLORE_REQUEST_EXCLUSION:
          retVal = ZW_EXPLORE_REQUEST_EXCLUSION();
          DoRespond();
          break;
#endif

#if SUPPORT_ZW_REPLICATION_COMMAND_COMPLETE
        case FUNC_ID_ZW_REPLICATION_COMMAND_COMPLETE:
          /*  */
          ZW_REPLICATION_COMMAND_COMPLETE();
          set_state(stateIdle);
          break;
#endif

#if SUPPORT_ZW_REPLICATION_SEND_DATA
        case FUNC_ID_ZW_REPLICATION_SEND_DATA:
          /* nodeID | dataLength | pData[] | txOptions | funcID */
          {
            BYTE dataLength;
            retVal = 0;
            dataLength = serFrameDataPtr[1];
            if (dataLength > BUF_SIZE_RX)
            {
              dataLength = BUF_SIZE_RX;
            }
            for (i = 0; i < dataLength; i++)
            {
              workbuf[i] = serFrameDataPtr[2+i];
            }
            funcID_ComplHandler_ZW_ReplicationSendData = serFrameDataPtr[3 + dataLength];
            retVal = ZW_REPLICATION_SEND_DATA(serFrameDataPtr[0], workbuf, dataLength, serFrameDataPtr[2 + dataLength],
                                              (serFrameDataPtr[3 + dataLength] != 0) ? ZCB_ComplHandler_ZW_ReplicationSendData : NULL);
            DoRespond();
          }
          break;
#endif  /* SUPPORT_ZW_REPLICATION_SEND_DATA */

#if SUPPORT_ZW_ASSIGN_RETURN_ROUTE
        case FUNC_ID_ZW_ASSIGN_RETURN_ROUTE:
          /* srcNodeID | destNodeID | funcID */
          funcID_ComplHandler_ZW_AssignReturnRoute = serFrameDataPtr[2];
          retVal = ZW_ASSIGN_RETURN_ROUTE(serFrameDataPtr[0], serFrameDataPtr[1],
                                          (serFrameDataPtr[2] != 0) ? ZCB_ComplHandler_ZW_AssignReturnRoute : NULL);
          DoRespond();
          break;
#endif  /* SUPPORT_ZW_ASSIGN_RETURN_ROUTE */

#if SUPPORT_ZW_ASSIGN_PRIORITY_RETURN_ROUTE
        case FUNC_ID_ZW_ASSIGN_PRIORITY_RETURN_ROUTE:
          /* srcNodeID | destNodeID | route[5] | funcID */
          funcID_ComplHandler_ZW_AssignPriorityReturnRoute = serFrameDataPtr[7];
          retVal = ZW_AssignPriorityReturnRoute(serFrameDataPtr[0], serFrameDataPtr[1],
                                                &serFrameDataPtr[2],
                                                (serFrameDataPtr[7] != 0) ? ZCB_ComplHandler_ZW_AssignPriorityReturnRoute : NULL);
          DoRespond();
          break;
#endif  /* SUPPORT_ZW_ASSIGN_PRIORITY_RETURN_ROUTE */

#if SUPPORT_ZW_DELETE_RETURN_ROUTE
        case FUNC_ID_ZW_DELETE_RETURN_ROUTE:
          /* nodeID | funcID */
          funcID_ComplHandler_ZW_DeleteReturnRoute = serFrameDataPtr[1];
          retVal = ZW_DELETE_RETURN_ROUTE(serFrameDataPtr[0],
                                          (serFrameDataPtr[1] != 0) ? ZCB_ComplHandler_ZW_DeleteReturnRoute : NULL);
          DoRespond();
          break;
#endif  /* SUPPORT_ZW_DELETE_RETURN_ROUTE */

#if SUPPORT_ZW_ASSIGN_SUC_RETURN_ROUTE
        case FUNC_ID_ZW_ASSIGN_SUC_RETURN_ROUTE:
          /* srcNodeID | funcID */
          funcID_ComplHandler_netWork_Management = serFrameDataPtr[1];
          retVal = ZW_ASSIGN_SUC_RETURN_ROUTE(serFrameDataPtr[0],
                                              (serFrameDataPtr[1] != 0) ? ZCB_ComplHandler_ZW_netWork_Management : NULL);
          management_Func_ID = serFrameCmd;
          DoRespond();
          break;
#endif  /* SUPPORT_ZW_ASSIGN_SUC_RETURN_ROUTE */

#if SUPPORT_ZW_ASSIGN_PRIORITY_SUC_RETURN_ROUTE
        case FUNC_ID_ZW_ASSIGN_PRIORITY_SUC_RETURN_ROUTE:
          /* srcNodeID | route[5] | funcID */
          funcID_ComplHandler_netWork_Management = serFrameDataPtr[6];
          retVal = ZW_AssignPrioritySUCReturnRoute(serFrameDataPtr[0],
                                                   &serFrameDataPtr[1],
                                                   (serFrameDataPtr[6] != 0) ? ZCB_ComplHandler_ZW_netWork_Management : NULL);
          management_Func_ID = serFrameCmd;
          DoRespond();
          break;
#endif  /* SUPPORT_ZW_ASSIGN_SUC_RETURN_ROUTE */

#if SUPPORT_ZW_DELETE_SUC_RETURN_ROUTE
        case FUNC_ID_ZW_DELETE_SUC_RETURN_ROUTE:
          /* srcNodeID | funcID */
          funcID_ComplHandler_netWork_Management = serFrameDataPtr[1];
          retVal = ZW_DELETE_SUC_RETURN_ROUTE(serFrameDataPtr[0],
                                              (serFrameDataPtr[1] != 0) ? ZCB_ComplHandler_ZW_netWork_Management : NULL);
          management_Func_ID = serFrameCmd;
          DoRespond();
          break;
#endif  /* SUPPORT_ZW_DELETE_SUC_RETURN_ROUTE */

#if SUPPORT_ZW_SEND_SUC_ID
        case FUNC_ID_ZW_SEND_SUC_ID:
          /* destNodeID | txOptions | funcID */
          funcID_ComplHandler_ZW_SendSUC_ID = serFrameDataPtr[2];
          retVal = ZW_SEND_SUC_ID(serFrameDataPtr[0], serFrameDataPtr[1], (serFrameDataPtr[2] != 0) ? ZCB_ComplHandler_ZW_SendSUC_ID : NULL);
          DoRespond();
          break;
#endif  /* SUPPORT_ZW_SEND_SUC_ID */

#if SUPPORT_ZW_SET_SUC_NODE_ID
        case FUNC_ID_ZW_SET_SUC_NODE_ID:
          /* nodeID | SUCState | txOptions | capabilities | funcID */
          funcID_ComplHandler_ZW_SetSUCNodeID = serFrameDataPtr[4];
          retVal = ZW_SET_SUC_NODEID(serFrameDataPtr[0],
                                     serFrameDataPtr[1],
                                     serFrameDataPtr[2],
                                     serFrameDataPtr[3],
                                     (serFrameDataPtr[4] != 0) ? ZCB_ComplHandler_ZW_SetSUCNodeID : NULL);
          DoRespond();
          break;
#endif  /* SUPPORT_ZW_SET_SUC_NODE_ID */

#if SUPPORT_ZW_GET_SUC_NODE_ID
        case FUNC_ID_ZW_GET_SUC_NODE_ID:
          /* */
          retVal = ZW_GET_SUC_NODEID();
          DoRespond();
          break;
#endif  /* SUPPORT_ZW_GET_SUC_NODE_ID */

#if SUPPORT_ZW_REMOVE_FAILED_NODE_ID
        case FUNC_ID_ZW_REMOVE_FAILED_NODE_ID:
          /* nodeID | funcID */
          funcID_ComplHandler_ZW_RemoveFailedNodeID = serFrameDataPtr[1];
          retVal = ZW_REMOVE_FAILED_NODE_ID(serFrameDataPtr[0],
                                            (serFrameDataPtr[1] != 0) ?
                                            ZCB_ComplHandler_ZW_RemoveFailedNodeID : NULL);
          DoRespond();
          break;
#endif  /* SUPPORT_ZW_REMOVE_FAILED_NODE_ID */

#if SUPPORT_ZW_IS_FAILED_NODE_ID
        case FUNC_ID_ZW_IS_FAILED_NODE_ID:
          /* nodeID */
          retVal = ZW_IS_FAILED_NODE_ID(serFrameDataPtr[0]);
          DoRespond();
          break;
#endif  /* SUPPORT_ZW_IS_FAILED_NODE_ID */

#if SUPPORT_ZW_REPLACE_FAILED_NODE
        case FUNC_ID_ZW_REPLACE_FAILED_NODE:
          /* nodeID | funcID */
          funcID_ComplHandler_ZW_ReplaceFailedNode = serFrameDataPtr[1];
          retVal = ZW_REPLACE_FAILED_NODE(serFrameDataPtr[0],
                                          /* Use NormalPower for including the replacement */
                                          TRUE,
                                          (serFrameDataPtr[1] != 0) ?
                                          ZCB_ComplHandler_ZW_ReplaceFailedNode : NULL);
          DoRespond();
          break;
#endif  /* SUPPORT_ZW_REPLACE_FAILED_NODE */

#if SUPPORT_GET_ROUTING_TABLE_LINE
        case FUNC_ID_GET_ROUTING_TABLE_LINE:
          /* HOST->ZW: bLine | bRemoveBad | bRemoveNonReps | funcID */
          ZW_GET_ROUTING_INFO(serFrameDataPtr[0], compl_workbuf,
                              ((serFrameDataPtr[1]) ? GET_ROUTING_INFO_REMOVE_BAD : 0) |
                              ((serFrameDataPtr[2]) ? GET_ROUTING_INFO_REMOVE_NON_REPS : 0));
          DoRespond_workbuf(MAX_NODEMASK_LENGTH);
          break;
#endif  /* SUPPORT_GET_ROUTING_TABLE_LINE */

#if SUPPORT_ZW_SET_ROUTING_INFO
        case FUNC_ID_ZW_SET_ROUTING_INFO:
          /* HOST->ZW: nodeId | NodeMask[29] */
          retVal = ZW_SetRoutingInfo(serFrameDataPtr[0], MAX_NODEMASK_LENGTH, &serFrameDataPtr[1]);
          DoRespond();
          break;
#endif

#if SUPPORT_GET_TX_COUNTER
        case FUNC_ID_GET_TX_COUNTER:
          /* */
          /* Get the transmit counter */
          BYTE_IN_AR(compl_workbuf, 0) = ZW_TX_COUNTER;
          DoRespond_workbuf(1);
          break;
#endif  /* SUPPORT_GET_TX_COUNTER */

#if SUPPORT_RESET_TX_COUNTER
       case FUNC_ID_RESET_TX_COUNTER:
          {
            /* */
            /* Reset the transmit counter */
            ZW_TX_COUNTER = 0;
            set_state(stateIdle);
          }
          break;
#endif  /* SUPPORT_RESET_TX_COUNTER */

#if SUPPORT_STORE_NODEINFO
       case FUNC_ID_STORE_NODEINFO:
          /*0         1                   2                   3                 4               5               6
          /* nodeID | nodeInfo.capability|nodeInfo.security|nodeInfo.reserved|nodeInfo.basic|nodeInfo.generic|nodeInfo.specific|funcId*/
          funcID_ComplHandler_ZW_StoreNodeInfo = serFrameDataPtr[7];

          retVal = ZW_STORE_NODE_INFO(serFrameDataPtr[0], &serFrameDataPtr[1],
                                      (serFrameDataPtr[7] != 0) ? ZCB_ComplHandler_ZW_StoreNodeInfo : NULL);
          DoRespond();
          break;
#endif  /* SUPPORT_STORE_NODEINFO */

#if SUPPORT_STORE_HOMEID
       case FUNC_ID_STORE_HOMEID:
          /* homeID1 | homeID2 | homeID3 | homeID4 | nodeID */
          /* Store homeID and Node ID. */
          ZW_STORE_HOME_ID(&serFrameDataPtr[0], serFrameDataPtr[4]);
          set_state(stateIdle);
          break;
#endif /* SUPPORT_STORE_HOMEID */

#if SUPPORT_LOCK_ROUTE_RESPONSE
       case FUNC_ID_LOCK_ROUTE_RESPONSE:
          /* HOST->ZW: lockID */
          /* Lock response routes / Last Working Routes. lockID == nodeID for locking. lockID == 0x00 to unlock */
          ZW_LOCK_RESPONSE_ROUTE(serFrameDataPtr[0]);
          set_state(stateIdle);
          break;
#endif

#if SUPPORT_ZW_GET_PRIORITY_ROUTE
       case FUNC_ID_ZW_GET_PRIORITY_ROUTE:
          /* HOST->ZW: nodeID */
          /* ZW->HOST: nodeID | anyRouteFound | repeater0 | repeater1 | repeater2 | repeater3 | routespeed */
          BYTE_IN_AR(compl_workbuf, 0) = serFrameDataPtr[0];
          BYTE_IN_AR(compl_workbuf, 1) = ZW_GET_PRIORITY_ROUTE(serFrameDataPtr[0], &compl_workbuf[2]);
          DoRespond_workbuf(7);
          break;
#endif

#if SUPPORT_ZW_SET_PRIORITY_ROUTE
       case FUNC_ID_ZW_SET_PRIORITY_ROUTE:
          /* HOST->ZW: nodeID | repeater0 | repeater1 | repeater2 | repeater3 | routespeed */
          /* ZW->HOST: nodeID | routeUpdated */
          BYTE_IN_AR(compl_workbuf, 0) = serFrameDataPtr[0];
          if (9 <= serFrameLen)
          {
            /* Set Priority Route Devkit 6.6x */
            BYTE_IN_AR(compl_workbuf, 1) = ZW_SET_PRIORITY_ROUTE(serFrameDataPtr[0], &serFrameDataPtr[1]);
          }
          else
          {
            /* Clear/Release Golden Route - Devkit 6.6x+ */
            BYTE_IN_AR(compl_workbuf, 1) = ZW_SET_PRIORITY_ROUTE(serFrameDataPtr[0], NULL);
          }
          DoRespond_workbuf(2);
          break;
#endif

#ifdef ZW_ENABLE_RTC
        case FUNC_ID_CLOCK_SET:
          /* pNewTime.weekday | pNewTime.hour | pNewTime.minute */
          clkTime.weekday = serFrameDataPtr[0];
          clkTime.hour = serFrameDataPtr[1];
          clkTime.minute = serFrameDataPtr[2];
          retVal = ClockSet(&clkTime);
          DoRespond();
          break;

        case FUNC_ID_CLOCK_GET:
          /* */
          ClockGet(&clkTime);
          BYTE_IN_AR(compl_workbuf, 0) = clkTime.weekday;
          BYTE_IN_AR(compl_workbuf, 1) = clkTime.hour;
          BYTE_IN_AR(compl_workbuf, 2) = clkTime.minute;
          DoRespond_workbuf(3);
          break;

        case FUNC_ID_CLOCK_CMP:
          /* pNewTime.weekday | pNewTime.hour | pNewTime.minute */
          clkTime.weekday = serFrameDataPtr[0];
          clkTime.hour = serFrameDataPtr[1];
          clkTime.minute = serFrameDataPtr[2];
          retVal = ClockCmp(&clkTime);
          DoRespond();
          break;

        case FUNC_ID_RTC_TIMER_CREATE:
          /* RTC_TIMER.status | RTC_TIMER.timeOn (3 bytes) | RTC_TIMER.timeOff (3 bytes) | RTC_TIMER.repeats | RTC_TIMER.parm | RTC_TIMER.funcID | funcID */
          timer.status = serFrameDataPtr[0];
          timer.timeOn.weekday = serFrameDataPtr[1];
          timer.timeOn.hour = serFrameDataPtr[2];
          timer.timeOn.minute = serFrameDataPtr[3];
          timer.timeOff.weekday = serFrameDataPtr[4];
          timer.timeOff.hour = serFrameDataPtr[5];
          timer.timeOff.minute = serFrameDataPtr[6];
          timer.repeats = serFrameDataPtr[7];
          timer.parm = serFrameDataPtr[8];
          timer.func = rtcArray[serFrameDataPtr[9]];

          retVal = (BYTE)-1;
          if (serFrameDataPtr[9] < RTC_TIMER_MAX)
          {
            funcID_ComplHandler_RTCTimerCreate = serFrameDataPtr[10];
            retVal = RTCTimerCreate(&timer, (serFrameDataPtr[10] != 0) ? ComplHandler_RTCTimerCreate : NULL);
          }
          DoRespond();
          break;

        case FUNC_ID_RTC_TIMER_READ:
          /* timerHandle */
          RespondRTCTimer();
          break;

        case FUNC_ID_RTC_TIMER_DELETE:
          /* timerHandle */
          RTCTimerDelete(serFrameDataPtr[0]);
          set_state(stateIdle);
          break;
#endif

#if SUPPORT_ZW_GET_VERSION
        case FUNC_ID_ZW_GET_VERSION:
          /* */
          BYTE_IN_AR(compl_workbuf, 12) = ZW_Version((BYTE *)compl_workbuf);
          DoRespond_workbuf(13);
          break;
#endif

#if SUPPORT_ZW_GET_PROTOCOL_VERSION
        case FUNC_ID_ZW_GET_PROTOCOL_VERSION:
          /*  */
          ZW_GetProtocolVersion((PROTOCOL_VERSION *)compl_workbuf);
          DoRespond_workbuf(sizeof(PROTOCOL_VERSION));
          break;
#endif

#if SUPPORT_SERIAL_API_APPL_NODE_INFORMATION
        case FUNC_ID_SERIAL_API_APPL_NODE_INFORMATION:
          /* listening | generic | specific | parmLength | nodeParms[] */
          applNodeInfo_deviceOptionsMask = serFrameDataPtr[0];
          applNodeInfo_nodeType_generic = serFrameDataPtr[1];
          applNodeInfo_nodeType_specific = serFrameDataPtr[2];
          applNodeInfo_parmLength = (serFrameDataPtr[3] < sizeof(applNodeInfo_nodeParm)) ? serFrameDataPtr[3] : sizeof(applNodeInfo_nodeParm);
          for (i = 0; i < applNodeInfo_parmLength; i++)
          {
            applNodeInfo_nodeParm[i] = serFrameDataPtr[4+i];
#if SUPPORT_SERIAL_API_APPL_NODE_INFORMATION_CMD_CLASSES
            applNodeInfo_unincluded_nodeParm[i] = serFrameDataPtr[4+i];
#endif
          }
          while (i < sizeof(applNodeInfo_nodeParm))
          {
            applNodeInfo_nodeParm[i] = 0;
#if SUPPORT_SERIAL_API_APPL_NODE_INFORMATION_CMD_CLASSES
            applNodeInfo_unincluded_nodeParm[i] = 0;
#endif
            i++;
          }
#if SUPPORT_SERIAL_API_APPL_NODE_INFORMATION_CMD_CLASSES
          applNodeInfo_unincluded_parmLength = applNodeInfo_parmLength;
          m_AppNIF.cmdClassListNonSecureCount = applNodeInfo_unincluded_parmLength;
          m_AppNIF.deviceOptionsMask = applNodeInfo_deviceOptionsMask;
          m_AppNIF.nodeType.generic = applNodeInfo_nodeType_generic;
          m_AppNIF.nodeType.specific = applNodeInfo_nodeType_specific;
          Transport_OnApplicationInitSW( &m_AppNIF);
#endif
          SaveApplicationSettings();
          set_state(stateIdle);
          break;
#endif

#if SUPPORT_SERIAL_API_APPL_NODE_INFORMATION_CMD_CLASSES
        case FUNC_ID_SERIAL_API_APPL_NODE_INFORMATION_CMD_CLASSES:
          /* unincluded_parmLength | unincluded_nodeParm[] | */
          /* included_unsecure_parmLength | included_unsecure_nodeParm[] |
          /* included_secure_parmLength | included_secure_nodeParm[] */
          for (i = 0; i < APPL_NODEPARM_MAX; i++)
          {
            applNodeInfo_unincluded_nodeParm[i] = 0;
            applNodeInfo_included_unsecure_nodeParm[i] = 0;
            applNodeInfo_included_secure_nodeParm[i] = 0;
          }
          applNodeInfo_unincluded_parmLength = serFrameDataPtr[0];
          for (i = 0; ((i < applNodeInfo_unincluded_parmLength) && (APPL_NODEPARM_MAX > i)); i++)
          {
            applNodeInfo_unincluded_nodeParm[i] = serFrameDataPtr[1 + i];
          }
          applNodeInfo_included_unsecure_parmLength = serFrameDataPtr[applNodeInfo_unincluded_parmLength + 1];
          for (i = 0; ((i < applNodeInfo_included_unsecure_parmLength) && (APPL_NODEPARM_MAX > i)); i++)
          {
            applNodeInfo_included_unsecure_nodeParm[i] = serFrameDataPtr[applNodeInfo_unincluded_parmLength + 1 + 1 + i];
          }
          applNodeInfo_included_secure_parmLength = serFrameDataPtr[applNodeInfo_unincluded_parmLength + applNodeInfo_included_unsecure_parmLength + 1 + 1];
          for (i = 0; ((i < applNodeInfo_included_secure_parmLength) && (APPL_NODEPARM_MAX > i)); i++)
          {
            applNodeInfo_included_secure_nodeParm[i] = serFrameDataPtr[applNodeInfo_unincluded_parmLength + applNodeInfo_included_unsecure_parmLength + 1 + 1 + 1 + i];
          }
          applNodeInfo_unincluded_parmLength = ((APPL_NODEPARM_MAX < applNodeInfo_unincluded_parmLength) ?  APPL_NODEPARM_MAX : applNodeInfo_unincluded_parmLength);
          applNodeInfo_included_unsecure_parmLength = ((APPL_NODEPARM_MAX < applNodeInfo_included_unsecure_parmLength) ?  APPL_NODEPARM_MAX : applNodeInfo_included_unsecure_parmLength);
          applNodeInfo_included_secure_parmLength = ((APPL_NODEPARM_MAX < applNodeInfo_included_secure_parmLength) ?  APPL_NODEPARM_MAX : applNodeInfo_included_secure_parmLength);
          m_AppNIF.cmdClassListNonSecureCount = applNodeInfo_unincluded_parmLength;
          m_AppNIF.cmdClassListNonSecureIncludedSecureCount = applNodeInfo_included_unsecure_parmLength;
          m_AppNIF.cmdClassListSecureCount = applNodeInfo_included_secure_parmLength;
          m_AppNIF.deviceOptionsMask = applNodeInfo_deviceOptionsMask;
          m_AppNIF.nodeType.generic = applNodeInfo_nodeType_generic;
          m_AppNIF.nodeType.specific = applNodeInfo_nodeType_specific;
          Transport_OnApplicationInitSW( &m_AppNIF);
          SaveApplicationSettings();
          retVal = TRUE;
          DoRespond();
          break;
#endif

#if SUPPORT_ZW_SEND_SLAVE_NODE_INFORMATION
        case FUNC_ID_ZW_SEND_SLAVE_NODE_INFORMATION:
          /* srcNode | destNode | txOptions | funcID */
          funcID_ComplHandler_ZW_SendSlaveNodeInformation = serFrameDataPtr[3];
          retVal = ZW_SendSlaveNodeInformation(serFrameDataPtr[0], serFrameDataPtr[1], serFrameDataPtr[2],
                                               (serFrameDataPtr[3] != 0) ? ZCB_ComplHandler_ZW_SendSlaveNodeInformation : NULL);
          DoRespond();
          break;
#endif

#if SUPPORT_ZW_SEND_SLAVE_DATA
        case FUNC_ID_ZW_SEND_SLAVE_DATA:
          /* srcID | destID | dataLength | pData[] | txOptions | funcID */
          {
            BYTE dataLength = serFrameDataPtr[2];
            if (dataLength > BUF_SIZE_RX)
            {
              dataLength = BUF_SIZE_RX;
            }
            for (i = 0; i < dataLength; i++)
            {
              workbuf[i] = serFrameDataPtr[3+i];
            }
            funcID_ComplHandler_ZW_SendSlaveData = serFrameDataPtr[4+dataLength];
            retVal = ZW_SendSlaveData(serFrameDataPtr[0], serFrameDataPtr[1], workbuf, dataLength, serFrameDataPtr[3 + dataLength],
                                      (serFrameDataPtr[4 + dataLength] != 0) ? ComplHandler_ZW_SendSlaveData : NULL);
            DoRespond();
          }
          break;
#endif

#if SUPPORT_ZW_SET_SLAVE_LEARN_MODE
        case FUNC_ID_ZW_SET_SLAVE_LEARN_MODE:
          /* node | mode | funcID */
          funcID_ComplHandler_ZW_SetSlaveLearnMode = serFrameDataPtr[2];
          retVal = ZW_SET_SLAVE_LEARN_MODE(serFrameDataPtr[0], serFrameDataPtr[1],
                                           (serFrameDataPtr[2] != 0) ? ZCB_ComplHandler_ZW_SetSlaveLearnMode : NULL);
          DoRespond();
          break;
#endif

#if SUPPORT_ZW_SEND_TEST_FRAME
        case FUNC_ID_ZW_SEND_TEST_FRAME:
          /* node | powerlevel | funcID */
          funcID_ComplHandler_ZW_SendTestFrame = serFrameDataPtr[2];
          retVal = ZW_SEND_TEST_FRAME(serFrameDataPtr[0], serFrameDataPtr[1],
                                       (serFrameDataPtr[2] != 0) ? ZCB_ComplHandler_ZW_SendTestFrame : NULL);
          DoRespond();
          break;
#endif

#if SUPPORT_SERIAL_API_APPL_SLAVE_NODE_INFORMATION
        case FUNC_ID_SERIAL_API_APPL_SLAVE_NODE_INFORMATION:
          /* srcNodeID | listening | generic | specific | parmLength | nodeParms[] */
          /* serFrameDataPtr[0] = srcNodeID */
          applSlaveNodeInfo_deviceOptionsMask = serFrameDataPtr[1];
          applSlaveNodeInfo_nodeType_generic = serFrameDataPtr[2];
          applSlaveNodeInfo_nodeType_specific = serFrameDataPtr[3];
          applSlaveNodeInfo_parmLength = (serFrameDataPtr[4] < sizeof(applSlaveNodeInfo_nodeParm)) ? serFrameDataPtr[4] : sizeof(applSlaveNodeInfo_nodeParm);
          for (i = 0; (i < applSlaveNodeInfo_parmLength); i++)
          {
            applSlaveNodeInfo_nodeParm[i] = serFrameDataPtr[5 + i];
          }
          while (i < sizeof(applSlaveNodeInfo_nodeParm))
          {
            applSlaveNodeInfo_nodeParm[i] = 0;
            i++;
          }
          set_state(stateIdle);
          break;
#endif

#if SUPPORT_ZW_IS_VIRTUAL_NODE
        case FUNC_ID_ZW_IS_VIRTUAL_NODE:
          /* */
          retVal = ZW_IS_VIRTUAL_NODE(serFrameDataPtr[0]);
          DoRespond();
          break;
#endif

#if SUPPORT_ZW_GET_VIRTUAL_NODES
        case FUNC_ID_ZW_GET_VIRTUAL_NODES:
          /* */
          ZW_GET_VIRTUAL_NODES(compl_workbuf);
          DoRespond_workbuf(ZW_MAX_NODES / 8);
          break;
#endif

#if SUPPORT_SERIAL_API_GET_INIT_DATA
        case FUNC_ID_SERIAL_API_GET_INIT_DATA:
          /*  */
          BYTE_IN_AR(compl_workbuf, 0) = SERIAL_API_VER;
          BYTE_IN_AR(compl_workbuf, 1) = 0; /* Flag byte - default: controller api, no timer support, no primary, no SUC */
#ifdef ZW_CONTROLLER
          if (!ZW_PRIMARYCTRL())
          {
            BYTE_IN_AR(compl_workbuf, 1) |= GET_INIT_DATA_FLAG_SECONDARY_CTRL; /* Set Primary/secondary bit */
          }
#ifdef ZW_CONTROLLER_STATIC
          if (ZW_GET_CONTROLLER_CAPABILITIES() & CONTROLLER_IS_SUC)    /* if (ZW_IS_SUC_ACTIVE()) */
          {
            BYTE_IN_AR(compl_workbuf, 1) |= GET_INIT_DATA_FLAG_IS_SUC; /* Set SUC bit if active */
          }
#endif /*ZW_CONTROLLER_STATIC*/
          {
            NODEINFO nodeInfo;

            /* compl_workbuf[1] is already set to controller api*/
            BYTE_IN_AR(compl_workbuf, 2) = ZW_MAX_NODES / 8;     /* node bitmask length */
            /* Remember to clear the buffer!! */
            for (i = 3; i < (ZW_MAX_NODES / 8) + 3; i++)
            {
              BYTE_IN_AR(compl_workbuf, i) = 0;
            }
            /* Next ZW_MAX_NODES/8 = 29 bytes of compl_workbuf reserved for node bitmask */
            for (i = 1; i <= ZW_MAX_NODES; i++)
            {
              ZW_GetNodeProtocolInfo(i, &nodeInfo);
              if (nodeInfo.nodeType.generic)
              {
                ZW_NodeMaskSetBit(compl_workbuf + 3, i);
              }
              ZW_POLL();
            }
          }
          BYTE_IN_AR(compl_workbuf, 3 + (ZW_MAX_NODES/8)) = ZW_CHIP_TYPE;
          BYTE_IN_AR(compl_workbuf, 4 + (ZW_MAX_NODES/8)) = ZW_CHIP_REVISION;
          DoRespond_workbuf(5 + (ZW_MAX_NODES/8));
#endif
#ifdef ZW_SLAVE
          BYTE_IN_AR(compl_workbuf, 1) |= GET_INIT_DATA_FLAG_SLAVE_API;  /* Flag byte */
          BYTE_IN_AR(compl_workbuf, 2) = 0;     /* node bitmask length */
          BYTE_IN_AR(compl_workbuf, 3) = ZW_CHIP_TYPE;
          BYTE_IN_AR(compl_workbuf, 4) = ZW_CHIP_REVISION;
          DoRespond_workbuf(5);
#endif /*ZW_SLAVE*/
          break;
#endif

#if SUPPORT_ZW_GET_CONTROLLER_CAPABILITIES
        case FUNC_ID_ZW_GET_CONTROLLER_CAPABILITIES:
          /* HOST->ZW: no params defined */
          retVal = ZW_GET_CONTROLLER_CAPABILITIES();
          DoRespond();
          break;
#endif

#if SUPPORT_ZW_REQUEST_NODE_INFO
        case FUNC_ID_ZW_REQUEST_NODE_INFO:
          /* HOST->ZW: nodeID */
          retVal = ZW_RequestNodeInfo(serFrameDataPtr[0], ZCB_ComplHandler_ZW_RequestNodeInfo);
          DoRespond();
          break;
#endif

#if SUPPORT_SERIAL_API_SET_TIMEOUTS
        case FUNC_ID_SERIAL_API_SET_TIMEOUTS:
          /* HOST->ZW: RX_ACK_timeout | RX_BYTE_timeout */
          /* ZW->HOST: RES | oldRX_ACK_timeout | oldRX_BYTE_timeout */
          BYTE_IN_AR(compl_workbuf, 0) = timeOutRX_ACK;     /* Respond with the old timeout settings */
          BYTE_IN_AR(compl_workbuf, 1) = timeOutRX_BYTE;
          timeOutRX_ACK = serFrameDataPtr[0];   /* Max time to wait for ACK after frame transmission in 10ms ticks */
          timeOutRX_BYTE = serFrameDataPtr[1];  /* Max time to wait for next byte when collecting a new frame in 10ms ticks */
          /* Respond with the old timeout settings */
          DoRespond_workbuf(2);
          break;
#endif

#if SUPPORT_SERIAL_API_SOFT_RESET
        case FUNC_ID_SERIAL_API_SOFT_RESET:
          /* HOST->ZW: No params defined */
          ZW_WATCHDOG_ENABLE;
          while (1)
          {
            ;
          }
          break;
#endif

#if SUPPORT_SERIAL_API_STARTUP_NOTIFICATION
#endif

#if SUPPORT_SERIAL_API_SETUP
        case FUNC_ID_SERIAL_API_SETUP:
          /**
           *  HOST->ZW: Cmd | [CmdData[]]
           *  ZW->HOST: Cmd | CmdRes[]
           *  We assume operation is nonesuccessful
           */
          retVal = FALSE;
          if ((FRAME_LENGTH_MIN + 1) <= serFrameLen)
          {
            i = 1;
            BYTE_IN_AR(compl_workbuf, 0) = serFrameDataPtr[0];
            switch (serFrameDataPtr[0])
            {
              /**
               *  Report which SerialAPI Setup commands are supported beside the SERIAL_API_SETUP_CMD_SUPPORTED
               */
              case SERIAL_API_SETUP_CMD_SUPPORTED:
                /**
                 *  HOST->ZW: SERIAL_API_SETUP_CMD_SUPPORTED
                 *  ZW->HOST: SERIAL_API_SETUP_CMD_SUPPORTED | supportedSerialAPISetupCmds
                 */
                BYTE_IN_AR(compl_workbuf, i++) = (SERIAL_API_SETUP_CMD_TX_STATUS_REPORT |
#ifndef slave_routing
                                                  SERIAL_API_SETUP_CMD_TX_POWERLEVEL_SET |
                                                  SERIAL_API_SETUP_CMD_TX_POWERLEVEL_GET |
#endif
                                                  SERIAL_API_SETUP_CMD_TX_GET_MAX_PAYLOAD_SIZE);
                break;

              case SERIAL_API_SETUP_CMD_TX_STATUS_REPORT:
                /**
                 *  HOST->ZW: SERIAL_API_SETUP_CMD_TX_STATUS_REPORT | EnableTxStatusReport
                 *  ZW->HOST: SERIAL_API_SETUP_CMD_TX_STATUS_REPORT | retVal
                 */
                if ((FRAME_LENGTH_MIN + 2) <= serFrameLen)
                {
                  /* Do we enable or disable */
                  bTxStatusReportEnabled = (0 != serFrameDataPtr[1]);
                  /* Operation successful */
                  retVal = TRUE;
                }
                BYTE_IN_AR(compl_workbuf, i++) = retVal;
                break;

#ifndef slave_routing
              case SERIAL_API_SETUP_CMD_TX_POWERLEVEL_SET:
                /**
                 *  HOST->ZW: SERIAL_API_SETUP_CMD_TX_POWER_SET | NormalPowerCh0 | NormalPowerCh1 |
                 *            NormalPowerCh2 | LowPowerCh0 |  LowPowerCh1 |  LowPowerCh2
                 *  ZW->HOST: SERIAL_API_SETUP_CMD_TX_POWER_SET | retVal
                 */
                if ((FRAME_LENGTH_MIN + 7) <= serFrameLen)
                {
                  sPowerlevels.normal[0] = CheckPowerlevel(serFrameDataPtr[1]);
                  sPowerlevels.normal[1] = CheckPowerlevel(serFrameDataPtr[2]);
                  sPowerlevels.normal[2] = CheckPowerlevel(serFrameDataPtr[3]);
                  sPowerlevels.low[0] = CheckPowerlevel(serFrameDataPtr[4]);
                  sPowerlevels.low[1] = CheckPowerlevel(serFrameDataPtr[5]);
                  sPowerlevels.low[2] = CheckPowerlevel(serFrameDataPtr[6]);

                  SaveApplicationSettings();
                  retVal = TRUE;
                }
                i = 1;
                BYTE_IN_AR(compl_workbuf, i++) = retVal;
                break;

              case SERIAL_API_SETUP_CMD_TX_POWERLEVEL_GET:
                /**
                 *  HOST->ZW: SERIAL_API_SETUP_CMD_TX_POWER_GET
                 *  ZW->HOST: SERIAL_API_SETUP_CMD_TX_POWER_GET | config |
                 *            NormalPowerCh0 | NormalPowerCh1 | NormalPowerCh2 |
                 *            LowPowerCh0 |  LowPowerCh1 |  LowPowerCh2
                 */
                if (sPowerlevels.normal[0])
                {
                  /* SerialAPI has a host configured Tx power setting */
                  BYTE_IN_AR(compl_workbuf, i++) = POWERLEVEL_HOST_CONFIG;
                  memcpy(&compl_workbuf[i], (BYTE*)&sPowerlevels, sizeof(sPowerlevels));
                }
                else
                {
                  /* Tx power setting in the Z-Wave firmware are used */
                  BYTE_IN_AR(compl_workbuf, i++) = POWERLEVEL_FIRMWARE_CONFIG;
                  ZW_GetDefaultPowerLevels(&compl_workbuf[i]);
                }
                i+=sizeof(sPowerlevels);
                break;
#endif

              case SERIAL_API_SETUP_CMD_TX_GET_MAX_PAYLOAD_SIZE:
                /**
                 *  HOST->ZW: SERIAL_API_SETUP_CMD_TX_GET_MAX_PAYLOAD_SIZE
                 *  ZW->HOST: SERIAL_API_SETUP_CMD_TX_GET_MAX_PAYLOAD_SIZE | txMaxPayloadSize
                 */
                BYTE_IN_AR(compl_workbuf, i++) = ZW_GetMaxPayloadSize();
                break;

                /* All other commands are unsupported */
              default:
                /**
                 *  HOST->ZW: [SomeUnsupportedCmd] | [SomeData]
                 *  ZW->HOST: SERIAL_API_SETUP_CMD_UNSUPPORTED | [SomeUnsupportedCmd]
                 */
                BYTE_IN_AR(compl_workbuf, 0) = SERIAL_API_SETUP_CMD_UNSUPPORTED;
                BYTE_IN_AR(compl_workbuf, i++) = serFrameDataPtr[0];
                break;
            }
            DoRespond_workbuf(i);
          }
          else
          {
            DoRespond();
          }
          break;
#endif

#if SUPPORT_SERIAL_API_GET_CAPABILITIES
        case FUNC_ID_SERIAL_API_GET_CAPABILITIES:
          /* HOST->ZW: no params defined */
          /* ZW->HOST: RES | 0x07 | */
          /*  SERIAL_APPL_VERSION | SERIAL_APPL_REVISION | SERIALAPI_MANUFACTURER_ID1 | SERIALAPI_MANUFACTURER_ID2 | */
          /*  SERIALAPI_MANUFACTURER_PRODUCT_TYPE1 | SERIALAPI_MANUFACTURER_PRODUCT_TYPE2 | */
          /*  SERIALAPI_MANUFACTURER_PRODUCT_ID1 | SERIALAPI_MANUFACTURER_PRODUCT_ID2 | FUNCID_SUPPORTED_BITMASK[] */
          Respond(serFrameCmd, SERIALAPI_CAPABILITIES, sizeof(SERIALAPI_CAPABILITIES));
          break;
#endif

#if SUPPORT_SERIAL_API_POWER_MANAGEMENT
        case FUNC_ID_SERIAL_API_POWER_MANAGEMENT:
          /* HOST-ZW: */
          if ((FRAME_LENGTH_MIN + 1) <= serFrameLen)
          {
            switch (serFrameDataPtr[0])
            {
              case PM_PIN_UP_CONFIGURATION_CMD:
                /* HOST-ZW: */
                if ((PM_PHYSICAL_PIN_UNDEFINED != powerManagementPoweredUp.bPin)
                    && (-1 != poweredUpTimerHandle))
                {
                  poweredUpTransition = 0;
                  ZW_TIMER_CANCEL(poweredUpTimerHandle);
                  poweredUpTimerHandle = -1;
                }
                /* Reset current PoweredUp pin to UNDEFINED */
                powerManagementPoweredUp.bPin = PM_PHYSICAL_PIN_UNDEFINED;
                /* First check if pin selected is inside the defined pin range */
                /* values 0x20, 0x21, 0x25, 0x26, 0x27, 0x32 and 0x33 are inside */
                /* range but should not be used. */
                if (((FRAME_LENGTH_MIN + 2) <= serFrameLen)
                    && (PM_PHYSICAL_PIN_MAX >= serFrameDataPtr[1])
                    && (0x07 >= (serFrameDataPtr[1] & 0x0F)))
                {
                  powerManagementPoweredUp.bPin = serFrameDataPtr[1] + PM_PHYSICAL_PIN_OFFSET;
                  /* Any parameter for setting PoweredUp pin active level */
                  if ((FRAME_LENGTH_MIN + 3) <= serFrameLen)
                  {
                    /* Set PoweredUp pin active level according to received setting */
                    powerManagementPoweredUp.bEnableLevel = (0 != serFrameDataPtr[2]) ? 1 : 0;
                  }
                  else
                  {
                    /* PoweredUp pin is Default active LOW */
                    powerManagementPoweredUp.bEnableLevel = 0;
                  }
                  /* Set Powered Up pin to input */
                  ZW_PortPinIn(powerManagementPoweredUp.bPin);
                  poweredUpTransition = 0;
                  poweredUpTimerHandle = ZW_TIMER_START(ZCB_powerManagementPoweredUpPinActive, 1, TIMER_FOREVER);
                }
                set_state(stateIdle);
                break;

              case PM_MODE_CONFIGURATION_CMD:
                /* HOST-ZW: */
                /* For now input pull-Ups are not set */
                /* Reset all powerManagement WakeUp Pins to UNDEFINED. */
                for (i = 0; i < PM_IO_PIN_MAX; i++)
                {
                  if (PM_PHYSICAL_PIN_UNDEFINED != powerManagementWakeUpPinConf[i].bPin)
                  {
                    /* Reset pin to Tri-state */
                    ZW_PortPinIn(powerManagementWakeUpPinConf[i].bPin);
                  }
                  powerManagementWakeUpPinConf[i].bPin = PM_PHYSICAL_PIN_UNDEFINED;
                }
                /* No HOST Wakeup IO pins are defined -> no Power Management mode active */
                powerManagementState = POWER_MODE_IDLE;
                /* Number of Power Management pins. */
                i = serFrameDataPtr[1];
                if ((0 < i)
                    && (PM_IO_PIN_MAX >= i)
                    && (serFrameLen >= (FRAME_LENGTH_MIN + 2 + (i << 1))))
                {
                  while (i--)
                  {
                    /* Check if pin selected is inside the defined pin range */
                    /* values 0x20, 0x21, 0x25, 0x26, 0x27, 0x32 and 0x33 are inside */
                    /* range but should not be used. */
                    if ((PM_PHYSICAL_PIN_MAX >= serFrameDataPtr[2 + (i << 1)])
                        && (0x07 >= (serFrameDataPtr[2 + (i << 1)] & 0x0F)))
                    {
                      powerManagementWakeUpPinConf[i].bPin = serFrameDataPtr[2 + (i << 1)] + PM_PHYSICAL_PIN_OFFSET;
                      powerManagementWakeUpPinConf[i].bEnableLevel = (0 != serFrameDataPtr[2 + 1 + (i << 1)]) ? 1 : 0;
                    }
                    else
                    {
                      /* Undefined IO Pin - */
                      break;
                    }
                  }
                  /* We have now defined HOST WakeUp IO Pins -> In Power Management RUNNING state */
                  powerManagementState = POWER_MODE_RUNNING;
                }
                set_state(stateIdle);
                break;

              case PM_POWERUP_ZWAVE_CONFIGURATION_CMD:
                /* HOST-ZW: */
                powerManagementWakeUpOnRFMode = PM_WAKEUP_UNDEFINED;
                for (i = 0; i < PM_WAKEUP_MAX_BYTES; i++)
                {
                  powerManagementWakeUpOnRF[i].bValue = 0;
                  powerManagementWakeUpOnRF[i].bMask = PM_MASK_DONTCARE;
                }
                /* Valid WakeUp Match Mode */
                if (((FRAME_LENGTH_MIN + 3) <= serFrameLen)
                    && (PM_WAKEUP_MODE_MAX >= serFrameDataPtr[1]))
                {
                  /* WakeUp Match Mode */
                  powerManagementWakeUpOnRFMode = serFrameDataPtr[1];
                  /* Number of match bytes */
                  i = serFrameDataPtr[2];
                  /* Check if valid */
                  if ((0 < i)
                      && (PM_WAKEUP_MAX_BYTES >= i)
                      && ((FRAME_LENGTH_MIN + 3 + (i << 1)) <= serFrameLen))
                  {
                    powerManagementWakeUpOnRFCount = i;
                    while (i--)
                    {
                      powerManagementWakeUpOnRF[i].bValue = serFrameDataPtr[3 + i];
                      powerManagementWakeUpOnRF[i].bMask = serFrameDataPtr[3 + powerManagementWakeUpOnRFCount + i];
                    }
                  }
                  /* If powerManagementWakeUpOnRFCount is ZERO all frames will wakeup HOST */
                }
                set_state(stateIdle);
                break;

              case PM_POWERUP_TIMER_CONFIGURATION_CMD:
                /* HOST-ZW: */
                powerManagementWakeUpOnTimer = PM_TIMER_UNDEFINED;
                powerManagementWakeUpOnTimerCount = 0;
                if ((PM_TIMER_MODE_MAX >= serFrameDataPtr[1])
                    && (serFrameLen >= (FRAME_LENGTH_MIN + 4)))
                {
                  powerManagementWakeUpOnTimer = serFrameDataPtr[1];
                  WORD_SET_HIGH_LOW_BYTES(powerManagementWakeUpOnTimerCount, serFrameDataPtr[2], serFrameDataPtr[3]);
                }
                set_state(stateIdle);
                break;

              case PM_POWERUP_EXTERNAL_CONFIGURATION_CMD:
                /* HOST-ZW: */
                if (-1 != wakeUpOnExternalTimerHandle)
                {
                  ZW_TIMER_CANCEL(wakeUpOnExternalTimerHandle);
                  wakeUpOnExternalTimerHandle = -1;
                }
                wakeUpOnExternal = 0;
                powerManagementWakeUpOnExternal.bPin = PM_PHYSICAL_PIN_UNDEFINED;
                powerManagementWakeUpOnExternal.bEnableLevel = 0;
                if (serFrameLen >= (FRAME_LENGTH_MIN + 3))
                {
                  if ((PM_PHYSICAL_PIN_MAX >= serFrameDataPtr[1])
                      && (0x07 >= (serFrameDataPtr[1] & 0x0F)))
                  {
                    powerManagementWakeUpOnExternal.bPin = serFrameDataPtr[1] + PM_PHYSICAL_PIN_OFFSET;
                    powerManagementWakeUpOnExternal.bEnableLevel = (0 != serFrameDataPtr[2]) ? 1 : 0;
                  }
                }
                set_state(stateIdle);
                break;

              case PM_SET_POWER_MODE_CMD:
                /* HOST-ZW: */
                /* Reset Power Management state to IDLE */
                powerManagementState = POWER_MODE_IDLE;
                /* Reset all Power down IO Pins to UNDEFINED. */
                for (i = 0; i < PM_IO_PIN_MAX; i++)
                {
                  powerManagementPowerMode[i].bPin = PM_PHYSICAL_PIN_UNDEFINED;
                }
                /* Check if Frame contains valid data for command */
                if ((0 < serFrameDataPtr[1])
                    && (PM_IO_PIN_MAX >= serFrameDataPtr[1])
                    && ((FRAME_LENGTH_MIN + 2 + (serFrameDataPtr[1] << 1)) <= serFrameLen))
                {
                  /* Number of Power down IO Pins */
                  i = serFrameDataPtr[1];
                  /* Power Down IO Pin configuration */
                  while (i--)
                  {
                    powerManagementPowerMode[i].bPin = serFrameDataPtr[2 + (i << 1)] + PM_PHYSICAL_PIN_OFFSET;
                    powerManagementPowerMode[i].bEnableLevel = (0 != serFrameDataPtr[2 + 1 + (i << 1)]) ? 1 : 0;
                  }
                  /* We should now initiate the Powerdown of HOST */
                  powerManagementState = POWER_MODE_POWERDOWN_TRANSITION;
                  /* Serial Link is now detached */
                  serialLinkState = SERIAL_LINK_DETACHED;
                  /* Purge Callback queue */
                  PurgeCallbackQueue();
                  PurgeCommandQueue();
                }
                set_state(stateIdle);
                break;

              case PM_GET_STATUS:
                /* HOST->ZW:*/
                /* ZW-HOST:*/
                retVal = powerManagementWakeUpReason;
                powerManagementWakeUpReason = PM_WAKEUP_REASON_NONE;
                DoRespond();
                break;

              default:
                set_state(stateIdle);
                break;
            }
          }
          else
          {
            set_state(stateIdle);
          }
          break;
#endif

#if SUPPORT_ZW_RF_POWER_LEVEL_REDISCOVERY_SET
        case FUNC_ID_ZW_RF_POWER_LEVEL_REDISCOVERY_SET:
          /* HOST->ZW: powerLevel */
          ZW_RF_POWER_LEVEL_REDISCOVERY_SET(serFrameDataPtr[0]);
          state = stateIdle;
          break;
#endif

#if SUPPORT_SERIAL_API_READY
        case FUNC_ID_SERIAL_API_READY:
          /* HOST->ZW: [serialLinkState] */
          /* serialLinkState is OPTIONAL, if not present then the READY command, means "READY" */
          if ((FRAME_LENGTH_MIN < serFrameLen) && (SERIAL_LINK_DETACHED == serFrameDataPtr[0]))
          {
            /* HOST want Serial link to be shutdown - SerialAPI will not transmit anything */
            /* via serial link as long as HOST do not reestablish the Serial Link by */
            /* transmitting a valid SerialAPI frame. */
            serialLinkState = SERIAL_LINK_DETACHED;
            /* Purge Callback queue */
            PurgeCallbackQueue();
            PurgeCommandQueue();
          }
          else
          {
            /* Missing serialLinkState parameter or Every other serialLinkState value means */
            /* HOST Ready for Serial Communication */
            serialLinkState = SERIAL_LINK_CONNECTED;
          }
          set_state(stateIdle);
          break;
#endif

#if SUPPORT_ZW_RF_POWER_LEVEL_SET
        case FUNC_ID_ZW_RF_POWER_LEVEL_SET:
          /* powerLevel */
          retVal = ZW_RF_POWERLEVEL_SET(serFrameDataPtr[0]);
          DoRespond();
          break;
#endif

#if SUPPORT_ZW_RF_POWER_LEVEL_GET
        case FUNC_ID_ZW_RF_POWER_LEVEL_GET:
          /* return powerlevel */
          retVal = ZW_RF_POWERLEVEL_GET();
          DoRespond();
          break;
#endif

#if SUPPORT_ZW_TYPE_LIBRARY
        case FUNC_ID_ZW_TYPE_LIBRARY:
          retVal = ZW_TYPE_LIBRARY();
          DoRespond();
          break;
#endif

#if SUPPORT_ZW_SET_EXT_INT_LEVEL
        case FUNC_ID_ZW_SET_EXT_INT_LEVEL:
          ZW_SET_EXT_INT_LEVEL(serFrameDataPtr[0], serFrameDataPtr[1]);
          set_state(stateIdle);
          break;
#endif

#if SUPPORT_PWR_SETSTOPMODE
        case FUNC_ID_PWR_SETSTOPMODE:
          ZW_PWR_SET_STOP_MODE;
          set_state(stateIdle);
          break;
#endif

#if SUPPORT_PWR_CLK_PD
        case FUNC_ID_PWR_CLK_PD:
          ZW_PWR_CLK_POWERDOWN(serFrameDataPtr[0]);
          set_state(stateIdle);
          break;
#endif

#if SUPPORT_PWR_CLK_PUP
        case FUNC_ID_PWR_CLK_PUP:
          ZW_PWR_CLK_POWERUP(serFrameDataPtr[0]);
          set_state(stateIdle);
          break;
#endif

#if SUPPORT_PWR_SELECT_CLK
        case FUNC_ID_PWR_SELECT_CLK:
          ZW_PWR_SELECT_CLK(serFrameDataPtr[0]);
          set_state(stateIdle);
          break;
#endif

#if SUPPORT_ZW_SET_WUT_TIMEOUT
        case FUNC_ID_ZW_SET_WUT_TIMEOUT:
          ZW_SET_WUT_TIMEOUT(serFrameDataPtr[0]);
          set_state(stateIdle);
          break;
#endif

#if SUPPORT_ZW_IS_WUT_KICKED
        case FUNC_ID_ZW_IS_WUT_KICKED:
          retVal = ZW_IS_WUT_KICKED();
          DoRespond();
          break;
#endif

#if SUPPORT_ZW_GET_PROTOCOL_STATUS
        case FUNC_ID_ZW_GET_PROTOCOL_STATUS:
          retVal = ZW_GET_PROTOCOL_STATUS();
          DoRespond();
          break;
#endif

#if SUPPORT_ZW_WATCHDOG_ENABLE
        case FUNC_ID_ZW_WATCHDOG_ENABLE:
          ZW_WATCHDOG_ENABLE;
          set_state(stateIdle);
          break;
#endif

#if SUPPORT_ZW_WATCHDOG_DISABLE
        case FUNC_ID_ZW_WATCHDOG_DISABLE:
          ZW_WATCHDOG_DISABLE;
          set_state(stateIdle);
          break;
#endif

#if SUPPORT_ZW_WATCHDOG_KICK
        case FUNC_ID_ZW_WATCHDOG_KICK:
          ZW_WATCHDOG_KICK;
          set_state(stateIdle);
          break;
#endif

#if SUPPORT_ZW_WATCHDOG_START
        case FUNC_ID_ZW_WATCHDOG_START:
          ZW_WATCHDOG_ENABLE;
          bWatchdogStarted = TRUE;
          ZW_MEM_PUT_BYTE((WORD)&EEOFFSET_WATCHDOG_STARTED_far, bWatchdogStarted);
          set_state(stateIdle);
          break;
#endif

#if SUPPORT_ZW_WATCHDOG_STOP
        case FUNC_ID_ZW_WATCHDOG_STOP:
          ZW_WATCHDOG_DISABLE;
          bWatchdogStarted = FALSE;
          ZW_MEM_PUT_BYTE((WORD)&EEOFFSET_WATCHDOG_STARTED_far, bWatchdogStarted);
          set_state(stateIdle);
          break;
#endif

#if SUPPORT_ZW_NUNIT
        case FUNC_ID_ZW_NUNIT_CMD:
          state = stateIdle;
          break;

        case FUNC_ID_ZW_NUNIT_INIT:
          ZW_UnitTestInit(ZCB_NUnitCmd, ZCB_GetCallbackCnt);
          state = stateIdle;
          break;

        case FUNC_ID_ZW_NUNIT_LIST:
          ZW_UnitTestList();
          state = stateIdle;
          break;

        case FUNC_ID_ZW_NUNIT_RUN:
          ZW_UnitTestTcRun(serFrameDataPtr[0]);
          state = stateIdle;
          break;

        case FUNC_ID_ZW_NUNIT_END:
          ZW_UnitTestEnd();
          state = stateIdle;
          break;
#endif /* SUPPORT_ZW_SEND_DATA */


#if SUPPORT_ZW_PORT_STATUS
        case FUNC_ID_IO_PORT_STATUS:
          /*Enable Port Monitor. See function EventHandler()*/
          if(TRUE == serFrameDataPtr[0])
          {
            ZW_PortMonitorInit(&EventHandler);
          }
          else /*Disable Port Monitor*/
          {
            ZW_PortMonitorInit(NULL);
          }
          state = stateIdle;
          break;

        case FUNC_ID_IO_PORT:
          retVal = TRUE;
          switch(serFrameDataPtr[0])
          {
            case ID_PORT_PIN_SET:
              ZW_PortPinSet(PM_PHYSICAL_PIN_OFFSET + serFrameDataPtr[1], serFrameDataPtr[2]);
              break;
            case ID_PORT_PIN_GET:
              retVal = ZW_PortPinGet(PM_PHYSICAL_PIN_OFFSET + serFrameDataPtr[1]);
              break;
            case ID_PORT_GET:
              retVal = ZW_PortGet(PM_PHYSICAL_PIN_OFFSET + serFrameDataPtr[1]);
              break;
            case ID_PORT_PIN_IN:
              ZW_PortMonitorPinIn(PM_PHYSICAL_PIN_OFFSET + serFrameDataPtr[1]);
              //ZW_PortPinIn(serFrameDataPtr[1]);
              break;
            case ID_PORT_PIN_OUT:
              ZW_PortMonitorPinOut(PM_PHYSICAL_PIN_OFFSET + serFrameDataPtr[1]);
              //ZW_PortPinOut(serFrameDataPtr[1]);
              break;
            default: break;

          }
          BYTE_IN_AR(compl_workbuf, 0) = serFrameDataPtr[0];
          BYTE_IN_AR(compl_workbuf, 1) = retVal;
          DoRespond_workbuf(2);
          break;

#endif /*SUPPORT_ZW_PORT_STATUS*/



#if SUPPORT_ZW_SET_PROMISCUOUS_MODE
        case FUNC_ID_ZW_SET_PROMISCUOUS_MODE:
          /* HOST->ZW: promiscuousMode */
          ZW_SET_PROMISCUOUS_MODE(serFrameDataPtr[0]);
          set_state(stateIdle);
        break;
#endif

#ifdef ZW_CONTROLLER_SINGLE
#if SUPPORT_SERIAL_API_TEST
        case FUNC_ID_SERIAL_API_TEST:
          /* HOST->ZW: testCmd | testDelay(MSB) | testDelay(LSB) | testPayloadLen | */
          /*           testCount(MSB) | testCount(LSB) | testTxOptions | nodemasklen | testnodemask[] | funcID */
          /* testCmd = 0x01 - use sendData */
          /* testCmd = 0x02 - use sendDataMeta */
          /* testCmd = 0x03 - use sendData with Basic Set toggle ON/OFF between rounds */
          /* testCmd = 0x04 - use sendDataMeta with Basic Set toggle ON/OFF between rounds */
          /* testCmd = 0x05 - use sendData with one group result frame (serial) at every round end */
          /* testCmd = 0x06 - use sendDataMeta with one group result frame (serial) at every round end */
          /* testCmd = 0x07 - use sendData with Basic Set toggle ON/OFF and one group result frame (serial) between rounds */
          /* testCmd = 0x08 - use sendDataMeta with Basic Set toggle ON/OFF and one group result frame (serial) between rounds */
          /* ZW->HOST: RES | testStarted */
          retVal = FALSE;
          testCmd = serFrameDataPtr[0];
          if (testCmd && (testState != POWERLEVEL_TEST_NODE_REPORT_ZW_TEST_INPROGRESS))
          {
            WORD_SET_HIGH_LOW_BYTES(testDelay, serFrameDataPtr[1], serFrameDataPtr[2]);
            testPayloadLen = serFrameDataPtr[3];
            WORD_SET_HIGH_LOW_BYTES(testCount, serFrameDataPtr[4], serFrameDataPtr[5]);
            testTxOptions = serFrameDataPtr[6];
            testnodemasklen = serFrameDataPtr[7];
            /*  - Boundary Check */
            if (testnodemasklen > MAX_NODEMASK_LENGTH)
            {
              testnodemasklen = MAX_NODEMASK_LENGTH;
            }
            ZW_NODE_MASK_CLEAR(testnodemask, MAX_NODEMASK_LENGTH);
            for (i = 0; i < testnodemasklen; i++)
            {
              BYTE_IN_AR(testnodemask, i) = serFrameDataPtr[i + 8];
            }
            funcID_ComplHandler_Serial_API_Test = serFrameDataPtr[serFrameDataPtr[7] + 8];
            if (testCount != 0)
            {
              testFailedCount = 0;
              testSuccessCount = 0;
              testState = POWERLEVEL_TEST_NODE_REPORT_ZW_TEST_INPROGRESS;
              TestStartRound();
              retVal = TRUE;
            }
            else
            {
              testState = POWERLEVEL_TEST_NODE_REPORT_ZW_TEST_FAILED;
              SendTestReport(0);
              break;
            }
          }
          else
          {
            /* STOP test ??? */
            if (!testCmd)
            {
              testState = POWERLEVEL_TEST_NODE_REPORT_ZW_TEST_FAILED;
              SendTestReport(0);
              retVal = TRUE;
            }
          }
          DoRespond();
          break;
#endif
#endif

#if SUPPORT_SERIAL_API_EXT
        case FUNC_ID_SERIAL_API_EXT:
          /* HOST->ZW: mode | data */
          if (serFrameLen > FRAME_LENGTH_MIN)
          {
            switch (serFrameDataPtr[0])
            {
              case 0:
                {
                  IBYTE *startIAdr = 0;
                  XBYTE IAdrIndex = 0;
                  XBYTE x = 0x40;
                  if (serFrameLen > FRAME_LENGTH_MIN + 1)
                  {
                    IAdrIndex = serFrameDataPtr[1];
                    if (serFrameLen > FRAME_LENGTH_MIN + 2)
                    {
                      x = serFrameDataPtr[2];
                    }
                  }
                  if (x > BUF_SIZE_TX - 1)
                  {
                    x = BUF_SIZE_TX - 1;
                  }
                  for (in0 = 0; in0 < x; in0++)
                  {
                    BYTE_IN_AR(compl_workbuf, in0 + 1) = startIAdr[(BYTE)(IAdrIndex + in0)];
                  }
                  BYTE_IN_AR(compl_workbuf, 0) = x;
                  DoRespond_workbuf(x + 1);
                }
                break;

              case 1:
                {
                  XBYTE *startXAdr = 0;
                  IWORD XAdrIndex = 0;
                  i = 0x40;
                  if (serFrameLen > FRAME_LENGTH_MIN + 2)
                  {
                    XAdrIndex = ((WORD)serFrameDataPtr[1] << 8) + serFrameDataPtr[2];
                    if (serFrameLen > FRAME_LENGTH_MIN + 3)
                    {
                      i = serFrameDataPtr[3];
                    }
                  }
                  if (i > BUF_SIZE_TX - 1)
                  {
                    i = BUF_SIZE_TX - 1;
                  }
                  for (in1 = 0; in1 < i; in1++)
                  {
                    BYTE_IN_AR(compl_workbuf, in1 + 1) = startXAdr[(WORD)(XAdrIndex + in1)];
                  }
                  BYTE_IN_AR(compl_workbuf, 0) = i;
                  DoRespond_workbuf(i + 1);
                }
                break;

              default:
                retVal = 0;
                DoRespond();
                break;
            }
          }
          break;
#endif

#if SUPPORT_ZW_SET_SLEEP_MODE
        case FUNC_ID_ZW_SET_SLEEP_MODE:
          /* HOST->ZW: mode | intEnable */
          /* ZW->HOST: retVal */
          state = stateIdle;
          retVal = ZW_SET_SLEEP_MODE(serFrameDataPtr[0], serFrameDataPtr[1]);
#ifdef ZW_SMARTSTART_ENABLED
          applPowerModeExtIntEnable = serFrameDataPtr[1];
          SaveApplicationSettings();
#endif
          DoRespond();
          if (retVal)
          {
            /* Busy loop a bit to allow serial communication to progress */
            while (ConTxActive())
            {
              ZW_POLL();
            }
          }
          break;
#endif

#if SUPPORT_ZW_GET_RANDOM
        case FUNC_ID_ZW_GET_RANDOM:
          /* HOST->ZW: noRandomBytes - Optional if not present or equal ZERO then  */
          /*                           2 random bytes are returned.                */
          /*                           Range 1..32 random bytes are supported      */
          /* ZW->HOST: RES | randomGenerationSuccess | noRandomBytesGenerated | noRandomGenerated[] */
          noRndBytes = serFrameDataPtr[0];
          if ((serFrameLen > FRAME_LENGTH_MIN) && (noRndBytes != 0))
          {
            if (noRndBytes > 32)
            {
              noRndBytes = 32;
            }
          }
          else
          {
            noRndBytes = 2;
          }
          if (ZW_SetRFReceiveMode(FALSE))
          {
            i = 2;
            do
            {
              if (!(ZW_GET_RANDOM_WORD((BYTE_P)&rndWord)))
              {
                break;
              }
              BYTE_IN_AR(compl_workbuf, i++) = BYTE_GET_HIGH_BYTE_IN_WORD(rndWord);
              BYTE_IN_AR(compl_workbuf, i++) = BYTE_GET_LOW_BYTE_IN_WORD(rndWord);
            } while (--noRndBytes && --noRndBytes);
          }
          i -= 2;
          /* Number of random bytes returned */
          if (noRndBytes == 0xff)
          {
            i--;
            noRndBytes++;
          }
          if (noRndBytes)
          {
            /* Could not generate the needed number of random bytes */
            i = 0;
          }
          BYTE_IN_AR(compl_workbuf, 0) = (!noRndBytes);
          BYTE_IN_AR(compl_workbuf, 1) = i;
          /* Check if RF should be set in Receive mode. */
          if (applNodeInfo_deviceOptionsMask & APPLICATION_NODEINFO_LISTENING)
          {
            /* We are a listening device so lets return to RF Receive */
            ZW_SetRFReceiveMode(TRUE);
          }
          else
          {
            /* Set RF in powerdown mode */
            ZW_SetRFReceiveMode(FALSE);
          }
          DoRespond_workbuf((i ? 2 : 1) + i);
          break;
#endif

#if SUPPORT_ZW_SET_ROUTING_MAX
        case FUNC_ID_ZW_SET_ROUTING_MAX:
        case FUNC_ID_ZW_SET_ROUTING_MAX_6_00:
          /* HOST->ZW: maxRouteTries */
          /* ZW->HOST: RES | isRoutingMAXSet */
          ZW_SetRoutingMAX(serFrameDataPtr[0]);
          retVal = TRUE;
          DoRespond();
          break;
#endif

#if SUPPORT_ZW_IS_PRIMARY_CTRL
        case FUNC_ID_ZW_IS_PRIMARY_CTRL:
          /* HOST->ZW: no params defined */
          /* ZW->HOST: RES | isPrimary */
          retVal = ZW_PRIMARYCTRL();
          DoRespond();
          break;
#endif

#if SUPPORT_ZW_AES_ECB
        case FUNC_ID_ZW_AES_ECB:
          /* HOST->ZW: key[] (16 bytes) | inputDat[] (16 bytes) */
          /* ZW->HOST: RES | outdata[] (16 bytes) */
          ZW_AES_ECB(&serFrameDataPtr[0], &serFrameDataPtr[16], compl_workbuf);
          DoRespond_workbuf(16);
          break;
#endif

#if SUPPORT_ZW_AUTO_PROGRAMMING
      case FUNC_ID_AUTO_PROGRAMMING:
        ZW_FLASH_auto_prog_set();
        break;
#endif

#if SUPPORT_ZW_GET_BACKGROUND_RSSI
        case FUNC_ID_ZW_GET_BACKGROUND_RSSI:
          /* HOST->ZW: (no arguments) */
          /* ZW->HOST: RES | RSSI Ch0 | RSSI Ch1 | RSSI Ch2 (3CH systems only) */
          ZW_GetBackgroundRSSI((RSSI_LEVELS*)compl_workbuf);
          /* Respond with the old timeout settings */
          DoRespond_workbuf(ZW_3CH_Build ? sizeof(RSSI_LEVELS_3CH) : sizeof(RSSI_LEVELS_2CH));
          break;
#endif

#if SUPPORT_ZW_SET_LISTEN_BEFORE_TALK_THRESHOLD
        case FUNC_ID_ZW_SET_LISTEN_BEFORE_TALK_THRESHOLD:
          /* HOST->ZW: bChannel | bThreshold */
          /* ZW->HOST: RES | TRUE */
          ZW_SetListenBeforeTalkThreshold(serFrameDataPtr[0], serFrameDataPtr[1]);
          retVal = TRUE;
          DoRespond();
          break;
#endif

#if SUPPORT_ZW_NETWORK_MANAGEMENT_SET_MAX_INCLUSION_REQUEST_INTERVALS
        case FUNC_ID_ZW_NETWORK_MANAGEMENT_SET_MAX_INCLUSION_REQUEST_INTERVALS:
          /* HOST->ZW: bInclRequestIntervals */
          /* ZW->HOST: RES | TRUE */
          ZW_NetworkManagementSetMaxInclusionRequestIntervals(serFrameDataPtr[0]);
          retVal = TRUE;
          DoRespond();
          break;
#endif

        default :
          /* TODO - send a "Not Supported" respond frame */
          /* UNKNOWN - just drop it */
          set_state(stateIdle);
          break;
      }
    }
    break;

    case stateTxSerial :
    {
      /* Wait for ACK on send respond. Retransmit as needed */
#ifdef ENABLE_LEDS
      LedOff(1);
      LedOff(2);
      LedOn(3);
#endif
      if ((conVal = ConUpdate(FALSE)) == conFrameSent)
      {
        /* One more RES transmitted succesfully */
        retry = 0;
        set_state(stateIdle);
      }
      else if (conVal == conTxTimeout)
      {
        /* Either a NAK has been received or we timed out waiting for ACK */
        if (retry++ < MAX_SERIAL_RETRY)
        {
          ConTxFrame(0, REQUEST, NULL, 0);  /* Retry... */
        }
        else
        {
          /* Drop RES as HOST could not be reached */
          retry = 0;
          set_state(stateIdle);
        }
      }
      /* All other states are ignored, as for now the only thing we are looking for is ACK/NAK! */
    }
    break;

    case stateCallbackTxSerial:
    {
      /* Wait for ack on unsolicited event (callback etc.) */
      /* Retransmit as needed. Remove frame from callbackQueue when done */
#ifdef ENABLE_LEDS
      LedOff(1);
      LedOn(2);
      LedOn(3);
#endif
      if ((conVal = ConUpdate(FALSE)) == conFrameSent)
      {
        /* One more REQ transmitted succesfully */
        PopCallBackQueue();
      }
      else if (conVal == conTxTimeout)
      {
        /* Either a NAK has been received or we timed out waiting for ACK */
        if (retry++ < MAX_SERIAL_RETRY)
        {
          ConTxFrame(0, REQUEST, NULL, 0);  /* Retry... */
        }
        else
        {
          /* Drop REQ as HOST could not be reached */
          PopCallBackQueue();
        }
      }
      /* All other states are ignored, as for now the only thing we are looking for is ACK/NAK! */
    }
    break;

    case stateCommandTxSerial:
    {
      /* Wait for ack on unsolicited ApplicationCommandHandler event */
      /* Retransmit as needed. Remove frame from comamndQueue when done */
#ifdef ENABLE_LEDS
      LedOff(1);
      LedOn(2);
      LedOn(3);
#endif
      if ((conVal = ConUpdate(FALSE)) == conFrameSent)
      {
        /* One more REQ transmitted succesfully */
        PopCommandQueue();
      }
      else if (conVal == conTxTimeout)
      {
        /* Either a NAK has been received or we timed out waiting for ACK */
        if (retry++ < MAX_SERIAL_RETRY)
        {
          ConTxFrame(0, REQUEST, NULL, 0);  /* Retry... */
        }
        else
        {
          /* Drop REQ as HOST could not be reached */
          PopCommandQueue();
        }
      }
      /* All other states are ignored, as for now the only thing we are looking for is ACK/NAK! */
    }
    break;

    default:
      set_state(stateIdle);
      break;
  }
#ifdef ZW_SMARTSTART_ENABLED
  /* Return E_APPLICATION_STATE_READY_FOR_POWERDOWN for system power down */
  return E_APPLICATION_STATE_ACTIVE;
#endif
}


void
PopCallBackQueue(void)
{
  if (callbackQueue.requestCnt)
  {
    callbackQueue.requestCnt--;
    if (++callbackQueue.requestOut >= MAX_CALLBACK_QUEUE)
    {
      callbackQueue.requestOut = 0;
    }
  }
  else
  {
    callbackQueue.requestOut = callbackQueue.requestIn;
  }
  retry = 0;
  set_state(stateIdle);
}


void
PopCommandQueue(void)
{
  if (commandQueue.requestCnt)
  {
    commandQueue.requestCnt--;
    if (++commandQueue.requestOut >= MAX_CALLBACK_QUEUE)
    {
      commandQueue.requestOut = 0;
    }
  }
  else
  {
    commandQueue.requestOut = commandQueue.requestIn;
  }
  retry = 0;
  set_state(stateIdle);
}


#if SUPPORT_ZW_NUNIT
code const BYTE (code * ZCB_GetCallbackCnt_p)(void) = &ZCB_GetCallbackCnt;

BYTE
ZCB_GetCallbackCnt(void)
{
  return callbackQueue.requestCnt;
}
#endif


/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/
void
ApplicationTestPoll(void)
{
#ifdef APPL_PROD_TEST
  /* If ApplicationInitHW returns FALSE this function will be called in place of
     ApplicationPoll */
  if (testRun)
    return;
  testRun = TRUE; /*it is only zero after reset*/
  switch (bProdtestState)
  {
    case PRODTEST_NR_1:
      /*Send a carrier on channel 0*/
      ZW_SendConst(1, 0, ZW_RF_TEST_SIGNAL_CARRIER);
    break;
    case PRODTEST_NR_2:
      /*Send a modulated carrier on channel 0*/
      ZW_SendConst(1, 0, ZW_RF_TEST_SIGNAL_CARRIER_MODULATED);
    break;
    case PRODTEST_NR_3:
      /*Send a carrier on channel 1*/
      ZW_SendConst(1, 1, ZW_RF_TEST_SIGNAL_CARRIER);
    break;
    case PRODTEST_NR_4:
      /*Send a modulated carrier on channel 1*/
      ZW_SendConst(1, 1, ZW_RF_TEST_SIGNAL_CARRIER_MODULATED);
    break;
    case PRODTEST_NR_5:
      /*Send a carrier on channel 2*/
      ZW_SendConst(1, 2, ZW_RF_TEST_SIGNAL_CARRIER);
    break;
    case PRODTEST_NR_6:
      /*Send a modulated carrier on channel 2*/
      ZW_SendConst(1, 2, ZW_RF_TEST_SIGNAL_CARRIER_MODULATED);
    break;
    case PRODTEST_NR_7:
      /*Set the chip in rx state*/
      ZW_SET_RX_MODE(TRUE);
    break;
  }
#endif
}


/*==============================   ApplicationInitHW   ======================
**    Init UART and setup port pins for LEDs
**
**--------------------------------------------------------------------------*/
BYTE                   /*RET TRUE - If normal operation, FALSE - if production test*/
ApplicationInitHW(
  BYTE bWakeupReason)  /* IN Reason for wakeup - ZW_WAKEUP_RESET (RESET)   */
                       /*                      - ZW_WAKEUP_WUT (WUT)       */
                       /*                      - ZW_WAKEUP_SENSOR (SENSOR) */
{
#ifdef APPL_PROD_TEST
    /* Enable IO pins */
    SET_PRODUCTIONTEST_PIN;
    ZW_IOS_enable(TRUE);
    for (testRun = 0; testRun < 100; testRun++) ;  /* Short delay in case pin is floating... */
    testRun = 0; /*set back the var to zero since it used as a flag*/
    if (IN_PRODUCTIONTEST) /* Anyone forcing it into production test mode ? */
    {/*bProdTestState is only initialised on ASIC power on*/
      if (bWakeupReason == ZW_WAKEUP_POR)
      {
        bProdtestState = PRODTEST_NR_1; /*Start test 1*/
      }
      else
      {/*if all test is been perform go to test 1*/
        if (++bProdtestState > PRODTEST_NR_7)
          bProdtestState = PRODTEST_NR_1;
      }
      return(FALSE);  /* Enter production test mode */
    }
#endif /* APPL_PROD_TEST */
#ifdef ZW_ISD51_DEBUG
  ISD_UART_init();
#endif
/* Productiontest pin not connected to a LED - hopefully */
#ifdef ENABLE_LEDS
  LedControlInit();

  LedOff(3);
#endif /* ENABLE_LEDS */
#ifdef USBDETECT
  /* Setup USB detect GPIO to OUT and LOW */
  PIN_LOW(USBDetect);
  PIN_OUT(USBDetect);
#endif
#ifdef UZB
  PIN_HIGH(LEDx);
  PIN_OUT(LEDx);
#endif

#if SUPPORT_SERIAL_API_READY
  if ((ZW_WAKEUP_WUT == bWakeupReason)
      || (ZW_WAKEUP_SENSOR == bWakeupReason)
      || (ZW_WAKEUP_EXT_INT == bWakeupReason)
      || (ZW_WAKEUP_USB_SUSPEND == bWakeupReason))
  {
    /* We have been waken from sleep by WUT, Beam or EXT_INT event - we must assume we are connected. */
    serialLinkState = SERIAL_LINK_CONNECTED;
  }
  else
  {
    /* We have been waken either by ZW_WAKEUP_RESET or ZW_WAKEUP_POR. Initially we are DETACHED */
    serialLinkState = SERIAL_LINK_CONNECTED;
  }
#endif
  /* applWakeupReason now contains lastest System Wakeup reason */
  applWakeupReason = bWakeupReason;
#ifdef ZW_SECURITY_PROTOCOL
  Transport_OnApplicationInitHW(bWakeupReason);
#endif
  return(TRUE); /*Return FALSE to enter production test mode*/
}


/*============================   CheckPowerlevel   =========================
**    Check if a powerlevel is in the correct range
**
**    returns:  0  - Powerlevel is not correct
**              >0 - Powerlevel
**--------------------------------------------------------------------------*/
BYTE
CheckPowerlevel(
  BYTE bPower)
{
  if ((bPower > 0x3F) || (bPower < 0x03))
  {
    return 0;
  }
  return bPower;
}


/*==============================   ApplicationInitSW   ======================
**    Initialization of the Application Software
**
**--------------------------------------------------------------------------*/
E_SYSTEM_TYPE              /*RET Application Power mode */
ApplicationInitSW(
  ZW_NVM_STATUS nvmStatus)
{
#ifdef ZW_ENABLE_RTC
  ZW_CLOCK_INIT();
  ZW_RTC_INIT();
#endif
  ConInit(1152);   /* 115200 baud */

#if SUPPORT_SERIAL_API_POWER_MANAGEMENT
  /* Reset Power Management Mode */
  powerManagementState = POWER_MODE_IDLE;
  powerManagementWakeUpReason = PM_WAKEUP_REASON_NONE;

  /* Reset Power Management Powered Up IO Pin */
  powerManagementPoweredUp.bPin = PM_PHYSICAL_PIN_UNDEFINED;
  poweredUpTimerHandle = -1;
  poweredUpTransition = FALSE;
  poweredUp = FALSE;

  /* Reset Power Management Wakeup HOST IO Pin configuration */
  for (i = 0; i < PM_IO_PIN_MAX; i++)
  {
    /* Set Power Management pins to undefined */
    powerManagementWakeUpPinConf[i].bPin = PM_PHYSICAL_PIN_UNDEFINED;
    /* Set Power Management pin Enabel Level to 0 */
    powerManagementWakeUpPinConf[i].bEnableLevel = 0;
  }

  /* Reset Power Management Powerdown IO Pin configuration */
  for (i = 0; i < PM_IO_PIN_MAX; i++)
  {
    powerManagementPowerMode[i].bPin = PM_PHYSICAL_PIN_UNDEFINED;
    powerManagementPowerMode[i].bEnableLevel = 0;
  }

  /* Reset Power Management Wakeup on RF Mode */
  powerManagementWakeUpOnRFMode = PM_WAKEUP_UNDEFINED;
  /* Reset Power Management Wakeup on RF Frame mask configuration */
  for (i = 0; i < PM_WAKEUP_MAX_BYTES; i++)
  {
    powerManagementWakeUpOnRF[i].bValue = PM_WAKEUP_UNDEFINED;
    powerManagementWakeUpOnRF[i].bMask = PM_MASK_DONTCARE;
  }
  powerManagementWakeUpOnRFCount = 0;
  wakeUpOnRF = FALSE;

  /* Reset Power Management Timer WakeUp configuration */
  powerManagementWakeUpOnTimer = PM_TIMER_UNDEFINED;
  powerManagementWakeUpOnTimerCount = 0;
  wakeUpOnTimerHandle = -1;
  wakeUpOnTimerCount = 0;
  wakeUpOnTimerRes = 0;
  wakeUpOnTimer = FALSE;

  /* Reset Power Management External PowerUp IO Pin configuration */
  powerManagementWakeUpOnExternal.bPin = PM_PHYSICAL_PIN_UNDEFINED;
  powerManagementWakeUpOnExternal.bEnableLevel = 0;
  wakeUpOnExternalTimerHandle = -1;
  wakeUpOnExternalTransition = 0;
  wakeUpOnExternal = FALSE;
#endif

#ifdef ZW_ENABLE_RTC
  /* Init RTC timer callback functions array */
  for (i = 0; i < RTC_TIMER_MAX; i++)
  {
    switch(i)
    {
      case 0: rtcArray[i] = RTCHandler1; break;
      case 1: rtcArray[i] = RTCHandler2; break;
      case 2: rtcArray[i] = RTCHandler3; break;
      case 3: rtcArray[i] = RTCHandler4; break;
      case 4: rtcArray[i] = RTCHandler5; break;
      case 5: rtcArray[i] = RTCHandler6; break;
      case 6: rtcArray[i] = RTCHandler7; break;
      case 7: rtcArray[i] = RTCHandler8; break;
      case 8: rtcArray[i] = RTCHandler9; break;
      case 9: rtcArray[i] = RTCHandler10; break;
    }
  }
#endif
//  ZW_DEBUG_INIT(ZW_DEBUG_BAUD_RATE);
#ifndef ZW_ISD51_DEBUG
  ZW_DEBUG_INIT(1152);
#endif
  ZW_MEMORY_GET_ID(appHomeId, &appNodeId);
  if (ZW_MEM_GET_BYTE((WORD)&EEOFFSET_MAGIC_far) == MAGIC_VALUE)
  {
#ifdef ZW_SLAVE
    applNodeInfo_deviceOptionsMask = ZW_MEM_GET_BYTE((WORD)&EEOFFSET_LISTENING_far);
    applNodeInfo_nodeType_generic = ZW_MEM_GET_BYTE((WORD)&EEOFFSET_GENERIC_far);
    applNodeInfo_nodeType_specific = ZW_MEM_GET_BYTE((WORD)&EEOFFSET_SPECIFIC_far);
#endif
#ifdef ZW_CONTROLLER
    ZW_GET_NODE_STATE(appNodeId, (NODEINFO *)compl_workbuf);
    /* Update application level nodetype variables */
    applNodeInfo_deviceOptionsMask = ((((NODEINFO *)compl_workbuf)->capability & NODEINFO_LISTENING_SUPPORT) ? APPLICATION_NODEINFO_LISTENING : 0)|
                                      ((((NODEINFO *)compl_workbuf)->security & NODEINFO_OPTIONAL_FUNC_SUPPORT ) ? APPLICATION_NODEINFO_OPTIONAL_FUNCTIONALITY : 0);
    applNodeInfo_nodeType_generic = ((NODEINFO *)compl_workbuf)->nodeType.generic;
    applNodeInfo_nodeType_specific = ((NODEINFO *)compl_workbuf)->nodeType.specific;
#endif
    /* Update command class membership */
    applNodeInfo_parmLength = ZW_MEM_GET_BYTE((WORD)&EEOFFSET_CMDCLASS_LEN_far);
    if (applNodeInfo_parmLength > APPL_NODEPARM_MAX)
    {
      applNodeInfo_parmLength = APPL_NODEPARM_MAX;
    }
    for (i = 0; i < applNodeInfo_parmLength; i++)
    {
      applNodeInfo_nodeParm[i] = ZW_MEM_GET_BYTE((WORD)&EEOFFSET_CMDCLASS_far + i);
    }

#if SUPPORT_ZW_WATCHDOG_START || SUPPORT_ZW_WATCHDOG_STOP

    bWatchdogStarted = ZW_MEM_GET_BYTE((WORD)&EEOFFSET_WATCHDOG_STARTED_far);

#if SUPPORT_SERIAL_API_APPL_NODE_INFORMATION_CMD_CLASSES
    /* Unincluded Supported Command Classes */
    applNodeInfo_unincluded_parmLength = ZW_MEM_GET_BYTE((WORD)&EEOFFSET_CMDCLASS_UNINCLUDED_LEN_far);
    if (applNodeInfo_unincluded_parmLength > APPL_NODEPARM_MAX)
    {
      applNodeInfo_unincluded_parmLength = APPL_NODEPARM_MAX;
    }
    for (i = 0; i < applNodeInfo_unincluded_parmLength; i++)
    {
      applNodeInfo_unincluded_nodeParm[i] = ZW_MEM_GET_BYTE((WORD)&EEOFFSET_CMDCLASS_UNINCLUDED_far + i);
    }
    /* Included Unsecure Supported Command Classes */
    applNodeInfo_included_unsecure_parmLength = ZW_MEM_GET_BYTE((WORD)&EEOFFSET_CMDCLASS_INCLUDED_UNSECURE_LEN_far);
    if (applNodeInfo_included_unsecure_parmLength > APPL_NODEPARM_MAX)
    {
      applNodeInfo_included_unsecure_parmLength = APPL_NODEPARM_MAX;
    }
    for (i = 0; i < applNodeInfo_included_unsecure_parmLength; i++)
    {
      applNodeInfo_included_unsecure_nodeParm[i] = ZW_MEM_GET_BYTE((WORD)&EEOFFSET_CMDCLASS_INCLUDED_UNSECURE_far + i);
    }
    /* Included Secure Supported Command Classes */
    applNodeInfo_included_secure_parmLength = ZW_MEM_GET_BYTE((WORD)&EEOFFSET_CMDCLASS_INCLUDED_SECURE_LEN_far);
    if (applNodeInfo_included_secure_parmLength > APPL_NODEPARM_MAX)
    {
      applNodeInfo_included_secure_parmLength = APPL_NODEPARM_MAX;
    }
    for (i = 0; i < applNodeInfo_included_secure_parmLength; i++)
    {
      applNodeInfo_included_secure_nodeParm[i] = ZW_MEM_GET_BYTE((WORD)&EEOFFSET_CMDCLASS_INCLUDED_SECURE_far + i);
    }
#endif
#ifndef slave_routing
    for (i = 0; i < POWERLEVEL_CHANNELS; i++)
    {
      sPowerlevels.normal[i] = CheckPowerlevel(ZW_MEM_GET_BYTE((WORD)&EEOFFSET_POWERLEVEL_NORMAL_far + i));
      sPowerlevels.low[i] = CheckPowerlevel(ZW_MEM_GET_BYTE((WORD)&EEOFFSET_POWERLEVEL_LOW_far + i));
    }
#ifdef ZW_SMARTSTART_ENABLED
    applPowerModeExtIntEnable = ZW_MEM_GET_BYTE((WORD)&EEOFFSET_MODULE_POWER_MODE_EXTINT_ENABLE_far);
    if (applPowerModeExtIntEnable == 0xFF)
    {
      applPowerModeExtIntEnable = 0;
    }
    applPowerMode = ZW_MEM_GET_BYTE((WORD)&EEOFFSET_MODULE_POWER_MODE_far);
    if (applPowerMode == 0xFF)
    {
      applPowerMode = 0;
    }
    applPowerModeWutTimeOut = ZW_MEM_GET_BYTE((WORD)&EEOFFSET_MODULE_POWER_MODE_WUT_TIMEOUT_far) << 24;
    applPowerModeWutTimeOut += ZW_MEM_GET_BYTE((WORD)&EEOFFSET_MODULE_POWER_MODE_WUT_TIMEOUT_far + 1) << 16;
    applPowerModeWutTimeOut += ZW_MEM_GET_BYTE((WORD)&EEOFFSET_MODULE_POWER_MODE_WUT_TIMEOUT_far + 2) << 8;
    applPowerModeWutTimeOut += ZW_MEM_GET_BYTE((WORD)&EEOFFSET_MODULE_POWER_MODE_WUT_TIMEOUT_far + 3);
    if ((0 == applPowerModeWutTimeOut) || (applPowerModeWutTimeOut = 0xFFFFFFFF))
    {
      applPowerModeWutTimeOut = 360;  /* For now 6 minutes */
    }
    ZW_Power_Management_Init(applPowerModeWutTimeOut, applPowerModeExtIntEnable);

#endif
    /* If powerlevel is set, then call protocol to change powerlevel */
    if (sPowerlevels.normal[0] != 0)
    {
      ZW_SetDefaultPowerLevels( sPowerlevels.normal[0],
                                sPowerlevels.normal[1],
                                sPowerlevels.normal[2],
                                sPowerlevels.low[0],
                                sPowerlevels.low[1],
                                sPowerlevels.low[2]);
    }
#endif

    /* Check if host has started the watchdog, and if it has */
    /* then enable the watchdog again */
    if (bWatchdogStarted)
    {
      ZW_WATCHDOG_ENABLE;
    }
#endif
  }
  else
  {
    /* Probably first time we start after a reset */
    applNodeInfo_parmLength = 1;
#ifdef ZW_CONTROLLER
    /* Controllers are members of the COMMAND_CLASS_CONTROLLER_REPLICATION command class */
    applNodeInfo_nodeParm[0] = COMMAND_CLASS_CONTROLLER_REPLICATION;
#else
    /* Slaves are members of the COMMAND_CLASS_SWITCH_MULTILEVEL command class */
    applNodeInfo_nodeParm[0] = COMMAND_CLASS_SWITCH_MULTILEVEL;
#endif
#if SUPPORT_SERIAL_API_APPL_NODE_INFORMATION_CMD_CLASSES
    applNodeInfo_unincluded_parmLength = 0;
    applNodeInfo_unincluded_nodeParm[0] = 0;
    applNodeInfo_included_unsecure_parmLength = 0;
    applNodeInfo_included_unsecure_nodeParm[0] = 0;
    applNodeInfo_included_secure_parmLength = 0;
    applNodeInfo_included_secure_nodeParm[0] = 0;
#endif
#ifndef slave_routing
    memset((BYTE*)&sPowerlevels, 0, sizeof(sPowerlevels));
#endif
#ifdef ZW_SMARTSTART_ENABLED
    applPowerMode = E_SYSTEM_TYPE_LISTENING;
    applPowerModeWutTimeOut = 720;
#endif
    SaveApplicationSettings();
  }

#if SUPPORT_SERIAL_API_POWER_MANAGEMENT
#ifdef POWER_MANAGEMENT_TEST
  powerManagementWakeUpPinConf[0].bPin = 0x87;
  powerManagementWakeUpPinConf[1].bPin = 0xB7;
  powerManagementWakeUpPinConf[2].bPin = 0x90;
  powerManagementWakeUpPinConf[3].bPin = 0x92;
  powerManagementWakeUpPinConf[0].bEnableLevel = 0;
  powerManagementWakeUpPinConf[1].bEnableLevel = 1;
  powerManagementWakeUpPinConf[2].bEnableLevel = 1;
  powerManagementWakeUpPinConf[3].bEnableLevel = 1;

  powerManagementPowerMode[0].bPin = 0x87;
  powerManagementPowerMode[1].bPin = 0xB7;
  powerManagementPowerMode[2].bPin = 0x90;
  powerManagementPowerMode[3].bPin = 0x92;
  powerManagementPowerMode[0].bEnableLevel = 1;
  powerManagementPowerMode[1].bEnableLevel = 0;
  powerManagementPowerMode[2].bEnableLevel = 0;
  powerManagementPowerMode[3].bEnableLevel = 0;

  /* WakeUpOnExternal test - Button (P1.1 = INT1) */
  powerManagementWakeUpOnExternal.bPin = 0x91;
  powerManagementWakeUpOnExternal.bEnableLevel = 0;

  /* Use Button (P1.1 = INT1) as PoweredUp pin */
  powerManagementPoweredUp.bPin = 0x91;
  powerManagementPoweredUp.bEnableLevel = 0;
  ZW_PortPinIn(powerManagementPoweredUp.bPin);
  poweredUpTransition = 0;
  poweredUpTimerHandle = ZW_TIMER_START(ZCB_powerManagementPoweredUpPinActive, 1, TIMER_FOREVER);

  /* WakeUpOnRF test */
  powerManagementWakeUpOnRFMode = PM_WAKEUP_MASK;
  powerManagementWakeUpOnRFCount = 8;
  powerManagementWakeUpOnRF[0].bValue = 0x94;
  powerManagementWakeUpOnRF[1].bValue = 0x01;
  powerManagementWakeUpOnRF[2].bValue = 0x00;
  powerManagementWakeUpOnRF[3].bValue = 0x00;
  powerManagementWakeUpOnRF[4].bValue = 0x00;
  powerManagementWakeUpOnRF[5].bValue = 0x00;
  powerManagementWakeUpOnRF[6].bValue = 0x00;
  powerManagementWakeUpOnRF[7].bValue = 0x27;

  powerManagementWakeUpOnRF[0].bMask = 0xFF;
  powerManagementWakeUpOnRF[1].bMask = 0xFF;
  powerManagementWakeUpOnRF[2].bMask = 0x00;
  powerManagementWakeUpOnRF[3].bMask = 0x07;
  powerManagementWakeUpOnRF[4].bMask = 0x00;
  powerManagementWakeUpOnRF[5].bMask = 0x00;
  powerManagementWakeUpOnRF[6].bMask = 0xFF;
  powerManagementWakeUpOnRF[7].bMask = 0xFF;

  /* WakeUpOnTimer test */
  powerManagementWakeUpOnTimer = PM_TIMER_SECONDS;
  powerManagementWakeUpOnTimerCount = 600;

  powerManagementState = POWER_MODE_POWERDOWN_TRANSITION;
#endif
#endif

#ifdef JP_DK
  /* This code is only for testing JP frequency outside of Japan */
  ZW_SetListenBeforeTalkThreshold(0, JP_DK_RSSI_THRESHOLD);
  ZW_SetListenBeforeTalkThreshold(1, JP_DK_RSSI_THRESHOLD);
  ZW_SetListenBeforeTalkThreshold(2, JP_DK_RSSI_THRESHOLD);
#endif

/* Do we together with the bTxStatus BYTE also transmit a sTxStatusReport struct on ZW_SendData callback to HOST */
#if SUPPORT_SEND_DATA_TIMING
  bTxStatusReportEnabled = TRUE;
#else
  bTxStatusReportEnabled = FALSE;
#endif

#if SUPPORT_SERIAL_API_STARTUP_NOTIFICATION
  /* ZW->HOST: bWakeupReason | bWatchdogStarted | deviceOptionMask | */
  /*           nodeType_generic | nodeType_specific | cmdClassLength | cmdClass[] */

  compl_workbuf[0] = applWakeupReason;
#if SUPPORT_ZW_WATCHDOG_START || SUPPORT_ZW_WATCHDOG_STOP
  compl_workbuf[1] = bWatchdogStarted;
#else
  compl_workbuf[1] = FALSE;
#endif
  compl_workbuf[2] = applNodeInfo_deviceOptionsMask;
  compl_workbuf[3] = applNodeInfo_nodeType_generic;
  compl_workbuf[4] = applNodeInfo_nodeType_specific;
#if SUPPORT_SERIAL_API_APPL_NODE_INFORMATION_CMD_CLASSES
  if (0 < applNodeInfo_unincluded_parmLength)
  {
    compl_workbuf[5] = applNodeInfo_unincluded_parmLength;
    for (i = 0; i < applNodeInfo_unincluded_parmLength; i++)
    {
      compl_workbuf[6 + i] = applNodeInfo_unincluded_nodeParm[i];
    }
  }
  else
#endif
  {
    compl_workbuf[5] = applNodeInfo_parmLength;
    for (i = 0; i < applNodeInfo_parmLength; i++)
    {
      compl_workbuf[6 + i] = applNodeInfo_nodeParm[i];
    }
  }
  Request(FUNC_ID_SERIAL_API_STARTED, compl_workbuf, 6 + i);

#endif /* #if SUPPORT_STARTUP_NOTIFICATION */

#if SUPPORT_SERIAL_API_APPL_NODE_INFORMATION_CMD_CLASSES
  m_AppNIF.cmdClassListNonSecureCount = applNodeInfo_unincluded_parmLength;
  m_AppNIF.cmdClassListNonSecureIncludedSecureCount = applNodeInfo_included_unsecure_parmLength;
  m_AppNIF.cmdClassListSecureCount = applNodeInfo_included_secure_parmLength;
  m_AppNIF.deviceOptionsMask = applNodeInfo_deviceOptionsMask;
  m_AppNIF.nodeType.generic = applNodeInfo_nodeType_generic;
  m_AppNIF.nodeType.specific = applNodeInfo_nodeType_specific;
  /* Now we are ready for setting up the active nodeinformation */
  Transport_OnApplicationInitSW( &m_AppNIF);
#endif
  UNUSED(nvmStatus);

  ZW_DEBUG_SEND_BYTE('S');
  ZW_DEBUG_SEND_BYTE('E');
  ZW_DEBUG_SEND_BYTE('R');
  ZW_DEBUG_SEND_BYTE(' ');
#ifdef ZW_SMARTSTART_ENABLED
  return applPowerMode;
#else
  return TRUE;
#endif
}


#ifdef ZW_SMARTSTART_ENABLED
#ifdef ZW_SLAVE
/*================   ApplicationNetworkLearnModeCompleted   ==================
**    Callback which is called on learnmode completed
**  Application specific handling of LearnModeCompleted - called from
**  protocol
**--------------------------------------------------------------------------*/
void
ApplicationNetworkLearnModeCompleted(
  BYTE bNodeID)                 /*IN The nodeID assigned*/
{
  /* If bNodeID == 0xff.. learn mode failed */
  if (bNodeID <= ZW_MAX_NODES)
  {
    appNodeId = bNodeID;
  }
#if SUPPORT_ZW_SET_LEARN_MODE
  ZCB_ComplHandler_ZW_SetLearnMode((APPLICATION_NETWORK_LEARN_MODE_COMPLETED_SMART_START_IN_PROGRESS == bNodeID)
                                   ? ASSIGN_NODEID_DONE :
                                     ASSIGN_COMPLETE, bNodeID);
#endif
  Transport_OnLearnCompleted(bNodeID);
}
#endif


#ifdef ZW_CONTROLLER
/*=================   ApplicationNetworkLearnModeCompleted   =================
**    Called by protocol Network Management Module when inclusion/exclusion
**  mode executes and finishes
**--------------------------------------------------------------------------*/
void
ApplicationNetworkLearnModeCompleted(
  LEARN_INFO *glearnNodeInfo)
{
  ZW_MEMORY_GET_ID(appHomeId, &appNodeId);
#if SUPPORT_ZW_SET_LEARN_MODE
  if (NULL != glearnNodeInfo)
  {
    ZCB_ComplHandler_ZW_NodeManagement(glearnNodeInfo);
  }
  else
  {
    LEARN_INFO tlearnNodeInfo = {0, 0, NULL, 0};
    ZCB_ComplHandler_ZW_NodeManagement(&tlearnNodeInfo);
  }
#else
  UNUSED(glearnNodeInfo)
#endif
}
#endif
#endif  /* #ifdef ZW_SMARTSTART_ENABLED */


#ifndef ZW_CONTROLLER_BRIDGE
#ifdef ZW_SECURITY_PROTOCOL
void                              /*RET Nothing                  */
Transport_ApplicationCommandHandler(
  ZW_APPLICATION_TX_BUFFER *pCmd, /* IN Payload from the received frame, */
                                  /*    the command is the very first byte */
  BYTE cmdLength,                 /* IN Number of command bytes including the command */
  RECEIVE_OPTIONS_TYPE *rxOpt)    /* rxOpt struct to use (may be partially filled out if
                                     parsing encapsulated command */
#else
/*==========================   ApplicationCommandHandler   ==================
**    Handling of received application commands and requests
**
**--------------------------------------------------------------------------*/
void                              /*RET Nothing                  */
ApplicationCommandHandler(
  ZW_APPLICATION_TX_BUFFER *pCmd, /* IN Payload from the received frame, */
                                  /*    the command is the very first byte */
  BYTE cmdLength,                 /* IN Number of command bytes including the command */
  RECEIVE_OPTIONS_TYPE *rxOpt)    /* rxOpt struct to use (may be partially filled out if
                                     parsing encapsulated command */
#endif
{
#if SUPPORT_SERIAL_API_POWER_MANAGEMENT
  if ((POWER_MODE_POWERDOWN == powerManagementState)
      && (!wakeUpOnRF && (PM_WAKEUP_UNDEFINED != powerManagementWakeUpOnRFMode)))
  {
    if ((PM_WAKEUP_ALL == powerManagementWakeUpOnRFMode)
        || ((PM_WAKEUP_ALL_NO_BROADCAST == powerManagementWakeUpOnRFMode) && !(rxOpt->rxStatus & RECEIVE_STATUS_TYPE_BROAD)))
    {
      /* Wakeup HOST */
      wakeUpOnRF = TRUE;
      powerManagementWakeUpReason = (PM_WAKEUP_ALL == powerManagementWakeUpOnRFMode)
                                    ? PM_WAKEUP_REASON_RF_ALL
                                    : PM_WAKEUP_REASON_RF_ALL_NO_BROADCAST;
    }
    else if ((PM_WAKEUP_MASK == powerManagementWakeUpOnRFMode)
             && (cmdLength >= powerManagementWakeUpOnRFCount))
    {
      /* Assume Match */
      wakeUpOnRF = TRUE;
      for (i = 0; i < powerManagementWakeUpOnRFCount; i++)
      {
        if ((powerManagementWakeUpOnRF[i].bMask & ((BYTE*)pCmd)[i]) != powerManagementWakeUpOnRF[i].bValue)
        {
          /* No Match */
          wakeUpOnRF = FALSE;
          break;
        }
      }
      if (wakeUpOnRF)
      {
        powerManagementWakeUpReason = PM_WAKEUP_REASON_RF_MASK;
      }
    }
  }
#endif
  n = 0;
  BYTE_IN_AR(compl_workbuf, n++) = rxOpt->rxStatus;
  BYTE_IN_AR(compl_workbuf, n++) = rxOpt->sourceNode;
  if (0 != (rxOpt->rxStatus & RECEIVE_STATUS_FOREIGN_FRAME))
  {
#if SUPPORT_PROMISCUOUS_APPLICATION_COMMAND_HANDLER
    /* Syntax when a promiscuous frame is received (i.e. RECEIVE_STATUS_FOREIGN_FRAME is set): */
    /* ZW->PC: REQ | 0xD1 | rxStatus | sourceNode | cmdLength | pCmd[] | destNode | multiNodeMaskLen [| multiNodeMask[multiNodeMaskLen]] | rssiVal */
    /* For libraries supporting promiscuous mode... */
    if (cmdLength > BUF_SIZE_TX - 6 - (0 != (rxOpt->rxStatus & RECEIVE_STATUS_TYPE_MULTI)) ? MAX_NODEMASK_LENGTH : 0)
    {
      cmdLength = BUF_SIZE_TX - 6 - (0 != (rxOpt->rxStatus & RECEIVE_STATUS_TYPE_MULTI)) ? MAX_NODEMASK_LENGTH : 0;
    }
    BYTE_IN_AR(compl_workbuf, n++) = cmdLength;
    for (i = 0; i < cmdLength; i++)
    {
      BYTE_IN_AR(compl_workbuf, n++) = *((BYTE_P)pCmd + i);
    }
    BYTE_IN_AR(compl_workbuf, n++) = rxOpt->destNode;
    {
      /* For now we always have ZERO byte in multicast nodemask even if it is a Multicast frame */
      /* No Bytes in Multicast nodemask */
      BYTE_IN_AR(compl_workbuf, n++) = 0;
    }
    BYTE_IN_AR(compl_workbuf, n++) = rxOpt->rxRSSIVal;
    RequestUnsolicited(FUNC_ID_PROMISCUOUS_APPLICATION_COMMAND_HANDLER, compl_workbuf, n);
#endif
  }
  else
  {
    /* ZW->PC: REQ | 0x04 | rxStatus | sourceNode | cmdLength | pCmd[] | rssiVal | securityKey */
    if (cmdLength > BUF_SIZE_TX - 5)
    {
      cmdLength = BUF_SIZE_TX - 5;
    }
    BYTE_IN_AR(compl_workbuf, n++) = cmdLength;
    for (i = 0; i < cmdLength; i++)
    {
      BYTE_IN_AR(compl_workbuf, n++) = *((BYTE_P)pCmd + i);
    }
    BYTE_IN_AR(compl_workbuf, n++) = rxOpt->rxRSSIVal;
    BYTE_IN_AR(compl_workbuf, n++) = rxOpt->securityKey;
    RequestUnsolicited(FUNC_ID_APPLICATION_COMMAND_HANDLER, compl_workbuf, n);
  }
}
#endif


#ifdef ZW_CONTROLLER_BRIDGE
/*======================   ApplicationCommandHandler_Bridge   ================
**    Handling of received application commands and requests
**
**--------------------------------------------------------------------------*/
void                              /*RET Nothing                  */
ApplicationCommandHandler_Bridge(
  ZW_MULTI_DEST *multi,           /* IN multicast structure - only valid if multicast frame */
  ZW_APPLICATION_TX_BUFFER *pCmd, /* IN Payload from the received frame, */
                                  /*    the command is the very first byte */
  BYTE cmdLength,                 /* IN Number of command bytes including the command */
  RECEIVE_OPTIONS_TYPE *rxOpt)    /* rxOpt struct to use (may be partially filled out if
                                     parsing encapsulated command */
{
#if SUPPORT_APPLICATION_COMMAND_HANDLER_BRIDGE
  n = 0;
  BYTE_IN_AR(compl_workbuf, n++) = rxOpt->rxStatus;
  if (0 != (rxOpt->rxStatus & RECEIVE_STATUS_FOREIGN_FRAME))
  {
#if SUPPORT_PROMISCUOUS_APPLICATION_COMMAND_HANDLER
    /* Syntax when a promiscuous frame is received (i.e. RECEIVE_STATUS_FOREIGN_FRAME is set): */
    /* ZW->PC: REQ | 0xD1 | rxStatus | sourceNode | cmdLength | pCmd[] | destNode | multiNodeMaskLen [| multiNodeMask[multiNodeMaskLen]] | rssiVal */
    BYTE_IN_AR(compl_workbuf, n++) = rxOpt->sourceNode;
    if (cmdLength > BUF_SIZE_TX - 6 - ((0 != (rxOpt->rxStatus & RECEIVE_STATUS_TYPE_MULTI)) ? MAX_NODEMASK_LENGTH : 0))
    {
      cmdLength = BUF_SIZE_TX - 6 - ((0 != (rxOpt->rxStatus & RECEIVE_STATUS_TYPE_MULTI)) ? MAX_NODEMASK_LENGTH : 0);
    }
    BYTE_IN_AR(compl_workbuf, n++) = cmdLength;
    for (i = 0; i < cmdLength; i++)
    {
      BYTE_IN_AR(compl_workbuf, n++) = *((BYTE_P)pCmd + i);
    }
    BYTE_IN_AR(compl_workbuf, n++) = rxOpt->destNode;
    {
      /* No Bytes in Multicast nodemask */
      BYTE_IN_AR(compl_workbuf, n++) = 0;
    }
    BYTE_IN_AR(compl_workbuf, n++) = rxOpt->rxRSSIVal;
    RequestUnsolicited(FUNC_ID_PROMISCUOUS_APPLICATION_COMMAND_HANDLER, compl_workbuf, n);
#endif
  }
  else
  {
    /* ZW->HOST: REQ | 0xA8 | rxStatus | destNode | sourceNode | cmdLength | pCmd[] | multiDestsOffset_NodeMaskLen [| multiDestsNodeMask[]] | rssiVal */
    BYTE_IN_AR(compl_workbuf, n++) = rxOpt->destNode;
    BYTE_IN_AR(compl_workbuf, n++) = rxOpt->sourceNode;
    if (cmdLength > BUF_SIZE_TX - 6 - (((0 != (rxOpt->rxStatus & RECEIVE_STATUS_TYPE_MULTI)) && (multi != NULL)) ? MAX_NODEMASK_LENGTH : 0))
    {
      cmdLength = BUF_SIZE_TX - 6 - (((0 != (rxOpt->rxStatus & RECEIVE_STATUS_TYPE_MULTI)) && (multi != NULL)) ? MAX_NODEMASK_LENGTH : 0);
    }
    BYTE_IN_AR(compl_workbuf, n++) = cmdLength;
    memcpy(compl_workbuf + n, (BYTE_P)pCmd, cmdLength);
    n += cmdLength;
    if ((0 != (rxOpt->rxStatus & RECEIVE_STATUS_TYPE_MULTI)) && (multi != NULL))
    {
      /* Its a Multicast frame */
      i = (multi->multiDestsOffset_NodeMaskLen & MULTI_DEST_MASK_LEN_MASK) + 1;
      if (i > BUF_SIZE_TX - n - 2)
      {
        i = BUF_SIZE_TX - n - 2;
      }
      memcpy(compl_workbuf + n, (BYTE_P)multi, i);
      n += i;
    }
    else
    {
      BYTE_IN_AR(compl_workbuf, n++) = 0;
    }
    BYTE_IN_AR(compl_workbuf, n++) = rxOpt->rxRSSIVal;
    /* Unified Application Command Handler for Bridge and Virtual nodes */
    RequestUnsolicited(FUNC_ID_APPLICATION_COMMAND_HANDLER_BRIDGE, compl_workbuf, n);
  }
#else
  /* Simulate old split Application Command Handlers */
  if (!ZW_IsVirtualNode(destNode))
  {
    /* ZW->PC: REQ | 0x04 | rxStatus | sourceNode | cmdLength | pCmd[] */
    BYTE_IN_AR(compl_workbuf, 0) = rxOpt->rxStatus;
    BYTE_IN_AR(compl_workbuf, 1) = rxOpt->sourceNode;
    if (cmdLength > BUF_SIZE_TX - 3)
    {
      cmdLength = BUF_SIZE_TX - 3;
    }
    BYTE_IN_AR(compl_workbuf, 2) = cmdLength;
    for (i = 0; i < cmdLength; i++)
    {
      BYTE_IN_AR(compl_workbuf, 3 + i) = *((BYTE_P)pCmd + i);
    }
    RequestUnsolicited(FUNC_ID_APPLICATION_COMMAND_HANDLER, compl_workbuf, 3 + cmdLength);
  }
  else
  {
    /* ZW->PC: REQ | 0xA1 | rxStatus | destNode | sourceNode | cmdLength | pCmd[] */
    BYTE_IN_AR(compl_workbuf, 0) = rxOpt->rxStatus;
    BYTE_IN_AR(compl_workbuf, 1) = rxOpt->destNode;
    BYTE_IN_AR(compl_workbuf, 2) = rxOpt->sourceNode;
    if (cmdLength > BUF_SIZE_TX - 4)
    {
      cmdLength = BUF_SIZE_TX - 4;
    }
    BYTE_IN_AR(compl_workbuf, 3) = cmdLength;
    for (i = 0; i < cmdLength; i++)
    {
      BYTE_IN_AR(compl_workbuf, 4 + i) = *((BYTE_P)pCmd + i);
    }

    RequestUnsolicited(FUNC_ID_APPLICATION_SLAVE_COMMAND_HANDLER, compl_workbuf, 4 + cmdLength);
  }
#endif
}
#endif


#if SUPPORT_SERIAL_API_APPL_NODE_INFORMATION_CMD_CLASSES

/* When building target with SECURITY support in protocol the ApplicationNodeInformation */
/* can be found in ZW_TransportSecProtocol.c */

#else
/*==========================   ApplicationNodeInformation   =================
**    Request Application Node information and current status
**    Called by the the Z-Wave application layer before transmitting a
**    "Node Information" frame.
**
**--------------------------------------------------------------------------*/
void               /*RET Nothing */
ApplicationNodeInformation(
  BYTE   *deviceOptionsMask,     /*OUT Bitmask with application options     */
  APPL_NODE_TYPE *nodeType, /*OUT  Device type Generic and Specific   */
  BYTE      **nodeParm,     /*OUT  Device parameter buffer pointer    */
  BYTE      *parmLength)    /*OUT  Number of Device parameter bytes   */
{
  *deviceOptionsMask = applNodeInfo_deviceOptionsMask;
  (*nodeType).generic = applNodeInfo_nodeType_generic;  /* Generic Device Type */
  (*nodeType).specific = applNodeInfo_nodeType_specific;  /* Specific Device Type */
  {
    *nodeParm = applNodeInfo_nodeParm;
    *parmLength = applNodeInfo_parmLength;
  }
}
#endif

#if SUPPORT_APPLICATION_SLAVE_COMMAND_HANDLER
/*======================   ApplicationSlaveCommandHandler   ==================
**    Handling of a received application slave commands and requests
**
**--------------------------------------------------------------------------*/
void                  /*RET  Nothing                  */
ApplicationSlaveCommandHandler(
  BYTE  rxStatus,     /*IN  Frame header info */
  BYTE  destNode,     /* To whom it might concern - which node is to receive the frame */
  BYTE  sourceNode,   /*IN  Command sender Node ID */
  ZW_APPLICATION_TX_BUFFER  *pCmd, /*IN  Payload from the received frame,
                                         the command is the very first byte */
  BYTE   cmdLength)   /*IN  Number of command bytes including the command */
{
  /* ZW->PC: REQ | 0xA1 | rxStatus | destNode | sourceNode | cmdLength | pCmd[] */
  BYTE_IN_AR(compl_workbuf, 0) = rxStatus;
  BYTE_IN_AR(compl_workbuf, 1) = destNode;
  BYTE_IN_AR(compl_workbuf, 2) = sourceNode;
  if (cmdLength > BUF_SIZE_TX - 4)
  {
    cmdLength = BUF_SIZE_TX - 4;
  }
  BYTE_IN_AR(compl_workbuf, 3) = cmdLength;
  for (i = 0; i < cmdLength; i++)
  {
    BYTE_IN_AR(compl_workbuf, 4 + i) = *((BYTE_P)pCmd + i);
  }

  RequestUnsolicited(FUNC_ID_APPLICATION_SLAVE_COMMAND_HANDLER, compl_workbuf, 4 + cmdLength);
}
#endif


#ifdef ZW_CONTROLLER_BRIDGE
/* TODO - Bridge work, need to determine if we want more slave nodetypes */
/* and if this should be handled by sending the nodeinformation request to the */
/* PC, so it is up to the PC to set the nodeinformation when needed */
/* Or it is OK to set nodeinformation from PC side and then start the learning */
/*======================   ApplicationSlaveNodeInformation   =================
**    Request Application Node information and current status
**    Called by the the Z-Wave application layer before transmitting a
**    "Node Information" frame.
**
**--------------------------------------------------------------------------*/
void               /*RET Nothing */
ApplicationSlaveNodeInformation(
  BYTE      destNode,       /* IN Which node do we want the nodeinfo on */
  BYTE   *deviceOptionsMask,     /*OUT Bitmask with application options     */
  APPL_NODE_TYPE *nodeType, /*OUT  Device type Generic and Specific   */
  BYTE      **nodeParm,     /*OUT  Device parameter buffer pointer    */
  BYTE      *parmLength)    /*OUT  Number of Device parameter bytes   */
{
  UNUSED(destNode);
  *deviceOptionsMask = applSlaveNodeInfo_deviceOptionsMask;
  (*nodeType).generic = applSlaveNodeInfo_nodeType_generic;  /* Generic Device Type */
  (*nodeType).specific = applSlaveNodeInfo_nodeType_specific;  /* Specific Device Type */
  *nodeParm = applSlaveNodeInfo_nodeParm;
  *parmLength = applSlaveNodeInfo_parmLength;
}
#endif


#ifdef ZW_CONTROLLER
/*=====================   ApplicationControllerUpdate   =====================
**    Inform the static controller of node information update done through
**   the network managment.
**
**--------------------------------------------------------------------------*/
void                                /* RET  Nothing                         */
ApplicationControllerUpdate(
  BYTE bStatus,                     /* IN   Status of learn mode            */
  BYTE bNodeID,                     /* IN   Node id of node sending nodeinfo*/
  BYTE *pCmd,                       /* IN   Pointer to appl. node info      */
  BYTE bLen)                        /* IN   Node info length                */
{
  BYTE_IN_AR(compl_workbuf, 0) = bStatus;
  BYTE_IN_AR(compl_workbuf, 1) = bNodeID;
  /*  - Buffer boundary check */
  if (bLen > BUF_SIZE_TX - 3)
  {
    bLen = BUF_SIZE_TX - 3;
  }
  BYTE_IN_AR(compl_workbuf, 2) = bLen;
  for (i = 0; i < bLen; i++)
  {
    BYTE_IN_AR(compl_workbuf, 3 + i) = *(pCmd + i);
  }
  if (bStatus)
  {
    RequestUnsolicited(FUNC_ID_ZW_APPLICATION_UPDATE, compl_workbuf, bLen + 3);
  }
}
#endif /* ZW_CONTROLLER */


#ifdef ZW_SLAVE
/*======================   ApplicationSlaveUpdate   =========================
**    Inform the slave of node information received
**
**--------------------------------------------------------------------------*/
void                                /* RET  Nothing                         */
ApplicationSlaveUpdate(
  BYTE bStatus,                     /* IN   Status of learn mode            */
  BYTE bNodeID,                     /* IN   Node id of node sending nodeinfo*/
  BYTE *pCmd,                       /* IN   Pointer to appl. node info      */
  BYTE bLen)                        /* IN   Node info length                */
{
  BYTE_IN_AR(compl_workbuf, 0) = bStatus;
  BYTE_IN_AR(compl_workbuf, 1) = bNodeID;
  /*  - Buffer boundary check */
  if (bLen > BUF_SIZE_TX - 3)
  {
    bLen = BUF_SIZE_TX - 3;
  }
  BYTE_IN_AR(compl_workbuf, 2) = bLen;
  for (i = 0; i < bLen; i++)
  {
    BYTE_IN_AR(compl_workbuf, 3 + i) = *(pCmd + i);
  }
  RequestUnsolicited(FUNC_ID_ZW_APPLICATION_UPDATE, compl_workbuf, bLen + 3);
}

#endif /* ZW_SLAVE */


#ifdef ZW_SECURITY_PROTOCOL
/*===========================   ApplicationSecurityEvent   =========================
**    Notify the application when the a security event are trickered
**
**  Possible security events:
**  - E_APPLICATION_SECURITY_EVENT_S2_INCLUSION_REQUEST_DSK_CSA - S2 inclusion needs
**    the CSA DSK for making the S2 inclusion.
**---------------------------------------------------------------------------------*/
void
ApplicationSecurityEvent(
  s_application_security_event_data_t *securityEvent)
{
  /* ZW->HOST: retValLen(1) | retVal[retValLen] */
  BYTE_IN_AR(compl_workbuf, 0) = securityEvent->event;
  BYTE_IN_AR(compl_workbuf, 1) = securityEvent->eventDataLength;
  for (i = 0; i < securityEvent->eventDataLength; i++)
  {
    BYTE_IN_AR(compl_workbuf, 2 + i) = securityEvent->eventData[i];
  }
  RequestUnsolicited(FUNC_ID_APPLICATION_SECURITY_EVENT, compl_workbuf, securityEvent->eventDataLength + 2);
}
#endif


/*===========================   ApplicationRfNotify   ===========================
**    Notify the application when the radio switch state
**    Called from the Z-Wave PROTOCOL When radio switch from Rx to Tx or from Tx to Rx
**    or when the modulator PA (Power Amplifier) turn on/off
**---------------------------------------------------------------------------------*/
void          /*RET Nothing */
ApplicationRfNotify(
  BYTE rfState)         /* IN state of the RF, the available values is as follow:
                               ZW_RF_TX_MODE: The RF switch from the Rx to Tx mode, the modualtor is started and PA is on
                               ZW_RF_PA_ON: The RF in the Tx mode, the modualtor PA is turned on
                               ZW_RF_PA_OFF: the Rf in the Tx mode, the modulator PA is turned off
                               ZW_RF_RX_MODE: The RF switch from Tx to Rx mode, the demodulator is started.*/
{
  UNUSED(rfState);
}


#if SUPPORT_SERIAL_API_APPL_NODE_INFORMATION_CMD_CLASSES
/**
* Set up security keys to request when joining a network.
* These are taken from the config_app.h header file.
*/
BYTE
ApplicationSecureKeysRequested(void)
{
  return sRequestedSecuritySettings.requestedSecurityKeysBits;
}


/**
* Set up security S2 inclusion authentication to request when joining a network.
* These are taken from the config_app.h header file.
*/
BYTE
ApplicationSecureAuthenticationRequested(void)
{
  return sRequestedSecuritySettings.requestedSecurityAuthentication;
}
#endif

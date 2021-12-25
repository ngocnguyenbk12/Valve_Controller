/*
 * uart_command.h
 *
 *  Created on: Oct 23, 2021
 *      Author: SF314-56
 */

#ifndef _UART_COMMAND_H_
#define _UART_COMMAND_H_

#include "hw_config.h"

#ifndef BOOL
#define BOOL char
#endif

//#define DBG
#ifndef BYTE
typedef unsigned char   BYTE;
typedef unsigned char   IBYTE;
typedef unsigned short  WORD;
typedef unsigned short  IWORD;
typedef unsigned long   DWORD;
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
/* serial Protocol handler states */
enum
{
  stateSOFHunt = 0,
  stateLen = 1,
  stateCmd = 2,
  stateType = 3,
  stateData = 4,
  stateChecksum = 5
};
/* return values for ConUpdate */
typedef enum _T_CON_TYPE
{
  conIdle =0,           // returned if nothing special has happened
  conFrameReceived,     // returned when a valid frame has been received
  conFrameSent,         // returned if frame was ACKed by the other end
  conFrameErr,          // returned if frame has error in Checksum
  conRxTimeout,         // returned if Rx timeout has happened
  conTxTimeout          // returned if Tx timeout (waiting for ACK) ahs happened
}T_CON_TYPE;

/* defines for accessing serial protocol data */
#define serFrameLen (*serBuf)
#define serFrameType (*(serBuf + 1))
#define serFrameCmd (*(serBuf + 2))
#define serFrameDataPtr (serBuf + 3)

#define SOF 0x01  /* Start Of Frame */
#define ACK 0x06  /* Acknowledge successfull frame reception */
#define NAK 0x15  /* Not Acknowledge successfull frame reception - please retransmit... */
#define CAN 0x18  /* Frame received (from host) was dropped - waiting for ACK */


/************************************************************/
/* Structs and unions that can be used by the application   */
/* to construct the frames to be sent                       */
/************************************************************/
typedef struct _ZW_UART_COMMON_FRAME_
{
  BYTE length;
  BYTE type;
  BYTE cmd;
} ZW_UART_COMMON_FRAME;

typedef struct _ZW_UART_COMMAND_SET
{
  BYTE length;
  BYTE type;
  BYTE cmd;
  BYTE value1;
//  BYTE value2;
 // BYTE value3;
 // BYTE value4;
}ZW_UART_COMMAND_SET;

typedef struct _ZW_UART_COMMAND_REPORT
{
  BYTE length;
  BYTE type;
  BYTE cmd;

  BYTE value1;
  BYTE value2;
  BYTE value3;
  BYTE value4;
  BYTE value5;
  BYTE value6;
  BYTE value7;
  BYTE value8;
  BYTE value9;
  BYTE value10;
  BYTE value11;
  BYTE value12;
  BYTE value13;
}ZW_UART_COMMAND_REPORT;

typedef struct _ZW_UART_COMMAND_GET
{
  BYTE length;
  BYTE type;
  BYTE cmd;
  BYTE value;
}ZW_UART_COMMAND_GET;


typedef union _ZW_UART_COMMAND
{
  ZW_UART_COMMON_FRAME zw_uartcommand;
  ZW_UART_COMMAND_SET zw_uartcommandset;
  ZW_UART_COMMAND_GET zw_uartcommandget;
  ZW_UART_COMMAND_REPORT zw_uartcommandreport;
}ZW_UART_COMMAND;




/* Command Zwave to device*/
#define COMMAND_STATUS		0x00
#define COMMAND_VERSION 	0x01
#define COMMAND_ZW_CONNECT      0x02
#define COMMAND_TIMER           0x03
#define COMMAND_TEMPERATURE     0x04
#define COMMAND_HUMIDITY        0x05
#define COMMAND_HEATER          0x06
#define COMMAND_ALARM           0x07
#define COMMAND_ACCELERATION    0x08
#define COMMAND_POWER_MODE      0x09
#define COMMAND_WAKEUP	        0x0A
#define COMMAND_ZWAVE_RESET     0x0B
#define COMMAND_SWITCHBINARY	0x0C
#define COMMAND_METER           0x0D
#define COMMAND_FLOW			0x0E
#define COMMAND_PAIR 			0X10
#define COMMAND_VALVE 			0x11
#define COMMAND_LEAK			0x12
#define COMMAND_CONFIGURATION 	0x13

#define COMMAND_END COMMAND_CONFIGURATION
/* Command Valve */
#define ZW_VALVE_GET		0x01
#define ZW_VALVE_SET		0x02
#define ZW_VALVE_ON			0x03
#define ZW_VALVE_OFF		0x04
#define ZW_VALVE_RESET		0x05


/* Command Zwave Status*/
#define ZW_SET_STATUS	        0x01
#define ZW_GET_STATUS		0x02
#define ZW_REPORT_STATUS	0x03

/* Command get version*/
#define ZW_SET_VERSION		0x01
#define ZW_GET_VERSION		0x02
#define ZW_REPORT_VERSION       0x03

/* Command zwave reset*/
#define ZW_SET_RESET            0x01
#define ZW_GET_RESET            0x02
#define ZW_REPORT_RESET         0x03

/* Command wakeup*/
#define ZW_SET_WAKEUP		0x01
#define ZW_GET_WAKEUP		0x02
#define ZW_REPORT_WAKEUP	0x03

/* Command z-wave inclusion Network*/
#define ZW_SET_CONNECT          0x01
#define ZW_GET_CONNECT          0x02
#define ZW_REPORT_CONNECT       0x03
#define ZW_CONNECT              0x01
#define ZW_DISCONNECT           0x00

/* Command get/set Temperature*/
#define ZW_SETPOINT_SET_TEMPERATURE      0x01
#define ZW_SETPOINT_GET_TEMPERATURE      0x02
#define ZW_SETPOINT_REPORT_TEMPERATURE   0x03
#define ZW_GET_TEMPERATURE               0x04
#define ZW_REPORT_TEMPERATURE            0x05

/* Command get Humidity*/
#define ZW_SET_HUMIDITY         0x01
#define ZW_GET_HUMIDITY         0x02
#define ZW_REPORT_HUMIDITY      0x03

/* Command set/get Heater*/
#define ZW_MODESET_HEATER       0x01
#define ZW_MODEGET_HEATER       0x02
#define ZW_MODEREPORT_HEATER    0x03
#define ZW_MODESUPPORT_HEATER	0x04

/* Command ACCELERATION */
#define ZW_ACCELERATION_GET_X   0x01
#define ZW_ACCELERATION_GET_Y   0x02
#define ZW_ACCELERATION_GET_Z   0x03

/* Command Alarm*/
#define ZW_ALARM_SET            0x01
#define ZW_ALARM_GET            0x02
#define ZW_ALARM_REPORT         0x03

/* Command Power Mode*/
#define ZW_POWER_SET            0x01
#define ZW_POWER_GET            0x02
#define ZW_POWER_HIGH_MODE      0x00
#define ZW_POWER_LOW_MODE       0x01

/* Command Switch binary*/
#define ZW_SWITCHBINARY_SET 	0x01
#define ZW_SWITCHBINARY_GET 	0x02
#define ZW_SWITCHBINARY_REPORT	0x03
#define SWITCH_1  0x02
#define SWITCH_2	0x03
#define SWITCH_ALL	0x01

/* Command Meter*/
#define ZW_METER_SET 	0x01
#define ZW_METER_GET 	0x02
#define ZW_METER_RESET  0x03
#define ZW_METER_REPORT	0x04

/* Command Flow*/
#define ZW_FLOW_SET	0x01
#define ZW_FLOW_GET	0x02
#define ZW_FLOW_REPORT	0x03

/* Command Valve Leak */
#define ZW_LEAK			0x01
#define ZW_NON_LEAK		0x02

/* Command For Configuration */
#define ZW_CONFIGURATION 0x01
#define ZW_CONFIGURATION_SET 	0x02
#define ZW_CONFIGURATION_RESET 	0x03

/****************************************************************************/
/*                              EXTERNAL DEFINED FUNCTIONS/DATA             */
/****************************************************************************/

/* Serial buffer size */
#define SERBUF_MAX  180
#define FRAME_LENGTH_MIN  3
#define FRAME_LENGTH_MAX  SERBUF_MAX

BOOL ZW_UART_transmit(ZW_UART_COMMAND cmd);
void ZW_UART_SEND_BYTE(uint8_t byte);
T_CON_TYPE uart_check_protocol(uint8_t c,  BYTE acknowledge);


#endif /* INC_UART_COMMAND_H_ */

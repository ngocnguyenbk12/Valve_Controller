/*
 * uart_command.c
 *
 *  Created on: Oct 23, 2021
 *      Author: SF314-56
 */

#include "uart_command.h"
#include "hw_config.h"
#include "string.h"
#include "stdio.h"

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/
volatile BYTE ZW_UART_RECEIVE=0;
volatile BYTE ZW_UART_RECEIVE_STATUS=0;
volatile BYTE serBuf[SERBUF_MAX];
IBYTE serBufLen ; // , con_state;
IBYTE bChecksum_RX;
volatile BOOL AckNakNeeded = FALSE;
volatile BOOL AckNak = ACK;
IBYTE con_state = stateSOFHunt;

/*
BYTE serBuf[SERBUF_MAX];
IBYTE serBufLen, con_state;
IBYTE bChecksum_RX;

volatile BOOL AckNakNeeded = FALSE;
volatile BOOL AckNak = ACK;
*/
/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/


/*===========================   Uart_send_command   =========================
**    This function send command via uart.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/


BOOL ZW_UART_transmit(ZW_UART_COMMAND cmd){

	uint8_t t_command;
	uint8_t* cmd_point;
	uint8_t i ;

	uint8_t command[20];

	uint8_t bChecksum = 0xFF;


	command[0] = SOF;
	command[1] = cmd.zw_uartcommand.length + 3;
	bChecksum ^= command[1];
	command[2] = 0x01;
	bChecksum ^= command[2];
	command[3] = cmd.zw_uartcommand.length;
	bChecksum ^= command[3];
/*
	cmd_point = &cmd.zw_uartcommand.length;
	i = *(cmd_point);
	t_command = 4;

	for(;i>0;i--)
	{
		command[t_command] = (*(cmd_point));
		bChecksum ^= *(cmd_point);
		cmd_point++;
		t_command++;
	}

*/

	command[4] = cmd.zw_uartcommand.cmd;
	bChecksum ^= command[4];

	command[5] = cmd.zw_uartcommand.type;
	bChecksum ^= command[5];

	command[6] = cmd.zw_uartcommandset.value1;
	bChecksum ^= command[6];



	command[7] = bChecksum;
	HAL_UART_Transmit(&huart1,(uint8_t *)command, cmd.zw_uartcommand.length + 5, 1000);
	AckNakNeeded = TRUE;
	AckNak = NAK;

	for(uint8_t res_time; res_time < 100000; i++){

		if(AckNakNeeded == FALSE){
			break;
		}

		if(AckNak == ACK)
		{
			return TRUE;
		}
	}
	return FALSE;


}

void ZW_UART_SEND_BYTE(uint8_t byte)
{
	HAL_UART_Transmit(&huart1, (uint8_t*)byte, 1, 1000);
}


T_CON_TYPE uart_check_protocol(uint8_t c , BYTE acknowledge){

	static T_CON_TYPE retval = conIdle;

		switch (con_state)
		{
		case stateSOFHunt:
			if(c == SOF){
				bChecksum_RX = 0xff;
				serBufLen = 0;
				con_state ++;

			}

			else{
				if (AckNakNeeded)
				          {
				              if (c == ACK)
				              {
				                retval = conFrameSent;
				                AckNakNeeded = FALSE;  // Done
				                AckNak = ACK;
				              }
				              else if (c == NAK)
				              {
				                retval = conTxTimeout;
				                AckNakNeeded = FALSE;
				                AckNak = NAK;
				              }
				              else
				              {
				                // Bogus character received...
				              }
				            }
			}

			break;
		case stateLen:
			 if (( c< FRAME_LENGTH_MIN) || (c> FRAME_LENGTH_MAX))
			          {
			            con_state = stateSOFHunt; // Restart looking for SOF
			            break;
			          }

        case stateCmd :
          if(serBufLen &&  (c > COMMAND_END))
          {
            con_state = stateSOFHunt; // Restart looking for SOF
            break;
          }

        case stateType:
        	con_state++;

		case stateData:
				if(serBufLen < SERBUF_MAX){
					serBuf[serBufLen] = c;
					serBufLen++;
					bChecksum_RX ^= c;
					if (serBufLen >= serFrameLen)
					   {
					              con_state++;
					   }
					}
				else{
					con_state++;
					}
				break;

		case stateChecksum:

			if(acknowledge){
				if(c == bChecksum_RX)
				{
					//gpio_SetPin(Led_flow, On);
					retval = conFrameReceived;
					con_state = stateSOFHunt;
				}
				else{
					retval = conFrameErr;
					con_state = stateSOFHunt;
				}
			}
			break;
		}
	return retval;
}


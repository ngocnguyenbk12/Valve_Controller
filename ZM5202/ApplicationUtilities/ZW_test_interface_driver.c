/*
 * ZW_test_interface_driver.c
 *
 *  Created on: Mar 27, 2011
 *      Author: esban
 */

/* Put XDATA in XDATA_LOWER because upper XDATA is full on some targets */
#ifdef __C51__
#pragma userclass (xdata = LOWER)
#endif

/****************************************************************************

 THIS SOFTWARE IS NOT COPYRIGHTED

 HP offers the following for use in the public domain.  HP makes no
 warranty with regard to the software or it's performance and the
 user accepts the software "AS IS" with all faults.

 HP DISCLAIMS ANY WARRANTIES, EXPRESS OR IMPLIED, WITH REGARD
 TO THIS SOFTWARE INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.

 ****************************************************************************/

/****************************************************************************
 *  Header: remcom.c,v 1.34 91/03/09 12:29:49 glenne Exp $
 *
 *  Module name: remcom.c $
 *  Revision: 1.34 $
 *  Date: 91/03/09 12:29:49 $
 *  Contributor:     Lake Stevens Instrument Division$
 *
 *  Description:     low level support for gdb debugger. $
 *
 *  Considerations:  only works on target hardware $
 *
 *  Written by:      Glenn Engel $
 *  ModuleState:     Experimental $
 *
 *  NOTES:           See Below $
 *
 *  Modified for 8051 by Anders Esbensen, Sigma Designs.
 *
 *  This code has been extensively tested on the Fujitsu SPARClite demo board.
 *
 *  To enable debugger support, two things need to happen.  One, a
 *  call to set_debug_traps() is necessary in order to allow any breakpoints
 *  or error conditions to be properly intercepted and reported to gdb.
 *  Two, a breakpoint needs to be generated to begin communication.  This
 *  is most easily accomplished by a call to breakpoint().  Breakpoint()
 *  simulates a breakpoint by executing a trap #1.
 *
 *************
 *
 *    The following gdb commands are supported:
 *
 * command          function                               Return value
 *
 *    g             return the value of the CPU registers  hex data or ENN
 *    G             set the value of the CPU registers     OK or ENN
 *
 *    mAA..AA,LLLL  Read LLLL bytes at address AA..AA      hex data or ENN
 *    MAA..AA,LLLL: Write LLLL bytes at address AA.AA      OK or ENN
 *
 *    c             Resume at current address              SNN   ( signal NN)
 *    cAA..AA       Continue at address AA..AA             SNN
 *
 *    s             Step one instruction                   SNN
 *    sAA..AA       Step one instruction from AA..AA       SNN
 *
 *    k             kill
 *
 *    ?             What was the last sigval ?             SNN   (signal NN)
 *
 * All commands and responses are sent with a packet which includes a
 * checksum.  A packet consists of
 *
 * $<packet info>#<checksum>.
 *
 * where
 * <packet info> :: <characters representing the command or response>
 * <checksum>    :: < two hex digits computed as modulo 256 sum of <packetinfo>>
 *
 * When a packet is received, it is first acknowledged with either '+' or '-'.
 * '+' indicates a successful transfer.  '-' indicates a failed transfer.
 *
 * Example:
 *
 * Host:                  Reply:
 * $m0,10#2a               +$00010203040506070809101112131415#42
 *
 ****************************************************************************/

#include "config_lib.h"

#ifdef ZW_TEST_INTERFACE_DRIVER

#include <INTRINS.h>
#include <ZW050x.h>
#include <ZW_pindefs.h>
#include <ZW_evaldefs.h>
#include <ZW_uart_api.h>
#include <ZW_util_queue_api.h>
#include <ZW_mem_api.h>
#include <ZW_test_interface_driver.h>
#include <ZW_task.h>


#define USER_INTERRUPT 0
#define BREAK_POINT 1
#define BREAK_SINGLESTEP 2

/************************************************************************
 *
 * external low-level support routines
 */

#define ZDB_USE_UART0
#ifdef ZDB_USE_UART0
/* write a single character      */
#define ZDB_SEND_BYTE ZW_UART0_tx_send_byte
/* read and return a single char */
#define ZDB_GET_BYTE ZW_UART0_rx_data_wait_get
#define ZDB_UART_INIT ZW_UART0_init
#define ZDB_UART_RI_BIT UARTSTAT_RI0_BIT
#define ZDB_ES ES0
#define ZDB_UART_rx_int_clear ZW_UART0_rx_int_clear
#define ZDB_INUM_SERIAL INUM_SERIAL0

// DMA fun
#define ZDB_UART_DMA_INIT ZW_UART0_rx_dma_init
#define ZDB_UART_DMA_EOR_SET ZW_UART0_rx_dma_eor_set
#define ZDB_UART_DMA_INT_BYTE_COUNT ZW_UART0_rx_dma_int_byte_count


#else /*ZDB_USE_UART0*/
#define ZDB_SEND_BYTE ZW_UART1_tx_send_byte
/* read and return a single char */
#define ZDB_GET_BYTE ZW_UART1_rx_data_wait_get
#define ZDB_UART_INIT ZW_UART1_init
#define ZDB_UART_RI_BIT UARTSTAT_RI1_BIT
#define ZDB_ES ES1
#define ZDB_UART_rx_int_clear ZW_UART1_rx_int_clear
#define ZDB_INUM_SERIAL INUM_SERIAL1
#endif /*ZDB_USE_UART0*/

/************************************************************************/
/* BUFMAX defines the maximum number of characters in inbound/outbound buffers*/
#define BUFMAX (32+1)

/*
 * DMA FUN
 */
#define DMA_EOR_CHARACTER ('\r')
#ifdef __C51__
XBYTE dmaBuffer[DMA_BUFFER_SIZE] _at_ 0x072D;
#else
XBYTE dmaBuffer[DMA_BUFFER_SIZE];
#endif
static BYTE dmaBufferOffset = DMA_BUFFER_SIZE_SINGLE; // Offset => Buffer 2
static BOOL dmaDataIsReady = FALSE;
/*
 * DMA FUN OVER
 */

/***************************  ASSEMBLY CODE MACROS *************************/
/*                     */
#ifdef __C51__
//#pragma NOAREGS /* Functions sometimes called from interrupt using other register bank */
#endif
/* Convert ch from a hex digit to an int */

/*
 * The following queue and buffer is declared with the purpose of having a
 * terminal where a user can input commands handled in the application layer.
 */
static QUEUE_T debugQueue;
#define DEBUG_QUEUE_BUFFER_SIZE (20)
static BYTE debugBuffer[DEBUG_QUEUE_BUFFER_SIZE];

void ZW_test_interface_driver_init(void)
{
  ZDB_UART_INIT(1152, TRUE, TRUE);

  // Init queue used for debug commands.
  ZW_util_queue_Init(&debugQueue, debugBuffer, sizeof(debugBuffer[0]), DEBUG_QUEUE_BUFFER_SIZE);

  ZDB_ES = 1;  /* Enable "serial" interrupt */

  // DMA INTERRUPT ON EOR
  ZDB_UART_DMA_INT_BYTE_COUNT(0);
  ZDB_UART_DMA_INIT(dmaBuffer, DMA_BUFFER_SIZE_SINGLE, (UART_RX_DMA_SWITCH_EOR | UART_RX_DMA_SWITCH_FULL));
  ZDB_UART_DMA_EOR_SET(DMA_EOR_CHARACTER);
}


/* UART interrupt handler */
#ifdef __C51__
static void ZDB_UART_RX_TX_interrupt(void) interrupt ZDB_INUM_SERIAL
{
  _push_(SFRPAGE);
  if (ZW_UART0_rx_int_get() != 0x00)
  {
    dmaBufferOffset = (ZW_UART0_rx_dma_status() & 0x02) ? 0 : DMA_BUFFER_SIZE_SINGLE;
    dmaDataIsReady = TRUE;
    TaskInterruptSignal();
    ZW_UART0_rx_int_clear();
  }
  _pop_(SFRPAGE);
}
#endif


BOOL ZW_test_interface_driver_getData(BYTE * pData, BYTE * pLength)
{
  BOOL fStatus = FALSE;
  ZDB_ES = 0; // Disable interrupt
  if (TRUE == dmaDataIsReady)
  {
    BYTE byteCount;
    for (byteCount = 0; byteCount < DMA_BUFFER_SIZE_SINGLE; byteCount++)
    {
      if (DMA_EOR_CHARACTER == dmaBuffer[dmaBufferOffset + byteCount])
      {
        *pLength = byteCount + 1;
        break;
      }
    }

    dmaDataIsReady = FALSE;
    memcpy(pData, &dmaBuffer[dmaBufferOffset], *pLength);
    fStatus = TRUE;
  }
  ZDB_ES = 1; // Enable interrupt
  return fStatus;
}

#endif  /* ZW_TEST_INTERFACE_DRIVER */

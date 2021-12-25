/****************************************************************************
 *
 * Copyright (c) 2001-2013
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 *---------------------------------------------------------------------------
 *
 * Description:     Header file for conhandle module
 *
 * Last Changed By:  $Author: jsi $
 * Revision:         $Revision: 30505 $
 * Last Changed:     $Date: 2014-12-23 13:26:32 +0100 (ti, 23 dec 2014) $
 *
 ****************************************************************************/
#ifndef _CONHANDLE_H_
#define _CONHANDLE_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/


/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/
/* return values for ConUpdate */
enum T_CON_TYPE
{
  conIdle,            // returned if nothing special has happened
  conFrameReceived,   // returned when a valid frame has been received
  conFrameSent,       // returned if frame was ACKed by the other end
  conFrameErr,        // returned if frame has error in Checksum
  conRxTimeout,       // returned if Rx timeout has happened
  conTxTimeout        // returned if Tx timeout (waiting for ACK) ahs happened
};

/* defines for accessing serial protocol data */
#define serFrameLen (*serBuf)
#define serFrameType (*(serBuf + 1))
#define serFrameCmd (*(serBuf + 2))
#define serFrameDataPtr (serBuf + 3)

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/
/* Serial buffer size */
#define SERBUF_MAX  180
#define FRAME_LENGTH_MIN  3
#define FRAME_LENGTH_MAX  SERBUF_MAX

/* Serial timeout definitions - These can be changed to fit the requirements present */
/* Either timeout definition should never be set lower than 2 (20ms) */

/* ACK Timeout is 1500 ms to allow HOST system to react - module transmits and waits */
/* max 1500ms before either starting a retransmit or simply drops the frame */
#define RX_ACK_TIMEOUT_DEFAULT  150
/* Receive byte inframe timeout is 150 ms to allow for byte not being fetched */
/* "instantaneously" from receive buffer after physically being buffered by the */
/* interrupt routine - this also includes HOST system byte transmit delays */
#define RX_BYTE_TIMEOUT_DEFAULT 15


extern BYTE timeOutRX_ACK;
extern BYTE timeOutRX_BYTE;
extern BYTE serBuf[SERBUF_MAX];
extern BYTE serBufLen;

/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

/*==============================   ConTxActive   =============================
**
**    Checks if TxQueue is actively transmitting
**
**--------------------------------------------------------------------------*/
BYTE
ConTxActive(void);

/*===============================   ConTxFrame   =============================
**
**   Transmit frame via serial port by adding SOF, Len, Type, cmd and Chksum.
**   Frame  : SOF-Len-Type-Cmd-DATA-Chksum
**    where SOF is Start of frame byte
**          Len is length of bytes between and including Len to last DATA byte
**          Type is Response or Request
**          Cmd Serial application command describing what DATA is
**          DATA as it says
**          Chksum is a XOR checksum taken on all BYTEs from Len to last DATA byte
**
**          NOTE: If Buf is NULL then the previously used cmd, type, Buf and len
**          is used again (Retransmission)
**
**--------------------------------------------------------------------------*/
extern void   /*RET Nothing */
ConTxFrame(
  BYTE cmd,   /* IN Command */
  BYTE type,  /* IN frame Type to send (Response or Request) */
  XBYTE *Buf, /* IN pointer to BYTE buffer containing DATA to send */
  BYTE len    /* IN the length of DATA to transmit */
);

/*==============================   ConUpdate   =============================
**
**   Parses serial data sent from external controller module (PC-controller).
**   Should be frequently called by main loop.
**
**   Return: conIdle          if nothing has happened
**           conFrameReceived if valid frame was received
**           conFrameSent     if transmitted frame was ACKed by other end
**           conFrameErr      if received frame has error in Checksum
**           conRxTimeout     if a Rx timeout happened
**           conTxTimeout     if a Tx timeout waiting for ACK happened
**
**--------------------------------------------------------------------------*/
extern enum T_CON_TYPE  /*RET conState (See above) */
ConUpdate(
  BYTE acknowledge      /* IN Nothing */
);

/*==============================   ConInit   =============================
**
**   Initialize the module.
**
**--------------------------------------------------------------------------*/
void                    /*RET  Nothing */
ConInit(
  WORD bBaudRate        /* IN  Baud rate / 100 (e.g. 96=>9600baud/s, 1152=>115200baud/s) */
);

#endif /* _CONHANDLE_H_ */

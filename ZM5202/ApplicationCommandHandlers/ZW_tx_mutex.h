/**
 * @file
 * Transmit mutex.
 *
 * Protected transmit buffers used to send data. Use this module to get
 * a transmit buffer and release the buffer again when data is sent and application
 * is notified with a callback. There are 2 types for buffers: one for unsolicited
 * commands and one for responses to incoming commands.
 *
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _ZW_MUTEX_H_
#define _ZW_MUTEX_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#ifdef __C51__
#include <ZW_stdint.h>
#else
#include <stdint.h>
#endif
#include <ZW_typedefs.h>
#include <ZW_classcmd.h>
#include <CommandClass.h>
#include <ZW_transport_api.h>
/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

/**
 * Allocates space for Supervision when used to declare a buffer.
 */
typedef struct _REQ_BUF_
{
  BYTE multichanCmdEncap[4]; //4 = sizeof(ZW_MULTI_CHANNEL_CMD_ENCAP_V2_FRAME) - sizeof(ALL_EXCEPT_ENCAP)
  ZW_SUPERVISION_GET_FRAME supervisionGet;
  ZW_APPLICATION_TX_BUFFER appTxBuf;
} REQ_BUF;

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/


/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

/**
 * @brief Initializes the TX mutexes.
 */
void
mutex_init(void);

/**
 * @brief Get pointer to Application tranmit buffer. If return NULL is a job busy and
 * current action should be cancel.
 * @param completedFunc function-pointer to to return status on job.
 * @return pointer to tranmit-buffer. NULL is it not free.
 */
ZW_APPLICATION_TX_BUFFER*
GetRequestBuffer( VOID_CALLBACKFUNC(completedFunc)(TRANSMISSION_RESULT * pTransmissionResult) );

/**
 * @brief Free transmit buffer by clear mutex and remove callback. This should be
 * called if ZW_SendData() return FALSE.
 */
void
FreeRequestBuffer(void);

/**
 * @brief This function must be used a Call-back status function for GetAppCmdFramePointer
 * when calling Z-Wave API ZW_SendData().
 * @param pTransmissionResult Transmission result.
 */
void
ZCB_RequestJobStatus(TRANSMISSION_RESULT * pTransmissionResult);

/**
 * @brief Get transmit buffer for response job. Return NULL if buffer is busy.
 * @return pointer to ZW_APPLICATION_TX_BUFFER. Return NULL if frame is occupeid or
 * job is not legal.
 */
ZW_APPLICATION_TX_BUFFER*
GetResponseBuffer(void);

/**
 * @brief Get transmit buffer for response job. Return NULL if buffer is busy.
 * @param completedFunc function-pointer to to return status on job.
 * @return pointer to ZW_APPLICATION_TX_BUFFER. Return NULL if frame is occupied or
 * job is not legal.
 */
ZW_APPLICATION_TX_BUFFER*
GetResponseBufferCb(VOID_CALLBACKFUNC(completedFunc)(BYTE));

/**
 * @brief Frees the response buffer. Must be used as callback on transmissions using the response
 * buffer.
 * @param txStatus Transmission status.
 */
void
ZCB_ResponseJobStatus(BYTE txStatus);

/**
 * @brief Free response TX-Buffer clear mutex and remove callback.
 */
void
FreeResponseBuffer(void);

/**
 * @brief Returns whether any of the mutexes are occupied.
 * @return TRUE if one or more mutexes are occupied, otherwise FALSE.
 */
BOOL
ZAF_mutex_isActive(void);

/**
 * @brief Set payload length in Supervision cmd.
 * @param[in] pPayload pointer to request buffer payload.
 * @param[in] payLoadlen add Supervision command.
 * @return boolean if ppPayload is legal.
 */
BOOL
RequestBufferSetPayloadLength(ZW_APPLICATION_TX_BUFFER* pPayload,  BYTE payLoadlen);

/**
 * @brief Get Requst buffer payload and payload length.
 * @param[in,out] ppPayload Pointer to payload. Pointer is changed to point on payload!
 * @param[in,out] pPayLoadlen Length of payload.
 * @param[in] supervision Specifies whether Supervision should be activated.
 * @return boolean if ppPayload is legal.
 */
BOOL
RequestBufferSupervisionPayloadActivate(
    ZW_APPLICATION_TX_BUFFER** ppPayload,
    BYTE* pPayLoadlen,
    BOOL supervision);

#endif /* _ZW_MUTEX_H_ */


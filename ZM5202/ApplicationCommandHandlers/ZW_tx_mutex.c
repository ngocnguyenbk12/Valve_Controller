/**
 * @file
 * Transmit mutex.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include <ZW_tx_mutex.h>
#include <ZW_uart_api.h>
#include <ZW_transport_api.h>
#include <ZW_mem_api.h>
#include <CommandClassSupervision.h>
#include <misc.h>
#include <ZW_stdint.h>

#ifdef ZW_DEBUG_MUTEX
#define ZW_DEBUG_MUTEX_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_MUTEX_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_MUTEX_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_MUTEX_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_MUTEX_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_MUTEX_SEND_BYTE(data)
#define ZW_DEBUG_MUTEX_SEND_STR(STR)
#define ZW_DEBUG_MUTEX_SEND_NUM(data)
#define ZW_DEBUG_MUTEX_SEND_WORD_NUM(data)
#define ZW_DEBUG_MUTEX_SEND_NL()
#endif

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

typedef struct _MUTEX
{
  BYTE mutexAppActive;
  BYTE mutexResponseActive;
  VOID_CALLBACKFUNC(pAppJob)(TRANSMISSION_RESULT * pTransmissionResult);
  VOID_CALLBACKFUNC(pResponseJob)(BYTE);
  REQ_BUF reqTxBuf;
  ZW_APPLICATION_TX_BUFFER responseTxBuf;
} MUTEX;

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

static MUTEX myMutex;

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

// Nothing here.

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/
static BOOL MutexSet(BYTE* pMutex);
static void MutexFree(BYTE* pMutex);

void
mutex_init(void)
{
  memset((BYTE *)&myMutex, 0x00, sizeof(myMutex));
}

ZW_APPLICATION_TX_BUFFER*
GetRequestBuffer( VOID_CALLBACKFUNC(completedFunc)(TRANSMISSION_RESULT * pTransmissionResult) )
{
  /*Set mutex*/
  if(FALSE == MutexSet(&(myMutex.mutexAppActive)))
  {
    /*Mutex is not free.. stop current job*/
    return NULL;
  }
  myMutex.pAppJob = completedFunc;
  CommandClassSupervisionGetAdd(&(myMutex.reqTxBuf.supervisionGet));

  return &(myMutex.reqTxBuf.appTxBuf);
}

BOOL
RequestBufferSetPayloadLength(ZW_APPLICATION_TX_BUFFER* pPayload,  BYTE payLoadlen)
{
  if(pPayload == &(myMutex.reqTxBuf.appTxBuf))
  {
    CommandClassSupervisionGetSetPayloadLength(&myMutex.reqTxBuf.supervisionGet, payLoadlen);
    return TRUE;
  }
  return FALSE;
}

BOOL
RequestBufferSupervisionPayloadActivate(
    ZW_APPLICATION_TX_BUFFER** ppPayload,
    BYTE* pPayLoadlen,
    BOOL supervision)
{
  if(TRUE == myMutex.mutexAppActive)
  {
    /*Rewrite SV-cmd if CCmultichannel has written in payload*/
    CommandClassSupervisionGetWrite(&(myMutex.reqTxBuf.supervisionGet));
    *pPayLoadlen = CommandClassSupervisionGetGetPayloadLength(&(myMutex.reqTxBuf.supervisionGet));
    if(TRUE == supervision)
    {
      *ppPayload = (ZW_APPLICATION_TX_BUFFER*)&(myMutex.reqTxBuf.supervisionGet);
      *pPayLoadlen += sizeof(ZW_SUPERVISION_GET_FRAME);
    }
    else
    {
      *ppPayload = &(myMutex.reqTxBuf.appTxBuf);
    }
    return TRUE;
  }
  return FALSE;
}

PCB(ZCB_RequestJobStatus)(TRANSMISSION_RESULT * pTransmissionResult)
{
  ZW_DEBUG_MUTEX_SEND_NL();
  ZW_DEBUG_MUTEX_SEND_STR("ZCB_RequestJobStatus");
  if(NON_NULL( myMutex.pAppJob ))
  {
    myMutex.pAppJob(pTransmissionResult);
  }

  if (TRANSMISSION_RESULT_FINISHED == pTransmissionResult->isFinished)
  {
    ZW_DEBUG_MUTEX_SEND_STR(" _FREE!_");
    FreeRequestBuffer();
  }
}

void
FreeRequestBuffer(void)
{
  ZW_DEBUG_MUTEX_SEND_NL();
  ZW_DEBUG_MUTEX_SEND_STR("FreeRequestBuffer");
  /*Remove application func-callback. User should not be called any more*/
  myMutex.pAppJob = NULL;
  /*Free Mutex*/
  MutexFree(&(myMutex.mutexAppActive));
}

ZW_APPLICATION_TX_BUFFER*
GetResponseBuffer(void)
{
  return GetResponseBufferCb(NULL);
}

ZW_APPLICATION_TX_BUFFER*
GetResponseBufferCb(VOID_CALLBACKFUNC(completedFunc)(BYTE))
{
   ZW_DEBUG_MUTEX_SEND_STR("GetResponseBufferCb ");
   ZW_DEBUG_MUTEX_SEND_NUM((BYTE)completedFunc);
   ZW_DEBUG_MUTEX_SEND_NL();

  /*Set mutex*/
  if(FALSE == MutexSet(&(myMutex.mutexResponseActive)))
  {
    /*Mutex is not free.. stop current job*/
    ZW_DEBUG_MUTEX_SEND_STR("Mutex RES not free!");
    ZW_DEBUG_MUTEX_SEND_NL();
    return NULL;
  }
  myMutex.pResponseJob = completedFunc;
  ZW_DEBUG_MUTEX_SEND_STR("rMutexOn");
  ZW_DEBUG_MUTEX_SEND_NL();
  memset((BYTE*)&myMutex.responseTxBuf, 0, sizeof(ZW_APPLICATION_TX_BUFFER) );
  return &myMutex.responseTxBuf;
}

PCB(ZCB_ResponseJobStatus)(BYTE txStatus)
{
  VOID_CALLBACKFUNC(tmpfunc)(BYTE);
  ZW_DEBUG_MUTEX_SEND_STR("rMCB ");
  ZW_DEBUG_MUTEX_SEND_NL();
  /*Free Mutex*/
  tmpfunc = myMutex.pResponseJob;
  FreeResponseBuffer();
  if(NON_NULL(tmpfunc))
  {
    tmpfunc(txStatus);
  }
}

void
FreeResponseBuffer(void)
{
  ZW_DEBUG_MUTEX_SEND_STR("FreeResponseBuffer");
  ZW_DEBUG_MUTEX_SEND_NL();
  /*Remove application func-callback. User should not be called any more*/
  myMutex.pResponseJob = NULL;
  /*Free Mutex*/
  MutexFree(&myMutex.mutexResponseActive);
}

/**
 * @brief Set mutex if it is not active
 * @param[in,out] pMutex pointer to the mutex-flag that should be changed.
 * @return TRUE if mutex was set else FALSE for mutex was not free.
 */
static BOOL
MutexSet(BYTE* pMutex)
{
  if( FALSE == *pMutex)
  {
    *pMutex = TRUE;
    return TRUE;
  }
  return FALSE;
}

/**
 * @brief MutexFree
 * @param[out] pMutex pointer to the mutex-flag that should be changed.
 * Free mutex
 */
static void
MutexFree(BYTE* pMutex)
{
  ZW_DEBUG_MUTEX_SEND_STR("MutexFree ");
  ZW_DEBUG_MUTEX_SEND_NUM((BYTE)pMutex);
  ZW_DEBUG_MUTEX_SEND_NL();
  *pMutex = FALSE;
}

BOOL
ZAF_mutex_isActive(void)
{
  if((TRUE == myMutex.mutexAppActive) || (TRUE == myMutex.mutexResponseActive))
  {
    ZW_DEBUG_MUTEX_SEND_STR("Mux");
    return TRUE;
  }
  return FALSE;
}

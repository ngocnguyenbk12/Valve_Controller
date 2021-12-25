/**
 * @file ZW_util_queue.c
 * @brief Queue utility for making ring buffers, task lists, etc.
 *
 * @date 17/02/2015
 * @author Thomas Roll (TRO)
 * @author Christian Salmony Olsen (COLSEN)
 *
 * Last changed by: $Author: $
 * Revision:        $Revision: $
 * Last changed:    $Date: $
 */
/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include <ZW_uart_api.h>
#include <ZW_util_queue_api.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
#ifdef ZW_DEBUG_UTIL_QUEUE
#define ZW_DEBUG_UTIL_QUEUE_SEND_BYTE(data)     ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_UTIL_QUEUE_SEND_STR(STR)       ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_UTIL_QUEUE_SEND_NUM(data)      ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_UTIL_QUEUE_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_UTIL_QUEUE_SEND_NL()           ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_UTIL_QUEUE_SEND_BYTE(data)
#define ZW_DEBUG_UTIL_QUEUE_SEND_STR(STR)
#define ZW_DEBUG_UTIL_QUEUE_SEND_NUM(data)
#define ZW_DEBUG_UTIL_QUEUE_SEND_WORD_NUM(data)
#define ZW_DEBUG_UTIL_QUEUE_SEND_NL()
#endif

void ZW_util_queue_Init(QUEUE_T * pQueue, void * pBuffer, BYTE sizeOfElements, BYTE numberOfElements)
{
  pQueue->out = 0;
  pQueue->in = 0;
  pQueue->queueSize = numberOfElements;
  pQueue->count = 0;
  pQueue->elementType = sizeOfElements;
  pQueue->array = (BYTE*)pBuffer;
}

BOOL ZW_util_queue_Enqueue(QUEUE_T * pQueue, void * pElement)
{
  if (pQueue->count >= pQueue->queueSize)
  {
    return FALSE;
  }
  else
  {
    BYTE i;
    BYTE* pEl = pElement;

    for (i = 0; i < pQueue->elementType; i++)
    {
      pQueue->array[ (pQueue->in)*(pQueue->elementType) + i ] = *(pEl + i);
    }

    pQueue->in = (pQueue->in + 1) % pQueue->queueSize;
    pQueue->count = pQueue->count + 1;
  }
  return TRUE;
}

BOOL ZW_util_queue_Dequeue(QUEUE_T * pQueue, void * pElement)
{
  if (pQueue->count <= 0)
  {
    return FALSE;
  }
  else
  {
    BYTE i;
    BYTE* pEl = pElement;

    for (i = 0; i < pQueue->elementType; i++)
    {
      *(pEl + i) = pQueue->array[(pQueue->out) * (pQueue->elementType) + i];
    }

    pQueue->out = (pQueue->out + 1) % pQueue->queueSize;
    pQueue->count = pQueue->count - 1;
  }
  return TRUE;
}

uint8_t ZW_util_queue_GetItemCount(QUEUE_T * pQueue)
{
  return pQueue->count;
}

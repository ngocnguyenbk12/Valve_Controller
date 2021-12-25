/**
 * @file
 * Queue utility for making ring buffers, task lists, etc.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

#ifndef Z_WAVE_INCLUDE_ZW_UTIL_QUEUE_API_H_
#define Z_WAVE_INCLUDE_ZW_UTIL_QUEUE_API_H_

#include <ZW_stdint.h>

/****************************************************************************/
/*                      DEFINITIONS                                         */
/****************************************************************************/

/**
 * Element type used in \ref QUEUE_T.
 */
typedef uint8_t QElementType;

/**
 * In order to use the Queue module, a variable of this type must be declared before calling
 * \ref ZW_util_queue_Init.
 */
typedef struct
{
  uint8_t out;
  uint8_t in;
  uint8_t count;
  uint8_t queueSize;
  uint8_t elementType;
  QElementType* array;
}
QUEUE_T;

/**
 * @brief Initializes a given queue with a given buffer.
 * @param[in] pQueue Pointer to the queue to initialize.
 * @param[in] pBuffer Pointer to a buffer to initialize the queue with.
 * @param[in] sizeOfElements Size of elements in the given buffer.
 * @param[in] numberOfElements Number of elements in the given buffer.
 */
void ZW_util_queue_Init(QUEUE_T * pQueue, void * pBuffer, BYTE sizeOfElements, BYTE numberOfElements);

/**
 * @brief Enqueues a given element to a given queue.
 * @param[in] pQueue Pointer to a queue.
 * @param[in] pElement Pointer to an element.
 * @return TRUE if element was enqueued, FALSE otherwise.
 */
BOOL ZW_util_queue_Enqueue(QUEUE_T * pQueue, void * pElement);

/**
 * @brief Dequeues a given element from a given queue.
 * @param[in] pQueue Pointer to a queue.
 * @param[out] pElement Pointer to an element.
 * @return TRUE if element was dequeued, FALSE otherwise.
 */
BOOL ZW_util_queue_Dequeue(QUEUE_T * pQueue, void * pElement);

/**
 * @brief Get number of elements in queue.
 * @param[in] pQueue Pointer to a queue.
 * @return Number of queue elements.
 */
uint8_t ZW_util_queue_GetItemCount(QUEUE_T * pQueue);

#endif /* Z_WAVE_INCLUDE_ZW_UTIL_QUEUE_API_H_ */

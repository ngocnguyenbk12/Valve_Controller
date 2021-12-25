/**
 * @file
 * Generic interrupt driver making it possible to register a callback function on edge detection.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

#ifndef INTERRUPT_H_
#define INTERRUPT_H_

typedef enum
{
  ZW_EXT_INT0,
  ZW_EXT_INT1,
  ZW_NUMBER_OF_EXT_INT
}
ZW_EXT_INT_NAME_T;

/**
 * @brief Initializes the interrupt driver.
 * @param[in] id of type ZW_EXT_INT_NAME_T
 * @param[in,out] pEdgeCallbackLocal Pointer to function to call on an edge detection.
 * @return TRUE if initialized, FALSE otherwise.
 */
BOOL
InterruptDriverInit(ZW_EXT_INT_NAME_T id, VOID_CALLBACKFUNC(pEdgeCallbackLocal)(BYTE));

#endif /* INTERRUPT_H_ */

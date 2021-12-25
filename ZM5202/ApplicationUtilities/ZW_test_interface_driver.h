/**
 * @file
 * Test Interface Driver.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _ZW_TEST_INTERFACE_DRIVER_H_
#define _ZW_TEST_INTERFACE_DRIVER_H_


/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>

/*
 * DMA FUN
 */
#define DMA_BUFFER_SIZE (64)
#define DMA_BUFFER_SIZE_SINGLE (DMA_BUFFER_SIZE / 2)
/*
 * DMA FUN OVER
 */

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/
/* Break to debugger if assert fails */
#ifdef ZW_TEST_INTERFACE_DRIVER
#define ZDB_ASSERT(x, sigval) if(!(x)) zdb_handle_exception(sigval);
#else
#define ZDB_ASSERT(x, sigval)
#define ZW_test_interface_driver_init(x)
#define zdb_handle_exception(x)
#endif

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/
#ifdef ZW_TEST_INTERFACE_DRIVER
void ZW_test_interface_driver_init(void);
#endif

/**
 * @brief Copies received data to a buffer referenced by a given pointer.
 * @param[in] pData Pointer to a buffer which can store the data.
 * @param[in] pLength Pointer to a variable which can store the length of the
 * received data.
 * @return TRUE if byte is read out, FALSE otherwise.
 */
BOOL ZW_test_interface_driver_getData(BYTE * pData, BYTE * pLength);

#endif /* _ZW_TEST_INTERFACE_DRIVER_H_ */

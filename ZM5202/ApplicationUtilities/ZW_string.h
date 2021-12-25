/**
 * @file
 * String utilities.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _ZW_STRING_H_
#define _ZW_STRING_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/


/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/


/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

/** 
 * @brief ZW_strlen computes the length of a given string up to but not including the
 * terminating null character.
 * @param str Pointer to a string.
 * @return The length of the given string.
 */
BYTE
ZW_strlen(BYTE* str);

#endif /* _ZW_STRING_H_ */

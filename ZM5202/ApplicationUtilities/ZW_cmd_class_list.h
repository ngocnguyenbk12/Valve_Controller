/**
 * @file
 * Validation of command class against application NIFs.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _ZW_CMD_CLASS_LIST_H_
#define _ZW_CMD_CLASS_LIST_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_security_api.h>


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
 * Returns whether a given command class is found in a given list of command classes.
 * @param commandClass CC to be found.
 * @param pList List of CCs to search within.
 * @param len Length of the CC list.
 * @return Returns TRUE if the CC is present and FALSE otherwise.
 */
BOOL CheckCmdClass(BYTE commandClass,
                   BYTE *pList,
                   BYTE len);

/**
 * @brief CmdClassSupported
 * Check incoming frame command class is in secure- or nonsecure-list
 * @param[in] eKey The security key used
 * @param[in] commandClass The command class of the frame
 * @param[in] command The command of the frame
 * @param[in] pSecurelist secure list
 * @param[in] securelistLen secure list length
 * @param[in] pNonSecurelist nonsecure list
 * @param[in] nonSecurelistLen nonsecure list length
 * @return boolean if command class is in list.
 */
BOOL
CmdClassSupported(security_key_t eKey,
                  BYTE commandClass,
                  BYTE command,
                  BYTE* pSecurelist,
                  BYTE securelistLen,
                  BYTE* pNonSecurelist,
                  BYTE nonSecurelistLen);

#endif /* _ZW_CMD_CLASS_LIST_H_ */

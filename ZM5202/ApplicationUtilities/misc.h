/**
 * @file
 * Miscellaneous stuff.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _MISC_H_
#define _MISC_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
//#include <ZW_classcmd.h>
#include <ZW_transport_api.h>

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

/**
 * This define holds a default set of transmit options.
 */
#define ZWAVE_PLUS_TX_OPTIONS (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_EXPLORE)

/**
 * This define forces the function into the interbank call table. It is needed
 * for every function which will be passed as a function pointer.
 */
#ifdef __C51__
#define PCB(func) code const void (code * func ## _p)(void) = &func; \
 void func
#else
#define PCB(func) void func
#endif

#ifdef __C51__
#define PCB_BOOL(func) code const BOOL (code * func ## _p)(void) = &func; \
 BOOL func
#else
#define PCB_BOOL(func) BOOL func
#endif

#ifdef __C51__
#define PCB_UINT8_T(func) code const uint8_t (code * func ## _p)(void) = &func; \
 uint8_t func
#else
#define PCB_UINT8_T(func) uint8_t func
#endif

/**
 * This define forces the function into the interbank call table. It is needed
 * for every function which will be passed as a function pointer.
 * "Static" keyword is added.
 */
#ifdef __C51__
#define SPCB(func) code const void (code * func ## _p)(void) = &func; \
 static void func
#else
#define SPCB(func) static void func
#endif


/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/


/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/




/*=============================   GetMyNodeID  ===============================
**
**        Get the device node ID
**  Side effects: None
**
**------------------------------------------------------------------------------*/
extern BYTE GetMyNodeID(void);

#endif /*#ifndef _MISC_H_*/

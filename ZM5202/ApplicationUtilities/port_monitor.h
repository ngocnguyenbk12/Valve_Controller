/**
 * @file
 * ZW050x Port Pin service functions module.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _ZW_PORT_MONITOR_H_
#define _ZW_PORT_MONITOR_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_portpin_api.h>

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

/**
 * Number of port groups
 */
#define PORT_GRP 4

/**
 * PIO event module
 */
typedef enum
{
  ID_EVENT_PIO_PORT = 1
} ID_EVENT_MODULE;

/**
 * Port event
 */
typedef enum
{
  ID_PORTIN_CHANGE = 1, /*data struct PORT_STATUS*/
  ID_PORTOUT_CHANGE = 2 /*data struct PORT_STATUS*/
} PORT_EVENT;

/**
 * Port status
 */
typedef struct _PORT_STATUS
{
  BYTE status[PORT_GRP];
  BYTE mask[PORT_GRP];
} tPORT_STATUS;

/*==============================   ZW_PortInit     ===========================
** Function used to setup a function pointer to handle pin in events. Callback
** is per default disabled or can be disable by calling ZW_PortInit(NULL)
**    Side effects: None
**
**--------------------------------------------------------------------------*/
/**
 * @brief ZW_PortMonitorInit
 * Function used to setup a function pointer to handle pin in events. Callback
 * is per default disabled or can be disable by calling ZW_PortInit(NULL)
 * @param pEventHandler is function pointer to Event-Handler
 */
void ZW_PortMonitorInit( VOID_CALLBACKFUNC(pEventHandler)(WORD, XBYTE*, BYTE));


/**
 * @brief ZW_PortMonitorPinIn
 * Setup bPortPin portpin as Input
 * @param bPortPin of enum type ENUM_PORTPINS
 */
void ZW_PortMonitorPinIn(ENUM_PORTPINS bPortPin);


/**
 * @brief ZW_PortMonitorPinOut
 * Setup bPortPin portpin as Output
 * @param bPortPin of enum type ENUM_PORTPINS
 */
void ZW_PortMonitorPinOut(ENUM_PORTPINS bPortPin);

#endif /*_ZW_PORT_MONITOR_H_*/

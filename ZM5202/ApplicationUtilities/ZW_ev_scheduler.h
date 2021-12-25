/**
 * @file
 * Z-Wave event scheduler module.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _ZW_EV_SCHEDULER_H_
#define _ZW_EV_SCHEDULER_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/


/**
 * @brief EventSchedulerAdd
 * Add an function call be scheduled
 * @param pSchedApp function pointer to scedule-function
 * @return event handler of scedule-function. failing if 0
 */
BYTE EventSchedulerAdd( VOID_CALLBACKFUNC(pSchedApp)(void));


/**
 * @brief EventSchedulerRemove
 * Function to remove application call from scheduler
 * @param pHandle handle
 * @return booelan
 */
BOOL EventSchedulerRemove( BYTE* pHandle);


/**
 * @brief ZCB_EventScheduler
 * Scheduler engine. Add this to poll routine.
 */
void ZCB_EventScheduler(void);

#endif /*_ZW_EV_SCHEDULER_H_*/



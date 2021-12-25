/**
 * @file
 * Framework event manager, handling all event types for the framework
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _ev_man_H_
#define _ev_man_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_basis_api.h>

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

#ifndef EVENT_QUEUE_BUFFER_SIZE
/**
 * Size of event queue buffer
 */
#define EVENT_QUEUE_BUFFER_SIZE 5
#endif /* EVENT_QUEUE_BUFFER_SIZE*/

#define DEFINE_EVENT_WAKEUP ZW_WAKEUP_RESET
/**
 * Event groups for application state event machine
 */
#define DEFINE_EVENT_KEYPAD_NBR 0x10
#define DEFINE_EVENT_IR_NBR 0x20
#define DEFINE_EVENT_SYSTEM_NBR 0x30

/**
 * Event group for keys 0x40 - 0x6F
 */
#define DEFINE_EVENT_KEY_NBR 0x40
#define DEFINE_EVENT_APP_NBR 0x70


/**
 * System events
 */
typedef enum _EVENT_SYSTEM_
{
  EVENT_SYSTEM_RESET = DEFINE_EVENT_SYSTEM_NBR,
  EVENT_SYSTEM_LEARNMODE_START,
  EVENT_SYSTEM_LEARNMODE_FINISH,
  EVENT_SYSTEM_LEARNMODE_END,
  EVENT_SYSTEM_WATCHDOG_RESET,
  EVENT_SYSTEM_OTA_START,
  EVENT_SYSTEM_LEARNMODE_DSK_START
} EVENT_SYSTEM;


/**
 * Protocol wakeup events
 */
typedef enum _EVENT_WAKEUP_
{
  EVENT_WAKEUP_RESET = DEFINE_EVENT_WAKEUP, /**< Woken up by reset or external int */
  EVENT_WAKEUP_WUT,                     /**< Woken up by the WUT timer */
  EVENT_WAKEUP_SENSOR,                  /**< Woken up by a wakeup beam */
  EVENT_WAKEUP_WATCHDOG,                /**< Reset because of a watchdog timeout */
  EVENT_WAKEUP_EXT_INT,                 /**< Woken up by external interrupt */
  EVENT_WAKEUP_POR,                     /**< Reset by Power on reset circuit */
  EVENT_WAKEUP_USB_SUSPEND,             /**< Woken up by USB suspend */
  EVENT_WAKEUP_NVR_ERROR,               /**< Error in protocol NVR read */
  EVENT_WAKEUP_SMART_START,             /**< Woken up by time to start smart start */
  EVENT_WAKEUP_MAX                      /**< EVENT_WAKEUP_MAX define the last enum type */
} EVENT_WAKEUP;

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/


/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

#define EventSchedulerInit ZAF_eventSchedulerInit
/**
 * @brief Initializes event scheduler.
 */
void ZAF_eventSchedulerInit(VOID_CALLBACKFUNC(pApplicationStateMachine)(BYTE));


#define ZCB_EventSchedulerEventAdd ZCB_eventSchedulerEventAdd
/**
 * @brief Adds a given event to the event queue.
 * @param event A given event.
 * @return TRUE if given event is added to queue, FALSE if queue full.
 */
BOOL ZCB_eventSchedulerEventAdd(BYTE event);


#define ZCB_EventScheduler ZCB_eventScheduler
/**
 * Processes events by calling the function given by pointer in ZAF_eventSchedulerInit().
 * @return Returns TRUE if there are more events in the event queue after this call. FALSE
 * otherwise.
 */
BOOL ZCB_eventScheduler(void);

/**
 * Returns the number of events in the event queue.
 * @return Number of events in the event queue.
 */
uint8_t ZAF_EventScheduler_GetNumberOfEventsInQueue(void);


#define ZCB_EventEnqueue ZAF_jobEnqueue
/**
 * @brief Adds a given event to the job queue.
 * @param event A given event.
 * @return TRUE if given event is added to queue, FALSE if queue full.
 */
BOOL ZAF_jobEnqueue(BYTE event);


#define ZCB_EventDequeue ZAF_jobDequeue
/**
 * @brief Deque job queue
 * @param[out] pEvent return event from the queue.
 * @return FALSE if queue is empty else TRUE.
 */
BOOL ZAF_jobDequeue(BYTE* pEvent);


/**
 * @brief Get number of events on queue
 * @return number of events on queue
 */
uint8_t ZAF_jobQueueCount(void);

#endif /* _ev_man_H_ */



/**
 * @file
 * Timer service functions that handle delayed function calls.
 *
 * NOTICE: We recommend not using this timer module unless an application needs a lot of timers.
 * Use instead the TimerLong API found in Z-Wave/include/ZW_timer_api.h.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _APPL_TIMER_API_H_
#define _APPL_TIMER_API_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/


/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/
/**
 * The maximum number of application timers that can be created.
 */
#ifndef APPL_TIMER_MAX
#define APPL_TIMER_MAX   10
#endif /* APPL_TIMER_MAX*/


/**
 * Start an application timer that runs once.
 */
#define APPL_TIMER_ONETIME     0


/**
 * Start an application timer that runs forever.
 */
#define APPL_TIMER_FOREVER      (BYTE)-1


/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

/**
 * @brief ApplTimerGetTime
 * Get the time passed since the start of the application timer
 * It return the time (in seconds) passed since the timer is created or restarted
 * The timer is identified with the timer handle
 * @param[in] btimerHandle the application timer ID.
 * @return if it is less than 0xFFFFFFFF, then it is the number of seconds passed since the timer was
 *         created or restarted. If it equal 0xFFFFFFFF then the timer handle is invalid.
 */
DWORD ApplTimerGetTime(BYTE btimerHandle);

/**
 * @brief ApplTimerStart
 * Creates an application timer instance
 * @param[in] func Timeout function address.
 * @param[in] ltimerTicks Timeout value (in seconds).
 * @param[in] brepeats Number of function calls if function func before the timer stop (-1: forever).
 * @return Returns timer handle ID creatd (1 to APPL_TIMER_MAX), 0 if no timer is created.
 */
BYTE ApplTimerStart( VOID_CALLBACKFUNC(func)(void), DWORD ltimerTicks, BYTE brepeats);


/**
 * @brief ApplTimerRestart
 * Set the specified timer back to the initial value.
 * @param[in] timerHandle The timer ID.
 * @return Returns TRUE if timer restarted, FALSE if timer ID is invalid or timer is not running
 */
BYTE ApplTimerRestart( BYTE timerHandle);

/**
 * @brief ApplTimerStop
 * Stop the specified timer. Set the timerHandle to 0
 * @param[in] pbTimerHandle pointer to the timer Handle.
 * @return none.
 */
void ApplTimerStop( BYTE *pbTimerHandle);

/**
 * @brief Initalize the application timer subsystem. Must be called once.
 * @return TRUE if application timer system initaliazed correctly else FALSE.
 */
BOOL ApplTimerInit(void);

#endif /* _APPL_TIMER_API_H_ */

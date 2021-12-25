/**
 * @file
 * Task pool handler.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _ZW_TASK_H_
#define _ZW_TASK_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include <ZW_stdint.h>

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/
#ifndef TASK_POOL_SIZE
#define TASK_POOL_SIZE 5
#endif


/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/


/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/


/**
 * @brief Add task to task pool.
 * @param[in] CBPolltask function pointer to the task to poll.
 * @param[in] pTaskName pointer to a string descriping the Task name.
 * @return task handle ID or 0 if task-pool is full. Increase TASK_POOL_SIZE
 * if pool is full.
 */
uint8_t TaskAdd(BOOL (CODE *CBPolltask)(void), const char * pTaskName);

/**
 * @brief Pause the task.
 * @param[in] taskhandeId task handle ID
 * @return FALSE if taskhandeId is unknown else TRUE.
 */
BOOL TaskPause(uint8_t taskhandeId);

/**
 * @brief Run the paused task. If running do nothing.
 * @param[in] taskhandeId task handle ID
 * @return FALSE if taskhandeId is unknown else TRUE.
 */
BOOL TaskRun(uint8_t taskhandeId);


/**
 * @brief Remove the task from pool.
 * @param[in] taskhandeId task handle ID
 * @return FALSE if taskhandeId is unknown else TRUE.
 */
BOOL TaskRemove(uint8_t taskhandeId);


/**
 * @brief Interrupt process signal to the Task Handler to run all task in pool
 * to check for new jobs.
 */
void TaskInterruptSignal(void);

/**
 * @brief Task handler main poll queue.
 * @return FALSE if pool tasks has no jobs or no Interrupt has occur.
 * TRUE if one or more task has job.
 */
BOOL TaskApplicationPoll(void);


/**
 * @brief TaskJobHasWork
 * Ask task handler if more task job to run. If not go to sleep.
 * @return FALSE if pool tasks has no jobs or no Interrupt has occur.
 * TRUE if one or more task has job.
 */
BOOL TaskJobHasWork();


#endif /* _ZW_TASK_H_ */



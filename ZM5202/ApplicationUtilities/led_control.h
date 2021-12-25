/**
 * @file
 * Implements board support functions that control the LEDs on the ZDP03A development board.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _LED_CONTROL_H_
#define _LED_CONTROL_H_


/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/


/****************************************************************************/
/*                              EXTERNAL DEFINED FUNCTIONS/DATA             */
/****************************************************************************/

extern BYTE zm4102_mode_enable;

/**
 * @brief This function turns on the specified LED
 * @param Led LED to turn on
 */
void LedOn(BYTE Led);


/**
 * @brief This function turns off the specified LED.
 * @param Led LED to turn on
 * @return description..
 */
void LedOff(BYTE Led);


/**
 * @brief This function toggles the specified LED.
 * @param Led LED to toggle
 * @return description..
 */
void LedToggle(BYTE Led);

/**
 * @brief This function initializes the LED I/O.
 */
void LedControlInit(void);

#endif /*_LED_CONTROL_H_*/

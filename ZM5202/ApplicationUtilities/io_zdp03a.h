/**
 * @file
 * A helper module which eases implementation of an application on the ZDP03A development board.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _IO_ZDP03A_H_
#define _IO_ZDP03A_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include <ev_man.h>
#include <key_driver.h>
/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

#define ZDP03A_KEY_INTP  0x11// P1.1

/**
 * Key manager events
 */
typedef enum _EVENT_KEY_
{
  EVENT_KEY_B1_DOWN = DEFINE_EVENT_KEY_NBR,
  EVENT_KEY_B1_UP,
  EVENT_KEY_B1_PRESS,
  EVENT_KEY_B1_HELD,
  EVENT_KEY_B1_TRIPLE_PRESS,
  EVENT_KEY_B1_HELD_10_SEC,
  EVENT_KEY_B2_DOWN,
  EVENT_KEY_B2_UP,
  EVENT_KEY_B2_PRESS,
  EVENT_KEY_B2_HELD,
  EVENT_KEY_B3_DOWN,
  EVENT_KEY_B3_UP,
  EVENT_KEY_B3_PRESS,
  EVENT_KEY_B3_HELD,
  EVENT_KEY_B4_DOWN,
  EVENT_KEY_B4_UP,
  EVENT_KEY_B4_PRESS,
  EVENT_KEY_B4_HELD,
  EVENT_KEY_B5_DOWN,
  EVENT_KEY_B5_UP,
  EVENT_KEY_B5_PRESS,
  EVENT_KEY_B5_HELD,
  EVENT_KEY_B6_DOWN,
  EVENT_KEY_B6_UP,
  EVENT_KEY_B6_PRESS,
  EVENT_KEY_B6_HELD,
  EVENT_KEY_MAX /**< EVENT_KEY_MAX define the last enum type*/
} EVENT_KEY;



/**
 * ZDP03A key port enum types
 */
typedef enum _ZDP03A_KEY_
{
  ZDP03A_KEY_1 = 0x24,
  ZDP03A_KEY_2 = 0x36,
  ZDP03A_KEY_3 = 0x23, // Do not use this key because it shares pin with SPI for NVM.
  ZDP03A_KEY_4 = 0x22,
  ZDP03A_KEY_5 = 0x21, // This button is used by the Test Interface as well.
  ZDP03A_KEY_6 = 0x34
} ZDP03A_KEY;


/**
 * ZDP03A Led port enum types
 */
typedef enum _LED_OUT_
{
  ZDP03A_LED_D1 = 0x07, // P0.7
  ZDP03A_LED_D2 = 0x37, // P3.7
  ZDP03A_LED_D3 = 0x10, // P1.0
  ZDP03A_LED_D4 = 0x12, // P1.2
  ZDP03A_LED_D5 = 0x14, // P1.4
  ZDP03A_LED_D6 = 0x15, // P1.5
  ZDP03A_LED_D7 = 0x16, // P1.6
  ZDP03A_LED_D8 = 0x17  // P1.7
} LED_OUT;


typedef enum _LED_ACTION_ { ON = 0, OFF } LED_ACTION;
/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/


/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

/**
 * @brief Initialization of ZDP03A IO drivers. This function must be called
 *        before setting op IO ports (SetPinIn(...) and SetPinOut(...) )
 * @param pEventKeyQueue is function pointer to application key-event queue or
 *        direct to application state machine
 */
void ZDP03A_InitHW(
  VOID_CALLBACKFUNC(pEventKeyQueue)(BYTE));


/**
 * @brief Setup input port for a button
 * @param[in] key  number
 * @param[in] pullUp define if port is active high/low
 */
void SetPinIn( ZDP03A_KEY key, BOOL pullUp);


/**
 * @brief Setup output port for a LED
 * @param[in] led number
 */
void SetPinOut( LED_OUT led);

/**
 * @brief Set led on/off
 * @param[in] led number
 * @param[in] action on/off of type LED_ACTION
 */
void Led( LED_OUT led, LED_ACTION action);

/**
 * @brief Get key state
 * @param[in] key port
 * @return high (1) or low (0)
 */
BYTE KeyGet(ZDP03A_KEY key);

void ti_csa_prompt(void);

#endif /* _IO_ZDP03A_H_ */


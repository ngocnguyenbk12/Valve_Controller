/**
 * @file
 * This key driver offers an interface for easily setting up key actions.
 *
 * The driver assumes a hardware design similar to the one on ZDP03A.
 *
 * Here's what this module does and does not:
 * - It does configure a given set of keys to trigger on certain events.
 * - It does offer the opportunity to add further keys than the six represented
 *   on the Z-Wave ZDP03A development platform.
 * - It does offer the opportunity to add further key events if you're willing
 *   to code a little.
 * - It does NOT trigger on several keys simultaneously due to the chip and
 *   hardware design.
 *
 * Ideas for future version:
 * - Configure repeat/non-repeat for hold event.
 * - Configure hold time.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

#ifndef KEY_DRIVER_H_
#define KEY_DRIVER_H_

#include <ZW_typedefs.h>

/**
 * Holds the key names and the max number of keys. Add more keys before
 * NUMBER_OF_KEYS, if necessary.
 */
typedef enum
{
  KEY01,         //!< KEY01
  KEY02,         //!< KEY02
  KEY03,         //!< KEY03
  KEY04,         //!< KEY04
  KEY05,         //!< KEY05
  KEY06,         //!< KEY06
  NUMBER_OF_KEYS,//!< NUMBER_OF_KEYS
  WHICH_BUTTON_RESET = 0xFF
}
KEY_NAME_T;

/**
 * Holds the key events and the number of events. Add more events before
 * NUMBER_OF_KEY_EVENTS, if necessary.
 */
typedef enum
{
  KEY_DOWN,           //!< KEY_DOWN
  KEY_UP,             //!< KEY_UP
  KEY_PRESS,          //!< KEY_PRESS
  KEY_HOLD,           //!< KEY_HOLD
  KEY_TRIPLE_PRESS,   //!< KEY_TRIPLE_PRESS
  NUMBER_OF_KEY_EVENTS//!< NUMBER_OF_KEY_EVENTS
}
KEY_EVENT_T;

/**
 * Holds the key events and the number of events. Add more events before
 * NUMBER_OF_KEY_EVENTS, if necessary.
 */
typedef enum
{
  BITFIELD_KEY_DOWN = 0x01,                  //!< KEY_DOWN
  BITFIELD_KEY_UP = (0x01 << 1),             //!< KEY_UP
  BITFIELD_KEY_PRESS = (0x01 << 2),          //!< KEY_PRESS
  BITFIELD_KEY_HOLD = (0x01 << 3),           //!< KEY_HOLD
  BITFIELD_KEY_TRIPLE_PRESS = (0x01 << 4),   //!< KEY_TRIPLE_PRESS
  BITFIELD_NUMBER_OF_KEY_EVENTS = 5          //!< NUMBER_OF_KEY_EVENTS
}
BITFIELD_KEY_EVENT_T;


/**
 * @brief Register a callback function for a certain key event.
 * @param keyName   The name of the key in correspondence to the enumeration.
 * @param keyPin    Physical pin of the chip.
 * @param keyEvents  Key event in correspondence to the enumeration.
 * @param pCallback Pointer to callback function.
 */
void KeyDriverRegisterCallback(KEY_NAME_T keyName, BYTE keyPin, BYTE keyEvents, VOID_CALLBACKFUNC(pCallback)(KEY_NAME_T keyName, KEY_EVENT_T keyEvent, BYTE holdCount));

/**
 * @brief Initializes the key driver.
 */
BOOL KeyDriverInit(void);

#endif /* KEY_DRIVER_H_ */

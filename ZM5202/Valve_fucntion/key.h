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

#include <ZW_typedefs.h>

/**
 * Holds the key names and the max number of keys. Add more keys before
 * NUMBER_OF_KEYS, if necessary.
 */


/**
 * Holds the key events and the number of events. Add more events before
 * NUMBER_OF_KEY_EVENTS, if necessary.
 */
typedef enum
{
	KEY_HELD,
    KEY_IDLE,
  KEY_DOUBLE,
  KEY_PREDOUBLE,
	KEY_LEARNMODE,
	KEY_FACTORYNEW_RESET,
	KEY_READY_FOR_LEARNMODE,
	KEY_READY_FOR_FACTORYNEW_RESET,
	KEY_PRE_LEARNMODE,
	KEY_PRE_FACTORYNEW
}
P_KEY_EVENT_T;

 
//void KeyDriverRegisterCallback(KEY_NAME_T keyName, BYTE keyPin, BYTE keyEvents, VOID_CALLBACKFUNC(pCallback)(KEY_NAME_T keyName, KEY_EVENT_T keyEvent, BYTE holdCount));

/**
 * @brief Initializes the key driver.
 */
BOOL Valvekeyinit(void);


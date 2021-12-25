/**
 * @file key_driver.c
 * @brief This key driver offers an interface for easily setting up key actions.
 * @details The driver assumes a hardware design similar to the one on the
 * Z-Wave ZDP03A Development Platform.
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
 *
 * @date 10/12/2014
 * @author Christian Salmony Olsen (COLSEN)
 *
 * Last changed by: $Author: $
 * Revision:        $Revision: $
 * Last changed:    $Date: $
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW050x.h>
#include <ZW_typedefs.h>
#include <ZW_uart_api.h>
#include <ZW_timer_api.h>
#include <misc.h>
#include <interrupt_driver.h>
#include <key_driver.h>
#include <gpio_driver.h>
#include <ZAF_pm.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
#ifdef ZW_DEBUG_KEY_DRIVER
#define ZW_DEBUG_KEY_DRIVER_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_KEY_DRIVER_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_KEY_DRIVER_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_KEY_DRIVER_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_KEY_DRIVER_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_KEY_DRIVER_SEND_BYTE(data)
#define ZW_DEBUG_KEY_DRIVER_SEND_STR(STR)
#define ZW_DEBUG_KEY_DRIVER_SEND_NUM(data)
#define ZW_DEBUG_KEY_DRIVER_SEND_WORD_NUM(data)
#define ZW_DEBUG_KEY_DRIVER_SEND_NL()
#endif

#define KEY_HOLD_THRESHOLD          (10)  //100 msec called 10 times for battery control
#define KEY_TRIPLE_PRESS_THRESHOLD  (30) // 300 mSec
#define KEY_DRIVER_HOLD_TIMEOUT (11) // 110 mSec
#define KEY_TRIPLE_PRESS_TIMEOUT (20)
typedef struct
{
  BYTE keyPin;
  BYTE keyEvents;
  VOID_CALLBACKFUNC(pEventCallback)(KEY_NAME_T, KEY_EVENT_T, BYTE);
}
KEY_LIST_T;


/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/
static KEY_LIST_T keyList[NUMBER_OF_KEYS];
static KEY_NAME_T whichButton;
static BOOL fKeyHoldThresholdExceeded;
static BYTE pressCount;
static BYTE hold10Count;
static BYTE holdCount;
static BOOL m_keep_alive_timer_activated = FALSE;
static BYTE keyTriplePressTimerHandle = 0;
/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/
void ZCB_EdgeCall(BYTE bLevelValue);
static void GetActiveButton();
static BOOL isFunctionPointerSet(KEY_NAME_T whichButton, BITFIELD_KEY_EVENT_T keyEvent);
static void ClearActiveButton(void);
void ZCB_KeyHeldTimerCallback(void);
void ZCB_KeyTriplePressTimerCallback(void);

BOOL
KeyDriverInit(void)
{
#ifndef __C51__
  memset((BYTE *)keyList, 0, (NUMBER_OF_KEYS * sizeof(KEY_LIST_T)));
  keyTriplePressTimerHandle = 0;
#endif //__C51__
  whichButton = WHICH_BUTTON_RESET;
  fKeyHoldThresholdExceeded = FALSE;
  pressCount = 0;
  holdCount = 0;
  hold10Count = 0;
  m_keep_alive_timer_activated = FALSE;

#ifndef NON_BATT
  // TODO: Keep prel detection active under powering on.
  ZAF_pm_KeepAwake(50);
#endif
  /*
   * Initialize the interrupt module with a pointer to a function to call on
   * every edge change.
   */
  return InterruptDriverInit(ZW_EXT_INT1, ZCB_EdgeCall);
}

void KeyDriverRegisterCallback(KEY_NAME_T keyName, BYTE keyPin, BYTE keyEvents, VOID_CALLBACKFUNC(pCallback)(KEY_NAME_T keyName, KEY_EVENT_T keyEvent, BYTE holdCount))
{
  ZW_DEBUG_KEY_DRIVER_SEND_STR("KeyDriverRegisterCallback");
  ZW_DEBUG_KEY_DRIVER_SEND_NUM(keyName);
  ZW_DEBUG_KEY_DRIVER_SEND_NUM(keyPin);
  ZW_DEBUG_KEY_DRIVER_SEND_NUM(keyEvents);
  ZW_DEBUG_KEY_DRIVER_SEND_NL();
  if (NUMBER_OF_KEYS > keyName)
  {
    ZW_DEBUG_KEY_DRIVER_SEND_STR(" ->CONF ");
    ZW_DEBUG_KEY_DRIVER_SEND_NUM(keyEvents);
    ZW_DEBUG_KEY_DRIVER_SEND_NUM(keyName);
    ZW_DEBUG_KEY_DRIVER_SEND_NUM(keyPin);
    ZW_DEBUG_KEY_DRIVER_SEND_NL();
    keyList[keyName].keyPin = keyPin;
    keyList[keyName].keyEvents = keyEvents;
    keyList[keyName].pEventCallback = pCallback;

    /*
     * Enable ZAF_pm_KeepAwake() for battery devices if specific keys are used.
     */
    if(keyEvents & (BITFIELD_KEY_PRESS | BITFIELD_KEY_HOLD | BITFIELD_KEY_UP | BITFIELD_KEY_TRIPLE_PRESS))
    {
      ZW_DEBUG_KEY_DRIVER_SEND_STR("\r\nm_keep_alive_timer_activated\r\n ");
      m_keep_alive_timer_activated = TRUE;
    }
    ZW_DEBUG_KEY_DRIVER_SEND_NL();
  }
}

/**
 * @brief Called on every edge change.
 * @details This function defines the different events and calls registered
 * functions.
 * @param[in] bLevelValue 1 for high, 0 for low.
 */
PCB(ZCB_EdgeCall)(BYTE bLevelValue)
{
  static BYTE keyHeldTimerHandle = 0;

  if (0 == bLevelValue)
  {
    // Falling edge
#ifndef NON_BATT
    if(TRUE == m_keep_alive_timer_activated)
    {
      ZAF_pm_KeepAwake(KEY_DRIVER_HOLD_TIMEOUT);
    }
#endif

    /*
     * Detect button only on falling edge. It makes no sense to check on rising
     * edge due to the hardware design.
     */
    GetActiveButton();

    ZW_DEBUG_KEY_DRIVER_SEND_STR("S");
    ZW_DEBUG_KEY_DRIVER_SEND_NUM(whichButton);

    if (TRUE == isFunctionPointerSet(whichButton, BITFIELD_KEY_DOWN))
    {
      keyList[whichButton].pEventCallback(whichButton, KEY_DOWN, 0);
    }
    ZW_DEBUG_KEY_DRIVER_SEND_STR("DOWN");

    // Start a timer to detect whether the key is held.
    if (TRUE == isFunctionPointerSet(whichButton, BITFIELD_KEY_HOLD))
    {
      fKeyHoldThresholdExceeded = FALSE;
      hold10Count = 10;
      keyHeldTimerHandle = TimerStart(ZCB_KeyHeldTimerCallback, (KEY_HOLD_THRESHOLD), -1);
    }
    if (TRUE == isFunctionPointerSet(whichButton, BITFIELD_KEY_TRIPLE_PRESS))
	  {
      if (0 == keyTriplePressTimerHandle)
      {
        keyTriplePressTimerHandle = TimerStart(ZCB_KeyTriplePressTimerCallback, (KEY_TRIPLE_PRESS_THRESHOLD), 1);
      }
      else
      {
        // Restart the timer for timeout pressCount.
        TimerRestart(keyTriplePressTimerHandle);
      }
      pressCount++;
    }
    else{
      pressCount = 1;
    }
  }
  else
  {
    // Rising edge
    // When key goes up, cancel the held timer.
    TimerCancel(keyHeldTimerHandle);
    keyHeldTimerHandle = 0;

    if (TRUE == isFunctionPointerSet(whichButton, BITFIELD_KEY_TRIPLE_PRESS))
    {
#ifndef NON_BATT
      if(TRUE == m_keep_alive_timer_activated)
      {
        ZAF_pm_KeepAwake(KEY_TRIPLE_PRESS_TIMEOUT);
      }
#endif
      // Key released, clear triple-press time-out
      if(0 != keyTriplePressTimerHandle)
      {
        if(3 == pressCount)
        {
          // Stop the timer, it is a triple press!
          if(0 != keyTriplePressTimerHandle)
          {
            TimerCancel(keyTriplePressTimerHandle);
            keyTriplePressTimerHandle = 0;
          }
        }
        else
        {
          // Restart the timer for timeout pressCount. Timer need to be started on rising edge the check triplepress release time.
          // Only start the timer if it is not time-out. If time-out it is a KEY_PRESS
          if(0 != keyTriplePressTimerHandle)
          {
            TimerRestart(keyTriplePressTimerHandle);
          }
        }
      }
    }

    if (TRUE == isFunctionPointerSet(whichButton, BITFIELD_KEY_UP))
    {
      keyList[whichButton].pEventCallback(whichButton, KEY_UP, 0);
    }


    if (FALSE == fKeyHoldThresholdExceeded)
    {
      // The key was pressed.
      ZW_DEBUG_KEY_DRIVER_SEND_STR("PRESS");

      //** Check timeout for triple-press (pressCount = 0). if ->send a key-press else do nothing
      if ((TRUE == isFunctionPointerSet(whichButton, BITFIELD_KEY_PRESS)) && (1 == pressCount))
      {
        keyList[whichButton].pEventCallback(whichButton, KEY_PRESS, 0);
      }
      if (3 == pressCount)
      {
        pressCount = 0;

        ZW_DEBUG_SEND_STR("TRIPLE");

        if (TRUE == isFunctionPointerSet(whichButton, BITFIELD_KEY_TRIPLE_PRESS))
        {
          keyList[whichButton].pEventCallback(whichButton, KEY_TRIPLE_PRESS, 0);
        }
      }
    }
    else
    {
      // The key was held.
      holdCount = 0;
    }
    ZW_DEBUG_KEY_DRIVER_SEND_STR("UP");
    ClearActiveButton();
  }
}

static BOOL
isFunctionPointerSet(KEY_NAME_T whichButton, BITFIELD_KEY_EVENT_T keyEvent)
{
  if (NUMBER_OF_KEYS > whichButton)
  {
    ZW_DEBUG_KEY_DRIVER_SEND_NL();
    ZW_DEBUG_KEY_DRIVER_SEND_STR("Key exists");
    if (NON_NULL(keyList[whichButton].pEventCallback) && (keyList[whichButton].keyEvents & keyEvent))
    {
      ZW_DEBUG_KEY_DRIVER_SEND_NL();
      ZW_DEBUG_KEY_DRIVER_SEND_STR("Func is set");
      return TRUE;
    }
  }
  return FALSE;
}

/**
 * @brief Detects the active button.
 * @details We're making the assumption that no key will never be pressed at
 * the exact same time (by a human).
 */
static void
GetActiveButton(void)
{
  BYTE keyIndex;
  BYTE pinState;
  BOOL pinResult;

  for (keyIndex = 0; keyIndex < NUMBER_OF_KEYS; keyIndex++)
  {
    ZW_DEBUG_KEY_DRIVER_SEND_NL();
    ZW_DEBUG_KEY_DRIVER_SEND_STR("Key: ");
    ZW_DEBUG_KEY_DRIVER_SEND_NUM(keyIndex);
    ZW_DEBUG_KEY_DRIVER_SEND_STR(" Pin: ");
    ZW_DEBUG_KEY_DRIVER_SEND_NUM(keyList[keyIndex].keyPin);
    if(0 != keyList[keyIndex].keyPin)
    {
      pinResult = gpio_GetPinBool(keyList[keyIndex].keyPin, &pinState);

      if (TRUE == pinResult && !pinState)
      {
        ZW_DEBUG_KEY_DRIVER_SEND_BYTE('x');
        whichButton = keyIndex;
        return;
      }
    }
  }
  ZW_DEBUG_KEY_DRIVER_SEND_NL();
  ZW_DEBUG_KEY_DRIVER_SEND_STR("Couldn't find pin");
}

/**
 * @brief Clears the active button.
 */
static void
ClearActiveButton(void)
{
  whichButton = WHICH_BUTTON_RESET;
}

/**
 * @brief Called when the key held threshold is exceeded.
 */
PCB(ZCB_KeyHeldTimerCallback)(void)
{
#ifndef NON_BATT
  if(TRUE == m_keep_alive_timer_activated)
  {
    ZAF_pm_KeepAwake(KEY_DRIVER_HOLD_TIMEOUT);
  }
#endif

  if(0 == hold10Count)
  {
    // The key was held.
    ZW_DEBUG_KEY_DRIVER_SEND_STR("HELD");
    fKeyHoldThresholdExceeded = TRUE;

    if (NON_NULL(keyList[whichButton].pEventCallback) && (keyList[whichButton].keyEvents & BITFIELD_KEY_HOLD))
    {
      holdCount++;
      keyList[whichButton].pEventCallback(whichButton, KEY_HOLD, holdCount);
    }
     hold10Count = 10;
  }
  else{
    hold10Count--;
  }
}

/**
 * @brief Resets triple press counter.
 */
PCB(ZCB_KeyTriplePressTimerCallback)(void)
{
  keyTriplePressTimerHandle = 0;
  pressCount = 0;
  ZW_DEBUG_KEY_DRIVER_SEND_STR("KEY RESET");
}

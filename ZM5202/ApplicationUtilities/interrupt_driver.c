/**
 * @file interrupt_driver.c
 * @brief Generic interrupt module making it possible to register a callback
 * function on edge detection.
 *
 * @copyright Copyright (c) 2001-2015
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 * @details
 * NOTICE: A source file containing an interrupt function can never have the
 * name "interrupt.c".
 *
 * Last changed by: $Author: $
 * Revision:        $Revision: $
 * Last changed:    $Date: $
 *
 * @date 03/12/2014
 * @author Christian Salmony Olsen (COLSEN)
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include <INTRINS.h>
#include <ZW050x.h>
#include <ZW_pindefs.h>
#include <ZW_evaldefs.h>
#include <ZW_uart_api.h>
#include <ZW_basis_api.h>
#include <ZW_timer_api.h>
#include <misc.h>
#include <gpio_driver.h>
#include <interrupt_driver.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
/**
 * @def ZW_DEBUG_INTERRUPT_SEND_BYTE(data)
 * Transmits a given byte to the debug port.
 * @def ZW_DEBUG_INTERRUPT_SEND_STR(STR)
 * Transmits a given string to the debug port.
 * @def ZW_DEBUG_INTERRUPT_SEND_NUM(data)
 * Transmits a given number to the debug port.
 * @def ZW_DEBUG_INTERRUPT_SEND_WORD_NUM(data)
 * Transmits a given WORD number to the debug port.
 * @def ZW_DEBUG_INTERRUPT_SEND_NL()
 * Transmits a newline to the debug port.
 */
#ifdef ZW_DEBUG_INTERRUPT
#define ZW_DEBUG_INTERRUPT_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_INTERRUPT_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_INTERRUPT_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_INTERRUPT_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_INTERRUPT_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_INTERRUPT_SEND_BYTE(data)
#define ZW_DEBUG_INTERRUPT_SEND_STR(STR)
#define ZW_DEBUG_INTERRUPT_SEND_NUM(data)
#define ZW_DEBUG_INTERRUPT_SEND_WORD_NUM(data)
#define ZW_DEBUG_INTERRUPT_SEND_NL()
#endif

#define INTERRUPT_CALLBACK_PERIOD (10) // In milliseconds.

#define DISABLE_INTERRUPT_(id) {EX##id = 0;}
#define ENABLE_INTERRUPT_(id)  {EX##id = 1;}

/**
 * The fIgnoreInterrupt variable is set to avoid the first interrupt which
 * triggers when setting the enable bit.
 */
typedef struct
{
  BYTE pin;                               /*!< Pin represented as expected by ZW_io_handler.  */
  BYTE fTrigger;                          /*!< Set upon a real interrupt. */
  BYTE fIgnoreInterrupt;                  /*!< See description of this struct. */
  VOID_CALLBACKFUNC(pEdgeCallback)(BYTE); /*!< Callback function to call upon detected edge. */
  BYTE fLevel;                            /*!< Expected interrupt level. */
} EXT_INT_T;

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

/**
 * This variable holds data for each of the external interrupts.
 */
static volatile EXT_INT_T ext_interrupts[ZW_NUMBER_OF_EXT_INT] = {
        {0x10, FALSE, FALSE, NULL, FALSE},
        {0x11, FALSE, FALSE, NULL, FALSE}
};

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/
static BOOL DetectEdge(ZW_EXT_INT_NAME_T id, BYTE * pDetectedEdge);
void ZCB_InterruptCallback(void);
static void enable_interrupt(ZW_EXT_INT_NAME_T id);

/**
 * @brief Loops through all external interrupts to detect whether an edge
 * change happened or not.
 */
PCB(ZCB_InterruptCallback)(void)
{
  BYTE int_count;

  /*
   * This loop assumes that external interrupt pins are numbered from 0 to N-1
   * where N equals the max number of interrupts defined by
   * ZW_NUMBER_OF_EXT_INT.
   */
  for (int_count = 0; int_count < ZW_NUMBER_OF_EXT_INT; int_count++)
  {
    if (TRUE == ext_interrupts[int_count].fTrigger)
    {
      BYTE detectedEdge;
      ext_interrupts[int_count].fTrigger = FALSE;
      if ((TRUE == DetectEdge(int_count, &detectedEdge)) && (NULL != ext_interrupts[int_count].pEdgeCallback))
      {
        (ext_interrupts[int_count].pEdgeCallback)(detectedEdge);
      }
      ext_interrupts[int_count].fIgnoreInterrupt = TRUE;
      enable_interrupt(int_count);
    }
  }
}

/**
 * @brief Detects falling and rising edges on a given interrupt pin.
 * @details The 8051 variant which Z-Wave uses, only supports level interrupt
 * why this function must detect edge interrupts.
 * @param[in] id ID of the external interrupt.
 */
static BOOL
DetectEdge(ZW_EXT_INT_NAME_T id, BYTE * pDetectedEdge)
{
  BYTE fPinState;
  BOOL fStatus = FALSE;

  if (TRUE != gpio_GetPinBool(ext_interrupts[id].pin, &fPinState))
  {
    return fStatus;
  }

  if (ext_interrupts[id].fLevel == (fPinState))
  {
    // The pin is matching expected level => Pin has changed.
    *pDetectedEdge = ext_interrupts[id].fLevel;

    // Swap the level.
    if (FALSE == ext_interrupts[id].fLevel)
    {
      // Set expected value
      ext_interrupts[id].fLevel = TRUE;
      ZW_DEBUG_INTERRUPT_SEND_BYTE('D');
    }
    else
    {
      // Set expected value
      ext_interrupts[id].fLevel = FALSE;
      ZW_DEBUG_INTERRUPT_SEND_BYTE('U');
    }

    //ext_interrupts[id].fLevel = !(ext_interrupts[id].fLevel);

    ZW_SetExtIntLevel(id, ext_interrupts[id].fLevel);

    fStatus = TRUE;
  }
  return fStatus;
}


/**
 * @brief Interrupt Service Routine for external interrupt 0.
 */
void
ext0_isr(void) interrupt INUM_INT0
{
//  EA = 0; // Disable global interrupt
//  _push_(SFRPAGE);

  if (TRUE == ext_interrupts[ZW_EXT_INT0].fIgnoreInterrupt)
  {
    ext_interrupts[ZW_EXT_INT0].fIgnoreInterrupt = FALSE;
  }
  else
  {
    ext_interrupts[ZW_EXT_INT0].fTrigger = TRUE;
    DISABLE_INTERRUPT_(0);
  }

//  _pop_(SFRPAGE);
//  EA = 1; // Enable global interrupt
}


/**
 * @brief Interrupt Service Routine for external interrupt 1.
 */
void
ext1_isr(void) interrupt INUM_INT1
{
//  EA = 0; // Disable global interrupt
//  _push_(SFRPAGE);

  if (TRUE == ext_interrupts[ZW_EXT_INT1].fIgnoreInterrupt)
  {
    ext_interrupts[ZW_EXT_INT1].fIgnoreInterrupt = FALSE;
  }
  else
  {
    ext_interrupts[ZW_EXT_INT1].fTrigger = TRUE;
    DISABLE_INTERRUPT_(1);
  }
//  _pop_(SFRPAGE);
//  EA = 1; // Enable global interrupt
}


static void
enable_interrupt(ZW_EXT_INT_NAME_T id)
{
  switch (id)
  {
    case ZW_EXT_INT0:
      ENABLE_INTERRUPT_(0);
      break;

    case ZW_EXT_INT1:
      ENABLE_INTERRUPT_(1);
      break;

    default:
      // Do nothing.
      break;
  }
}

/****************************************************************************/
/*                            PUBLIC FUNCTIONS                              */
/****************************************************************************/

BOOL
InterruptDriverInit(ZW_EXT_INT_NAME_T id, VOID_CALLBACKFUNC(pEdgeCallbackLocal)(BYTE))
{
  static BOOL fInitialized = FALSE;
  gpio_SetPinIn(ext_interrupts[id].pin, TRUE);
  ZW_SetExtIntLevel(id, FALSE);
  enable_interrupt(id);

  ext_interrupts[id].pEdgeCallback = pEdgeCallbackLocal;

  /*
   * We don't want any functions to be called inside the ISR. Hence we start
   * a timer which runs forever.
   */
  if (FALSE == fInitialized)
  {
    fInitialized = TRUE;
    if (-1 == TimerStart(ZCB_InterruptCallback, (INTERRUPT_CALLBACK_PERIOD/10), (-1)))
    {
      return FALSE;
    }
  }

  return TRUE;
}

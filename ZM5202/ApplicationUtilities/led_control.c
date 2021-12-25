/***************************************************************************
*
* Copyright (c) 2001-2013
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: Implements board support functions that controls the LEDs
*              on the ZDP03A development board.
*
* Author: Samer
*
* Last Changed By: $Author: tro $
* Revision: $Revision: 0.00 $
* Last Changed: $Date: 2013/05/29 11:36:41 $
*
****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/



#include <ZW_basis_api.h>
#include <ZW_pindefs.h>
#include <ZW_evaldefs.h>
#include <led_control.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/


/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/


/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/


/*===================================   LedOn   =============================
**    This function turns on the specified LED.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
LedOn( BYTE Led )                       /* LED to turn on. */

{
  switch (Led)
  {
    case 1:
      LED_ON(1);
      break;
    case 2:
      LED_ON(2);
      break;
#ifndef ZM5202
    case 3:
      LED_ON(3);
      break;
    case 4:
      LED_ON(4);
      break;
#endif
    default:
      break;
  }
}

/*===================================   LedOff   ============================
**    This function turns off the specified LED.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
LedOff( BYTE Led )                       /* LED to turn on. */

{
  switch (Led)
  {
    case 1:
      LED_OFF(1);
      break;
    case 2:
      LED_OFF(2);
      break;
#ifndef ZM5202
    case 3:
      LED_OFF(3);
      break;
    case 4:
      LED_OFF(4);
      break;
#endif
    default:
      break;
  }
}

/*==================================   LedToggle   ==========================
**    This function toggles the specified LED.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
LedToggle( BYTE Led )                       /* LED to toggle. */
{
  switch (Led)
  {
    case 1:
      LED_TOGGLE(1);
      break;
    case 2:
      LED_TOGGLE(2);
      break;
#ifndef ZM5202
    case 3:
      LED_TOGGLE(3);
      break;
    case 4:
      LED_TOGGLE(4);
      break;
#endif      
    default:
      break;
  }
}

/*===============================   LedControlInit   ========================
**    This function initializes the LED I/O.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
LedControlInit(void)
{
  /* Setup specifics for current dim module */
  PIN_OUT(LED1);
  PIN_OUT(LED2);
#ifndef ZM5202
  PIN_OUT(LED3);
  PIN_OUT(LED4);
#endif
}

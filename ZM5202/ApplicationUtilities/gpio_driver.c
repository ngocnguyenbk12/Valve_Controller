/**
 * @file gpio_driver.c
 *
 * @copyright Copyright (c) 2001-2017
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 * @brief Offers functions for controlling GPIO.
 *
 * @details This driver includes pin swap because certain Z-Wave development
 * boards have pins which are swapped. This driver takes the pin swap into
 * account.
 *
 * The GPIO driver is dependent on config_pin.c which must be placed in the
 * application folder and implement an array of PIN_T type and the function
 * gpio_GetPinSwapList externally declared in the header file for the GPIO
 * driver.
 *
 * @date 08/05/2015
 * @author Thomas Roll
 * @author Christian Salmony Olsen
 */

/***************************************************************************/
/*                              INCLUDE FILES                              */
/***************************************************************************/

#include <ZW_pindefs.h>
#include <ZW_typedefs.h>
#include <ZW_uart_api.h>
#include <misc.h>
#include <ZW_nvr_api.h>
#include <gpio_driver.h>
#include <ZW_sysdefs.h>
#include <ZW050x.h>
/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

#ifdef ZW_DEBUG_GPIO_DRIVER
#define ZW_DEBUG_GPIO_DRIVER_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_GPIO_DRIVER_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_GPIO_DRIVER_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_GPIO_DRIVER_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_GPIO_DRIVER_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_GPIO_DRIVER_SEND_BYTE(data)
#define ZW_DEBUG_GPIO_DRIVER_SEND_STR(STR)
#define ZW_DEBUG_GPIO_DRIVER_SEND_NUM(data)
#define ZW_DEBUG_GPIO_DRIVER_SEND_WORD_NUM(data)
#define ZW_DEBUG_GPIO_DRIVER_SEND_NL()
#endif

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

static PIN_T_ARRAY xdata * pPinSwapList = NULL;
static BYTE pinListSize = 0;
static BOOL fDoPinSwap = FALSE;

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

static void searchPinList(BYTE * pPin);

static void
searchPinList(BYTE * pPin)
{
  BYTE count;

  for (count = 0; count < pinListSize; count++)
  {
    ZW_DEBUG_GPIO_DRIVER_SEND_NL();
    ZW_DEBUG_GPIO_DRIVER_SEND_STR("GPIO.searchPinList: pPin = ");
    ZW_DEBUG_GPIO_DRIVER_SEND_NUM(*pPin);
    ZW_DEBUG_GPIO_DRIVER_SEND_STR(", pPinSwapList[");
    ZW_DEBUG_GPIO_DRIVER_SEND_NUM(count);
    ZW_DEBUG_GPIO_DRIVER_SEND_STR("].pin = ");
    ZW_DEBUG_GPIO_DRIVER_SEND_NUM((*pPinSwapList)[count].pin);
    if ((*pPinSwapList)[count].pin == *pPin)
    {
      *pPin = (*pPinSwapList)[count].pinSwap;
      break;
    }
  }
}

/**
 * @brief Sets a given pin as input.
 * @param pin A given pin.
 * @param pullUp TRUE for pull up, FALSE if not.
 * @return TRUE if pin port exists, FALSE otherwise.
 */
static BOOL
PinIn( BYTE pin, BOOL pullUp)
{
  BYTE port = pin>>4;
  BYTE portPin = pin&0x0F;

  switch(port)
  {
    case 0:
      P0DIR_PAGE;
      (P0ShadowDIR |= (1 << portPin));
      (pullUp)?(P0Shadow &=~(1 << portPin)):(P0Shadow |= (1 << portPin));
      P0 = P0Shadow;P0DIR = P0ShadowDIR;
      break;
    case 1:
      P1DIR_PAGE;
      (P1ShadowDIR |= (1<<portPin));
      (pullUp)?(P1Shadow &=~(1 << portPin)):(P1Shadow |= (1 << portPin));
      P1 = P1Shadow;P1DIR = P1ShadowDIR;
      break;
    case 2:
      P2DIR_PAGE;
      (P2ShadowDIR |= (1<<portPin));
      (pullUp)?(P2Shadow &=~(1 << portPin)):(P2Shadow |= (1 << portPin));
      P2 = P2Shadow;P2DIR = P2ShadowDIR;
      break;
    case 3:
      P3DIR_PAGE;
      (P3ShadowDIR |= (1<<portPin));
      (pullUp)?(P3Shadow &=~(1 << portPin)):(P3Shadow |= (1 << portPin));
      P3 = P3Shadow;P3DIR = P3ShadowDIR;
      break;
    default: return FALSE;
  }
  return TRUE;
}

/**
 * @brief Sets pin as output.
 * @param pin A given pin.
 * @return TRUE if pin port exists, FALSE otherwise.
 */
static BOOL
PinOut(BYTE pin)
{
  BYTE port = pin>>4;
  BYTE portPin = pin&0x0F;

  switch(port)
  {
    case 0:
      P0DIR_PAGE;
      P0ShadowDIR &=~(1 << portPin);
      P0DIR = P0ShadowDIR;
      break;
    case 1:
      P1DIR_PAGE;
      P1ShadowDIR &=~(1 << portPin);
      P1DIR = P1ShadowDIR;
      break;
    case 2:
      P2DIR_PAGE;
      P2ShadowDIR &=~(1 << portPin);
      P2DIR = P2ShadowDIR;
      break;
    case 3:
      P3DIR_PAGE;
      P3ShadowDIR &=~(1 << portPin);
      P3DIR = P3ShadowDIR;
      break;
    default: return FALSE;
  }
  return TRUE;
}

/**
 * @brief Returns the state of a given pin as boolean.
 * @param pin A given pin.
 * @param pfState Pointer to result variable.
 * @return TRUE if pin port exists, FALSE otherwise.
 */
BOOL
gpio_GetPinBool(BYTE pin, BYTE * pfState)
{
  BYTE port = pin>>4;
  BYTE portPin = pin&0x0F;

  switch(port)
  {
    case 0:
      *pfState = (P0 & (1 << portPin)) ? TRUE : FALSE;
      break;
    case 1:
      *pfState = (P1 & (1 << portPin)) ? TRUE : FALSE;
      break;
    case 2:
      *pfState = (P2 & (1 << portPin)) ? TRUE : FALSE;
      break;
    case 3:
      *pfState = (P3 & (1 << portPin)) ? TRUE : FALSE;
      break;
    default:
      return FALSE;
      break;
  }
  return TRUE;
}

/**
 * @brief Sets a pin high.
 * @param pin A given pin.
 */
static void
PinOn(BYTE pin)
{
  BYTE port = pin>>4;
  BYTE portPin = pin&0x0F;

  switch(port)
  {
    case 0:
      P0Shadow |= (1 << portPin);
      P0 = P0Shadow;
      break;
    case 1:
      P1Shadow |= (1 << portPin);
      P1 = P1Shadow;
      break;
    case 2:
      P2Shadow |= (1 << portPin);
      P2 = P2Shadow;
      break;
    case 3:
      P3Shadow |= (1 << portPin);
      P3 = P3Shadow;
      break;
  }
  return;
}

/**
 * @brief Sets a pin low.
 * @param pin A given pin.
 */
static void
PinOff(BYTE pin)
{
  BYTE port = pin>>4;
  BYTE portPin = pin&0x0F;

  switch(port)
  {
    case 0:
      P0Shadow &= ~(1 << portPin);
      P0 = P0Shadow;
      break;
    case 1:
      P1Shadow &= ~(1 << portPin);
      P1 = P1Shadow;
      break;
    case 2:
      P2Shadow &= ~(1 << portPin);
      P2 = P2Shadow;
      break;
    case 3:
      P3Shadow &= ~(1 << portPin);
      P3 = P3Shadow;
      break;
  }
  return;
}

BOOL
gpio_DriverInit(BOOL automaticPinSwap)
{
  BYTE nvrPinSwapValue;

  ZW_DEBUG_GPIO_DRIVER_SEND_NL();
  ZW_DEBUG_GPIO_DRIVER_SEND_STR("GPIO init!");

  if (TRUE != automaticPinSwap)
  {
    ZW_DEBUG_GPIO_DRIVER_SEND_NL();
    ZW_DEBUG_GPIO_DRIVER_SEND_STR("GPIO: Exit!");
    return TRUE;
  }

  ZW_NVRGetValue(offsetof(NVR_FLASH_STRUCT, bPinSwap) , 1, &nvrPinSwapValue);

  if (1 == nvrPinSwapValue)
  {
    // The pins are swapped.
    ZW_DEBUG_GPIO_DRIVER_SEND_NL();
    ZW_DEBUG_GPIO_DRIVER_SEND_STR("GPIO: Pins swapped.");
    gpio_GetPinSwapList(&pPinSwapList, &pinListSize);
  }
  else if((0x02 <= nvrPinSwapValue) && (0xFE >= nvrPinSwapValue))
  {
    ZW_DEBUG_GPIO_DRIVER_SEND_NL();
    ZW_DEBUG_GPIO_DRIVER_SEND_STR("GPIO: Invalid value!");
    return FALSE;
  }

  ZW_DEBUG_GPIO_DRIVER_SEND_NL();
  ZW_DEBUG_GPIO_DRIVER_SEND_STR("GPIO: pinListSize = ");
  ZW_DEBUG_GPIO_DRIVER_SEND_NUM(pinListSize);
  ZW_DEBUG_GPIO_DRIVER_SEND_STR("GPIO: pPinSwapList = ");
  ZW_DEBUG_GPIO_DRIVER_SEND_WORD_NUM((WORD)pPinSwapList);
  if (pinListSize > 0 && NON_NULL(pPinSwapList))
  {
    ZW_DEBUG_GPIO_DRIVER_SEND_NL();
    ZW_DEBUG_GPIO_DRIVER_SEND_STR("GPIO: Do pin swap.");
    fDoPinSwap = TRUE;
  }

  return TRUE;
}

void
gpio_SetPinIn(BYTE pin, BOOL fPullUp)
{
  if (TRUE == fDoPinSwap)
  {
    searchPinList(&pin);
  }
  PinIn(pin, fPullUp);
}

void gpio_SetPinOut(BYTE pin)
{
  ZW_DEBUG_GPIO_DRIVER_SEND_NL();
  ZW_DEBUG_GPIO_DRIVER_SEND_STR("GPIO: SetPinOut = ");
  ZW_DEBUG_GPIO_DRIVER_SEND_NUM(pin);
  if (TRUE == fDoPinSwap)
  {
    searchPinList(&pin);
  }
  ZW_DEBUG_GPIO_DRIVER_SEND_STR(" swapped = ");
  ZW_DEBUG_GPIO_DRIVER_SEND_NUM(pin);
  PinOut(pin);
}

void gpio_SetPin(BYTE pin, BOOL fValue)
{
  if (TRUE == fDoPinSwap)
  {
    searchPinList(&pin);
  }
  ZW_DEBUG_GPIO_DRIVER_SEND_NL();
  ZW_DEBUG_GPIO_DRIVER_SEND_STR("GPIO: SetPin ");
  ZW_DEBUG_GPIO_DRIVER_SEND_NUM(pin);
  ZW_DEBUG_GPIO_DRIVER_SEND_STR(" = ");
  ZW_DEBUG_GPIO_DRIVER_SEND_NUM(fValue);
  if (TRUE == fValue)
  {
    PinOn(pin);
  }
  else
  {
    PinOff(pin);
  }
}

BOOL gpio_GetPin(BYTE pin)
{
  BYTE fPinState;
  if (TRUE == fDoPinSwap)
  {
    searchPinList(&pin);
  }
  gpio_GetPinBool(pin, &fPinState);
  return (BOOL)fPinState;
}

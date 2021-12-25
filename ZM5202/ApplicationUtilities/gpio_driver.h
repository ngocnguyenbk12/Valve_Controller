/**
 * @file
 * Offers functions for controlling GPIO.
 *
 * This driver includes pin swap because certain Z-Wave development
 * boards have pins which are swapped. This driver takes the pin swap into
 * account.
 *
 * The GPIO driver is dependent on function gpio_GetPinSwapList() which must be
 * placed in the application folder and implement an array of PIN_T type and
 * the function gpio_GetPinSwapList externally declared in the header file for
 * the GPIO driver. Function gpio_GetPinSwapList()is not called if function
 * gpio_driver_init() is initiated with automaticPinSwap = FALSE!
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _GPIO_DRIVER_H_
#define _GPIO_DRIVER_H_

#include <ZW_typedefs.h>

typedef struct
{
  BYTE pin;
  BYTE pinSwap;
} PIN_T;

typedef PIN_T PIN_T_ARRAY[];

/**
 * @brief Gets the pin list and its size from the application.
 * @param[out] pPinList Pointer to an array containing the swapped pins.
 * @param[out] pPinListSize Pointer to the size of the pin list.
 */
extern void
gpio_GetPinSwapList(PIN_T_ARRAY xdata ** pPinList, BYTE * const pPinListSize);

/**
 * @brief Initializes the GPIO driver by reading out pin swap value from NVM.
 * @param[in] automaticPinSwap Sets whether the GPIO driver should swap pins
 * automatically or not.
 * @return TRUE if initialized successfully, FALSE otherwise.
 */
BOOL
gpio_DriverInit(BOOL automaticPinSwap);

/**
 * @brief Sets a pin as input.
 * @param[in] pin A given pin.
 * @param[in] fPullUp set pin to intern pull high or low
 */
void gpio_SetPinIn(BYTE pin, BOOL fPullUp);

/**
 * @brief Sets a given pin as output.
 * @param[in] pin A given pin.
 */
void gpio_SetPinOut(BYTE pin);

/**
 * @brief Sets a given pin to a given state.
 * @param[in] pin A given pin.
 * @param[in] fValue TRUE for high, FALSE for low.
 */
void gpio_SetPin(BYTE pin, BOOL fValue);

/**
 * @brief Returns the state of a given pin.
 * @param[in] pin A given pin.
 * @return TRUE if high, FALSE if low.
 */
BOOL gpio_GetPin(BYTE pin);

/**
 * @brief Returns the state of a given pin as boolean.
 * @param pin A given pin.
 * @param pfState Pointer to result variable.
 * @return TRUE if pin port exists, FALSE otherwise.
 */
BOOL gpio_GetPinBool(BYTE pin, BYTE * pfState);

#endif /* _GPIO_DRIVER_H_ */


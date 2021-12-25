/*
 * gpio.h
 *
 *  Created on: Oct 23, 2021
 *      Author: SF314-56
 */

#include "stm32f0xx.h"                  // Device header
#include "hw_config.h"
#include "stdint.h"
#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_gpio.h"


#ifndef _GPIO_H_
#define _GPIO_H_


typedef enum __LED_STATE__{
	On,
	Off,
}Led_state;

typedef enum __VALVE_CONTROL_STATE{
	Valve_On,
	Valve_Off,
}Valve_control_state;

typedef enum __BTN_STATE__{
	Pressed,
	Released,
}Btn_state;

Btn_state Btn_read(uint8_t Btn);
void gpio_SetPin(uint8_t led_pin, Led_state state);
void Valve_control(Valve_control_state);
void Delay_ms(uint32_t time);


#endif /* _GPIO_H_ */

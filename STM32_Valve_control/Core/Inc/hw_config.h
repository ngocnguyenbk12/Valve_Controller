/*
 * hw_config.h
 *
 *  Created on: Oct 23, 2021
 *      Author: SF314-56
 */
#include "stm32f0xx.h"                  // Device header

#include "stdint.h"
#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_conf.h"
#include "stm32f0xx_it.h"


#ifndef INC_HW_CONFIG_H_
#define INC_HW_CONFIG_H_


UART_HandleTypeDef huart1;
#define ZM_UART USART1
#define ZM_UART_RxPin GPIO_Pin_10
#define	ZM_UART_TxPin GPIO_Pin_9
#define ZM_UART_IRQHandler 	USART1_IRQHandler


#define Led_leak 			GPIO_PIN_5
#define Led_flow 			GPIO_PIN_6
#define Led_open_close		GPIO_PIN_7
#define Valve_control_1 	GPIO_PIN_3
#define Valve_control_2 	GPIO_PIN_2
#define Water_probe 		GPIO_PIN_1
#define Flow_meter			GPIO_PIN_0
#define Btn_close 			GPIO_PIN_4
#define Btn_open 			GPIO_PIN_1


#endif /* INC_HW_CONFIG_H_ */

/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "gpio.h"
#include "hw_config.h"
#include "uart_command.h"
#include "stm32f0xx_hal_uart.h"
#include "string.h"
#include "stdio.h"
#include "stm32f0xx_hal_exti.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

enum
{
	VALVE_BEGIN = 0 ,
	VALVE_IDLE 	= 1,
	VALVE_FACTORYNEW = 2,
	VALVE_LEAK 	= 3,
	VALVE_OPEN 	= 4,
	VALVE_CLOSE = 5,
	VALVE_NON_LEAK = 6,
	VALVE_SETUP = 7,
	VALVE_METER_REPORT = 8,
} STATE_APP;

enum
{
	MODE_0 = 0,
	MODE_1 = 1,
}VALVE_MODE;

/*****************************************************************/
uint8_t Valve_state = VALVE_BEGIN;
uint8_t next_state = VALVE_IDLE;


uint8_t leak_state = 0;
uint8_t Valve_mode = 0;
uint8_t Meter_mode ;
uint8_t Meter_scale_value ;
uint8_t Meter_value ;
BYTE FLow_reading_state= FALSE;
uint32_t Flow_pulse_value;



/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim1;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

BYTE serBuf[SERBUF_MAX];
IBYTE serBufLen ;
IBYTE bChecksum_RX;
BOOL Zw_connect_status = FALSE;
ZW_UART_COMMAND cmd;
ZW_UART_COMMAND uart_cmd;
volatile uint8_t buffer[7] ;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM1_Init(void);
/* USER CODE BEGIN PFP */

Btn_state Btn_read(uint8_t Btn);
void gpio_SetPin(uint8_t led_pin, Led_state state);
void Valve_control(Valve_control_state state);
void Delay_ms(uint32_t time);
volatile T_CON_TYPE cmd_ready;
BYTE rx_receive[6];

void memcopy(uint8_t *des, uint8_t *src, uint8_t length){
	for(int i = 0; i < length ; i++){
		des[i] = src[i];
	}
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
	if (huart->Instance == USART1)
	{

		cmd_ready = uart_check_protocol(rx_receive[0], TRUE);

		HAL_UART_Receive_IT(&huart1,(uint8_t *)rx_receive, 1);

		if(cmd_ready == conFrameReceived){
			cmd_ready = conIdle;
			memcopy(&cmd, &serBuf[0], serBufLen);
			switch (cmd.zw_uartcommandset.cmd)
			{
				case COMMAND_VALVE:
					if(cmd.zw_uartcommandset.value1 == ZW_VALVE_ON){
					 	next_state = VALVE_OPEN		;
					}
					if(cmd.zw_uartcommandset.value1 == ZW_VALVE_OFF){
					 	next_state = VALVE_CLOSE	;
					}

				break;
				case COMMAND_CONFIGURATION:
					Zw_connect_status = TRUE;
					if(cmd.zw_uartcommandset.value1 == ZW_CONFIGURATION_SET)
					{
						Valve_mode = 1;
						//next_state = VALVE_SETUP;
					}
					if(cmd.zw_uartcommandset.value1 == ZW_CONFIGURATION_RESET)
					{
						Valve_mode = 0;
						//next_state = VALVE_SETUP;
					}
					break;
				case COMMAND_METER:
					if(cmd.zw_uartcommandset.type == ZW_METER_SET){
						Meter_scale_value = cmd.zw_uartcommandset.value1;
				//		next_state = VALVE_METER_SET ;
					}
					if(cmd.zw_uartcommandset.type == ZW_METER_GET){
						next_state = VALVE_METER_REPORT;

					}
					if(cmd.zw_uartcommandset.type == ZW_METER_RESET){
						Meter_mode = 0;
					}
					if(cmd.zw_uartcommandset.type == ZW_METER_REPORT){
						next_state = VALVE_METER_REPORT;
					}
				break;
			}
		}
	}
}

void Flow_meter_cal(uint32_t Tick){
	uint32_t Tick2 = 2*Tick/60000 ;
	uint32_t Frequency = 1/Tick2;
	Meter_value = Meter_scale_value * Frequency;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
	if(GPIO_Pin == Flow_meter)
	{
		if(FLow_reading_state){
			Flow_pulse_value = __HAL_TIM_GET_COUNTER(&htim1);
			Flow_meter_cal(Flow_pulse_value);
			HAL_TIM_Base_Stop(&htim1);

		}
		else{
		HAL_TIM_Base_Start(&htim1);
		}
	}
}



void State(void){
	switch(Valve_state){

	  case VALVE_BEGIN:

		 if(Zw_connect_status){
			 uart_cmd.zw_uartcommandset.length = 3;
			 uart_cmd.zw_uartcommandset.cmd = COMMAND_ZW_CONNECT;
			 uart_cmd.zw_uartcommandset.type = COMMAND_ZW_CONNECT;
			 uart_cmd.zw_uartcommandset.value1 = COMMAND_ZW_CONNECT;
			 if(ZW_UART_transmit(uart_cmd))
			 {
			 Valve_state = VALVE_IDLE;
			 }
		 }

		  Valve_state = VALVE_IDLE;

		 break;

		  case VALVE_IDLE:
		  			  if( (next_state == VALVE_OPEN) || (Btn_read(Btn_open)== Pressed)){
		  									  uart_cmd.zw_uartcommandset.length = 3;
		  									  uart_cmd.zw_uartcommandset.cmd = COMMAND_VALVE;
		  									  uart_cmd.zw_uartcommandset.type =ZW_VALVE_SET;
		  									  uart_cmd.zw_uartcommandset.value1 = ZW_VALVE_ON;
		  									  if(ZW_UART_transmit(uart_cmd)){
		  										  Valve_state = VALVE_OPEN;
		  										  Valve_control(On);
		  									  }
		  			  }
		  			  if( (next_state == VALVE_CLOSE) || (Btn_read(Btn_close)== Pressed)){
		  									  uart_cmd.zw_uartcommandset.length = 3;
		  									  uart_cmd.zw_uartcommandset.cmd = COMMAND_VALVE;
		  									  uart_cmd.zw_uartcommandset.type =ZW_VALVE_SET;
		  									  uart_cmd.zw_uartcommandset.value1 = ZW_VALVE_OFF;
		  									  if(ZW_UART_transmit(uart_cmd)){
		  										  Valve_state = VALVE_CLOSE;
		  										  Valve_control(Off);
		  									  }
		  			  }

		  			  if( (leak_state == 0) && HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_RESET) {
		  				  if(Valve_mode == 0){
		  				  					  uart_cmd.zw_uartcommandset.length = 3;
		  				  					  uart_cmd.zw_uartcommandset.cmd = COMMAND_LEAK;
		  				  					  uart_cmd.zw_uartcommandset.type =ZW_LEAK;
		  				  					  uart_cmd.zw_uartcommandset.value1 = ZW_LEAK;
		  				  					  if(ZW_UART_transmit(uart_cmd)){
		  				  					  gpio_SetPin(Led_leak, On);
		  				  					  Valve_state = VALVE_IDLE;
		  				  					  leak_state = 1;
		  				  					  }
		  				  				  }
		  				  				  else{
		  				  					  Valve_state = VALVE_IDLE;
		  				  					  leak_state = 1;
		  				  				  }
		  			  }

		  			  if( (leak_state == 1) && HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_SET ){
		  				  if(Valve_mode == 0){
		  				  					  uart_cmd.zw_uartcommandset.length = 3;
		  				  					  uart_cmd.zw_uartcommandset.cmd = COMMAND_LEAK;
		  				  					  uart_cmd.zw_uartcommandset.type =ZW_NON_LEAK;
		  				  					  uart_cmd.zw_uartcommandset.value1 = ZW_NON_LEAK;
		  				  					 if(ZW_UART_transmit(uart_cmd)){
		  				  					  gpio_SetPin(Led_leak, Off);
		  				  					  Valve_state = VALVE_IDLE;
		  				  					  leak_state = 0;
		  				  					 }
		  				  				  }
		  				  				  else{
		  				  			  		gpio_SetPin(Led_leak, Off);
		  				  			  		Valve_state = VALVE_IDLE;
		  				  			  		leak_state = 0;
		  				  			  	}
		  			  }
		  			  if(next_state == VALVE_BEGIN){
		  				  Valve_state = VALVE_BEGIN;
		  				  next_state = VALVE_IDLE;
		  			  }
		  			  break;

		  			  if(next_state == VALVE_METER_REPORT){
		  				  uart_cmd.zw_uartcommandset.length = 3;
		  				  uart_cmd.zw_uartcommandset.cmd = COMMAND_METER;
		  				  uart_cmd.zw_uartcommandset.type = ZW_METER_REPORT;
		  				  uart_cmd.zw_uartcommandset.type = Meter_value;
		  				  if(ZW_UART_transmit(uart_cmd)){
		  					  next_state = VALVE_IDLE;
		  				  }
		  			  }


		  		  case VALVE_OPEN:
		  			  gpio_SetPin(Led_open_close, On);
		  			  if( (next_state == VALVE_CLOSE) || (Btn_read(Btn_close)== Pressed)){
		  				  	  	  uart_cmd.zw_uartcommandset.length = 3;
		  						  uart_cmd.zw_uartcommandset.cmd = COMMAND_VALVE;
		  						  uart_cmd.zw_uartcommandset.type =ZW_VALVE_SET;
		  						  uart_cmd.zw_uartcommandset.value1 = ZW_VALVE_OFF;

		  						  if(ZW_UART_transmit(uart_cmd)){
		  							  Valve_state = VALVE_CLOSE;
		  							  next_state = VALVE_IDLE;
		  							  Valve_control(Off);
		  						  }

		  			  }
		  			  if( (leak_state == 0) && HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_RESET) {
		  				  if(Valve_mode == 0){
		  					  uart_cmd.zw_uartcommandset.length = 3;
		  					  uart_cmd.zw_uartcommandset.cmd = COMMAND_LEAK;
		  					  uart_cmd.zw_uartcommandset.type =ZW_LEAK;
		  					  uart_cmd.zw_uartcommandset.value1 = ZW_LEAK;
		  					  if(ZW_UART_transmit(uart_cmd)){
		  					  gpio_SetPin(Led_leak, On);
		  					  Valve_state = VALVE_IDLE;
		  					  leak_state = 1;
		  					  }
		  				  }
		  				  else{
		  					  Valve_state = VALVE_IDLE;
		  					  leak_state = 1;
		  				  }
		  			  }

		  			  if( (leak_state == 1) && HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_SET ){
		  				  if(Valve_mode == 0){
		  					  uart_cmd.zw_uartcommandset.length = 3;
		  					  uart_cmd.zw_uartcommandset.cmd = COMMAND_LEAK;
		  					  uart_cmd.zw_uartcommandset.type =ZW_NON_LEAK;
		  					  uart_cmd.zw_uartcommandset.value1 = ZW_NON_LEAK;
		  					 if(ZW_UART_transmit(uart_cmd)){
		  					  gpio_SetPin(Led_leak, Off);
		  					  Valve_state = VALVE_IDLE;
		  					  leak_state = 0;
		  					 }
		  				  }
		  				  else{
		  			  		gpio_SetPin(Led_leak, Off);
		  			  		Valve_state = VALVE_IDLE;
		  			  		leak_state = 0;
		  			  	}
		  			  }
		  			  if(next_state == VALVE_BEGIN){
		  							  Valve_state = VALVE_BEGIN;
		  							  next_state = VALVE_IDLE;
		  						  }
		  			  break;

		  		  case VALVE_CLOSE:
		  			  gpio_SetPin(Led_open_close, Off);
		  			  if( (next_state == VALVE_OPEN) || (Btn_read(Btn_open)== Pressed)){


		  							uart_cmd.zw_uartcommandset.length = 3;
		  							uart_cmd.zw_uartcommandset.cmd = COMMAND_VALVE;
		  							uart_cmd.zw_uartcommandset.type =ZW_VALVE_SET;
		  							uart_cmd.zw_uartcommandset.value1 = ZW_VALVE_ON;
		  							if(ZW_UART_transmit(uart_cmd)){
		  								Valve_state = VALVE_OPEN;
		  								next_state = VALVE_IDLE;
		  								Valve_control(On);
		  							}
		  			  	  }
		  			  if( (leak_state == 0) && HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_RESET) {
		  			  				  if(Valve_mode == 0){
		  			  					  uart_cmd.zw_uartcommandset.length = 3;
		  			  					  uart_cmd.zw_uartcommandset.cmd = COMMAND_LEAK;
		  			  					  uart_cmd.zw_uartcommandset.type =ZW_LEAK;
		  			  					  uart_cmd.zw_uartcommandset.value1 = ZW_LEAK;
		  			  					  if(ZW_UART_transmit(uart_cmd)){
		  			  					  gpio_SetPin(Led_leak, On);
		  			  					  Valve_state = VALVE_IDLE;
		  			  					  leak_state = 1;
		  			  					  }
		  			  				  }
		  			  				  else{
		  			  					  Valve_state = VALVE_IDLE;
		  			  					  leak_state = 1;
		  			  				  }
		  			  			  }

		  			  if( (leak_state == 1) && HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_SET ){
		  			  				  if(Valve_mode == 0){
		  			  					  uart_cmd.zw_uartcommandset.length = 3;
		  			  					  uart_cmd.zw_uartcommandset.cmd = COMMAND_LEAK;
		  			  					  uart_cmd.zw_uartcommandset.type =ZW_NON_LEAK;
		  			  					  uart_cmd.zw_uartcommandset.value1 = ZW_NON_LEAK;
		  			  					  if(ZW_UART_transmit(uart_cmd)){
		  			  					  gpio_SetPin(Led_leak, Off);
		  			  					  Valve_state = VALVE_IDLE;
		  			  					  leak_state = 0;
		  			  					  }
		  			  				  }
		  			  				  else{
		  			  			  		gpio_SetPin(Led_leak, Off);
		  			  			  		Valve_state = VALVE_IDLE;
		  			  			  		leak_state = 0;
		  			  			  	}
		  			  			  }
		  			  if(next_state == VALVE_BEGIN){
		  				  Valve_state = VALVE_BEGIN;
		  				  next_state = VALVE_IDLE;
		  			  }
		  			  break;
	}
}
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
  HAL_UART_Receive_IT(&huart1,(uint8_t *)rx_receive, 1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7, GPIO_PIN_SET);
  while (1)
  {
	  State();




    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSE;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 8;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 8000;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_5|GPIO_PIN_6
                          |GPIO_PIN_7, GPIO_PIN_RESET);

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PA1 */
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PA2 PA3 PA5 PA6
                           PA7 */
  GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_5|GPIO_PIN_6
                          |GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PA4 */
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PB1 */
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);

}

/* USER CODE BEGIN 4 */
/*
T_CON_TYPE uart_check_protocol(uint8_t c , BYTE acknowledge){

	static T_CON_TYPE retval = conIdle;

		switch (con_state)
		{
		case stateSOFHunt:
			if(c == SOF){
				bChecksum_RX = 0xff;
				serBufLen = 0;
				con_state ++;

			}

			else{
				if (AckNakNeeded)
				          {
				              if (c == ACK)
				              {
				                retval = conFrameSent;
				                AckNakNeeded = FALSE;  // Done
				                AckNak = ACK;
				              }
				              else if (c == NAK)
				              {
				                retval = conTxTimeout;
				                AckNakNeeded = FALSE;
				                AckNak = NAK;
				              }
				              else
				              {
				                // Bogus character received...
				              }
				            }
			}

			break;
		case stateLen:
			 if (( c< FRAME_LENGTH_MIN) || (c> FRAME_LENGTH_MAX))
			          {
			            con_state = stateSOFHunt; // Restart looking for SOF
			            break;
			          }

        case stateCmd :
          if(serBufLen &&  (c > COMMAND_END))
          {
            con_state = stateSOFHunt; // Restart looking for SOF
            break;
          }

        case stateType:
        	con_state++;

		case stateData:
				if(serBufLen < SERBUF_MAX){
					serBuf[serBufLen] = c;
					serBufLen++;
					bChecksum_RX ^= c;
					if (serBufLen >= serFrameLen)
					   {
					              con_state++;
					   }
					}
				else{
					con_state++;
					}
				break;

		case stateChecksum:

			if(acknowledge){
				if(c == bChecksum_RX)
				{
					//gpio_SetPin(Led_flow, On);
					retval = conFrameReceived;
					con_state = stateSOFHunt;
				}
				else{
					retval = conFrameErr;
					con_state = stateSOFHunt;
				}
			}
			break;
		}
	return retval;
}
*/
Btn_state Btn_read(uint8_t Btn){
	switch (Btn){
		case Btn_open:
			if(HAL_GPIO_ReadPin(GPIOB, Btn_open) == GPIO_PIN_RESET)
				{
				return Pressed;
				}
			else
				{
				return Released;
				}

		case Btn_close:
			if(HAL_GPIO_ReadPin(GPIOA, Btn_close) == GPIO_PIN_RESET)
				{
				return Pressed;
				}
			else
				{
				return Released;
				}
	}
	return Released;
}


void gpio_SetPin(uint8_t led_pin, Led_state state){
	switch (state){
		case On:
			HAL_GPIO_WritePin(GPIOA, led_pin, GPIO_PIN_RESET);
			break;
		case Off:
			HAL_GPIO_WritePin(GPIOA, led_pin, GPIO_PIN_SET);
		break;
	}
}
void Delay_ms(uint32_t time){
	HAL_Delay(time);
}

void Valve_control(Valve_control_state state)
{
	switch (state){
		case Valve_On:
			HAL_GPIO_WritePin(GPIOA, Valve_control_2, GPIO_PIN_RESET);
			Delay_ms(1000);
			HAL_GPIO_WritePin(GPIOA, Valve_control_1, GPIO_PIN_SET);
			Delay_ms(7000);
			HAL_GPIO_WritePin(GPIOA, Valve_control_1, GPIO_PIN_RESET);



		break;
		case Valve_Off:


			HAL_GPIO_WritePin(GPIOA, Valve_control_1, GPIO_PIN_RESET);
			Delay_ms(1000);
			HAL_GPIO_WritePin(GPIOA, Valve_control_2, GPIO_PIN_SET);
			Delay_ms(7000);
			HAL_GPIO_WritePin(GPIOA, Valve_control_2, GPIO_PIN_RESET);
	}
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

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
 * @date
 * @author Ngoc Nguyen
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
#include <key.h>
#include <key_driver.h>
#include <gpio_driver.h>
#include <ZAF_pm.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/


#define KEY_HOLD_THRESHOLD          (10)  //100 msec called 10 times for battery control
#define KEY_TRIPLE_PRESS_THRESHOLD  (30) // 300 mSec
#define KEY_DRIVER_HOLD_TIMEOUT (11) // 110 mSec
#define KEY_TRIPLE_PRESS_TIMEOUT (20)
#define KEY_HOLD_TIME 								(250)
#define KEY_DOUBLE_TIME								(10)
#define BLINKLIGHT_TIME 							(10)


P_KEY_EVENT_T Button_state  = KEY_IDLE;

	static BYTE TimerCounter = 0;

	static BYTE keyHeldTimerHandle = 0;
	static BYTE keyonemoretaphandle = 0;
	static BYTE doubleClickCounter = 0;
	static BYTE BlickLightCounter = 0;
	
	
	

void ZCB_InterruptCall(BYTE bLevelValue);
void ZCB_ResetClickCounter(void);
void ZCB_holdClickCounter(void);

void clearbuttoncounter(void);
void clearbuttonstate(void);
void ZBC_BlinkLightCallBack(void);
void BlinkToggle(void);
void BlinkLight(void);


	

BOOL Valvekeyinit(void){
	#ifndef __C51__
  //memset((BYTE *)keyList, 0, (NUMBER_OF_KEYS * sizeof(KEY_LIST_T)));
//  keyTriplePressTimerHandle = 0;
#endif //__C51__
// whichButton = WHICH_BUTTON_RESET;
//  fKeyHoldThresholdExceeded = FALSE;
//  pressCount = 0;
//  holdCount = 0;
//  hold10Count = 0;
//  m_keep_alive_timer_activated = FALSE;

#ifndef NON_BATT
  // TODO: Keep prel detection active under powering on.
  ZAF_pm_KeepAwake(50);
#endif
  /*
   * Initialize the interrupt module with a pointer to a function to call on
   * every edge change.
   */
  // drop callback function and init zw_ext_int1
  // drop 1 for interrupt 1
  
	

  return InterruptDriverInit(ZW_EXT_INT1, ZCB_InterruptCall);
  
}

PCB(ZCB_InterruptCall)(BYTE bLevelValue)
{
//	gpio_SetPin(0x10, FALSE);
//					gpio_SetPin(0x36, FALSE);
//					gpio_SetPin(0x04, FALSE);
	


	
	//					gpio_SetPin(0x10, TRUE);
//					gpio_SetPin(0x36, TRUE);
//					gpio_SetPin(0x04, TRUE);
	
	
	
	
	
	
  if (0 == bLevelValue)
  {
//					gpio_SetPin(0x10, FALSE);
//					gpio_SetPin(0x36, FALSE);
//					gpio_SetPin(0x04, FALSE);
		
			if(Button_state == KEY_IDLE || KEY_HELD){
					Button_state = KEY_HELD;
					keyHeldTimerHandle = TimerStart(ZCB_holdClickCounter, (KEY_HOLD_TIME), 1);
			}
			
			if( Button_state == KEY_READY_FOR_LEARNMODE){
					Button_state = KEY_LEARNMODE;
			}
			
			if(Button_state == KEY_READY_FOR_FACTORYNEW_RESET){
					Button_state = KEY_FACTORYNEW_RESET;
			}
		
			
	}
	else
	{
				TimerCancel(keyHeldTimerHandle);
		if(Button_state == KEY_PRE_FACTORYNEW){	
					gpio_SetPin(0x04, TRUE);
					gpio_SetPin(0x10, TRUE);
					gpio_SetPin(0x36, TRUE);
				Button_state = KEY_READY_FOR_FACTORYNEW_RESET;
				keyonemoretaphandle = TimerStart(ZCB_ResetClickCounter, (BLINKLIGHT_TIME), 1);
		}
		if(Button_state == KEY_PRE_LEARNMODE){
					gpio_SetPin(0x04, TRUE);
					gpio_SetPin(0x10, TRUE);
					gpio_SetPin(0x36, TRUE);
			TimerCounter = 0;
			Button_state = KEY_READY_FOR_LEARNMODE;
				keyonemoretaphandle = TimerStart(ZCB_ResetClickCounter, (BLINKLIGHT_TIME), 1);
		}
		if(Button_state == KEY_HELD){
					gpio_SetPin(0x04, TRUE);
					gpio_SetPin(0x10, TRUE);
					gpio_SetPin(0x36, TRUE);
			clearbuttoncounter();
		}
	}
}
	

PCB(ZCB_holdClickCounter)(void)
{
			TimerCounter++;
	if(TimerCounter ==  1){              // Pink 
							gpio_SetPin(0x04, FALSE);
					gpio_SetPin(0x10, FALSE);
					gpio_SetPin(0x36, FALSE);
		TimerCancel(keyHeldTimerHandle);
		keyHeldTimerHandle = TimerStart(ZCB_holdClickCounter, (KEY_HOLD_TIME), 1);
		Button_state = KEY_PRE_LEARNMODE;
		//BlinkLight();
	}
	if(TimerCounter == 2 || TimerCounter == 3 || TimerCounter >4 ){
		Button_state = KEY_HELD;
		gpio_SetPin(0x10, TRUE);
		gpio_SetPin(0x04, TRUE);
		gpio_SetPin(0x36, TRUE);
		TimerCancel(keyHeldTimerHandle);
		keyHeldTimerHandle = TimerStart(ZCB_holdClickCounter, (KEY_HOLD_TIME), 1);
		TimerCancel(BlickLightCounter);
	}
	
	if(TimerCounter == 4){            //  white
		gpio_SetPin(0x36, FALSE);
		gpio_SetPin(0x04, FALSE);
		gpio_SetPin(0x10, FALSE);
		TimerCancel(keyHeldTimerHandle);
		Button_state = KEY_PRE_FACTORYNEW;
		TimerCounter = 0;
		//BlinkLight();
	}
}

PCB(ZCB_ResetClickCounter)(void){
	static ResetCounter = 0;
	static BOOL checker = TRUE;
	ResetCounter++;
	
					BlinkToggle();
					TimerCancel(keyonemoretaphandle);
					keyonemoretaphandle = TimerStart(ZCB_ResetClickCounter, (BLINKLIGHT_TIME), 1);
	if(ResetCounter > 50){
					Button_state = KEY_IDLE;
					gpio_SetPin(0x04, FALSE);
					gpio_SetPin(0x10, FALSE);
					gpio_SetPin(0x36, FALSE);
					ResetCounter = 0;
					TimerCancel(keyonemoretaphandle);
	}

}



void clearbuttoncounter(void){
	Button_state = KEY_IDLE;
	TimerCounter = 0;
}


KEY_EVENT_T GetButtonstatus(void){
	if(Button_state == KEY_HELD){
	return KEY_IDLE;
	}
	else{
		return Button_state;
	}
}

void Clear_Button_status(void){
	Button_state = KEY_IDLE;
					gpio_SetPin(0x10, TRUE);
					gpio_SetPin(0x36, TRUE);
					gpio_SetPin(0x04, TRUE);
}

void BlinkLight(void)
{
	static BYTE BlinkLightCounter = 0;
	static BYTE BlinkLightTimer ;
	BlickLightCounter = TimerStart(ZBC_BlinkLightCallBack, (BLINKLIGHT_TIME), 1);
}

void ZBC_BlinkLightCallBack(void){
	static BYTE i = 0;
	static BOOL checker = TRUE;
	if( i > 50 ){
		TimerCancel(BlickLightCounter);
		BlickLightCounter = 0;
		i = 0;
		gpio_SetPin(0x04, TRUE);
		gpio_SetPin(0x10, TRUE);
		gpio_SetPin(0x36, TRUE);
		}
		
		else{
			
			//BlinkToggle();
			i++;
			TimerRestart(BlickLightCounter);
				if(checker == FALSE){
					gpio_SetPin(0x04, FALSE);
					gpio_SetPin(0x10, FALSE);
					gpio_SetPin(0x36, FALSE);
					checker = TRUE;
				}
				else
				{
					gpio_SetPin(0x04, TRUE);
					gpio_SetPin(0x10, TRUE);
					gpio_SetPin(0x36, TRUE);
					checker = FALSE;
				}
		}
}

void BlinkToggle(void){
	static BOOL i = TRUE;
	if(i == FALSE){
		gpio_SetPin(0x04, FALSE);
		gpio_SetPin(0x10, FALSE);
		gpio_SetPin(0x36, FALSE);
		i = TRUE;
	}
	else
	{
		gpio_SetPin(0x04, TRUE);
		gpio_SetPin(0x10, TRUE);
		gpio_SetPin(0x36, TRUE);
		i = FALSE;
	}
}

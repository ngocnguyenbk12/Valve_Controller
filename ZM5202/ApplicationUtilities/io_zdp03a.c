/***************************************************************************
*
* Copyright (c) 2001-2014
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: Some nice descriptive description.
*
* Author: Thomas Roll
*
* Last Changed By: $Author: tro $
* Revision: $Revision: 0.00 $
* Last Changed: $Date: 2014/12/15 20:09:03 $
*
****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW050x.h>
#include <io_zdp03a.h>
#include <ZW_uart_api.h>
#include <ZW_nvr_api.h>
#include <appl_timer_api.h>
#include <key_driver.h>
#include <gpio_driver.h>
#include <misc.h>
#include <ota_util.h>
#include <CommandClassVersion.h>
#include <ZW_string.h>
#include <ZW_security_api.h>

#ifdef TEST_INTERFACE_SUPPORT
#include <ZW_test_interface.h>
#endif /*TEST_INTERFACE_SUPPORT*/
/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
#ifdef ZW_DEBUG_IO_ZDP03A
#define ZW_DEBUG_IO_ZDP03A_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_IO_ZDP03A_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_IO_ZDP03A_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_IO_ZDP03A_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_IO_ZDP03A_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_IO_ZDP03A_SEND_BYTE(data)
#define ZW_DEBUG_IO_ZDP03A_SEND_STR(STR)
#define ZW_DEBUG_IO_ZDP03A_SEND_NUM(data)
#define ZW_DEBUG_IO_ZDP03A_SEND_WORD_NUM(data)
#define ZW_DEBUG_IO_ZDP03A_SEND_NL()
#endif

//#define IO_ZDP03A_HELP
//#define IO_ZDP03A_EXTRAS


typedef struct _IO_ZDP03A_
{
  VOID_CALLBACKFUNC(pFlasTimeout)(void);
  BYTE bTimerHandle;
} IO_ZDP03A;


typedef struct _KEYMAN_
{
  VOID_CALLBACKFUNC(pEventKeyQueue)(BYTE);
} t_KEYMAN;

union DSK_PART
{
  uint16_t key16;
  uint8_t keyArray[2];
};

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

IO_ZDP03A myIo = { NULL, 0};
t_KEYMAN mykeyMan = {NULL};
/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/
void ZCB_KeyAction(KEY_NAME_T keyName, KEY_EVENT_T keyEvent, BYTE holdCount);
#ifdef TEST_INTERFACE_SUPPORT
void ZCB_AppTestInterface(char channel, char * pString);
#endif

#ifdef TEST_INTERFACE_SUPPORT
/** Get application state*/
extern BYTE GetAppState(void);
#endif

/**
 * List here the pins which are used as GPIO and have been pin swapped.
 * The pins listed here are the two swapped pins on the Z-Wave ZM5202
 * Development Module which is used as LED and button at the Z-Wave ZDP03A
 * Development Platform.
 */
static PIN_T_ARRAY xdata pins = {
        {0x07, 0x10},
        {0x22, 0x32}
};

void
gpio_GetPinSwapList(PIN_T_ARRAY xdata ** pPinList, BYTE * const pPinListSize)
{
  *pPinList = (PIN_T_ARRAY xdata *)&pins;
  *pPinListSize = sizeof(pins) / sizeof(PIN_T);
}



/*============================ InitZDP03A ===============================
**-------------------------------------------------------------------------*/
void ZDP03A_InitHW(
  VOID_CALLBACKFUNC(pEventKeyQueue)(BYTE))             /*function-pointer to application event-queue or state machine*/
{
  ZW_DEBUG_IO_ZDP03A_SEND_STR("InitZDP03A");

  gpio_DriverInit(TRUE);
  mykeyMan.pEventKeyQueue = pEventKeyQueue;

  /*
   * Initialize keys.
   */
  if (!KeyDriverInit())
  {
    //ZW_DEBUG_IO_ZDP03A_SEND_NL();
    //ZW_DEBUG_IO_ZDP03A_SEND_STR("Couldn't initialize Key Driver :(");
  }

  /**
   * Setup interrupt for EXT1 which is used by the key driver.
   */
  gpio_SetPinIn(ZDP03A_KEY_INTP, TRUE);
  ZW_SetExtIntLevel(ZW_INT1, FALSE);
  EX1 = 1; // Enable external interrupt 1

#ifdef TEST_INTERFACE_SUPPORT
  if (TRUE == ZW_test_interface_allocate('a', ZCB_AppTestInterface))
  {
    //ZW_DEBUG_IO_ZDP03A_SEND_NL();
    //ZW_DEBUG_IO_ZDP03A_SEND_STR("TI init.");
  }
  else
#endif
  {
    //ZW_DEBUG_IO_ZDP03A_SEND_NL();
    //ZW_DEBUG_IO_ZDP03A_SEND_STR("No TI.");
  }
}

void SetPinIn( ZDP03A_KEY key, BOOL pullUp)
{
    //ZW_DEBUG_IO_ZDP03A_SEND_NL();
    //ZW_DEBUG_IO_ZDP03A_SEND_STR("SetPinIn key");
    //ZW_DEBUG_IO_ZDP03A_SEND_NUM(key);
    //ZW_DEBUG_IO_ZDP03A_SEND_NUM(pullUp);
  switch(key)
  {
    case ZDP03A_KEY_1:
      KeyDriverRegisterCallback(KEY01, key, BITFIELD_KEY_DOWN | BITFIELD_KEY_UP | BITFIELD_KEY_PRESS |BITFIELD_KEY_HOLD,  ZCB_KeyAction);
      break;
    case ZDP03A_KEY_2:
      KeyDriverRegisterCallback(KEY02, key, BITFIELD_KEY_DOWN | BITFIELD_KEY_UP | BITFIELD_KEY_PRESS |BITFIELD_KEY_HOLD, ZCB_KeyAction);
      break;
    case ZDP03A_KEY_3:
      // ZDP03A S3 is used for SPI/NVM.
      break;
    case ZDP03A_KEY_4:
      KeyDriverRegisterCallback(KEY04, key, BITFIELD_KEY_DOWN | BITFIELD_KEY_UP | BITFIELD_KEY_PRESS |BITFIELD_KEY_HOLD, ZCB_KeyAction);
      break;
    case ZDP03A_KEY_5:
      KeyDriverRegisterCallback(KEY05, key, BITFIELD_KEY_DOWN | BITFIELD_KEY_UP | BITFIELD_KEY_PRESS |BITFIELD_KEY_HOLD, ZCB_KeyAction);
      break;
    case ZDP03A_KEY_6:
      KeyDriverRegisterCallback(KEY06, key, BITFIELD_KEY_DOWN | BITFIELD_KEY_UP | BITFIELD_KEY_PRESS |BITFIELD_KEY_HOLD, ZCB_KeyAction);
      break;
  }
  gpio_SetPinIn(key, pullUp);
}

void SetPinOut( LED_OUT led)
{
  gpio_SetPinOut(led);
}


/*============================ Led ===============================
**-------------------------------------------------------------------------*/
void Led( LED_OUT led,  LED_ACTION action)
{
  gpio_SetPin(led, action);
}


/*============================ ButtonGet ===============================
**-------------------------------------------------------------------------*/
BYTE KeyGet(ZDP03A_KEY key)
{
  return gpio_GetPin( key);
}

/**
 * Uswed to calc key-BX group start
 */
EVENT_KEY keyGrpStart[NUMBER_OF_KEYS] =
{
  EVENT_KEY_B1_DOWN,
  EVENT_KEY_B2_DOWN,
  EVENT_KEY_B3_DOWN,
  EVENT_KEY_B4_DOWN,
  EVENT_KEY_B5_DOWN,
  EVENT_KEY_B6_DOWN
};

/**
 * @brief Call specific Key0XAction function dependent on keyName.
 */
PCB(ZCB_KeyAction)(KEY_NAME_T keyName, KEY_EVENT_T keyEvent, BYTE holdCount)
{
  BYTE keyCalcEvent = 0xff; /* not legal number*/

  //ZW_DEBUG_IO_ZDP03A_SEND_STR("ZCB_KeyAction ");
  //ZW_DEBUG_IO_ZDP03A_SEND_NUM(keyName);
  //ZW_DEBUG_IO_ZDP03A_SEND_NUM(keyEvent);
  //ZW_DEBUG_IO_ZDP03A_SEND_NUM(holdCount);
  //ZW_DEBUG_IO_ZDP03A_SEND_NL();

  if(IS_NULL(mykeyMan.pEventKeyQueue))
  {
    //ZW_DEBUG_IO_ZDP03A_SEND_STR("pEventKeyQueue not initialize!!");
    //ZW_DEBUG_IO_ZDP03A_SEND_NL();
    return;
  }
  if((KEY01 != keyName))
  {
    /* We do not handle KEY_TRIPLE_PRESS for other keys than key1*/
    if((KEY_TRIPLE_PRESS != keyEvent))
    {
      if(KEY_HOLD == keyEvent)
      {
        if(1 == holdCount)
        {
          /* Only send the first hold indication. Use KEY_UP for hold is finish!*/
          keyCalcEvent = keyGrpStart[keyName] + keyEvent;
        }
      }
      else /*KEY_DOWN || KEY_UP || KEY_PRESS*/
      {
        keyCalcEvent = keyGrpStart[keyName] + keyEvent;
      }
    }
  }
  else
  {
    if(KEY_HOLD == keyEvent)
    {
      if(1 == holdCount)
      {
        keyCalcEvent = keyGrpStart[keyName] + keyEvent;
      }
      else if (10 == holdCount)
      {
        keyCalcEvent = keyGrpStart[keyName] + (EVENT_KEY_B1_HELD_10_SEC - DEFINE_EVENT_KEY_NBR);
      }
    }
    else
    {
      keyCalcEvent = keyGrpStart[keyName] + keyEvent;
    }
  }

  if(0xff != keyCalcEvent)
  {
    ZW_DEBUG_IO_ZDP03A_SEND_NL();
    ZW_DEBUG_IO_ZDP03A_SEND_BYTE('E');
    ZW_DEBUG_IO_ZDP03A_SEND_NUM(keyCalcEvent);
    mykeyMan.pEventKeyQueue(keyCalcEvent);
  }
}

#ifdef TEST_INTERFACE_SUPPORT
/**
 * @brief Extract key action.
 * @param channel The desired channel to listen on.
 * @param pString includes event string as:
 *      Key:
 *        pString[0]    = k    (k)ey
 *        pString[1]    = 1..6 key number
 *        pString[2,3]  = d (d)own
 *                      = u (u)p
 *                      = p   -> (p)ress
 *                      = h   -> (h)old
 *                      = tp  -> (t)riple (p)ress
 *                      = t   -> (t)riple press
 *                      = h   -> (h)old
 *                      = 10h -> 10 sec (h)old
 *      Reset: pString[0..4] = "reset"
 */
PCB(ZCB_AppTestInterface)(char channel, char * pString)
{
  uint8_t stringLength;
  KEY_NAME_T keyName = NUMBER_OF_KEYS; /**<Not legal key number*/
  KEY_EVENT_T keyEvent = NUMBER_OF_KEY_EVENTS; /**<Not legal key event*/
  BYTE holdCount = 1;
  UNUSED(channel);
  //ZW_DEBUG_IO_ZDP03A_SEND_NL();
  //ZW_DEBUG_IO_ZDP03A_SEND_BYTE(channel);
  //ZW_DEBUG_IO_ZDP03A_SEND_STR(": ");
  //ZW_DEBUG_IO_ZDP03A_SEND_STR(pString);
  //ZW_DEBUG_IO_ZDP03A_SEND_NL();

  stringLength = ZW_strlen((BYTE *)pString);

  /**< Check legal key*/
  if ('k' == pString[0])
  {
    /**< Find key name*/
    switch (pString[1])
    {
      case '1':
        keyName = KEY01;
        break;
      case '2':
        keyName = KEY02;
        break;
      case '3':
        keyName = KEY03;
        break;
      case '4':
        keyName = KEY04;
        break;
      case '5':
        keyName = KEY05;
        break;
      case '6':
        keyName = KEY06;
        break;
      default:
        // Unknown key => Return
        return;
        break;
    }

    /**< Find key action (add 2 to see key action)*/
    switch (pString[2])
    {
      case 'd':
        keyEvent = KEY_DOWN;
        break;
      case 'u':
        keyEvent = KEY_UP;
        break;
      case 'p':
        keyEvent = KEY_PRESS;
        break;
      case 'h':
        keyEvent = KEY_HOLD;
        if (stringLength > 3)
        {
          // There's a hold value
          if (('1' == pString[3])
           && ('0' == pString[4]))
          {
            holdCount = 10;
          }
        }
        break;
      case 't':
        keyEvent = KEY_TRIPLE_PRESS;
        break;
      default:
        // Unknown event => Return.
        return;
        break;
    }

    if((NUMBER_OF_KEYS != keyName) && (NUMBER_OF_KEY_EVENTS != keyEvent))
    {
      ZCB_KeyAction(keyName, keyEvent, holdCount);
    }
    else
    {
      //ZW_DEBUG_IO_ZDP03A_SEND_STR("key error!");
      //ZW_DEBUG_IO_ZDP03A_SEND_NL();
    }
  }
  else if ('s' == pString[0] && 'p' == pString[1])
  {
    // "a sp XX Y"
    uint8_t pin;
    pin = ((pString[3] - 0x30) << 4) + (pString[4] - 0x30);
    gpio_SetPin(pin, (0x31 == pString[6])?TRUE:FALSE);
  }
  else if ('g' == pString[0] && 'p' == pString[1])
  {
    uint8_t pin;

    //ZW_DEBUG_IO_ZDP03A_SEND_NL();
    //ZW_DEBUG_IO_ZDP03A_SEND_BYTE('G');
    //ZW_DEBUG_IO_ZDP03A_SEND_BYTE('P');

    if (5 != stringLength)
    {
      // Simple check for the command length. Return if invalid.
      return;
    }

    pin = ((pString[3] - 0x30) << 4) + (pString[4] - 0x30);

    if (TRUE == gpio_GetPin(pin))
    {
      ZW_UART0_tx_send_byte('1');
    }
    else
    {
      ZW_UART0_tx_send_byte('0');
    }
  }
  else if (!memcmp(pString, "reset", 5))
  {
    mykeyMan.pEventKeyQueue(EVENT_SYSTEM_RESET);
  }
  else if (!memcmp(pString, "csa", 3))
  {
    /*
     * This is used for Client Side Authentification (CSA).
     */
#define DSK_PART_SIZE (5)
#define DSK_OFFSET (4)
    uint8_t partCount;
    uint8_t charCount;
    union DSK_PART dskPart;
    s_SecurityS2InclusionCSAPublicDSK_t response;
    uint8_t byteCount = 0;

    for (partCount = 0; partCount < 2; partCount++)
    {
      dskPart.key16 = 0;
      for (charCount = 0; charCount < 5; charCount++)
      {
        dskPart.key16 += (pString[DSK_OFFSET + (DSK_PART_SIZE * partCount) + charCount] - 0x30); // Convert from ascii to int and add.
        if (charCount < 4)
        {
          // Do only "shift" the first four digits.
          dskPart.key16 *= 10;
        }
      }
      response.aCSA_DSK[byteCount++] = dskPart.keyArray[0];
      response.aCSA_DSK[byteCount++] = dskPart.keyArray[1];
    }

    ZW_UART0_tx_send_nl();
    ZW_UART0_tx_send_str((BYTE*)"Got it!");

#ifdef __C51__
    ZW_UART0_tx_send_nl();
    ZW_UART0_tx_send_num(response.aCSA_DSK[0]);
    ZW_UART0_tx_send_num(response.aCSA_DSK[1]);
    ZW_UART0_tx_send_num(response.aCSA_DSK[2]);
    ZW_UART0_tx_send_num(response.aCSA_DSK[3]);
#endif

    ZW_SetSecurityS2InclusionPublicDSK_CSA(&response);
  }
  else if (!memcmp(pString, "lms", 3))
  {
    mykeyMan.pEventKeyQueue(EVENT_SYSTEM_LEARNMODE_START);
  }
  else if (!memcmp(pString, "lme", 3))
  {
    mykeyMan.pEventKeyQueue(EVENT_SYSTEM_LEARNMODE_END);
  }
  else if (!memcmp(pString, "wdr", 3))
  {
    mykeyMan.pEventKeyQueue(EVENT_SYSTEM_WATCHDOG_RESET);
  }
#ifdef IO_ZDP03A_EXTRAS
  else if (!memcmp(pString, "ast", 3))
  {
    ZW_UART0_tx_send_nl();
    ZW_UART0_tx_send_str((BYTE*)"Application state: ");
    ZW_UART0_tx_send_num(GetAppState());
    ZW_UART0_tx_send_nl();
  }
#endif
//#ifdef IO_ZDP03A_EXTRAS
  else if (!memcmp(pString, "ZW", 2))
  {
    BYTE tempString[10];
    BYTE x;
    ZW_UART0_tx_send_nl();
    ZW_UART0_tx_send_str((BYTE*)"ZW app:\r\n");
    ZW_UART0_tx_send_str((BYTE*)" Firmware ID: ");
    ZW_UART0_tx_send_w_num(handleFirmWareIdGet(0));
    ZW_UART0_tx_send_str((BYTE*)"\r\n Version: ");
    handleGetFirmwareVersion( 0, (VG_VERSION_REPORT_V2_VG*)tempString ); // tempString[0,1] = APP_VERSION;
    tempString[0] += 0x30; //convert to ASCII
    x = tempString[1];
    tempString[1] = '.';
    tempString[2] = 0x30 + x/10;
    tempString[3] = 0x30 + x%10;
    tempString[4] = '\r';
    tempString[5] = '\n';
    tempString[6] = 0; // line end
    ZW_UART0_tx_send_str(tempString);
  }
//#endif
#ifdef IO_ZDP03A_HELP
  else if (!memcmp(pString, "help", 4))
  {
    ZW_UART0_tx_send_nl();
    ZW_UART0_tx_send_str((BYTE*)"*************************************\r\n");
    ZW_UART0_tx_send_str((BYTE*)"io_zdp03a command descriptions.\r\n");
    ZW_UART0_tx_send_str((BYTE*)"Desc:\t>'chan' 'cmd'\\r\t-> chan~channel, cmd~commands\r\n");
    ZW_UART0_tx_send_str((BYTE*)"Eks:\t>a k1p\t\t-> a~application, k1p~key1Press\r\n");
    ZW_UART0_tx_send_nl();
    ZW_UART0_tx_send_str((BYTE*)"Commmands:\r\n");
    ZW_UART0_tx_send_str((BYTE*)"-help:\r\n");
    ZW_UART0_tx_send_str((BYTE*)"\tcmd[0..3]\t= help\r\n");
    ZW_UART0_tx_send_str((BYTE*)"-Reset:\r\n");
    ZW_UART0_tx_send_str((BYTE*)"\tcmd[0..4]\t= reset\r\n");
    ZW_UART0_tx_send_str((BYTE*)"-LearnMode start:\r\n");
    ZW_UART0_tx_send_str((BYTE*)"\tcmd[0..2]\t= lms\r\n");
    ZW_UART0_tx_send_str((BYTE*)"-LearnMode end:\r\n");
    ZW_UART0_tx_send_str((BYTE*)"\tcmd[0..2]\t= lme\r\n");
    ZW_UART0_tx_send_str((BYTE*)"-Watchdog reset:\r\n");
    ZW_UART0_tx_send_str((BYTE*)"\tcmd[0..2]\t= wdr\r\n");
    ZW_UART0_tx_send_str((BYTE*)"-Get application state:\r\n");
    ZW_UART0_tx_send_str((BYTE*)"\tcmd[0..2]\t= ast\r\n");
    ZW_UART0_tx_send_str((BYTE*)"-Get IO pin:\r\n");
    ZW_UART0_tx_send_str((BYTE*)"\tcmd[0..5]\t= getpin\r\n");
    ZW_UART0_tx_send_str((BYTE*)"-Zw device firmware-Id & -version:\r\n");
    ZW_UART0_tx_send_str((BYTE*)"\tcmd[0,1]\t= ZW\r\n");
    ZW_UART0_tx_send_str((BYTE*)"-Key :\r\n");
    ZW_UART0_tx_send_str((BYTE*)"\tcmd[0]\t\t= k (k)ey\r\n");
    ZW_UART0_tx_send_str((BYTE*)"\tcmd[1]\t\t= 1..6 key number\r\n");
    ZW_UART0_tx_send_str((BYTE*)"\tcmd[2,3]\t= d (d)own\r\n");
    ZW_UART0_tx_send_str((BYTE*)"\t\t\t= u (u)p\r\n");
    ZW_UART0_tx_send_str((BYTE*)"\t\t\t= p   -> (p)ress\r\n");
    ZW_UART0_tx_send_str((BYTE*)"\t\t\t= h   -> (h)old\r\n");
    ZW_UART0_tx_send_str((BYTE*)"\t\t\t= tp  -> (t)riple (p)ress\r\n");
    ZW_UART0_tx_send_str((BYTE*)"\t\t\t= t   -> (t)riple press\r\n");
    ZW_UART0_tx_send_str((BYTE*)"\t\t\t= h   -> (h)old\r\n");
    ZW_UART0_tx_send_str((BYTE*)"\t\t\t= 10h -> 10 sec (h)old\r\n");
    ZW_UART0_tx_send_str((BYTE*)"*************************************\r\n");
  }
  else
  {
    ZW_UART0_tx_send_str((BYTE*)"\r\nUnknown command! >a help\r\n");
  }
#endif /* IO_ZDP03A_HELP */
}
#endif /*TEST_INTERFACE_SUPPORT*/

void
ti_csa_prompt(void)
{
#ifdef TEST_INTERFACE_SUPPORT
  ZW_UART0_tx_send_nl();
  ZW_UART0_tx_send_str((BYTE*)"Type 'a csa <10 digit key>' and press Enter.");
  ZW_UART0_tx_send_nl();
  ZW_UART0_tx_send_str((BYTE*)"> ");
#endif
}

/********************************  ZW_portpin.c  ****************************
 *
 *          Z-Wave, the wireless language.
 *
 *              Copyright (c) 2011
 *              Sigma Designs, Inc
 *
 *              All Rights Reserved
 *
 *    This source file is subject to the terms and conditions of the
 *    Sigma Designs Software License Agreement which restricts the manner
 *    in which it may be used.
 *
 *---------------------------------------------------------------------------
 *
 * Description: ZW040x Port monitor service functions module
 *
 * Last Changed By:  $Author: $
 * Revision:         $Revision: $
 * Last Changed:     $Date: $
 *
 ****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include <ZW_typedefs.h>
#include <ZW_portpin_api.h>
#include <port_monitor.h>
#include <ZW_ev_scheduler.h>
#include <misc.h>

#include <ZW_uart_api.h>
#include <ZW_debug_api.h>

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
#ifdef ZW_DEBUG_PORT_MON
#define ZW_DEBUG_PORT_MON_SEND_BYTE(val) ZW_UART_SEND_BYTE(val)
#define ZW_DEBUG_PORT_MON_SEND_NUM(val) ZW_UART_SEND_NUM(val)
#define ZW_DEBUG_PORT_MON_SEND_STR(val) ZW_UART_SEND_STR(val)
#define ZW_DEBUG_PORT_MON_SEND_NL() ZW_UART_SEND_NL()
#else
#define ZW_DEBUG_PORT_MON_SEND_BYTE(val)
#define ZW_DEBUG_PORT_MON_SEND_NUM(val)
#define ZW_DEBUG_PORT_MON_SEND_STR(val)
#define ZW_DEBUG_PORT_MON_SEND_NL()
#endif



typedef struct _PORT_MONITOR
{
  VOID_CALLBACKFUNC(pEventHandler)(WORD, XBYTE*, BYTE);
  BYTE monitorHandle;
  BYTE portIn[PORT_GRP];
  BYTE portOut[PORT_GRP];
  BYTE portPost[PORT_GRP];
  BYTE maskDebounce[PORT_GRP];
  BYTE portDebounce[PORT_GRP];
  BYTE debounceHandle;
  BYTE debounceCount;
} PORT_MONITOR;

BYTE portR[4] = {0, 0, 0, 0};

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/
static PORT_MONITOR xdata myPortMonitor = {NULL, 0, /* pEventHandler & monitorHandle*/
                            0, 0, 0, 0,             /* portIn*/
                            0, 0, 0, 0,             /* portOut*/
                            0, 0, 0, 0,             /* portPost*/
                            0, 0, 0, 0,             /* maskDebounce*/
                            0, 0, 0, 0,             /* portDebounce*/
                            0, 0};                  /* debounceHandle, debounceCount*/

  tPORT_STATUS port;
/****************************************************************************/
/*                              PRIVATE FUNCTIONS                           */
/****************************************************************************/
void MapPin2Bit( BYTE* pBits, ENUM_PORTPINS bPortPin, BOOL bVal);
/****************************************************************************/
/*                              EXPORTED FUNCTIONS                          */
/****************************************************************************/



/*==============================   MapPin2Bit   ===========================
** Function used to map port numbers to bit 0 to 31
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void MapPin2Bit( BYTE* pBits, ENUM_PORTPINS bPortPin, BOOL bVal)
{
  /* Port 0-3 are placed 0x80, 0x90, 0xA0 and 0xB0 */
  switch (bPortPin & 0x70)
  {
    /* Port 0 */
    case 0x00:
      if (bVal)
      {
        *(pBits)|= 0x01 << (bPortPin & 0x07);
      }
      else
      {
        *(pBits) &= ~(0x01 << (bPortPin & 0x07));
      }
      break;

    /* Port 1 */
    case 0x10:
      if (bVal)
      {
        *(pBits + 1)|= 0x01 << (bPortPin & 0x07);
      }
      else
      {
        *(pBits + 1) &= ~(0x01 << (bPortPin & 0x07));
      }
      break;

    /* Port 2 */
    case 0x20:
      if (bVal)
      {
        *(pBits + 2)|= 0x01 << (bPortPin & 0x07);
      }
      else
      {
        *(pBits + 2) &= ~(0x01 << (bPortPin & 0x07));
      }
      break;

    /* Port 3 */
    case 0x30:
      if (bVal)
      {
        *(pBits + 3)|= 0x01 << (bPortPin & 0x07);
      }
      else
      {
        *(pBits + 3) &= ~(0x01 << (bPortPin & 0x07));
      }
      break;

    default:
      break;
  }
}


/*==============================   ZW_PortMonitorInit     ===========================
** Function used to setup a function pointer to handle pin in events. Callback
** is per default disabled or can be disable by calling ZW_PortInit(NULL).
** ZW_PortMonitorInit is both CONSTRUCTOR and DESTRUCTOR.
**    Side effects: None
**
**--------------------------------------------------------------------------*/
void
ZW_PortMonitorInit( VOID_CALLBACKFUNC(pEventHandler)(WORD, XBYTE*, BYTE)) /* callback function pointer*/
{
  BYTE i;

  /*init data struct*/
  for(i = 0; i < PORT_GRP; i++)
  {
    myPortMonitor.portIn[i]       = 0;
    myPortMonitor.portOut[i]      = 0;
    myPortMonitor.maskDebounce[i] = 0;
    myPortMonitor.portDebounce[i] = 0;
  }

  /*Set portPost to current value of port to secure no events is sent first
  time an in/out-port is enabled*/
  myPortMonitor.portPost[0] = ZW_PortGet(PORTPIN_P0);
  myPortMonitor.portPost[1] = ZW_PortGet(PORTPIN_P1);
  myPortMonitor.portPost[2] = ZW_PortGet(PORTPIN_P2);
  myPortMonitor.portPost[3] = ZW_PortGet(PORTPIN_P3);

  if(0 != myPortMonitor.monitorHandle)
  {
    /* remove Portmonitor from EventMonitor*/
    EventSchedulerRemove(&myPortMonitor.monitorHandle);
    myPortMonitor.monitorHandle = 0;
  }
  myPortMonitor.pEventHandler = pEventHandler;

  /*Debug info*/
  ZW_DEBUG_SEND_STR("PortMonInit:");
  if(NON_NULL(pEventHandler))
  {
    ZW_DEBUG_SEND_STR("Enable");
  }
  else
  {
    ZW_DEBUG_SEND_STR("Disable");
  }
  ZW_DEBUG_PORT_MON_SEND_NL();
}


/*==============================   ZCB_PortDebounceMonitor     ===============
** Function used to debounce check
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
PCB(ZCB_PortDebounceMonitor)(void)
{
  if(2 < myPortMonitor.debounceCount++)
  {
    /* port in mask*/
    port.mask[0] = myPortMonitor.maskDebounce[0];
    port.mask[1] = myPortMonitor.maskDebounce[1];
    port.mask[2] = myPortMonitor.maskDebounce[2];
    port.mask[3] = myPortMonitor.maskDebounce[3];
    /* port in status*/
    port.status[0] = myPortMonitor.portDebounce[0];
    port.status[1] = myPortMonitor.portDebounce[1];
    port.status[2] = myPortMonitor.portDebounce[2];
    port.status[3] = myPortMonitor.portDebounce[3];

    ZW_DEBUG_PORT_MON_SEND_STR("PMonEv:");
    ZW_DEBUG_PORT_MON_SEND_NUM(port.status[2]);
    ZW_DEBUG_PORT_MON_SEND_NUM(port.mask[2]);
    ZW_DEBUG_PORT_MON_SEND_NL();
    /*send function pointer with in-data*/
    myPortMonitor.pEventHandler(((ID_EVENT_PIO_PORT << 8) | ID_PORTIN_CHANGE),
                                 &(port), (BYTE)sizeof(port));

    EventSchedulerRemove(&myPortMonitor.debounceHandle);
   }

}




/*==============================   ZW_PortPinSet   ===========================
** Function used to Monitor ports and is called by EventScheduler. event is
** send if port status is changed.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
PCB(ZCB_PortMonitor)(void)
{
  if(NON_NULL( myPortMonitor.pEventHandler ))
  {
    portR[0] = ZW_PortGet(PORTPIN_P0) & (myPortMonitor.portIn[0] | myPortMonitor.portOut[0]);
    portR[1] = ZW_PortGet(PORTPIN_P1) & (myPortMonitor.portIn[1] | myPortMonitor.portOut[1]);
    portR[2] = ZW_PortGet(PORTPIN_P2) & (myPortMonitor.portIn[2] | myPortMonitor.portOut[2]);
    portR[3] = ZW_PortGet(PORTPIN_P3) & (myPortMonitor.portIn[3] | myPortMonitor.portOut[3]);

    /*check port has change status*/
    if ((0 != (myPortMonitor.portPost[0] ^ portR[0])) ||
        (0 != (myPortMonitor.portPost[1] ^ portR[1])) ||
        (0 != (myPortMonitor.portPost[2] ^ portR[2])) ||
        (0 != (myPortMonitor.portPost[3] ^ portR[3])))
    {

      ZW_DEBUG_PORT_MON_SEND_STR("Mon:");
      /*Check port in has change status*/
      if (((myPortMonitor.portPost[0] ^ portR[0]) &  myPortMonitor.portIn[0]) ||
          ((myPortMonitor.portPost[1] ^ portR[1]) &  myPortMonitor.portIn[1]) ||
          ((myPortMonitor.portPost[2] ^ portR[2]) &  myPortMonitor.portIn[2]) ||
          ((myPortMonitor.portPost[3] ^ portR[3]) &  myPortMonitor.portIn[3]))
      {

        /*DEBOUNCE DETECTION*/
         ZW_DEBUG_PORT_MON_SEND_STR("IN");
        if(0 == myPortMonitor.debounceHandle)
        {
         ZW_DEBUG_PORT_MON_SEND_STR(":NEW");
          /*Setup debounce check*/
          /*Register in port pins*/
          myPortMonitor.portDebounce[0] = portR[0] & myPortMonitor.portIn[0];
          myPortMonitor.portDebounce[1] = portR[1] & myPortMonitor.portIn[1];
          myPortMonitor.portDebounce[2] = portR[2] & myPortMonitor.portIn[2];
          myPortMonitor.portDebounce[3] = portR[3] & myPortMonitor.portIn[3];

          /*Register in port mask*/
          myPortMonitor.maskDebounce[0] = (myPortMonitor.portPost[0] ^ portR[0]) & myPortMonitor.portIn[0];
          myPortMonitor.maskDebounce[1] = (myPortMonitor.portPost[1] ^ portR[1]) & myPortMonitor.portIn[1];
          myPortMonitor.maskDebounce[2] = (myPortMonitor.portPost[2] ^ portR[2]) & myPortMonitor.portIn[2];
          myPortMonitor.maskDebounce[3] = (myPortMonitor.portPost[3] ^ portR[3]) & myPortMonitor.portIn[3];

          /*reset count*/
          myPortMonitor.debounceCount = 0;

          /*Add ZCB_PortDebounceMonitor to eventScheduler*/
          myPortMonitor.debounceHandle = EventSchedulerAdd(&ZCB_PortDebounceMonitor);
        }
        else
        {
          BYTE i;
          for (i = 0; i < 4; i++)
          {
            /* check  pin's has changed*/
            if ( myPortMonitor.portDebounce[i] != (myPortMonitor.portIn[i] & portR[i]))
            {
              /*same mask pin has changes*/
              if ((myPortMonitor.maskDebounce[i] & myPortMonitor.portDebounce[i]) !=
                 (myPortMonitor.maskDebounce[i] & (myPortMonitor.portIn[i] & portR[i])))
              {
                BYTE maskRemove = (myPortMonitor.maskDebounce[i] & myPortMonitor.portDebounce[i]) ^
                                    (myPortMonitor.maskDebounce[i] & (myPortMonitor.portIn[i] & portR[i]));
                 ZW_DEBUG_PORT_MON_SEND_STR(":SAME");
                /*DEBUG*/

                myPortMonitor.portDebounce[i] = ~(myPortMonitor.portDebounce[i] & maskRemove) & /*toogle port mask */
                                                (myPortMonitor.portDebounce[i] & ~maskRemove);/*mask  other bits*/
                myPortMonitor.maskDebounce[i] &= ~maskRemove;

              }
              else
              {
                /* check other pin's has changed. Add new pin's and reset counter!*/
                BYTE maskAdd = myPortMonitor.portDebounce[i] ^ (myPortMonitor.portIn[i] & portR[i]);
                ZW_DEBUG_PORT_MON_SEND_STR("(:OTHER)");
                myPortMonitor.portDebounce[i] = myPortMonitor.portIn[i] & portR[i];
                myPortMonitor.maskDebounce[i] |= maskAdd;
                myPortMonitor.debounceCount = 0;
              }
            }
          }
          /*Check if monitor should be removed*/
          if( (0 == myPortMonitor.maskDebounce[0]) & (0 == myPortMonitor.maskDebounce[1]) &
              (0 == myPortMonitor.maskDebounce[2]) & (0 == myPortMonitor.maskDebounce[3]))
          {
            if (FALSE == EventSchedulerRemove(&myPortMonitor.debounceHandle))
            {
              ZW_DEBUG_PORT_MON_SEND_STR(":REMOVE");;/*something went wrong!*/
            }
            /*reset count*/
            myPortMonitor.debounceCount = 0;
          }
        }
      }

      /*Check port out has change status*/
      if (((myPortMonitor.portPost[0] ^ portR[0]) &  myPortMonitor.portOut[0]) ||
          ((myPortMonitor.portPost[1] ^ portR[1]) &  myPortMonitor.portOut[1]) ||
          ((myPortMonitor.portPost[2] ^ portR[2]) &  myPortMonitor.portOut[2]) ||
          ((myPortMonitor.portPost[3] ^ portR[3]) &  myPortMonitor.portOut[3]))
      {
         ZW_DEBUG_PORT_MON_SEND_STR(":OUT=");

        /* port in mask*/
        port.mask[0] = (myPortMonitor.portPost[0] ^ portR[0]) &  myPortMonitor.portOut[0];
        port.mask[1] = (myPortMonitor.portPost[1] ^ portR[1]) &  myPortMonitor.portOut[1];
        port.mask[2] = (myPortMonitor.portPost[2] ^ portR[2]) &  myPortMonitor.portOut[2];
        port.mask[3] = (myPortMonitor.portPost[3] ^ portR[3]) &  myPortMonitor.portOut[3];
        /* port in status*/
        port.status[0] = portR[0] & myPortMonitor.portOut[0];
        port.status[1] = portR[1] & myPortMonitor.portOut[1];
        port.status[2] = portR[2] & myPortMonitor.portOut[2];
        port.status[3] = portR[3] & myPortMonitor.portOut[3];

        ZW_DEBUG_PORT_MON_SEND_NUM(port.status[2]);
        ZW_DEBUG_PORT_MON_SEND_NUM(port.mask[2]);
        ZW_DEBUG_PORT_MON_SEND_NL();
        /*send function pointer with in-data*/
        myPortMonitor.pEventHandler(((ID_EVENT_PIO_PORT << 8) | ID_PORTOUT_CHANGE),
                                     &(port), (BYTE)sizeof(port));
      }
      /*Set port post*/
      myPortMonitor.portPost[0] = portR[0];
      myPortMonitor.portPost[1] = portR[1];
      myPortMonitor.portPost[2] = portR[2];
      myPortMonitor.portPost[3] = portR[3];
      ZW_DEBUG_PORT_MON_SEND_STR("-post");
      ZW_DEBUG_PORT_MON_SEND_NL();
    }
  }
  else
  {
    EventSchedulerRemove(&myPortMonitor.monitorHandle);
    myPortMonitor.monitorHandle = 0;
  }
}


/*===============================   ZW_PortMonitorPinIn  =====================
**    Setup on bPortPin portpin as Input and monitor it.
**
**--------------------------------------------------------------------------*/
void
ZW_PortMonitorPinIn(
  ENUM_PORTPINS bPortPin)
{
  register BYTE curVal;

  ZW_DEBUG_PORT_MON_SEND_STR("ZW_PortMonitorPinIn=");
  ZW_DEBUG_PORT_MON_SEND_NUM(bPortPin);
  ZW_DEBUG_PORT_MON_SEND_NUM(myPortMonitor.portIn[0]);
  ZW_DEBUG_PORT_MON_SEND_NUM(myPortMonitor.portIn[1]);
  ZW_DEBUG_PORT_MON_SEND_NUM(myPortMonitor.portIn[2]);
  ZW_DEBUG_PORT_MON_SEND_NUM(myPortMonitor.portIn[3]);
  ZW_DEBUG_PORT_MON_SEND_NUM(myPortMonitor.portOut[0]);
  ZW_DEBUG_PORT_MON_SEND_NUM(myPortMonitor.portOut[1]);
  ZW_DEBUG_PORT_MON_SEND_NUM(myPortMonitor.portOut[2]);
  ZW_DEBUG_PORT_MON_SEND_NUM(myPortMonitor.portOut[3]);
  ZW_DEBUG_PORT_MON_SEND_NL();
  MapPin2Bit( &myPortMonitor.portIn[0], bPortPin, TRUE);
  MapPin2Bit( &myPortMonitor.portOut[0], bPortPin, FALSE);

  if((NULL != myPortMonitor.pEventHandler) & (0 == myPortMonitor.monitorHandle))
  {
    /*Add PortMonitor to eventScheduler*/
    myPortMonitor.monitorHandle = EventSchedulerAdd(&ZCB_PortMonitor);
  }

  ZW_DEBUG_PORT_MON_SEND_STR("monitorHandle=");
  ZW_DEBUG_PORT_MON_SEND_NUM(myPortMonitor.monitorHandle);
  ZW_DEBUG_PORT_MON_SEND_NUM(myPortMonitor.portIn[0]);
  ZW_DEBUG_PORT_MON_SEND_NUM(myPortMonitor.portIn[1]);
  ZW_DEBUG_PORT_MON_SEND_NUM(myPortMonitor.portIn[2]);
  ZW_DEBUG_PORT_MON_SEND_NUM(myPortMonitor.portIn[3]);
  ZW_DEBUG_PORT_MON_SEND_NUM(myPortMonitor.portOut[0]);
  ZW_DEBUG_PORT_MON_SEND_NUM(myPortMonitor.portOut[1]);
  ZW_DEBUG_PORT_MON_SEND_NUM(myPortMonitor.portOut[2]);
  ZW_DEBUG_PORT_MON_SEND_NUM(myPortMonitor.portOut[3]);
  ZW_DEBUG_PORT_MON_SEND_NL();
  ZW_PortPinIn(bPortPin);
}


/*===============================   ZW_PortMonitorPinOut  ====================
**    Setup bPortPin portpin as Output and disable portpin monitor if pin was
**    an input pin.
**
**--------------------------------------------------------------------------*/
void
ZW_PortMonitorPinOut(
  ENUM_PORTPINS bPortPin)
{
  register BYTE curVal;

  ZW_DEBUG_PORT_MON_SEND_STR("ZW_PortMonitorPinOut=");
  ZW_DEBUG_PORT_MON_SEND_NUM(bPortPin);
  ZW_DEBUG_PORT_MON_SEND_NUM(myPortMonitor.portIn[0]);
  ZW_DEBUG_PORT_MON_SEND_NUM(myPortMonitor.portIn[1]);
  ZW_DEBUG_PORT_MON_SEND_NUM(myPortMonitor.portIn[2]);
  ZW_DEBUG_PORT_MON_SEND_NUM(myPortMonitor.portIn[3]);
  ZW_DEBUG_PORT_MON_SEND_NUM(myPortMonitor.portOut[0]);
  ZW_DEBUG_PORT_MON_SEND_NUM(myPortMonitor.portOut[1]);
  ZW_DEBUG_PORT_MON_SEND_NUM(myPortMonitor.portOut[2]);
  ZW_DEBUG_PORT_MON_SEND_NUM(myPortMonitor.portOut[3]);
  ZW_DEBUG_PORT_MON_SEND_NL();
  MapPin2Bit( &myPortMonitor.portIn[0], bPortPin, FALSE);
  MapPin2Bit( &myPortMonitor.portOut[0], bPortPin, TRUE);

  if((NULL != myPortMonitor.pEventHandler) & (0 == myPortMonitor.monitorHandle))
  {
    /*Add PortMonitor to eventScheduler*/
    myPortMonitor.monitorHandle = EventSchedulerAdd(&ZCB_PortMonitor);
  }
  ZW_DEBUG_PORT_MON_SEND_STR("monitorHandle=");
  ZW_DEBUG_PORT_MON_SEND_NUM(myPortMonitor.monitorHandle);
  ZW_DEBUG_PORT_MON_SEND_NUM(myPortMonitor.portIn[0]);
  ZW_DEBUG_PORT_MON_SEND_NUM(myPortMonitor.portIn[1]);
  ZW_DEBUG_PORT_MON_SEND_NUM(myPortMonitor.portIn[2]);
  ZW_DEBUG_PORT_MON_SEND_NUM(myPortMonitor.portIn[3]);
  ZW_DEBUG_PORT_MON_SEND_NUM(myPortMonitor.portOut[0]);
  ZW_DEBUG_PORT_MON_SEND_NUM(myPortMonitor.portOut[1]);
  ZW_DEBUG_PORT_MON_SEND_NUM(myPortMonitor.portOut[2]);
  ZW_DEBUG_PORT_MON_SEND_NUM(myPortMonitor.portOut[3]);
  ZW_DEBUG_PORT_MON_SEND_NL();

  ZW_PortPinOut(bPortPin);
}


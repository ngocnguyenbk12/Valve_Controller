/***************************************************************************
*
* Copyright (c) 2001-2013
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: This module implements functions used in combination with
*              command class firmware update.
*
* Author: Samer  Seoud
*
* Last Changed By: $Author: tro $
* Revision: $Revision: 0.00 $
* Last Changed: $Date: 2013/07/02 14:03:30 $
*
****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include <ZW_sysdefs.h>
#include <ZW_tx_mutex.h>
#ifdef ZW_CONTROLLER
/*These are a part of the standard static controller API*/
#include <ZW_controller_api.h>
#endif
/* Enhanced Slave - needed for battery operation (RTC timer) on 100 series */
/* 200 Series have WUT */
#ifdef ZW_SLAVE_32
#include <ZW_slave_32_api.h>
#else
#ifdef  ZW_SLAVE
#include <ZW_slave_api.h>
#endif
#endif
/* ASIC power management functionality */
#include <ZW_power_api.h>
/* Allows data storage of application data even after reset */
#include <ZW_non_zero.h>
#include <eeprom.h>
#include <ZW_TransportLayer.h>

#include <ZW_crc.h>
#include <ZW_firmware_descriptor.h>
#include <ZW_firmware_bootloader_defs.h>
#include <ZW_firmware_update_nvm_api.h>

#include <ZW_uart_api.h>
#include <misc.h>

#include <CommandClassFirmwareUpdate.h>
#include <ota_util.h>
#include <ZW_ota_compression_header.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

#ifdef ZW_DEBUG_CMD_OTA

#define ZW_DEBUG_CMD_OTA_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_CMD_OTA_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_CMD_OTA_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_CMD_OTA_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_CMD_OTA_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_CMD_OTA_SEND_BYTE(data)
#define ZW_DEBUG_CMD_OTA_SEND_STR(STR)
#define ZW_DEBUG_CMD_OTA_SEND_NUM(data)
#define ZW_DEBUG_CMD_OTA_SEND_WORD_NUM(data)
#define ZW_DEBUG_CMD_OTA_SEND_NL()
#endif

#ifdef ZW_DEBUG_SECURITY
#define ZW_DEBUG_SECURITY_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_SECURITY_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_SECURITY_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_SECURITY_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_SECURITY_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_SECURITY_SEND_BYTE(data)
#define ZW_DEBUG_SECURITY_SEND_STR(STR)
#define ZW_DEBUG_SECURITY_SEND_NUM(data)
#define ZW_DEBUG_SECURITY_SEND_WORD_NUM(data)
#define ZW_DEBUG_SECURITY_SEND_NL()
#endif


typedef enum _FW_STATE_
{
  FW_STATE_DISABLE,
  FW_STATE_READY,
  FW_STATE_ACTIVE,
} FW_STATE;

typedef enum _FW_EV_
{
  FW_EV_FORCE_DISABLE,
  FW_EV_VALID_COMBINATION,
  FW_EV_MD_REQUEST_REPORT_SUCCESS,
  FW_EV_MD_REQUEST_REPORT_FAILED,
  FW_EV_GET_NEXT_FW_FRAME,
  FW_EV_RETRY_NEXT_FW_FRAME,
  FW_EV_UPDATE_STATUS_SUCCESS,
  FW_EV_UPDATE_STATUS_SUCCESS_USER_REBOOT,
  FW_EV_UPDATE_STATUS_UNABLE_TO_RECEIVE,
  FW_EV_UPDATE_STATUS_CRC_ERROR

} FW_EV;

#define FIRMWARE_UPDATE_REQUEST_PACKET_TIMEOUT  1000   /* unit: 10 ms ticks */

#define FIRMWARE_UPDATE_REQEUST_TIMEOUT_UNIT    200    /* unit: 10 ms ticks per sub-timeout */

#define HOST_REPONSE_TIMEOUT                    200    /* unit: 10 ms ticks */

/* number of sub-timeouts to achieve total of FIRMWARE_UPDATE_REQUEST_PACKET_TIMEOUT */
#define FIRMWARE_UPDATE_REQUEST_TIMEOUTS        (FIRMWARE_UPDATE_REQUEST_PACKET_TIMEOUT / FIRMWARE_UPDATE_REQEUST_TIMEOUT_UNIT)

#define FIRMWARE_UPDATE_MAX_RETRY 10

/*
 * Maximum fragment size definitions.
 */
#define S2_ENCAPSULATION_LENGTH 12
#define FIRMWARE_UPDATE_MD_REPORT_ENCAPSULATION_LENGTH 6
#define S0_ENCAPSULATION_LENGTH 20
#define S0_SEQUENCING_FACTOR 2


typedef enum _ST_DATA_WRITE_TO_HOST_
{
  ST_DATA_WRITE_PROGRESS,       //!< ST_DATA_WRITE_PROGRESS
  ST_DATA_WRITE_LAST,           //!< ST_DATA_WRITE_LAST
  ST_DATA_WRITE_WAIT_HOST_STATUS//!< ST_DATA_WRITE_WAIT_HOST_STATUS
} ST_DATA_WRITE_TO_HOST;

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/
static BYTE firmware_update_packetsize = 0;

ST_DATA_WRITE_TO_HOST st_data_write_to_host;
typedef struct _OTA_UTIL_
{
  BOOL (CODE *pOtaStart)(WORD fwId, WORD CRC);
  VOID_CALLBACKFUNC(pOtaExtWrite)( BYTE *pData, BYTE dataLen);
  VOID_CALLBACKFUNC(pOtaFinish)(BYTE val);
  FW_STATE fwState;
  OTA_STATUS finishStatus;
  WORD firmwareCrc;
  BYTE fw_numOfRetries;
  BYTE timerFwUpdateFrameGetHandle;
  WORD firmwareUpdateReportNumberPrevious;
  WORD fw_crcrunning;
  BYTE timerFWUpdateCount;
  BYTE fwExtern;
  BYTE handleHostResponse;
  BYTE userReboot;
  RECEIVE_OPTIONS_TYPE_EX rxOpt;
  BYTE NVM_valid;
} OTA_UTIL;

OTA_UTIL myOta = {
    NULL, // pOtaStart
    NULL, // pOtaExtWrite
    NULL, // pOtaFinish
    FW_STATE_DISABLE,
    OTA_STATUS_DONE,
    0, // firmwareCrc
    0, // fw_numOfRetries
    0, // timerFwUpdateFrameGetHandle
    0, // firmwareUpdateReportNumberPrevious
    0, // fw_crcrunning
    0, // timerFWUpdateCount
    0, // fwExtern
    0, // handleHostResponse
    0, // userReboot
    {0}, // rxOpt
    0  // NVM_valid
};

static uint8_t m_requestGetStatus = FIRMWARE_UPDATE_MD_REQUEST_REPORT_VALID_COMBINATION_V4;

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/
void InitEvState();
//WORD NVMCheckCRC16( WORD crc, DWORD nvmOffset, WORD blockSize);
void SetEvState( FW_EV ev);
void TimerCancelFwUpdateFrameGet();
void TimerStartFwUpdateFrameGet();
void ZCB_TimerOutFwUpdateFrameGet();
void ZCB_FinishFwUpdate(TRANSMISSION_RESULT * pTransmissionResult);
void Reboot();
void OTA_WriteData(DWORD offset, BYTE* pData, WORD legth);
void OTA_Invalidate();
void TimerCancelHostResponse();
void TimerStartHostResponse();
void ZCB_TimerTimeoutHostResponse();
void ZCB_UpdateStatusSuccess(void);
void SendFirmwareUpdateStatusReport(BYTE status);

/*============================ Reboot ===============================
** Function description
** Reboot the node by enabling watchdog.
**
** Side effects:
****------------------------------------------------------------------*/
void
Reboot()
{
#ifdef WATCHDOG_ENABLED
  ZW_WatchDogEnable();
#endif
  while(1);
}


/*============================ OtaInit ===============================
** Function description
** Init Ota transmit options.
**
** Side effects:
**
**-------------------------------------------------------------------------*/
BYTE
OtaInit(
  BOOL (CODE *pOtaStart)(WORD fwId, WORD CRC),
  VOID_CALLBACKFUNC(pOtaExtWrite)(   BYTE *pData, BYTE dataLen),
  VOID_CALLBACKFUNC(pOtaFinish)(BYTE val))
{
  myOta.pOtaStart = pOtaStart;
  myOta.pOtaExtWrite = pOtaExtWrite;
  myOta.pOtaFinish = pOtaFinish;
  myOta.handleHostResponse = 0;
  myOta.NVM_valid = ZW_FirmwareUpdate_NVM_Init();
  return myOta.NVM_valid;
}


/*============================ OTA_WriteData ===============================
** Function description
** Write data to the OTA NVM
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
OTA_WriteData(DWORD offset, BYTE *pData, WORD length)
{
  ZW_FirmwareUpdate_NVM_Write(pData, length, offset);
}


/*============================ OTA_Invalidate ===============================
** Function description
** Invalidate the FW update NVM to write a new image
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
OTA_Invalidate()
{
  /* Mark possible current NVM Image Not Valid */
  ZW_FirmwareUpdate_NVM_Set_NEWIMAGE(0);
}

/*======================== ZCB_UpdateStatusSuccess ==========================
** Timer callback to start CRC calculation *after* we have ack/routed-ack'ed
** the last fw update frame.
**
** Side effects:
**
**-------------------------------------------------------------------------*/
PCB(ZCB_UpdateStatusSuccess)(void)
{
  ZW_DEBUG_SECURITY_SEND_STR("OTA_SUCCESS_CB");
  if( FALSE == myOta.userReboot)
  {
    /* send FIRMWARE_UPDATE_MD_STATUS_REPORT_SUCCESSFULLY_V4 to controller.
       Device reboot itself*/
    SetEvState(FW_EV_UPDATE_STATUS_SUCCESS);
  }
  else
  {
    /* send FIRMWARE_UPDATE_MD_STATUS_REPORT_SUCCESSFULLY_STORED_V4 to controller.
       User need to reboot device*/
    SetEvState(FW_EV_UPDATE_STATUS_SUCCESS_USER_REBOOT);
  }
  myOta.userReboot = FALSE;
}


/*============================ handleCmdClassFirmwareUpdateMdReport ===============================
** Function description
** This function...
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
handleCmdClassFirmwareUpdateMdReport( WORD crc16Result,
                                      WORD firmwareUpdateReportNumber,
                                      BYTE properties,
                                      BYTE* pData,
                                      BYTE fw_actualFrameSize)
{

/*  ZW_DEBUG_CMD_OTA_SEND_STR("UpdateMdReport");
  ZW_DEBUG_CMD_OTA_SEND_NUM( crc16Result);
  ZW_DEBUG_CMD_OTA_SEND_NUM( firmwareUpdateReportNumber);
  ZW_DEBUG_CMD_OTA_SEND_NUM( properties);
  ZW_DEBUG_CMD_OTA_SEND_NUM( fw_actualFrameSize);
  ZW_DEBUG_CMD_OTA_SEND_NL();
*/
  /*Check correct state*/
  if( FW_STATE_ACTIVE != myOta.fwState)
  {
    /*Not correct state.. just stop*/
    return;
  }

  /*Check checksum*/
  if (0 == crc16Result)
  {
    //ZW_DEBUG_CMD_OTA_SEND_BYTE('C');
    myOta.fw_numOfRetries = 0;
    /* Check report number */
    if (firmwareUpdateReportNumber == myOta.firmwareUpdateReportNumberPrevious + 1)
    {
      /* Right number*/
      DWORD firstAddr = 0;
      if (0 == myOta.firmwareUpdateReportNumberPrevious)
      {
        /* First packet sets the packetsize for the whole firmware update transaction */
        /* TODO: Make negativ response if packetsize too big... */
        //firmware_update_packetsize = fw_actualFrameSize;
      }
      else
      {
        if ((firmware_update_packetsize != fw_actualFrameSize) && (!(properties & FIRMWARE_UPDATE_MD_REPORT_PROPERTIES1_LAST_BIT_MASK)))
        {
          ZW_DEBUG_SEND_BYTE('%');
          ZW_DEBUG_SEND_WORD_NUM(firmware_update_packetsize);
          ZW_DEBUG_SEND_BYTE('-');
          ZW_DEBUG_SEND_WORD_NUM(fw_actualFrameSize);
          /* (firmware_update_packetsize != fw_actualFrameSize) and not last packet - do not match.. do nothing. */
          /* Let the timer handle retries */
          return;
        }
      }
      //ZW_DEBUG_CMD_OTA_SEND_BYTE('N');
      myOta.firmwareUpdateReportNumberPrevious = firmwareUpdateReportNumber;

      firstAddr = ((DWORD)(firmwareUpdateReportNumber - 1) * firmware_update_packetsize);
      myOta.fw_crcrunning = ZW_CheckCrc16(myOta.fw_crcrunning, pData, fw_actualFrameSize);

      /** Check: intern or extern Firmware update **/
      if( FALSE == myOta.fwExtern)
      {
        /** Intern firmware to update **/
        OTA_WriteData(firstAddr, pData, fw_actualFrameSize);

        /* Is this the last report ? */
        if (properties & FIRMWARE_UPDATE_MD_REPORT_PROPERTIES1_LAST_BIT_MASK)
        {
          /*check CRC for received dataBuffer*/
          if (myOta.fw_crcrunning == myOta.firmwareCrc)
          {
             ZW_DEBUG_SEND_STR("**OTA_SUCCESS_CRC**");
            /* Delay starting the CRC calculation so we can transmit
             * the ack or routed ack first */
            if(0xFF == ZW_TIMER_START(ZCB_UpdateStatusSuccess, 10, 1))
            {
              ZW_DEBUG_SEND_STR("OTA_SUCCESS_NOTIMER");
              SetEvState(FW_EV_UPDATE_STATUS_SUCCESS);
            }
          }
          else
          {
            ZW_DEBUG_SEND_STR("**OTA_FAIL CRC!!**");
            ZW_DEBUG_SEND_WORD_NUM(myOta.fw_crcrunning);
            ZW_DEBUG_SEND_BYTE('-');
            ZW_DEBUG_SEND_WORD_NUM(myOta.firmwareCrc);
            ZW_DEBUG_SEND_NL();
            SetEvState(FW_EV_UPDATE_STATUS_UNABLE_TO_RECEIVE);
          }
        }
        else
        {
          SetEvState(FW_EV_GET_NEXT_FW_FRAME);
        }
      }
      else
      {
        /** Extern firmware to update **/
        if(NON_NULL( myOta.pOtaExtWrite ))
        {
          /* Is this the last report ? */
          if(properties & FIRMWARE_UPDATE_MD_REPORT_PROPERTIES1_LAST_BIT_MASK)
          {
            /*check CRC for received dataBuffer*/
            if (myOta.fw_crcrunning == myOta.firmwareCrc)
            {
              ZW_DEBUG_SEND_STR("**OTA_SUCCESS_CRC**");
              st_data_write_to_host = ST_DATA_WRITE_LAST;
              /* Start timeout on host to response on OtaWriteFinish() */
              TimerStartHostResponse();
            }
            else
            {
              ZW_DEBUG_SEND_STR("**OTA_FAIL CRC!!**");
              ZW_DEBUG_SEND_WORD_NUM(myOta.fw_crcrunning);
              ZW_DEBUG_SEND_BYTE('-');
              ZW_DEBUG_SEND_WORD_NUM(myOta.firmwareCrc);
              ZW_DEBUG_SEND_NL();
              SetEvState(FW_EV_UPDATE_STATUS_UNABLE_TO_RECEIVE);
            }
          }
          else
          {
            st_data_write_to_host = ST_DATA_WRITE_PROGRESS;
            /* Start timeout on host to response on OtaWriteFinish() */
            TimerStartHostResponse();
          }


          myOta.pOtaExtWrite( pData, fw_actualFrameSize);

        }
        else{
          /* fail to send data to host*/
          SetEvState(FW_EV_UPDATE_STATUS_UNABLE_TO_RECEIVE);
        }
      }
    }
    else{
      ZW_DEBUG_CMD_OTA_SEND_BYTE('n');
      /* (firmwareUpdateReportNumber == myOta.firmwareUpdateReportNumberPrevious + 1) do noth match.. do nothing.
         Let the timer hande retries*/
    }

  }
  else
  {
    ZW_DEBUG_CMD_OTA_SEND_BYTE('c');
    // TODO: else we have no indication if the checksum check fails ?????
    if (FIRMWARE_UPDATE_REQUEST_TIMEOUTS < myOta.timerFWUpdateCount)
    {
      if (FIRMWARE_UPDATE_MAX_RETRY < myOta.fw_numOfRetries)
      {
        ZW_DEBUG_CMD_OTA_SEND_STR("FAILED!");
        /* Send invalid status*/
        SetEvState(FW_EV_UPDATE_STATUS_UNABLE_TO_RECEIVE);
      }
    }
  }
}

void handleCmdClassFirmwareUpdateMdReqGet(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  BYTE fwTarget,
  WORD fragmentSize,
  FW_UPDATE_GET* pData,
  BYTE* pStatus)
{
  BYTE i = 0;
  uint16_t firmwareId;
  uint16_t max_fragment_size = handleCommandClassFirmwareUpdateMaxFragmentSize();

  i = FIRMWARE_UPGRADABLE;
  if((0 == fwTarget) && (0 == i))
  {
    *pStatus = FIRMWARE_UPDATE_MD_REQUEST_REPORT_NOT_UPGRADABLE_V4;
    m_requestGetStatus = *pStatus;
    return;
  }

  if (!(fragmentSize > 0 && fragmentSize <= max_fragment_size))
  {
    *pStatus = FIRMWARE_UPDATE_MD_REQUEST_REPORT_INVALID_FRAGMENT_SIZE_V4;
    m_requestGetStatus = *pStatus;
    return;
  }

  firmwareId = handleFirmWareIdGet(fwTarget);
  if ((pData->manufacturerId != firmwareDescriptor.manufacturerID) ||
      (pData->firmwareId != firmwareId))
  {
    *pStatus = FIRMWARE_UPDATE_MD_REQUEST_REPORT_INVALID_COMBINATION_V4;
    m_requestGetStatus = *pStatus;
    return;
  }

  if (0 == myOta.NVM_valid)
  {
    *pStatus = FIRMWARE_UPDATE_MD_REQUEST_REPORT_NOT_UPGRADABLE_V4;
    m_requestGetStatus = *pStatus;
    return;
  }

  /*Firmware valid.. ask OtaStart to start update*/
  if(NON_NULL( myOta.pOtaStart ))
  {

    if(FALSE == myOta.pOtaStart(handleFirmWareIdGet(fwTarget), pData->checksum))
    {
      ZW_DEBUG_CMD_OTA_SEND_BYTE('&');
      //SetEvState(FW_EV_FORCE_DISABLE);
      *pStatus = FIRMWARE_UPDATE_MD_REQUEST_REPORT_REQUIRES_AUTHENTICATION_V4;
      m_requestGetStatus = *pStatus;
      return;
    }
  }
  InitEvState();
  memcpy( (BYTE*) &myOta.rxOpt, (BYTE*)rxOpt, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  SetEvState(FW_EV_VALID_COMBINATION);
  myOta.firmwareCrc = pData->checksum;

  if(0 != fwTarget)
  {
     myOta.fwExtern = TRUE;
  }
  else
  {
    myOta.fwExtern = FALSE;
  }

  firmware_update_packetsize = fragmentSize;
  *pStatus = FIRMWARE_UPDATE_MD_REQUEST_REPORT_VALID_COMBINATION_V4;
  m_requestGetStatus = *pStatus;
}

BOOL Ota_UpdateIsInProgress(void)
{
  return (myOta.fwState == FW_STATE_ACTIVE) ? TRUE : FALSE;
}

PCB(ZCB_CmdClassFwUpdateMdReqReport)(BYTE txStatus)
{
  if (FIRMWARE_UPDATE_MD_REQUEST_REPORT_VALID_COMBINATION_V4 != m_requestGetStatus)
  {
    // If the request get command failed, nothing was initiated. Hence, don't restart.
    return;
  }
  if(txStatus == TRANSMIT_COMPLETE_OK)
  {
    SetEvState(FW_EV_MD_REQUEST_REPORT_SUCCESS);
  }
  else{
    SetEvState(FW_EV_MD_REQUEST_REPORT_FAILED);
  }
}


/*============================ InitEvState ===============================
** Function description
** This function...
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
InitEvState()
{
  myOta.fwState = FW_STATE_DISABLE;
  myOta.fw_crcrunning = 0x1D0F;
  myOta.firmwareUpdateReportNumberPrevious = 0;
  myOta.fw_numOfRetries = 0;
  myOta.firmwareCrc = 0;
  myOta.timerFWUpdateCount = 0;
}


/*============================ SetState ===============================
** Function description
** This function...
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
SetEvState(FW_EV ev)
{
  ZW_DEBUG_CMD_OTA_SEND_STR("SetEvState ev=");
  ZW_DEBUG_CMD_OTA_SEND_NUM(ev);
  ZW_DEBUG_CMD_OTA_SEND_STR(", st=");
  ZW_DEBUG_CMD_OTA_SEND_NUM(myOta.fwState);
  ZW_DEBUG_CMD_OTA_SEND_NL();

  switch(myOta.fwState)
  {
    case FW_STATE_DISABLE:
      if(ev == FW_EV_VALID_COMBINATION)
      {
        myOta.fwState = FW_STATE_READY;
        SetEvState(ev);
      }
      else
      {
        if( NON_NULL( myOta.pOtaFinish ) )
        {
          myOta.pOtaFinish(OTA_STATUS_ABORT);
        }
        else
        {
          Reboot();
        }
      }
      break;


    case FW_STATE_READY:
      if(ev == FW_EV_FORCE_DISABLE)
      {
        /*Tell application it is aborted*/
        if( NON_NULL( myOta.pOtaFinish ) )
        {
          myOta.pOtaFinish(OTA_STATUS_ABORT);
        }
        else
        {
          Reboot();
        }
        myOta.fwState = FW_STATE_DISABLE;
      }
      else  if (ev == FW_EV_VALID_COMBINATION)
      {
        OTA_Invalidate();
        myOta.fw_crcrunning = 0x1D0F;
        myOta.firmwareUpdateReportNumberPrevious = 0;
        //myOta.fw_numOfRetries = 0;
        //myOta.timerFwUpdateFrameGetHandle = 0;
        TimerCancelFwUpdateFrameGet();
        /*Stop timer*/
      }
      else if(ev == FW_EV_MD_REQUEST_REPORT_SUCCESS)
      {
        myOta.fwState = FW_STATE_ACTIVE;
        SetEvState(FW_EV_GET_NEXT_FW_FRAME);
      }
      else if((ev == FW_EV_MD_REQUEST_REPORT_FAILED)||
              (ev == FW_EV_GET_NEXT_FW_FRAME)||
              (ev == FW_EV_RETRY_NEXT_FW_FRAME)||
              (ev == FW_EV_UPDATE_STATUS_SUCCESS)||
              (ev == FW_EV_UPDATE_STATUS_SUCCESS_USER_REBOOT)||
              (ev == FW_EV_UPDATE_STATUS_UNABLE_TO_RECEIVE))
      {
        myOta.fwState = FW_STATE_DISABLE;
        if( NON_NULL( myOta.pOtaFinish ) )
        {
          myOta.pOtaFinish(OTA_STATUS_ABORT);
        }
        else
        {
          Reboot();
        }

        /*Stop timer*/
      }
      break;


    case FW_STATE_ACTIVE:
      switch(ev)
      {
        case FW_EV_FORCE_DISABLE:
        case FW_EV_VALID_COMBINATION:
          TimerCancelFwUpdateFrameGet();
          /*Tell application it is aborted*/
          if( NON_NULL( myOta.pOtaFinish ) )
          {
            myOta.pOtaFinish(OTA_STATUS_ABORT);
          }
          else
          {
            Reboot();
          }

          myOta.fwState = FW_STATE_DISABLE;
          break;

        case FW_EV_MD_REQUEST_REPORT_SUCCESS:
        case FW_EV_MD_REQUEST_REPORT_FAILED:
          /* Ignore - this happens when someone sends us Request Gets
           * while we are busy updating */
          break;

        case FW_EV_GET_NEXT_FW_FRAME:
          TimerStartFwUpdateFrameGet();
          CmdClassFirmwareUpdateMdGet( &myOta.rxOpt, myOta.firmwareUpdateReportNumberPrevious + 1);
          /*Start/restart timer*/
          break;
        case FW_EV_RETRY_NEXT_FW_FRAME:
          ZW_DEBUG_CMD_OTA_SEND_STR(" FWUpdateCount ");
          ZW_DEBUG_CMD_OTA_SEND_NUM(myOta.timerFWUpdateCount);
          ZW_DEBUG_CMD_OTA_SEND_STR(" retry nbr ");
          ZW_DEBUG_CMD_OTA_SEND_NUM(myOta.fw_numOfRetries);
          if( FIRMWARE_UPDATE_REQUEST_TIMEOUTS < ++(myOta.timerFWUpdateCount))
          {
            myOta.timerFWUpdateCount = 0;
            if (FIRMWARE_UPDATE_MAX_RETRY > ++(myOta.fw_numOfRetries))
            {
              CmdClassFirmwareUpdateMdGet( &myOta.rxOpt, myOta.firmwareUpdateReportNumberPrevious + 1);
              /*Start/restart timer*/
            }
            else
            {
              ZW_DEBUG_CMD_OTA_SEND_BYTE('+');
              SetEvState(FW_EV_UPDATE_STATUS_UNABLE_TO_RECEIVE);
            }
          }
          break;
        case FW_EV_UPDATE_STATUS_SUCCESS:
          SendFirmwareUpdateStatusReport(FIRMWARE_UPDATE_MD_STATUS_REPORT_SUCCESSFULLY_V4);
          myOta.finishStatus = OTA_STATUS_DONE;
          myOta.fwState = FW_STATE_DISABLE;
          break;

        case FW_EV_UPDATE_STATUS_SUCCESS_USER_REBOOT:
          /*Success but user need to reboot device*/
          SendFirmwareUpdateStatusReport(FIRMWARE_UPDATE_MD_STATUS_REPORT_SUCCESSFULLY_STORED_V4);
          myOta.finishStatus = OTA_STATUS_DONE;
          myOta.fwState = FW_STATE_DISABLE;
          break;

        case FW_EV_UPDATE_STATUS_UNABLE_TO_RECEIVE:
          SendFirmwareUpdateStatusReport(FIRMWARE_UPDATE_MD_STATUS_REPORT_UNABLE_TO_RECEIVE_V4);
          ZW_DEBUG_CMD_OTA_SEND_STR("FIRMWARE_UPDATE_MD_STATUS_REPORT_UNABLE_TO_RECEIVE_V4");
          myOta.finishStatus = OTA_STATUS_ABORT;
          myOta.fwState = FW_STATE_DISABLE;
          break;

        case FW_EV_UPDATE_STATUS_CRC_ERROR:
          SendFirmwareUpdateStatusReport(FIRMWARE_UPDATE_MD_STATUS_REPORT_UNABLE_TO_RECEIVE_WITHOUT_CHECKSUM_ERROR_V4);
          ZW_DEBUG_CMD_OTA_SEND_STR("FIRMWARE_UPDATE_MD_STATUS_REPORT_UNABLE_TO_RECEIVE_WITHOUT_CHECKSUM_ERROR_V4");
          myOta.finishStatus = OTA_STATUS_ABORT;
          myOta.fwState = FW_STATE_DISABLE;
          break;

      }
      break;
  }
}

/*============================ SendFirmwareUpdateStatus ==========================
**-------------------------------------------------------------------------*/
void
SendFirmwareUpdateStatusReport(BYTE status)
{
  BYTE waitTime = WAITTIME_FWU_FAIL;
  TimerCancelFwUpdateFrameGet();

  if(FALSE == myOta.fwExtern)
  {
    BOOL retVal = FALSE;
    if((FIRMWARE_UPDATE_MD_STATUS_REPORT_SUCCESSFULLY_STORED_V4 == status) ||
        (FIRMWARE_UPDATE_MD_STATUS_REPORT_SUCCESSFULLY_V4 == status) ||
        (FIRMWARE_UPDATE_MD_STATUS_REPORT_SUCCESSFULLY_WAITING_FOR_ACTIVATION_V4 == status))
    {
      retVal = ZW_FirmwareUpdate_NVM_isValidCRC16();
      if(FALSE == retVal)
      {
        status = FIRMWARE_UPDATE_MD_STATUS_REPORT_INSUFFICIENT_MEMORY_V4;
      }
    }
    ZW_FirmwareUpdate_NVM_Set_NEWIMAGE(retVal);
  }

  if((FIRMWARE_UPDATE_MD_STATUS_REPORT_SUCCESSFULLY_STORED_V4 == status) ||
      (FIRMWARE_UPDATE_MD_STATUS_REPORT_SUCCESSFULLY_V4 == status) ||
      (FIRMWARE_UPDATE_MD_STATUS_REPORT_SUCCESSFULLY_WAITING_FOR_ACTIVATION_V4 == status))
  {
    waitTime = WAITTIME_FWU_SUCCESS;
  }

  if(JOB_STATUS_SUCCESS != CmdClassFirmwareUpdateMdStatusReport( &myOta.rxOpt,
                                                              status,
                                                              waitTime,
                                                              ZCB_FinishFwUpdate))
  {
    /*Failed to send frame and we do not get a CB. Inform app we are finish*/
    ZCB_FinishFwUpdate(NULL);
  }
}


/*============================ ZCB_FinishFwUpdate ==========================
** Function description
** Callback Finish Fw update status to application.
** a new Get frame.
**
** Side effects:
**
**-------------------------------------------------------------------------*/
PCB(ZCB_FinishFwUpdate)(TRANSMISSION_RESULT * pTransmissionResult)
{
  UNUSED(pTransmissionResult);
  if( NON_NULL( myOta.pOtaFinish ) )
  {
    myOta.pOtaFinish(myOta.finishStatus);
  }
  else
  {
    /*Reoot device only if ota has succeeded*/
    if (OTA_STATUS_DONE == myOta.finishStatus)
      Reboot();
  }
}


/*============================ TimerCancelFwUpdateFrameGet ==================
** Function description
** Cancel timer for retries on Get next firmware update frame.
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
TimerCancelFwUpdateFrameGet(void)
{
  if (myOta.timerFwUpdateFrameGetHandle)
  {
    ZW_TIMER_CANCEL(myOta.timerFwUpdateFrameGetHandle);
    myOta.timerFwUpdateFrameGetHandle = 0;
  }
  myOta.fw_numOfRetries = 0;
}


/*============================ ZCB_TimerOutFwUpdateFrameGet =================
** Function description
** Callback on timeout on Get next firmware update frame. It retry to Send
** a new Get frame.
**
** Side effects:
**
**-------------------------------------------------------------------------*/
PCB(ZCB_TimerOutFwUpdateFrameGet)(void)
{
  SetEvState(FW_EV_RETRY_NEXT_FW_FRAME);
}


/*============================ TimerStartFwUpdateFrameGet ==================
** Function description
** Start or restart timer for retries on Get next firmware update frame.
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
TimerStartFwUpdateFrameGet(void)
{
  myOta.fw_numOfRetries = 0;
  myOta.timerFWUpdateCount = 0;
  if (0 == myOta.timerFwUpdateFrameGetHandle)
  {
    /* Allocate timer for calling ZCB_TimerOutFwUpdateFrameGet every DIM_TIMER_FREQ, forever */
    myOta.timerFwUpdateFrameGetHandle = ZW_TIMER_START(ZCB_TimerOutFwUpdateFrameGet, FIRMWARE_UPDATE_REQEUST_TIMEOUT_UNIT, TIMER_FOREVER);
    if (0xFF == myOta.timerFwUpdateFrameGetHandle)
    {
      /* No timer! we update without a timer for retries */
      myOta.timerFwUpdateFrameGetHandle = 0;
    }
  }
  else
  {
    ZW_TIMER_RESTART(myOta.timerFwUpdateFrameGetHandle);
  }
}

/*============================ TimerCancelHostResponse ===============================
** Function description
** Cancel timeout timer for host to response on OtaHostFWU_WriteFinish()
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
TimerCancelHostResponse(void)
{
  ZW_TIMER_CANCEL(myOta.handleHostResponse);
}

/*============================ TimerStartHostResponse ===============================
** Function description
** Start timeout timer for host to response on OtaHostFWU_WriteFinish()
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
TimerStartHostResponse(void)
{
  if( 0 == myOta.handleHostResponse )
  {
    myOta.handleHostResponse = ZW_TIMER_START(ZCB_TimerTimeoutHostResponse, HOST_REPONSE_TIMEOUT, TIMER_ONE_TIME);
  }
  else
  {
    ZW_TIMER_RESTART(myOta.handleHostResponse);
  }
}

/*============================ TimerTimeoutHostResponse ====================
** Function description
** Host did not response finish reading last frame. Just fail the process!
**
** Side effects:
**
**-------------------------------------------------------------------------*/
PCB(ZCB_TimerTimeoutHostResponse)(void)
{
  SetEvState(FW_EV_UPDATE_STATUS_UNABLE_TO_RECEIVE);
}


/*============================ OtaWriteFinish ===============================
** Function description
** Host call function when finish reading incoming frame. Ota start to get
** next frame.
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
OtaHostFWU_WriteFinish(void)
{
//  ZW_DEBUG_SEND_STR("OtaHostFWU_WriteFinish");
//  ZW_DEBUG_SEND_NL();
  /* cancel timer*/
  TimerCancelHostResponse();
  if( ST_DATA_WRITE_PROGRESS == st_data_write_to_host)
  {
    //ZW_DEBUG_SEND_STR("next F");
    /* get next frame*/
    SetEvState(FW_EV_GET_NEXT_FW_FRAME);
  }
  else if(ST_DATA_WRITE_LAST == st_data_write_to_host)
  {
    ZW_DEBUG_SEND_STR("last F");
    /*Wait on host status.*/
    st_data_write_to_host = ST_DATA_WRITE_WAIT_HOST_STATUS;
    /* Tell host it was last frame*/
    if (NON_NULL( myOta.pOtaExtWrite ))
      myOta.pOtaExtWrite( NULL, 0);
  }
}


/*============================ OtaHostFWU_Status ===============================
** Function description
** This function...
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
OtaHostFWU_Status( BOOL userReboot, BOOL status )
{
  ZW_DEBUG_SEND_STR("OtaHostFWU_Status B:");
  ZW_DEBUG_SEND_NUM(userReboot);
  ZW_DEBUG_SEND_BYTE('_');
  ZW_DEBUG_SEND_NUM(status);
  ZW_DEBUG_SEND_NL();
  TimerCancelHostResponse();

  if( TRUE == status )
  {
    /*Check state machine is finish geting data. We both states: ST_DATA_WRITE_LAST
      and ST_DATA_WRITE_WAIT_HOST_STATUS*/
    if((ST_DATA_WRITE_WAIT_HOST_STATUS == st_data_write_to_host) ||
       (ST_DATA_WRITE_LAST == st_data_write_to_host))
    {
      myOta.userReboot = userReboot;
      /* Delay starting the CRC calculation so we can transmit
       * the ack or routed ack first */
      if(0xFF == ZW_TIMER_START(ZCB_UpdateStatusSuccess, 10, 1))
      {
        ZW_DEBUG_SEND_STR("OTA_SUCCESS_NOTIMER");
        ZCB_UpdateStatusSuccess();
      }
    }
    else{
      /* We are not finish getting data!!!*/
      SetEvState(FW_EV_UPDATE_STATUS_UNABLE_TO_RECEIVE);
    }
  }
  else
  {
    /*host failed.. something is wrong.. stop process*/
    SetEvState(FW_EV_UPDATE_STATUS_UNABLE_TO_RECEIVE);
  }
}

uint16_t handleCommandClassFirmwareUpdateMaxFragmentSize(void)
{
  register BYTE bMaxPayload;

  /*
   * Since this function does not include security nor command classes in it's return value, we
   * have to calculate it.
   */
  bMaxPayload = ZW_GetMaxPayloadSize();

  if (SECURITY_KEY_S2_MASK & ZW_GetSecurityKeys())
  {
    return (bMaxPayload - S2_ENCAPSULATION_LENGTH - FIRMWARE_UPDATE_MD_REPORT_ENCAPSULATION_LENGTH);
  }
  else if (SECURITY_KEY_S0_BIT & ZW_GetSecurityKeys())
  {
    return ((bMaxPayload - S0_ENCAPSULATION_LENGTH) * S0_SEQUENCING_FACTOR - FIRMWARE_UPDATE_MD_REPORT_ENCAPSULATION_LENGTH);
  }
  else if (SECURITY_KEY_NONE_MASK == ZW_GetSecurityKeys())
  {
    return (bMaxPayload - FIRMWARE_UPDATE_MD_REPORT_ENCAPSULATION_LENGTH);
  }
  return 0;
}

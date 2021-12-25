/***************************************************************************
 *
 * Copyright (c) 2001-2011
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 *---------------------------------------------------------------------------
 *
 * Description: Power Level Command Class source file
 *
 * Author:
 *
 * Last Changed By:  $Author:  $
 * Revision:         $Revision:  $
 * Last Changed:     $Date:  $
 *
 ****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_basis_api.h>
#include <ZW_tx_mutex.h>
#include <ZW_TransportLayer.h>
#include "eeprom.h"

#include "config_app.h"
#include <CommandClassPowerLevel.h>
#include <ZW_basis_api.h>
#include <ZW_timer_api.h>
#include <misc.h>
#include <ZW_uart_api.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

#ifdef ZW_DEBUG_POW
#define ZW_DEBUG_POW_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_POW_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_POW_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_POW_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_POW_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_POW_SEND_BYTE(data)
#define ZW_DEBUG_POW_SEND_STR(STR)
#define ZW_DEBUG_POW_SEND_NUM(data)
#define ZW_DEBUG_POW_SEND_WORD_NUM(data)
#define ZW_DEBUG_POW_SEND_NL()
#endif

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

static uint8_t testNodeID = ZW_TEST_NOT_A_NODEID;
static uint8_t testPowerLevel;
static uint8_t testSourceNodeID;
static uint16_t testFrameSuccessCount;
static uint8_t testState = POWERLEVEL_TEST_NODE_REPORT_ZW_TEST_FAILED;

static uint8_t timerPowerLevelHandle = 0;
static uint8_t DelayTestFrameHandle = 0;
static uint16_t testFrameCount;
static uint8_t currentPower = normalPower;

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

void ZCB_DelayTestFrame(void);
static void SendTestReport(void);
void ZCB_PowerLevelTimeout(void);
void ZCB_SendTestDone(uint8_t bStatus, TX_STATUS_TYPE *txStatusReport);

/*===========================   SendTestReport   ============================
 **    Send current Powerlevel test results
 **
 **    This is an application function example
 **
 **--------------------------------------------------------------------------*/
static void
SendTestReport(void)
{
  ZW_APPLICATION_TX_BUFFER *pTxBuf = GetRequestBuffer(NULL);
  /*Check pTxBuf is free*/
  if (NON_NULL(pTxBuf))
  {
    MULTICHAN_NODE_ID masterNode;
    TRANSMIT_OPTIONS_TYPE_SINGLE_EX txOptionsEx;

    masterNode.node.nodeId = testSourceNodeID;
    masterNode.node.endpoint = 0;
    masterNode.node.BitAddress = 0;
    masterNode.nodeInfo.BitMultiChannelEncap = 0;
    masterNode.nodeInfo.security = GetHighestSecureLevel(ZW_GetSecurityKeys());
    txOptionsEx.txOptions = ZWAVE_PLUS_TX_OPTIONS;
    txOptionsEx.sourceEndpoint = 0;
    txOptionsEx.pDestNode = &masterNode;

    pTxBuf->ZW_PowerlevelTestNodeReportFrame.cmdClass = COMMAND_CLASS_POWERLEVEL;
    pTxBuf->ZW_PowerlevelTestNodeReportFrame.cmd = POWERLEVEL_TEST_NODE_REPORT;
    pTxBuf->ZW_PowerlevelTestNodeReportFrame.testNodeid = testNodeID;
    pTxBuf->ZW_PowerlevelTestNodeReportFrame.statusOfOperation = testState;
    pTxBuf->ZW_PowerlevelTestNodeReportFrame.testFrameCount1 =
        (uint8_t)(testFrameSuccessCount >> 8);
    pTxBuf->ZW_PowerlevelTestNodeReportFrame.testFrameCount2 = (uint8_t)testFrameSuccessCount;

    if (ZW_TX_IN_PROGRESS != Transport_SendRequestEP(
        (uint8_t*)pTxBuf,
        sizeof(pTxBuf->ZW_PowerlevelTestNodeReportFrame),
        &txOptionsEx,
        ZCB_RequestJobStatus))
    {
      /*Free transmit-buffer mutex*/
      FreeRequestBuffer();
    }
  }
  return;
}

/**
 * @brief Test frame has been transmitted to DUT and the result is noted for later reporting. If
 * not finished then another Test frame is transmitted. If all test frames has been transmitted
 * then the test is stopped and the final result is reported to the PowerlevelTest initiator.
 * @param bStatus Status of transmission.
 * @param txStatusReport Status report.
 */
PCB(ZCB_SendTestDone) (uint8_t bStatus, TX_STATUS_TYPE *txStatusReport)
{
  UNUSED(txStatusReport);

  if (bStatus == TRANSMIT_COMPLETE_OK)
  {
    testFrameSuccessCount++;
  }

  if (0 != DelayTestFrameHandle)
  {
    TimerCancel(DelayTestFrameHandle);
  }

  if (testFrameCount && (--testFrameCount))
  {
    DelayTestFrameHandle = TimerStart(ZCB_DelayTestFrame, 4, TIMER_ONE_TIME);
    if ((uint8_t)-1 == DelayTestFrameHandle)
    {
    }
  }
  else
  {
    if (testFrameSuccessCount)
    {
      testState = POWERLEVEL_TEST_NODE_REPORT_ZW_TEST_SUCCES;
    }
    else
    {
      testState = POWERLEVEL_TEST_NODE_REPORT_ZW_TEST_FAILED;
    }
    ZW_RFPowerLevelSet(currentPower); /* Back to previous setting */
    SendTestReport();
  }
}

/**
 * Stops the power level timer.
 */
void PowerLevelTimerCancel(void)
{
  ZW_TimerLongCancel(timerPowerLevelHandle);
  timerPowerLevelHandle = 0;
}

/**
 * @brief Timer callback which makes sure that the RF transmit powerlevel is set back to
 * normalPower after the designated time period.
 */
PCB(ZCB_PowerLevelTimeout) (void)
{
  ZW_RFPowerLevelSet(normalPower); /* Reset powerlevel to normalPower */
  PowerLevelTimerCancel();
}

/**
 * Starts the power level test.
 *
 * Before starting the test, this function will backup the current power level so that it can be
 * restored after the test has ended.
 */
static void StartTest(void)
{
  testState = POWERLEVEL_TEST_NODE_REPORT_ZW_TEST_INPROGRESS;
  currentPower = ZW_RFPowerLevelGet();
  ZW_RFPowerLevelSet(testPowerLevel);
  DelayTestFrameHandle = TimerStart(ZCB_DelayTestFrame, 4, TIMER_ONE_TIME);
}

static BOOL IsPowerLevelTimerActive(void)
{
  if ((0 != timerPowerLevelHandle) && (0xFF != timerPowerLevelHandle))
  {
    return TRUE;
  }
  return FALSE;
}

/**
 * This function is called when the delay timer triggers.
 */
PCB(ZCB_DelayTestFrame) (void)
{
  DelayTestFrameHandle = 0;
  if (TRUE != ZW_SendTestFrame(testNodeID, testPowerLevel, ZCB_SendTestDone))
  {
    ZCB_SendTestDone(TRANSMIT_COMPLETE_FAIL, NULL);
  }
}

void loadStatusPowerLevel(void)
{

}

void loadInitStatusPowerLevel(void)
{

  testNodeID = 0;
  testPowerLevel = 0;
  testFrameCount = 0;
  testSourceNodeID = 0;
  testState = POWERLEVEL_TEST_NODE_REPORT_ZW_TEST_FAILED;
}

received_frame_status_t
CC_Powerlevel_handler(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  uint8_t cmdLength)
{
  ZW_APPLICATION_TX_BUFFER * pTxBuf = NULL;
  TRANSMIT_OPTIONS_TYPE_SINGLE_EX * pTxOptionsEx;

  UNUSED(cmdLength);

  switch (pCmd->ZW_Common.cmd)
  {
    case POWERLEVEL_SET:
      if (pCmd->ZW_PowerlevelSetFrame.powerLevel > miniumPower)
      {
        return RECEIVED_FRAME_STATUS_FAIL;
      }

      /*Allways cancel timer if receiving POWERLEVEL_SET*/
      if (timerPowerLevelHandle)
      {
        PowerLevelTimerCancel();
      }

      if (normalPower == pCmd->ZW_PowerlevelSetFrame.powerLevel)
      {
        // If the power level is set to normal, we ignore the timeout.
        ZW_RFPowerLevelSet(normalPower);
      }
      else
      {
        if (0 == pCmd->ZW_PowerlevelSetFrame.timeout)
        {
          // A timeout value of zero is invalid.
          return RECEIVED_FRAME_STATUS_FAIL;
        }

        /*Start or Restart test*/
        timerPowerLevelHandle = ZW_TimerLongStart(
            ZCB_PowerLevelTimeout,
            (DWORD)pCmd->ZW_PowerlevelSetFrame.timeout * 1000,
            TIMER_ONE_TIME);
        if (0xFF > timerPowerLevelHandle)
        {
          ZW_RFPowerLevelSet(pCmd->ZW_PowerlevelSetFrame.powerLevel);
        }
      }
      return RECEIVED_FRAME_STATUS_SUCCESS;
    break;

    case POWERLEVEL_GET:
      if (TRUE == Check_not_legal_response_job(rxOpt))
      {
        return RECEIVED_FRAME_STATUS_FAIL;
      }

      pTxBuf = GetResponseBuffer();
      /*Check pTxBuf is free*/
      if (IS_NULL(pTxBuf))
      {
        return RECEIVED_FRAME_STATUS_FAIL;
      }

      RxToTxOptions(rxOpt, &pTxOptionsEx);
      pTxBuf->ZW_PowerlevelReportFrame.cmdClass = COMMAND_CLASS_POWERLEVEL;
      pTxBuf->ZW_PowerlevelReportFrame.cmd = POWERLEVEL_REPORT;
      pTxBuf->ZW_PowerlevelReportFrame.powerLevel = ZW_RFPowerLevelGet();
      pTxBuf->ZW_PowerlevelReportFrame.timeout = 0;

      if (IsPowerLevelTimerActive())
      {
        pTxBuf->ZW_PowerlevelReportFrame.timeout = (uint8_t)(ZW_TimerLongGetTimeLeft(timerPowerLevelHandle) / 1000);
        if (0 == pTxBuf->ZW_PowerlevelReportFrame.timeout)
        {
          /*we always report 1 sec left if we have less than 1 sec before timeout*/
          pTxBuf->ZW_PowerlevelReportFrame.timeout = 1;
        }
      }

      if (ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
                                                        (uint8_t *)pTxBuf,
                                                        sizeof(pTxBuf->ZW_PowerlevelReportFrame),
                                                        pTxOptionsEx,
                                                        ZCB_ResponseJobStatus))
      {
        /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
        FreeResponseBuffer();
        return RECEIVED_FRAME_STATUS_FAIL;
      }
      return RECEIVED_FRAME_STATUS_SUCCESS;
    break;

    case POWERLEVEL_TEST_NODE_SET:
      if (POWERLEVEL_TEST_NODE_REPORT_ZW_TEST_INPROGRESS == testState) // 0x02
      {
        return RECEIVED_FRAME_STATUS_FAIL;
      }

      testFrameCount = (((uint16_t)pCmd->ZW_PowerlevelTestNodeSetFrame.testFrameCount1) << 8);
      testFrameCount |= (uint16_t)pCmd->ZW_PowerlevelTestNodeSetFrame.testFrameCount2;

      if (0 == testFrameCount)
      {
        return RECEIVED_FRAME_STATUS_FAIL;
      }

      testSourceNodeID = rxOpt->sourceNode.nodeId;
      testNodeID = pCmd->ZW_PowerlevelTestNodeSetFrame.testNodeid;
      testPowerLevel = pCmd->ZW_PowerlevelTestNodeSetFrame.powerLevel;
      testFrameSuccessCount = 0;

      StartTest();
      return RECEIVED_FRAME_STATUS_SUCCESS;
    break;

    case POWERLEVEL_TEST_NODE_GET:
      if (TRUE == Check_not_legal_response_job(rxOpt))
      {
        return RECEIVED_FRAME_STATUS_FAIL;
      }

      pTxBuf = GetResponseBuffer();
      /*Check pTxBuf is free*/
      if (IS_NULL(pTxBuf))
      {
        return RECEIVED_FRAME_STATUS_FAIL;
      }

      RxToTxOptions(rxOpt, &pTxOptionsEx);
      pTxBuf->ZW_PowerlevelTestNodeReportFrame.cmdClass = COMMAND_CLASS_POWERLEVEL;
      pTxBuf->ZW_PowerlevelTestNodeReportFrame.cmd = POWERLEVEL_TEST_NODE_REPORT;
      pTxBuf->ZW_PowerlevelTestNodeReportFrame.testNodeid = testNodeID;
      pTxBuf->ZW_PowerlevelTestNodeReportFrame.statusOfOperation = testState;
      pTxBuf->ZW_PowerlevelTestNodeReportFrame.testFrameCount1 = (uint8_t)(testFrameSuccessCount
                                                                           >> 8);
      pTxBuf->ZW_PowerlevelTestNodeReportFrame.testFrameCount2 = (uint8_t)testFrameSuccessCount;

      if (ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
          (uint8_t *)pTxBuf,
          sizeof(pTxBuf->ZW_PowerlevelTestNodeReportFrame),
          pTxOptionsEx,
          ZCB_ResponseJobStatus))
      {
        /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
        FreeResponseBuffer();
      }
      return RECEIVED_FRAME_STATUS_SUCCESS;
    break;

    default:
      break;
  }
  return RECEIVED_FRAME_STATUS_NO_SUPPORT;
}

BOOL
CC_Powerlevel_isInProgress(void)
{
  return (((POWERLEVEL_TEST_NODE_REPORT_ZW_TEST_INPROGRESS == testState) ||
      ((0xFF > timerPowerLevelHandle) && (0 < timerPowerLevelHandle))) ? TRUE : FALSE);
}

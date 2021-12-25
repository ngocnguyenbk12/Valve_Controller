/**
 * @file
 * Handler for Command Class Version.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_basis_api.h>
#include <ZW_TransportLayer.h>
#include <ZW_tx_mutex.h>
#include <ZW_nvr_app_api.h>
#include "config_app.h"
#include <CommandClassVersion.h>
#include <misc.h>
#include <sdk_version.h>
#include <zaf_version.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

static version_info_t hostInterfaceVersionInfo = {0, 0, 0, 0};
static version_info_t applicationVersionInfo = {0, 0, 0, 0};

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

received_frame_status_t
CC_Version_handler(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  uint8_t cmdLength)
{
  PROTOCOL_VERSION protocolVersion;
  ZW_APPLICATION_TX_BUFFER * pTxBuf;
  TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx;
  UNUSED(cmdLength);

  if(TRUE == Check_not_legal_response_job(rxOpt))
  {
    /*Do not support endpoint bit-addressing */
    return RECEIVED_FRAME_STATUS_FAIL;
  }

  switch (pCmd->ZW_VersionGetFrame.cmd)
  {
    case VERSION_GET_V2:
      pTxBuf = GetResponseBuffer();
      /*Check pTxBuf is free*/
      if( NON_NULL( pTxBuf ) )
      {
        uint8_t firmwareTargetIndex;/*firmware target number 1..N */
        uint8_t numberOfFirmwareTargets;
        RxToTxOptions(rxOpt, &pTxOptionsEx);
        pTxBuf->ZW_VersionReport1byteV2Frame.cmdClass = COMMAND_CLASS_VERSION_V2;
        pTxBuf->ZW_VersionReport1byteV2Frame.cmd = VERSION_REPORT_V2;
        pTxBuf->ZW_VersionReport1byteV2Frame.zWaveLibraryType = ZW_TYPE_LIBRARY();
        ZW_GetProtocolVersion(&protocolVersion);
        pTxBuf->ZW_VersionReport1byteV2Frame.zWaveProtocolVersion = protocolVersion.protocolVersionMajor;
        pTxBuf->ZW_VersionReport1byteV2Frame.zWaveProtocolSubVersion = protocolVersion.protocolVersionMinor;
        CC_Version_GetFirmwareVersion_handler( 0,
          (VG_VERSION_REPORT_V2_VG*)&(pTxBuf->ZW_VersionReport1byteV2Frame.firmware0Version));
        pTxBuf->ZW_VersionReport1byteV2Frame.hardwareVersion = CC_Version_GetHardwareVersion();
        numberOfFirmwareTargets = CC_Version_getNumberOfFirmwareTargets_handler();
        pTxBuf->ZW_VersionReport1byteV2Frame.numberOfFirmwareTargets = numberOfFirmwareTargets - 1;/*-1 : Firmware version 0*/

        for (firmwareTargetIndex = 1; firmwareTargetIndex < numberOfFirmwareTargets; firmwareTargetIndex++)
        {
          uint8_t * pFrame = (uint8_t *)&(pTxBuf->ZW_VersionReport1byteV2Frame.variantgroup1);
          CC_Version_GetFirmwareVersion_handler(firmwareTargetIndex, (VG_VERSION_REPORT_V2_VG *)(pFrame + 2 * (firmwareTargetIndex - 1)));
        }

        if(ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
            (uint8_t *)pTxBuf,
            /* comment to len calc: frame size           + size of number of firmwareversions                                 -   variantgroup1 (calc in size of number of firmwareversions)*/
            sizeof(pTxBuf->ZW_VersionReport1byteV2Frame) + (numberOfFirmwareTargets - 1)* sizeof(VG_VERSION_REPORT_V2_VG) - sizeof(VG_VERSION_REPORT_V2_VG) , /*-1 is Firmware version 0*/
            pTxOptionsEx,
            ZCB_ResponseJobStatus))
        {
          /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
          FreeResponseBuffer();
        }
        return RECEIVED_FRAME_STATUS_SUCCESS;
      }
      return RECEIVED_FRAME_STATUS_FAIL;
      break;

    case VERSION_COMMAND_CLASS_GET_V2:
      pTxBuf = GetResponseBuffer();
      /*Check pTxBuf is free*/
      if( NON_NULL( pTxBuf ) )
      {
        RxToTxOptions(rxOpt, &pTxOptionsEx);
        pTxBuf->ZW_VersionCommandClassReportFrame.cmdClass = COMMAND_CLASS_VERSION_V2;
        pTxBuf->ZW_VersionCommandClassReportFrame.cmd = VERSION_COMMAND_CLASS_REPORT_V2;
        pTxBuf->ZW_VersionCommandClassReportFrame.requestedCommandClass = pCmd->ZW_VersionCommandClassGetFrame.requestedCommandClass;
        pTxBuf->ZW_VersionCommandClassReportFrame.commandClassVersion = CC_Version_getCommandClassVersion_handler(pCmd->ZW_VersionCommandClassGetFrame.requestedCommandClass);

        if(ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
            (uint8_t *)pTxBuf,
            sizeof(pTxBuf->ZW_VersionCommandClassReportFrame),
            pTxOptionsEx,
            ZCB_ResponseJobStatus))
        {
          /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
          FreeResponseBuffer();
        }
        return RECEIVED_FRAME_STATUS_SUCCESS;
      }
      return RECEIVED_FRAME_STATUS_FAIL;
      break;

    case VERSION_CAPABILITIES_GET_V3:
      pTxBuf = GetResponseBuffer();

      if (IS_NULL(pTxBuf))
      {
        return RECEIVED_FRAME_STATUS_FAIL;
      }

      RxToTxOptions(rxOpt, &pTxOptionsEx);

      pTxBuf->ZW_VersionCapabilitiesReportV3Frame.cmdClass    = COMMAND_CLASS_VERSION_V3;
      pTxBuf->ZW_VersionCapabilitiesReportV3Frame.cmd         = VERSION_CAPABILITIES_REPORT_V3;

      /*
       * The following pointer constellation might seem unnecessary, but it's done to make the
       * code more readable and to enable/disable each flags easier.
       *
       * Sigma Designs' Z-Wave applications support all commands, but the support can be toggled
       * corresponding to the Version Command Class specification
       */
      {
        uint8_t * pProperties = &(pTxBuf->ZW_VersionCapabilitiesReportV3Frame.properties1);
        *pProperties = 0x00;

        *pProperties |= VERSION_CAPABILITIES_REPORT_PROPERTIES1_VERSION_BIT_MASK_V3;
        *pProperties |= VERSION_CAPABILITIES_REPORT_PROPERTIES1_COMMAND_CLASS_BIT_MASK_V3;
        *pProperties |= VERSION_CAPABILITIES_REPORT_PROPERTIES1_Z_WAVE_SOFTWARE_BIT_MASK_V3;
      }

      if(ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
          (uint8_t *)pTxBuf,
          sizeof(pTxBuf->ZW_VersionCapabilitiesReportV3Frame),
          pTxOptionsEx,
          ZCB_ResponseJobStatus))
      {
        /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
        FreeResponseBuffer();
      }
      return RECEIVED_FRAME_STATUS_SUCCESS;
      break;

    case VERSION_ZWAVE_SOFTWARE_GET_V3:
      pTxBuf = GetResponseBuffer();

      if (IS_NULL(pTxBuf))
      {
        return RECEIVED_FRAME_STATUS_FAIL;
      }

      RxToTxOptions(rxOpt, &pTxOptionsEx);

      pTxBuf->ZW_VersionZwaveSoftwareReportV3Frame.cmdClass    = COMMAND_CLASS_VERSION_V3;
      pTxBuf->ZW_VersionZwaveSoftwareReportV3Frame.cmd         = VERSION_ZWAVE_SOFTWARE_REPORT_V3;

      pTxBuf->ZW_VersionZwaveSoftwareReportV3Frame.sdkVersion1 = SDK_VERSION_MAJOR;
      pTxBuf->ZW_VersionZwaveSoftwareReportV3Frame.sdkVersion2 = SDK_VERSION_MINOR;
      pTxBuf->ZW_VersionZwaveSoftwareReportV3Frame.sdkVersion3 = SDK_VERSION_PATCH;

      pTxBuf->ZW_VersionZwaveSoftwareReportV3Frame.applicationFrameworkApiVersion1 = ZAF_VERSION_MAJOR;
      pTxBuf->ZW_VersionZwaveSoftwareReportV3Frame.applicationFrameworkApiVersion2 = ZAF_VERSION_MINOR;
      pTxBuf->ZW_VersionZwaveSoftwareReportV3Frame.applicationFrameworkApiVersion3 = ZAF_VERSION_PATCH;

      pTxBuf->ZW_VersionZwaveSoftwareReportV3Frame.applicationFrameworkBuildNumber1 = (uint8_t)(ZAF_BUILD_NO >> 8);
      pTxBuf->ZW_VersionZwaveSoftwareReportV3Frame.applicationFrameworkBuildNumber2 = (uint8_t)ZAF_BUILD_NO;

      pTxBuf->ZW_VersionZwaveSoftwareReportV3Frame.hostInterfaceVersion1 = hostInterfaceVersionInfo.major;
      pTxBuf->ZW_VersionZwaveSoftwareReportV3Frame.hostInterfaceVersion2 = hostInterfaceVersionInfo.minor;
      pTxBuf->ZW_VersionZwaveSoftwareReportV3Frame.hostInterfaceVersion3 = hostInterfaceVersionInfo.patch;
      pTxBuf->ZW_VersionZwaveSoftwareReportV3Frame.hostInterfaceBuildNumber1 = (uint8_t)(hostInterfaceVersionInfo.build_number >> 8);
      pTxBuf->ZW_VersionZwaveSoftwareReportV3Frame.hostInterfaceBuildNumber2 = (uint8_t)hostInterfaceVersionInfo.build_number;

      {
        PROTOCOL_VERSION protocolVersion;
        ZW_GetProtocolVersion(&protocolVersion);
        pTxBuf->ZW_VersionZwaveSoftwareReportV3Frame.zWaveProtocolVersion1 = protocolVersion.protocolVersionMajor;
        pTxBuf->ZW_VersionZwaveSoftwareReportV3Frame.zWaveProtocolVersion2 = protocolVersion.protocolVersionMinor;
        pTxBuf->ZW_VersionZwaveSoftwareReportV3Frame.zWaveProtocolVersion3 = 0;
        pTxBuf->ZW_VersionZwaveSoftwareReportV3Frame.zWaveProtocolBuildNumber1 = (uint8_t)(protocolVersion.build_number >> 8);
        pTxBuf->ZW_VersionZwaveSoftwareReportV3Frame.zWaveProtocolBuildNumber2 = (uint8_t)protocolVersion.build_number;
      }

      pTxBuf->ZW_VersionZwaveSoftwareReportV3Frame.applicationVersion1 = applicationVersionInfo.major;
      pTxBuf->ZW_VersionZwaveSoftwareReportV3Frame.applicationVersion2 = applicationVersionInfo.minor;
      pTxBuf->ZW_VersionZwaveSoftwareReportV3Frame.applicationVersion3 = applicationVersionInfo.patch;
      pTxBuf->ZW_VersionZwaveSoftwareReportV3Frame.applicationBuildNumber1 = (uint8_t)(applicationVersionInfo.build_number >> 8);
      pTxBuf->ZW_VersionZwaveSoftwareReportV3Frame.applicationBuildNumber2 = (uint8_t)applicationVersionInfo.build_number;

      if(ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
          (uint8_t *)pTxBuf,
          sizeof(pTxBuf->ZW_VersionZwaveSoftwareReportV3Frame),
          pTxOptionsEx,
          ZCB_ResponseJobStatus))
      {
        /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
        FreeResponseBuffer();
      }
      return RECEIVED_FRAME_STATUS_SUCCESS;
      break;

    default:
      // Do nothing.
      break;
  }
  return RECEIVED_FRAME_STATUS_NO_SUPPORT;
}

uint8_t CC_Version_GetHardwareVersion(void)
{
  uint8_t hwVersion;
  /* Read hwVersion from NVR.*/
  ZW_NVRGetAppValue(offsetof(NVR_APP_FLASH_STRUCT, hwVersion), 1, &hwVersion );
  return hwVersion; /*HW version*/
}

void CC_Version_SetHostInterfaceVersionInfo(
    uint8_t major,
    uint8_t minor,
    uint8_t patch,
    uint16_t build_number)
{
  hostInterfaceVersionInfo.major = major;
  hostInterfaceVersionInfo.minor = minor;
  hostInterfaceVersionInfo.patch = patch;
  hostInterfaceVersionInfo.build_number = build_number;
}

void CC_Version_SetApplicationVersionInfo(
    uint8_t major,
    uint8_t minor,
    uint8_t patch,
    uint16_t build_number)
{
  applicationVersionInfo.major = major;
  applicationVersionInfo.minor = minor;
  applicationVersionInfo.patch = patch;
  applicationVersionInfo.build_number = build_number;
}

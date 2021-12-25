/**
 * @file
 * Handler for Command Class Central Scene.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

#ifndef PRODUCTPLUS_APPLICATIONCOMMANDHANDLERS_COMMANDCLASSCENTRALSCENE_H_
#define PRODUCTPLUS_APPLICATIONCOMMANDHANDLERS_COMMANDCLASSCENTRALSCENE_H_


#include <CommandClass.h>
#include <agi.h>

/**
 * Contains all configuration parameters.
 */
typedef struct
{
  uint8_t slowRefresh;
}
central_scene_configuration_t;

/**
 * Returns the version of this CC.
 */
#define CommandClassCentralSceneVersionGet() CENTRAL_SCENE_VERSION_V3

/**
 * Function implemented by the application which defines what Central Scene capabilities the
 * application supports.
 * @param pData Pointer to the variable storing data as defined in the
 * specification of the Central Scene command class.
 * @return Number of bytes with key attributes.
 */
extern uint8_t getAppCentralSceneReportData(ZW_CENTRAL_SCENE_SUPPORTED_REPORT_1BYTE_V3_FRAME * pData);

/**
 * Function implemented by the application which defines what Central Scene Configuration Get Command
 * is used to read the use of optional node capabilities for scene notifications.
 * @param pConfiguration Pointer to struct containing Central Scene configuration parameters.
 */
extern void getAppCentralSceneConfiguration(central_scene_configuration_t * pConfiguration);

/**
 * Function implemented by the application which defines what Central Scene Configuration Set Command
 * is used to configure the use of optional node capabilities for scene notifications.
 * @param pConfiguration Pointer to struct containing Central Scene configuration parameters.
 */
extern void setAppCentralSceneConfiguration(central_scene_configuration_t * pConfiguration);

/**
 * Handles incoming commands in the Central Scene command class.
 * @param[in] rxOpt Receive options.
 * @param[in] pCmd Incoming payload.
 * @param cmdLength Length of the payload.
 * @return receive frame status.
 */
received_frame_status_t
handleCommandClassCentralScene(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  BYTE cmdLength);

/**
 * Initiates the transmission of a "Central Scene Notification" command.
 * @param[in] pProfile pointer to AGI profile
 * @param[in] sourceEndpoint source endpoint
 * @param[in] keyAttribute The key event.
 * @param[in] sceneNumber The scene number.
 * @param[in] pCbFunc Callback function to be called when transmission is done/failed.
 * @return Status of the job.
 */
JOB_STATUS
CommandClassCentralSceneNotificationTransmit(
  AGI_PROFILE* pProfile,
  BYTE sourceEndpoint,
  BYTE keyAttribute,
  BYTE sceneNumber,
  VOID_CALLBACKFUNC(pCbFunc)(TRANSMISSION_RESULT * pTransmissionResult));

#endif /* PRODUCTPLUS_APPLICATIONCOMMANDHANDLERS_COMMANDCLASSCENTRALSCENE_H_ */

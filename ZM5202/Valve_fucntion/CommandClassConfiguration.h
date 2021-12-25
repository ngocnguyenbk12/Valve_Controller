/**
 * @file
 * Handler for Command Class Configuration 
 */




#include <CommandClass.h>
#include <agi.h>

#define WATER_MODE 	0x01
#define METER_VALUE 0x02
#define METER_MODE  0x03

#define WATER_MODE_0 0x01
#define WATER_MODE_1 0x02
/**
 * Contains all configuration parameters.
 */

typedef struct
{
	uint8_t value;
	uint8_t Endpoint_node;
}
configuration_t;



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
extern uint8_t getConfiguration(uint8_t parameterNumber);

/**
 * Function implemented by the application which defines what Central Scene Configuration Get Command
 * is used to read the use of optional node capabilities for scene notifications.
 * @param pConfiguration Pointer to struct containing Central Scene configuration parameters.
 */
//extern void getConfiguration2(central_scene_configuration_t * pConfiguration);

/**
 * Function implemented by the application which defines what Central Scene Configuration Set Command
 * is used to configure the use of optional node capabilities for scene notifications.
 * @param pConfiguration Pointer to struct containing Central Scene configuration parameters.
 */
extern void setConfiguration(uint8_t parameterNumber,	uint8_t value );

/**
 * Handles incoming commands in the Central Scene command class.
 * @param[in] rxOpt Receive options.
 * @param[in] pCmd Incoming payload.
 * @param cmdLength Length of the payload.
 * @return receive frame status.
 */
received_frame_status_t
handleCommandClassConfiguration(
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
CommandClassConfigurationReport(
	AGI_PROFILE* pProfile,
  BYTE sourceEndpoint,
  BYTE bvalue,
  VOID_CALLBACKFUNC(pCallback)(TRANSMISSION_RESULT * pTransmissionResult));


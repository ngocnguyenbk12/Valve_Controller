/**
 * @file
 * Handler for Command Class Device Reset Locally.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include <ZW_typedefs.h>
#include <ZW_classcmd.h>
#include <ZW_TransportEndpoint.h>
#include <misc.h>
#include <CommandClassDeviceResetLocally.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

// Nothing here.

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

// Nothing here.

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

// Nothing here.

/****************************************************************************/
/*                               FUNCTIONS                                  */
/****************************************************************************/

void CC_DeviceResetLocally_notification_tx(
  agi_profile_t * pProfile,
  VOID_CALLBACKFUNC(pCallback)(transmission_result_t * pTransmissionResult))
{
  transmission_result_t transmissionResult;
  transmissionResult.nodeId = 0;
  transmissionResult.status = ZW_TX_FAILED;
  transmissionResult.isFinished = TRANSMISSION_RESULT_FINISHED;

  if(!GetMyNodeID())
  {
    /*
     * The node is not included, and therefore it makes no sense to send
     * anything. Finish by calling the callback function.
     */

    if (NON_NULL( pCallback ))
    {
      pCallback(&transmissionResult);
    }
  }
  else
  {
    cc_group_t cmdGrp = {COMMAND_CLASS_DEVICE_RESET_LOCALLY, DEVICE_RESET_LOCALLY_NOTIFICATION};

    if(JOB_STATUS_SUCCESS != cc_engine_multicast_request(
        pProfile,
        ENDPOINT_ROOT,
        &cmdGrp,
        NULL,
        0,
        FALSE,
        pCallback))
    {
      if (NON_NULL(pCallback))
      {
        pCallback(&transmissionResult);
      }
    }
  }
}

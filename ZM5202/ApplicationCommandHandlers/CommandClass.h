/**
 * @file
 * Common types and definitions for all command classes.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

#ifndef PRODUCTPLUS_APPLICATIONCOMMANDHANDLERS_COMMANDCLASS_H_
#define PRODUCTPLUS_APPLICATIONCOMMANDHANDLERS_COMMANDCLASS_H_

#include <ZW_typedefs.h>
#include <ZW_stdint.h>

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

/**
 * Can be used for pairing a command class with a specific command in the command class.
 */
typedef struct
{
  uint8_t cmdClass; /**< Command class*/
  uint8_t cmd; /**< Command*/
}
cc_group_t;

/**
 * For backwards compatibility.
 */
typedef cc_group_t CMD_CLASS_GRP;

/**
 * Can be used for pairing AGI profile identifiers listed in \cite SDS12657.
 */
typedef struct
{
  uint8_t profile_MS; /**< AGI profile of type: ASSOCIATION_GROUP_INFO_REPORT_PROFILE_...*/
  uint8_t profile_LS; /**< AGI profile of type: ASSOCIATION_GROUP_INFO_REPORT_PROFILE_...*/
}
agi_profile_t;

/**
 * For backwards compatibility.
 */
typedef agi_profile_t AGI_PROFILE;

/**
 * Callback status used on framework API for request/response-job
 */
typedef enum
{
  JOB_STATUS_SUCCESS = 0, /**< Job has been started. */
  JOB_STATUS_BUSY, /**< Job couldn't start.  */
  JOB_STATUS_NO_DESTINATIONS /**< Job couldn't start because there is no destinations. */
}
job_status_t;

/**
 * For backwards compatibility.
 */
typedef job_status_t JOB_STATUS;

/**
 * Indicates whether all transmissions are done. Used by \ref TRANSMISSION_RESULT.
 */
typedef enum
{
  TRANSMISSION_RESULT_NOT_FINISHED, /**< Still transmitting. */
  TRANSMISSION_RESULT_FINISHED /**< Done transmitting to all nodes. */
} TRANSMISSION_RESULT_FINISH_STATUS;


/**
 * Status on incoming frame. Use same values as cc_supervision_status_t
 */
typedef enum
{
  RECEIVED_FRAME_STATUS_NO_SUPPORT = 0x00, /**< Frame not supported*/
  RECEIVED_FRAME_STATUS_FAIL = 0x02,       /**< Could not handle incoming frame*/
  RECEIVED_FRAME_STATUS_SUCCESS = 0xFF     /**< Frame received successfully*/
} received_frame_status_t;


/**
 * This struct defines the values which can be parsed to a callback function
 * upon an ended transmission regardless of the result.
 */
typedef struct
{
  uint8_t nodeId; /**< The ID of the node to which the transmission has been done. */
  uint8_t status; /**< Status of the transmission. See ZW_transport_api.h. */
  /**
   * If transmission to several nodes, this flag is set if transmission for the last node has ended.
   */
  TRANSMISSION_RESULT_FINISH_STATUS isFinished;
}
transmission_result_t;

/**
 * For backwards compatibility.
 */
typedef transmission_result_t TRANSMISSION_RESULT;

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

JOB_STATUS cc_engine_multicast_request(AGI_PROFILE* pProfile,
                                       uint8_t endpoint,
                                       CMD_CLASS_GRP *pcmdGrp,
                                       uint8_t* pPayload,
                                       uint8_t size,
                                       uint8_t fSupervisionEnable,
                                       VOID_CALLBACKFUNC(pCbFunc) (TRANSMISSION_RESULT * pTransmissionResult));

#endif /* PRODUCTPLUS_APPLICATIONCOMMANDHANDLERS_COMMANDCLASS_H_ */

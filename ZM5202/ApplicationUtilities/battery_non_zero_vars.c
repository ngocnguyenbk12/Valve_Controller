/**
 * @file
 * Defines variables whose values must be kept during powerdown.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

#ifdef __C51__
#pragma userclass (xdata = NON_ZERO_VARS_APP)
#endif
/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include <ZW_typedefs.h>
#include <ZW_TransportEndpoint.h>

/**
 * This variable contains the Supervision session ID which must me kept during powerdown.
 */
XBYTE m_sessionId;
XBYTE previously_receive_session_id;
XBYTE previously_rxStatus;
MULTICHAN_DEST_NODE_ID xdata previously_received_destination;

/**
 * This variable is used by the Battery Monitor.
 */
XBYTE lowBattReportAcked;

/**
 * This variable is used by the Battery Monitor.
 */
XBYTE st_battery;


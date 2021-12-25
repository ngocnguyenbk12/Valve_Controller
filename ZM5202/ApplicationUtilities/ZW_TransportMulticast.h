/**
 * @file
 * Handles multicast frames in the Z-Wave Framework.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

#ifndef PRODUCTPLUS_APPLICATIONUTILITIES_ZW_TRANSPORTMULTICAST_H_
#define PRODUCTPLUS_APPLICATIONUTILITIES_ZW_TRANSPORTMULTICAST_H_

//#ifdef ZW_SECURITY_PROTOCOL

#include <ZW_stdint.h>
#include <ZW_typedefs.h>
#include <CommandClassAssociation.h>

/**
 * @brief Initiates transmission of a request using multicast if possible. If
 * not it falls back to singlecast.
 * @param[in] p_data Pointer to data.
 * @param[in] data_length Length of data in bytes.
 * @param[in] fSupervisionEnable Enable Supervision encapsulation if set to TRUE.
 * @param[in] p_nodelist Pointer to a list of nodes.
 * @param[in] p_callback Pointer to a callback function which is called for each transmission to a node.
 * @return Returns status of transmission.
 */
enum ZW_SENDDATA_EX_RETURN_CODES
ZW_TransportMulticast_SendRequest(const uint8_t * const p_data,
                                  uint8_t data_length,
                                  uint8_t fSupervisionEnable,
                                  TRANSMIT_OPTIONS_TYPE_EX * p_nodelist,
                                  VOID_CALLBACKFUNC(p_callback)(TRANSMISSION_RESULT * pTransmissionResult));

/**
 * @brief
 */
void ZW_TransportMulticast_clearTimeout(void);

//#endif /* ZW_SECURITY_PROTOCOL */

#endif /* PRODUCTPLUS_APPLICATIONUTILITIES_ZW_TRANSPORTMULTICAST_H_ */

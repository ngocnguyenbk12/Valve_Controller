/**
 *
 * Copyright (c) 2001-2014
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 * @file config_app.h
 *
 * @brief This header file contains defines for application version
 *  in a generalized way.
 *
 *  Don't change the name of the file, and son't change the names of
 *  APP_VERSION and APP_REVISION, as they are handled automatically by
 *  the release procedure. The version information will be set automatically
 *  by the "make_release.bat"-script.
 *
 * Author: Erik Friis Harck
 *
 * Last Changed By: $Author: tro $
 * Revision: $Revision: 0.00 $
 * Last Changed: $Date: 2014/12/09 14:28:21 $
 *
 */
#ifndef _CONFIG_APP_H_
#define _CONFIG_APP_H_

#ifdef __C51__
#include <ZW_product_id_enum.h>
#include <ZW_security_api.h>
#endif

#define APP_MANUFACTURER_ID     MFG_ID_SIGMA_DESIGNS
#define APP_PRODUCT_TYPE_ID     PRODUCT_TYPE_ID_ZWAVE_PLUS
#define APP_PRODUCT_ID          PRODUCT_ID_SerialAPIPlus
#define APP_FIRMWARE_ID         APP_PRODUCT_ID | (APP_PRODUCT_TYPE_ID << 8)


#ifdef ZW_SECURITY_PROTOCOL

#define REQUESTED_SECURITY_AUTHENTICATION SECURITY_AUTHENTICATION_SSA

#ifdef ZW_SECURITY_PROTOCOL_SINGLE_NETWORK_KEY

/* Only ONE network key allowed */
#define REQUESTED_SECURITY_KEYS ( SECURITY_KEY_S2_UNAUTHENTICATED_BIT )

#else

#define REQUESTED_SECURITY_KEYS ( SECURITY_KEY_S0_BIT | SECURITY_KEY_S2_UNAUTHENTICATED_BIT | SECURITY_KEY_S2_AUTHENTICATED_BIT | SECURITY_KEY_S2_ACCESS_BIT)

#endif  /* ZW_SECURITY_SINGLE_NETWORK_KEY */

#endif  /* ZW_SECURITY_PROTOCOL */

/* Accept all incoming command classes, regardless of NIF contents. */
#define ACCEPT_ALL_CMD_CLASSES

#endif /* _CONFIG_APP_H_ */


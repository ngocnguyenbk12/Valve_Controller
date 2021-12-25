/****************************************************************************
 *
 * Copyright (c) 2001-2013
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 *---------------------------------------------------------------------------
 *
 * Description: Definition and initialization of firmware size field to put
 *              ahead of the firmware.
 *
 * Author:   Erik Friis Harck
 *
 * Last Changed By:  $Author: iza $
 * Revision:         $Revision: 22797 $
 * Last Changed:     $Date: 2012-05-10 15:55:06 +0200 (to, 10 maj 2012) $
 *
 ****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include "config_app.h"
#include "ZW_basis_api.h"
#include "ZW_firmware_descriptor.h"

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/* Size field of firmware descriptor for OTA firmware update.                 */
/* This size field must be, and will be, located just after the reset vector. */
#ifdef __C51__
code WORD firmwareSize = (WORD)&firmwareDescriptor + sizeof(t_firmwareDescriptor);
#else
code WORD firmwareSize = 0;
#endif

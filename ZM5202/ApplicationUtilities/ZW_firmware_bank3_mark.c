/****************************************************************************
 *
 * Copyright (c) 2001-2013
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 *---------------------------------------------------------------------------
 *
 * Description: Definition and initialization of BANK3 end marker field
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
#include "ZW_typedefs.h"
#include "ZW_firmware_descriptor.h"

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/* BANK3 End marker for OTA firmware update.               */
/* This firmware BANK3 end marker must be, and will be, located at     */
/* the end of BANK3 */
code BYTE bBank3EndMarker = 3;

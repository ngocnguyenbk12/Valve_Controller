/****************************************************************************
 *
 * Copyright (c) 2001-2013
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 *---------------------------------------------------------------------------
 *
 * Description: Definition and initialization of Z-Wave bootloader descriptor.
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

#include "ZW_bootloader_descriptor.h"

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/* Bootloader descriptor for OTA firmware update */
code t_bootloaderDescriptor bootloaderDescriptor =
{
  /* TODO: Fill in your unique and assigned manufacturer ID here                */
  APP_MANUFACTURER_ID,                  /* WORD manufacturerID; */
  /* TODO: Fill in your own unique firmware ID here                             */
  APP_FIRMWARE_ID,                      /* WORD firmwareID; */
  /* A CRC-CCITT must be, and will be, filled in here by a software build tool (fixbootcrc.exe) */
  0                                     /* WORD checksum; */
};


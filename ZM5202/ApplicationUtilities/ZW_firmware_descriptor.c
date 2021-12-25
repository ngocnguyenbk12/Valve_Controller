/****************************************************************************
 *
 * Copyright (c) 2001-2013
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 *---------------------------------------------------------------------------
 *
 * Description: Definition and initialization of Z-Wave firmware descriptor.
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
#include <ZW_typedefs.h>
#include "config_app.h"
#include <ZW_basis_api.h>
#include <ZW_firmware_bootloader_defs.h>
#include <ZW_firmware_descriptor.h>


/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
#define BANK_OFFSET   MCU_BANK_SIZE


/****************************************************************************/
/*                              EXTERNALS                                   */
/****************************************************************************/

extern code BYTE bBank1EndMarker;
extern code BYTE bBank2EndMarker;
extern code BYTE bBank3EndMarker;


/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/* Firmware descriptor for OTA firmware update */
code t_firmwareDescriptor firmwareDescriptor =
{
 /* Total amount of code used in COMMON bank */
#ifdef __C51__
  (WORD)&firmwareDescriptor + sizeof(firmwareDescriptor),
#else
  0, // Value for unit tests.
#endif
#ifdef BOOTLOADER_ENABLED
  /* Above Size is inclusive the Bootloader code - substract FIRMWARE_BOOTLOADER_SIZE for real Firmware COMMON BANK usage */
  /* Total amount of code saved in NVM for the BANK1 bank */
  (WORD)&bBank1EndMarker + sizeof(bBank1EndMarker) - BANK_OFFSET,
  /* Total amount of code saved in NVM for the BANK2 bank */
  (WORD)&bBank2EndMarker + sizeof(bBank2EndMarker) - BANK_OFFSET,
  /* Total amount of code saved in NVM for the BANK3 bank */
  (WORD)&bBank3EndMarker + sizeof(bBank3EndMarker) - BANK_OFFSET,
#else
  0,
  0,
  0,
#endif
  /* TODO: Fill in your unique and assigned manufacturer ID here                */
  APP_MANUFACTURER_ID,                  /* WORD manufacturerID; */
  /* TODO: Fill in your own unique firmware ID here                             */
  APP_FIRMWARE_ID,                      /* WORD firmwareID; */
  /* A CRC-CCITT must be, and will be, filled in here by a software build tool (fixbootcrc.exe) */
  0                                     /* WORD checksum; */
};

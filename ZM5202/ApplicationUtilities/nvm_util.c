/***************************************************************************
*
* Copyright (c) 2001-2017
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: This module implements functions needed by the application to
*              handling of the nvm.
*
* Author: Torsten Rasmussen
*
* Last Changed By: $Author: trasmussen $
* Revision: $Revision: 0.00 $
* Last Changed: $Date: 2017/01/28 11:40:30 $
*
****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <eeprom.h>
#include <ZW_security_api.h>
#include <ZW_TransportSecProtocol.h>
#include <association_plus.h>
#include "nvm_util.h"
/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
#define MAGIC_VALUE                 0x42

#define BIT_SDK_660_UPDATE          0x02 // Bit used to state whether update is from 6.60.
#define BIT_SDK_67X_UPDATE          0x08 // Bit used to state whether update is from 6.70.
#define BIT_SECURITY0_ENABLED       0x04 // Bit used to state whether Security 0 is enabled.
#define BIT_SECURITY0_SUPPORTED     0x10 // Bit used to state whether Security 0 is supported.

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

static void
NvmApplicationDataMigrate ( void )
{
  BYTE buffer[NETWORK_KEY_LENGTH];
  BYTE version = 0x01; // By coming here, we know the data meets 6.51 req.
  WORD i;
  // Current knowledge:
  // At this point it is already detected that valid application data is present.
  // Thus it must be detected which layout is in use: 6.51, 6.6x, 6.7x w/ or wo/ security.
  // The bit mask will be created depending on the magic values to identify the firmware:
  // the following bits will denote version.
  // 000s 0nba
  // 00a = 6.51, 0ba=6.6x.
  // n = node info security.
  // s = with security.

  version |= MemoryGetByte((WORD)&EEOFFS_SECURITY_RESERVED.EEOFFS_MAGIC_BYTE_field) == EEPROM_MAGIC_BYTE_VALUE ? BIT_SECURITY0_SUPPORTED : 0; // Security magic byte. If present, then security was enabled.
  if (version & BIT_SECURITY0_SUPPORTED) // Security field was present, hence offset of 6.6x magic can be directly checked.
  {
    version |= MemoryGetByte((WORD)&EEOFFS_SECURITY_RESERVED.EEOFFS_NETWORK_SECURITY_field) == TRUE ? BIT_SECURITY0_ENABLED : 0; // 0x04 = S0 Secure, 0x00 = non-secure.
    version |= MemoryGetByte((WORD)&EEOFFSET_ASSOCIATION_ENDPOINT_MAGIC_far) == MAGIC_VALUE ? BIT_SDK_660_UPDATE : 0;
    version |= MemoryGetByte((WORD)&EEOFFSET_MAGIC_SDK_6_70_ASSOCIATION_SECURE_far) == MAGIC_VALUE ? BIT_SDK_67X_UPDATE : 0;
  }
  else
  {
    WORD EEOFFSET_ASSOCIATION_ENDPOINT_MAGIC_far_addr = (WORD)(&EEOFFSET_ASSOCIATION_ENDPOINT_MAGIC_far);
    WORD EEOFFSET_MAGIC_SDK_6_70_ASSOCIATION_SECURE_far_addr = (WORD)(&EEOFFSET_MAGIC_SDK_6_70_ASSOCIATION_SECURE_far) -sizeof(EEOFFS_NETWORK_SECURITY_STRUCT);

    // Some sample apps have the EEOFFSET_ASSOCIATION_ENDPOINT_MAGIC_far located after the
    // EEOFFS_NETWORK_SECURITY_STRUCT, hence to find the value before update, the size of
    // EEOFFS_NETWORK_SECURITY_STRUCT must be subtracted.
    if (EEOFFSET_ASSOCIATION_ENDPOINT_MAGIC_far_addr > ((WORD)&EEOFFS_SECURITY_RESERVED))
    {
      EEOFFSET_ASSOCIATION_ENDPOINT_MAGIC_far_addr -= sizeof(EEOFFS_NETWORK_SECURITY_STRUCT);
    }

    version |= MemoryGetByte(EEOFFSET_ASSOCIATION_ENDPOINT_MAGIC_far_addr) == MAGIC_VALUE ? BIT_SDK_660_UPDATE : 0;
    version |= MemoryGetByte(EEOFFSET_MAGIC_SDK_6_70_ASSOCIATION_SECURE_far_addr) == MAGIC_VALUE ? BIT_SDK_67X_UPDATE : 0;
  }

  switch (version)
  {
    case 0x03:  // We are coming from 6.6x without security.
                // Need to relocate data, and set default NodeInfo secure to 0 (non-secure).
      for (i = (((WORD)&nvmApplicationDescriptor - ((WORD)&EEOFFS_SECURITY_RESERVED + sizeof(EEOFFS_NETWORK_SECURITY_STRUCT))) - sizeof(buffer)); !(i & 0x8000); i -= sizeof(buffer))
      {
        MemoryGetBuffer(((WORD)&EEOFFS_SECURITY_RESERVED + i), buffer, sizeof(buffer));
        MemoryPutBuffer(((WORD)&EEOFFS_SECURITY_RESERVED + sizeof(EEOFFS_NETWORK_SECURITY_STRUCT) + i), buffer, sizeof(buffer), NULL);
      }

      if (i != 0)
      {
        i += sizeof(buffer);
        MemoryGetBuffer(((WORD)&EEOFFS_SECURITY_RESERVED), buffer, i);
        MemoryPutBuffer(((WORD)&EEOFFS_SECURITY_RESERVED + sizeof(EEOFFS_NETWORK_SECURITY_STRUCT)), buffer, i, NULL);
      }
      MemoryPutByte((WORD)&EEOFFSET_ASSOCIATION_ENDPOINT_MAGIC_far, MAGIC_VALUE);
      MemoryPutByte((WORD)&EEOFFSET_MAGIC_SDK_6_70_ASSOCIATION_SECURE_far, MAGIC_VALUE);
      break;

    case 0x17:  // We are coming from 6.6x with security and S0 on end-points.
                // No need to relocate data, only fetch S0 key and push to protocol and update NodeInfo 0x04 (S0 secure).
      for (i = 0; i < ((NUMBER_OF_ENDPOINTS + 1) * ASSOCIATION_SIZE); ++i)
      {
        BYTE transportCap = MemoryGetByte( (WORD)&EEOFFSET_TRANSPORT_CAPABILITIES_START_far[i]);

        (((EEOFFSET_TRANSPORT_CAPABILITIES_STRUCT*)&transportCap))->security = SECURITY_KEY_S0;

        MemoryPutByte((WORD)&EEOFFSET_TRANSPORT_CAPABILITIES_START_far[i], transportCap);
      }
      /** Fall-through */
#ifdef slave_routing
#else
    case 0x15:  // We are coming from 6.51 with security set and S0 key.
                // No need to relocate data, only fetch S0 key and push to protocol.
      MemoryGetBuffer((WORD)&EEOFFS_SECURITY_RESERVED.EEOFFS_NETWORK_KEY_START_field, buffer, sizeof(buffer));
      ZW_SetSecurityS0NetworkKey(buffer);
      /*Clear application NMV key!*/
      memset(buffer, 0,  sizeof(buffer));
      MemoryPutBuffer((WORD)&EEOFFS_SECURITY_RESERVED.EEOFFS_NETWORK_KEY_START_field, buffer,  sizeof(buffer), NULL);
      /** Fall-through */
#endif
    case 0x13:  // We are coming from 6.6x with security, but no S0 key.
    case 0x11:  // We are coming from 6.51 with security, but no S0 key.
    case 0x01:
      MemoryPutByte((WORD)&EEOFFSET_ASSOCIATION_ENDPOINT_MAGIC_far, MAGIC_VALUE); /* Now ASSOCIATION EP should be OK */
      MemoryPutByte((WORD)&EEOFFSET_MAGIC_SDK_6_70_ASSOCIATION_SECURE_far, MAGIC_VALUE);
      break;

    case 0x0B:
    case 0x1B:
      /*FWU from SDK 6.7x. Nothing to migrate*/
      break;
    default:
      break;
  }
}



void
NvmInit(ZW_NVM_STATUS nvmStatus)
{
  if (ZW_NVM_UPGRADED == nvmStatus)
  {
    NvmApplicationDataMigrate();
  }
}


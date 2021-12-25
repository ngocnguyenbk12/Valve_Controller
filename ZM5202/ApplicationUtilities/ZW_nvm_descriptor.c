/****************************************************************************
 *
 * Copyright (c) 2001-2014
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 *---------------------------------------------------------------------------
 *
 * Description: Definition and initialization of Z-Wave NVM descriptor.
 *
 * Author:   Erik Friis Harck
 *
 * Last Changed By:  $Author: iza $
 * Revision:         $Revision: 22797 $
 * Last Changed:     $Date: 2012-05-10 15:55:06 +0200 (to, 10 maj 2012) $
 *
 ****************************************************************************/
#ifdef __C51__
#pragma USERCLASS(CONST=NVM)
#endif
/* Make sure compiler won't shuffle around with these variables,            */
/* as there are external dependencies.                                      */
/* Note from Keil C51 manual:                                               */
/* Variables with memory type, initilalization, and without initilalization */
/* have all different tables. Therefore only variables with the same        */
/* attributes are kept within order.                                        */
/* Note from Keil knowledgebase: http://www.keil.com/support/docs/901.htm   */
/* "The order is not necessarily taken from the variable declarations, but  */
/* the first use of the variables."                                         */
/* Therefore, when using #pragma ORDER to order variables, declare them in  */
/* the order they should be in a collection. And none of them may be        */
/* declared or known in any way from other header files.                    */
#ifdef __C51__
#pragma ORDER
#endif
/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include "config_app.h"
#include "ZW_basis_api.h"
#include "ZW_nvm_descriptor.h"

/****************************************************************************/
/*                              EXTERNALS                                   */
/****************************************************************************/

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/* The below externals are listed here to be able to access                 */
/* nvmModuleSizeEndMarker without disturbing the order of the definitions.  */
extern WORD far nvmDescriptorSize;
extern t_nvmDescriptor far nvmDescriptor;
extern t_nvmModuleDescriptor far nvmDescriptorDescriptor;
extern WORD far nvmModuleSizeEndMarker;

/************************************/
/*      NVM variable definition     */
/************************************/

/*--------------------------------------------------------------------------*/
/* NVM layout ZW_nvm_descriptor                                             */

/* Offset from &nvmModule where nvmModuleDescriptor structure is placed */
#ifdef __C51__
t_NvmModuleSize far nvmDescriptorSize = (t_NvmModuleSize)&_FD_ZW_NVM_DESCRIPTOR_L_ - sizeof(nvmModuleSizeEndMarker);
#else
t_NvmModuleSize far nvmDescriptorSize = 0;
#endif

/*--------------------------------------------------------------------------*/
/* NVM descriptor for firmware                                              */
/* A nice place to define the following IDs and versions is in config_app.h */
t_nvmDescriptor far nvmDescriptor =
{
#ifdef __C51__
  /* TODO: Fill in your unique and assigned manufacturer ID here            */
  APP_MANUFACTURER_ID,                  /* WORD manufacturerID;             */
  /* TODO: Fill in your own unique firmware ID here                         */
  APP_FIRMWARE_ID,                      /* WORD firmwareID;                 */
  /* TODO: Fill in your own unique Product Type ID here                     */
  APP_PRODUCT_TYPE_ID,                  /* WORD productTypeID;              */
  /* TODO: Fill in your own unique Product ID here                          */
  APP_PRODUCT_ID,                       /* WORD productID;                  */
  /* Unique Application Version (from config_app.h)                         */
  (WORD)&_APP_VERSION_,                 /* WORD applicationVersion;         */
  /* Unique Z-Wave protocol Version (from config_lib.h)                     */
  (WORD)&_ZW_VERSION_,                  /* WORD zwaveProtocolVersion;       */
#else
  0,
  0,
  0,
  0,
  0,
  0,
#endif
};

/* NVM module descriptor for module. Located at the end of NVM module.      */
/* During the initialization phase, the NVM still contains the NVM contents */
/* from the old version of the firmware.                                    */
t_nvmModuleDescriptor far nvmDescriptorDescriptor =
{
#ifdef __C51__
  (t_NvmModuleSize)&_FD_ZW_NVM_DESCRIPTOR_L_ - sizeof(nvmModuleSizeEndMarker), /* t_NvmModuleSize wNvmModuleSize */
#else
  0,
#endif
  NVM_MODULE_TYPE_NVM_DESCRIPTOR,       /* eNvmModuleType bNvmModuleType    */
#ifdef __C51__
  (WORD)&_APP_VERSION_                  /* WORD wNvmModuleVersion           */
#else
  0
#endif
};

/* Mark the end of NVM (no more NVM modules)                                */
WORD far nvmModuleSizeEndMarker = 0;

/* Variables put in non volatile memory (end) */

/* NVM module update descriptor for this new version of firmware. Located   */
/* in code space.                                                           */
const t_nvmModuleUpdate code nvmDescriptorUpdate =
{
  (p_nvmModule)&nvmDescriptorSize,      /* nvmModulePtr                     */
  /* nvmDescriptorDescriptor is the first new NVM variable since devkit_6_5x_branch */
#ifdef __C51__
  (t_NvmModuleSize)((WORD)&nvmDescriptorDescriptor),   /* wNvmModuleSizeOld */
#else
  0,
#endif
  {
#ifdef __C51__
    (t_NvmModuleSize)&_FD_ZW_NVM_DESCRIPTOR_L_ - sizeof(nvmModuleSizeEndMarker), /* t_NvmModuleSize wNvmModuleSize */
#else
    0,
#endif
    NVM_MODULE_TYPE_NVM_DESCRIPTOR,     /* eNvmModuleType bNvmModuleType    */
#ifdef __C51__
    (WORD)&_APP_VERSION_                  /* WORD wNvmModuleVersion           */
#else
    0
#endif
  }
};


/****************************************************************************
 *
 * Copyright (c) 2001-2013
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 *---------------------------------------------------------------------------
 *
 * Description: Application ZW0x0x RF setting setup table
 *
 * Author:   Johann Sigfredsson
 *
 * Last Changed By:  $Author: efh $
 * Revision:         $Revision: 9763 $
 * Last Changed:     $Date: 2008-01-10 11:28:42 +0100 (Thu, 10 Jan 2008) $
 *
 ****************************************************************************/
#include "ZW_rf050x.h"

tsFlashApplTable code sFlashApplTable =
{
  0xFF,                   /* BYTE dummy1;                                   */
  0xFF,                   /* BYTE dummy2;                                   */
  /* What should the power level be when in normal power TX mode */
#if defined (ZM5304) && defined (US)
  0x1F, /* BYTE FLASH_APPL_NORM_POWER_OFFS_0;   Channel 0 */
  0x1F, /* BYTE FLASH_APPL_NORM_POWER_OFFS_1;   Channel 1 */
  0x1F, /* BYTE FLASH_APPL_NORM_POWER_OFFS_2;   Channel 2 */

#else
  APP_DEFAULT_NORM_POWER, /* BYTE FLASH_APPL_NORM_POWER_OFFS_0;   Channel 0 */
  APP_DEFAULT_NORM_POWER, /* BYTE FLASH_APPL_NORM_POWER_OFFS_1;   Channel 1 */
  APP_DEFAULT_NORM_POWER, /* BYTE FLASH_APPL_NORM_POWER_OFFS_2;   Channel 2 */
#endif
  /* What should the power level be when in low power TX mode */
  APP_DEFAULT_LOW_POWER,  /* BYTE FLASH_APPL_LOW_POWER_OFFS_0;    Channel 0 */
  APP_DEFAULT_LOW_POWER,  /* BYTE FLASH_APPL_LOW_POWER_OFFS_1;    Channel 1 */
  APP_DEFAULT_LOW_POWER   /* BYTE FLASH_APPL_LOW_POWER_OFFS_2;    Channel 2 */
};

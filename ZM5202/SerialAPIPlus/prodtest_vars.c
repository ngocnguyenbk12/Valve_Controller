/****************************************************************************
 *
 * Copyright (c) 2001-2011
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 *---------------------------------------------------------------------------
 *
 * Description: critical memory vars used for the production test
 *              in ZM5304 modules.
 * Author:  Samer Seoud
 *
 * Last Changed By:  $Author: efh $
 * Revision:         $Revision: 11509 $
 * Last Changed:     $Date: 2012-03-05 10:45:45 +0200 (Wed, 5 Mar 2012) $
 *
 ****************************************************************************/


/* Allocate the variables in the application area of the critical memory   */
#pragma userclass (xdata = NON_ZERO_VARS_APP)

/****************************************************************************/
/*                              INCLUDE FILES                               */
#include <ZW_typedefs.h>

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/
BYTE bProdtestState;


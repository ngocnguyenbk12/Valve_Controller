/**
 * @file
 * A utility to find the endpoint ID / index.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _ENDPOINT_LOOKUP_H_
#define _ENDPOINT_LOOKUP_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include <ZW_sysdefs.h>
#include <ZW_evaldefs.h>
#define ENDPOINT_NOT_VALID   0xFF

/**
 * Structure Endpoint look-up
 */

typedef struct _ENDPOINT_LOOKUP_
{
  BYTE bEndPointsCount;
  BYTE * pEndPointList;
}ENDPOINT_LOOKUP;


/**
 * @brief FindEndPointID
 * Get the supported primary switch type used by the HW.
 * @param pEPLookup pointer to the endpoint lookup data structure
 * @param endpointIndex the index of the endpoint in the endpoints IDs list
 * @return endpoint ID
 */
BYTE FindEndPointID(ENDPOINT_LOOKUP *pEPLookup, BYTE endpointIndex);

/**
 * @brief GetEndPointCount
 * Get the number of endpoints ID in the endpoints IDs list.
 * @param pEPLookup pointer to the endpoint lookup data structure
 * @return endpoints count
 */
BYTE GetEndPointCount(ENDPOINT_LOOKUP *pEPLookup);

/**
 * @brief FindEndPointIndex
 * Find the endpoint index in the endpoints IDs list.
  * @param pEPLookup pointer to the endpoint lookup data structure
 * @param endpoint the endpoint ID
 * @return endpoints count
 */
BYTE FindEndPointIndex(ENDPOINT_LOOKUP *pEPLookup, BYTE endpoint);

#endif /* _ENDPOINT_LOOKUP_H_ */


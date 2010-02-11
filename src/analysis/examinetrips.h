#ifndef __EXAMINETRIPS_H
#define __EXAMINETRIPS_H

#include "sqlite3.h"

// If two trips are further than this far apart [kilometers] at any
//   point, we consider them different
#define OBDTRIP_CUTOFF 0.4

/// Get the distance between these co-ordinates, in km
double haversine_dist(double latA, double lonA, double latB, double lonB);

/// Compare two trips
/** \return 0 for equal, nonzero for not equal
 */
int comparetrips(sqlite3 *db, int tripA, int tripB, int gpspointsA, int gpspointsB);

/// How much petrol do we think was drunk this trip?
double petrolusage(sqlite3 *db, int trip);

/// Total length of this trip
double tripdist(sqlite3 *db, int trip);

/// Weighted median,mean of lat and lon for this trip
int tripmeanmedian(sqlite3 *db, int trip, double *meanlat, double *meanlon,
	double *medianlat, double *medianlon);

#endif // __EXAMINETRIPS_H


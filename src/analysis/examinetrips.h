#ifndef __EXAMINETRIPS_H
#define __EXAMINETRIPS_H

#include "sqlite3.h"

/// Get the distance between these co-ordinates, in km
double haversine_dist(double latA, double lonA, double latB, double lonB);

/// How much petrol do we think was drunk this trip?
double petrolusage(sqlite3 *db, int trip);

/// Total length of this trip
double tripdist(sqlite3 *db, int trip);

/// Weighted median,mean of lat and lon for this trip
int tripmeanmedian(sqlite3 *db, int trip, double *meanlat, double *meanlon,
	double *medianlat, double *medianlon);

#endif // __EXAMINETRIPS_H


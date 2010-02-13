#ifndef __ANALYSISTABLES_H
#define __ANALYSISTABLES_H

#include "sqlite3.h"

/// Create the analysis tables we need in this database
int createAnalysisTables(sqlite3 *db);

/// Purge any analyiss data from the tables we've created
int resetTripAnalysisTables(sqlite3 *db);

/// Insert [or update] these parameters for this trip
int insertTripAnalysis(sqlite3 *db, int trip, double length,
	double meanlat, double meanlon, double medianlat, double medianlon);

/// Get the parameters for the requested trip
int getTripAnalysis(sqlite3 *db, int trip, double *length,
	double *meanlat, double *meanlon, double *medianlat, double *medianlon);

/// Populate the analysis tables
int fillAnalysisTables(sqlite3 *db);

/// Export gps analysis to CSV
int exportGpsCSV(sqlite3 *db, FILE *f);

#endif // __ANALYSISTABLES_H



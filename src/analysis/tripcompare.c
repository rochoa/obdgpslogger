#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "sqlite3.h"

// If two trips are further than this far apart [kilometers] at any
//   point, we consider them different
#define OBDTRIP_CUTOFF 0.2

/// Print help
void printhelp(const char *argv0);

/// Compare two trips
/** \return 0 for equal, nonzero for not equal
 */
int comparetrips(sqlite3 *db, int tripA, int tripB, int gpspointsA, int gpspointsB);

/// How much petrol do we think was drunk this trip?
double thirstyboy(sqlite3 *db, int trip);

/// Total length of this trip
double tripdist(sqlite3 *db, int trip);

int main(int argc, char *argv[]) {
	// Do not attempt to buffer stdout
	setvbuf(stdout, (char *)NULL, _IONBF, 0);

	if(argc < 2 || 0 == strcmp("--help", argv[1]) ||
			0 == strcmp("-h", argv[1])) {
		printhelp(argv[0]);
		exit(0);
	}

	sqlite3 *db;

	int rc;
	rc = sqlite3_open_v2(argv[1], &db, SQLITE_OPEN_READONLY, NULL);
	if(SQLITE_OK != rc) {
		fprintf(stderr, "Can't open database %s: %s\n", argv[1], sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(1);
	}

	// Find out how much petrol each trip burned

	sqlite3_stmt *obdtripstmt;
	const char obdtrip_sql[] = "SELECT DISTINCT trip FROM obd ORDER BY TRIP";

	rc = sqlite3_prepare_v2(db, obdtrip_sql, -1, &obdtripstmt, NULL);
	if(SQLITE_OK != rc) {
		fprintf(stderr, "Cannot prepare select statement gpstrips (%i): %s\n", rc, sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(1);
	}

	while(SQLITE_ROW == sqlite3_step(obdtripstmt)) {
		thirstyboy(db, sqlite3_column_int(obdtripstmt, 0));
	}

	sqlite3_finalize(obdtripstmt);


	// See which trips are actually the same

	// Need a pair of trip statements for iterating
	const char tripselect_sql[] = "SELECT trip,COUNT(lat) FROM gps "
			"WHERE trip>? "
			"GROUP BY trip "
			"ORDER BY trip";

	sqlite3_stmt *tripstmt_A, *tripstmt_B;

	rc = sqlite3_prepare_v2(db, tripselect_sql, -1, &tripstmt_A, NULL);
	if(SQLITE_OK != rc) {
		fprintf(stderr, "Cannot prepare select statement trip A (%i): %s\n", rc, sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(1);
	}

	rc = sqlite3_prepare_v2(db, tripselect_sql, -1, &tripstmt_B, NULL);
	if(SQLITE_OK != rc) {
		fprintf(stderr, "Cannot prepare select statement trip B (%i): %s\n", rc, sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(1);
	}

	// Iterate across pairs of trips
	sqlite3_bind_int(tripstmt_A, 1, -1);
	while(SQLITE_ROW == sqlite3_step(tripstmt_A)) {
		sqlite3_reset(tripstmt_B);
		sqlite3_bind_int(tripstmt_B, 1, sqlite3_column_int(tripstmt_A, 0));
		while(SQLITE_ROW == sqlite3_step(tripstmt_B)) {
				// Currently it tests for A being a subset of B, so do each test twice
			comparetrips(db,
								sqlite3_column_int(tripstmt_A, 0), sqlite3_column_int(tripstmt_B, 0),
								sqlite3_column_int(tripstmt_A, 1), sqlite3_column_int(tripstmt_B, 1)
							);
			comparetrips(db,
								sqlite3_column_int(tripstmt_B, 0), sqlite3_column_int(tripstmt_A, 0),
								sqlite3_column_int(tripstmt_B, 1), sqlite3_column_int(tripstmt_A, 1)
							);
		}
	}

	sqlite3_finalize(tripstmt_A);
	sqlite3_finalize(tripstmt_B);
	sqlite3_close(db);
	return 0;
}

double haversine_dist(double latA, double lonA, double latB, double lonB) {
	// Haversine formula
	// R = earth radius ~= 6,371km
	// delta lat = lat2 − lat1
	// delta lon = lon2 − lon1
	// a = sin2(delta lat/2) + cos(lat1) * cos(lat2) * sin2(delta lon/2)
	// c = 2 * atan2(sqrt(a), sqrt(1−a))
	// d = R * c

	double R = 6371;
	double dlat = latB-latA;
	double dlon = lonB-lonA;
	double sinlat = sin((dlat/2) * (M_PI/180));
	double sinlon = sin((dlon/2) * (M_PI/180));

	double a=sinlat*sinlat + sinlon*sinlon * cos(latA * (M_PI/180)) * cos(latB * (M_PI/180));
	double c = 2 * atan2(sqrt(a), sqrt(1-a));
	double d = R*c;

	return d;
}

int comparetrips(sqlite3 *db, int tripA, int tripB, int gpspointsA, int gpspointsB) {
	int rc;

	int retvalue = 0;

	const char gpsselect_sql[] = "SELECT lat,lon FROM gps "
			"WHERE trip=? "
			"ORDER BY time";

	sqlite3_stmt *gpsstmt_A, *gpsstmt_B;

	rc = sqlite3_prepare_v2(db, gpsselect_sql, -1, &gpsstmt_A, NULL);
	if(SQLITE_OK != rc) {
		fprintf(stderr, "Cannot prepare select statement gps A (%i): %s\n", rc, sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(1);
	}

	rc = sqlite3_prepare_v2(db, gpsselect_sql, -1, &gpsstmt_B, NULL);
	if(SQLITE_OK != rc) {
		fprintf(stderr, "Cannot prepare select statement gps B (%i): %s\n", rc, sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(1);
	}

	sqlite3_bind_int(gpsstmt_A, 1, tripA);

	// Largest distance of the minimum distances
	double maxd = -1;

	// Progress counter
	int progress = 0;
	int progress_mod = gpspointsA/40; // Want a total of about this many dots

	printf("%i,%i:", tripA, tripB);

	while(SQLITE_ROW == (rc = sqlite3_step(gpsstmt_A))) {
		double latA,lonA;

		latA = sqlite3_column_double(gpsstmt_A, 0);
		lonA = sqlite3_column_double(gpsstmt_A, 1);

		sqlite3_reset(gpsstmt_B);
		sqlite3_bind_int(gpsstmt_B, 1, tripB);
		
		// Minimum diameter between this point and a point in the other trace
		double mind = 7000; // Larger than the earth's radius

		while(SQLITE_ROW == (rc = sqlite3_step(gpsstmt_B))) {
			double latB,lonB;

			latB = sqlite3_column_double(gpsstmt_B, 0);
			lonB = sqlite3_column_double(gpsstmt_B, 1);

			double d=haversine_dist(latA, lonA, latB, lonB);
			if(d < mind) mind = d;

			if(OBDTRIP_CUTOFF > mind) {
				// Known-good
				break;
			}
		}
		progress=(progress+1)%progress_mod;
		if(1 == progress) printf(".");

		if(mind > maxd) maxd = mind;

		if(OBDTRIP_CUTOFF < maxd) {
			retvalue = 1;
			// Known-bad
			break;
		}
	}
	if(OBDTRIP_CUTOFF < maxd) {
		retvalue = 1;
	}
	printf("\nFound distance: %f. Trip %i is %sa subset of trip %i\n",
					maxd, tripA,  retvalue?"not ":"", tripB);

	sqlite3_finalize(gpsstmt_A);
	sqlite3_finalize(gpsstmt_B);

	return retvalue;
}

double thirstyboy(sqlite3 *db, int trip) {
	int rc;

	int retvalue = 0;

	const char mafselect_sql[] = "SELECT SUM(a.maf*(a.time-b.time)), count(a.maf), max(a.time) - min(a.time) "
			"FROM obd a LEFT JOIN obd b "
			"ON a.rowid=b.rowid+1 "
			"WHERE a.trip=? AND b.trip=a.trip";

	sqlite3_stmt *mafstmt;

	rc = sqlite3_prepare_v2(db, mafselect_sql, -1, &mafstmt, NULL);
	if(SQLITE_OK != rc) {
		fprintf(stderr, "Cannot prepare select statement maf (%i): %s\n", rc, sqlite3_errmsg(db));
		sqlite3_close(db);
		return -1;
	}

	sqlite3_bind_int(mafstmt, 1, trip);
	printf("Trip %i ", trip);

	double trip_dist = tripdist(db, trip);
	double total_maf;
	double delta_time;
	int maf_count;
	if(SQLITE_ROW == sqlite3_step(mafstmt)) {
		total_maf = sqlite3_column_double(mafstmt,0);
		maf_count = sqlite3_column_int(mafstmt,1);
		delta_time = sqlite3_column_double(mafstmt,2);
	}

	const float ratio = 0.147;
	const float petrol_density = 737.22; //  kg/m^3
	const float cubic_meters_to_gallons = 264.172052;
	const float km_to_miles = 0.621371192;

	double petrol_kilograms = ratio * total_maf / 1000.0;
	double petrol_m3 = petrol_kilograms/petrol_density;
	double petrol_gal = petrol_m3 * cubic_meters_to_gallons;

	double trip_miles = trip_dist * km_to_miles;

	printf("(%.0f sec): %.2fg petrol, %.3fgal, %.2fmiles, %.1fmpg\n",
				delta_time, 1000*petrol_kilograms,
				petrol_gal, trip_miles, trip_miles/petrol_gal);

	sqlite3_finalize(mafstmt);

	return total_maf;
}

double tripdist(sqlite3 *db, int trip) {
	int rc;

	double total_dst = 0;

	const char dstselect_sql[] = "SELECT a.lat,a.lon,b.lat,b.lon "
			"FROM gps a LEFT JOIN gps b "
			"ON b.rowid=a.rowid+1 "
			"WHERE a.trip=? AND b.trip=a.trip";

	sqlite3_stmt *dststmt;

	rc = sqlite3_prepare_v2(db, dstselect_sql, -1, &dststmt, NULL);
	if(SQLITE_OK != rc) {
		fprintf(stderr, "Cannot prepare select statement dst (%i): %s\n", rc, sqlite3_errmsg(db));
		sqlite3_close(db);
		return -1;
	}

	sqlite3_bind_int(dststmt, 1, trip);

	while(SQLITE_ROW == sqlite3_step(dststmt)) {
		total_dst += haversine_dist( sqlite3_column_double(dststmt, 0),
							sqlite3_column_double(dststmt, 1),
							sqlite3_column_double(dststmt, 2),
							sqlite3_column_double(dststmt, 3) );
	}

	sqlite3_finalize(dststmt);

	return total_dst;
}

void printhelp(const char *argv0) {
	printf("Usage: %s <database>\n"
		"Guess which trips are the same for comparing\n", argv0);
}


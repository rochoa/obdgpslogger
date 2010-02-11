#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "sqlite3.h"
#include "examinetrips.h"

/// Print help
void printhelp(const char *argv0);

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

	// Find out weighted mean position of each trip
	sqlite3_stmt *gpstripstmt;
	const char gpstrip_sql[] = "SELECT DISTINCT trip FROM gps ORDER BY TRIP";

	rc = sqlite3_prepare_v2(db, gpstrip_sql, -1, &gpstripstmt, NULL);
	if(SQLITE_OK != rc) {
		fprintf(stderr, "Cannot prepare select statement gpstrips (%i): %s\n", rc, sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(1);
	}

	printf("Trip,Weighted mean lat,Weighted mean lon,Weighted median lat,Weighted median lon\n");
	while(SQLITE_ROW == sqlite3_step(gpstripstmt)) {
		double meanlat = 0;
		double meanlon = 0;
		double medianlat = 0;
		double medianlon = 0;

		int trip = sqlite3_column_int(gpstripstmt, 0);
		if(0 == tripmeanmedian(db, trip, &meanlat, &meanlon, &medianlat, &medianlon)) {
			printf("%i,%f,%f,%f,%f\n", trip, meanlat, meanlon, medianlat, medianlon);
		}
	}


	// Find out how much petrol each trip burned
	sqlite3_stmt *obdtripstmt;
	const char obdtrip_sql[] = "SELECT DISTINCT trip FROM obd ORDER BY TRIP";

	rc = sqlite3_prepare_v2(db, obdtrip_sql, -1, &obdtripstmt, NULL);
	if(SQLITE_OK != rc) {
		fprintf(stderr, "Cannot prepare select statement obdtrips (%i): %s\n", rc, sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(1);
	}

	while(SQLITE_ROW == sqlite3_step(obdtripstmt)) {
		petrolusage(db, sqlite3_column_int(obdtripstmt, 0));
	}


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

	sqlite3_finalize(obdtripstmt);
	sqlite3_finalize(gpstripstmt);

	sqlite3_close(db);
	return 0;
}

void printhelp(const char *argv0) {
	printf("Usage: %s <database>\n"
		"Guess which trips are the same for comparing\n", argv0);
}


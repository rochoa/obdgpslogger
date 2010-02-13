#include <stdio.h>
#include "sqlite3.h"

#include "analysistables.h"
#include "examinetrips.h"

int createAnalysisTables(sqlite3 *db) {
	int rc;
	char *errmsg;


	const char attachsql[] = "ATTACH DATABASE \":memory:\" AS analysis";

	if(SQLITE_OK != (rc = sqlite3_exec(db, attachsql, NULL, NULL, &errmsg))) {
		fprintf(stderr, "sqlite error on statement %s (%i): %s\n", attachsql, rc, errmsg);
		sqlite3_free(errmsg);
		return -1;
	}

	const char gpssql[] = "CREATE TABLE IF NOT EXISTS analysis.gpsanalysis "
				"(trip INTEGER UNIQUE, length REAL, "
				"meanlat REAL, meanlon REAL, "
				"medianlat REAL, medianlon REAL)";

	if(SQLITE_OK != (rc = sqlite3_exec(db, gpssql, NULL, NULL, &errmsg))) {
		fprintf(stderr, "sqlite error on statement %s (%i): %s\n", gpssql, rc, errmsg);
		sqlite3_free(errmsg);
		return -1;
	}

	const char obdsql[] = "CREATE TABLE IF NOT EXISTS analysis.obdanalysis "
				"(trip INTEGER UNIQUE, petrolusage REAL)";

	if(SQLITE_OK != (rc = sqlite3_exec(db, obdsql, NULL, NULL, &errmsg))) {
		fprintf(stderr, "sqlite error on statement %s (%i): %s\n", obdsql, rc, errmsg);
		sqlite3_free(errmsg);
		return -1;
	}

	return 0;
}

int resetTripAnalysisTables(sqlite3 *db) {
	int rc;
	char *errmsg;


	const char gpssql[] = "DELETE FROM analysis.gpsanalysis WHERE 1";

	if(SQLITE_OK != (rc = sqlite3_exec(db, gpssql, NULL, NULL, &errmsg))) {
		fprintf(stderr, "sqlite error on statement %s (%i): %s\n", gpssql, rc, errmsg);
		sqlite3_free(errmsg);
		return -1;
	}

	const char obdsql[] = "DELETE FROM analysis.obdanalysis WHERE 1";

	if(SQLITE_OK != (rc = sqlite3_exec(db, obdsql, NULL, NULL, &errmsg))) {
		fprintf(stderr, "sqlite error on statement %s (%i): %s\n", obdsql, rc, errmsg);
		sqlite3_free(errmsg);
		return -1;
	}

	return 0;
}

int insertTripAnalysis(sqlite3 *db, int trip, double length,
			double meanlat, double meanlon, double medianlat, double medianlon) {
	const char sql[] = "INSERT OR REPLACE INTO analysis.gpsanalysis "
				"(trip, length, meanlat, meanlon, medianlat, medianlon) "
				"VALUES "
				"(?,?,?,?,?,?)";

	int rc;
	sqlite3_stmt *stmt;

	rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

	if(SQLITE_OK != rc) {
		fprintf(stderr, "Couldn't create insert statement \"%s\" (%i): %s\n",
						sql, rc, sqlite3_errmsg(db));
		return -1;
	}

	sqlite3_bind_int(stmt, 1, trip);
	sqlite3_bind_double(stmt, 2, length);
	sqlite3_bind_double(stmt, 3, meanlat);
	sqlite3_bind_double(stmt, 4, meanlon);
	sqlite3_bind_double(stmt, 5, medianlat);
	sqlite3_bind_double(stmt, 6, medianlon);

	rc = sqlite3_step(stmt);

	sqlite3_finalize(stmt);
	return 0;
}

int getTripAnalysis(sqlite3 *db, int trip, double *length,
			double *meanlat, double *meanlon, double *medianlat, double *medianlon) {

	const char sql[] = "SELECT length, meanlat, meanlon, medianlat, medianlon "
				"FROM analysis.gpsanalysis WHERE trip=?";

	int rc;
	sqlite3_stmt *stmt;

	rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

	if(SQLITE_OK != rc) {
		fprintf(stderr, "Couldn't create select statement \"%s\" (%i): %s\n",
						sql, rc, sqlite3_errmsg(db));
		return -1;
	}

	sqlite3_bind_int(stmt, 1, trip);

	int retvalue = -1;

	if(SQLITE_ROW == (rc = sqlite3_step(stmt))) {
		if(NULL != length) {
			*length = sqlite3_column_double(stmt, 0);
		}
		if(NULL != meanlat) {
			*meanlat = sqlite3_column_double(stmt, 1);
		}
		if(NULL != meanlon) {
			*meanlon = sqlite3_column_double(stmt, 2);
		}
		if(NULL != medianlat) {
			*medianlat = sqlite3_column_double(stmt, 3);
		}
		if(NULL != medianlon) {
			*medianlon = sqlite3_column_double(stmt, 4);
		}

		retvalue = 0;
	}

	sqlite3_finalize(stmt);
	return retvalue;
}

int fillAnalysisTables(sqlite3 *db) {
	const char sql[] = "SELECT DISTINCT tripid FROM trip ORDER BY tripid";

	int rc;
	sqlite3_stmt *stmt;

	rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

	if(SQLITE_OK != rc) {
		fprintf(stderr, "Couldn't create select statement \"%s\" (%i): %s\n",
						sql, rc, sqlite3_errmsg(db));
		return -1;
	}

	while(SQLITE_ROW == (rc = sqlite3_step(stmt))) {
		int trip = sqlite3_column_int(stmt, 0);

		double length=tripdist(db, trip);
		double meanlat=0;
		double meanlon=0;
		double medianlat=0;
		double medianlon=0;

		int status = tripmeanmedian(db, trip, &meanlat, &meanlon, &medianlat, &medianlon);
		if(0 == status) {
			insertTripAnalysis(db, trip, length, meanlat, meanlon, medianlat, medianlon);
		}
	}

	sqlite3_finalize(stmt);

	return 0;
}

int exportGpsCSV(sqlite3 *db, FILE *f) {
	const char sql[] = "SELECT * FROM analysis.gpsanalysis ORDER BY trip";

	int rc;
	sqlite3_stmt *stmt;

	rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

	if(SQLITE_OK != rc) {
		fprintf(stderr, "Couldn't create select statement \"%s\" (%i): %s\n",
						sql, rc, sqlite3_errmsg(db));
		return -1;
	}

	if(SQLITE_ROW != (sqlite3_step(stmt))) {
		fprintf(stderr, "No rows returned from gps analysis table\n");
		return -1;
	}

	int i;
	for(i=0;i<sqlite3_column_count(stmt);i++) {
		fprintf(f, "%s,", sqlite3_column_name(stmt, i));
	}
	fprintf(f, "\n");

	sqlite3_reset(stmt);

	while(SQLITE_ROW == (rc = sqlite3_step(stmt))) {
		for(i=0;i<sqlite3_column_count(stmt);i++) {
			fprintf(f, "%f,", sqlite3_column_double(stmt, i));
		}
		fprintf(f, "\n");
	}

	sqlite3_finalize(stmt);

	return 0;
}



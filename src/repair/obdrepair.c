/* Copyright 2009 Gary Briggs

This file is part of obdgpslogger.

obdgpslogger is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

obdgpslogger is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with obdgpslogger.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "sqlite3.h"

/// Print help
void printhelp(const char *argv0);

/// Check indices on tables
/** \return 0 if we changed nothing. -1 for error. >0 if we changed stuff. */
int checkindices(sqlite3 *db);

/// Fix ends of trips
/** \return 0 if we changed nothing. -1 for error. >0 if we changed stuff. */
int checktripends(sqlite3 *db);

int main(int argc, const char **argv) {
	if(argc < 2 || 0 == strcmp("--help", argv[1]) ||
				0 == strcmp("-h", argv[1])) {
		printhelp(argv[0]);
		exit(0);
	}

	sqlite3 *db;
	int rc;

	if(SQLITE_OK != (rc = sqlite3_open_v2(argv[1], &db, SQLITE_OPEN_READWRITE, NULL))) {
		fprintf(stderr, "Can't open database %s: %s\n", argv[1], sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(1);
	}

	checkindices(db);

	checktripends(db);

	sqlite3_close(db);

	printf("Done\n");
	return 0;
}

void printhelp(const char *argv0) {
	printf("Usage: %s <database>\n"
			"Take a few best guesses at repairing an obdgpslogger log\n", argv0);
}

int checkindices(sqlite3 *db) {
	int retvalue = 0;
	int rc;
	const char *dbend;
	sqlite3_stmt *gpsidx_list_stmt;
	sqlite3_stmt *obdidx_list_stmt;

	const char gpsidx_list_sql[] = "PRAGMA index_list(gps)";
	const char obdidx_list_sql[] = "PRAGMA index_list(obd)";
	const char create_gpsidx_sql[] = "CREATE INDEX IDX_GPSTIME ON gps (time)";
	const char create_obdidx_sql[] = "CREATE INDEX IDX_OBDTIME ON obd (time)";

	// If any one of these failes, the previous ones are a memory leak
	//  This is a throwaway one-short-use low-memory app.
	if(SQLITE_OK != (rc = sqlite3_prepare_v2(db, gpsidx_list_sql, -1, &gpsidx_list_stmt, &dbend))) {
		fprintf(stderr,"Error preparing SQL: (%i) %s\nSQL: \"%s\"\n", rc, sqlite3_errmsg(db), gpsidx_list_sql);
		return -1;
	}

	if(SQLITE_OK != (rc = sqlite3_prepare_v2(db, obdidx_list_sql, -1, &obdidx_list_stmt, &dbend))) {
		fprintf(stderr,"Error preparing SQL: (%i) %s\nSQL: \"%s\"\n", rc, sqlite3_errmsg(db), obdidx_list_sql);
		return -1;
	}

	// Check the GPS table
	sqlite3_bind_text(gpsidx_list_stmt, 1, "gps", strlen("gps"), NULL);
	// It's not "ideal", but we'll assume that if IDX_GPSTIME exists, it's an index for time on the gps table.
	int found_gpstime = 0;
	while(SQLITE_ROW == sqlite3_step(gpsidx_list_stmt)) {
		const char *c = (const char *)sqlite3_column_text(gpsidx_list_stmt, 1);
		if(0 == strcmp("IDX_GPSTIME",c)) {
			printf("Found IDX_GPSTIME on the gps table\n");
			found_gpstime++;
		} else {
			printf("Found index [not IDX_GPSTIME]: %s\n", sqlite3_column_text(gpsidx_list_stmt, 1));
		}
	}
	if(0 == found_gpstime) {
		char *errmsg;
		if(SQLITE_OK != (rc = sqlite3_exec(db, create_gpsidx_sql, NULL, NULL, &errmsg))) {
			fprintf(stderr, "UPDATE db. SQL reported: %s\nSQL: \"%s\"\n", errmsg, create_gpsidx_sql);
			sqlite3_free(errmsg);
			return -1;
		} else {
			printf("Added idx to gps table: \"%s\"\n", create_gpsidx_sql);
		}
	}


	// Check the OBD table
	// It's not "ideal", but we'll assume that if IDX_OBDTIME exists, it's an index for time on the obd table.
	int found_obdtime = 0;
	while(SQLITE_ROW == sqlite3_step(obdidx_list_stmt)) {
		const char *c = (const char *)sqlite3_column_text(obdidx_list_stmt, 1);
		if(0 == strcmp("IDX_OBDTIME",c)) {
			printf("Found IDX_OBDTIME on the obd table\n");
			found_obdtime++;
		} else {
			printf("Found index [not IDX_OBDTIME]: %s\n", sqlite3_column_text(obdidx_list_stmt, 1));
		}
	}
	if(0 == found_obdtime) {
		char *errmsg;
		if(SQLITE_OK != (rc = sqlite3_exec(db, create_obdidx_sql, NULL, NULL, &errmsg))) {
			fprintf(stderr, "UPDATE db. SQL reported: %s\nSQL: \"%s\"\n", errmsg, create_obdidx_sql);
			sqlite3_free(errmsg);
			return -1;
		} else {
			printf("Added idx to obd table: \"%s\"\n", create_obdidx_sql);
		}
	}


	sqlite3_finalize(gpsidx_list_stmt);
	sqlite3_finalize(obdidx_list_stmt);

	return retvalue;
}

int checktripends(sqlite3 *db) {
	int retvalue = 0;
	char *errmsg = NULL;

	const char update_sql[] = "UPDATE trip SET end="
			"(SELECT (t2.start-0.01) AS newstart from trip t2 WHERE trip.tripid=t2.tripid+1) "
		"WHERE trip.end = -1";

	if(SQLITE_OK != sqlite3_exec(db, update_sql, NULL, NULL, &errmsg)) {
		fprintf(stderr, "UPDATE db. SQL reported: %s\nSQL: \"%s\"\n", errmsg, update_sql);
		sqlite3_free(errmsg);
		return -1;
	} else {
		printf("Ran UPDATE sql: \"%s\"\n", update_sql);
	}

	return retvalue;
}


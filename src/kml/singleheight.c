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


/** \file
 \brief Dump a single value in a db as height in a KML document
 */


#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#include "singleheight.h"

#include "sqlite3.h"

void kmlvalueheight(sqlite3 *db, FILE *f, const char *name, const char *desc, const char *columnname, int height, int defaultvis, double start, double end, int trip) {
	int rc; // return from sqlite
	sqlite3_stmt *stmt; // sqlite statement
	const char *dbend; // ignored handle for sqlite

	// For normalising the data
	float normalfactor = 1;
	sqlite3_stmt *normal_stmt;
	char normal_sql[1024];
	
	snprintf(normal_sql, sizeof(normal_sql),
			"SELECT %i/(SELECT MAX(%s) FROM obd WHERE trip=%i)",
			height, columnname, trip
			);
	rc = sqlite3_prepare_v2(db, normal_sql, -1, &normal_stmt, &dbend);
	if(SQLITE_OK != rc) {
		printf("SQL Error in valueheight: %i, %s\n", rc, sqlite3_errmsg(db));
		printf("SQL: %s\n", normal_sql);
		return;
	} else {
		if(SQLITE_ROW != sqlite3_step(normal_stmt)) {
			printf("SQL Error stepping: %i, %s\n", rc, sqlite3_errmsg(db));
			printf("SQL: %s\n", normal_sql);
			return;
		}
		normalfactor=sqlite3_column_double(normal_stmt, 0);
	}
	sqlite3_finalize(normal_stmt);
	
	// And the actual output
	char select_sql[2048]; // the select statement

	snprintf(select_sql,sizeof(select_sql),
					"SELECT T1.obdkmlthing AS height,T2.lat,T2.lon "
					"FROM (SELECT %s AS obdkmlthing,time FROM obd WHERE trip=%i) AS T1 "
					"INNER JOIN (SELECT lat,lon,time FROM gps WHERE trip=%i) AS T2 "
					"ON T1.time=T2.time ",
					columnname, trip, trip);

	rc = sqlite3_prepare_v2(db, select_sql, -1, &stmt, &dbend);

	if(SQLITE_OK != rc) {
		printf("SQL Error in valueheight: %i, %s\n", rc, sqlite3_errmsg(db));
		printf("SQL: %s\n", select_sql);
		return;
	} else {

		fprintf(f,
			"<Document>\n"
			"<Style>\n"
			"<ListStyle><listItemType>checkHideChildren</listItemType></ListStyle>\n"
			"</Style>\n"
			"<visibility>%i</visibility>\n"
			"<name>%s</name>\n"
			"<description>%s</description>\n",defaultvis,name,desc);

		fprintf(f,"<Placemark>\n"
			"<name>chart</name>\n"
			"<LineString>\n"
			"<extrude>1</extrude>\n"
			"<tessellate>1</tessellate>\n"
			"<altitudeMode>relativeToGround</altitudeMode>\n"
			"<coordinates>");

		int ismoving=1; // Set when the car is moving

		int have_firstpos = 0;
		double firstpos[3] = {0,0,0};
		double lastpos[3] = {0,0,0};

		while(SQLITE_ROW == sqlite3_step(stmt)) {
			if(0 == have_firstpos) {
				firstpos[2] = sqlite3_column_double(stmt, 2);
				firstpos[1] = sqlite3_column_double(stmt, 1);
				firstpos[0] = sqlite3_column_double(stmt, 0);
				have_firstpos = 1;
			}

			float height = sqlite3_column_double(stmt, 0);
			if(abs(height) > 0.001) {
				ismoving = 1;
			}
			if(ismoving) {
				fprintf(f, "%f,%f,%f\n", sqlite3_column_double(stmt, 2),sqlite3_column_double(stmt, 1),normalfactor*height);
			}
			if(abs(height) < 0.001) {
				ismoving = 0;
			}

			lastpos[2] = sqlite3_column_double(stmt, 2);
			lastpos[1] = sqlite3_column_double(stmt, 1);
			lastpos[0] = sqlite3_column_double(stmt, 0);

		}

		fprintf(f,"</coordinates>\n"
			"</LineString>\n"
			"</Placemark>\n");

		// Now print start and end beacons
		time_t endt = (time_t)floor(end);
		time_t startt = (time_t)floor(start);

		fprintf(f, "<Placemark>\n"
			"<name>Start (%s)</name>\n"
			"<Point>\n"
			"<coordinates>\n"
			"%f,%f,%f"
			"</coordinates>\n"
			"</Point>\n"
			"</Placemark>\n", ctime(&startt),
				firstpos[2],firstpos[1],firstpos[0]);

		fprintf(f, "<Placemark>\n"
			"<name>End (%s)</name>\n"
			"<Point>\n"
			"<coordinates>\n"
			"%f,%f,%f"
			"</coordinates>\n"
			"</Point>\n"
			"</Placemark>\n", ctime(&endt),
				lastpos[2],lastpos[1],lastpos[0]);

		fprintf(f,"</Document>\n");
	}

	sqlite3_finalize(stmt);
}





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

#include "singleheight.h"

#include "sqlite3.h"

void kmlvalueheight(sqlite3 *db, FILE *f, const char *name, const char *desc, const char *columnname, int height, int defaultvis, double start, double end) {
	int rc; // return from sqlite
	sqlite3_stmt *stmt; // sqlite statement
	const char *dbend; // ignored handle for sqlite

	char select_sql[2048]; // the select statement

	snprintf(select_sql,sizeof(select_sql),
					"SELECT %i*%s/(SELECT MAX(%s) FROM obd) AS height,gps.lat, gps.lon "
					"FROM obd INNER JOIN gps ON obd.time=gps.time "
					"WHERE obd.time>%f AND obd.time<%f",
					height, columnname, columnname, start, end);

	rc = sqlite3_prepare_v2(db, select_sql, -1, &stmt, &dbend);

	if(rc != SQLITE_OK) {
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

		while(SQLITE_DONE != sqlite3_step(stmt)) {
			if(abs(sqlite3_column_double(stmt, 0)) > 0.001) {
				ismoving = 1;
			}
			if(ismoving) {
				fprintf(f, "%f,%f,%f\n", sqlite3_column_double(stmt, 2),sqlite3_column_double(stmt, 1),sqlite3_column_double(stmt, 0));
			}
			if(abs(sqlite3_column_double(stmt, 0)) < 0.001) {
				ismoving = 0;
			}
		}

		fprintf(f,"</coordinates>\n"
			"</LineString>\n"
			"</Placemark>\n"
			"</Document>\n");
	}

	sqlite3_finalize(stmt);
}





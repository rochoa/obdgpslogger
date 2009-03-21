/** \file
 \brief Dump a single value in a db as height in a KML document
 */


#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "singleheight.h"

#include "sqlite3.h"

void kmlvalueheight(sqlite3 *db, FILE *f, const char *columnname, int height) {
	int rc; // return from sqlite
	sqlite3_stmt *stmt; // sqlite statement
	const char *dbend; // ignored handle for sqlite

	char select_sql[2048]; // the select statement

	snprintf(select_sql,sizeof(select_sql),
					"SELECT %i*obd.%s/(SELECT MAX(%s) FROM obd) AS height,gps.lat, gps.lon "
					"FROM obd INNER JOIN gps ON obd.time=gps.time\n", height, columnname, columnname);

	rc = sqlite3_prepare(db, select_sql, -1, &stmt, &dbend);

	if(rc != SQLITE_OK) {
		printf("SQL Error in valueheight: %i\n", rc);
		return;
	} else {
		fprintf(f,"<Placemark>\n"
			"<name>Speed and Position</name>\n"
			"<description>Height indicates speed</description>\n"
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
				"</Placemark>\n");
	}

}





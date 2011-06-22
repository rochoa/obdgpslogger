/* Copyright 2011 Gary Briggs

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
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "sqlite3.h"

#include "obdservicecommands.h"

/// Print help
void printhelp(const char *argv0);

int main(int argc, char *argv[]) {
	if(argc < 2 || 0 == strcmp("--help", argv[1]) ||
			0 == strcmp("-h", argv[1])) {
		printhelp(argv[0]);
		exit(0);
	}

	sqlite3 *db;

	int rc;
	rc = sqlite3_open_v2(argv[1], &db, SQLITE_OPEN_READWRITE, NULL);
	if(SQLITE_OK != rc) {
		fprintf(stderr, "Can't open database %s: %s\n", argv[1], sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(1);
	}

	const char *columnlist_sql = "PRAGMA table_info(obd)";
	sqlite3_stmt *columnlist_stmt;

	rc = sqlite3_prepare_v2(db, columnlist_sql, -1, &columnlist_stmt, NULL);
	if(SQLITE_OK != rc) {
		fprintf(stderr, "Cannot prepare select statement columnlist (%i): %s\n", rc, sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(1);
	}

	int i;

	int colcount = 0;
	struct obdservicecmd *columns[20]; // Only support this many columns at most.
	while(SQLITE_ROW == sqlite3_step(columnlist_stmt)) {
		struct obdservicecmd *cmd = obdGetCmdForColumn(sqlite3_column_text(columnlist_stmt, 1));
		if(NULL != cmd) {
			columns[colcount] = cmd;
			colcount++;
		}
	}
	sqlite3_finalize(columnlist_stmt);

	char select_format[] = "SELECT %s FROM obd "
		"ORDER BY %s";
	char select_format_vss[] = "SELECT %s FROM obd "
		"WHERE VSS>0.01 "
		"ORDER BY %s";

	FILE *gnuplot = popen("gnuplot -persist", "w");

	fprintf(gnuplot, "set datafile separator ','\n");
	fprintf(gnuplot, "set terminal png\n");
	fprintf(gnuplot, "set nokey\n");
	fprintf(gnuplot, "set boxwidth 0.5\n");

	for(i=0;i<colcount;i++) {
		char gnuplot_datname[] = "/tmp/obd_XXXXXX";
		int gnuplot_datfd = mkstemp(gnuplot_datname);
		if(0 > gnuplot_datfd) {
			perror("Error creating temporary gnuplot datafile");
			exit(1);
		}

		sqlite3_stmt *stmt;
		const char *db_column = columns[i]->db_column;
		printf("Column: %s\n", db_column);
		char dataline[1024];

		char sql[1024];
		// Number of rows
		int cnt = 0;
		// Row position for each quartile
		int pos[5];
		double quartiles[5];

		// Data range
		double maxVal = -10000;
		double minVal = 10000;

		// Get the "with VSS" version
		snprintf(sql, sizeof(sql), select_format, db_column, db_column);

		rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
		if(SQLITE_OK != rc) {
			fprintf(stderr, "Cannot prepare select statement for col %s (%i): %s\n",
				db_column, rc, sqlite3_errmsg(db));
			sqlite3_close(db);
			exit(1);
		}

		cnt = 0;
		while(SQLITE_ROW == sqlite3_step(stmt)) cnt++;
		sqlite3_reset(stmt);

		pos[0] = 0;
		pos[1] = (int)(cnt * 1.0/4.0);
		pos[2] = (int)(cnt * 2.0/4.0);
		pos[3] = (int)(cnt * 3.0/4.0);
		pos[4] = cnt-1;

		cnt = 0;
		while(SQLITE_ROW == sqlite3_step(stmt)) {
			int j;
			for(j=0;j<=5;j++) {
				if(cnt == pos[j]) {
					quartiles[j] = sqlite3_column_double(stmt,0);
					if(quartiles[j] > maxVal) {
						maxVal = quartiles[j];
					}
					if(quartiles[j] < minVal) {
						minVal = quartiles[j];
					}
				}
			}
			cnt++;
		}
		snprintf(dataline, sizeof(dataline),"%s,1,%f,%f,%f,%f,%f\n", db_column,
			quartiles[0], quartiles[1],
			quartiles[2], quartiles[3],
			quartiles[4]);
		write(gnuplot_datfd, dataline, strlen(dataline));

		sqlite3_finalize(stmt);

		// Get the "no VSS" version
		snprintf(sql, sizeof(sql), select_format_vss, db_column, db_column);

		rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
		if(SQLITE_OK != rc) {
			fprintf(stderr, "Cannot prepare select statement for col %s (%i): %s\n",
				db_column, rc, sqlite3_errmsg(db));
			sqlite3_close(db);
			exit(1);
		}

		cnt = 0;
		while(SQLITE_ROW == sqlite3_step(stmt)) cnt++;
		sqlite3_reset(stmt);

		pos[0] = 0;
		pos[1] = (int)(cnt * 1.0/4.0);
		pos[2] = (int)(cnt * 2.0/4.0);
		pos[3] = (int)(cnt * 3.0/4.0);
		pos[4] = cnt-1;

		cnt = 0;
		while(SQLITE_ROW == sqlite3_step(stmt)) {
			int j;
			for(j=0;j<=5;j++) {
				if(cnt == pos[j]) {
					quartiles[j] = sqlite3_column_double(stmt,0);
					if(quartiles[j] > maxVal) {
						maxVal = quartiles[j];
					}
					if(quartiles[j] < minVal) {
						minVal = quartiles[j];
					}
				}
			}
			cnt++;
		}
		snprintf(dataline, sizeof(dataline),"%s_moving,2,%f,%f,%f,%f,%f\n", db_column,
			quartiles[0], quartiles[1],
			quartiles[2], quartiles[3],
			quartiles[4]);
		write(gnuplot_datfd, dataline, strlen(dataline));

		sqlite3_finalize(stmt);

		fsync(gnuplot_datfd);
		close(gnuplot_datfd);
		
		maxVal += 0.05 * (maxVal-minVal);
		minVal -= 0.05 * (maxVal-minVal);

		fprintf(gnuplot, "set yrange [%f:%f]\n", minVal, maxVal);
		fprintf(gnuplot, "set xrange [0.5:2.5]\n");
		fprintf(gnuplot, "set title \"%s\"\n", columns[i]->human_name);
		fprintf(gnuplot, "set ylabel \"%s\"\n", columns[i]->units);
		fprintf(gnuplot, "set xtics (\"%s\" 1, \"%s (when speed>0)\" 2)\n", db_column, db_column);
		fprintf(gnuplot, "set output \"%s.png\"\n", db_column);
		fprintf(gnuplot, "plot ");
		fprintf(gnuplot, "\"%s\" u 2:4:3:7:6 t \"%s\" lt 1 w candlesticks whiskerbars,\\\n",
			gnuplot_datname, db_column);
		fprintf(gnuplot, "\"%s\" u 2:5:5:5:5 lt 1 w candlesticks whiskerbars\n",
			gnuplot_datname, db_column);

		fsync(fileno(gnuplot));

		// unlink(gnuplot_datname);
	}

	fclose(gnuplot);

	// unlink(gnuplot_datname);
	sqlite3_close(db);
}


void printhelp(const char *argv0) {
	printf("Usage: %s <database>\n"
		"New adventures in box and whisker plots, using gnuplot\n", argv0);
}


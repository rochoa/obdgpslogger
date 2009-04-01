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
 \brief OBD GPS KML main entrypoint
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "obdgpskml.h"
#include "singleheight.h"
#include "heightandcolor.h"

#include "sqlite3.h"

int main(int argc, char **argv) {

	/// Output file
	FILE *outfile;

	/// Database to dump
	sqlite3 *db;

	/// outfile filename
	char *outfilename = NULL;

	/// Database file to open
	char *databasename = NULL;

	/// Name for kml folder
	char *kmlfoldername = NULL;

	/// Max altitiude to chart to
	int maxaltitude = DEFAULT_MAXALTITUDE;

	/// getopt's current option
	int optc;

	/// might get set during option parsing. Exit when done parsing
	int mustexit = 0;

	while ((optc = getopt_long (argc, argv, kmlshortopts, kmllongopts, NULL)) != -1) {
		switch (optc) {
			case 'h':
				kmlprinthelp(argv[0]);
				mustexit = 1;
				break;
			case 'v':
				kmlprintversion();
				mustexit = 1;
				break;
			case 'd':
				if(NULL != databasename) {
					free(databasename);
				}
				databasename = strdup(optarg);
				break;
			case 'o':
				if(NULL != outfilename) {
					free(outfilename);
				}
				outfilename = strdup(optarg);
				break;
			case 'n':
				if(NULL != kmlfoldername) {
					free(kmlfoldername);
				}
				kmlfoldername = strdup(optarg);
				break;
			case 'a':
				maxaltitude = atoi(optarg);
				break;
			default:
				kmlprinthelp(argv[0]);
				mustexit = 1;
				break;
		}
	}
	if(mustexit) exit(0);

	if(NULL == databasename) {
		databasename = DEFAULT_DATABASE;
	}

	if(NULL == outfilename) {
		outfilename = DEFAULT_OUTFILENAME;
	}

	if(NULL == kmlfoldername) {
		kmlfoldername = DEFAULT_KMLFOLDERNAME;
	}


	// sqlite return status
	int rc;
	rc = sqlite3_open(databasename, &db);
	if( SQLITE_OK != rc ) {
		fprintf(stderr, "Can't open database %s: %s\n", databasename, sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(1);
	}

	outfile = fopen(outfilename, "w");
	if(NULL == outfile) {
		perror(outfilename);
		exit(1);
	}

	fprintf(outfile, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n"
		"<Folder>\n"
		"<name>%s</name>\n"
		"<description>OBD GPS Logger [http://icculus.org/obdgpslogger] was used to log a car journey and export this kml file</description>\n",
		kmlfoldername);

	writekmlgraphs(db,outfile,maxaltitude);


	fprintf(outfile,"</Folder>\n</kml>\n\n");

	fclose(outfile);
	sqlite3_close(db);

	return 0;
}

void writekmlgraphs(sqlite3 *db, FILE *f, int maxaltitude) {
	// Before entering this function, you should have written all the xml fluff
	//  that comes at the top of the kml file, and be ready to dump the other fluff afterwards
	
	sqlite3_stmt *trip_stmt;
	char select_trip_sql[] = "SELECT tripid,start,end FROM trip ORDER BY tripid";
	int rc;
	const char *dbend;

	rc = sqlite3_prepare_v2(db, select_trip_sql, -1, &trip_stmt, &dbend);
	if(rc != SQLITE_OK) {
		printf("SQL Error in trip select(%i): %s\n", rc, sqlite3_errmsg(db));
		return;
	}

	// Do a simple RPM vs position one first:
	while(SQLITE_DONE != sqlite3_step(trip_stmt)) {
		char graphname[64];
		snprintf(graphname, sizeof(graphname), "RPM and Position, trip #%i\n", sqlite3_column_int(trip_stmt, 0));

		printf("Writing graph %s\n", graphname);

		kmlvalueheight(db,f, graphname, "Height indicates engine revs", "rpm", maxaltitude, 0,
			sqlite3_column_double(trip_stmt, 1), sqlite3_column_double(trip_stmt, 2));
	}

	sqlite3_reset(trip_stmt);

	while(SQLITE_DONE != sqlite3_step(trip_stmt)) {
		char graphname[64];
		snprintf(graphname, sizeof(graphname), "MPG, Speed and Position, trip #%i\n", sqlite3_column_int(trip_stmt, 0));

		printf("Writing graph %s\n", graphname);

		kmlvalueheightcolor(db,f,graphname, "Height indicates speed, color indicates mpg [green == better]",
			"vss",maxaltitude, "(710.7*vss/maf)", 5, 1,
			sqlite3_column_double(trip_stmt, 1), sqlite3_column_double(trip_stmt, 2));
	}

	sqlite3_finalize(trip_stmt);
}

void kmlprinthelp(const char *argv0) {
	printf("Usage: %s [params]\n"
		"   [-o|--out[=" DEFAULT_OUTFILENAME "]\n"
		"   [-d|--db[=" DEFAULT_DATABASE "]]\n"
		"   [-n|--name[=" DEFAULT_KMLFOLDERNAME "]]\n"
		"   [-a|--altitude[=%i]]\n"
		"   [-v|--version] [-h|--help]\n", argv0, DEFAULT_MAXALTITUDE);
}

void kmlprintversion() {
	printf("Version: %i.%i\n", OBDLOGGER_MAJOR_VERSION, OBDLOGGER_MINOR_VERSION);
}



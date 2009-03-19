/* \file
 \brief OBD GPS KML main entrypoint
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "obdgpskml.h"
#include "singleheight.h"

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

	fprintf(outfile,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n"
		"<Document>\n"
		"<name>OBD GPS Logger</name>\n"
		"<description>Output from OBD GPS Logger</description>\n"
	);


	// Now can fprintf some <Placemark>s
	kmlvalueheight(db,outfile,"vss",maxaltitude);


	fprintf(outfile,"</Document>\n\n");

	fclose(outfile);
	sqlite3_close(db);

	return 0;
}

void kmlprinthelp(const char *argv0) {
	printf("Usage: %s [params]\n"
		"   [-o|--out[=" DEFAULT_OUTFILENAME "]\n"
		"   [-d|--db[=" DEFAULT_DATABASE "]]\n"
		"   [-a|--altitude[=%i]]\n"
		"   [-v|--version] [-h|--help]\n", argv0, DEFAULT_MAXALTITUDE);
}

void kmlprintversion() {
	printf("Version: %i.%i\n", OBDLOGGER_MAJOR_VERSION, OBDLOGGER_MINOR_VERSION);
}



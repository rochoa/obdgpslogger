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
 \brief main obdlogger entrypoint
 */

// Some references:
// mpg calculation: http://www.mp3car.com/vbulletin/engine-management-obd-ii-engine-diagnostics-etc/75138-calculating-mpg-vss-maf-obd2.html
// function list: http://www.kbmsystems.net/obd_tech.htm

#include "main.h"
#include "obdservicecommands.h"
#include "database.h"
#include "obddb.h"
#include "gpsdb.h"
#include "tripdb.h"
#include "obdserial.h"
#include "gpscomm.h"

#ifdef HAVE_GPSD
#include "gps.h"
#endif //HAVE_GPSD
#include "sqlite3.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <getopt.h>
#include <unistd.h>

int main(int argc, char** argv) {
	/// Serial port full path to open
	char *serialport = NULL;

	/// Database file to open
	char *databasename = NULL;

	/// Number of samples to take
	int samplecount = -1;

	/// Number of samples per second
	int samplespersecond = 1;

	/// Time between samples, measured in microseconds
	long frametime = 0;

	int optc;
	int mustexit = 0;
	while ((optc = getopt_long (argc, argv, shortopts, longopts, NULL)) != -1) {
		switch (optc) {
			case 'h':
				printhelp(argv[0]);
				mustexit = 1;
				break;
			case 'v':
				printversion();
				mustexit = 1;
				break;
			case 's':
				if(NULL != serialport) {
					free(serialport);
				}
				serialport = strdup(optarg);
				break;
			case 'c':
				samplecount = atoi(optarg);
				break;
			case 'd':
				if(NULL != databasename) {
					free(databasename);
				}
				databasename = strdup(optarg);
				break;
			case 'a':
				samplespersecond = atoi(optarg);
				break;
			default:
				mustexit = 1;
				break;
		}
	}

	if(mustexit) exit(0);

	if(0 >= samplespersecond) {
		frametime = 0;
	} else {
		frametime = 1000000 / samplespersecond;
	}

	if(NULL == serialport) {
		serialport = DEFAULT_SERIAL_PORT;
	}
	if(NULL == databasename) {
		databasename = DEFAULT_DATABASE;
	}


	// sqlite database
	sqlite3 *db;

	// sqlite statement
	sqlite3_stmt *obdinsert;

	// number of columns in the insert
	int obdnumcols;

	// sqlite return status
	int rc;

	const char *zTail;

	// Open the database and create the obd table
	if(NULL == (db = opendb(databasename))) {
		exit(1);
	}

	createobdtable(db);

	// Create the insert statement. On success, we'll have the number of columns
	if(0 == (obdnumcols = createobdinsertstmt(db,&obdinsert)) || NULL == obdinsert) {
		closedb(db);
		exit(1);
	}

	createtriptable(db);

	int cmdlist[obdnumcols-1]; // The last column is time
	int i,j;
	for(i=0,j=0; i<sizeof(obdcmds)/sizeof(obdcmds[0]); i++) {
		if(NULL != obdcmds[i].db_column) {
			cmdlist[j++] = obdcmds[i].cmdid;
		}
	}


#ifdef HAVE_GPSD
	// sqlite statement
	sqlite3_stmt *gpsinsert;

	// number of columns in the insert
	int gpsnumcols;

	creategpstable(db);

	if(0 == (gpsnumcols = creategpsinsertstmt(db, &gpsinsert) || NULL == gpsinsert)) {
		closedb(db);
		exit(1);
	}
#endif //HAVE_GPSD


	// Open the serial port.
	int obd_serial_port = openserial(serialport);

	if(-1 == obd_serial_port) {
		printf("Couldn't open obd serial port.\n");
	} else {
		printf("Successfully connected to serial port. Will log obd data\n");
	}



#ifdef HAVE_GPSD
	// Open the gps device
	struct gps_data_t *gpsdata;
	gpsdata = opengps("127.0.0.1", "2947");

	if(NULL == gpsdata) {
		printf("Couldn't open gps port.\n");
	} else {
		printf("Successfully connected to gpsd. Will log gps data\n");
	}

#endif //HAVE_GPSD


	// Quit if we couldn't find anything to log
	if(-1 == obd_serial_port
#ifdef HAVE_GPSD
					&& NULL == gpsdata
#endif //HAVE_GPSD
				) {
		printf("Couldn't find anything to log. Exiting\n");
		closedb(db);
		exit(1);
	}

#ifdef HAVE_GPSD
	// Ping a message to stdout the first time we get
	//   enough of a satellite lock to begin logging
	int have_gps_lock = 0;
#endif //HAVE_GPSD


	// The current thing returned by starttrip
	sqlite3_int64 currenttrip;

	// Set when we're actually inside a trip
	int ontrip = 0;


	while(samplecount == -1 || samplecount-- > 0) {

		struct timeval starttime; // start time through loop
		struct timeval endtime; // end time through loop
		struct timeval selecttime; // =endtime-starttime [for select()]

		if(0 != gettimeofday(&starttime,NULL)) {
			perror("Couldn't gettimeofday");
			break;
		}

		double time_insert = (double)starttime.tv_sec+(double)starttime.tv_usec/1000000.0f;

		enum obd_serial_status obdstatus;
		if(-1 < obd_serial_port) {

			// Get all the OBD data
			for(i=0; i<obdnumcols-1; i++) {
				long val;
				obdstatus = getobdvalue(obd_serial_port, cmdlist[i], &val);
				if(OBD_SUCCESS == obdstatus) {
					sqlite3_bind_int(obdinsert, i+1, (int)val);
					// printf("cmd: %02X, val: %02li\n",cmdlist[i],val);
				} else {
					break;
				}
			}

			if(obdstatus == OBD_SUCCESS) {
				sqlite3_bind_double(obdinsert, i+1, time_insert);

				// Do the OBD insert
				rc = sqlite3_step(obdinsert);
				if(SQLITE_DONE != rc) {
					printf("sqlite3 obd insert failed(%i): %s\n", rc, sqlite3_errmsg(db));
				}

				// If they're not on a trip but the engine is going, start a trip
				if(0 == ontrip) {
					printf("Creating a new trip\n");
					currenttrip = starttrip(db, time_insert);
					ontrip = 1;
				}
			} else {
				// If they're on a trip, and the engine has desisted, stop the trip
				if(0 != ontrip) {
					printf("Ending current trip\n");
					endtrip(db, time_insert, currenttrip);
					ontrip = 0;
				}
			}
			sqlite3_reset(obdinsert);
		}


#ifdef HAVE_GPSD
		// Get the GPS data
		double lat,lon,alt;

		int gpsstatus = -1;
		if(NULL != gpsdata) {
			gpsstatus = getgpsposition(gpsdata, &lat, &lon, &alt);
		}
		if(gpsstatus < 0 || NULL == gpsdata) {
			// Nothing yet
		} else if(gpsstatus >= 0) {
			if(0 == have_gps_lock) {
				printf("GPS acquisition complete\n");
				have_gps_lock = 1;
			}

			sqlite3_bind_double(gpsinsert, 1, lat);
			sqlite3_bind_double(gpsinsert, 2, lon);
			if(gpsstatus >= 1) {
				sqlite3_bind_double(gpsinsert, 3, alt);
			} else {
				sqlite3_bind_double(gpsinsert, 3, -1000.0);
			}
			// Use time worked out before.
			//  This makes table joins reliable, but the time itself may be wrong depending on gpsd lagginess
			sqlite3_bind_double(gpsinsert, 4, time_insert);

			// Do the GPS insert
			rc = sqlite3_step(gpsinsert);
			if(SQLITE_DONE != rc) {
				printf("sqlite3 gps insert failed(%i): %s\n", rc, sqlite3_errmsg(db));
			}
			sqlite3_reset(gpsinsert);
		}
#endif //HAVE_GPSD

		if(0 != gettimeofday(&endtime,NULL)) {
			perror("Couldn't gettimeofday");
			break;
		}

		// usleep() not as portable as select()
		
		if(0 != frametime) {
			selecttime.tv_sec = endtime.tv_sec - starttime.tv_sec;
			if (selecttime.tv_sec != 0) {
					endtime.tv_usec += 1000000*selecttime.tv_sec;
					selecttime.tv_sec = 0;
			}
			selecttime.tv_usec = (frametime) - 
					(endtime.tv_usec - starttime.tv_usec);
			if(selecttime.tv_usec < 0) {
					selecttime.tv_usec = 1;
			}
			select(0,NULL,NULL,NULL,&selecttime);
		}
	}

	sqlite3_finalize(obdinsert);
#ifdef HAVE_GPSD
	sqlite3_finalize(gpsinsert);
#endif //HAVE_GPSD

	closeserial(obd_serial_port);
	closedb(db);


	return 0;
}


void printhelp(const char *argv0) {
	printf("Usage: %s [params]\n"
				"   [-s|--serial[=" DEFAULT_SERIAL_PORT "]]\n"
				"   [-c|--count[=infinite]\n"
				"   [-a|--samplerate[=1]\n"
				"   [-d|--db[=" DEFAULT_DATABASE "]]\n"
				"   [-v|--version] [-h|--help]\n", argv0);
}

void printversion() {
	printf("Version: %i.%i\n", OBDLOGGER_MAJOR_VERSION, OBDLOGGER_MINOR_VERSION);
}


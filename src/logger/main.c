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

#include "obdconfig.h"
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

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif // HAVE_SIGNAL_H

/// Set when we catch a signal we want to exit on
static int receive_exitsignal = 0;

/// If we catch a signal to start the trip, set this
static int sig_starttrip = 0;

/// If we catch a signal to end the trip, set this
static int sig_endtrip = 0;

static void catch_quitsignal(int sig) {
	receive_exitsignal = 1;
}

static void catch_tripstartsignal(int sig) {
	sig_starttrip = 1;
}

static void catch_tripendsignal(int sig) {
	sig_endtrip = 1;
}

int main(int argc, char** argv) {
	/// Serial port full path to open
	char *serialport = NULL;

	/// Database file to open
	char *databasename = NULL;

	/// Number of samples to take
	int samplecount = -1;

	/// Number of samples per second
	int samplespersecond = 1;

	/// Ask to show the capabilities of the OBD device then exit
	int showcapabilities = 0;

	/// Time between samples, measured in microseconds
	long frametime = 0;

	/// Disable automatic trip starting and stopping
	int disable_autotrip = 0;

	/// Spam all readings to stdout
	int spam_stdout = 0;

	/// Enable elm optimisations
	int enable_optimisations = 0;

	/// Enable serial logging
	int enable_seriallog = 0;

	/// Serial log filename
	char *seriallogname = NULL;

	// Do not attempt to buffer stdout at all
	setvbuf(stdout, (char *)NULL, _IONBF, 0);

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
			case 'n':
				disable_autotrip = 1;
				break;
			case 'o':
				enable_optimisations = 1;
				break;
			case 't':
				spam_stdout = 1;
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
			case 'l':
				enable_seriallog = 1;
				if(NULL != seriallogname) {
					free(seriallogname);
				}
				seriallogname = strdup(optarg);
				break;
			case 'p':
				showcapabilities = 1;
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
		serialport = OBD_DEFAULT_SERIALPORT;
	}
	if(NULL == databasename) {
		databasename = OBD_DEFAULT_DATABASE;
	}


	if(enable_seriallog && NULL != seriallogname) {
		startseriallog(seriallogname);
	}

	// Open the serial port.
	int obd_serial_port = openserial(serialport);

	if(-1 == obd_serial_port) {
		fprintf(stderr, "Couldn't open obd serial port.\n");
	} else {
		fprintf(stderr, "Successfully connected to serial port. Will log obd data\n");
	}

	// Just figure out our car's OBD port capabilities and print them
	if(showcapabilities) {
		if(-1 == obd_serial_port) {
			fprintf(stderr, "No capabilities when we can't open serial port\n");
			exit(1);
		}

		unsigned int A,B,C,D;
		int bytes_returned;
		enum obd_serial_status cap_status;
		unsigned int current_cmd = 0x00;

		printf("Your OBD Device claims to support PIDs:\n");

		while(1) {
			cap_status = getobdbytes(obd_serial_port, current_cmd, 0,
				&A, &B, &C, &D, &bytes_returned);

			if(OBD_SUCCESS != cap_status || 4 != bytes_returned) {
				fprintf(stderr, "Couldn't get obd bytes for cmd %02X\n", current_cmd);
				exit(1);
			}

			unsigned long val;
			val = (unsigned long)A*(256*256*256) + (unsigned long)B*(256*256) + (unsigned long)C*(256) + (unsigned long)D;

			int currbit;
			int c;
			for(c=current_cmd+1, currbit=31 ; currbit>=0 ; currbit--, c++) {
				if(val & ((unsigned long)1<<currbit)) {
					if(c > sizeof(obdcmds)/sizeof(obdcmds[0])) {
						printf("%02X: unknown\n", c);
					} else {
						printf("%02X: %s\n", c, obdcmds[c].human_name);
					}
				}
			}

			if(D&0x01) {
				current_cmd += 0x20;
			} else {
				break;
			}
		}
		exit(0);
	}


#ifdef HAVE_GPSD
	// Open the gps device
	struct gps_data_t *gpsdata;
	gpsdata = opengps("127.0.0.1", "2947");

	if(NULL == gpsdata) {
		fprintf(stderr, "Couldn't open gps port.\n");
	} else {
		fprintf(stderr, "Successfully connected to gpsd. Will log gps data\n");
	}

#endif //HAVE_GPSD


	// Quit if we couldn't find anything to log
	if(-1 == obd_serial_port
#ifdef HAVE_GPSD
					&& NULL == gpsdata
#endif //HAVE_GPSD
				) {
		fprintf(stderr, "Couldn't find anything to log. Exiting\n");
		exit(1);
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

	// All of these have obdnumcols-1 since the last column is time
	int cmdlist[obdnumcols-1]; // Commands to send [index into obdcmds]

	int i,j;
	for(i=0,j=0; i<sizeof(obdcmds)/sizeof(obdcmds[0]); i++) {
		if(NULL != obdcmds[i].db_column) {
			cmdlist[j] = i;
			j++;
		}
	}


	// We create the gps table even if gps is disabled, so that other
	//  SQL commands expecting the table to at least exist will work.

	// sqlite statement
	sqlite3_stmt *gpsinsert;

	// number of columns in the insert
	int gpsnumcols;

	creategpstable(db);

	if(0 == (gpsnumcols = creategpsinsertstmt(db, &gpsinsert) || NULL == gpsinsert)) {
		closedb(db);
		exit(1);
	}


#ifdef HAVE_GPSD
	// Ping a message to stdout the first time we get
	//   enough of a satellite lock to begin logging
	int have_gps_lock = 0;
#endif //HAVE_GPSD

	// Set up signal handling

#ifdef HAVE_SIGACTION
	struct sigaction sa_new;

	// Exit on ctrl+c
	sa_new.sa_handler = catch_quitsignal;
	sigemptyset(&sa_new.sa_mask);
	sigaddset(&sa_new.sa_mask, SIGINT);
	sigaction(SIGINT, &sa_new, NULL);

#ifdef SIGUSR1
	// Start a trip on USR1
	sa_new.sa_handler = catch_tripstartsignal;
	sigemptyset(&sa_new.sa_mask);
	sigaddset(&sa_new.sa_mask, SIGUSR1);
	sigaction(SIGUSR1, &sa_new, NULL);
#endif //SIGUSR1

#ifdef SIGUSR2
	// End a trip on USR2
	sa_new.sa_handler = catch_tripendsignal;
	sigemptyset(&sa_new.sa_mask);
	sigaddset(&sa_new.sa_mask, SIGUSR2);
	sigaction(SIGUSR2, &sa_new, NULL);
#endif //SIGUSR2

#else // HAVE_SIGACTION

// If your unix implementation doesn't have sigaction, we can fall
//  back to the older [bad, unsafe] signal().
#ifdef HAVE_SIGNAL_FUNC

	// Exit on ctrl+c
	signal(SIGINT, catch_quitsignal);

#ifdef SIGUSR1
	// Start a trip on USR1
	signal(SIGUSR1, catch_tripstartsignal);
#endif //SIGUSR1

#ifdef SIGUSR2
	// Start a trip on USR2
	signal(SIGUSR2, catch_tripstartsignal);
#endif //SIGUSR2

#endif // HAVE_SIGNAL_FUNC


#endif //HAVE_SIGACTION



	// The current thing returned by starttrip
	sqlite3_int64 currenttrip;

	// Set when we're actually inside a trip
	int ontrip = 0;

	// The current time we're inserting
	double time_insert;

	while(samplecount == -1 || samplecount-- > 0) {

		struct timeval starttime; // start time through loop
		struct timeval endtime; // end time through loop
		struct timeval selecttime; // =endtime-starttime [for select()]

		if(0 != gettimeofday(&starttime,NULL)) {
			perror("Couldn't gettimeofday");
			break;
		}

		time_insert = (double)starttime.tv_sec+(double)starttime.tv_usec/1000000.0f;

		if(sig_endtrip) {
			if(ontrip) {
				fprintf(stderr,"Ending current trip\n");
				endtrip(db, time_insert, currenttrip);
				ontrip = 0;
			}
			sig_endtrip = 0;
		}

		if(sig_starttrip) {
			if(!ontrip) {
				fprintf(stderr,"Creating a new trip\n");
				currenttrip = starttrip(db, time_insert);
				ontrip = 1;
			}
			sig_starttrip = 0;
		}

		enum obd_serial_status obdstatus;
		if(-1 < obd_serial_port) {

			// Get all the OBD data
			for(i=0; i<obdnumcols-1; i++) {
				float val;
				unsigned int cmdid = obdcmds[cmdlist[i]].cmdid;
				int numbytes = enable_optimisations?obdcmds[cmdlist[i]].bytes_returned:0;
				OBDConvFunc conv = obdcmds[cmdlist[i]].conv;

				obdstatus = getobdvalue(obd_serial_port, cmdid, &val, numbytes, conv);
				if(OBD_SUCCESS == obdstatus) {
					if(spam_stdout) {
						printf("%s=%f\n", obdcmds[cmdlist[i]].db_column, val);
					}
					sqlite3_bind_double(obdinsert, i+1, (double)val);
					// printf("cmd: %02X, val: %f\n",obdcmds[cmdlist[i]].cmdid,val);
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
				if(0 == ontrip && !disable_autotrip) {
					printf("Creating a new trip\n");
					currenttrip = starttrip(db, time_insert);
					ontrip = 1;
				}
			} else {
				// If they're on a trip, and the engine has desisted, stop the trip
				if(0 != ontrip && !disable_autotrip) {
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
				fprintf(stderr,"GPS acquisition complete\n");
				have_gps_lock = 1;
			}

			sqlite3_bind_double(gpsinsert, 1, lat);
			sqlite3_bind_double(gpsinsert, 2, lon);
			if(gpsstatus >= 1) {
				sqlite3_bind_double(gpsinsert, 3, alt);
			} else {
				sqlite3_bind_double(gpsinsert, 3, -1000.0);
			}

			if(spam_stdout) {
				printf("gpspos=%f,%f,%f\n", (float)lat, (float)lon, (float)(gpsstatus>=1?alt:-1000.0));
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

		// Set via the signal handler
		if(receive_exitsignal) {
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

	if(0 != ontrip) {
		endtrip(db, time_insert, currenttrip);
		ontrip = 0;
	}

	sqlite3_finalize(obdinsert);
	sqlite3_finalize(gpsinsert);

	closeserial(obd_serial_port);
#ifdef HAVE_GPSD
	gps_close(gpsdata);
#endif //HAVE_GPSD
	closedb(db);

	if(enable_seriallog) {
		closeseriallog();
	}


	return 0;
}


void printhelp(const char *argv0) {
	printf("Usage: %s [params]\n"
				"   [-s|--serial[=" OBD_DEFAULT_SERIALPORT "]]\n"
				"   [-c|--count[=infinite]]\n"
				"   [-n|--no-autotrip]\n"
				"   [-t|--spam-stdout]\n"
				"   [-p|--capabilities]\n"
				"   [-o|--enable-optimisations]\n"
				"   [-l|--serial-log=<filename>]\n"
				"   [-a|--samplerate[=1]]\n"
				"   [-d|--db[=" OBD_DEFAULT_DATABASE "]]\n"
				"   [-v|--version] [-h|--help]\n", argv0);
}

void printversion() {
	printf("Version: %i.%i\n", OBDLOGGER_MAJOR_VERSION, OBDLOGGER_MINOR_VERSION);
}


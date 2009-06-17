/** \file
 \brief Generate data from an obdgps logfile
*/

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#include "sqlite3.h"

#include "obdservicecommands.h"
#include "datasource.h"

/// This is the void * generator
struct logger_gen {
	sqlite3 *db; //< The sqlite3 database
	struct timeval simstart;  // The time that the simulation began
	unsigned long supportedpids_00; // Supported pids according to 0100
	unsigned long supportedpids_20; // Supported pids according to 0120
	unsigned long supportedpids_40; // Supported pids according to 0140
	unsigned long supportedpids_60; // Supported pids according to 0160
};

int obdsim_generator_create(void **gen, void *seed) {
	const char *filename = (const char *)seed;
	if(NULL == filename || 0 == strlen(filename)) {
		fprintf(stderr, "Must send (const char *) filename as the seed\n");
		return 1;
	}

	sqlite3 *db;
	int rc;
	rc = sqlite3_open(filename, &db);
	if( SQLITE_OK != rc ) {
		fprintf(stderr, "Can't open database %s: %s\n", filename, sqlite3_errmsg(db));
		sqlite3_close(db);
		return 1;
	}

	struct logger_gen *g = (struct logger_gen *)malloc(sizeof(struct logger_gen));
	if(NULL == g) return 1;

	g->db = db;

	// Get the supported PIDs according to the database
	g->supportedpids_00 = 0x01; // We can support getting higher PIDs without supporting anything else
	g->supportedpids_20 = 0x01;
	g->supportedpids_40 = 0x01;
	g->supportedpids_60 = 0x00;

	sqlite3_stmt *pragma_stmt; // The stmt for gathering table_info
	const char *dbend; // ignored handle for sqlite
	rc = sqlite3_prepare_v2(g->db, "PRAGMA table_info(obd)", -1, &pragma_stmt, &dbend);
	if(SQLITE_OK != rc) {
		printf("Couldn't get table info in database: %s\n", sqlite3_errmsg(g->db));
		sqlite3_finalize(pragma_stmt);

		sqlite3_close(db);
		free(g);
		return 1;
	}

	while(SQLITE_DONE != sqlite3_step(pragma_stmt)) {
		const char *columnname = sqlite3_column_text(pragma_stmt, 1);

		struct obdservicecmd *cmd = obdGetCmdForColumn(columnname);

		if(NULL == cmd || 0 == strcmp(cmd->db_column, "time")) {
			printf("Couldn't find cmd for column %s\n", columnname);
			continue;
		}

		unsigned int pid = cmd->cmdid;

		if(pid <= 0x20) {
			g->supportedpids_00 |= ((unsigned long)1<<(0x20 - pid));
		} else if(pid > 0x20 && pid <= 0x40) {
			g->supportedpids_20 |= ((unsigned long)1<<(0x40 - pid));
		} else if(pid > 0x40 && pid <= 0x60) {
			g->supportedpids_40 |= ((unsigned long)1<<(0x60 - pid));
		} else if(pid > 0x60 &&  pid <= 0x80) {
			g->supportedpids_60 |= ((unsigned long)1<<(0x80 - pid));
		} else {
			fprintf(stderr,"Don't support PIDs this high in sim yet: %i\n", pid);
		}
	}

	sqlite3_finalize(pragma_stmt);
	// Got the supported PIDs


	if(0 != gettimeofday(&(g->simstart), NULL)) {
		fprintf(stderr, "Couldn't get time of day\n");
		sqlite3_close(db);
		free(g);
		return 1;
	}

	*gen = g;
	return 0;
}

void obdsim_generator_destroy(void *gen) {
	struct logger_gen *g = gen;
	sqlite3_close(g->db);
	free(gen);
}

int obdsim_generator_getvalue(void *gen, unsigned int PID, unsigned int *A, unsigned int *B, unsigned int *C, unsigned int *D) {
	struct logger_gen *g = gen;

	if(0x00 == PID || 0x20 == PID || 0x40 == PID || 0x60 == PID) {
			unsigned long bits;
			if(0x00 == PID) bits = g->supportedpids_00;
			else if(0x20 == PID) bits = g->supportedpids_20;
			else if(0x40 == PID) bits = g->supportedpids_40;
			else if(0x60 == PID) bits = g->supportedpids_60;
			else return 0;

			*D = bits & 0xFF;
			bits >>= 8;
			*C = bits & 0xFF;
			bits >>= 8;
			*B = bits & 0xFF;
			bits >>= 8;
			*A = bits & 0xFF;

			return 4;
	}

	struct obdservicecmd *cmd = obdGetCmdForPID(PID);
	if(NULL == cmd || 0 == strlen(cmd->db_column)) {
			fprintf(stderr, "Requested unsupported PID\n");
			return 0;
	}

	// Getting here means we need to look up a real value.
	struct timeval currtime;
	if(0 != gettimeofday(&currtime, NULL)) {
			fprintf(stderr, "Couldn't get time of day\n");
			return 0;
	}
	struct timeval dt;
	dt.tv_sec = currtime.tv_sec - g->simstart.tv_sec;
	dt.tv_usec = currtime.tv_usec - g->simstart.tv_usec;

	// Time we're aiming for in the SELECT, compared to when the database starts
	double seltime = (double)dt.tv_sec+(double)dt.tv_usec/1000000.0f;

	char sql[2048];

	// Taking our best guess means interpolating the value.
	snprintf(sql, sizeof(sql), "SELECT "
				"(s.%s + ((%f + (SELECT MIN(obd.time) FROM OBD) -s.time)/(e.time-s.time))*(e.%s - s.%s)) AS est%s "
				"FROM obd s, obd e "
				"WHERE s.time = (SELECT MAX(obd.time) FROM obd WHERE time < (%f + (SELECT MIN(obd.time) FROM obd))) "
				"AND     e.time = (SELECT MIN(obd.time) FROM obd WHERE time >= (%f + (SELECT MIN(obd.time) FROM obd)))",
				cmd->db_column, seltime, cmd->db_column, cmd->db_column, cmd->db_column, seltime, seltime);

	// printf("SQL Select:\n%s\n", sql);

	sqlite3_stmt *select_stmt; // Our actual select statement
	const char *dbend; // ignored handle for sqlite
	int rc = sqlite3_prepare_v2(g->db, sql, -1, &select_stmt, &dbend);
	if(SQLITE_OK != rc) {
		printf("Couldn't prepare select %s: %s\n", select, sqlite3_errmsg(g->db));
		sqlite3_finalize(select_stmt);
		return 0;
	}


	sqlite3_step(select_stmt); // We only step once - that's all we asked for.

	double val = sqlite3_column_double(select_stmt, 0);
	int retval = cmd->convrev(val, A, B, C, D);

	sqlite3_finalize(select_stmt);

	return retval;
}


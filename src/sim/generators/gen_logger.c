/** \file
 \brief Generate random data
*/

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#include "sqlite3.h"

#include "datasource.h"

/// This is the void * generator
struct logger_gen {
	sqlite3 *db; //< The sqlite3 database
	struct timeval simstart;  // The time that the simulation began
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
	if(0x00 == PID) {
		// We're capable of pulling *anything* out of our collective asses!
		*A = 0xFF;
		*B = 0xFF;
		*C = 0xFF;
		*D = 0xFE;
	} else {
		*A = (unsigned int) random();
		*B = (unsigned int) random();
		*C = (unsigned int) random();
		*D = (unsigned int) random();
	}
	return 4;
}


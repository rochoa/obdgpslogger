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
 \brief Generate random data
*/

#include <stdlib.h>
#include <stdio.h>

#include "datasource.h"

const char *random_simgen_name() {
	return "Random";
}

int random_simgen_create(void **gen, const char *seed) {
	if(NULL != seed && '\0' != *seed) {
		int s = atoi(seed);
		printf("Seeding RNG with %i\n", s);
		srandom(s);
	}
	return 0;
}

void random_simgen_destroy(void *gen) {
}

int random_simgen_getvalue(void *gen, unsigned int mode, unsigned int PID, unsigned int *A, unsigned int *B, unsigned int *C, unsigned int *D) {
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

int random_simgen_idle(void *gen, int idlems) {
	return 0;
}

// Declare our obdsim_generator. This is pulled in as an extern in obdsim.c
struct obdsim_generator obdsimgen_random = {
	random_simgen_name,
	random_simgen_create,
	random_simgen_destroy,
	random_simgen_getvalue,
	random_simgen_idle
};


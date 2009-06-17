/** \file
 \brief Generate random data
*/

#include <stdlib.h>
#include <stdio.h>

#include "datasource.h"

int obdsim_generator_create(void **gen, void *seed) {
	return 0;
}

void obdsim_generator_destroy(void *gen) {
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


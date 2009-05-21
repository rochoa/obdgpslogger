/** \file
 \brief Guess likely serial device names
 */

#include <stdio.h>
#include <string.h>
#include <glob.h>
#include <FL/Fl_Input_Choice.H>

#include "guessdevs.h"

int populateSerialDevs(Fl_Input_Choice *addto, const char *def) {
	glob_t names;

	addto->clear();
	if(NULL != def && 0 < strlen(def)) {
		addto->add(def);
		addto->value(0);
	}

	// Just take a bunch of semi-educated guesses:

	// This is the OSX standard way of doing things
	glob("/dev/cu.*", 0, NULL, &names);
	// Linux bluetooth
	glob("/dev/rfcomm*", GLOB_APPEND, NULL, &names);
	// Linux USB
	glob("/dev/ttyUSB*", GLOB_APPEND, NULL, &names);
	// Good ol' fashioned serial port
	glob("/dev/ttyS*", GLOB_APPEND, NULL, &names);

	size_t i;
	for(i=0; i<names.gl_pathc; i++) {
		addto->add(names.gl_pathv[i]);
	}
	
	globfree(&names);

	return 0;
}


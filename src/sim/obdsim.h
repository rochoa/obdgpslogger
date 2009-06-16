/** \file
 \brief OBD Simulator Main Entrypoint
*/
#ifndef __OBDSIM_H
#define __OBDSIM_H

#include <getopt.h>
#include <stdlib.h>

/// This is the elm prompt
#define ELM_PROMPT ">"

/// Default hide headers
#define ELM_HEADERS 0

/// Default show spaces
#define ELM_SPACES 1

/// Default echo
#define ELM_ECHO 1


/// getopt() long options
static const struct option longopts[] = {
        { "help", no_argument, NULL, 'h' }, ///< Print the help text
	{ "version", no_argument, NULL, 'v' }, ///< Print the version text
        { "db", required_argument, NULL, 'd' }, ///< Database file
        { NULL, 0, NULL, 0 } ///< End
};

/// getopt() short options
static const char shortopts[] = "hvd:";

/// Print Help for --help
/** \param argv0 your program's argv[0]
 */
void printhelp(const char *argv0);

/// Print the version string
void printversion();


#endif //__OBDSIM_H



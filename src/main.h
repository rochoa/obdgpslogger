/** \file
 \brief main obdlogger entrypoint
 */
#ifndef __MAIN_H
#define __MAIN_H

#include <getopt.h>
#include <stdlib.h>

/// getopt() long options
static const struct option longopts[] = {
	{ "help", no_argument, NULL, 'h' }, ///< Print the help text
	{ "version", no_argument, NULL, 'v' }, ///< Print the version text
	{ "serial", required_argument, NULL, 's' }, ///< Serial Port
	{ "samplerate", required_argument, NULL, 'a' }, ///< Number of samples per second
	{ "count", required_argument, NULL, 'c' }, ///< Number of values to grab
	{ "db", required_argument, NULL, 'd' }, ///< Database file
	{ NULL, 0, NULL, 0 } ///< End
};

/// getopt() short options
static const char shortopts[] = "hvs:d:c:a:";

/// Print Help for --help
/** \param argv0 your program's argv[0]
 */
void printhelp(const char *argv0);

/// Print the version string
void printversion();


#endif // __MAIN_H


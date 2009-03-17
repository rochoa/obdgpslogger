/** \file
 \brief main obdlogger entrypoint
 */
#ifndef __MAIN_H
#define __MAIN_H

#include <getopt.h>

/// Major version
#define OBDLOGGER_MAJOR_VERSION 0
/// Minor version
#define OBDLOGGER_MINOR_VERSION 1

/// Default serial port. This is the one for my bluetooth obdkey on my mac
#define DEFAULT_SERIAL_PORT "/dev/cu.OBDKeyPro-DevB-1"

/// Default database to open
#define DEFAULT_DATABASE "./obdlogger.db"

/// getopt() long options
static const struct option longopts[] = {
	{ "help", no_argument, NULL, 'h' }, ///< Print the help text
	{ "version", no_argument, NULL, 'v' }, ///< Print the version text
	{ "serial", required_argument, NULL, 's' }, ///< Serial Port
	{ "count", required_argument, NULL, 'c' }, ///< Number of values to grab
	{ "db", required_argument, NULL, 'd' }, ///< Database file
	{ NULL, 0, NULL, 0 } ///< End
};

/// getopt() short options
static const char shortopts[] = "hvs:d:c:";

/// Print Help for --help
/** \param argv0 your program's argv[0]
 */
void printhelp(const char *argv0);

/// Print the version string
void printversion();


#endif // __MAIN_H


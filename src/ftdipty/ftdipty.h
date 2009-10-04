#ifndef __FTDIPTY_H
#define __FTDIPTY_H

#include <getopt.h>

/// getopt() long options
static const struct option longopts[] = {
	{ "help", no_argument, NULL, 'h' }, ///< Print the help text
	{ "version", no_argument, NULL, 'v' }, ///< Print the version text
	{ "daemonise", no_argument, NULL, 'd' }, ///< Daemonise
	{ "baud", required_argument, NULL, 'b' }, ///< Set the baudrate
	{ "modifyconf", no_argument, NULL, 'c' }, ///< Modify the config file
};

/// getopt() short options
static const char shortopts[] = "hvb:cd";


/// Print Help for --help
/** \param argv0 your program's argv[0]
 */
void printhelp(const char *argv0);

/// Print the version string
void printversion();

#endif // __FTDIPTY_H



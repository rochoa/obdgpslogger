/* \file
 \brief OBD GPS KML main entrypoint
 */
#ifndef __OBDGPSKML_H
#define __OBDGPSKML_H

#include <getopt.h>

/// Default out filename
#define DEFAULT_OUTFILENAME "./obdlogger.kml"

/// Default max altitude
#define DEFAULT_MAXALTITUDE 1000

/// getopt_long long options
static const struct option kmllongopts[] = {
	{ "help", no_argument, NULL, 'h' }, ///< Print the help text
	{ "version", no_argument, NULL, 'v' }, ///< Print the version text
	{ "db", required_argument, NULL, 'd' }, ///< Database file
	{ "out", required_argument, NULL, 'o' }, ///< Output file
	{ "altitude", required_argument, NULL, 'a' }, ///< Max altitude
	{ NULL, 0, NULL, 0 } ///< End
};


/// getopt() short options
static const char kmlshortopts[] = "hvd:o:a:";

/// Print Help for --help
/** \param argv0 your program's argv[0]
 */
void kmlprinthelp(const char *argv0);

/// Print the version string
void kmlprintversion();


#endif //__OBDGPSKML_H


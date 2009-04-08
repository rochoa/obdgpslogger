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
 \brief OBD GPS CSV main entrypoint
 */
#ifndef __OBDGPSCSV_H
#define __OBDGPSCSV_H

#include <getopt.h>

/// Default out filename
#define DEFAULT_OUTFILENAME "./obdlogger.csv"

/// getopt_long long options
static const struct option csvlongopts[] = {
	{ "help", no_argument, NULL, 'h' }, ///< Print the help text
	{ "version", no_argument, NULL, 'v' }, ///< Print the version text
	{ "progress", no_argument, NULL, 'p' }, ///< Print parsable progress
	{ "db", required_argument, NULL, 'd' }, ///< Database file
	{ "out", required_argument, NULL, 'o' }, ///< Output file
	{ NULL, 0, NULL, 0 } ///< End
};


/// getopt() short options
static const char csvshortopts[] = "hvpd:o:";

/// Print Help for --help
/** \param argv0 your program's argv[0]
 */
void csvprinthelp(const char *argv0);

/// Print the version string
void csvprintversion();


#endif //__OBDGPSKML_H


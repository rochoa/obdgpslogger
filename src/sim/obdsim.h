/* Copyright 2009 Gary Briggs, Michael Carpenter

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
 \brief OBD Simulator Main Entrypoint
*/
#ifndef __OBDSIM_H
#define __OBDSIM_H

#include <getopt.h>
#include <stdlib.h>

/// This is the elm prompt
#define ELM_PROMPT "\n>"

/// Default hide headers
#define ELM_HEADERS 0

/// Default show spaces
#define ELM_SPACES 1

/// Default echo
#define ELM_ECHO 1

/// ELM Version string
#define ELM_VERSION_STRING "ELM327 v1.3 OBDGPSLogger"

/// ELM "don't know" prompt
#define ELM_QUERY_PROMPT "\n?" ELM_PROMPT

/// ELM "OK" prompt
#define ELM_OK_PROMPT "\nOK" ELM_PROMPT

/// ELM "NO DATA" prompt
#define ELM_NODATA_PROMPT "\nNO DATA" ELM_PROMPT


/// getopt() long options
static const struct option longopts[] = {
        { "help", no_argument, NULL, 'h' }, ///< Print the help text
	{ "version", no_argument, NULL, 'v' }, ///< Print the version text
        { "seed", required_argument, NULL, 's' }, ///< Seed
	{ "generator", required_argument, NULL, 'g' }, ///< Choose a generator
        { "launch-logger", no_argument, NULL, 'o' }, ///< Launch obdgpslogger
        { "launch-screen", no_argument, NULL, 'c' }, ///< Launch screen
        { NULL, 0, NULL, 0 } ///< End
};

/// getopt() short options
static const char shortopts[] = "hvs:g:oc";

/// Print Help for --help
/** \param argv0 your program's argv[0]
 */
void printhelp(const char *argv0);

/// Print the version string
void printversion();


#endif //__OBDSIM_H



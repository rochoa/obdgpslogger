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
#define ELM_PROMPT ">"

/// Default hide headers
#define ELM_HEADERS 0

/// Default show spaces
#define ELM_SPACES 1

/// Default echo
#define ELM_ECHO 1

/// Default linefeed
#define ELM_LINEFEED 1

/// ELM Device Description
#define ELM_DEVICE_STRING "OBDGPSLogger"

/// ELM Version string
#define ELM_VERSION_STRING "ELM327 v1.3a OBDGPSLogger"

/// Protocol version
#define ELM_PROTOCOL_DESCRIPTION "AUTO ISO 15765-4 (CAN 11/250)"

/// Protocol number
#define ELM_PROTOCOL_NUMBER "A8"

/// ELM "don't know" prompt
#define ELM_QUERY_PROMPT "?"

/// ELM "OK" prompt
#define ELM_OK_PROMPT "OK"

/// ELM "NO DATA" prompt
#define ELM_NODATA_PROMPT "NO DATA"


/// getopt() long options
static const struct option longopts[] = {
	{ "help", no_argument, NULL, 'h' }, ///< Print the help text
	{ "genhelp", required_argument, NULL, 'e' }, ///< Print the help for a generator
	{ "list-generators", no_argument, NULL, 'l' }, ///< Print a list of generators
	{ "version", no_argument, NULL, 'v' }, ///< Print the version text
	{ "seed", required_argument, NULL, 's' }, ///< Seed
	{ "generator", required_argument, NULL, 'g' }, ///< Choose a generator
	{ "logfile", required_argument, NULL, 'q' }, ///< Write to this logfile
	{ "elm-version", required_argument, NULL, 'V' }, ///< Pretend to be this on ATZ
	{ "elm-device", required_argument, NULL, 'D' }, ///< Pretend to be this on AT@1
#ifdef OBDPLATFORM_POSIX
	{ "launch-logger", no_argument, NULL, 'o' }, ///< Launch obdgpslogger
	{ "launch-screen", no_argument, NULL, 'c' }, ///< Launch screen
	{ "tty-device", required_argument, NULL, 't' }, ///< Open this actual device instead of a pty
#endif //OBDPLATFORM_POSIX
#ifdef OBDPLATFORM_WINDOWS
	{ "com-port", required_argument, NULL, 'w' }, ///< Windows com port to open
#endif //OBDPLATFORM_WINDOWS
	{ NULL, 0, NULL, 0 } ///< End
};

/// getopt() short options
static const char shortopts[] = "hle:vs:g:q:V:D:"
#ifdef OBDPLATFORM_POSIX
	"oct:"
#endif //OBDPLATFORM_POSIX
#ifdef OBDPLATFORM_WINDOWS
	"w:"
#endif //OBDPLATFORM_WINDOWS
;

/// Print Help for --help
/** \param argv0 your program's argv[0]
 */
void printhelp(const char *argv0);

/// Print the version string
void printversion();


#endif //__OBDSIM_H



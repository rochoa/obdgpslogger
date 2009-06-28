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
  \brief Tools to open the sim port
*/

#ifndef __SIMPORT_H
#define __SIMPORT_H

/// Open the sim port.
/** \return an opaque type you pass to other simport_ functions
  */
void *simport_open();

/// Close the sim port opened with simport_open
int simport_close(void *simport);

/// Get the device that's open on the other end of the passed fd
/** You must take a copy if you want to use this - the memory will be
      overwritten next call */
char *simport_getptyslave(void *simport);

/// Get a line from the passed simport
/** This is local static memory. keep a copy if you want one, otherwise
      don't expect a second call to this to not screw up your previous
      call. */
char *simport_readline(void *simport);

/// Write a line to the simport
void simport_writeline(void *simport, const char *line);

#endif //__SIMPORT_H


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
 \brief Data generators need to implement the functions defined herein
*/

#ifndef __DATASOURCE_H
#define __DATASOURCE_H

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/// Declare a generator by building one of these structs
struct obdsim_generator {
	/// Get a human-friendly name for this generator
	const char *(*name)();

	/// Initialise the data generator
	/** \return 0 for success, 1 for failure
    	  \param gen opaque data generator
    	  \param seed intialisation seed. implementation specific
	*/
	int (*create)(void **gen, const char *seed);

	/// Shut down the data generator
	void (*destroy)(void *gen);

	/// Get a value for the specified PID
	/** \return number of values created, or -1 for "must exit"
    	  \param PID the PID this is for
    	  \param gen opaque data generator
    	  \param A,B,C,D four values to fill
	*/
	int (*getvalue)(void *gen, unsigned int PID,
		unsigned int *A, unsigned int *B, unsigned int *C, unsigned int *D);

	/// Called whenever the simulator is idle
	/** \param idlems Take no longer than this many milliseconds
	 \return anything other than zero is considered a condition which means we must exit
	 */
	int (*idle)(void *gen, int idlems);
};

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //__DATASOURCE_H


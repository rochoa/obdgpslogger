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
 \brief Dlopen()'d data generators need to implement the functions defined herein
*/

/*
 In the intention of being able to work with closed-source plugins,
 this file is explicitly NOT under the GPL. I [Gary "ChunkyKs" Briggs]
 disclaim copyright on this source code, and in the spirit of SQLite
 instead place a blessing here:
 
     May you do good and not evil.
     May you find forgiveness for yourself and forgive others.
     May you share freely, never taking more than you give.
 */

#ifndef __SIMDL_DATASOURCE_H
#define __SIMDL_DATASOURCE_H

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/// Get a human-friendly name for this generator
const char *simdl_name();

/// Initialise the data generator
/** \return 0 for success, 1 for failure
   	  \param gen opaque data generator
   	  \param seed intialisation seed. implementation specific
*/
int simdl_create(void **gen, const char *seed);

/// Shut down the data generator
void simdl_destroy(void *gen);

/// Get a value for the specified PID
/** \return number of values created
   	  \param PID the PID this is for
   	  \param gen opaque data generator
   	  \param A,B,C,D four values to fill
*/
int simdl_getvalue(void *gen, unsigned int PID,
		unsigned int *A, unsigned int *B, unsigned int *C, unsigned int *D);

/// Called whenever the sim is idle. Do not block more than timems milliseconds
/** \return anything other than zero is considered a condition which means we must exit
 */
int simdl_idle(void *gen, int timems);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //__SIMDL_DATASOURCE_H


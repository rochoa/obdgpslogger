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

#endif //__SIMDL_DATASOURCE_H


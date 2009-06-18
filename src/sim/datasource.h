/** \file
 \brief Data generators need to implement the functions defined herein
*/

#ifndef __DATASOURCE_H
#define __DATASOURCE_H

/// Get a human-friendly name for this generator
const char *obdsim_generator_name();

/// Initialise the data generator
/** \return 0 for success, 1 for failure
    \param gen opaque data generator
    \param seed intialisation seed. implementation specific
*/
int obdsim_generator_create(void **gen, void *seed);

/// Shut down the data generator
void obdsim_generator_destroy(void *gen);

/// Get a value for the specified PID
/** \return number of values created
    \param PID the PID this is for
    \param gen opaque data generator
    \param A,B,C,D four values to fill
*/
int obdsim_generator_getvalue(void *gen, unsigned int PID, unsigned int *A, unsigned int *B, unsigned int *C, unsigned int *D);

#endif //__DATASOURCE_H


/** \file
 \brief Data generators need to implement the functions defined herein
*/

#ifndef __DATASOURCE_H
#define __DATASOURCE_H

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/// Declate a generator by building one of these structs
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
	/** \return number of values created
    	  \param PID the PID this is for
    	  \param gen opaque data generator
    	  \param A,B,C,D four values to fill
	*/
	int (*getvalue)(void *gen, unsigned int PID,
		unsigned int *A, unsigned int *B, unsigned int *C, unsigned int *D);

};

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //__DATASOURCE_H


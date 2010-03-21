#ifndef __DTCCODES_H
#define __DTCCODES_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/// Check whether this string is a valid error code
/** \return 1 if this is a valid error code, 0 otherwise */
int dtc_isvalid(const char *test);

/// Convert a human-friendly DTC to the byte representation
/** \param human the string code
	\param A,B put converted data into these
	\return 0 on success, -1 on error
*/
int dtc_humantobytes(const char *human, unsigned int *A, unsigned int *B);

#ifdef __cplusplus
}
#endif // __cplusplus


#endif //__DTCCODES_H


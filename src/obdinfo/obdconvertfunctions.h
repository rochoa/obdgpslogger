/** \file
  \brief Functions to convert from OBD output to actual values
  Functions declared in this file are named by their OBD Service command
   which is two hex digits. It's easier than coming up with short unique
   names myself. Please refer to obdservicecommands.h to see what's what.
*/

#ifndef __OBDCONVERTFUNCTIONS_H
#define __OBDCONVERTFUNCTIONS_H

#ifdef __cplusplus
extern "C" {
#endif //  __cplusplus

// http://en.wikipedia.org/wiki/Table_of_OBD-II_Codes

/// All obd conversion functions adhere to this
typedef float (*OBDConvFunc)(unsigned int A, unsigned int B,
	unsigned int C, unsigned int D);

float obdConvert_04    (unsigned int A, unsigned int B, unsigned int C, unsigned int D);
float obdConvert_05    (unsigned int A, unsigned int B, unsigned int C, unsigned int D);
float obdConvert_06_09 (unsigned int A, unsigned int B, unsigned int C, unsigned int D);
float obdConvert_0A    (unsigned int A, unsigned int B, unsigned int C, unsigned int D);
float obdConvert_0B    (unsigned int A, unsigned int B, unsigned int C, unsigned int D);
float obdConvert_0C    (unsigned int A, unsigned int B, unsigned int C, unsigned int D);
float obdConvert_0D    (unsigned int A, unsigned int B, unsigned int C, unsigned int D);
float obdConvert_0E    (unsigned int A, unsigned int B, unsigned int C, unsigned int D);
float obdConvert_0F    (unsigned int A, unsigned int B, unsigned int C, unsigned int D);
float obdConvert_10    (unsigned int A, unsigned int B, unsigned int C, unsigned int D);
float obdConvert_11    (unsigned int A, unsigned int B, unsigned int C, unsigned int D);
float obdConvert_14_1B (unsigned int A, unsigned int B, unsigned int C, unsigned int D);
float obdConvert_1F    (unsigned int A, unsigned int B, unsigned int C, unsigned int D);
float obdConvert_21    (unsigned int A, unsigned int B, unsigned int C, unsigned int D);
float obdConvert_22    (unsigned int A, unsigned int B, unsigned int C, unsigned int D);
float obdConvert_23    (unsigned int A, unsigned int B, unsigned int C, unsigned int D);
float obdConvert_24_2B (unsigned int A, unsigned int B, unsigned int C, unsigned int D);
float obdConvert_2C    (unsigned int A, unsigned int B, unsigned int C, unsigned int D);
float obdConvert_2D    (unsigned int A, unsigned int B, unsigned int C, unsigned int D);
float obdConvert_2E    (unsigned int A, unsigned int B, unsigned int C, unsigned int D);
float obdConvert_2F    (unsigned int A, unsigned int B, unsigned int C, unsigned int D);
float obdConvert_30    (unsigned int A, unsigned int B, unsigned int C, unsigned int D);
float obdConvert_31    (unsigned int A, unsigned int B, unsigned int C, unsigned int D);
float obdConvert_32    (unsigned int A, unsigned int B, unsigned int C, unsigned int D);
float obdConvert_33    (unsigned int A, unsigned int B, unsigned int C, unsigned int D);
float obdConvert_34_3B (unsigned int A, unsigned int B, unsigned int C, unsigned int D);
float obdConvert_3C_3F (unsigned int A, unsigned int B, unsigned int C, unsigned int D);
float obdConvert_42    (unsigned int A, unsigned int B, unsigned int C, unsigned int D);
float obdConvert_43    (unsigned int A, unsigned int B, unsigned int C, unsigned int D);
float obdConvert_44    (unsigned int A, unsigned int B, unsigned int C, unsigned int D);
float obdConvert_45    (unsigned int A, unsigned int B, unsigned int C, unsigned int D);
float obdConvert_46    (unsigned int A, unsigned int B, unsigned int C, unsigned int D);
float obdConvert_47_4B (unsigned int A, unsigned int B, unsigned int C, unsigned int D);
float obdConvert_4C    (unsigned int A, unsigned int B, unsigned int C, unsigned int D);
float obdConvert_4D    (unsigned int A, unsigned int B, unsigned int C, unsigned int D);
float obdConvert_4E    (unsigned int A, unsigned int B, unsigned int C, unsigned int D);
float obdConvert_52    (unsigned int A, unsigned int B, unsigned int C, unsigned int D);


#ifdef __cplusplus
}
#endif //  __cplusplus


#endif // __OBDCONVERTFUNCTIONS_H


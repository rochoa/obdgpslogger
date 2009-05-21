/** \file
 \brief Guess likely serial device names
 */
#ifndef __GUESSDEVS_H
#define __GUESSDEVS_H

#include <FL/Fl_Input_Choice.H>

/// Take some guesses at likely serial devices and populate the inputchoice
/** \param addto the input_choice to add all the likely candidates to
  \param def if provided, is added and made the default choice */
int populateSerialDevs(Fl_Input_Choice *addto, const char *def = NULL);

#endif //__GUESSDEVS_H


/** \file
 \brief Dump a single value in a db as height in a KML document
 */

#ifndef __SINGLEHEIGHT_H
#define __SINGLEHEIGHT_H

#include <stdlib.h>
#include <stdio.h>

#include "sqlite3.h"


/// print single db column as height in kml, normalised to maximum height
/** 
 \param db the sqlite3 database the data is in
 \param f the file to output the data to
 \param columnname the columnname to dump it as
 \param height the max height to normalise everything to
 */
void kmlvalueheight(sqlite3 *db, FILE *f, const char *columnname, int height);


#endif //__SINGLEHEIGHT_H



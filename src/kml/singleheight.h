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
 \param name the name of the document to output this as
 \param desc the description of the document to output this as
 \param columnname the columnname to dump it as
 \param height the max height to normalise everything to
 \param defaultvis the default visilibity [1 for on, 0 for off]
 */
void kmlvalueheight(sqlite3 *db, FILE *f, const char *name, const char *desc, const char *columnname, int height, int defaultvis);


#endif //__SINGLEHEIGHT_H



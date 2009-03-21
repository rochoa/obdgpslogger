/** \file
 \brief Dump a single value in a db as height in a KML document
 */

#ifndef __HEIGHTANDCOLOR_H
#define __HEIGHTANDCOLOR_H

#include <stdlib.h>
#include <stdio.h>

#include "sqlite3.h"


/// print db column as height in kml, normalised to maximum height
/**  Additionally, color it based on column col with style prefix stylepref
 \param db the sqlite3 database the data is in
 \param f the file to output the data to
 \param name the name of the document to output this as
 \param desc the description of the document to output this as
 \param columnname the columnname to dump it as
 \param height the max height to normalise everything to
 \param col the column to use for coloring
 \param numcols the number of colors we have, 0..numcols
 */
void kmlvalueheightcolor(sqlite3 *db, FILE *f, const char *name, const char *desc, const char *columnname, int height, const char *col, int numcols);


#endif //__HEIGHTANDCOLOR_H



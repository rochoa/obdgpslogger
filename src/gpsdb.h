/** \file
 \brief gps database logging stuff
 */

#ifdef HAVE_GPSD

#ifndef __GPSDB_H
#define __GPSDB_H

#include "sqlite3.h"

/// Create the gps table in the database
int creategpstable(sqlite3 *db);

/// Prepare the sqlite3 insert statement for the gps table
/**
 \param db the database handle this is for
 \param ret_stmt the prepared statement is placed in this value
 \return number of columns in the insert statement, or zero on fail
 */
int creategpsinsertstmt(sqlite3 *db, sqlite3_stmt **ret_stmt);


#endif //__GPSDB_H

#endif //HAVE_GPSD


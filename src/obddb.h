/** \file
 \brief obd database stuff
 */

#ifndef __OBDDB_H
#define __OBDDB_H

#include "sqlite3.h"

/// Create the obd table in the database
int createobdtable(sqlite3 *db);

/// Prepare the sqlite3 insert statement for the obd table
/**
 \param db the database handle this is for
 \param ret_stmt the prepared statement is placed in this value
 \return number of columns in the insert statement, or zero on fail
 */
int createobdinsertstmt(sqlite3 *db, sqlite3_stmt **ret_stmt);



#endif // __OBDDB_H


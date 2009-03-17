/** \file
   \brief Database open and close functions
*/
#ifndef __DATABASE_H
#define __DATABASE_H

#include "sqlite3.h"

/// Open the sqlite database
/** This will create the table "odb" if it does not exist.
 \param dbfilename filename of the database
 \return an sqlite3 handle if successful, NULL otherwise
*/
sqlite3 *opendb(const char *dbfilename);

/// Close the sqlite database
void closedb(sqlite3 *db);


#endif //__DATABASE_H


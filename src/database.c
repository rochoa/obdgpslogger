/** \file
 \brief database stuff
 */

#include "database.h"

#include "sqlite3.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


sqlite3 *opendb(const char *dbfilename) {
	// sqlite database
	sqlite3 *db;
	// sqlite return status
	int rc;

	rc = sqlite3_open(dbfilename, &db);
	if( SQLITE_OK != rc ) {
		fprintf(stderr, "Can't open database %s: %s\n", dbfilename, sqlite3_errmsg(db));
		sqlite3_close(db);
		return NULL;
	}

	return db;
}

void closedb(sqlite3 *db) {
	sqlite3_close(db);
}



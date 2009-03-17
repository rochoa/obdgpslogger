/** \file
 \brief obd database stuff
 */

#include "obddb.h"
#include "obdservicecommands.h"

#include "sqlite3.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>



/// Create the obd table in the database
int createobdtable(sqlite3 *db) {
		// TODO calculate buffer size and create correct sized one,
		//   otherwise this could overflow if obdservicecommands contains a *lot* of non-NULL fields
	int i;

	char create_stmt[4096] = "CREATE TABLE obd (";
	for(i=0; i<sizeof(obdcmds)/sizeof(obdcmds[0]); i++) {
		if(NULL != obdcmds[i].db_column) {
			strcat(create_stmt,obdcmds[i].db_column);
			strcat(create_stmt," INTEGER,");
		}
	}
	strcat(create_stmt,"time INTEGER)");

	// printf("Create_stmt:\n  %s\n", create_stmt);

	/// sqlite3 return status
	int rc;
	/// sqlite3 error message
	char *errmsg;

	if(SQLITE_OK != sqlite3_exec(db, create_stmt, NULL, NULL, &errmsg)) {
		printf("sqlite error on statement %s: %s\n", create_stmt, errmsg);
		sqlite3_free(errmsg);
		return 1;
	}
	return 0;
}
 
int createobdinsertstmt(sqlite3 *db,sqlite3_stmt **ret_stmt) {
		// TODO calculate buffer size and create correct sized one,
		//   otherwise this could overflow if obdservicecommands contains a *lot* of non-NULL fields
	int i;

	int columncount = 0;
	char insert_sql[4096] = "INSERT INTO obd (";
	for(i=0; i<sizeof(obdcmds)/sizeof(obdcmds[0]); i++) {
		if(NULL != obdcmds[i].db_column) {
			strcat(insert_sql,obdcmds[i].db_column);
			strcat(insert_sql,",");
			columncount++;
		}
	}
	strcat(insert_sql,"time) VALUES (");
	for(i=0; i<columncount; i++) {
		strcat(insert_sql,"?,");
	}
	strcat(insert_sql,"?)");

	columncount++; // for time
	// printf("insert_sql:\n  %s\n", insert_sql);

	int rc;
	const char *zTail;
	rc = sqlite3_prepare(db,insert_sql,-1,ret_stmt,&zTail);
	if(SQLITE_OK != rc) {
		fprintf(stderr, "Can't prepare statement %s: %s\n", insert_sql, sqlite3_errmsg(db));
		return 0;
	}

	return columncount;
}




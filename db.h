#ifndef __DB_H_
#define __DB_H_


#include <stdio.h>
#include <stdlib.h>
#include <libpq-fe.h>
#include "msgs.h"

#define MAX_STRING 1024

#define htonll(x) ((((uint64_t)htonl(x)) << 32) + htonl((x) >> 32))

typedef enum nfd_db_type_s {
	NFD_DB_PGSQL
} nfd_db_type_t;


typedef struct nfd_db_s {
	nfd_db_type_t db_type;
	PGconn *conn;
	//PGresult *ins_res;
	char ins_name[MAX_STRING];
	char upd_name[MAX_STRING];
	int inserted;
	int updated;
} nfd_db_t;

int nfd_db_begin_transaction(nfd_db_t *db);
int nfd_db_end_transaction(nfd_db_t *db);

#endif //__DB_H_


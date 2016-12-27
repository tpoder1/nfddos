#ifndef __DB_H_
#define __DB_H_


#include <stdio.h>
#include <stdlib.h>
#include <libpq-fe.h>
#include "msgs.h"


typedef enum nfd_db_type_s {
	NFD_DB_PGSQL
} nfd_db_type_t;


typedef struct nfd_db_s {
	PGconn *conn;
	nfd_db_type_t db_type;
} nfd_db_t;



#endif //__DB_H_


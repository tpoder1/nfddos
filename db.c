
#include <string.h>
#include "db.h"
#include "nfddos.h"

int nfd_db_init(nfd_db_t *db, nfd_db_type_t db_type, const char *connstr) {

	//Oid oidTypes[6] = {VARCHAROID, VARCHAROID, INT8OID, INT8OID, INT8OID, INT8OID};
	//Oid oidTypes[5] = {1043, 1043, 20, 20, 20};
	PGresult *res;

	if (db_type != NFD_DB_PGSQL) {
		msg(MSG_ERROR, "Unsupported database type.");
	}

	db->db_type = db_type;
	db->conn = PQconnectdb(connstr);
	if (PQstatus(db->conn) == CONNECTION_BAD) {
		msg(MSG_ERROR, "Can not connect to the database: %s", PQerrorMessage(db->conn));

		return 0;
	}

	/* get server vesion */
	msg(MSG_INFO, "Connected to database, server version: %d", PQserverVersion(db->conn));

	/* prepare update statement */
	strncpy(db->upd_name, "update_counters", MAX_STRING);
	res = PQprepare(db->conn, db->upd_name, 
            "UPDATE nfd_counters SET "
			"bytes = bytes + $3::int8, pkts = pkts + $4::int8, flows = flows + $5::int8, " 
			"bytes_ps = $6::int8, pkts_ps = $7::int8, flows_ps = $8::int8, updated = NOW() " 
			"WHERE profile_id = $1 and aggr_key = $2",
            5,
        //    (const Oid *) oidTypes
			NULL
            );

	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		msg(MSG_ERROR, "Can not prepare update query: %s", PQerrorMessage(db->conn));
        PQclear(res);
        return 0;
    }
    

	/* prepare insert statement */
	strncpy(db->ins_name, "insert_counters", MAX_STRING);
	res = PQprepare(db->conn, db->ins_name, 
            "INSERT INTO nfd_counters (profile_id, aggr_key, updated, bytes, pkts, flows, bytes_ps, pkts_ps, flows_ps) "
			"VALUES ($1, $2, NOW(), $3::int8, $4::int8, $5::int8, $6::int8, $7::int8, $8::int8)",
            5,
        //    (const Oid *) oidTypes
			NULL
            );

	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		msg(MSG_ERROR, "Can not prepare insert query: %s", PQerrorMessage(db->conn));
        PQclear(res);
        return 0;
    }
    
	return 1;
	
}

/* create new profile  */
int nfd_db_mk_profile(nfd_profile_t **nfd_profile, char *id, char *filter, char *fields, char *errbuf) {

	nfd_profile_t *tmp;
	char *token = fields;
	int field, numbits, numbits6, fieldcnt;

	
	tmp = calloc(1, sizeof(nfd_profile_t));

	if (tmp == NULL) {
		snprintf(errbuf, MAX_STRING, "Can not allocate memory in %s:%d", __FILE__, __LINE__);
		return 0;
	}

	if (id != NULL && strnlen(id, MAX_STRING) > 0) {
		strncpy(tmp->id, id, MAX_STRING);
	} else {
		snprintf(errbuf, MAX_STRING, "Missing profile id %s:%d", __FILE__, __LINE__);
		return 0;
	}

	/* parse aggregation fields */
	if (fields != NULL && strnlen(fields, MAX_STRING) > 0) {

		if (lnf_mem_init(&tmp->mem) != LNF_OK) {
			free(tmp);
			snprintf(errbuf, MAX_STRING, "Can not initialize lnf_mem in %s:%d", __FILE__, __LINE__);
			return 0;
		}

//		strncpy(tmp->fields, fields, MAX_STRING);
		fieldcnt = 0;		

		while ( (token = strsep(&fields, ",")) != NULL ) {

			field = lnf_fld_parse(token, &numbits, &numbits6);
	
			if (field == LNF_FLD_ZERO_) {
				snprintf(errbuf, MAX_STRING, "Can not parse field '%s' from %s", token, fields);
				free(tmp);
				return 0;
			}

			if (lnf_fld_type(field) == LNF_ADDR && (numbits > 32 || numbits6 > 128)) {
				snprintf(errbuf, MAX_STRING, "Invalid bit size (%d/%d) for %s",
						numbits, numbits6, token);
				return 0;
			}

			msg(MSG_DEBUG, "Set field id %d for profile id %s", field, tmp->id);
			lnf_mem_fadd(tmp->mem, field, LNF_AGGR_KEY, numbits, numbits6);

			if (fieldcnt < MAX_AGGR_FIELDS - 1) {
				tmp->fields[fieldcnt++] = field;
			}

		}

		lnf_mem_fadd(tmp->mem, LNF_FLD_DPKTS, LNF_AGGR_SUM, 0, 0);
		lnf_mem_fadd(tmp->mem, LNF_FLD_DOCTETS, LNF_AGGR_SUM, 0, 0);
		lnf_mem_fadd(tmp->mem, LNF_FLD_AGGR_FLOWS, LNF_AGGR_SUM, 0, 0);
	}


	if (filter != NULL && strnlen(filter, MAX_STRING) > 0) {
		if (lnf_filter_init_v2(&tmp->filter, filter) != LNF_OK) {
			snprintf(errbuf, MAX_STRING, "Can not initlaise filter  (%s)", filter);
			free(tmp);
			return 0;
		}
	}

	if (!histc_init(&tmp->hcounter, HISTC_SLOTS, HISTC_SIZE)) {
		free(tmp);
		snprintf(errbuf, MAX_STRING, "Can not initialize hcounter in %s:%d", __FILE__, __LINE__);
		return 0;
	}

	*nfd_profile = tmp;

	return 1;
}

int nfd_db_load_profiles(nfd_db_t *db, nfd_profile_t **root_profile) {

	PGresult *res;
	int i;
	char *id, *filter, *fields;
	nfd_profile_t *tmp;
	char errbuf[MAX_STRING];

	res = PQexec(db->conn, "SELECT nfd_profile_id, nfd_filter_expr, nfd_aggr_expr FROM nfd_profiles");  


	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        msg(MSG_ERROR, "Can not load profiles: %s", PQerrorMessage(db->conn));        
        PQclear(res);
        return 0;;
    }    
    
	for (i = 0; i < PQntuples(res); i++) {

		id = PQgetvalue(res, i, 0);
		filter = PQgetvalue(res, i, 1);
		fields = PQgetvalue(res, i, 2);

		if ( nfd_db_mk_profile(&tmp, id, filter, fields, errbuf) ) {
			/* add profile to root profile */
			tmp->next_profile = *root_profile;
			*root_profile = tmp;
			msg(MSG_DEBUG, "Added profile id=%s, filter=%s, aggr=%s", id, filter, fields);
		} else {
			msg(MSG_ERROR, "Can not create profile: %s", errbuf);
		}
	}
	return 1;

}

int nfd_db_begin_transaction(nfd_db_t *db) {

	PGresult *res; 

	res = PQexec(db->conn, "BEGIN TRANSACTION");  


	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        msg(MSG_ERROR, "Can not BEGIN transaction: %s", PQerrorMessage(db->conn));        
        PQclear(res);
        return 0;
    }    

	db->inserted = 0;
	db->updated = 0;

	return 1;
}

int nfd_db_end_transaction(nfd_db_t *db) {

	PGresult *res; 

	res = PQexec(db->conn, "END TRANSACTION");  


	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        msg(MSG_ERROR, "Can not END transaction: %s", PQerrorMessage(db->conn));        
        PQclear(res);
        return 0;
    }    

	return 1;

}

int nfd_db_update_counters(nfd_db_t *db) {

	PGresult *res; 

	res = PQexec(db->conn, ""
		"UPDATE nfd_counters SET "
   		"	interval = EXTRACT(SECONDS FROM (updated - prev_updated)), " 
   		"	bytes_ps = (bytes - prev_bytes) /  (EXTRACT(SECONDS FROM (updated - prev_updated)) + 1), "
   		"	pkts_ps = (pkts - prev_pkts) /  (EXTRACT(SECONDS FROM (updated - prev_updated)) +1 ), "
   		"	flows_ps = (pkts - prev_flows) /  (EXTRACT(SECONDS FROM (updated - prev_updated)) + 1), "
   		"	prev_updated = updated, "
   		"	prev_bytes = bytes, "
   		"	prev_pkts = pkts, "
   		"	prev_flows = flows ");  


	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        msg(MSG_ERROR, "Can not update counter statistics: %s", PQerrorMessage(db->conn));        
    }    

	PQclear(res);
	return 1;

}

int nfd_db_store_stats(nfd_db_t *db, char *id, char *key, int window, int64_t bytes, int64_t pkts, int64_t flows) {

	int paramFormats[8] = {0, 0, 1, 1, 1, 1, 1, 1};
	const char* paramValues[8];
	int paramLengths[8];
	PGresult *res;
	int64_t bytes_ps, pkts_ps, flows_ps;

	paramValues[0] = id;
	if (id != NULL) {
		paramLengths[0] = strlen(id);
	}

	paramValues[1] = key;
	if (key != NULL) {
		paramLengths[1] = strlen(key);
	}

	bytes_ps = bytes / window;
	pkts_ps = pkts / window;
	flows_ps = flows / window;

	bytes = htonll(bytes);
	pkts = htonll(pkts);
	flows = htonll(flows);

	bytes_ps = htonll(bytes_ps);
	pkts_ps = htonll(pkts_ps);
	flows_ps = htonll(flows_ps);

	paramValues[2] = (char *)&bytes;
	paramLengths[2] = sizeof(bytes);
	paramValues[3] = (char *)&pkts;
	paramLengths[3] = sizeof(pkts);
	paramValues[4] = (char *)&flows;
	paramLengths[4] = sizeof(flows);

	paramValues[5] = (char *)&bytes_ps;
	paramLengths[5] = sizeof(bytes_ps);
	paramValues[6] = (char *)&pkts_ps;
	paramLengths[6] = sizeof(pkts_ps);
	paramValues[7] = (char *)&flows_ps;
	paramLengths[7] = sizeof(flows_ps);
	/* try to update */
	res = PQexecPrepared(db->conn, db->upd_name, 8,
            paramValues, paramLengths, paramFormats, 0);

	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		msg(MSG_ERROR, "Can not update data in db: %s", PQerrorMessage(db->conn));
        PQclear(res);
        return 0;
    } 

	/* PQntuples returns varchar ! */ 
	if (atoi(PQcmdTuples(res)) > 0 ) { 
        PQclear(res);
		/* ceonters were updated - we are done here */
		db->updated++;
		return 1;
	}


	/* try to insert record */	
	res = PQexecPrepared(db->conn, db->ins_name, 8,
            paramValues, paramLengths, paramFormats, 0);

	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		msg(MSG_ERROR, "Can not insert data in db: %s", PQerrorMessage(db->conn));
        PQclear(res);
        return 0;;
    }    

	db->inserted++;

	return 1;

}


void nfd_db_free(nfd_db_t *db) {

	PQfinish(db->conn);
}


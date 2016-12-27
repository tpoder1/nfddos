
#include <string.h>
#include "db.h"
#include "nfddos.h"

int nfd_db_init(nfd_db_t *db, nfd_db_type_t db_type, const char *connstr) {

	if (db_type != NFD_DB_PGSQL) {
		msg(MSG_ERROR, "Unsupported database type.");
	}

	db->db_type = db_type;
	db->conn = PQconnectdb(connstr);
	if (PQstatus(db->conn) == CONNECTION_BAD) {
		msg(MSG_ERROR, "Can not connect to the database: %s", PQerrorMessage(db->conn));

		return 0;
	}

	msg(MSG_INFO, "Connected to database. Server version: %d", PQserverVersion(db->conn));

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
        msg(MSG_ERROR, "Can not load profiles");        
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


void nfd_db_free(nfd_db_t *db) {

	PQfinish(db->conn);
}

/* create new profile  */
int nfd_db_mk_profile(nfd_profile_t **nfd_profile, char *id, char *filter, char *fields, char *errbuf) {

	nfd_profile_t *tmp;
	char *token = fields;
	int field, numbits, numbits6;

	
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

			msg(MSG_DEBUG, "Set fields %d if profile id %s\n", field, tmp->id);
			lnf_mem_fadd(tmp->mem, field, LNF_AGGR_KEY, numbits, numbits6);

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


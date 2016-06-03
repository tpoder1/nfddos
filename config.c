
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h>
#include <syslog.h>
#include "msgs.h"
#include "nfddos.h"

#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void* yyscan_t;
#endif

#ifndef YY_TYPEDEF_YY_BUFFER_STATE
#define YY_TYPEDEF_YY_BUFFER_STATE
typedef struct yy_buffer_state *YY_BUFFER_STATE;
#endif


int nfd_parse_config(nfd_options_t *opt) {

	yyscan_t scanner;
//	YY_BUFFER_STATE buf;
	int parse_ret;
	FILE * fp;

	fp = fopen(opt->config_file, "r");

	if (fp == NULL) {
		msg(MSG_INFO, "Can't open file %s (%s)", opt->config_file, strerror(errno));
		return 0;
	}

	yylex_init(&scanner);
	yyset_in(fp, scanner);

    parse_ret = yyparse(scanner, opt);
//    parse_ret = yyparse(scanner);

	yylex_destroy(scanner);

	//fclose(fp);

	if (parse_ret == 0) {
		msg(MSG_INFO, "Config file parsed");

	} else {

		msg(MSG_ERROR, "Can't load config file");
		return 0;
	}
	
	return 1;
}

nfd_profile_t * nfd_profile_new(nfd_options_t *opt, const char *name) {

	nfd_profile_t *tmp;

	tmp = malloc(sizeof(nfd_profile_t));

	if (tmp == NULL) {
		return NULL;
	}

	memset(tmp, 0x0, sizeof(nfd_profile_t));

    tmp->window_size = opt->window_size;
    tmp->stop_delay = opt->stop_delay;
    strncpy(tmp->name, name, MAX_STRING);

	tmp->next_profile = opt->root_profile;
	opt->root_profile = tmp;

    return tmp;
}

/* set input filter in the profile */
int nfd_profile_set_filter(nfd_profile_t *nfd_profile, char *expr) {

	if (lnf_filter_init(&nfd_profile->input_filter, expr) != LNF_OK) {

		msg(MSG_ERROR, "Invalid input filter %s\n", expr);
		return 0;
	}

	return 1;
}

/* add track into profile */
int nf_profile_add_track(nfd_profile_t *nfd_profile, char *fields, char *filter) {

	nfd_track_t *tmp;
	char *token = fields;
	int field, numbits, numbits6;

	
	tmp = malloc(sizeof(nfd_track_t));

	if (tmp == NULL) {
		return 0;
	}

	memset(tmp, 0x0, sizeof(nfd_track_t));


	/* parse aggregation fields */
	if (fields != NULL) {

		if (lnf_mem_init(&tmp->mem) != LNF_OK) {
			free(tmp);
			return 0;
		}

		strncpy(tmp->fields, fields, MAX_STRING);

		lnf_mem_fadd(tmp->mem, LNF_FLD_FIRST, LNF_AGGR_MIN, 0, 0);
		lnf_mem_fadd(tmp->mem, LNF_FLD_LAST, LNF_AGGR_MAX, 0, 0);
		lnf_mem_fadd(tmp->mem, LNF_FLD_CALC_DURATION, LNF_AGGR_SUM, 0, 0);

		while ( (token = strsep(&fields, ",")) != NULL ) {

			field = lnf_fld_parse(token, &numbits, &numbits6);
	
			if (field == LNF_FLD_ZERO_) {
				msg(MSG_ERROR, "Cannot parse field %s", token);
				free(tmp);
				return 0;
			}

			if (lnf_fld_type(field) == LNF_ADDR && (numbits > 32 || numbits6 > 128)) {
				fprintf(stderr, "Invalid bit size (%d/%d) for %s",
				numbits, numbits6, token);
				return 0;
			}

			msg(MSG_DEBUG, "Set fields %d\n", field);
			lnf_mem_fadd(tmp->mem, field, LNF_AGGR_KEY, numbits, numbits6);


		}

		lnf_mem_fadd(tmp->mem, LNF_FLD_DPKTS, LNF_AGGR_SUM, 0, 0);
		lnf_mem_fadd(tmp->mem, LNF_FLD_DOCTETS, LNF_AGGR_SUM, 0, 0);
		lnf_mem_fadd(tmp->mem, LNF_FLD_AGGR_FLOWS, LNF_AGGR_SUM, 0, 0);
	}


	if (filter != NULL) {
		if (lnf_filter_init(&tmp->filter, filter) != LNF_OK) {
			msg(MSG_ERROR, "Invalid action filter %s\n", filter);
			free(tmp);
			return 0;
		}
	}


	tmp->next_track = nfd_profile->root_track;
	nfd_profile->root_track = tmp;


	return 1;
}


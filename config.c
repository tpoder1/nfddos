
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

nfd_profile_t * nfd_profile_new(nfd_options_t *opt) {

	nfd_profile_t *tmp;

	tmp = malloc(sizeof(nfd_profile_t));

	if (tmp == NULL) {
		return NULL;
	}

	memset(tmp, 0x0, sizeof(nfd_profile_t));

    tmp->window_size = opt->window_size;
    tmp->stop_delay = opt->stop_delay;
    tmp->treshold = opt->treshold;

	if (lnf_mem_init(&tmp->main_mem) != LNF_OK) {

		free(tmp);
		return NULL;
	}

	tmp->next_profile = opt->root_profile;
	opt->root_profile = tmp;

    return tmp;
}


int nfd_profile_set_filter(nfd_profile_t *nfd_profile, char *expr) {

	if (lnf_filter_init(&nfd_profile->input_filter, expr) != LNF_OK) {

		msg(MSG_ERROR, "Invalid filter %s\n", expr);
		return 1;
	}


	return 0;
}


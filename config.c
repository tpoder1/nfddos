
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


int nfd_cfg_parse(nfd_options_t *opt) {

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

//	yyset_in(stdin, scanner);

    parse_ret = yyparse(scanner, opt);

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


int nfd_cfg_set_filter(nfd_options_t *opt, char *filter) {

	if (filter == NULL || strnlen(filter, MAX_STRING) == 0) {
		return 0;
	}

	if (opt->filter != NULL) {
		lnf_filter_free(opt->filter); 
		opt->filter = NULL;
	}

	if (lnf_filter_init_v2(&opt->filter, filter) != LNF_OK) {
		msg(MSG_ERROR, "Invalid input filter %s\n", filter);
		return 0;
	}

	return 1;
}



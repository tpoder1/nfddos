

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include "nfddos.h"

int log_debug = 0;

/***************************************************
*             HLESENI CHYB                         *
****************************************************/

void msg_init(int debug) {

	log_debug = debug;

	openlog(LOG_NAME, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

}


void msg(int type, char* msg, ...) {

//    if (type != MSG_DEBUG || debug) {
	va_list arg;
	int level;
	char buf[MAX_STRING];

	if (type == MSG_DEBUG && log_debug == 0) {
		return;	
	}

	switch (type) {
		case MSG_ERROR: 	level = LOG_ERR; break;
		case MSG_WARNING: 	level = LOG_WARNING; break;
		case MSG_DEBUG:		level = LOG_DEBUG; break; 
		default:			level = LOG_INFO; break;
	}

	va_start(arg, msg);
	vsnprintf(buf, MAX_STRING - 1, msg, arg);
	va_end(arg);

	syslog(level, "%s", buf);
	if (log_debug) {
		printf("%s\n", buf);
	}
    
}

/*
* help
*/
void help() {
    msg(MSG_INFO, "Welcome bandwidth daemon %s", LOG_VERSION);
    msg(MSG_INFO, "Comment please send to <tpoder@cis.vutbr.cz>");
    msg(MSG_INFO, "");
    msg(MSG_INFO, "Usage: bwd [-i <interface>] [ -c <config_file> ] [ -d <debug_level> ] [ -p <pidfile> ] [ -F ] ");
    msg(MSG_INFO, ""); 
    msg(MSG_INFO, "-c config file (default: /etc/bwd/bwd.conf)"); 
    msg(MSG_INFO, "-p pid file (default: /var/run/bwd.pid)"); 
    msg(MSG_INFO, "-F run in foreground"); 
    msg(MSG_INFO, "-i interface name to listen on"); 
    msg(MSG_INFO, ""); 
    exit(0);
}

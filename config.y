/* 

 Copyright (c) 2013-2015, Tomas Podermanski
    
 This file is part of libnf.net project.

 Libnf is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Libnf is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with libnf.  If not, see <http://www.gnu.org/licenses/>.

*/

%defines
%pure-parser
%lex-param   { yyscan_t scanner } 
%lex-param   { nfd_options_t *opt }
%parse-param { yyscan_t scanner } 
%parse-param { nfd_options_t *opt }

%{
	#include <stdio.h>
	#include "nfddos.h"
	#include <string.h>
//	#include "ffilter.h"
//	#include "ffilter_internal.h"

	#define YY_EXTRA_TYPE nfd_options_t

//	int ff2_lex();


#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void* yyscan_t;
#endif

/*
#ifndef YY_TYPEDEF_YY_BUFFER_STATE
#define YY_TYPEDEF_YY_BUFFER_STATE
typedef struct yy_buffer_state *YY_BUFFER_STATE;
#endif
*/

	#define MAX_STRING 1024

	//void yyerror(yyscan_t scanner, ff_t *filter, char *msg) {
	void yyerror(yyscan_t scanner, nfd_options_t *opt, char *errmsg) {

		msg(MSG_ERROR, "config file: %s (line: %d)", errmsg, yyget_lineno(scanner) );
//		ff_set_error(filter, msg);
	}


%}

%union {
	long int	number;	
	char 		string[MAX_STRING];
	void		*node;
	nfd_profile_t	*nfd_profile;
};

%token OBRACE EBRACE SEMICOLON
%token PROFILETOK LIMITTOK FILTERTOK
%token BITPSTOK PPSTOK FPSTOK FILETOK STARTTOK STOPTOK
%token OPTIONSTOK DEBUGTOK PIDTOK ACTIONTOK WHENTOK TRACKTOK
%token LEVELTOK COMMANDTOK NEWTOK DELTOK 
%token HASHTOK BUCKETSTOK DSTIPTOK WINDOWTOK DYNAMICTOK 
%token SLOTSTOK TIMETOK SIZETOK EXPIRETOK DELAYTOK INPUTTOK
%token SHMTOK DBTOK CONNECTTOK EXPORTTOK INTERVALTOK MINTOK
%token <number> NUMBER FACTOR
%token <string> STRING
/* %type <string> action */
%type <nfd_profile> rule rules ruleparam ruleparams  
/* %type <string> options optionparams option */

%%

config: /* empty */
	options rules
	;

options: /* empty */
	OPTIONSTOK OBRACE optionparams EBRACE; 
	;

optionparams: /* empty */ 
	| optionparams option SEMICOLON;
	;

option:
	| DEBUGTOK LEVELTOK NUMBER 		{ if (!opt->debug_fromarg) opt->debug = $3; }
	| PIDTOK FILETOK STRING         { if (!opt->pid_file_fromarg) strncpy(opt->pid_file, $3, MAX_STRING); } 
	| WINDOWTOK SIZETOK NUMBER 			{ opt->window_size = $3; }
	| INPUTTOK SHMTOK STRING       		{ strncpy(opt->shm, $3, MAX_STRING); } 
	| INPUTTOK FILTERTOK STRING		{ if (!nfd_cfg_set_filter(opt, $3)) { msg(MSG_ERROR, "Can not set filter \"%s\"", $3); YYABORT; } ; }
	| DBTOK CONNECTTOK STRING         		{ strncpy(opt->db_connstr, $3, MAX_STRING); } 
	| DBTOK EXPORTTOK INTERVALTOK NUMBER 	{ opt->export_interval = $4; }
	| DBTOK EXPORTTOK MINTOK PPSTOK NUMBER 	{ opt->export_min_pps = $5; }
	| ACTIONTOK STARTTOK COMMANDTOK STRING	{ strncpy(opt->exec_start, $4, MAX_STRING); }
	| ACTIONTOK STOPTOK COMMANDTOK STRING	{ strncpy(opt->exec_start, $4, MAX_STRING); }
	| ACTIONTOK STOPTOK DELAYTOK NUMBER 	{ opt->stop_delay = $4; }
	;

rules: /* empty */
	| rules rule;

rule: 
	| PROFILETOK STRING OBRACE { $<nfd_profile>$ = nfd_prof_new($2); if (!$<nfd_profile>$) { YYABORT; } else { nfd_prof_add(&opt->root_profile, $<nfd_profile>$); }; } ruleparams EBRACE	{ ;  } 
	| PROFILETOK DBTOK SEMICOLON { opt->db_profiles = 1; }
	;

ruleparams: /* empty */ 
	| ruleparams { $<nfd_profile>$ = $<nfd_profile>0; } ruleparam SEMICOLON;
	;

ruleparam:
	| FILTERTOK STRING 					{ if (!nfd_prof_set_filter($<nfd_profile>0, $2)) { msg(MSG_ERROR, "Can not set filter \"%s\"", $2); YYABORT; } ; }
	| DYNAMICTOK STRING 				{ if (!nfd_prof_set_dynamic($<nfd_profile>0, $2)) { msg(MSG_ERROR, "Can not set dynamic \"%s\"", $2); YYABORT; } ; }
	| LIMITTOK NUMBER BITPSTOK 			{ $<nfd_profile>0->limits.bytes = $2; }
	| LIMITTOK NUMBER PPSTOK 			{ $<nfd_profile>0->limits.pkts  = $2; }
	| LIMITTOK NUMBER FPSTOK 			{ $<nfd_profile>0->limits.flows = $2; }
	| LIMITTOK NUMBER FACTOR BITPSTOK 	{ $<nfd_profile>0->limits.bytes = ($2 * $3) / 8; }
	| LIMITTOK NUMBER FACTOR PPSTOK 	{ $<nfd_profile>0->limits.pkts  = $2 * $3; }
	| LIMITTOK NUMBER FACTOR FPSTOK 	{ $<nfd_profile>0->limits.flows = $2 * $3; }
	; 

%% 

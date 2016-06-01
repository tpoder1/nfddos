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

%option outfile="lex.yy.c"
%option reentrant bison-bridge
%option noyywrap 
%option nounput
%option noinput
%option yylineno

%{
	#include <stdlib.h>
	#include <string.h>
	#include "nfddos.h"
	#include "config.tab.h"
%}

%%
#.*				{ ; }	// comments 
\"(.+)\"			{
					strncpy(yylval->string, yytext + 1, MAX_STRING - 1);
					yylval->string[yyleng - 2] = '\0';
					return STRING;
				}
\{				return OBRACE;
\}				return EBRACE;
;				return SEMICOLON;
options			return OPTIONSTOK; 
pid				return PIDTOK; 
file			return FILETOK; 
debug			return DEBUGTOK; 
level			return LEVELTOK; 
action			return ACTIONTOK; 
command			return COMMANDTOK; 
start			return STARTTOK; 
stop			return STOPTOK; 
window			return WINDOWTOK; 
size			return SIZETOK; 
expire			return EXPIRETOK; 
delay			return DELAYTOK; 
filter			return FILTERTOK; 
profile			return PROFILETOK; 
limit			return LIMITTOK; 
input			return INPUTTOK; 
bps				return BITPSTOK; 
b\/s			return BITPSTOK; 
pps				return PPSTOK; 
p\/s			return PPSTOK; 
[K|k]			{
					yylval->number = 1000;
					return FACTOR;
				}
[M|m]			{
					yylval->number = 1000 * 1000;
					return FACTOR;
				}
[0-9]+			{
					yylval->number = atoi(yytext);
					return NUMBER;
				}
\n				{ ; } 	// ignore EOL 
[ \t]+			{ ; } 	// ignore whitespace 
%%


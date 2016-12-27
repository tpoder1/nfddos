/* A Bison parser, made by GNU Bison 2.7.  */

/* Bison interface for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2012 Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

#ifndef YY_YY_CONFIG_TAB_H_INCLUDED
# define YY_YY_CONFIG_TAB_H_INCLUDED
/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     OBRACE = 258,
     EBRACE = 259,
     SEMICOLON = 260,
     PROFILETOK = 261,
     LIMITTOK = 262,
     FILTERTOK = 263,
     BITPSTOK = 264,
     PPSTOK = 265,
     FILETOK = 266,
     STARTTOK = 267,
     STOPTOK = 268,
     OPTIONSTOK = 269,
     DEBUGTOK = 270,
     PIDTOK = 271,
     ACTIONTOK = 272,
     WHENTOK = 273,
     TRACKTOK = 274,
     LEVELTOK = 275,
     COMMANDTOK = 276,
     NEWTOK = 277,
     DELTOK = 278,
     HASHTOK = 279,
     BUCKETSTOK = 280,
     DSTIPTOK = 281,
     WINDOWTOK = 282,
     SLOTSTOK = 283,
     TIMETOK = 284,
     SIZETOK = 285,
     EXPIRETOK = 286,
     DELAYTOK = 287,
     INPUTTOK = 288,
     NUMBER = 289,
     FACTOR = 290,
     STRING = 291
   };
#endif


#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{
/* Line 2058 of yacc.c  */
#line 65 "config.y"

	long int	number;	
	char 		string[MAX_STRING];
	void		*node;
	nfd_profile_t	*nfd_profile;
	nfd_track_t		*nfd_track;


/* Line 2058 of yacc.c  */
#line 102 "config.tab.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (yyscan_t scanner, nfd_options_t *opt);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */

#endif /* !YY_YY_CONFIG_TAB_H_INCLUDED  */

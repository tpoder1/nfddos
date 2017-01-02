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
     FPSTOK = 266,
     FILETOK = 267,
     STARTTOK = 268,
     STOPTOK = 269,
     OPTIONSTOK = 270,
     DEBUGTOK = 271,
     PIDTOK = 272,
     ACTIONTOK = 273,
     WHENTOK = 274,
     TRACKTOK = 275,
     LEVELTOK = 276,
     COMMANDTOK = 277,
     NEWTOK = 278,
     DELTOK = 279,
     HASHTOK = 280,
     BUCKETSTOK = 281,
     DSTIPTOK = 282,
     WINDOWTOK = 283,
     DYNAMICTOK = 284,
     SLOTSTOK = 285,
     TIMETOK = 286,
     SIZETOK = 287,
     EXPIRETOK = 288,
     DELAYTOK = 289,
     INPUTTOK = 290,
     SHMTOK = 291,
     DBTOK = 292,
     CONNECTTOK = 293,
     EXPORTTOK = 294,
     INTERVALTOK = 295,
     MINTOK = 296,
     NUMBER = 297,
     FACTOR = 298,
     STRING = 299
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


/* Line 2058 of yacc.c  */
#line 109 "config.tab.h"
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

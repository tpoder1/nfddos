/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

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
     WINDOWTOK = 279,
     SIZETOK = 280,
     EXPIRETOK = 281,
     DELAYTOK = 282,
     INPUTTOK = 283,
     NUMBER = 284,
     FACTOR = 285,
     STRING = 286
   };
#endif
/* Tokens.  */
#define OBRACE 258
#define EBRACE 259
#define SEMICOLON 260
#define PROFILETOK 261
#define LIMITTOK 262
#define FILTERTOK 263
#define BITPSTOK 264
#define PPSTOK 265
#define FILETOK 266
#define STARTTOK 267
#define STOPTOK 268
#define OPTIONSTOK 269
#define DEBUGTOK 270
#define PIDTOK 271
#define ACTIONTOK 272
#define WHENTOK 273
#define TRACKTOK 274
#define LEVELTOK 275
#define COMMANDTOK 276
#define NEWTOK 277
#define DELTOK 278
#define WINDOWTOK 279
#define SIZETOK 280
#define EXPIRETOK 281
#define DELAYTOK 282
#define INPUTTOK 283
#define NUMBER 284
#define FACTOR 285
#define STRING 286




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 62 "config.y"
{
	long int	number;	
	char 		string[MAX_STRING];
	void		*node;
	nfd_profile_t	*nfd_profile;
	nfd_track_t		*nfd_track;
}
/* Line 1529 of yacc.c.  */
#line 119 "config.tab.h"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif




/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __CF_GRAMATICAL_H__
#define __CF_GRAMATICAL_H__

#ifndef YYSTYPE
typedef union	{
			cf_value Val;
		} yystype;
# define YYSTYPE yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif
# define	ADD_ASSIGN	257
# define	ASSIGN	258
# define	VARIABLE	259
# define	STRING	260
# define	SEMICOLON	261
# define	PLUS	262
# define	MINUS	263
# define	MULT	264
# define	DIVIDE	265
# define	RPAREN	266
# define	LPAREN	267
# define	RBRACE	268
# define	LBRACE	269
# define	COMMA	270
# define	INT	271
# define	REAL	272


extern YYSTYPE cflval;

#endif /* not __CF_GRAMATICAL_H__ */

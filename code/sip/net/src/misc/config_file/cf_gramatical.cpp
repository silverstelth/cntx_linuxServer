/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

/* A Bison parser, made from .\misc\config_file\cf_gramatical.yxx
   by GNU bison 1.35.  */

#define YYBISON 1  /* Identify Bison output.  */

#define yyparse cfparse
#define yylex cflex
#define yyerror cferror
#define yylval cflval
#define yychar cfchar
#define yydebug cfdebug
#define yynerrs cfnerrs
#define	ADD_ASSIGN	257
#define	ASSIGN	258
#define	VARIABLE	259
#define	STRING	260
#define	SEMICOLON	261
#define	PLUS	262
#define	MINUS	263
#define	MULT	264
#define	DIVIDE	265
#define	RPAREN	266
#define	LPAREN	267
#define	RBRACE	268
#define	LBRACE	269
#define	COMMA	270
#define	INT	271
#define	REAL	272

#line 1 ".\\misc\\config_file\\cf_gramatical.yxx"

#ifdef SIP_OS_WINDOWS
#pragma warning (disable : 4786)
#endif // SIP_OS_WINDOWS

#include <stdio.h>
#include <vector>
#include <string>

#include "misc/config_file.h"
#include "misc/common.h"
#include "misc/debug.h"

using namespace std;
using namespace SIPBASE;

/* Constantes */

#define YYPARSE_PARAM pvararray

// WARNING!!!! DEBUG_PRINTF are commented using // so IT MUST HAVE NO INSTRUCTION AFTER A DEBUG_PRINTF OR THEY LL BE COMMENTED
/*
#define DEBUG_PRINTF	InfoLog->displayRaw
#define DEBUG_PRINT(a)	InfoLog->displayRaw(a)
*/

#define DEBUG_PRINT(a)
#ifdef __GNUC__
#define DEBUG_PRINTF(format, args...)
#else // __GNUC__
#define DEBUG_PRINTF	// InfoLog->displayRaw
#endif // __GNUC__


/* Types */

enum cf_operation { OP_PLUS, OP_MINUS, OP_MULT, OP_DIVIDE, OP_NEG };

struct cf_value
{
	SIPBASE::CConfigFile::CVar::TVarType	Type;
	int						Int;
	double					Real;
	char					String[1024];
};

/* Externals */

extern bool cf_Ignore;

extern bool LoadRoot;

extern FILE *yyin;

/* Variables */

SIPBASE::CConfigFile::CVar		cf_CurrentVar;

int		cf_CurrentLine;

bool	cf_OverwriteExistingVariable;	// setup in the config_file.cpp reparse()



/* Prototypes */

int yylex (void);

cf_value cf_op (cf_value a, cf_value b, cf_operation op);

void cf_print (cf_value Val);

void cf_setVar (SIPBASE::CConfigFile::CVar &Var, cf_value Val);

int yyerror (const char *);


#line 84 ".\\misc\\config_file\\cf_gramatical.yxx"
#ifndef YYSTYPE
typedef union	{
			cf_value Val;
		} yystype;
# define YYSTYPE yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif
#ifndef YYDEBUG
# define YYDEBUG 0
#endif



#define	YYFINAL		44
#define	YYFLAG		-32768
#define	YYNTBASE	19

/* YYTRANSLATE(YYLEX) -- Bison token number corresponding to YYLEX. */
#define YYTRANSLATE(x) ((unsigned)(x) <= 272 ? yytranslate[x] : 28)

/* YYTRANSLATE[YYLEX] -- Bison token number corresponding to YYLEX. */
static const char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18
};

#if YYDEBUG
static const short yyprhs[] =
{
       0,     0,     2,     3,     6,     8,    13,    18,    20,    24,
      29,    32,    34,    38,    40,    44,    48,    50,    54,    58,
      61,    64,    68,    70,    72,    74,    76
};
static const short yyrhs[] =
{
      20,     0,     0,    20,    21,     0,    21,     0,     5,     4,
      22,     7,     0,     5,     3,    22,     7,     0,    24,     0,
      15,    23,    14,     0,    15,    23,    16,    14,     0,    15,
      14,     0,    24,     0,    23,    16,    24,     0,    25,     0,
      24,     8,    25,     0,    24,     9,    25,     0,    26,     0,
      25,    10,    26,     0,    25,    11,    26,     0,     8,    26,
       0,     9,    26,     0,    13,    22,    12,     0,    17,     0,
      18,     0,     6,     0,    27,     0,     5,     0
};

#endif

#if YYDEBUG
/* YYRLINE[YYN] -- source line where rule number YYN was defined. */
static const short yyrline[] =
{
       0,   103,   103,   106,   107,   110,   172,   232,   233,   234,
     235,   238,   239,   242,   243,   244,   247,   248,   249,   252,
     253,   254,   255,   256,   257,   258,   261
};
#endif


#if (YYDEBUG) || defined YYERROR_VERBOSE

/* YYTNAME[TOKEN_NUM] -- String name of the token TOKEN_NUM. */
static const char *const yytname[] =
{
  "$", "error", "$undefined.", "ADD_ASSIGN", "ASSIGN", "VARIABLE", "STRING", 
  "SEMICOLON", "PLUS", "MINUS", "MULT", "DIVIDE", "RPAREN", "LPAREN", 
  "RBRACE", "LBRACE", "COMMA", "INT", "REAL", "ROOT", "instlist", "inst", 
  "expression", "exprbrace", "expr2", "expr3", "expr4", "variable", 0
};
#endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives. */
static const short yyr1[] =
{
       0,    19,    19,    20,    20,    21,    21,    22,    22,    22,
      22,    23,    23,    24,    24,    24,    25,    25,    25,    26,
      26,    26,    26,    26,    26,    26,    27
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN. */
static const short yyr2[] =
{
       0,     1,     0,     2,     1,     4,     4,     1,     3,     4,
       2,     1,     3,     1,     3,     3,     1,     3,     3,     2,
       2,     3,     1,     1,     1,     1,     1
};

/* YYDEFACT[S] -- default rule to reduce with in state S when YYTABLE
   doesn't specify something else to do.  Zero means the default is an
   error. */
static const short yydefact[] =
{
       2,     0,     1,     4,     0,     0,     3,    26,    24,     0,
       0,     0,     0,    22,    23,     0,     7,    13,    16,    25,
       0,    19,    20,     0,    10,     0,    11,     6,     0,     0,
       0,     0,     5,    21,     8,     0,    14,    15,    17,    18,
       9,    12,     0,     0,     0
};

static const short yydefgoto[] =
{
      42,     2,     3,    15,    25,    16,    17,    18,    19
};

static const short yypact[] =
{
      -3,     7,    -3,-32768,     0,     0,-32768,-32768,-32768,    49,
      49,     0,    21,-32768,-32768,     9,    11,    22,-32768,-32768,
      24,-32768,-32768,    16,-32768,    -2,    11,-32768,    49,    49,
      49,    49,-32768,-32768,-32768,    35,    22,    22,-32768,-32768,
  -32768,    11,    36,    37,-32768
};

static const short yypgoto[] =
{
  -32768,-32768,    40,    -4,-32768,   -12,    -7,    -6,-32768
};


#define	YYLAST		67


static const short yytable[] =
{
      26,    20,     1,    21,    22,     7,     8,    23,     9,    10,
       4,     5,    34,    11,    35,    12,    27,    13,    14,    28,
      29,    36,    37,    41,    38,    39,     7,     8,    33,     9,
      10,    32,    30,    31,    11,    24,    43,    44,    13,    14,
       7,     8,     6,     9,    10,     0,     0,     0,    11,    40,
       0,     0,    13,    14,     7,     8,     0,     9,    10,     0,
       0,     0,    11,     0,     0,     0,    13,    14
};

static const short yycheck[] =
{
      12,     5,     5,     9,    10,     5,     6,    11,     8,     9,
       3,     4,    14,    13,    16,    15,     7,    17,    18,     8,
       9,    28,    29,    35,    30,    31,     5,     6,    12,     8,
       9,     7,    10,    11,    13,    14,     0,     0,    17,    18,
       5,     6,     2,     8,     9,    -1,    -1,    -1,    13,    14,
      -1,    -1,    17,    18,     5,     6,    -1,     8,     9,    -1,
      -1,    -1,    13,    -1,    -1,    -1,    17,    18
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "c:/program files/GnuWin32/share/bison/bison.simple"

/* Skeleton output parser for bison,

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002 Free Software
   Foundation, Inc.

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
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* This is the parser code that is written into each bison parser when
   the %semantic_parser declaration is not specified in the grammar.
   It was written by Richard Stallman by simplifying the hairy parser
   used when %semantic_parser is specified.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

#if ! defined (yyoverflow) || defined (YYERROR_VERBOSE)

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# if YYSTACK_USE_ALLOCA
#  define YYSTACK_ALLOC alloca
# else
#  ifndef YYSTACK_USE_ALLOCA
#   if defined (alloca) || defined (_ALLOCA_H)
#    define YYSTACK_ALLOC alloca
#   else
#    ifdef __GNUC__
#     define YYSTACK_ALLOC __builtin_alloca
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
# else
#  if defined (__STDC__) || defined (__cplusplus)
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   define YYSIZE_T size_t
#  endif
#  define YYSTACK_ALLOC malloc
#  define YYSTACK_FREE free
# endif
#endif /* ! defined (yyoverflow) || defined (YYERROR_VERBOSE) */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (YYLTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short yyss;
  YYSTYPE yyvs;
# if YYLSP_NEEDED
  YYLTYPE yyls;
# endif
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAX (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# if YYLSP_NEEDED
#  define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE) + sizeof (YYLTYPE))	\
      + 2 * YYSTACK_GAP_MAX)
# else
#  define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))				\
      + YYSTACK_GAP_MAX)
# endif

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  register YYSIZE_T yyi;		\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAX;	\
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif


#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# if defined (__STDC__) || defined (__cplusplus)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# endif
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	goto yyacceptlab
#define YYABORT 	goto yyabortlab
#define YYERROR		goto yyerrlab1
/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");			\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).

   When YYLLOC_DEFAULT is run, CURRENT is set the location of the
   first token.  By default, to implement support for ranges, extend
   its range to the last symbol.  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)       	\
   Current.last_line   = Rhs[N].last_line;	\
   Current.last_column = Rhs[N].last_column;
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#if YYPURE
# if YYLSP_NEEDED
#  ifdef YYLEX_PARAM
#   define YYLEX		yylex (&yylval, &yylloc, YYLEX_PARAM)
#  else
#   define YYLEX		yylex (&yylval, &yylloc)
#  endif
# else /* !YYLSP_NEEDED */
#  ifdef YYLEX_PARAM
#   define YYLEX		yylex (&yylval, YYLEX_PARAM)
#  else
#   define YYLEX		yylex (&yylval)
#  endif
# endif /* !YYLSP_NEEDED */
#else /* !YYPURE */
# define YYLEX			yylex ()
#endif /* !YYPURE */


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (0)
/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
#endif /* !YYDEBUG */

/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#if YYMAXDEPTH == 0
# undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif

#ifdef YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  register const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  register char *yyd = yydest;
  register const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif
#endif

#line 315 "c:/program files/GnuWin32/share/bison/bison.simple"


/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into yyparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
#  define YYPARSE_PARAM_ARG void *YYPARSE_PARAM
#  define YYPARSE_PARAM_DECL
# else
#  define YYPARSE_PARAM_ARG YYPARSE_PARAM
#  define YYPARSE_PARAM_DECL void *YYPARSE_PARAM;
# endif
#else /* !YYPARSE_PARAM */
# define YYPARSE_PARAM_ARG
# define YYPARSE_PARAM_DECL
#endif /* !YYPARSE_PARAM */

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
# ifdef YYPARSE_PARAM
int yyparse (void *);
# else
int yyparse (void);
# endif
#endif

/* YY_DECL_VARIABLES -- depending whether we use a pure parser,
   variables are global, or local to YYPARSE.  */

#define YY_DECL_NON_LSP_VARIABLES			\
/* The lookahead symbol.  */				\
int yychar;						\
							\
/* The semantic value of the lookahead symbol. */	\
YYSTYPE yylval;						\
							\
/* Number of parse errors so far.  */			\
int yynerrs;

#if YYLSP_NEEDED
# define YY_DECL_VARIABLES			\
YY_DECL_NON_LSP_VARIABLES			\
						\
/* Location data for the lookahead symbol.  */	\
YYLTYPE yylloc;
#else
# define YY_DECL_VARIABLES			\
YY_DECL_NON_LSP_VARIABLES
#endif


/* If nonreentrant, generate the variables here. */

#if !YYPURE
YY_DECL_VARIABLES
#endif  /* !YYPURE */

int
yyparse (YYPARSE_PARAM_ARG)
     YYPARSE_PARAM_DECL
{
  /* If reentrant, generate the variables here. */
#if YYPURE
  YY_DECL_VARIABLES
#endif  /* !YYPURE */

  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yychar1 = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack. */
  short	yyssa[YYINITDEPTH];
  short *yyss = yyssa;
  register short *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;

#if YYLSP_NEEDED
  /* The location stack.  */
  YYLTYPE yylsa[YYINITDEPTH];
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;
#endif

#if YYLSP_NEEDED
# define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
# define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  YYSIZE_T yystacksize = YYINITDEPTH;


  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
#if YYLSP_NEEDED
  YYLTYPE yyloc;
#endif

  /* When reducing, the number of symbols on the RHS of the reduced
     rule. */
  int yylen;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;
#if YYLSP_NEEDED
  yylsp = yyls;
#endif
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  */
# if YYLSP_NEEDED
	YYLTYPE *yyls1 = yyls;
	/* This used to be a conditional around just the two extra args,
	   but that might be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yyls1, yysize * sizeof (*yylsp),
		    &yystacksize);
	yyls = yyls1;
# else
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);
# endif
	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;

      {
	short *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);
# if YYLSP_NEEDED
	YYSTACK_RELOCATE (yyls);
# endif
# undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
#if YYLSP_NEEDED
      yylsp = yyls + yysize - 1;
#endif

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yychar1 = YYTRANSLATE (yychar);

#if YYDEBUG
     /* We have to keep this `#if YYDEBUG', since we use variables
	which are defined only if `YYDEBUG' is set.  */
      if (yydebug)
	{
	  YYFPRINTF (stderr, "Next token is %d (%s",
		     yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise
	     meaning of a token, for further debugging info.  */
# ifdef YYPRINT
	  YYPRINT (stderr, yychar, yylval);
# endif
	  YYFPRINTF (stderr, ")\n");
	}
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */
  YYDPRINTF ((stderr, "Shifting token %d (%s), ",
	      yychar, yytname[yychar1]));

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#if YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to the semantic value of
     the lookahead token.  This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

#if YYLSP_NEEDED
  /* Similarly for the default location.  Let the user run additional
     commands if for instance locations are ranges.  */
  yyloc = yylsp[1-yylen];
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
#endif

#if YYDEBUG
  /* We have to keep this `#if YYDEBUG', since we use variables which
     are defined only if `YYDEBUG' is set.  */
  if (yydebug)
    {
      int yyi;

      YYFPRINTF (stderr, "Reducing via rule %d (line %d), ",
		 yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (yyi = yyprhs[yyn]; yyrhs[yyi] > 0; yyi++)
	YYFPRINTF (stderr, "%s ", yytname[yyrhs[yyi]]);
      YYFPRINTF (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif

  switch (yyn) {

case 2:
#line 103 ".\\misc\\config_file\\cf_gramatical.yxx"
{ ;
    break;}
case 3:
#line 106 ".\\misc\\config_file\\cf_gramatical.yxx"
{ ;
    break;}
case 4:
#line 107 ".\\misc\\config_file\\cf_gramatical.yxx"
{ ;
    break;}
case 5:
#line 111 ".\\misc\\config_file\\cf_gramatical.yxx"
{
				DEBUG_PRINTF("                                   (TYPE %d VARIABLE=", yyvsp[-3].Val.Type);
				cf_print (yyvsp[-3].Val);
				DEBUG_PRINTF("), (TYPE %d VALUE=", yyvsp[-1].Val.Type);
				cf_print (yyvsp[-1].Val);
				DEBUG_PRINT(")\n");
				int i = 0;
				// on recherche l'existence de la variable
				int nSize = (int)((*((vector<SIPBASE::CConfigFile::CVar>*)(YYPARSE_PARAM))).size());
				if ( nSize != 0 )
					for(i = nSize - 1; i >= 0 ; i--)
					{
						if ((*((vector<SIPBASE::CConfigFile::CVar>*)(YYPARSE_PARAM)))[i].Name == yyvsp[-3].Val.String)
						{
							if (cf_OverwriteExistingVariable || (*((vector<SIPBASE::CConfigFile::CVar>*)(YYPARSE_PARAM)))[i].Root || !strcmp(yyvsp[-3].Val.String,"InsertConfigFilename"))
							{
								DEBUG_PRINTF("Variable '%s' existe deja, ecrasement\n", yyvsp[-3].Val.String);
							}
							break;
						}
					}
				if ( i < 0 )	i = nSize;
				SIPBASE::CConfigFile::CVar Var;
				Var.Comp = false;
				Var.Callback = NULL;
				if (cf_CurrentVar.Comp)
				{
					DEBUG_PRINTF ("yacc: new assign complex variable '%s'\n", yyvsp[-3].Val.String);
					Var = cf_CurrentVar;
				}
				else
				{
					DEBUG_PRINTF ("yacc: new assign normal variable '%s'\n", yyvsp[-3].Val.String);
					cf_setVar (Var, yyvsp[-1].Val);
				}
				Var.Name = yyvsp[-3].Val.String;
				if (i == (int)((*((vector<SIPBASE::CConfigFile::CVar>*)(YYPARSE_PARAM))).size ()))
				{
					// nouvelle variable
					DEBUG_PRINTF ("yacc: new assign var '%s'\n", yyvsp[-3].Val.String);
					(*((vector<SIPBASE::CConfigFile::CVar>*)(YYPARSE_PARAM))).push_back (Var);
				}
				else if (cf_OverwriteExistingVariable || (*((vector<SIPBASE::CConfigFile::CVar>*)(YYPARSE_PARAM)))[i].Root || !strcmp(yyvsp[-3].Val.String,"InsertConfigFilename"))
				{
					// reaffectation d'une variable 
					Var.Callback = (*((vector<SIPBASE::CConfigFile::CVar>*)(YYPARSE_PARAM)))[i].Callback;
					DEBUG_PRINTF ("yacc: reassign var name '%s' type %d\n", Var.Name.c_str(), Var.Type);
					if (Var != (*((vector<SIPBASE::CConfigFile::CVar>*)(YYPARSE_PARAM)))[i] && Var.Callback != NULL)
						(Var.Callback)(Var);
					(*((vector<SIPBASE::CConfigFile::CVar>*)(YYPARSE_PARAM)))[i] = Var;
				}
				else if ( !cf_OverwriteExistingVariable )				
				{
					// nouvelle variable
					DEBUG_PRINTF ("yacc: new assign var '%s'\n", yyvsp[-3].Val.String);
					(*((vector<SIPBASE::CConfigFile::CVar>*)(YYPARSE_PARAM))).push_back (Var);
				}				
				else
				{
					DEBUG_PRINTF ("yacc: don't reassign var '%s' because the variable already exists\n", yyvsp[-3].Val.String);
				}

				cf_CurrentVar.IntValues.clear ();
				cf_CurrentVar.RealValues.clear ();
				cf_CurrentVar.StrValues.clear ();
				cf_CurrentVar.Comp = false;
				cf_CurrentVar.Type = SIPBASE::CConfigFile::CVar::T_UNKNOWN;
			;
    break;}
case 6:
#line 173 ".\\misc\\config_file\\cf_gramatical.yxx"
{
				DEBUG_PRINT("                                   (VARIABLE+=");
				cf_print (yyvsp[-3].Val);
				DEBUG_PRINT("), (VALUE=");
				cf_print (yyvsp[-1].Val);
				DEBUG_PRINT(")\n");
				int i;
				// on recherche l'existence de la variable
				for(i = 0; i < (int)((*((vector<SIPBASE::CConfigFile::CVar>*)(YYPARSE_PARAM))).size()); i++)
				{
					if ((*((vector<SIPBASE::CConfigFile::CVar>*)(YYPARSE_PARAM)))[i].Name == yyvsp[-3].Val.String)
					{
						DEBUG_PRINTF("Variable '%s' existe deja, ajout\n", yyvsp[-3].Val.String);
						break;
					}
				}
				SIPBASE::CConfigFile::CVar Var;
				Var.Comp = false;
				Var.Callback = NULL;
				if (cf_CurrentVar.Comp) Var = cf_CurrentVar;
				else cf_setVar (Var, yyvsp[-1].Val);
				Var.Name = yyvsp[-3].Val.String;
				if (i == (int)((*((vector<SIPBASE::CConfigFile::CVar>*)(YYPARSE_PARAM))).size ()))
				{
					// nouvelle variable
					DEBUG_PRINTF ("yacc: new add assign var '%s'\n", yyvsp[-3].Val.String);
					(*((vector<SIPBASE::CConfigFile::CVar>*)(YYPARSE_PARAM))).push_back (Var);
				}
				else
				{
					// reaffectation d'une variable
					Var.Callback = (*((vector<SIPBASE::CConfigFile::CVar>*)(YYPARSE_PARAM)))[i].Callback;
					DEBUG_PRINTF ("yacc: add assign var '%s'\n", yyvsp[-3].Val.String);
					if ((*((vector<SIPBASE::CConfigFile::CVar>*)(YYPARSE_PARAM)))[i].FromLocalFile)
					{
						// this var was created in the current cfg, append the new value at the end
						(*((vector<SIPBASE::CConfigFile::CVar>*)(YYPARSE_PARAM)))[i].add(Var);

						if (Var.size() > 0 && Var.Callback != NULL)
							(Var.Callback)(Var);
					}
					else
					{
						// this var has been created in a parent Cfg, append at the begining of the array
						Var.add ((*((vector<SIPBASE::CConfigFile::CVar>*)(YYPARSE_PARAM)))[i]);
						if (Var != (*((vector<SIPBASE::CConfigFile::CVar>*)(YYPARSE_PARAM)))[i] && Var.Callback != NULL)
							(Var.Callback)(Var);
						(*((vector<SIPBASE::CConfigFile::CVar>*)(YYPARSE_PARAM)))[i] = Var;
					}
				}

				cf_CurrentVar.IntValues.clear ();
				cf_CurrentVar.RealValues.clear ();
				cf_CurrentVar.StrValues.clear ();
				cf_CurrentVar.Comp = false;
				cf_CurrentVar.Type = SIPBASE::CConfigFile::CVar::T_UNKNOWN;
			;
    break;}
case 7:
#line 232 ".\\misc\\config_file\\cf_gramatical.yxx"
{ yyval.Val = yyvsp[0].Val; cf_CurrentVar.Comp = false; DEBUG_PRINT("false\n"); ;
    break;}
case 8:
#line 233 ".\\misc\\config_file\\cf_gramatical.yxx"
{ yyval.Val = yyvsp[-1].Val; cf_CurrentVar.Comp = true; DEBUG_PRINT("true\n"); ;
    break;}
case 9:
#line 234 ".\\misc\\config_file\\cf_gramatical.yxx"
{ yyval.Val = yyvsp[-2].Val; cf_CurrentVar.Comp = true; DEBUG_PRINT("true\n"); ;
    break;}
case 10:
#line 235 ".\\misc\\config_file\\cf_gramatical.yxx"
{ yyval.Val = yyvsp[0].Val; cf_CurrentVar.Comp = true; DEBUG_PRINT("true\n"); ;
    break;}
case 11:
#line 238 ".\\misc\\config_file\\cf_gramatical.yxx"
{ yyval.Val = yyvsp[0].Val; /*cf_CurrentVar.Type = $1.Type;*/ cf_setVar (cf_CurrentVar, yyvsp[0].Val); ;
    break;}
case 12:
#line 239 ".\\misc\\config_file\\cf_gramatical.yxx"
{ yyval.Val = yyvsp[0].Val; /*cf_CurrentVar.Type = $3.Type;*/ cf_setVar (cf_CurrentVar, yyvsp[0].Val); ;
    break;}
case 13:
#line 242 ".\\misc\\config_file\\cf_gramatical.yxx"
{ yyval.Val = yyvsp[0].Val; ;
    break;}
case 14:
#line 243 ".\\misc\\config_file\\cf_gramatical.yxx"
{ yyval.Val = cf_op(yyvsp[-2].Val, yyvsp[0].Val, OP_PLUS); ;
    break;}
case 15:
#line 244 ".\\misc\\config_file\\cf_gramatical.yxx"
{ yyval.Val = cf_op(yyvsp[-2].Val, yyvsp[0].Val, OP_MINUS); ;
    break;}
case 16:
#line 247 ".\\misc\\config_file\\cf_gramatical.yxx"
{ yyval.Val = yyvsp[0].Val; ;
    break;}
case 17:
#line 248 ".\\misc\\config_file\\cf_gramatical.yxx"
{ yyval.Val = cf_op(yyvsp[-2].Val, yyvsp[0].Val, OP_MULT); ;
    break;}
case 18:
#line 249 ".\\misc\\config_file\\cf_gramatical.yxx"
{ yyval.Val = cf_op (yyvsp[-2].Val, yyvsp[0].Val, OP_DIVIDE); ;
    break;}
case 19:
#line 252 ".\\misc\\config_file\\cf_gramatical.yxx"
{ yyval.Val = yyvsp[0].Val; ;
    break;}
case 20:
#line 253 ".\\misc\\config_file\\cf_gramatical.yxx"
{ cf_value v; v.Type=SIPBASE::CConfigFile::CVar::T_INT; /* just to avoid a warning, I affect 'v' with a dummy value */ yyval.Val = cf_op(yyvsp[0].Val,v,OP_NEG); ;
    break;}
case 21:
#line 254 ".\\misc\\config_file\\cf_gramatical.yxx"
{ yyval.Val = yyvsp[-1].Val; ;
    break;}
case 22:
#line 255 ".\\misc\\config_file\\cf_gramatical.yxx"
{ yyval.Val = yylval.Val; ;
    break;}
case 23:
#line 256 ".\\misc\\config_file\\cf_gramatical.yxx"
{ yyval.Val = yylval.Val; ;
    break;}
case 24:
#line 257 ".\\misc\\config_file\\cf_gramatical.yxx"
{ yyval.Val = yylval.Val; ;
    break;}
case 25:
#line 258 ".\\misc\\config_file\\cf_gramatical.yxx"
{ yyval.Val = yyvsp[0].Val; ;
    break;}
case 26:
#line 262 ".\\misc\\config_file\\cf_gramatical.yxx"
{
				DEBUG_PRINT("yacc: cont\n");
				bool ok=false;
				int i;
				for(i = 0; i < (int)((*((vector<SIPBASE::CConfigFile::CVar>*)(YYPARSE_PARAM))).size()); i++)
				{
					if ((*((vector<SIPBASE::CConfigFile::CVar>*)(YYPARSE_PARAM)))[i].Name == yyvsp[0].Val.String)
					{
						ok = true;
						break;
					}
				}
				if (ok)
				{
					cf_value Var;
					Var.Type = (*((vector<SIPBASE::CConfigFile::CVar>*)(YYPARSE_PARAM)))[i].Type;
					DEBUG_PRINTF("vart %d\n", Var.Type);
					switch (Var.Type)
					{
					case SIPBASE::CConfigFile::CVar::T_INT: Var.Int = (*((vector<SIPBASE::CConfigFile::CVar>*)(YYPARSE_PARAM)))[i].IntValues[0]; break;
					case SIPBASE::CConfigFile::CVar::T_REAL: Var.Real = (*((vector<SIPBASE::CConfigFile::CVar>*)(YYPARSE_PARAM)))[i].RealValues[0]; break;
					case SIPBASE::CConfigFile::CVar::T_STRING: strcpy (Var.String, (*((vector<SIPBASE::CConfigFile::CVar>*)(YYPARSE_PARAM)))[i].StrValues[0].c_str()); break;
					default: DEBUG_PRINT("*** CAN T DO THAT!!!\n"); break;
					}
					yyval.Val = Var;
				}
				else
				{
					DEBUG_PRINT("var existe pas\n");
				}
			;
    break;}
}

#line 705 "c:/program files/GnuWin32/share/bison/bison.simple"


  yyvsp -= yylen;
  yyssp -= yylen;
#if YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG
  if (yydebug)
    {
      short *yyssp1 = yyss - 1;
      YYFPRINTF (stderr, "state stack now");
      while (yyssp1 != yyssp)
	YYFPRINTF (stderr, " %d", *++yyssp1);
      YYFPRINTF (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;
#if YYLSP_NEEDED
  *++yylsp = yyloc;
#endif

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  char *yymsg;
	  int yyx, yycount;

	  yycount = 0;
	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  for (yyx = yyn < 0 ? -yyn : 0;
	       yyx < (int) (sizeof (yytname) / sizeof (char *)); yyx++)
	    if (yycheck[yyx + yyn] == yyx)
	      yysize += yystrlen (yytname[yyx]) + 15, yycount++;
	  yysize += yystrlen ("parse error, unexpected ") + 1;
	  yysize += yystrlen (yytname[YYTRANSLATE (yychar)]);
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "parse error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[YYTRANSLATE (yychar)]);

	      if (yycount < 5)
		{
		  yycount = 0;
		  for (yyx = yyn < 0 ? -yyn : 0;
		       yyx < (int) (sizeof (yytname) / sizeof (char *));
		       yyx++)
		    if (yycheck[yyx + yyn] == yyx)
		      {
			const char *yyq = ! yycount ? ", expecting " : " or ";
			yyp = yystpcpy (yyp, yyq);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yycount++;
		      }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    yyerror ("parse error; also virtual memory exhausted");
	}
      else
#endif /* defined (YYERROR_VERBOSE) */
	yyerror ("parse error");
    }
  goto yyerrlab1;


/*--------------------------------------------------.
| yyerrlab1 -- error raised explicitly by an action |
`--------------------------------------------------*/
yyerrlab1:
  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;
      YYDPRINTF ((stderr, "Discarding token %d (%s).\n",
		  yychar, yytname[yychar1]));
      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;


/*-------------------------------------------------------------------.
| yyerrdefault -- current state does not do anything special for the |
| error token.                                                       |
`-------------------------------------------------------------------*/
yyerrdefault:
#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */

  /* If its default is to accept any token, ok.  Otherwise pop it.  */
  yyn = yydefact[yystate];
  if (yyn)
    goto yydefault;
#endif


/*---------------------------------------------------------------.
| yyerrpop -- pop the current state because it cannot handle the |
| error token                                                    |
`---------------------------------------------------------------*/
yyerrpop:
  if (yyssp == yyss)
    YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#if YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG
  if (yydebug)
    {
      short *yyssp1 = yyss - 1;
      YYFPRINTF (stderr, "Error: state stack now");
      while (yyssp1 != yyssp)
	YYFPRINTF (stderr, " %d", *++yyssp1);
      YYFPRINTF (stderr, "\n");
    }
#endif

/*--------------.
| yyerrhandle.  |
`--------------*/
yyerrhandle:
  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;
#if YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

/*---------------------------------------------.
| yyoverflowab -- parser overflow comes here.  |
`---------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}
#line 294 ".\\misc\\config_file\\cf_gramatical.yxx"


/* compute the good operation with a, b and op */
cf_value cf_op (cf_value a, cf_value b, cf_operation op)
{
	DEBUG_PRINTF("[OP:%d; ", op);
	cf_print(a);
	DEBUG_PRINT("; ");
	cf_print(b);
	DEBUG_PRINT("; ");

	switch (op)
	{
	case OP_MULT:																//  *********************
		switch (a.Type)
		{
		case SIPBASE::CConfigFile::CVar::T_INT:
			switch (b.Type)
			{
			case SIPBASE::CConfigFile::CVar::T_INT:		a.Int *= b.Int; break;
			case SIPBASE::CConfigFile::CVar::T_REAL:	a.Int *= (int)b.Real; break;
			case SIPBASE::CConfigFile::CVar::T_STRING:	DEBUG_PRINT("ERROR: int*str\n"); break;
			default: break;
			}
			break;
		case SIPBASE::CConfigFile::CVar::T_REAL:
			switch (b.Type)
			{
			case SIPBASE::CConfigFile::CVar::T_INT:		a.Real *= (double)b.Int; break;
			case SIPBASE::CConfigFile::CVar::T_REAL:	a.Real *= b.Real; break;
			case SIPBASE::CConfigFile::CVar::T_STRING:	DEBUG_PRINT("ERROR: real*str\n"); break;
			default: break;
			}
			break;
		case SIPBASE::CConfigFile::CVar::T_STRING:
			switch (b.Type)
			{
			case SIPBASE::CConfigFile::CVar::T_INT:		DEBUG_PRINT("ERROR: str*int\n");  break;
			case SIPBASE::CConfigFile::CVar::T_REAL:	DEBUG_PRINT("ERROR: str*real\n");  break;
			case SIPBASE::CConfigFile::CVar::T_STRING:	DEBUG_PRINT("ERROR: str*str\n");  break;
			default: break;
			}
			break;
		default: break;
		}
		break;
	case OP_DIVIDE:																//  //////////////////////
		switch (a.Type)
		{
		case SIPBASE::CConfigFile::CVar::T_INT:
			switch (b.Type)
			{
			case SIPBASE::CConfigFile::CVar::T_INT:		a.Int /= b.Int; break;
			case SIPBASE::CConfigFile::CVar::T_REAL:	a.Int /= (int)b.Real; break;
			case SIPBASE::CConfigFile::CVar::T_STRING:	DEBUG_PRINT("ERROR: int/str\n"); break;
			default: break;
			}
			break;
		case SIPBASE::CConfigFile::CVar::T_REAL:
			switch (b.Type)
			{
			case SIPBASE::CConfigFile::CVar::T_INT:		a.Real /= (double)b.Int; break;
			case SIPBASE::CConfigFile::CVar::T_REAL:	a.Real /= b.Real; break;
			case SIPBASE::CConfigFile::CVar::T_STRING:	DEBUG_PRINT("ERROR: real/str\n"); break;
			default: break;
			}
			break;
		case SIPBASE::CConfigFile::CVar::T_STRING:
			switch (b.Type)
			{
			case SIPBASE::CConfigFile::CVar::T_INT:		DEBUG_PRINT("ERROR: str/int\n"); break;
			case SIPBASE::CConfigFile::CVar::T_REAL:	DEBUG_PRINT("ERROR: str/real\n"); break;
			case SIPBASE::CConfigFile::CVar::T_STRING:	DEBUG_PRINT("ERROR: str/str\n"); break;
			 default: break;
			}
			break;
		default: break;
		}
		break;
	case OP_PLUS:																//  ++++++++++++++++++++++++
		switch (a.Type)
		{
		case SIPBASE::CConfigFile::CVar::T_INT:
			switch (b.Type)
			{
			case SIPBASE::CConfigFile::CVar::T_INT:	a.Int += b.Int; break;
			case SIPBASE::CConfigFile::CVar::T_REAL:	a.Int += (int)b.Real; break;
			case SIPBASE::CConfigFile::CVar::T_STRING:	a.Int += atoi(b.String); break;
			default: break;
			}
			break;
		case SIPBASE::CConfigFile::CVar::T_REAL:
			switch (b.Type)
			{
			case SIPBASE::CConfigFile::CVar::T_INT:	a.Real += (double)b.Int; break;
			case SIPBASE::CConfigFile::CVar::T_REAL:	a.Real += b.Real; break;
			case SIPBASE::CConfigFile::CVar::T_STRING:	a.Real += atof (b.String); break;
			default: break;
			}
			break;
		case SIPBASE::CConfigFile::CVar::T_STRING:
			switch (b.Type)
			{
			case SIPBASE::CConfigFile::CVar::T_INT:	{ char str2[60]; SIPBASE::smprintf(str2, 60, "%d", b.Int); strcat(a.String, str2); break; }
			case SIPBASE::CConfigFile::CVar::T_REAL:	{ char str2[60]; SIPBASE::smprintf(str2, 60, "%f", b.Real); strcat(a.String, str2); break; }
			case SIPBASE::CConfigFile::CVar::T_STRING:	strcat (a.String, b.String); break;
			default: break;
			}
			break;
		default: break;
		}
		break;
	case OP_MINUS:																//  -------------------------
		switch (a.Type)
		{
		case SIPBASE::CConfigFile::CVar::T_INT:
			switch (b.Type)
			{
			case SIPBASE::CConfigFile::CVar::T_INT:	a.Int -= b.Int; break;
			case SIPBASE::CConfigFile::CVar::T_REAL:	a.Int -= (int)b.Real; break;
			case SIPBASE::CConfigFile::CVar::T_STRING:	DEBUG_PRINT("ERROR: int-str\n"); break;
			default: break;
			}
			break;
		case SIPBASE::CConfigFile::CVar::T_REAL:
			switch (b.Type)
			{
			case SIPBASE::CConfigFile::CVar::T_INT:	a.Real -= (double)b.Int; break;
			case SIPBASE::CConfigFile::CVar::T_REAL:	a.Real -= b.Real; break;
			case SIPBASE::CConfigFile::CVar::T_STRING:	DEBUG_PRINT("ERROR: real-str\n"); break;
			default: break;
			}
			break;
		case SIPBASE::CConfigFile::CVar::T_STRING:
			switch (b.Type)
			{
			case SIPBASE::CConfigFile::CVar::T_INT:	DEBUG_PRINT("ERROR: str-int\n"); break;
			case SIPBASE::CConfigFile::CVar::T_REAL:	DEBUG_PRINT("ERROR: str-real\n"); break;
			case SIPBASE::CConfigFile::CVar::T_STRING:	DEBUG_PRINT("ERROR: str-str\n"); break;
			default: break;
			}
			break;
		default: break;
		}
		break;
	case OP_NEG:																// neg
		switch (a.Type)
		{
		case SIPBASE::CConfigFile::CVar::T_INT:		a.Int = -a.Int; break;
		case SIPBASE::CConfigFile::CVar::T_REAL:		a.Real = -a.Real; break;
		case SIPBASE::CConfigFile::CVar::T_STRING:		DEBUG_PRINT("ERROR: -str\n"); break;
		default: break;
		}
		break;
	}
	cf_print(a);
	DEBUG_PRINT("]\n");
	return a;
}

/* print a value, it's only for debug purpose */
void cf_print (cf_value Val)
{
	switch (Val.Type)
	{
	case SIPBASE::CConfigFile::CVar::T_INT:
		DEBUG_PRINTF("'%d'", Val.Int);
		break;
	case SIPBASE::CConfigFile::CVar::T_REAL:
		DEBUG_PRINTF("`%f`", Val.Real);
		break;
	case SIPBASE::CConfigFile::CVar::T_STRING:
		DEBUG_PRINTF("\"%s\"", Val.String);
		break;
	default: break;
	}
}

/* put a value into a var */
void cf_setVar (SIPBASE::CConfigFile::CVar &Var, cf_value Val)
{
	DEBUG_PRINTF("Set var (type %d var name '%s') with new var type %d with value : ", Var.Type, Var.Name.c_str(), Val.Type);
	cf_print(Val);
	DEBUG_PRINTF("\n");
	Var.Root = LoadRoot;
	if (Var.Type == SIPBASE::CConfigFile::CVar::T_UNKNOWN || Var.Type == Val.Type)
	{
		if (Var.Type == SIPBASE::CConfigFile::CVar::T_UNKNOWN)
		{
			DEBUG_PRINTF("var type is unknown, set to the val type\n");
		}
		else
		{
			DEBUG_PRINTF("val type is same var type, just add\n");
		}

		Var.Type = Val.Type;
		switch (Val.Type)
		{
		case SIPBASE::CConfigFile::CVar::T_INT: Var.IntValues.push_back (Val.Int); break;
		case SIPBASE::CConfigFile::CVar::T_REAL: Var.RealValues.push_back (Val.Real); break;
		case SIPBASE::CConfigFile::CVar::T_STRING: Var.StrValues.push_back(Val.String); break;
		default: break;
		}
	}
	else
	{
		// need to convert the type
		switch (Var.Type)
		{
		case SIPBASE::CConfigFile::CVar::T_INT:
			switch (Val.Type)
			{
			case SIPBASE::CConfigFile::CVar::T_REAL:		Var.IntValues.push_back ((int)Val.Real); break;
			case SIPBASE::CConfigFile::CVar::T_STRING:	Var.IntValues.push_back (atoi(Val.String)); break;
			default: break;
			}
			break;
		case SIPBASE::CConfigFile::CVar::T_REAL:
			switch (Val.Type)
			{
			case SIPBASE::CConfigFile::CVar::T_INT:		Var.RealValues.push_back ((double)Val.Int); break;
			case SIPBASE::CConfigFile::CVar::T_STRING:	Var.RealValues.push_back (atof(Val.String)); break;
			default: break;
			}
			break;
		case SIPBASE::CConfigFile::CVar::T_STRING:
			switch (Val.Type)
			{
			case SIPBASE::CConfigFile::CVar::T_INT:	Var.StrValues.push_back(toStringA(Val.Int)); break;
			case SIPBASE::CConfigFile::CVar::T_REAL:	Var.StrValues.push_back(toStringA(Val.Real)); break;
			default: break;
			}
			break;
		default: break;
		}
	}
}

int yyerror (const char *s)
{
	DEBUG_PRINTF("%s\n",s);
	return 1;
}



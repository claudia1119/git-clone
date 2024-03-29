/*************************************************************************
*									 *
*	 YAP Prolog 							 *
*										 *
*	Yap Prolog was developed at NCCUP - Universidade do Porto	 *
*									 *
* Copyright L.Damas, V.S.Costa and Universidade do Porto 1985-1997	 *
*									 *
**************************************************************************
*									 *
* File:		iopreds.c						 *
* Last rev:	5/2/88							 *
* mods:									 *
* comments:	Input/Output C implemented predicates			 *
*									 *
*************************************************************************/
#ifdef SCCS
static char SccsId[] = "%W% %G%";
#endif

/*
* This file includes the definition of a miscellania of standard predicates
* for yap refering to: Files and GLOBAL_Streams, Simple Input/Output,
*
*/

#include "Yap.h"
#include "Yatom.h"
#include "YapHeap.h"
#include "YapFlags.h"
#include "yapio.h"
#include "eval.h"
#include "YapText.h"
#include <stdlib.h>
#if HAVE_STDARG_H
#include <stdarg.h>
#endif
#if HAVE_CTYPE_H
#include <ctype.h>
#endif
#if HAVE_WCTYPE_H
#include <wctype.h>
#endif
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#if HAVE_SYS_SELECT_H && !_MSC_VER && !defined(__MINGW32__)
#include <sys/select.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_SIGNAL_H
#include <signal.h>
#endif
#if HAVE_FCNTL_H
/* for O_BINARY and O_TEXT in WIN32 */
#include <fcntl.h>
#endif
#ifdef _WIN32
#if HAVE_IO_H
/* Windows */
#include <io.h>
#endif
#endif
#if !HAVE_STRNCAT
#define strncat(X, Y, Z) strcat(X, Y)
#endif
#if !HAVE_STRNCPY
#define strncpy(X, Y, Z) strcpy(X, Y)
#endif
#if _MSC_VER || defined(__MINGW32__)
#if HAVE_SOCKET
#include <winsock2.h>
#endif
#include <windows.h>
#ifndef S_ISDIR
#define S_ISDIR(x) (((x)&_S_IFDIR) == _S_IFDIR)
#endif
#endif
#include "iopreds.h"

#if _MSC_VER || defined(__MINGW32__)
#define SYSTEM_STAT _stat
#else
#define SYSTEM_STAT stat
#endif

static Term readFromBuffer(const char *s, Term opts);
 
static void clean_vars(VarEntry *p) {
  if (p == NULL)
    return;
  p->VarAdr = TermNil;
  clean_vars(p->VarLeft);
  clean_vars(p->VarRight);
}

#undef PAR

#ifdef O_QUASIQUOTATIONS
/** '$qq_open'(+QQRange, -Stream) is det.

Opens a quasi-quoted memory range.

@arg QQRange is a term '$quasi_quotation'(ReadData, Start, Length)
@arg Stream  is a UTF-8 encoded string, whose position indication
reflects the location in the real file.
*/

static Int qq_open(USES_REGS1) {
  PRED_LD

  Term t = Deref(ARG1);
  if (!IsVarTerm(t) && IsApplTerm(t) && FunctorOfTerm(t) =
          FunctorDQuasiQuotation) {
    void *ptr;
    char *start;
    size_t l int s;
    Term t0, t1, t2;

    if (IsPointerTerm((t0 = ArgOfTerm(1, t))) &&
        IsPointerTerm((t1 = ArgOfTerm(2, t))) &&
        IsIntegerTerm((t2 = ArgOfTerm(3, t)))) {
      ptr = PointerOfTerm(t0);
      start = PointerOfTerm(t1);
      len = IntegerOfTerm(t2);
      if ((s = Yap_open_buf_read_stream(start, len, ENC_UTF8, MEM_BUF_USER)) <
          0)
        return false;
      return Yap_unify(ARG2, Yap_MkStream(s));
    } else {
      Yap_Error(TYPE_ERROR_READ_CONTEXT, t);
    }

    return FALSE;
  }
}

static int parse_quasi_quotations(ReadData _PL_rd ARG_LD) {
  if (_PL_rd->qq_tail) {
    term_t av;
    int rc;

    if (!PL_unify_nil(_PL_rd->qq_tail))
      return FALSE;

    if (!_PL_rd->quasi_quotations) {
      if ((av = PL_new_term_refs(2)) && PL_put_term(av + 0, _PL_rd->qq) &&
#if __YAP_PROLOG__
          PL_put_atom(av + 1, YAP_SWIAtomFromAtom(_PL_rd->module->AtomOfME)) &&
#else
          PL_put_atom(av + 1, _PL_rd->module->name) &&
#endif
          PL_cons_functor_v(av, FUNCTOR_dparse_quasi_quotations2, av)) {
        term_t ex;
        rc = callProlog(MODULE_system, av + 0, PL_Q_CATCH_EXCEPTION, &ex);
        if (rc)
          return TRUE;
        _PL_rd->exception = ex;
        _PL_rd->has_exception = TRUE;
      }
      return FALSE;
    } else
      return TRUE;
  } else if (_PL_rd->quasi_quotations) /* user option, but no quotes */
  {
    return PL_unify_nil(_PL_rd->quasi_quotations);
  } else
    return TRUE;
}

#endif /*O_QUASIQUOTATIONS*/

#define READ_DEFS()                                                            \
  PAR("comments", list_filler, READ_COMMENTS),\
   PAR("module", isatom, READ_MODULE),  \
      PAR("priority", nat, READ_PRIORITY),                                     \
      PAR("quasi_quotations", filler, READ_QUASI_QUOTATIONS),                  \
      PAR("term_position", filler, READ_TERM_POSITION),                        \
      PAR("syntax_errors", isatom, READ_SYNTAX_ERRORS),                        \
      PAR("singletons", filler, READ_SINGLETONS),                              \
      PAR("variables", filler, READ_VARIABLES),                                \
      PAR("variable_names", filler, READ_VARIABLE_NAMES),                      \
      PAR("character_escapes", booleanFlag, READ_CHARACTER_ESCAPES),           \
      PAR("backquoted_string", isatom, READ_BACKQUOTED_STRING),                \
      PAR("cycles", ok, READ_CYCLES), PAR(NULL, ok, READ_END)

#define PAR(x, y, z) z

typedef enum open_enum_choices { READ_DEFS() } read_choices_t;

#undef PAR

#define PAR(x, y, z)                                                           \
  { x, y, z }

static const param_t read_defs[] = {READ_DEFS()};
#undef PAR

/**
* Syntax Error Handler
*
* @par tokptr: the sequence of tokens
* @par sno: the stream numbet
*
* Implicit arguments:
*    +
*/
Term Yap_syntax_error(TokEntry *errtok, int sno) {
  CACHE_REGS
  Term info;
  Term startline, errline, endline;
  Term tf[4];
  Term *tailp = tf + 3;
  CELL *Hi = HR;
  TokEntry *tok = LOCAL_tokptr;
  Int cline = tok->TokPos;

  startline = MkIntegerTerm(cline);
  if (errtok != LOCAL_toktide) {
    errtok = LOCAL_toktide;
  }
  LOCAL_Error_TYPE = YAP_NO_ERROR;
  errline = MkIntegerTerm(errtok->TokPos);
  if (LOCAL_ErrorMessage)
    tf[0] = MkStringTerm(LOCAL_ErrorMessage);
  else
    tf[0] = MkStringTerm("");
  while (tok) {
    Term ts[2];

    if (HR > ASP - 1024) {
      errline = MkIntegerTerm(0);
      endline = MkIntegerTerm(0);
      /* for some reason moving this earlier confuses gcc on solaris */
      HR = Hi;
      break;
    }
    if (tok->TokPos != cline) {
      *tailp = MkPairTerm(TermNewLine, TermNil);
      tailp = RepPair(*tailp) + 1;
      cline = tok->TokPos;
    }
    if (tok == errtok && tok->Tok != Error_tok) {
      *tailp = MkPairTerm(MkAtomTerm(AtomError), TermNil);
      tailp = RepPair(*tailp) + 1;
    }
    info = tok->TokInfo;
    switch (tok->Tok) {
    case Name_tok: {
      Term t0[1];
      if (info) {
        t0[0] = MkAtomTerm((Atom)info);
      } else {
        t0[0] = TermNil;
      }
      ts[0] = Yap_MkApplTerm(Yap_MkFunctor(AtomAtom, 1), 1, t0);
    } break;
    case QuasiQuotes_tok: {
      Term t0[2];
      t0[0] = MkAtomTerm(Yap_LookupAtom("<QQ>"));
      ts[0] = Yap_MkApplTerm(Yap_MkFunctor(AtomAtom, 1), 1, t0);
    } break;
    case WQuasiQuotes_tok: {
      Term t0[2];
      t0[0] = MkAtomTerm(Yap_LookupAtom("<WideQQ>"));
      ts[0] = Yap_MkApplTerm(Yap_MkFunctor(AtomAtom, 1), 1, t0);
    } break;
    case Number_tok:
      ts[0] = Yap_MkApplTerm(Yap_MkFunctor(AtomNumber, 1), 1, &(tok->TokInfo));
      break;
    case Var_tok: {
      Term t[2];
      VarEntry *varinfo = (VarEntry *)info;

      t[0] = MkIntTerm(0);
      t[1] = Yap_CharsToString(varinfo->VarRep, ENC_ISO_LATIN1 PASS_REGS);
      ts[0] = Yap_MkApplTerm(Yap_MkFunctor(AtomGVar, 2), 2, t);
    } break;
    case String_tok: {
      Term t0 =
          Yap_CharsToTDQ((char *)info, CurrentModule, ENC_ISO_LATIN1 PASS_REGS);
      if (!t0) {
        return 0;
      }
      ts[0] = Yap_MkApplTerm(Yap_MkFunctor(AtomString, 1), 1, &t0);
    } break;
    case WString_tok: {
      Term t0 = Yap_WCharsToTDQ((wchar_t *)info, CurrentModule PASS_REGS);
      if (!t0)
        return 0;
      ts[0] = Yap_MkApplTerm(Yap_MkFunctor(AtomString, 1), 1, &t0);
    } break;
    case BQString_tok: {
      Term t0 =
          Yap_CharsToTBQ((char *)info, CurrentModule, ENC_ISO_LATIN1 PASS_REGS);
      ts[0] = Yap_MkApplTerm(Yap_MkFunctor(AtomString, 1), 1, &t0);
    } break;
    case WBQString_tok: {
      Term t0 = Yap_WCharsToTBQ((wchar_t *)info, CurrentModule PASS_REGS);
      ts[0] = Yap_MkApplTerm(Yap_MkFunctor(AtomString, 1), 1, &t0);
    } break;
    case Error_tok: {
      ts[0] = MkAtomTerm(AtomError);
    } break;
    case eot_tok:
      endline = MkIntegerTerm(tok->TokPos);
      ts[0] = MkAtomTerm(Yap_LookupAtom("EOT"));

      break;
    case Ponctuation_tok: {
      char s[2];
      s[1] = '\0';
      if ((info) == 'l') {
        s[0] = '(';
      } else {
        s[0] = (char)info;
      }
      ts[0] = MkAtomTerm(Yap_LookupAtom(s));
    }
    }
    tok = tok->TokNext;
    if (!tok)
      break;
    *tailp = MkPairTerm(ts[0], TermNil);
    tailp = RepPair(*tailp) + 1;
  }
  {
    Term t[3];
    t[0] = startline;
    t[1] = errline;
    t[2] = endline;
    tf[1] = Yap_MkApplTerm(Yap_MkFunctor(AtomBetween, 3), 3, t);
  }
  /* 0: id */
  /* 1: strat, error, end line */
  /*2 msg */
  /* file */
  tf[2] = Yap_StreamUserName(sno);
  clean_vars(LOCAL_VarTable);
  clean_vars(LOCAL_AnonVarTable);
  Term terr = Yap_MkApplTerm(FunctorSyntaxError, 4, tf);
  Term tn[2];
  tn[0] = Yap_MkApplTerm(FunctorShortSyntaxError, 1, &terr);
  tn[1] = TermNil;
  terr = Yap_MkApplTerm(FunctorError, 2, tn);
#if DEBUG
  if (Yap_ExecutionMode == YAP_BOOT_MODE) {
    fprintf(stderr, "SYNTAX ERROR while booting: ");
    Yap_DebugPlWriteln(terr);
  }
#endif
  return terr;
}

typedef struct FEnv {
  Term qq, tp, sp, np, vp, ce;
  Term tpos;           /// initial position of the term to be read.
  Term t;              /// the output term
  TokEntry *tokstart;  /// the token list
  TokEntry *toklast;   /// the last token
  CELL *old_H;         /// initial H, will be reset on stack overflow.
  tr_fr_ptr old_TR;    /// initial TR
  xarg *args;          /// input args
  bool reading_clause; /// read_clause
  size_t nargs;        /// arity of current procedure
  encoding_t enc;      /// encoding of the stream being read
  Term tcomms;         /// Access to comments
  Term cmod;           /// Access to comments
} FEnv;

typedef struct renv {
  Term bq;
  bool ce, sw;
  Term sy;
  UInt cpos;
#if HAVE_FGETPOS
  fpos_t rpos;
#endif
  int prio;
  int ungetc_oldc;
  int had_ungetc;
  bool seekable;
} REnv;

static xarg *setClauseReadEnv(Term opts, FEnv *fe, struct renv *re,
                              int inp_stream);
static xarg *setReadEnv(Term opts, FEnv *fe, struct renv *re, int inp_stream) {
  CACHE_REGS
  LOCAL_VarTable = NULL;
  LOCAL_AnonVarTable = NULL;
  fe->cmod = CurrentModule;
  fe->enc = GLOBAL_Stream[inp_stream].encoding;
  xarg *args = Yap_ArgListToVector(opts, read_defs, READ_END);
  if (args == NULL) {
    return NULL;
  }

  re->bq = getBackQuotesFlag();
  if (args[READ_MODULE].used) {
    CurrentModule = args[READ_MODULE].tvalue;
  }
  if (args[READ_BACKQUOTED_STRING].used) {
    if (!setBackQuotesFlag(args[READ_BACKQUOTED_STRING].tvalue))
      return false;
  }
  if (args[READ_QUASI_QUOTATIONS].used) {
    fe->qq = args[READ_QUASI_QUOTATIONS].tvalue;
  } else {
    fe->qq = 0;
  }
  if (args[READ_COMMENTS].used) {
    fe->tcomms = args[READ_COMMENTS].tvalue;
    if (fe->tcomms == TermProlog)
      fe->tcomms = PROLOG_MODULE;
  } else {
    fe->tcomms = 0;
  }
  if (args[READ_TERM_POSITION].used) {
    fe->tp = args[READ_TERM_POSITION].tvalue;
  } else {
    fe->tp = 0;
  }
  if (args[READ_SINGLETONS].used) {
    fe->sp = args[READ_SINGLETONS].tvalue;
  } else {
    fe->sp = 0;
  }
  if (args[READ_SYNTAX_ERRORS].used) {
    re->sy = args[READ_SYNTAX_ERRORS].tvalue;
  } else {
    re->sy = TermError; // getYapFlag( MkAtomTerm(AtomSyntaxErrors) );
  }
  if (args[READ_VARIABLES].used) {
    fe->vp = args[READ_VARIABLES].tvalue;
  } else {
    fe->vp = 0;
  }
  if (args[READ_VARIABLE_NAMES].used) {
    fe->np = args[READ_VARIABLE_NAMES].tvalue;
  } else {
    fe->np = 0;
  }
  if (args[READ_CHARACTER_ESCAPES].used ||
      Yap_CharacterEscapes(CurrentModule)) {
    fe->ce = true;
  } else {
    fe->ce = false;
  }
  re->seekable = (GLOBAL_Stream[inp_stream].status & Seekable_Stream_f) != 0;
  if (re->seekable) {
#if HAVE_FGETPOS
    fgetpos(GLOBAL_Stream[inp_stream].file, &re->rpos);
#else
    re->cpos = GLOBAL_Stream[inp_stream].charcount;
#endif
  }
  if (args[READ_PRIORITY].used) {
    re->prio = IntegerOfTerm(args[READ_PRIORITY].tvalue);
    if (re->prio > GLOBAL_MaxPriority) {
      Yap_Error(DOMAIN_ERROR_OPERATOR_PRIORITY, opts,
                "max priority in Prolog is %d, not %ld", GLOBAL_MaxPriority,
                re->prio);
    }
  } else {
    re->prio = LOCAL_default_priority;
  }
  return args;
}

typedef enum {
  YAP_START_PARSING,  /// initialization
  YAP_SCANNING,       /// input to list of tokens
  YAP_SCANNING_ERROR, /// serious error (eg oom); trying error handling, followd
                      /// by either restart or failure
  YAP_PARSING,        /// list of tokens to term
  YAP_PARSING_ERROR,  /// oom or syntax error
  YAP_PARSING_FINISHED /// exit parser
} parser_state_t;

Int Yap_FirstLineInParse(void) {
  CACHE_REGS
  return LOCAL_StartLineCount;
}

#define PUSHFET(X) *HR++ = fe->X
#define POPFET(X) fe->X = *--HR

static void reset_regs(TokEntry *tokstart, FEnv *fe) {
  CACHE_REGS

  restore_machine_regs();

  /* restart global */
  PUSHFET(qq);
  PUSHFET(tp);
  PUSHFET(sp);
  PUSHFET(np);
  PUSHFET(vp);
  PUSHFET(tpos);
  PUSHFET(t);
  HR = fe->old_H;
  TR = (tr_fr_ptr)LOCAL_ScannerStack;
  LOCAL_Error_TYPE = YAP_NO_ERROR;
  Yap_growstack_in_parser(&fe->old_TR, &tokstart, &LOCAL_VarTable);
  LOCAL_ScannerStack = (char *)TR;
  TR = fe->old_TR;
  POPFET(t);
  POPFET(tpos);
  POPFET(vp);
  POPFET(np);
  POPFET(sp);
  POPFET(tp);
  POPFET(qq);
}

static Term
get_variables(FEnv *fe, TokEntry *tokstart)
{
    Term v;
  if (fe->vp) {
    while (true) {
      fe->old_H = HR;

      if (setjmp(LOCAL_IOBotch) == 0) {
        if ((v = Yap_Variables(LOCAL_VarTable, TermNil))) {
          fe->old_H = HR;
          return v;
        }
      } else {
        reset_regs(tokstart, fe);
      }
    }
  } 
  return 0;   
}


static Term
get_varnames(FEnv *fe, TokEntry *tokstart)
{
    Term v;
  if (fe->np) {
    while (true) {
      fe->old_H = HR;

      if (setjmp(LOCAL_IOBotch) == 0) {
        if ((v = Yap_VarNames(LOCAL_VarTable, TermNil))) {
          fe->old_H = HR;
          return v;
        }
      } else {
        reset_regs(tokstart, fe);
      }
    }
  } 
  return 0;   
}


static Term
get_singletons(FEnv *fe, TokEntry *tokstart)
{
    Term v;
  if (fe->sp) {
    while (TRUE) {
      fe->old_H = HR;

      if (setjmp(LOCAL_IOBotch) == 0) {
        if ((v = Yap_Singletons(LOCAL_VarTable, TermNil)))
          return v;
      } else {
        reset_regs(tokstart, fe);
      }
    }
  }
  return 0;   
}

static void
warn_singletons(FEnv *fe, TokEntry *tokstart)
{
    Term v;
   fe->sp = TermNil;
   v = get_singletons(fe,  tokstart);
   if (v && v != TermNil) {
     Term singls[4];
     singls[0] = Yap_MkApplTerm(Yap_MkFunctor(AtomSingleton, 1), 1, &v);
     singls[1] = MkIntegerTerm(LOCAL_SourceFileLineno);
     singls[2] = MkAtomTerm(LOCAL_SourceFileName);
     if (fe->t)
       singls[3] =  fe->t;
     else
       singls[1] = TermTrue;
     Term t = Yap_MkApplTerm(Yap_MkFunctor(AtomStyleCheck, 4), 4, singls);
     singls[0] = Yap_MkApplTerm(Yap_MkFunctor(AtomStyleCheck, 1), 1, &t);
     
     singls[1] = v;
     Yap_PrintWarning(Yap_MkApplTerm(FunctorError, 2, singls));
   }
}
  

static Term
get_stream_position(FEnv *fe, TokEntry *tokstart)
{
    Term v;
  if (fe->tp) {
    while (true) {
      fe->old_H = HR;

      if (setjmp(LOCAL_IOBotch) == 0) {
        if ((v = CurrentPositionToTerm()))
          return v;
      } else {
        reset_regs(tokstart, fe);
      }
    }
  }
  return 0;   
}



static bool complete_processing(FEnv *fe, TokEntry *tokstart) {
  CACHE_REGS
    Term v1, v2, v3, vc, tp;

  CurrentModule = fe->cmod;
  if (CurrentModule == TermProlog)
    CurrentModule = PROLOG_MODULE;
  if (fe->t && fe->vp)
    v1 = get_variables(fe, tokstart);
  else
    v1 = 0L;
  if (fe->t && fe->np)
    v2 = get_varnames(fe, tokstart);
  else
    v2 = 0L;
  if (fe->t && fe->sp)
    v3 = get_singletons(fe, tokstart);
  else
    v3 = 0L;
  if (fe->t && fe->tcomms)
    vc = LOCAL_Comments;
  else
    vc = 0L;
  if (fe->t && fe->tp)
    tp = get_stream_position(fe, tokstart );
  else
    tp = 0L;
  Yap_clean_tokenizer(tokstart, LOCAL_VarTable, LOCAL_AnonVarTable);

  // trail must be ok by now.]
  if ( ( !v1 || Yap_unify(v1, fe->vp) ) &&
       ( !v2 || Yap_unify(v2, fe->np))  &&
       ( !v3 || Yap_unify(v3, fe->sp))  &&
       ( !tp || Yap_unify(tp, fe->tp))  &&
       ( !vc || Yap_unify(vc, fe->tcomms)))
    return fe->t;
  return 0;
}

static bool complete_clause_processing(FEnv *fe, TokEntry
                                        *tokstart) {
  CACHE_REGS
    Term v_vp, v_vnames, v_comments, v_pos;

  CurrentModule = fe->cmod;
  if (CurrentModule == TermProlog)
    CurrentModule = PROLOG_MODULE;
  if (fe->t && fe->vp)
    v_vp = get_variables(fe, tokstart);
  else
    v_vp = 0L;
  if (fe->t && fe->np)
    v_vnames = get_varnames(fe, tokstart);
  else
    v_vnames = 0L;
  if (fe->t && trueLocalPrologFlag(SINGLE_VAR_WARNINGS_FLAG)) {
    warn_singletons(fe, tokstart);
  }
  if (fe->t && fe->tcomms)
    v_comments = LOCAL_Comments;
  else
    v_comments = 0L;
  if (fe->t && fe->tp)
    v_pos = get_stream_position(fe, tokstart );
  else
    v_pos = 0L;
  Yap_clean_tokenizer(tokstart, LOCAL_VarTable, LOCAL_AnonVarTable);

 // trail must be ok by now.]
  if ( ( !v_vp || Yap_unify(v_vp, fe->vp) ) &&
       ( !v_vnames || Yap_unify(v_vnames, fe->np))  &&
       ( !v_pos || Yap_unify(v_pos, fe->tp))  &&
       ( !v_comments || Yap_unify(v_comments, fe->tcomms)))
     return fe->t;
  return 0;
}

static parser_state_t initParser(Term opts, FEnv *fe, REnv *re, int inp_stream,
                                 int nargs);

static parser_state_t parse(REnv *re, FEnv *fe, int inp_stream);

static parser_state_t scanError(REnv *re, FEnv *fe, int inp_stream);

static parser_state_t scanEOF(FEnv *fe, int inp_stream);

static parser_state_t scan(REnv *re, FEnv *fe, int inp_stream);

static parser_state_t scanEOF(FEnv *fe, int inp_stream) {
  CACHE_REGS
  // bool store_comments = false;
  TokEntry *tokstart = LOCAL_tokptr;
  // check for an user abort
  if (tokstart != NULL && tokstart->Tok != Ord(eot_tok)) {
    /* we got the end of file from an abort */
    if (LOCAL_ErrorMessage && !strcmp(LOCAL_ErrorMessage, "Abort")) {
      fe->t = 0L;
      Yap_clean_tokenizer(tokstart, LOCAL_VarTable, LOCAL_AnonVarTable);
      return YAP_PARSING_FINISHED;
    }
    // a :- <eof>
    /* we need to force the next read to also give end of file.*/
    GLOBAL_Stream[inp_stream].status |= Push_Eof_Stream_f;
    LOCAL_ErrorMessage = "end of file found before end of term";
    return YAP_PARSING;
  } else {
    // <eof>
    // return end_of_file
    TR = (tr_fr_ptr)tokstart;
    Yap_clean_tokenizer(tokstart, LOCAL_VarTable, LOCAL_AnonVarTable);
    fe->t = MkAtomTerm(AtomEof);
    if (fe->np && !Yap_unify(TermNil, fe->np))
      fe->t = 0;
    if (fe->sp && !Yap_unify(TermNil, fe->sp))
      fe->t = 0;
    if (fe->vp && !Yap_unify(TermNil, fe->vp))
      fe->t = 0;
    if (fe->tp && !Yap_unify(fe->tp, fe->tpos))
      fe->t = 0;
    post_process_eof(GLOBAL_Stream + inp_stream);
#if DEBUG
    if (GLOBAL_Option['p' - 'a' + 1]) {
      fprintf(stderr, "[ end_of_file %p ]\n", GLOBAL_Stream[inp_stream].name);
    }
#endif
    return YAP_PARSING_FINISHED;
  }
}

static parser_state_t initParser(Term opts, FEnv *fe, REnv *re, int inp_stream,
                                 int nargs) {
  CACHE_REGS
  LOCAL_ErrorMessage = NULL;
  fe->old_TR = TR;
  LOCAL_Error_TYPE = YAP_NO_ERROR;
  LOCAL_SourceFileName = GLOBAL_Stream[inp_stream].name;
  LOCAL_eot_before_eof = false;
  fe->tpos = StreamPosition(inp_stream);
  fe->old_H = HR;
  fe->reading_clause = nargs < 0;
  if (fe->reading_clause) {
    fe->nargs = -nargs;
    fe->args = setClauseReadEnv(opts, fe, re, inp_stream);
  } else {
    fe->nargs = nargs;
    fe->args = setReadEnv(opts, fe, re, inp_stream);
  }
  if (fe->args == NULL) {
    if (LOCAL_Error_TYPE == DOMAIN_ERROR_OUT_OF_RANGE)
      LOCAL_Error_TYPE = DOMAIN_ERROR_READ_OPTION;
    if (LOCAL_Error_TYPE)
      Yap_Error(LOCAL_Error_TYPE, LOCAL_Error_Term, NULL);
    fe->t = 0;
    return YAP_PARSING_FINISHED;
    ;
  }
  if (GLOBAL_Stream[inp_stream].status & Push_Eof_Stream_f) {
    fe->t = MkAtomTerm(AtomEof);
    GLOBAL_Stream[inp_stream].status &= ~Push_Eof_Stream_f;
    return YAP_PARSING_FINISHED;
  }
  if (!fe->args) {
    return YAP_PARSING_FINISHED;
  }
  return YAP_SCANNING;
}

static parser_state_t scan(REnv *re, FEnv *fe, int inp_stream) {
  CACHE_REGS
  /* preserve   value of H after scanning: otherwise we may lose strings
      and floats */
  LOCAL_tokptr = LOCAL_toktide =

      Yap_tokenizer(GLOBAL_Stream + inp_stream, false, &fe->tpos);
  if (LOCAL_ErrorMessage)
    return YAP_SCANNING_ERROR;
  if (LOCAL_tokptr->Tok != Ord(eot_tok)) {
    // next step
    return YAP_PARSING;
  }
  if (LOCAL_tokptr->Tok == eot_tok && LOCAL_tokptr->TokInfo == TermNl) {
    char *out = malloc( strlen("Empty clause" + 1 ) );
    strcpy( out, "Empty clause" );
    LOCAL_ErrorMessage = out;
    LOCAL_Error_TYPE = SYNTAX_ERROR;
    LOCAL_Error_Term = TermEof;
    return YAP_PARSING_ERROR;
  }
  return scanEOF(fe, inp_stream);
}

static parser_state_t scanError(REnv *re, FEnv *fe, int inp_stream) {
  CACHE_REGS
  fe->t = 0;
  // running out of memory
  if (LOCAL_Error_TYPE == RESOURCE_ERROR_TRAIL) {
    LOCAL_Error_TYPE = YAP_NO_ERROR;
    if (!Yap_growtrail(sizeof(CELL) * K16, FALSE)) {
      return YAP_PARSING_FINISHED;
    }
  } else if (LOCAL_Error_TYPE == RESOURCE_ERROR_AUXILIARY_STACK) {
    LOCAL_Error_TYPE = YAP_NO_ERROR;
    if (!Yap_ExpandPreAllocCodeSpace(0, NULL, TRUE)) {
      return YAP_PARSING_FINISHED;
    }
  } else if (LOCAL_Error_TYPE == RESOURCE_ERROR_HEAP) {
    LOCAL_Error_TYPE = YAP_NO_ERROR;
    if (!Yap_growheap(FALSE, 0, NULL)) {
      return YAP_PARSING_FINISHED;
    }
  } else if (LOCAL_Error_TYPE == RESOURCE_ERROR_STACK) {
    LOCAL_Error_TYPE = YAP_NO_ERROR;
    if (!Yap_gcl(LOCAL_Error_Size, fe->nargs, ENV, CP)) {
      return YAP_PARSING_FINISHED;
    }
  }
  // go back to the start
  if (re->seekable) {
    if (GLOBAL_Stream[inp_stream].status & InMemory_Stream_f) {
      GLOBAL_Stream[inp_stream].u.mem_string.pos = re->cpos;
    } else if (GLOBAL_Stream[inp_stream].status) {
#if HAVE_FGETPOS
      fsetpos(GLOBAL_Stream[inp_stream].file, &re->rpos);
#else
      fseek(GLOBAL_Stream[inp_stream].file, re->cpos, 0L);
#endif
    }
  }
  return YAP_SCANNING;
}

static parser_state_t parseError(REnv *re, FEnv *fe, int inp_stream) {
  CACHE_REGS
  fe->t = 0;
  if (LOCAL_Error_TYPE == RESOURCE_ERROR_TRAIL ||
      LOCAL_Error_TYPE == RESOURCE_ERROR_AUXILIARY_STACK ||
      LOCAL_Error_TYPE == RESOURCE_ERROR_HEAP ||
      LOCAL_Error_TYPE == RESOURCE_ERROR_STACK) {
    return YAP_SCANNING_ERROR;
  }
  Term ParserErrorStyle = re->sy;
  if (ParserErrorStyle == TermQuiet) {
    /* just fail */
    LOCAL_Error_TYPE = YAP_NO_ERROR;
    return YAP_PARSING_FINISHED;
  } else {
    Term terr = Yap_syntax_error(fe->toklast, inp_stream);
    if (ParserErrorStyle == TermError) {
      LOCAL_ErrorMessage = NULL;
      LOCAL_Error_TYPE = SYNTAX_ERROR;
      Yap_Error(SYNTAX_ERROR, terr, LOCAL_ErrorMessage);
      return YAP_PARSING_FINISHED;
    } else {
      LOCAL_Error_TYPE = YAP_NO_ERROR;
      if (ParserErrorStyle == TermDec10) {
        if (Yap_PrintWarning(terr))
          return YAP_SCANNING;
        return YAP_PARSING_FINISHED;
      }
    }
  }
  LOCAL_Error_TYPE = YAP_NO_ERROR;
  return YAP_PARSING_FINISHED;
}

static parser_state_t parse(REnv *re, FEnv *fe, int inp_stream) {
  CACHE_REGS
  TokEntry *tokstart = LOCAL_tokptr;
  encoding_t e = LOCAL_encoding;
  LOCAL_encoding = fe->enc;
  fe->t = Yap_Parse(re->prio);
  LOCAL_encoding = e;
  fe->toklast = LOCAL_tokptr;
  LOCAL_tokptr = tokstart;
  TR = (tr_fr_ptr)tokstart;
#if EMACS
  first_char = tokstart->TokPos;
#endif /* EMACS */
  return YAP_PARSING_FINISHED;
}

/**
 * @brief generic routine to read terms from a stream
 *
 *
 * @arg inp_stream: where we read from
 * @arg: opts, a list with options
 * @arg: if called from read_term, arity
 *  called from read_clause, -arity
 *
 * @return the term or 0 in case of error.
 *
 * Implementation uses a state machine: default is init, scan, parse, complete.
 *
 *
 */
Term Yap_read_term(int inp_stream, Term opts, int nargs) {
  FEnv fe;
  REnv re;
#if EMACS
  int emacs_cares = FALSE;
#endif

  parser_state_t state = YAP_START_PARSING;
  while (true) {
    switch (state) {
    case YAP_START_PARSING:
      state = initParser(opts, &fe, &re, inp_stream, nargs);
      break;
    case YAP_SCANNING:
      state = scan(&re, &fe, inp_stream);
      break;
    case YAP_SCANNING_ERROR:
      state = scanError(&re, &fe, inp_stream);
      break;
    case YAP_PARSING:
      state = parse(&re, &fe, inp_stream);
      break;
    case YAP_PARSING_ERROR:
      state = parseError(&re, &fe, inp_stream);
      break;
    case YAP_PARSING_FINISHED:
      {
	CACHE_REGS
	  bool done;
	if (fe.reading_clause)
	  done = complete_clause_processing(&fe, LOCAL_tokptr);
	else
	  done = complete_processing(&fe, LOCAL_tokptr);
	if (!done) {
	  state = YAP_PARSING_ERROR;
	  fe.t = 0;
	  break;
	}
#if EMACS
      first_char = tokstart->TokPos;
#endif /* EMACS */
      return fe.t;
      }
    }
  }
  return 0;
}

static Int
    read_term2(USES_REGS1) { /* '$read'(+Flag,?Term,?Module,?Vars,-Pos,-Err) */
  Term rc;
  yhandle_t h = Yap_PushHandle(ARG1);
  if ((rc = Yap_read_term(LOCAL_c_input_stream, ARG2, 2)) == 0)
    return FALSE;
  Term tf = Yap_PopHandle(h);
  return rc && Yap_unify(tf, rc);
}

static Int read_term(
    USES_REGS1) { /* '$read2'(+Flag,?Term,?Module,?Vars,-Pos,-Err,+Stream)  */
  int inp_stream;
  Int out;

  /* needs to change LOCAL_output_stream for write */
  yhandle_t h = Yap_PushHandle(ARG2);
  inp_stream = Yap_CheckTextStream(ARG1, Input_Stream_f, "read/3");
  if (inp_stream == -1) {
    return (FALSE);

  }
  out = Yap_read_term(inp_stream, ARG3, 3);
  UNLOCK(GLOBAL_Stream[inp_stream].streamlock);
  Term tf = Yap_PopHandle(h);
  return out != 0L && Yap_unify(tf, out);
}

#define READ_CLAUSE_DEFS()                                                     \
      PAR("comments", list_filler, READ_CLAUSE_COMMENTS),      \
      PAR("module", isatom, READ_CLAUSE_MODULE),                               \
      PAR("variable_names", filler, READ_CLAUSE_VARIABLE_NAMES),               \
      PAR("variables", filler, READ_CLAUSE_VARIABLES),               \
      PAR("term_position", filler, READ_CLAUSE_TERM_POSITION),                 \
      PAR("syntax_errors", isatom, READ_CLAUSE_SYNTAX_ERRORS),                 \
      PAR(NULL, ok, READ_CLAUSE_END)

#define PAR(x, y, z) z

typedef enum read_clause_enum_choices {
  READ_CLAUSE_DEFS()
} read_clause_choices_t;

#undef PAR

#define PAR(x, y, z)                                                           \
  { x, y, z }

static const param_t read_clause_defs[] = {READ_CLAUSE_DEFS()};
#undef PAR

static xarg *setClauseReadEnv(Term opts, FEnv *fe, struct renv *re,
                              int inp_stream) {
  CACHE_REGS

  xarg *args = Yap_ArgListToVector(opts, read_clause_defs, READ_END);
  if (args == NULL) {
    return NULL;
  }
  re->bq = getBackQuotesFlag();
  fe->enc = GLOBAL_Stream[inp_stream].encoding;
  fe->cmod = CurrentModule;
  CurrentModule = LOCAL_SourceModule;
  if (CurrentModule == TermProlog)
    CurrentModule = PROLOG_MODULE;
  if (args[READ_CLAUSE_MODULE].used) {
    fe->tcomms = args[READ_CLAUSE_MODULE].tvalue;
  } else {
    fe->tcomms = 0L;
  }
   fe->sp = 0;
  fe->qq = 0;
  if (args[READ_CLAUSE_TERM_POSITION].used) {
    fe->tp = args[READ_CLAUSE_TERM_POSITION].tvalue;
  } else {
    fe->tp = 0;
  }
  fe->sp = 0;
  if (args[READ_CLAUSE_COMMENTS].used) {
    fe->tcomms = args[READ_CLAUSE_COMMENTS].tvalue;
    if (fe->tcomms == TermProlog)
      fe->tcomms = PROLOG_MODULE;
  } else {
    fe->tcomms = 0L;
  }
  if (args[READ_CLAUSE_SYNTAX_ERRORS].used) {
    re->sy = args[READ_CLAUSE_SYNTAX_ERRORS].tvalue;
  } else {
    re->sy = TermDec10;
  }
  fe->vp = 0;
  if (args[READ_CLAUSE_VARIABLE_NAMES].used) {
    fe->np = args[READ_CLAUSE_VARIABLE_NAMES].tvalue;
  } else {
    fe->np = 0;
  }
  if (args[READ_CLAUSE_VARIABLES].used) {
    fe->vp = args[READ_CLAUSE_VARIABLES].tvalue;
  } else {
    fe->vp = 0;
  }
  fe->ce = Yap_CharacterEscapes(CurrentModule);
  re->seekable = (GLOBAL_Stream[inp_stream].status & Seekable_Stream_f) != 0;
  if (re->seekable) {
#if HAVE_FGETPOS
    fgetpos(GLOBAL_Stream[inp_stream].file, &re->rpos);
#else
    re->cpos = GLOBAL_Stream[inp_stream].charcount;
#endif
  }
  re->prio = LOCAL_default_priority;
  return args;
}

/**
* @pred read_clause( +_Stream_, -_Clause_, ?_Opts) is det
*
 u* Same as read_clause/3, but from the standard input stream.
*
*/
static Int read_clause2(USES_REGS1) {
  Term rc;
  yhandle_t h = Yap_InitSlot(ARG1);
  rc = Yap_read_term(LOCAL_c_input_stream, Deref(ARG2), 2);
  Term tf = Yap_GetFromSlot(h);
  Yap_RecoverSlots(1, h);
  return rc && Yap_unify(tf, rc);
}


/**
* @pred read_clause( +_Stream_, -_Clause_, ?_Opts) is det
*
* This predicate receives a set of options _OPts_ based on read_term/3, but
*specific
* to readin clauses. The following options are considered:
*
*   + The `comments` option unifies its argument with the comments in the term,
*     represented as strings
*   + The `process_comments` option calls a hook, it is current ignored by YAP.
*   + The `term_position` unifies its argument with a term describing the
*     position of the term.
*   + The `syntax_errors` flag controls response to syntactic errors, the
*default is `dec10`.
*
* The next two options are called implicitly:
*
*   + The `module` option is initialized to the current source module, by
*default.
*   + The `singletons` option is set from the single var flag
*/
static Int read_clause(
    USES_REGS1) { /* '$read2'(+Flag,?Term,?Module,?Vars,-Pos,-Err,+Stream)  */
  int inp_stream;
  Int out;
  Term t3 = Deref(ARG3);
  yhandle_t h = Yap_InitSlot(ARG2);
  /* needs to change LOCAL_output_stream for write */
  inp_stream = Yap_CheckTextStream(ARG1, Input_Stream_f, "read/3");
  if (inp_stream < 0)
    return false;
  out = Yap_read_term(inp_stream, t3, -3);
  UNLOCK(GLOBAL_Stream[inp_stream].streamlock);
  Term tf = Yap_GetFromSlot(h);
  Yap_RecoverSlots(1, h);
  return out && Yap_unify(tf, out);
}


/**
* @pred source_location( - _File_ , _Line_ )
*
* unify  _File_ and  _Line_ wuth the position of the last term read, if the term
* comes from a stream created by opening a file-system path with open/3 and
*friends.>position
* It ignores user_input or
* sockets.
*
* @param - _File_
* @param - _Line_
*
* @note SWI-Prolog built-in.
*/
static Int source_location(USES_REGS1) {
  return Yap_unify(ARG1, MkAtomTerm(LOCAL_SourceFileName)) &&
         Yap_unify(ARG2, MkIntegerTerm(LOCAL_SourceFileLineno));
}

/**
* @pred read(+ _Stream_, - _Term_ ) is iso
*
* Reads term  _T_ from the stream  _S_ instead of from the current input
* stream.
*
* @param - _Stream_
* @param - _Term_
*
*/
static Int read2(
    USES_REGS1) { /* '$read2'(+Flag,?Term,?Module,?Vars,-Pos,-Err,+Stream)  */
  int inp_stream;
  Int out;

  /* needs to change LOCAL_output_stream for write */
  inp_stream = Yap_CheckTextStream(ARG1, Input_Stream_f, "read/3");
  if (inp_stream == -1) {
    return (FALSE);
  }
  out = Yap_read_term(inp_stream, TermNil, 1);
  UNLOCK(GLOBAL_Stream[inp_stream].streamlock);
  return out && Yap_unify(ARG2, out);
}

/** @pred  read(- _T_) is iso

Reads the next term from the current input stream, and unifies it with
_T_. The term must be followed by a dot (`.`) and any blank-character
as previously defined. The syntax of the term must match the current
declarations for operators (see op). If the end-of-stream is reached,
_T_ is unified with the atom `end_of_file`. Further reads from of
the same stream may cause an error failure (see open/3).

*/
static Int read1(
    USES_REGS1) { /* '$read2'(+Flag,?Term,?Module,?Vars,-Pos,-Err,+Stream)  */
  Term out = Yap_read_term(LOCAL_c_input_stream, TermNil, 1);
  return out && Yap_unify(ARG1, out);
}

/**  @pred fileerrors

Switches on the file_errors flag so that in certain error conditions
Input/Output predicates will produce an appropriated message and abort.

*/
static Int fileerrors(USES_REGS1) {
  return setYapFlag(TermFileErrors, TermTrue);
}

/**
@pred  nofileerrors

Switches off the `file_errors` flag, so that the predicates see/1,
tell/1, open/3 and close/1 just fail, instead of producing
an error message and aborting whenever the specified file cannot be
opened or closed.

*/
static Int nofileerrors(
    USES_REGS1) { /* '$read2'(+Flag,?Term,?Module,?Vars,-Pos,-Err,+Stream)  */
  return setYapFlag(TermFileerrors, TermFalse);
}

static Int style_checker(USES_REGS1) {
  Term t = Deref(ARG1);

  if (IsVarTerm(t)) {
    Term t = TermNil;
    if (getYapFlag(MkAtomTerm(AtomSingleVarWarnings)) == TermTrue) {
      t = MkPairTerm(MkAtomTerm(AtomSingleVarWarnings), t);
    }
    if (getYapFlag(MkAtomTerm(AtomDiscontiguousWarnings)) == TermTrue) {
      t = MkPairTerm(MkAtomTerm(AtomDiscontiguousWarnings), t);
    }
    if (getYapFlag(MkAtomTerm(AtomRedefineWarnings)) == TermTrue) {
      t = MkPairTerm(MkAtomTerm(AtomRedefineWarnings), t);
    }
  } else {
    while (IsPairTerm(t)) {
      Term h = HeadOfTerm(t);
      t = TailOfTerm(t);

      if (IsVarTerm(h)) {
        Yap_Error(INSTANTIATION_ERROR, t, "style_check/1");
        return (FALSE);
      } else if (IsAtomTerm(h)) {
        Atom at = AtomOfTerm(h);
        if (at == AtomSingleVarWarnings)
          setYapFlag(MkAtomTerm(AtomSingleVarWarnings), TermTrue);
        else if (at == AtomDiscontiguousWarnings)
          setYapFlag(MkAtomTerm(AtomDiscontiguousWarnings), TermTrue);
        else if (at == AtomRedefineWarnings)
          setYapFlag(MkAtomTerm(AtomRedefineWarnings), TermTrue);
      } else {
        Atom at = AtomOfTerm(ArgOfTerm(1, h));
        if (at == AtomSingleVarWarnings)
          setYapFlag(MkAtomTerm(AtomSingleVarWarnings), TermFalse);
        else if (at == AtomDiscontiguousWarnings)
          setYapFlag(MkAtomTerm(AtomDiscontiguousWarnings), TermFalse);
        else if (at == AtomRedefineWarnings)
          setYapFlag(MkAtomTerm(AtomRedefineWarnings), TermFalse);
      }
    }
  }
  return TRUE;
}

Term Yap_StringToTerm(const char *s, size_t len, encoding_t *encp, int prio,
                      Term *bindings) {
  CACHE_REGS
  Term bvar = MkVarTerm(), ctl;
  yhandle_t sl;

  if (bindings) {
    ctl = Yap_MkApplTerm(Yap_MkFunctor(AtomVariableNames, 1), 1, &bvar);
    sl = Yap_PushHandle(bvar);
  } else {
    ctl = TermNil;
    sl = 0;
  }

  Term rval;
  int stream = Yap_open_buf_read_stream(s, len, encp, MEM_BUF_USER);

  rval = Yap_read_term(stream, ctl, 3);
  Yap_CloseStream(stream);
  UNLOCK(GLOBAL_Stream[stream].streamlock);
  if (rval && bindings) {
    *bindings = Yap_PopHandle(sl);
  }
  return rval;
}



/**
 * @pred read_term_from_atom( +_Atom_ , - _T_ , + _VarNames_
 *
 * read a term _T_ stored in constant _Atom_ and report their names
 *
 * @param _Atom_ the source _Atom_
 * @param _T_ the output term _T_, may be any term
 * @param _VarNames_ list of _Var_ = _Name_ tuples.
 *
 * @notes Originally from SWI-Prolog, in YAP only works with  atoms.
 */
static Int atom_to_term(USES_REGS1) {
  Term t1 = Deref(ARG1), ctl, rc;
  Atom at;
  if (IsVarTerm(t1)) {
    Yap_Error(INSTANTIATION_ERROR, t1, "atom_to_term/2");
    return (FALSE);
  } else if (!IsAtomTerm(t1)) {
    Yap_Error(TYPE_ERROR_ATOM, t1, "atom_to_term/2");
    return (FALSE);
  } else {
    at = AtomOfTerm(t1);
  }
  ctl = TermNil;
  if ((rc = Yap_ReadFromAtom(at, ctl)) == 0L)
    return false;
  return Yap_unify(rc, ARG2);
}

/**
 * @pred string_to_term( ?_Atom_ , ? _T_ )
 *
 * read a term _T_ stored in constant _String_, or write the term T as
 * a constant +Atom
 *
 * @param _Atom_ the source _Atom_
 * @param _T_ the output term _T_, may be any term
 *
 */
static Int term_to_string(USES_REGS1) {
  Term t2 = Deref(ARG2), rc = false, t1 = Deref(ARG1);
  const char *s;
  if (IsVarTerm(t2)) {
    size_t length;
    s = Yap_TermToString(ARG1, NULL, 0, &length, NULL,
                         Quote_illegal_f | Handle_vars_f);
    if (!s || !MkStringTerm(s)) {
      Yap_Error(RESOURCE_ERROR_HEAP, t1,
                "Could not get memory from the operating system");
      return false;
    }
    return Yap_unify(ARG2, MkStringTerm(s));
  } else if (!IsStringTerm(t2)) {
    Yap_Error(TYPE_ERROR_STRING, t2, "string_to_ter®m/2");
    return false;
  } else {
    s = StringOfTerm(t2);
  }
  return (rc = readFromBuffer(s, TermNil)) != 0L && Yap_unify(rc, ARG1);
}

/**
 * @pred atom_to_term( ?_Atom_ , ? _T_ )
 *
 * read a term _T_ stored in constant _Atom_, or write the term T as
 * a constant +Atom
 *
 * @param _Atom_ the source _Atom_
 * @param _T_ the output term _T_, may be any term
 *
 */
static Int term_to_atom(USES_REGS1) {
  Term t2 = Deref(ARG2), ctl, rc = false;
  Atom at;
  if (IsVarTerm(t2)) {
    size_t length;
    char *s = Yap_TermToString(Deref(ARG1), NULL, 0, &length, NULL,
                               Quote_illegal_f | Handle_vars_f);
    if (!s || !(at = Yap_LookupAtom(s))) {
      Yap_Error(RESOURCE_ERROR_HEAP, t2,
                "Could not get memory from the operating system");
      return false;
    }
    return Yap_unify(ARG2, MkAtomTerm(at));
  } else if (!IsAtomTerm(t2)) {
    Yap_Error(TYPE_ERROR_ATOM, t2, "atom_to_term/2");
    return (FALSE);
  } else {
    at = AtomOfTerm(t2);
  }
  ctl = TermNil;
  return Yap_ReadFromAtom(at, ctl) == 0L && Yap_unify(rc, ARG1);
}

/**
* @pred read_term_from_atom( +_Atom_ , - _T_ , + _Options_
*
* read a term _T_ stored in constant _Atom_ according to  _Options_
*
* @param _Atom_ the source _Atom_
* @param _T_ the output term _T_, may be any term
* @param _Options_ read_term/3 options.
*
* @notes Originally from SWI-Prolog, in YAP only works with internalised atoms
* Check  read_term_from_atomic/3 for the general version. Also, the built-in is
*supposed to
* use YAP's internal encoding, so please avoid the encoding/1 option.
*/
static Int read_term_from_atom(USES_REGS1) {
  Term t1 = Deref(ARG1), rc;
  Atom at;
  if (IsVarTerm(t1)) {
    Yap_Error(INSTANTIATION_ERROR, t1, "style_check/1");
    return (FALSE);
  } else if (!IsAtomTerm(t1)) {
    Yap_Error(TYPE_ERROR_ATOM, t1, "style_check/1");
    return (FALSE);
  } else {
    at = AtomOfTerm(t1);
  }
  if ((rc = Yap_ReadFromAtom(at, Deref(ARG3))) == 0L)
    return false;
  return Yap_unify(rc, ARG2);
}

Term Yap_ReadFromAtom(Atom a, Term opts) {
  Term rval;
  int sno;
  if (IsWideAtom(a)) {
    wchar_t *ws = a->WStrOfAE;
    size_t len = wcslen(ws);
    encoding_t enc = ENC_ISO_ANSI;
    sno = Yap_open_buf_read_stream((char *)ws, len, &enc, MEM_BUF_USER);
  } else {
    char *s = a->StrOfAE;
    size_t len = strlen(s);
    encoding_t enc = ENC_ISO_LATIN1;
    sno = Yap_open_buf_read_stream((char *)s, len, &enc, MEM_BUF_USER);
  }
 
  rval = Yap_read_term(sno, opts, 3);
  Yap_CloseStream(sno);
  return rval;
}
static Term readFromBuffer(const char *s, Term opts) {
 Term rval;
  int sno;
  encoding_t enc = ENC_ISO_UTF8;
  sno = Yap_open_buf_read_stream((char *)s, strlen_utf8((unsigned char *)s),
                                 &enc, MEM_BUF_USER);

  rval = Yap_read_term(sno, opts, 3);
  Yap_CloseStream(sno);
  return rval;
}
 

/**
* @pred read_term_from_string( +_String_ , - _T_ , + _Options_
*
* read a term _T_ stored in constant _String_ according to  _Options_
*
* @param _String_ the source _String_
* @param _T_ the output term _T_, may be any term
* @param _Options_ read_term/3 options.
*
* @notes Idea from SWI-Prolog, in YAP only works with strings
* Check  read_term_from_atomic/3 for the general version.
*/
static Int read_term_from_string(USES_REGS1) {
  Term t1 = Deref(ARG1), rc;
  const unsigned char *s;
  size_t len;
  if (IsVarTerm(t1)) {
    Yap_Error(INSTANTIATION_ERROR, t1, "read_term_from_string/3");
    return (FALSE);
  } else if (!IsStringTerm(t1)) {
    Yap_Error(TYPE_ERROR_STRING, t1, "read_term_from_string/3");
    return (FALSE);
  } else {
    s = UStringOfTerm(t1);
    len = strlen_utf8(s);
  }
  char *ss = (char *)s;
  encoding_t enc = ENC_ISO_UTF8;
  int sno = Yap_open_buf_read_stream(ss, len, &enc, MEM_BUF_USER);
  rc = readFromBuffer(ss, Deref(ARG3));
  Yap_CloseStream(sno);
  if (!rc)
    return false;
  return Yap_unify(rc, ARG2);
}

/**
*  @pred read_term_from_atomic( +_Atomic_ , - _T_ , + _Options_ )
*
* read a term _T_ stored in text _Atomic_ according to  _Options_
*
* @param _Atomic_ the source may be an atom, string, list of codes, or list of
*chars.
* @param _T_ the output term _T_, may be any term
* @param _Options_ read_term/3 options.
*
* @notes Idea originally from SWI-Prolog, but in YAP we separate atomic and
*atom.
* Encoding is fixed in atoms and strings.
*/
static Int read_term_from_atomic(USES_REGS1) {
  Term t1 = Deref(ARG1), rc;
  const unsigned char *s;
  size_t len;
  if (IsVarTerm(t1)) {
    Yap_Error(INSTANTIATION_ERROR, t1, "read_term_from_atomic/3");
    return (FALSE);
  } else if (!IsAtomicTerm(t1)) {
    Yap_Error(TYPE_ERROR_ATOMIC, t1, "read_term_from_atomic/3");
    return (FALSE);
  } else {
    Term t = Yap_AtomicToString(t1 PASS_REGS);
    s = UStringOfTerm(t);
    len = strlen_utf8((unsigned char *)s);
  }
  char *ss = (char *)s;
  encoding_t enc = ENC_ISO_UTF8;
  int sno = Yap_open_buf_read_stream(ss, len, &enc, MEM_BUF_USER);
  rc = readFromBuffer(ss, Deref(ARG3));
  Yap_CloseStream(sno);
  if (!rc)
    return false;
  return Yap_unify(rc, ARG2);
}

void Yap_InitReadTPreds(void) {
  Yap_InitCPred("read", 1, read1, SyncPredFlag);
  Yap_InitCPred("read", 2, read2, SyncPredFlag);
  Yap_InitCPred("read_term", 2, read_term2, SyncPredFlag);
  Yap_InitCPred("read_term", 3, read_term, SyncPredFlag);
  Yap_InitCPred("read_clause", 2, read_clause2, SyncPredFlag);
  Yap_InitCPred("read_clause", 3, read_clause, 0);

  Yap_InitCPred("term_to_string", 2, term_to_string, 0);
  Yap_InitCPred("term_to_atom", 2, term_to_atom, 0);
  Yap_InitCPred("atom_to_term", 3, atom_to_term, 0);
  Yap_InitCPred("read_term_from_atom", 3, read_term_from_atom, 0);
  Yap_InitCPred("read_term_from_atomic", 3, read_term_from_atomic, 0);
  Yap_InitCPred("read_term_from_string", 3, read_term_from_string, 0);

  Yap_InitCPred("fileerrors", 0, fileerrors, SyncPredFlag);
  Yap_InitCPred("nofileeleerrors", 0, nofileerrors, SyncPredFlag);
  Yap_InitCPred("source_location", 2, source_location, SyncPredFlag);
  Yap_InitCPred("$style_checker", 1, style_checker,
                SyncPredFlag | HiddenPredFlag);
}

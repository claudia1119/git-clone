/*************************************************************************
*									 *
*	 YAP Prolog 							 *
*									 *
*	Yap Prolog was developed at NCCUP - Universidade do Porto	 *
*									 *
* Copyright L.Damas, V.S.Costa and Universidade do Porto 2015-		 *
*									 *
**************************************************************************
*									 *
* File:		YapFlags.h						 *
* Last rev:								 *
* mods:									 *
* comments:	flag system interface.					 *
*									 *
*************************************************************************/

/** @file YapFlags.h

    @ingroup Flags
*/

#ifndef YAP_FLAGS_H
#define YAP_FLAGS_H 1

// INLINE_ONLY inline EXTERN  bool nat( Term inp );

#define SYSTEM_OPTION_0 "attributed_variables,rational_trees]"
#if THREADS
#define SYSTEM_OPTION_1 "threads,"
#endif
#if USE_GMP
#define SYSTEM_OPTION_3 "big_numbers,"
#endif
#if DEPTH_LIMIT
#define SYSTEM_OPTION_4 "depth_limit,"
#endif
#if LOW_LEVEL_TRACE
#define SYSTEM_OPTION_5 "low_level_tracer,"
#endif
#if YAPOR
#define SYSTEM_OPTION_6 "or_parallelism,"
#endif
#if USE_READLINE
#define SYSTEM_OPTION_7 "readline,"
#endif
#if TABLING
#define SYSTEM_OPTION_8 "tabling,"
#endif

static inline bool nat(Term inp) {
  if (IsVarTerm(inp)) {
    Yap_Error(INSTANTIATION_ERROR, inp, "set_prolog_flag: value must be %s",
              "bound");
    return false;
  }
  if (IsIntTerm(inp)) {
    Int i = IntOfTerm(inp);
    if (i >= 0)
      return true;
    Yap_Error(DOMAIN_ERROR_NOT_LESS_THAN_ZERO, inp,
              "set_prolog_flag: value must be %s", ">= 0");
    return false;
  }
  Yap_Error(TYPE_ERROR_INTEGER, inp, "set_prolog_flag: value must be %s",
            "integer");
  return false;
}

static inline bool at2n(Term inp) {
  Yap_Error(PERMISSION_ERROR_READ_ONLY_FLAG, inp, "set_prolog_flag %s",
            "flag is read-only");
  return false;
}

static inline bool isfloat(Term inp) {
  if (IsVarTerm(inp)) {
    Yap_Error(INSTANTIATION_ERROR, inp, "set_prolog_flag: value must be %s",
              "integer");

    return false;
  }
  if (IsFloatTerm(inp)) {
    return true;
  }
  Yap_Error(TYPE_ERROR_FLOAT, inp, "set_prolog_flag: value must be %s",
            "floating-point");
  return false;
}

static inline bool ro(Term inp);

static inline bool ro(Term inp) {
  if (IsVarTerm(inp)) {
    Yap_Error(INSTANTIATION_ERROR, inp, "set_prolog_flag: value must be %s",
              "bound");
    return false;
  }
  Yap_Error(PERMISSION_ERROR_READ_ONLY_FLAG, inp, "set_prolog_flag %s",
            "flag is read-only");
  return false;
}

INLINE_ONLY inline EXTERN bool aro(Term inp) {
  if (IsVarTerm(inp)) {
    Yap_Error(INSTANTIATION_ERROR, inp, "set_prolog_flag %s",
              "value must be bound");

    return false;
  }
  Yap_Error(PERMISSION_ERROR_READ_ONLY_FLAG, inp, "set_prolog_flag %s",
            "flag is read-only");

  return false;
}

// INLINE_ONLY inline EXTERN bool booleanFlag( Term inp );

static inline bool booleanFlag(Term inp) {
  if (inp == TermTrue || inp == TermFalse || inp == TermOn || inp == TermOff)
    return true;
  if (IsVarTerm(inp)) {
    Yap_Error(INSTANTIATION_ERROR, inp, "set_prolog_flag %s",
              "value must be bound");
    ;
    return false;
  }
  if (IsAtomTerm(inp)) {
    Yap_Error(DOMAIN_ERROR_OUT_OF_RANGE, inp,
              "set_prolog_flag in {true,false,on,off}");
    return false;
  }
  Yap_Error(TYPE_ERROR_ATOM, inp, "set_prolog_flag in {true,false,on,off");
  return false;
}

static bool synerr(Term inp) {
  if (inp == TermDec10 || inp == TermFail || inp == TermError ||
      inp == TermQuiet)
    return true;

  if (IsAtomTerm(inp)) {
    Yap_Error(DOMAIN_ERROR_OUT_OF_RANGE, inp,
              "set_prolog_flag in {dec10,error,fail,quiet}");
    return false;
  }
  Yap_Error(TYPE_ERROR_ATOM, inp,
            "set_prolog_flag in {dec10,error,fail,quiet}");
  return false;
}

static inline bool filler(Term inp) { return true; }

static inline bool list_filler(Term inp) {
  if (IsVarTerm(inp) ||
      IsPairTerm(inp) ||
      inp == TermNil)
    return true;

    Yap_Error(TYPE_ERROR_LIST, inp,
              "set_prolog_flag in {codes,string}");
      
  return false; }

static bool bqs(Term inp) {
  if (inp == TermCodes || inp == TermString || inp == TermSymbolChar)
    return true;

  if (IsAtomTerm(inp)) {
    Yap_Error(DOMAIN_ERROR_OUT_OF_RANGE, inp,
              "set_prolog_flag in {codes,string}");
    return false;
  }
  Yap_Error(TYPE_ERROR_ATOM, inp, "set_prolog_flag in {codes,string}");
  return false;
}

// INLINE_ONLY inline EXTERN  bool isatom( Term inp );

static inline bool isatom(Term inp) {
  if (IsVarTerm(inp)) {
    Yap_Error(INSTANTIATION_ERROR, inp, "set_prolog_flag %s",
              "value must be bound");
    return false;
  }
  if (IsAtomTerm(inp))
    return true;
  Yap_Error(TYPE_ERROR_ATOM, inp, "set_prolog_flag");
  return false;
}

static inline bool options(Term inp) { return Yap_IsGroundTerm(inp); }

// INLINE_ONLY inline EXTERN  bool ok( Term inp );

static inline bool ok(Term inp) { return true; }

// a pair, obtained from x(y) -> 1,2,y)
typedef struct x_el {
  bool used;
  Term tvalue;
} xarg;

typedef struct struct_param {
  char *name;
  flag_func type;
  int id;
} param_t;

typedef struct struct_param2 {
  char *name;
  flag_func type;
  int id;
  const char *scope;
} param2_t;

typedef struct {
  char *name;
  bool writable;
  flag_func def;
  const char *init;
  flag_func helper;
} flag_info;

typedef struct {
  char *name;
  flag_func def;
  const char *init;
} arg_info;

typedef union flagTerm {
  Term at;
  struct DB_TERM *DBT;
} flag_term;

void Yap_InitFlags(bool);

#define YAP_FLAG(x, NAME, WRITABLE, DEF, INIT, HELPER) x

typedef enum {
#include "YapGFlagInfo.h"
} global_flag_t;

typedef enum {
#include "YapLFlagInfo.h"
} local_flag_t;
#undef YAP_FLAG

bool setYapFlag(Term tflag, Term t2);
Term getYapFlag(Term tflag);

int Yap_ArgKey(Atom key, const param_t *def, int n);

static inline bool check_refs_to_ltable(void) { return true; }

static inline void setAtomicGlobalPrologFlag(int id, Term v) {
  GLOBAL_Flags[id].at = v;
}

static inline Term getAtomicGlobalPrologFlag(int id) {
  return GLOBAL_Flags[id].at;
}

static inline void setAtomicLocalPrologFlag(int id, Term v) {
  CACHE_REGS
  check_refs_to_ltable();
  LOCAL_Flags[id].at = v;
}

static inline void setBooleanLocalPrologFlag(int id, bool v) {
  CACHE_REGS
  check_refs_to_ltable();
  if (v) {
    LOCAL_Flags[id].at = TermTrue;
  } else {
    LOCAL_Flags[-id].at = TermFalse;
  }
}

static inline void setBooleanGlobalPrologFlag(int id, bool v) {
  if (v) {
    GLOBAL_Flags[id].at = TermTrue;
  } else {
    GLOBAL_Flags[id].at = TermFalse;
  }
}

static inline bool trueGlobalPrologFlag(int id) {
  return GLOBAL_Flags[id].at == TermTrue;
}

static inline bool falseGlobalPrologFlag(int id) {
  return GLOBAL_Flags[id].at == TermFalse;
}

static inline bool trueLocalPrologFlag(int id) {
  CACHE_REGS
  return LOCAL_Flags[id].at == TermTrue;
}

static inline bool falsePrologFlag(int id) {
  CACHE_REGS
  return LOCAL_Flags[id].at == TermFalse;
}

static inline bool isoLanguageFlag(void) {
  return GLOBAL_Flags[ISO_FLAG].at == TermTrue;
}

static inline bool strictISOFlag(void) {
  return GLOBAL_Flags[STRICT_ISO_FLAG].at == TermTrue;
}

static inline bool silentMode(void) {
  return GLOBAL_Flags[VERBOSE_FLAG].at == TermSilent;
}

static inline void setVerbosity(Term val) {
  GLOBAL_Flags[VERBOSE_FLAG].at = val;
}

static inline bool setSyntaxErrorsFlag(Term val) {
  if (!synerr(val))
    return false;
  CACHE_REGS
  LOCAL_Flags[SYNTAX_ERRORS_FLAG].at = val;
  return true;
}

static inline Term getSyntaxErrorsFlag(void) {
  CACHE_REGS
  return LOCAL_Flags[SYNTAX_ERRORS_FLAG].at;
}

static inline bool setBackQuotesFlag(Term val) {
  if (!bqs(val))
    return false;
  if (val == TermSymbolChar)
    val = TermString;
  GLOBAL_Flags[BACKQUOTED_STRING_FLAG].at = val;
  return true;
}

static inline Term getBackQuotesFlag(void) {
  return GLOBAL_Flags[BACKQUOTED_STRING_FLAG].at;
}

static inline int indexingMode(void) { return GLOBAL_Flags[INDEX_FLAG].at; }

static inline const char *floatFormat(void) {
  return RepAtom(AtomOfTerm(GLOBAL_Flags[FLOAT_FORMAT_FLAG].at))->rep.uStrOfAE;
}

static inline size_t indexingDepth(void) {
  return IntOfTerm(GLOBAL_Flags[INDEX_SUB_TERM_SEARCH_DEPTH_FLAG].at);
}

static inline Term gcTrace(void) {
  return GLOBAL_Flags[GC_TRACE_FLAG].at;
}

bool rmdot(Term inp);

xarg *Yap_ArgListToVector(Term listl, const param_t *def, int n);

xarg *Yap_ArgList2ToVector(Term listl, const param2_t *def, int n);

#endif // YAP_FLAGS_H

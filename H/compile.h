/*************************************************************************
*									 *
*	 YAP Prolog  %W% %G%						 *
*									 *
*	Yap Prolog was developed at NCCUP - Universidade do Porto	 *
*									 *
* Copyright L.Damas, V.S.Costa and Universidade do Porto 1985-1997	 *
*									 *
**************************************************************************
*									 *
* File:		compile.h						 *
* Last rev:								 *
* mods:									 *
* comments:	compiler data structures and routines			 *
*									 *
*************************************************************************/

/* consult stack management */

/* virtual machine instruction op-codes					*/
typedef enum compiler_op {
  nop_op,
  get_var_op,
  put_var_op,
  get_val_op,
  put_val_op,
  get_atom_op,
  put_atom_op,
  get_num_op,
  put_num_op,
  get_float_op,
  put_float_op,
  align_float_op,
  get_longint_op,
  put_longint_op,
  get_bigint_op,
  put_bigint_op,
  get_list_op,
  put_list_op,
  get_struct_op,
  put_struct_op,
  put_unsafe_op,
  unify_var_op,
  write_var_op,
  unify_val_op,
  write_val_op,
  unify_atom_op,
  write_atom_op,
  unify_num_op,
  write_num_op,
  unify_float_op,
  write_float_op,
  unify_longint_op,
  write_longint_op,
  unify_bigint_op,
  write_bigint_op,
  unify_list_op,
  write_list_op,
  unify_struct_op,
  write_struct_op,
  write_unsafe_op,
  fail_op,
  cut_op,
  cutexit_op,
  allocate_op,
  deallocate_op,
  tryme_op,
  jump_op,
  jumpi_op,
  procceed_op,
  call_op,
  execute_op,
  safe_call_op,
  label_op,
  name_op,
  pop_op,
  retryme_op,
  trustme_op,
  either_op,
  orelse_op,
  orlast_op,
  push_or_op,
  pushpop_or_op,
  pop_or_op,
  save_b_op,
  commit_b_op,
  patch_b_op,
  try_op,
  retry_op,
  trust_op,
  try_in_op,
  jump_v_op,
  jump_nv_op,
  cache_arg_op,
  cache_sub_arg_op,
  switch_on_type_op,
  switch_c_op,
  if_c_op,
  switch_f_op,
  if_f_op,
  if_not_op,
  index_dbref_op,
  index_blob_op,
  if_nonvar_op,
  save_pair_op,
  save_appl_op,
  commit_opt_op,
  unify_local_op,
  write_local_op,
  unify_last_list_op,
  write_last_list_op,
  unify_last_struct_op,
  write_last_struct_op,
  unify_last_var_op,
  unify_last_val_op,
  unify_last_local_op,
  unify_last_atom_op,
  unify_last_num_op,
  unify_last_float_op,
  unify_last_longint_op,
  unify_last_bigint_op,
  mark_initialised_pvars_op,
  mark_live_regs_op,
  fetch_args_vv_op,
  fetch_args_cv_op,
  fetch_args_vc_op,
  f_var_op,
  f_val_op,
  enter_profiling_op,
  retry_profiled_op,
  count_call_op,
  count_retry_op,
  restore_tmps_op,
  restore_tmps_and_skip_op,
  enter_lu_op,
  empty_call_op,
#ifdef YAPOR
  sync_op,
#endif /* YAPOR */
#ifdef TABLING
  table_new_answer_op,
  table_try_single_op,
#endif /* TABLING */
#ifdef TABLING_INNER_CUTS
  clause_with_cut_op,
#endif /* TABLING_INNER_CUTS */
  fetch_args_for_bccall,
  bccall_op,
  blob_op
#ifdef SFUNC
  ,
  get_s_f_op,
  put_s_f_op,
  unify_s_f_op,
  write_s_f_op,
  unify_s_var_op,
  write_s_var_op,
  unify_s_val_op,
  write_s_val_op,
  unify_s_a_op,
  write_s_a_op,
  get_s_end_op,
  put_s_end_op,
  unify_s_end_op,
  write_s_end_op,
#endif

} compiler_vm_op;

typedef struct PSEUDO {
	struct PSEUDO *nextInst;
	enum compiler_op op;
        CELL rnd1;
	union {
	    Int   oprnd2;
	    CELL opseqt[1];
	} ops;
    } PInstr;

#define arnds ops.opseqt
#define rnd2  ops.oprnd2
#define rnd3  ops.opseqt[1]
#define rnd4  ops.opseqt[2]

typedef struct VENTRY {
	 CELL SelfOfVE;
	 Term AdrsOfVE;
	 Int  KindOfVE;
	 CELL NoOfVE;
	 PInstr *FirstOpForV;
	 PInstr *LastOpForV;
	 BITS16 AgeOfVE;
	 BITS16 BranchOfVE;
	 BITS16 FirstOfVE;
	 BITS16 RCountOfVE;
	 BITS16 FlagsOfVE;
	 struct VENTRY *NextOfVE;
    } Ventry;

typedef struct CEXPENTRY {
         Term   TermOfCE;
	 PInstr *CodeOfCE;
	 Term   VarOfCE;
	 struct CEXPENTRY *RightCE, *LeftCE;
       } CExpEntry;


typedef struct intermediates {
  char *freep;
  char *freep0;
  struct PSEUDO *cpc;
  struct PSEUDO *CodeStart;
  struct PSEUDO *icpc;
  struct PSEUDO *BlobsStart;
  int  *label_offset;
  Int  *uses;
  Term *contents;
  struct pred_entry *CurrentPred;  
  jmp_buf CompilerBotch;
  yamop *code_addr;
  yamop *expand_block;
  /* for expanding code */
  union { 
    struct static_index *si;
    struct logic_upd_index *lui;
  } current_cl;
} CIntermediates;

#define	 SafeVar	0x01
#define  PermFlag	0x02
#define  GlobalVal	0x04
#define  OnHeadFlag	0x08
#define  NonVoid	0x10
#define  BranchVar      0x20
#define  OnLastGoal     0x40

#define  MaskVarClass   0x0f000000L
#define  MaskVarAdrs    0x00ffffffL
#define	 Unassigned	0x00000000L
#define	 VoidVar	0x01000000L
#define	 TempVar	0x02000000L
#define	 PermVar	0x03000000L


#define save_b_flag	   10000
#define commit_b_flag	   10001
#define save_appl_flag	   10002
#define save_pair_flag	   10004
#define f_flag		   10008
#define bt1_flag	   10010
#define bt2_flag	   10020
#define patch_b_flag	   10040


#define Zero	 0
#define One	 1
#define Two	 2


yamop  *STD_PROTO(Yap_assemble,(int,Term,struct pred_entry *,int, struct intermediates *));
void	STD_PROTO(Yap_emit,(compiler_vm_op,Int,CELL, struct intermediates *));
void	STD_PROTO(Yap_emit_3ops,(compiler_vm_op,CELL,CELL,CELL, struct intermediates *));
void	STD_PROTO(Yap_emit_4ops,(compiler_vm_op,CELL,CELL,CELL,CELL, struct intermediates *));
CELL   *STD_PROTO(Yap_emit_extra_size,(compiler_vm_op,CELL,int, struct intermediates *));
char   *STD_PROTO(Yap_AllocCMem,(int, struct intermediates *));
int	STD_PROTO(Yap_is_a_test_pred,(Term, Term));
void    STD_PROTO(Yap_bip_name,(Int, char *));
#ifdef DEBUG
void	STD_PROTO(Yap_ShowCode,(struct intermediates *));
#endif /* DEBUG */



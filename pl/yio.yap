/*************************************************************************
*									 *
*	 YAP Prolog 							 *
*									 *
*	Yap Prolog was developed at NCCUP - Universidade do Porto	 *
*									 *
* Copyright L.Damas, V.S.Costa and Universidade do Porto 1985-1997	 *
*									 *
**************************************************************************
*									 *
* File:		yio.yap							 *
* Last rev:								 *
* mods:									 *
* comments:	Input output predicates			 		 *
*									 *
*************************************************************************/

/* stream predicates							*/

open(Source,M,T) :- var(Source), !,
	throw(error(instantiation_error,open(Source,M,T))).
open(Source,M,T) :- var(M), !,
	throw(error(instantiation_error,open(Source,M,T))).
open(Source,M,T) :- nonvar(T), !,
	throw(error(type_error(variable,T),open(Source,M,T))).
open(File,Mode,Stream) :-
	'$open'(File,Mode,Stream,0).

/* meaning of flags for '$write' is
	 1	quote illegal atoms
	 2	ignore operator declarations
	 4	output '$VAR'(N) terms as A, B, C, ...
	 8	use portray(_)
*/

close(V) :- var(V), !,
	throw(error(instantiation_error,close(V))).
close(File) :-
	atom(File), !,
	(
	    '$access_yap_flags'(8, 0),
	    current_stream(_,_,Stream),
	    '$user_file_name'(Stream,File)
        ->
	    '$close'(Stream)
	;
	    '$close'(File)
	).
close(Stream) :-
	'$close'(Stream).

close(V,Opts) :- var(V), !,
	throw(error(instantiation_error,close(V,Opts))).
close(S,Opts) :-
	'$check_io_opts'(Opts,close(S,Opts)),
	/* YAP ignores the force/1 flag */ 
	close(S).
	
open(F,T,S,Opts) :-
	'$check_io_opts'(Opts,open(F,T,S,Opts)),
	'$process_open_opts'(Opts, 0, N, Aliases),
	'$open2'(F,T,S,N),
	'$process_open_aliases'(Aliases,S).

'$open2'(Source,M,T,N) :- var(Source), !,
	throw(error(instantiation_error,open(Source,M,T,N))).
'$open2'(Source,M,T,N) :- var(M), !,
	throw(error(instantiation_error,open(Source,M,T,N))).
'$open2'(Source,M,T,N) :- nonvar(T), !,
	throw(error(type_error(variable,T),open(Source,M,T,N))).
'$open2'(File,Mode,Stream,N) :-
	'$open'(File,Mode,Stream,N).

'$process_open_aliases'([],_).
'$process_open_aliases'([Alias|Aliases],S) :-
	'$add_alias_to_stream'(Alias, S),
	'$process_open_aliases'(Aliases,S).

'$process_open_opts'([], N, N, []).
'$process_open_opts'([type(T)|L], N0, N, Aliases) :-
	'$value_open_opt'(T,type,I1,I2),
	N1 is I1\/N0,
	N2 is I2/\N1,
	'$process_open_opts'(L,N2,N, Aliases).
'$process_open_opts'([reposition(T)|L], N0, N, Aliases) :-
	'$value_open_opt'(T,reposition,I1,I2),
	N1 is I1\/N0,
	N2 is I2/\N1,
	'$process_open_opts'(L,N2,N, Aliases).
'$process_open_opts'([eof_action(T)|L], N0, N, Aliases) :-
	'$value_open_opt'(T,eof_action,I1,I2),
	N1 is I1\/N0,
	N2 is I2/\N1,
	'$process_open_opts'(L,N2,N, Aliases).
'$process_open_opts'([alias(Alias)|L], N0, N, [Alias|Aliases]) :-
	'$process_open_opts'(L,N0,N, Aliases).


'$value_open_opt'(text,_,1,X) :- X is 128-2. % default
'$value_open_opt'(binary,_,2, X) :- X is 128-1.
'$value_open_opt'(true,_,4, X) :- X is 128-8.
'$value_open_opt'(false,_,8, X) :- X is 128-4.
'$value_open_opt'(error,_,16, X) :- X is 128-32-64.
'$value_open_opt'(eof_code,_,32, X) :- X is 128-16-64.
'$value_open_opt'(reset,64, X) :- X is 128-32-16.

/* check whether a list of options is valid */
'$check_io_opts'(V,G) :- var(V), !,
	throw(error(instantiation_error,G)).
'$check_io_opts'([],_) :- !.
'$check_io_opts'([H|_],G) :- var(H), !,
	throw(error(instantiation_error,G)).
'$check_io_opts'([Opt|T],G) :- !,
	'$check_opt'(G,Opt,G),
	'$check_io_opts'(T,G).
'$check_io_opts'(T,G) :-
	throw(error(type_error(list,T),G)).

'$check_opt'(close(_,_),Opt,G) :- !,
	(Opt = force(X) ->
	    '$check_force_opt_arg'(X,G) ;
	    throw(error(domain_error(close_option,Opt),G))
	).
'$check_opt'(open(_,_,_,_),Opt,G) :- !,
	'$check_opt_open'(Opt, G).
'$check_opt'(read_term(_,_),Opt,G) :- !,
	'$check_opt_read'(Opt, G).
'$check_opt'(stream_property(_,_),Opt,G) :- !,
	'$check_opt_sp'(Opt, G).
'$check_opt'(write_term(_,_),Opt,G) :- !,
	'$check_opt_write'(Opt, G).


'$check_opt_open'(type(T), G) :- !,
	'$check_open_type_arg'(T, G).
'$check_opt_open'(reposition(T), G) :- !,
	'$check_open_reposition_arg'(T, G).
'$check_opt_open'(alias(T), G) :- !,
	'$check_open_alias_arg'(T, G).
'$check_opt_open'(eof_action(T), G) :- !,
	'$check_open_eof_action_arg'(T, G).
'$check_opt_open'(A, G) :-
	throw(error(domain_error(stream_option,A),G)).

'$check_opt_read'(variables(_), _) :- !.
'$check_opt_read'(variable_names(_), _) :- !.
'$check_opt_read'(singletons(_), _) :- !.
'$check_opt_read'(syntax_errors(T), G) :- !,
	'$check_read_syntax_errors_arg'(T, G).
'$check_opt_read'(A, G) :-
	throw(error(domain_error(read_option,A),G)).

'$check_opt_sp'(file_name(_), _) :- !.
'$check_opt_sp'(mode(_), _) :- !.
'$check_opt_sp'(input, _) :- !.
'$check_opt_sp'(output, _) :- !.
'$check_opt_sp'(alias(_), _) :- !.
'$check_opt_sp'(position(_), _) :- !.
'$check_opt_sp'(end_of_stream(_), _) :- !.
'$check_opt_sp'(eof_action(_), _) :- !.
'$check_opt_sp'(reposition(_), _) :- !.
'$check_opt_sp'(type(_), _) :- !.
'$check_opt_sp'(A, G) :-
	throw(error(domain_error(stream_property,A),G)).

'$check_opt_write'(quoted(T), G) :- !,
	'$check_write_quoted_arg'(T, G).
'$check_opt_write'(ignore_ops(T), G) :- !,
	'$check_write_ignore_ops_arg'(T, G).
'$check_opt_write'(numbervars(T), G) :- !,
	'$check_write_numbervars_arg'(T, G).
'$check_opt_write'(portrayed(T), G) :- !,
	'$check_write_portrayed'(T, G).
'$check_opt_write'(max_depth(T), G) :- !,
	'$check_write_max_depth'(T, G).
'$check_opt_write'(A, G) :-
	throw(error(domain_error(write_option,A),G)).

%
% check force arg
%
'$check_force_opt_arg'(X,G) :- var(X), !,
	throw(error(instantiation_error,G)).
'$check_force_opt_arg'(true,_) :- !.
'$check_force_opt_arg'(false,_) :- !.
'$check_force_opt_arg'(X,G) :-
	throw(error(domain_error(close_option,force(X)),G)).

'$check_open_type_arg'(X, G) :- var(X), !,
	throw(error(instantiation_error,G)).
'$check_open_type_arg'(text,_) :- !.
'$check_open_type_arg'(binary,_) :- !.
'$check_open_opt_arg'(X,G) :-
	throw(error(domain_error(io_mode,type(X)),G)).

'$check_open_reposition_arg'(X, G) :- var(X), !,
	throw(error(instantiation_error,G)).
'$check_open_reposition_arg'(true,_) :- !.
'$check_open_reposition_arg'(false,_) :- !.
'$check_open_reposition_arg'(X,G) :-
	throw(error(domain_error(io_mode,reposition(X)),G)).

'$check_open_alias_arg'(X, G) :- var(X), !,
	throw(error(instantiation_error,G)).
'$check_open_alias_arg'(X,G) :- atom(X), !,
	( '$check_if_valid_new_alias'(X), X \= user ->
	    true ;
	    throw(error(permission_error(open, source_sink, alias(X)),G))
	).
'$check_open_alias_arg'(X,G) :-
	throw(error(domain_error(io_mode,alias(X)),G)).


'$check_open_eof_action_arg'(X, G) :- var(X), !,
	throw(error(instantiation_error,G)).
'$check_open_eof_action_arg'(error,_) :- !.
'$check_open_eof_action_arg'(eof_code,_) :- !.
'$check_open_eof_action_arg'(reset,_) :- !.
'$check_open_eof_action_arg'(X,G) :-
	throw(error(domain_error(io_mode,eof_action(X)),G)).

'$check_read_syntax_errors_arg'(X, G) :- var(X), !,
	throw(error(instantiation_error,G)).
'$check_read_syntax_errors_arg'(dec10,_) :- !.
'$check_read_syntax_errors_arg'(fail,_) :- !.
'$check_read_syntax_errors_arg'(error,_) :- !.
'$check_read_syntax_errors_arg'(quiet,_) :- !.
'$check_read_syntax_errors_arg'(X,G) :-
	throw(error(domain_error(read_option,syntax_errors(X)),G)).

'$check_write_quoted_arg'(X, G) :- var(X), !,
	throw(error(instantiation_error,G)).
'$check_write_quoted_arg'(true,_) :- !.
'$check_write_quoted_arg'(false,_) :- !.
'$check_write_quoted_arg'(X,G) :-
	throw(error(domain_error(write_option,write_quoted(X)),G)).

'$check_write_ignore_ops_arg'(X, G) :- var(X), !,
	throw(error(instantiation_error,G)).
'$check_write_ignore_ops_arg'(true,_) :- !.
'$check_write_ignore_ops_arg'(false,_) :- !.
'$check_write_ignore_ops_arg'(X,G) :-
	throw(error(domain_error(write_option,ignore_ops(X)),G)).

'$check_write_numbervars_arg'(X, G) :- var(X), !,
	throw(error(instantiation_error,G)).
'$check_write_numbervars_arg'(true,_) :- !.
'$check_write_numbervars_arg'(false,_) :- !.
'$check_write_numbervars_arg'(X,G) :-
	throw(error(domain_error(write_option,numbervars(X)),G)).

'$check_write_portrayed'(X, G) :- var(X), !,
	throw(error(instantiation_error,G)).
'$check_write_portrayed'(true,_) :- !.
'$check_write_portrayed'(false,_) :- !.
'$check_write_portrayed'(X,G) :-
	throw(error(domain_error(write_option,portrayed(X)),G)).

'$check_write_max_depth'(X, G) :- var(X), !,
	throw(error(instantiation_error,G)).
'$check_write_max_depth'(I,_) :- integer(I), I > 0, !.
'$check_write_max_depth'(X,G) :-
	throw(error(domain_error(write_option,max_depth(X)),G)).

set_input(Stream) :-
	'$set_input'(Stream).
	
set_output(Stream) :-
	'$set_output'(Stream).

open_null_stream(S) :- '$open_null_stream'(S).

open_pipe_streams(P1,P2) :- '$open_pipe_stream'(P1, P2).

fileerrors :- '$set_value'(fileerrors,1).
nofileerrors :- '$set_value'(fileerrors,0).

exists(F) :- '$exists'(F,read).

see(user) :- !, set_input(user_input).
see(F) :- var(F), !,
	throw(error(instantiation_error,see(F))).
see(F) :- current_input(Stream),
	'$user_file_name'(Stream,F).
see(F) :- current_stream(_,read,Stream), '$user_file_name'(Stream,F), !,
	set_input(Stream).
see(Stream) :- '$stream'(Stream), current_stream(_,read,Stream), !,
	set_input(Stream).
see(F) :- open(F,read,Stream), set_input(Stream).

seeing(File) :- current_input(Stream),
	'$user_file_name'(Stream,NFile),
	( '$user_file_name'(user_input,NFile) -> File = user ; NFile = File).

seen :- current_input(Stream), '$close'(Stream), set_input(user).

tell(user) :- !, set_output(user_output).
tell(F) :- var(F), !,
	throw(error(instantiation_error,tell(F))).
tell(F) :- current_output(Stream),
	'$user_file_name'(Stream,F), !.
tell(F) :- current_stream(_,write,Stream), '$user_file_name'(Stream, F), !,
	set_output(Stream).
tell(Stream) :- '$stream'(Stream), current_stream(_,write,Stream), !,
	set_output(Stream).
tell(F) :- open(F,write,Stream), set_output(Stream).
		
telling(File) :- current_output(Stream),
	'$user_file_name'(Stream,NFile),
	( '$user_file_name'(user_output,NFile) -> File = user ; File = NFile ).

told :- current_output(Stream), '$close'(Stream), set_output(user).


/* Term IO	*/

read(T) :- '$read'(false,T,[]).

read(Stream,T) :-
	'$read'(false,T,_,Stream).


read_term(T, Options) :-
	'$check_io_opts'(Options,read_term(T, Options)),
	'$preprocess_read_terms_options'(Options),
	'$read'(true,T,VL),
	'$postprocess_read_terms_options'(Options, T, VL).

read_term(Stream, T, Options) :-
	'$check_io_opts'(Options,read_term(T, Options)),
	'$preprocess_read_terms_options'(Options),
	'$read'(true,T,VL,Stream),
	'$postprocess_read_terms_options'(Options, T, VL).

%
% support flags to read
%
'$preprocess_read_terms_options'([]).
'$preprocess_read_terms_options'([syntax_errors(NewVal)|L]) :- !,
	'$get_read_error_handler'(OldVal),
	'$set_value'('$read_term_error_handler', OldVal),
	'$set_read_error_handler'(NewVal),
	'$preprocess_read_terms_options'(L).
'$preprocess_read_terms_options'([_|L]) :-
	'$preprocess_read_terms_options'(L).

'$postprocess_read_terms_options'([], _, _).
'$postprocess_read_terms_options'([H|Tail], T, VL) :- !,
	'$postprocess_read_terms_option'(H, T, VL),
	'$postprocess_read_terms_options_list'(Tail, T, VL).
	
'$postprocess_read_terms_options_list'([], _, _).
'$postprocess_read_terms_options_list'([H|Tail], T, VL) :-
	'$postprocess_read_terms_option'(H, T, VL),
	'$postprocess_read_terms_options_list'(Tail, T, VL).

'$postprocess_read_terms_option'(syntax_errors(_), _, _) :-
	'$get_value'('$read_term_error_handler', OldVal),
	'$set_read_error_handler'(OldVal).
'$postprocess_read_terms_option'(variable_names(Vars), _, VL) :-
	'$read_term_non_anonymous'(VL, Vars).
'$postprocess_read_terms_option'(singletons(Val), T, VL) :-
	'$singletons_in_term'(T, Val1),
	'$fetch_singleton_names'(Val1,VL,Val).
'$postprocess_read_terms_option'(variables(Val), T, _) :-
	'$variables_in_term'(T, [], Val).
%'$postprocess_read_terms_option'(cycles(Val), _, _).

'$read_term_non_anonymous'([], []).
'$read_term_non_anonymous'([[S|V]|VL], [Name=V|Vars]) :-
	atom_codes(Name,S),
	'$read_term_non_anonymous'(VL, Vars).


% problem is what to do about _ singletons.
% no need to do ordering, the two lists already come ordered.
'$fetch_singleton_names'([], _, []).
'$fetch_singleton_names'([_|_], [], []) :- !.
'$fetch_singleton_names'([V1|Ss], [[Na|V2]|Ns], ONs) :-
	V1 == V2, !,
	'$add_singleton_if_no_underscore'(Na,V2,NSs,ONs),
	'$fetch_singleton_names'(Ss, Ns, NSs).
'$fetch_singleton_names'([V1|Ss], [[_|V2]|Ns], NSs) :-
	V1 @> V2, !,
	'$fetch_singleton_names'([V1|Ss], Ns, NSs).
'$fetch_singleton_names'([_V1|Ss], Ns, NSs) :-
%	V1 @> V2,
	'$fetch_singleton_names'(Ss, Ns, NSs).

'$add_singleton_if_no_underscore'([95|_],_,NSs,NSs) :- !.
'$add_singleton_if_no_underscore'(Na,V2,NSs,[(Name=V2)|NSs]) :-
	atom_codes(Name, Na).

/* meaning of flags for '$write' is
	 1	quote illegal atoms
	 2	ignore operator declarations
	 4	output '$VAR'(N) terms as A, B, C, ...
	 8	use portray(_)
*/


nl(Stream) :- '$put'(Stream,10).

nl :- current_output(Stream), '$put'(Stream,10), fail.
nl.

write(T) :-
	'$write'(4, T).

write(Stream,T) :- 
	'$write'(Stream,4,T).

writeq(T) :- '$write'(5,T).

writeq(Stream,T) :-
	'$write'(Stream,5,T),
	fail.
writeq(_,_).

display(T) :- '$write'(2,T).

display(Stream,T) :-
	'$write'(Stream,2,T),
	fail.
display(_,_).

write_canonical(T) :- '$write'(3,T).

write_canonical(Stream,T) :-
	'$write'(Stream,3,T),
	fail.
write_canonical(_,_).

write_term(T,Opts) :-
	'$check_io_opts'(Opts, write_term(T,Opts)),
	'$process_wt_opts'(Opts, 0, Flag, Callbacks),
	'$write'(Flag, T),
	'$process_wt_callbacks'(Callbacks),
	fail.
write_term(_,_).

write_term(S, T, Opts) :-
	'$check_io_opts'(Opts, write_term(T,Opts)),
	'$process_wt_opts'(Opts, 0, Flag, Callbacks),
	'$write'(S, Flag, T),
	'$process_wt_callbacks'(Callbacks),
	fail.
write_term(_,_,_).

'$process_wt_opts'([], Flag, Flag, []).
'$process_wt_opts'([quoted(true)|Opts], Flag0, Flag, CallBacks) :-
	FlagI is Flag0 \/ 1,
	'$process_wt_opts'(Opts, FlagI, Flag, CallBacks).
'$process_wt_opts'([quoted(false)|Opts], Flag0, Flag, CallBacks) :-
	FlagI is Flag0 /\ 14,
	'$process_wt_opts'(Opts, FlagI, Flag, CallBacks).
'$process_wt_opts'([ignore_ops(true)|Opts], Flag0, Flag, CallBacks) :-
	FlagI is Flag0 \/ 2,
	'$process_wt_opts'(Opts, FlagI, Flag, CallBacks).
'$process_wt_opts'([ignore_ops(false)|Opts], Flag0, Flag, CallBacks) :-
	FlagI is Flag0 /\ 13,
	'$process_wt_opts'(Opts, FlagI, Flag, CallBacks).
'$process_wt_opts'([numbervars(true)|Opts], Flag0, Flag, CallBacks) :-
	FlagI is Flag0 \/ 4,
	'$process_wt_opts'(Opts, FlagI, Flag, CallBacks).
'$process_wt_opts'([numbervars(false)|Opts], Flag0, Flag, CallBacks) :-
	FlagI is Flag0 /\ 11,
	'$process_wt_opts'(Opts, FlagI, Flag, CallBacks).
'$process_wt_opts'([portrayed(true)|Opts], Flag0, Flag, CallBacks) :-
	FlagI is Flag0 \/ 8,
	'$process_wt_opts'(Opts, FlagI, Flag, CallBacks).
'$process_wt_opts'([portrayed(false)|Opts], Flag0, Flag, CallBacks) :-
	FlagI is Flag0 /\ 7,
	'$process_wt_opts'(Opts, FlagI, Flag, CallBacks).
'$process_wt_opts'([max_depth(D)|Opts], Flag0, Flag, [max_depth(D1,D0)|CallBacks]) :-
	write_depth(D1,D0),
	write_depth(D,D),
	'$process_wt_opts'(Opts, Flag0, Flag, CallBacks).

'$process_wt_callbacks'([]).
'$process_wt_callbacks'([max_depth(D1,D0)|Cs]) :-
	write_depth(D1,D0),
	'$process_wt_callbacks'(Cs).


print(T) :- '$write'(12,T), fail.
print(_).

print(Stream,T) :-
	'$write'(Stream,12,T),
	fail.
print(_,_).


format(N,A) :- atom(N), !, atom_codes(N, S), '$format'(S,A).
format(F,A) :- '$format'(F,A).

format(Stream, N, A) :- atom(N), !, atom_codes(N, S), '$format'(Stream, S ,A).
format(Stream, S, A) :- '$format'(Stream, S, A).

/* interface to user portray	*/
'$portray'(T) :-
	\+ '$undefined'(portray(_),user),
	user:portray(T), !,
	'$set_value'('$portray',true), fail.
'$portray'(_) :- '$set_value'('$portray',false), fail.

/* character I/O	*/

get(N) :- current_input(S), '$get'(S,N).

get_byte(V) :-
	\+ var(V), (\+ integer(V) ; V < -1 ; V > 256), !,
	throw(error(type_error(in_byte,V),get_byte(V))).
get_byte(V) :-
	current_input(S), 
	'$get_byte'(S,V).

get_byte(S,V) :-
	\+ var(V), (\+ integer(V) ; V < -1 ; V > 256), !,
	throw(error(type_error(in_byte,V),get_byte(S,V))).
get_byte(S,V) :-
	'$get_byte'(S,V).

peek_byte(V) :-
	\+ var(V), (\+ integer(V) ; V < -1 ; V > 256), !,
	throw(error(type_error(in_byte,V),get_byte(V))).
peek_byte(V) :-
	current_input(S), 
	'$peek_byte'(S,V).

peek_byte(S,V) :-
	\+ var(V), (\+ integer(V) ; V < -1 ; V > 256), !,
	throw(error(type_error(in_byte,V),get_byte(S,V))).
peek_byte(S,V) :-
	'$peek_byte'(S,V).

get_char(V) :-
	\+ var(V),
	( atom(V)  -> atom_codes(V,[_,_|_]), V \= end_of_file ; true ), !,
	throw(error(type_error(in_character,V),get_char(V))).
get_char(V) :-
	current_input(S),
	'$get0'(S,I),
	( I = -1 -> V = end_of_file ; atom_codes(V,[I])).

get_char(S,V) :-
	\+ var(V),
	( atom(V)  -> atom_codes(V,[_,_|_]), V \= end_of_file ; true ), !,
	throw(error(type_error(in_character,V),get_char(S,V))).
get_char(S,V) :-
	'$get0'(S,I),
	( I = -1 -> V = end_of_file ; atom_codes(V,[I])).

peek_char(V) :-
	\+ var(V),
	( atom(V)  -> atom_codes(V,[_,_|_]), V \= end_of_file ; true ), !,
	throw(error(type_error(in_character,V),get_char(V))).
peek_char(V) :-
	current_input(S),
	'$peek'(S,I),
	( I = -1 -> V = end_of_file ; atom_codes(V,[I])).

peek_char(S,V) :-
	\+ var(V),
	( atom(V)  -> atom_codes(V,[_,_|_]), V \= end_of_file ; true ), !,
	throw(error(type_error(in_character,V),get_char(S,V))).
peek_char(S,V) :-
	'$peek'(S,I),
	( I = -1 -> V = end_of_file ; atom_codes(V,[I])).

get_code(S,V) :-
	\+ var(V), (\+ integer(V) ; V < -1 ; V > 256), !,
	throw(error(type_error(in_character_code,V),get_code(S,V))).
get_code(S,V) :-
	'$get0'(S,V).

get_code(V) :-
	\+ var(V), (\+ integer(V) ; V < -1 ; V > 256), !,
	throw(error(type_error(in_character_code,V),get_code(V))).
get_code(V) :-
	current_input(S),
	'$get0'(S,V).

peek_code(S,V) :-
	\+ var(V), (\+ integer(V) ; V < -1 ; V > 256), !,
	throw(error(type_error(in_character_code,V),get_code(S,V))).
peek_code(S,V) :-
	'$peek'(S,V).

peek_code(V) :-
	\+ var(V), (\+ integer(V) ; V < -1 ; V > 256), !,
	throw(error(type_error(in_character_code,V),get_code(V))).
peek_code(V) :-
	current_input(S),
	'$peek'(S,V).

put_byte(V) :- var(V), !,
	throw(error(instantiation_error,put_byte(V))).
put_byte(V) :-
	(\+ integer(V) ; V < 0 ; V > 256), !,
	throw(error(type_error(byte,V),put_byte(V))).
put_byte(V) :-
	current_output(S), 
	'$put_byte'(S,V).


put_byte(S,V) :- var(V), !,
	throw(error(instantiation_error,put_byte(S,V))).
put_byte(S,V) :-
	(\+ integer(V) ; V < 0 ; V > 256), !,
	throw(error(type_error(byte,V),put_byte(S,V))).
put_byte(S,V) :-
	'$put_byte'(S,V).

put_char(V) :- var(V), !,
	throw(error(instantiation_error,put_char(V))).
put_char(V) :-
	( atom(V)  -> atom_codes(V,[_,_|_]) ; true ), !,
	throw(error(type_error(character,V),put_char(V))).
put_char(V) :-
	current_output(S),
	atom_codes(V,[I]),
	'$put'(S,I).

put_char(S,V) :- var(V), !,
	throw(error(instantiation_error,put_char(S,V))).
put_char(S,V) :-
	( atom(V)  -> atom_codes(V,[_,_|_]) ; true ), !,
	throw(error(type_error(character,V),put_char(S,V))).
put_char(S,V) :-
	atom_codes(V,[I]),
	'$put'(S,I).

put_code(V) :- var(V), !,
	throw(error(instantiation_error,put_code(V))).
put_code(V) :-
	(\+ integer(V) ; V < 0 ; V > 256), !,
	throw(error(type_error(character_code,V),put_code(V))).
put_code(V) :-
	current_output(S), 
	'$put'(S,V).


put_code(S,V) :- var(V), !,
	throw(error(instantiation_error,put_code(S,V))).
put_code(S,V) :-
	(\+ integer(V) ; V < 0 ; V > 256), !,
	throw(error(type_error(character_code,V),put_code(S,V))).
put_code(S,V) :-
	'$put'(S,V).



get(Stream,N) :- '$get'(Stream,N).

get0(N) :- current_input(S), '$get0'(S,N).

get0(Stream,N) :- '$get0'(Stream,N).

put(N) :- current_output(S),  N1 is N, '$put'(S,N1).

put(Stream,N) :-  N1 is N, '$put'(Stream,N1).

skip(N) :- current_input(S),  N1 is N, '$skip'(S,N1).

skip(Stream,N) :- N1 is N, '$skip'(Stream,N1).

'$tab'(N) :- N<1, !.

'$tab'(N) :- put(32), N1 is N-1, '$tab'(N1).

tab(N) :- '$tab'(N), fail.
tab(_).

'$tab'(_,N) :- N<1, !.
'$tab'(Stream,N) :- put(Stream,32), N1 is N-1, '$tab'(Stream,N1).

tab(Stream,N) :- '$tab'(Stream,N), fail.
tab(_,_).

ttyget(N) :- '$get'(user_input,N).

ttyget0(N) :- '$get0'(user_input,N).

ttyskip(N) :-  N1 is N, '$skip'(user_input,N1).

ttyput(N) :-  N1 is N, '$put'(user_output,N1).

ttynl :- nl(user_output).

ttyflush :- flush_output(user_output).

flush_output :-
	current_output(Stream),
	flush_output(Stream).

current_line_number(N) :-
	current_input(Stream), '$current_line_number'(Stream,N).

current_line_number(user,N) :- !,
	'$current_line_number'(user_input,N).
current_line_number(A,N) :- 
	atom(A),
	current_stream(_,_,S), '$user_file_name'(S,A), !,
	'$current_line_number'(S,N).
current_line_number(S,N) :-
	'$current_line_number'(S,N).

line_count(Stream,N) :- current_line_number(Stream,N).

character_count(user,N) :- !,
	'$character_count'(user_input,N).
character_count(A,N) :- 
	atom(A),
	current_stream(_,_,S), '$user_file_name'(S,A), !,
	'$character_count'(S,N).
character_count(S,N) :-
	'$character_count'(S,N).

line_position(user,N) :- !,
	'$line_position'(user_input,N).
line_position(A,N) :- 
	atom(A),
	current_stream(_,_,S), '$user_file_name'(S,A), !,
	'$line_position'(S,N).
line_position(S,N) :-
	'$line_position'(S,N).

stream_position(user,N) :- !,
	'$show_stream_position'(user_input,N).
stream_position(A,N) :- 
	atom(A),
	'$current_stream'(_,_,S), '$user_file_name'(S,A), !,
	'$show_stream_position'(S,N).
stream_position(S,N) :-
	'$show_stream_position'(S,N).

stream_position(user,N,M) :- !,
	'$stream_position'(user_input,N,M).
stream_position(A,N,M) :- 
	atom(A),
	'$current_stream'(_,_,S), '$user_file_name'(S,A), !,
	'$stream_position'(S,N,M).
stream_position(S,N,M) :-
	'$stream_position'(S,N,M).

'$stream_position'(S,N,M) :-
	var(M), !,
	'$show_stream_position'(S,N),
	M = N.
'$stream_position'(S,N,M) :-
	'$show_stream_position'(S,N),
	'$set_stream_position'(S,M).


set_stream_position(S,N) :- var(S), !,
	throw(error(instantiation_error, set_stream_position(S, N))).
set_stream_position(user,N) :- !,
	'$set_stream_position'(user_input,N).
set_stream_position(A,N) :- 
	atom(A),
	'$current_stream'(_,_,S), '$user_file_name'(S,A), !,
	'$set_stream_position'(S,N).
set_stream_position(S,N) :-
	'$set_stream_position'(S,N).

stream_property(Stream, Prop) :-  var(Prop), !,
        (var(Stream) -> '$current_stream'(_,_,Stream) ; true),
        '$generate_prop'(Prop),
	'$stream_property'(Stream, Prop).
stream_property(Stream, Props) :-  var(Stream), !,
	'$current_stream'(_,_,Stream),
	'$stream_property'(Stream, Props), !.
stream_property(Stream, Props) :-
	'$stream_property'(Stream, Props).
stream_property(Stream, Props) :-
	throw(error(domain_error(stream,Stream),stream_property(Stream, Props))).

'$generate_prop'(file_name(_F)).
'$generate_prop'(mode(_M)).
'$generate_prop'(input).
'$generate_prop'(output).
'$generate_prop'(position(_P)).
%'$generate_prop'(end_of_stream(_E)).
'$generate_prop'(eof_action(_E)).
%'$generate_prop'(reposition(_R)).
'$generate_prop'(type(_T)).
'$generate_prop'(alias(_A)).

'$stream_property'(Stream, Props) :-
	var(Props), !,
	throw(error(instantiation_error, stream_properties(Stream, Props))).
'$stream_property'(Stream, Props0) :-
	'$check_stream_props'(Props0, Props),
	'$check_io_opts'(Props, stream_property(Stream, Props)),
	'$current_stream'(F,Mode,Stream),
	'$process_stream_properties'(Props, Stream, F, Mode).

'$check_stream_props'([], []) :- !.
'$check_stream_props'([H|T], [H|T]) :- !.
'$check_stream_props'(Prop, [Prop]).


'$process_stream_properties'([], _, _, _).
'$process_stream_properties'([file_name(F)|Props], Stream, F, Mode) :-
	'$process_stream_properties'(Props, Stream, F, Mode).
'$process_stream_properties'([mode(Mode)|Props], Stream, F, Mode) :-
	'$process_stream_properties'(Props, Stream, F, Mode).
'$process_stream_properties'([input|Props], Stream, F, read) :-
	'$process_stream_properties'(Props, Stream, F, read).
'$process_stream_properties'([output|Props], Stream, F, append) :-
	'$process_stream_properties'(Props, Stream, F, append).
'$process_stream_properties'([output|Props], Stream, F, write) :-
	'$process_stream_properties'(Props, Stream, F, write).
'$process_stream_properties'([position(P)|Props], Stream, F, Mode) :-
	'$show_stream_position'(Stream, P),
	'$process_stream_properties'(Props, Stream, F, Mode).
'$process_stream_properties'([end_of_stream(P)|Props], Stream, F, Mode) :-
	'$show_stream_eof'(Stream, P),
	'$process_stream_properties'(Props, Stream, F, Mode).
'$process_stream_properties'([eof_action(P)|Props], Stream, F, Mode) :-
	'$show_stream_flags'(Stream, Fl),
	'$show_stream_eof_action'(Fl, P),
	'$process_stream_properties'(Props, Stream, F, Mode).
'$process_stream_properties'([reposition(P)|Props], Stream, F, Mode) :-
	'$show_stream_flags'(Stream, Fl),
	'$show_stream_reposition'(Fl, P),
	'$process_stream_properties'(Props, Stream, F, Mode).
'$process_stream_properties'([type(P)|Props], Stream, F, Mode) :-
	'$show_stream_flags'(Stream, Fl),
	'$show_stream_type'(Fl, P),
	'$process_stream_properties'(Props, Stream, F, Mode).
'$process_stream_properties'([alias(Alias)|Props], Stream, F, Mode) :-
	'$fetch_stream_alias'(Stream, Alias),
	'$process_stream_properties'(Props, Stream, F, Mode).

'$show_stream_eof'(Stream, past) :-
	'$past_eof'(Stream), !.
'$show_stream_eof'(Stream, at) :-
	'$peek'(Stream,N), N = -1, !.
'$show_stream_eof'(_, not).
	
'$show_stream_eof_action'(Fl, error) :-
	Fl /\ 16'0200 =:= 16'0200, !.
'$show_stream_eof_action'(Fl, reset) :-
	Fl /\ 16'0400 =:= 16'0400, !.
'$show_stream_eof_action'(_, eof_code).

'$show_stream_reposition'(Fl, true) :-
	Fl /\ 16'2000 =:= 16'2000, !.
'$show_stream_reposition'(_, false).

'$show_stream_type'(Fl, binary) :-
	Fl /\ 16'0100 =:= 16'0100, !.
'$show_stream_type'(_, text).

at_end_of_stream :-
	current_input(S),
	at_end_of_stream(S).

at_end_of_stream(S) :-
	'$past_eof'(S), !.
at_end_of_stream(S) :-
	'$peek'(S,N), N = -1.


consult_depth(LV) :- '$show_consult_level'(LV).

absolute_file_name(V,Out) :- var(V), !,
	throw(error(instantiation_error, absolute_file_name(V, Out))).
absolute_file_name(user,user) :- !.
absolute_file_name(RelFile,AbsFile) :-
	'$find_in_path'(RelFile,PathFile,absolute_file_name(RelFile,AbsFile)),
	'$exists'(PathFile,'$csult', AbsFile), !.
absolute_file_name(RelFile, AbsFile) :-
	'$file_expansion'(RelFile, AbsFile).

'$exists'(F,Mode,AbsFile) :-
	'$get_value'(fileerrors,V),
	'$set_value'(fileerrors,0),
	( '$open'(F,Mode,S,0), !,
	    '$file_name'(S, AbsFile),
	     '$close'(S), '$set_value'(fileerrors,V);
	     '$set_value'(fileerrors,V), fail).


current_char_conversion(X,Y) :-
	var(X), !,
	'$all_char_conversions'(List),
	'$fetch_char_conversion'(List,X,Y).
current_char_conversion(X,Y) :-
	'$current_char_conversion'(X,Y).


'$fetch_char_conversion'([X,Y|_],X,Y).
'$fetch_char_conversion'([_,_|List],X,Y) :-
	'$fetch_char_conversion'(List,X,Y).


current_stream(File, Opts, Stream) :-
	'$current_stream'(File, Opts, Stream).

	

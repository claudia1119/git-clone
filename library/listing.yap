/* 

 emulates listing.pl, but just the interface for now.

*/


:- module(swi_listing,
	[ listing/0,
	  listing/1,
	  portray_clause/1,		% +Clause
	  portray_clause/2,		% +Stream, +Clause
	  portray_clause/3		% +Stream, +Clause, +Options
	]).


:- meta_predicate portray_clause( +, + , : ).

portray_clause(Stream, Term, M:Options) :-
    portray_clause( Stream, Term ).

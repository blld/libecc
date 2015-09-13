//
//  lexer.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef io_libecc_lexer_h
#define io_libecc_lexer_h

#include "namespace_io_libecc.h"

#include "input.h"
#include "value.h"
#include "error.h"


#include "interface.h"

#define Module \
	io_libecc_Lexer

#define io_libecc_Lexer(X) io_libecc_lexer_ ## X

#define io_libecc_lexer_Tokens \
	_( no ,"end of script",= 0)\
	_( error ,,= 128)\
	_( todo ,,)\
	\
	_( integer ,,)\
	_( binary ,,)\
	_( string ,,)\
	_( identifier ,,)\
	_( null ,,)\
	_( true ,,)\
	_( false ,,)\
	_( regex ,,)\
	\
	_( this ,,)\
	_( instanceof ,,)\
	_( typeof ,,)\
	_( arguments ,,)\
	_( break ,,)\
	_( case ,,)\
	_( default ,,)\
	_( catch ,,)\
	_( continue ,,)\
	_( debugger ,,)\
	_( delete ,,)\
	_( do ,,)\
	_( else ,,)\
	_( finally ,,)\
	_( for ,,)\
	_( function ,,)\
	_( if ,,)\
	_( in ,,)\
	_( new ,,)\
	_( return ,,)\
	_( switch ,,)\
	_( throw ,,)\
	_( try ,,)\
	_( var ,,)\
	_( void ,,)\
	_( while ,,)\
	_( with ,,)\
	_( get ,,)\
	_( set ,,)\
	\
	_( equal ,"'=='",)\
	_( notEqual ,"'!='",)\
	_( identical ,"'==='",)\
	_( notIdentical ,"'!=='",)\
	_( leftShiftAssign ,"'<<='",)\
	_( rightShiftAssign ,"'>>='",)\
	_( unsignedRightShiftAssign ,"'>>>='",)\
	_( leftShift ,"'<<'",)\
	_( rightShift ,"'>>'",)\
	_( unsignedRightShift ,"'>>>'",)\
	_( lessOrEqual ,"'<='",)\
	_( moreOrEqual ,"'>='",)\
	_( increment ,"'++'",)\
	_( decrement ,"'--'",)\
	_( logicalAnd ,"'&&'",)\
	_( logicalOr ,"'||'",)\
	_( addAssign ,"'+='",)\
	_( minusAssign ,"'-='",)\
	_( multiplyAssign ,"'*='",)\
	_( divideAssign ,"'/='",)\
	_( moduloAssign ,"'%='",)\
	_( andAssign ,"'&='",)\
	_( orAssign ,"'|='",)\
	_( xorAssign ,"'^='",)\
	\

#define _(X, S, ...) Module(X ## Token) __VA_ARGS__,
enum Module(Token) {
	io_libecc_lexer_Tokens
};
#undef _

Interface(
	(Instance, createWithInput ,(struct Input *))
	(void, destroy, (Instance))
	(enum Lexer(Token), nextToken, (Instance))
	(const char *, tokenChars ,(enum Module(Token) token))
	,
	{
		struct Input *input;
		uint32_t offset;
		
		struct Value value;
		struct Text text;
		int didLineBreak;
		int disallowRegex;
	}
)

#endif

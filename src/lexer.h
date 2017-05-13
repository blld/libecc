//
//  lexer.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef io_libecc_lexer_h
#ifdef Implementation
#undef Implementation
#include __FILE__
#include "implementation.h"
#else
#include "interface.h"
#define io_libecc_lexer_h

	#include "builtin/error.h"
	#include "input.h"

	#define io_libecc_lexer_Tokens \
		_( no ,"end of script",= 0)\
		_( error ,,= 128)\
		\
		/* literal */\
		_( null ,,)\
		_( true ,,)\
		_( false ,,)\
		_( integer ,"number",)\
		_( binary ,"number",)\
		_( string ,,)\
		_( identifier ,,)\
		_( regexp ,,)\
		\
		/* keyword */\
		_( break ,,)\
		_( case ,,)\
		_( catch ,,)\
		_( continue ,,)\
		_( debugger ,,)\
		_( default ,,)\
		_( delete ,,)\
		_( do ,,)\
		_( else ,,)\
		_( finally ,,)\
		_( for ,,)\
		_( function ,,)\
		_( if ,,)\
		_( in ,,)\
		_( instanceof ,,)\
		_( new ,,)\
		_( return ,,)\
		_( switch ,,)\
		_( this ,,)\
		_( throw ,,)\
		_( try ,,)\
		_( typeof ,,)\
		_( var ,,)\
		_( void ,,)\
		_( with ,,)\
		_( while ,,)\
		\
		/* operator */\
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

	#define _(X, S, V) Lexer(X ## Token) V,
	enum Lexer(Token) {
		io_libecc_lexer_Tokens
	};
	#undef _

#endif


Interface(Lexer,
	
	(struct Lexer *, createWithInput ,(struct Input *))
	(void, destroy, (struct Lexer *))
	
	(enum Lexer(Token), nextToken, (struct Lexer *))
	
	(const char *, tokenChars ,(enum Lexer(Token) token))
	
	(struct Value, parseBinary ,(struct Text text))
	(struct Value, parseInteger ,(struct Text text, int base))
	(uint32_t, parseElement ,(struct Text text))
	
	(uint8_t, uint8Hex ,(char a, char b))
	(uint16_t, uint16Hex ,(char a, char b, char c, char d))
	,
	{
		struct Input *input;
		uint32_t offset;
		
		struct Value value;
		struct Text text;
		int didLineBreak;
		int allowRegex;
		int disallowKeyword;
	}
)

#endif

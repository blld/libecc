//
//  op.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef io_libecc_op_h
#define io_libecc_op_h

#include "namespace_io_libecc.h"

#include "native.h"
#include "value.h"
#include "object/function.h"

#include "interface.h"


#define io_libecc_op_List \
	\
	/* expression */\
	_( noop )\
	_( value )\
	_( valueConstRef )\
	_( text )\
	_( function )\
	_( object )\
	_( array )\
	_( this )\
	\
	_( getLocal )\
	_( getLocalRef )\
	_( setLocal )\
	\
	_( getLocalSlot )\
	_( getLocalSlotRef )\
	_( setLocalSlot )\
	\
	_( getParentSlot )\
	_( getParentSlotRef )\
	_( setParentSlot )\
	\
	_( getMember )\
	_( getMemberRef )\
	_( setMember )\
	_( deleteMember )\
	\
	_( getProperty )\
	_( getPropertyRef )\
	_( setProperty )\
	_( deleteProperty )\
	\
	_( result )\
	_( resultValue )\
	_( pushContext )\
	_( popContext )\
	_( exchange )\
	_( typeOf )\
	_( equal )\
	_( notEqual )\
	_( identical )\
	_( notIdentical )\
	_( less )\
	_( lessOrEqual )\
	_( more )\
	_( moreOrEqual )\
	_( instanceOf )\
	_( in )\
	_( multiply )\
	_( divide )\
	_( modulo )\
	_( add )\
	_( minus )\
	_( multiplyBinary )\
	_( divideBinary )\
	_( moduloBinary )\
	_( addBinary )\
	_( minusBinary )\
	_( leftShift )\
	_( rightShift )\
	_( unsignedRightShift )\
	_( bitwiseAnd )\
	_( bitwiseXor )\
	_( bitwiseOr )\
	_( logicalAnd )\
	_( logicalOr )\
	_( positive )\
	_( negative )\
	_( invert )\
	_( not )\
	_( construct )\
	_( call )\
	_( eval )\
	\
	/* assignement expression */\
	_( incrementRef )\
	_( decrementRef )\
	_( postIncrementRef )\
	_( postDecrementRef )\
	_( multiplyAssignRef )\
	_( divideAssignRef )\
	_( moduloAssignRef )\
	_( addAssignRef )\
	_( minusAssignRef )\
	_( leftShiftAssignRef )\
	_( rightShiftAssignRef )\
	_( unsignedRightShiftAssignRef )\
	_( bitAndAssignRef )\
	_( bitXorAssignRef )\
	_( bitOrAssignRef )\
	\
	/* statement */\
	_( try )\
	_( throw )\
	_( debug )\
	_( next )\
	_( nextIf )\
	_( expression )\
	_( discard )\
	_( discard2 )\
	_( discard3 )\
	_( discard4 )\
	_( discard5 )\
	_( discard6 )\
	_( discard7 )\
	_( discard8 )\
	_( discard9 )\
	_( discard10 )\
	_( discard11 )\
	_( discard12 )\
	_( discard13 )\
	_( discard14 )\
	_( discard15 )\
	_( discard16 )\
	_( jump )\
	_( jumpIf )\
	_( jumpIfNot )\
	_( switchOp )\
	_( iterate )\
	_( iterateLessRef )\
	_( iterateMoreRef )\
	_( iterateLessOrEqualRef )\
	_( iterateMoreOrEqualRef )\
	_( iterateInRef )\


#define _(X) (struct Value, X , (const struct Op ** const ops, struct Ecc * const ecc))
Interface(Op,
	
	(struct Op, make ,(const Native native, struct Value value, struct Text text))
	(const char *, toChars ,(const Native native))
	
	(void , assertParameterCount ,(struct Ecc * const ecc, int parameterCount))
	(int , argumentCount ,(struct Ecc * const ecc))
	(struct Value, argument ,(struct Ecc * const ecc, int argumentIndex))
	
	(void , assertVariableParameter ,(struct Ecc * const ecc))
	(int , variableArgumentCount ,(struct Ecc * const ecc))
	(struct Value, variableArgument ,(struct Ecc * const ecc, int argumentIndex))
	
	(struct Value, callFunctionArguments ,(struct Function *function, struct Ecc * const ecc, struct Value this, struct Object *arguments))
	(struct Value, callFunctionVA ,(struct Function *function, struct Ecc * const ecc, struct Value this, int argumentCount, ... ))
	
	io_libecc_op_List
	,
	{
		Native native;
		struct Value value;
		struct Text text;
	}
)
#undef _

#include "oplist.h"

#endif

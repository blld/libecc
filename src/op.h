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

#include "function.h"
#include "value.h"
#include "closure.h"


#include "interface.h"

#define Module \
	io_libecc_Op

#define io_libecc_op_List \
	\
	/* expression */\
	_( noop )\
	_( value )\
	_( valueConstRef )\
	_( text )\
	_( closure )\
	_( object )\
	_( array )\
	_( this )\
	\
	_( getLocal )\
	_( getLocalRef )\
	_( setLocal )\
	_( deleteLocal )\
	\
	_( getLocalSlot )\
	_( getLocalSlotRef )\
	_( setLocalSlot )\
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

#define _(X) (struct Value, X , (const Instance * const ops, struct Ecc * const ecc))
Interface(
	(struct Op, make ,(const Function function, struct Value value, struct Text text))
	(const char *, toChars ,(const Function function))
	
	(void , assertParameterCount ,(struct Ecc * const ecc, int parameterCount))
	(int , argumentCount ,(struct Ecc * const ecc))
	(struct Value, argument ,(struct Ecc * const ecc, int argumentIndex))
	
	(void , assertVariableParameter ,(struct Ecc * const ecc))
	(int , variableArgumentCount ,(struct Ecc * const ecc))
	(struct Value, variableArgument ,(struct Ecc * const ecc, int argumentIndex))
	
	(struct Value, callClosureVA ,(struct Closure *closure, struct Ecc * const ecc, struct Value this, int argumentCount, ... ))
	
	io_libecc_op_List
	,
	{
		const Function function;
		const struct Value value;
		const struct Text text;
	}
)
#undef _

#include "oplist.h"
#include "interface.h"
#define Module io_libecc_Op

#endif

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
#include "builtin/function.h"

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
	_( callMember )\
	_( deleteMember )\
	\
	_( getProperty )\
	_( getPropertyRef )\
	_( setProperty )\
	_( callProperty )\
	_( deleteProperty )\
	\
	_( result )\
	_( resultValue )\
	_( pushEnvironment )\
	_( popEnvironment )\
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
	_( discardN )\
	_( jump )\
	_( jumpIf )\
	_( jumpIfNot )\
	_( switchOp )\
	_( breaker )\
	_( iterate )\
	_( iterateLessRef )\
	_( iterateMoreRef )\
	_( iterateLessOrEqualRef )\
	_( iterateMoreOrEqualRef )\
	_( iterateInRef )\


#define _(X) (struct Value, X , (struct Native(Context) * const))
Interface(Op,
	
	(struct Op, make ,(const Native(Function) native, struct Value value, struct Text text))
	(const char *, toChars ,(const Native(Function) native))
	
	(struct Value, getRefValue ,(struct Native(Context) * const context, struct Value *ref, struct Value this))
	(struct Value, setRefValue ,(struct Native(Context) * const context, struct Value *ref, struct Value this, struct Value value, const struct Text *text))
	
	(struct Value, callFunctionArguments ,(struct Native(Context) * const, int argumentOffset, struct Function *function, struct Value this, struct Object *arguments))
	(struct Value, callFunctionVA ,(struct Native(Context) * const, int argumentOffset, struct Function *function, struct Value this, int argumentCount, ... ))
	
	io_libecc_op_List
	,
	{
		Native(Function) native;
		struct Value value;
		struct Text text;
	}
)
#undef _

#include "oplist.h"

#endif

//
//  op.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef io_libecc_op_h
#ifdef Implementation
#undef Implementation
#include __FILE__
#include "implementation.h"
#else
#include "interface.h"
#define io_libecc_op_h

	#include "builtin/function.h"

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
		_( getLocalRef )\
		_( getLocal )\
		_( setLocal )\
		\
		_( getLocalSlotRef )\
		_( getLocalSlot )\
		_( setLocalSlot )\
		\
		_( getParentSlotRef )\
		_( getParentSlot )\
		_( setParentSlot )\
		\
		_( getMemberRef )\
		_( getMember )\
		_( setMember )\
		_( callMember )\
		_( deleteMember )\
		\
		_( getPropertyRef )\
		_( getProperty )\
		_( setProperty )\
		_( callProperty )\
		_( deleteProperty )\
		\
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
		_( add )\
		_( minus )\
		_( multiply )\
		_( divide )\
		_( modulo )\
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
		_( addAssignRef )\
		_( minusAssignRef )\
		_( multiplyAssignRef )\
		_( divideAssignRef )\
		_( moduloAssignRef )\
		_( leftShiftAssignRef )\
		_( rightShiftAssignRef )\
		_( unsignedRightShiftAssignRef )\
		_( bitAndAssignRef )\
		_( bitXorAssignRef )\
		_( bitOrAssignRef )\
		\
		/* statement */\
		_( debugger )\
		_( try )\
		_( throw )\
		_( next )\
		_( nextIf )\
		_( autoreleaseExpression )\
		_( autoreleaseDiscard )\
		_( expression )\
		_( discard )\
		_( discardN )\
		_( jump )\
		_( jumpIf )\
		_( jumpIfNot )\
		_( result )\
		_( resultVoid )\
		_( switchOp )\
		_( breaker )\
		_( iterate )\
		_( iterateLessRef )\
		_( iterateMoreRef )\
		_( iterateLessOrEqualRef )\
		_( iterateMoreOrEqualRef )\
		_( iterateInRef )\

#endif


#define _(X) (struct Value, X , (struct Context * const))
Interface(Op,
	
	(struct Op, make ,(const Native(Function) native, struct Value value, struct Text text))
	(const char *, toChars ,(const Native(Function) native))
	
	(struct Value, callFunctionArguments ,(struct Context * const, int argumentOffset, struct Function *function, struct Value this, struct Object *arguments))
	(struct Value, callFunctionVA ,(struct Context * const, int argumentOffset, struct Function *function, struct Value this, int argumentCount, va_list ap))
	
	io_libecc_op_List
	,
	{
		Native(Function) native;
		struct Value value;
		struct Text text;
	}
)
#undef _

#endif

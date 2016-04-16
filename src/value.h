//
//  value.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef io_libecc_value_h
#ifdef Implementation
#undef Implementation
#include __FILE__
#include "implementation.h"
#else
#include "interface.h"
#define io_libecc_value_h

	struct Context;

	#include "key.h"

	enum Value(Type) {
		
		/* Primitive */
		
		/* false */
		
		/* 1000 0110 */ Value(nullType) = -1 & ~0x79,
		/* 1010 0110 */ Value(falseType) = -1 & ~0x59,
		/* 0000 0000 */ Value(undefinedType) = 0x00,
		
		/* computed truth */
		
		/* 0000 1000 */ Value(integerType) = 0x08,
		/* 0000 1010 */ Value(binaryType) = 0x0a,
		
		/* 0001 0000 */ Value(keyType) = 0x10,
		/* 0001 0010 */ Value(textType) = 0x12,
		/* 0001 0011 */ Value(charsType) = 0x13,
		
		/* true */
		
		/* 0010 0000 */ Value(trueType) = 0x20,
		
		/* Objects */
		
		/* 0100 0000 */ Value(objectType) = 0x40,
		/* 0100 0001 */ Value(errorType) = 0x41,
		/* 0100 0010 */ Value(functionType) = 0x42,
		/* 0100 0100 */ Value(dateType) = 0x44,
		/* 0100 1000 */ Value(numberType) = 0x48,
		/* 0101 0000 */ Value(stringType) = 0x50,
		/* 0110 0000 */ Value(booleanType) = 0x60,
		
		/* 0100 0110 */ Value(hostType) = 0x46,
		/* 0100 0111 */ Value(referenceType) = 0x47,
	};

	enum Value(Mask) {
		/* 0000 1000 */ Value(numberMask) = 0x08,
		/* 0001 0000 */ Value(stringMask) = 0x10,
		/* 0010 0000 */ Value(booleanMask) = 0x20,
		/* 0100 0000 */ Value(objectMask) = 0x40,
		/* 0100 0001 */ Value(dynamicMask) = 0x41,
	};

	enum Value(Flags)
	{
		Value(readonly) = 1 << 0,
		Value(hidden) = 1 << 1,
		Value(sealed) = 1 << 2,
		Value(getter) = 1 << 3,
		Value(setter) = 1 << 4,
		
		Value(frozen) = Value(readonly) | Value(sealed),
		Value(accessor) = Value(getter) | Value(setter),
	};

	enum Value(hintPrimitive)
	{
		Value(hintAuto) = 0,
		Value(hintString) = 1,
		Value(hintNumber) = -1,
	};

	extern const struct Value Value(undefined);
	extern const struct Value Value(true);
	extern const struct Value Value(false);
	extern const struct Value Value(null);

#endif


Interface(Value,
	
	(struct Value, none ,(void))
	(struct Value, truth ,(int truth))
	(struct Value, integer ,(int32_t integer))
	(struct Value, binary ,(double binary))
	(struct Value, key ,(struct Key key))
	(struct Value, text ,(const struct Text *text))
	(struct Value, chars ,(struct Chars *chars))
	(struct Value, object ,(struct Object *))
	(struct Value, error ,(struct Error *))
	(struct Value, string ,(struct String *))
	(struct Value, number ,(struct Number *))
	(struct Value, boolean ,(struct Boolean *))
	(struct Value, date ,(struct Date *))
	(struct Value, function ,(struct Function *))
	(struct Value, host ,(struct Object *))
	(struct Value, reference ,(struct Value *))
	
	(int, isPrimitive ,(struct Value))
	(int, isBoolean ,(struct Value))
	(int, isNumber ,(struct Value))
	(int, isString ,(struct Value))
	(int, isObject ,(struct Value))
	(int, isDynamic ,(struct Value))
	(int, isTrue ,(struct Value))
	
	(struct Value, toPrimitive ,(struct Context * const, struct Value, enum Value(hintPrimitive)))
	
	(struct Value, toBinary ,(struct Value))
	(struct Value, toInteger ,(struct Value))
	(struct Value, binaryToString ,(double binary, int base))
	
	(struct Value, toString ,(struct Context * const, struct Value))
	(uint16_t, stringLength ,(struct Value))
	(const char *, stringBytes ,(struct Value))
	
	(struct Value, toObject ,(struct Context * const, struct Value))
	
	(struct Value, toType ,(struct Value))
	
	(struct Value, equals ,(struct Context * const, struct Value, struct Value))
	(struct Value, same ,(struct Context * const, struct Value, struct Value))
	(struct Value, add ,(struct Context * const, struct Value, struct Value))
	(struct Value, subtract ,(struct Context * const, struct Value, struct Value))
	(struct Value, compare ,(struct Context * const, struct Value, struct Value))
	(struct Value, less ,(struct Context * const, struct Value, struct Value))
	(struct Value, more ,(struct Context * const, struct Value, struct Value))
	(struct Value, lessOrEqual ,(struct Context * const, struct Value, struct Value))
	(struct Value, moreOrEqual ,(struct Context * const, struct Value, struct Value))
	
	(const char *, typeName ,(enum Value(Type)))
	(const char *, maskName ,(enum Value(Mask)))
	
	(void, dumpTo ,(struct Value, FILE *))
	,
	{
		union
		{
			int32_t integer;
			double binary;
			struct Key key;
			const struct Text *text;
			
			struct Chars *chars;
			
			struct Object *object;
			struct Error *error;
			struct String *string;
			struct Number *number;
			struct Boolean *boolean;
			struct Date *date;
			struct Function *function;
			
			struct Value *reference;
		} data;
		int8_t type;
		uint8_t flags;
		uint16_t check;
	}
)

#endif

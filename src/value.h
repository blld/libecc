//
//  value.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef io_libecc_value_h
#define io_libecc_value_h

#include "namespace_io_libecc.h"
struct Function;
struct Ecc;
struct Op;
struct Native(Context);

#include "key.h"

#include "interface.h"


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
	
	Value(frozen) = Value(readonly) | Value(sealed),
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
	
	(struct Value, toPrimitive ,(struct Native(Context) * const, struct Value, const struct Text *, enum Value(hintPrimitive)))
	
	(struct Value, toBinary ,(struct Value))
	(struct Value, toInteger ,(struct Value))
	(struct Value, binaryToString ,(double binary, int base))
	
	(struct Value, toString ,(struct Native(Context) * const, struct Value))
	(uint16_t, toLength ,(struct Native(Context) * const, struct Value))
	(uint16_t, toBytes ,(struct Native(Context) * const, struct Value, char *bytes))
	(uint16_t, stringLength ,(struct Value))
	(const char *, stringBytes ,(struct Value))
	
	(struct Value, toObject ,(struct Native(Context) * const, struct Value, int argumentIndex))
	
	(struct Value, toType ,(struct Value))
	
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


// import object (cyclic dependency)
#ifndef io_libecc_object_h
#ifndef io_libecc_lexer_h
#include "native.h"
#include "object/function.h"
#include "object/object.h"
#include "object/string.h"
#include "object/boolean.h"
#include "object/number.h"
#include "object/array.h"
#include "object/arguments.h"
#include "object/date.h"
#include "object/error.h"
#include "ecc.h"
#endif

#endif

#endif

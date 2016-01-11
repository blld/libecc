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

#include "key.h"

#include "interface.h"


enum Value(Type) {
	
	/* Primitive */
	
	/* static */
	
	Value(nullType) = -2 & ~0x70,
	Value(falseType) = -1 & ~0x70,
	Value(undefinedType) = 0x00,
	Value(trueType) = 0x01,
	
	Value(breakerType) = 0x02,
	
	Value(integerType) = 0x10,
	Value(binaryType) = 0x12,
	
	Value(keyType) = 0x20,
	Value(textType) = 0x22,
	
	/* dynamic */
	
	Value(charsType) = 0x24,
	
	/* Objects */
	
	Value(objectType) = 0x40,
	Value(booleanType) = 0x41,
	Value(errorType) = 0x42,
	Value(functionType) = 0x44,
	Value(stringType) = 0x46,
	Value(dateType) = 0x48,
	Value(numberType) = 0x50,
	
	Value(referenceType) = 0x4e,
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
	(struct Value, breaker ,(int32_t integer))
	(struct Value, reference ,(struct Value *))
	
	(struct Value, toPrimitive ,(const struct Op ** const, struct Ecc *, struct Value, const struct Text *, enum Value(hintPrimitive)))
	(int, isPrimitive ,(struct Value))
	(int, isBoolean ,(struct Value))
	(int, isDynamic ,(struct Value))
	(int, isTrue ,(struct Value))
	
	(struct Value, toString ,(struct Value))
	(struct Value, binaryToString ,(double binary, int base))
	(uint16_t, toBufferLength ,(struct Value))
	(uint16_t, toBuffer ,(struct Value, char *buffer, uint16_t length))
	(int, isString ,(struct Value))
	(const char *, stringChars ,(struct Value))
	(uint16_t, stringLength ,(struct Value))
	
	(struct Value, toBinary ,(struct Value))
	(struct Value, toInteger ,(struct Value))
	(int, isNumber ,(struct Value))
	
	(struct Value, toObject ,(const struct Op ** const, struct Ecc *, struct Value, int argumentIndex))
	(int, isObject ,(struct Value))
	
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
#include "object/function.h"
#include "object/object.h"
#include "object/string.h"
#include "object/boolean.h"
#include "object/number.h"
#include "object/array.h"
#include "object/date.h"
#include "object/error.h"
#include "ecc.h"
#endif

#endif

#endif

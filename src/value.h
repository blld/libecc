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

#include "key.h"


#define io_libecc_Value(X) io_libecc_value_ ## X

enum Value(Type) {
	
	/* Primitive */
	
	/* static */
	
	Value(null) = -2 & ~0x70,
	Value(false) = -1 & ~0x70,
	Value(undefined) = 0x00,
	Value(true) = 0x01,
	
	Value(breaker) = 0x02,
	
	Value(integer) = 0x10,
	Value(binary) = 0x12,
	
	Value(key) = 0x20,
	Value(text) = 0x22,
	
	/* dynamic */
	
	Value(chars) = 0x24,
	
	/* Objects */
	
	Value(object) = 0x40,
	Value(error) = 0x42,
	Value(function) = 0x44,
	Value(string) = 0x46,
	Value(date) = 0x48,
	
	Value(reference) = 0x4e,
};


#include "interface.h"

Interface(Value,
	
	(struct Value, undefined ,(void))
	(struct Value, null ,(void))
	(struct Value, false ,(void))
	(struct Value, true ,(void))
	(struct Value, boolean ,(int))
	(struct Value, integer ,(int32_t integer))
	(struct Value, binary ,(double binary))
	(struct Value, key ,(struct Key key))
	(struct Value, text ,(const struct Text *text))
	(struct Value, chars ,(struct Chars *chars))
	(struct Value, object ,(struct Object *))
	(struct Value, error ,(struct Error *))
	(struct Value, string ,(struct String *))
	(struct Value, date ,(struct Date *))
	(struct Value, function ,(struct Function *))
	(struct Value, breaker ,(int32_t integer))
	(struct Value, reference ,(struct Value *))
	
	(struct Value, toPrimitive ,(struct Value, struct Ecc *ecc, const struct Text *text, int hint /* 0: auto, 1: String, -1: Number */))
	(int, isPrimitive ,(struct Value))
	(int, isBoolean ,(struct Value))
	(int, isDynamic ,(struct Value))
	(int, isTrue ,(struct Value))
	
	(struct Value, toString ,(struct Value))
	(int, isString ,(struct Value))
	(const char *, stringChars ,(struct Value))
	(uint16_t, stringLength ,(struct Value))
	
	(struct Value, toBinary ,(struct Value))
	(struct Value, toInteger ,(struct Value))
	(int, isNumber ,(struct Value))
	
	(struct Value, toObject ,(struct Value, struct Ecc *ecc, const struct Text *text))
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
			struct Date *date;
			struct Function *function;
			
			struct Value *reference;
		} data;
		enum Value(Type) type;
	}
)


// import object and redefine correct module (cyclic dependency)
#ifndef io_libecc_object_h
#ifndef io_libecc_lexer_h
#include "object/function.h"
#include "object/object.h"
#include "object/string.h"
#include "object/array.h"
#include "object/date.h"
#include "object/error.h"
#include "ecc.h"
#endif

#endif

#endif

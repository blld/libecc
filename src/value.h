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

#include "identifier.h"


#include "interface.h"

#define Module \
	io_libecc_Value

#define io_libecc_Value(X) io_libecc_value_ ## X

enum Module(Type) {
	
	/* Primitive */
	
	/* static */
	
	Module(null) = -2 & ~0x70,
	Module(false) = -1 & ~0x70,
	Module(undefined) = 0x00,
	Module(true) = 0x01,
	
	Module(breaker) = 0x02,
	
	Module(integer) = 0x10,
	Module(binary) = 0x12,
	
	Module(identifier) = 0x20,
	Module(text) = 0x22,
	
	/* dynamic */
	
	Module(chars) = 0x24,
	
	/* Objects */
	
	Module(object) = 0x40,
	Module(error) = 0x42,
	Module(function) = 0x44,
	Module(string) = 0x46,
	Module(date) = 0x48,
	
	Module(reference) = 0x4e,
};

Interface(
	(Structure, undefined ,(void))
	(Structure, null ,(void))
	(Structure, false ,(void))
	(Structure, true ,(void))
	(Structure, boolean ,(int))
	(Structure, integer ,(int32_t integer))
	(Structure, binary ,(double binary))
	(Structure, identifier ,(struct Identifier identifier))
	(Structure, text ,(const struct Text *text))
	(Structure, chars ,(struct Chars *chars))
	(Structure, object ,(struct Object *))
	(Structure, error ,(struct Error *))
	(Structure, string ,(struct String *))
	(Structure, date ,(struct Date *))
	(Structure, function ,(struct Function *))
	(Structure, breaker ,(int32_t integer))
	(Structure, reference ,(Instance))
	
	(Structure, toPrimitive ,(Structure, struct Ecc *ecc, const struct Text *text, int hint /* 0: auto, 1: String, -1: Number */))
	(int, isPrimitive ,(Structure))
	(int, isBoolean ,(Structure))
	(int, isDynamic ,(Structure))
	(int, isTrue ,(Structure))
	
	(Structure, toString ,(Structure))
	(int, isString ,(Structure))
	(const char *, stringChars ,(Structure))
	(uint16_t, stringLength ,(Structure))
	
	(Structure, toBinary ,(Structure))
	(Structure, toInteger ,(Structure))
	(int, isNumber ,(Structure))
	
	(Structure, toObject ,(Structure, struct Ecc *ecc, const struct Text *text))
	(int, isObject ,(Structure))
	
	(Structure, toType ,(Structure))
	
	(void, dumpTo ,(Structure, FILE *))
	,
	{
		union
		{
			int32_t integer;
			double binary;
			struct Identifier identifier;
			const struct Text *text;
			
			struct Chars *chars;
			
			struct Object *object;
			struct Error *error;
			struct String *string;
			struct Date *date;
			struct Function *function;
			
			Instance reference;
		} data;
		enum Module(Type) type;
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

#include "interface.h"
#define Module io_libecc_Value
#endif

#endif

//
//  value.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#include "value.h"

// MARK: - Private

#define valueMake(T) { { 0 }, Value(T), 0, 1 }

const struct Value Value(undefined) = valueMake(undefinedType);
const struct Value Value(true) = valueMake(trueType);
const struct Value Value(false) = valueMake(falseType);
const struct Value Value(null) = valueMake(nullType);

static inline uint16_t textToBuffer (struct Text text, char *buffer, uint16_t length)
{
	memcpy(buffer, text.location, text.length);
	return text.length;
}

static inline uint16_t binaryToBuffer (double binary, int base, char *buffer, uint16_t length)
{
	if (!base || base == 10)
	{
		if (binary <= -1e+21 || binary >= 1e+21)
			return snprintf(buffer, length, "%g", binary);
		else
		{
			double dblDig10 = pow(10, DBL_DIG);
			int precision = binary >= -dblDig10 && binary <= dblDig10? DBL_DIG: 21;
			
			return snprintf(buffer, length, "%.*g", precision, binary);
		}
	}
	else
	{
		int sign = signbit(binary);
		unsigned long integer = sign? -binary: binary;
		
		if (base == 8 || base == 16)
		{
			const char *format = sign? (base == 8? "-%lo": "-%lx"): (base == 8? "%lo": "%lx");
			
			return snprintf(buffer, length, format, integer);
		}
		else
		{
			static char const digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
			char bytes[1 + sizeof(integer) * CHAR_BIT];
			char *p = bytes + sizeof(bytes);
			uint16_t count;
			
			while (integer) {
				*(--p) = digits[integer % base];
				integer /= base;
			}
			if (sign)
				*(--p) = '-';
			
			count = bytes + sizeof(bytes) - p;
			if (buffer && count > length)
				count = length;
			
			if (buffer)
				memcpy(buffer, p, count);
			
			return count;
		}
	}
}

// MARK: - Static Members

// MARK: - Methods

struct Value none (void)
{
	return (struct Value){
		.check = 0,
	};
}

struct Value truth (int truth)
{
	return (struct Value){
		.type = truth? Value(trueType): Value(falseType),
		.check = 1,
	};
}

struct Value integer (int32_t integer)
{
	return (struct Value){
		.data = { .integer = integer },
		.type = Value(integerType),
		.check = 1,
	};
}

struct Value binary (double binary)
{
	return (struct Value){
		.data = { .binary = binary },
		.type = Value(binaryType),
		.check = 1,
	};
}

struct Value key (struct Key key)
{
	return (struct Value){
		.data = { .key = key },
		.type = Value(keyType),
		.check = 0,
	};
}

struct Value text (const struct Text *text)
{
	return (struct Value){
		.data = { .text = text },
		.type = Value(textType),
		.check = 1,
	};
}

struct Value chars (struct Chars *chars)
{
	assert(chars);
	
	return (struct Value){
		.data = { .chars = chars },
		.type = Value(charsType),
		.check = 1,
	};
}

struct Value object (struct Object *object)
{
	assert(object);
	
	return (struct Value){
		.data = { .object = object },
		.type = Value(objectType),
		.check = 1,
	};
}

struct Value error (struct Error *error)
{
	assert(error);
	
	return (struct Value){
		.data = { .error = error },
		.type = Value(errorType),
		.check = 1,
	};
}

struct Value string (struct String *string)
{
	assert(string);
	
	return (struct Value){
		.data = { .string = string },
		.type = Value(stringType),
		.check = 1,
	};
}

struct Value number (struct Number *number)
{
	assert(number);
	
	return (struct Value){
		.data = { .number = number },
		.type = Value(numberType),
		.check = 1,
	};
}

struct Value boolean (struct Boolean *boolean)
{
	assert(boolean);
	
	return (struct Value){
		.data = { .boolean = boolean },
		.type = Value(booleanType),
		.check = 1,
	};
}

struct Value date (struct Date *date)
{
	assert(date);
	
	return (struct Value){
		.data = { .date = date },
		.type = Value(dateType),
		.check = 1,
	};
}

struct Value function (struct Function *function)
{
	assert(function);
	
	return (struct Value){
		.data = { .function = function },
		.type = Value(functionType),
		.check = 1,
	};
}

struct Value reference (struct Value *reference)
{
	assert(reference);
	
	return (struct Value){
		.data = { .reference = reference },
		.type = Value(referenceType),
		.check = 0,
	};
}

struct Value toPrimitive (struct Native(Context) * const context, struct Ecc *ecc, struct Value value, const struct Text *text, enum Value(hintPrimitive) hint)
{
	struct Object *object;
	struct Key aKey;
	struct Key bKey;
	struct Value aFunction;
	struct Value bFunction;
	struct Value result;
	
	if (value.type < Value(objectType))
		return value;
	
	object = value.data.object;
	hint = hint? hint: value.type == Value(dateType)? 1: -1;
	aKey = hint > 0? Key(toString): Key(valueOf);
	bKey = hint > 0? Key(valueOf): Key(toString);
	
	aFunction = Object.get(object, aKey);
	if (aFunction.type == Value(functionType))
	{
		struct Value result = Op.callFunctionVA(context, ecc, 0, aFunction.data.function, value, 0);
		if (isPrimitive(result))
			return result;
	}
	
	bFunction = Object.get(object, bKey);
	if (bFunction.type == Value(functionType))
	{
		result = Op.callFunctionVA(context, ecc, 0, bFunction.data.function, value, 0);
		if (isPrimitive(result))
			return result;
	}
	
	Ecc.jmpEnv(ecc, error(Error.typeError(text? *text: Text(empty), "cannot convert %.*s%sto primitive", text? text->length: 0, text? text->location: "", text? " ": "")));
}

int isPrimitive (struct Value value)
{
	return value.type < Value(objectType);
}

int isBoolean (struct Value value)
{
	return value.type & 0x01;
}

int isDynamic (struct Value value)
{
	return value.type >= Value(charsType);
}

int isTrue (struct Value value)
{
	if (value.type <= 0)
		return 0;
	else if (value.type == Value(integerType))
		return value.data.integer != 0;
	else if (value.type == Value(binaryType))
		return value.data.binary != 0;
	else if (isString(value))
		return stringLength(value) > 0;
	else
		return 1;
}

struct Value toString (struct Value value)
{
	switch ((enum Value(Type))value.type)
	{
		case Value(textType):
		case Value(charsType):
			return value;
		
		case Value(keyType):
			return text(Key.textOf(value.data.key));
		
		case Value(stringType):
			return chars(value.data.string->value);
		
		case Value(nullType):
			return text(&Text(null));
		
		case Value(undefinedType):
			return text(&Text(undefined));
		
		case Value(falseType):
			return text(&Text(false));
		
		case Value(trueType):
			return text(&Text(true));
		
		case Value(booleanType):
			return value.data.boolean->truth? text(&Text(true)): text(&Text(false));
		
		case Value(integerType):
			return binaryToString(value.data.integer, 10);
		
		case Value(numberType):
			value.data.binary = value.data.number->value;
			/*vvv*/
		
		case Value(binaryType):
			return binaryToString(value.data.binary, 10);
		
		case Value(objectType):
		case Value(dateType):
		case Value(functionType):
			return text(value.data.object->type);
		
		case Value(errorType):
		{
			uint16_t length = Error.toBufferLength(&value.data.error->object);
			struct Chars *chars = Chars.createSized(length);
			Error.toBuffer(&value.data.error->object, chars->chars, length + 1);
			return Value.chars(chars);
		}
		
		case Value(referenceType):
			break;
	}
	assert(0);
	abort();
}

uint16_t toBufferLength (struct Value value)
{
	switch ((enum Value(Type))value.type)
	{
		case Value(keyType):
			return Key.textOf(value.data.key)->length;
		
		case Value(textType):
			return value.data.text->length;
		
		case Value(stringType):
			return value.data.string->value->length;
		
		case Value(charsType):
			return value.data.chars->length;
		
		case Value(nullType):
			return Text(null).length;
		
		case Value(undefinedType):
			return Text(undefined).length;
		
		case Value(falseType):
			return Text(false).length;
		
		case Value(trueType):
			return Text(true).length;
		
		case Value(booleanType):
			if (value.data.boolean->truth)
				return Text(true).length;
			else
				return Text(false).length;
		
		case Value(integerType):
			return binaryToBuffer(value.data.integer, 10, NULL, 0);
		
		case Value(numberType):
			return binaryToBuffer(value.data.number->value, 10, NULL, 0);
		
		case Value(binaryType):
			return binaryToBuffer(value.data.binary, 10, NULL, 0);
		
		case Value(objectType):
			if (value.data.object->type == &Text(arrayType))
				return Array.toBufferLength(value.data.object, (struct Text){ ",", 1 });
			/*vvv*/
		
		case Value(dateType):
			return value.data.object->type->length;
		
		case Value(errorType):
			return Error.toBufferLength(&value.data.error->object);
		
		case Value(functionType):
			return Function.toBufferLength(value.data.function);
		
		case Value(referenceType):
			break;
	}
	
	assert(0);
	abort();
}

uint16_t toBuffer (struct Value value, char *buffer, uint16_t length)
{
	switch ((enum Value(Type))value.type)
	{
		case Value(keyType):
			return textToBuffer(*Key.textOf(value.data.key), buffer, length);
		
		case Value(textType):
			return textToBuffer(*value.data.text, buffer, length);
		
		case Value(stringType):
			return textToBuffer((struct Text){ value.data.string->value->chars, value.data.string->value->length }, buffer, length);
		
		case Value(charsType):
			return textToBuffer((struct Text){ value.data.chars->chars, value.data.chars->length }, buffer, length);
		
		case Value(nullType):
			return textToBuffer(Text(null), buffer, length);
		
		case Value(undefinedType):
			return textToBuffer(Text(undefined), buffer, length);
		
		case Value(falseType):
			return textToBuffer(Text(false), buffer, length);
		
		case Value(trueType):
			return textToBuffer(Text(true), buffer, length);
		
		case Value(booleanType):
			return textToBuffer(value.data.boolean->truth? Text(true): Text(false), buffer, length);
		
		case Value(integerType):
			return binaryToBuffer(value.data.integer, 10, buffer, length);
		
		case Value(numberType):
			return binaryToBuffer(value.data.number->value, 10, buffer, length);
		
		case Value(binaryType):
			return binaryToBuffer(value.data.binary, 10, buffer, length);
		
		case Value(objectType):
			if (value.data.object->type == &Text(arrayType))
				return Array.toBuffer(value.data.object, (struct Text){ ",", 1 }, buffer, length);
			/*vvv*/
		
		case Value(dateType):
			return textToBuffer(*value.data.object->type, buffer, length);
		
		case Value(errorType):
			return Error.toBuffer(&value.data.error->object, buffer, length);
		
		case Value(functionType):
			return Function.toBuffer(value.data.function, buffer, length);
		
		case Value(referenceType):
			break;
	}
	
	assert(0);
	abort();
}

struct Value binaryToString (double binary, int base)
{
	uint16_t length;
	struct Chars *chars;
	
	if (binary == 0)
		return text(&Text(zero));
	else if (binary == 1)
		return text(&Text(one));
	else if (isnan(binary))
		return text(&Text(nan));
	else if (isinf(binary))
	{
		if (signbit(binary))
			return text(&Text(negativeInfinity));
		else
			return text(&Text(infinity));
	}
	
	length = binaryToBuffer(binary, base, NULL, 0);
	chars = Chars.createSized(length);
	binaryToBuffer(binary, base, chars->chars, chars->length + 1);
	return Value.chars(chars);
}

int isString (struct Value value)
{
	return value.type & 0x20;
}

const char * stringChars (struct Value value)
{
	if (value.type == Value(charsType))
		return value.data.chars->chars;
	else if (value.type == Value(textType))
		return value.data.text->location;
	else if (value.type == Value(stringType))
		return value.data.string->value->chars;
	else
		return NULL;
}

uint16_t stringLength (struct Value value)
{
	if (value.type == Value(charsType))
		return value.data.chars->length;
	else if (value.type == Value(textType))
		return value.data.text->length;
	else if (value.type == Value(stringType))
		return value.data.string->value->length;
	else
		return 0;
}

struct Value toBinary (struct Value value)
{
	switch ((enum Value(Type))value.type)
	{
		case Value(binaryType):
			return value;
		
		case Value(integerType):
			return binary(value.data.integer);
		
		case Value(numberType):
			return binary(value.data.number->value);
		
		case Value(nullType):
		case Value(falseType):
			return binary(0);
		
		case Value(trueType):
			return binary(1);
		
		case Value(booleanType):
			return binary(value.data.boolean->truth? 1: 0);
		
		case Value(undefinedType):
			return binary(NAN);
		
		case Value(textType):
			if (value.data.text == &Text(zero))
				return binary(0);
			else if (value.data.text == &Text(one))
				return binary(1);
			else if (value.data.text == &Text(nan))
				return binary(NAN);
			else if (value.data.text == &Text(infinity))
				return binary(INFINITY);
			else if (value.data.text == &Text(negativeInfinity))
				return binary(-INFINITY);
			
			/*vvv*/
			
		case Value(keyType):
		case Value(charsType):
		case Value(stringType):
			return Lexer.parseBinary(Text.make(stringChars(value), stringLength(value)));
		
		case Value(objectType):
		case Value(errorType):
		case Value(dateType):
		case Value(functionType):
			return binary(NAN);
		
		case Value(referenceType):
			break;
	}
	assert(0);
	abort();
}

struct Value toInteger (struct Value value)
{
	double binary = toBinary(value).data.binary;
	
	if (isnan(binary) || isinf(binary))
		return integer(0);
	else
		return integer((uint32_t)binary);
}

int isNumber (struct Value value)
{
	return value.type & 0x10;
}

struct Value toObject (struct Native(Context) * const context, struct Ecc *ecc, struct Value value, enum Native(Index) argumentIndex)
{
	if (value.type >= Value(objectType))
		return value;
	
	switch ((enum Value(Type))value.type)
	{
		case Value(binaryType):
			return number(Number.create(value.data.binary));
		
		case Value(integerType):
			return number(Number.create(value.data.integer));
		
		case Value(keyType):
		case Value(textType):
		case Value(charsType):
			return string(String.create(Chars.create("%.*s", stringLength(value), stringChars(value))));
		
		case Value(falseType):
		case Value(trueType):
			return boolean(Boolean.create(value.type == Value(trueType)));
		
		case Value(nullType):
			Ecc.jmpEnv(ecc, error(Error.typeError(Native.textSeek(context, ecc, argumentIndex), "can't convert null to object")));
		
		case Value(undefinedType):
			Ecc.jmpEnv(ecc, error(Error.typeError(Native.textSeek(context, ecc, argumentIndex), "can't convert undefined to object")));
		
		case Value(referenceType):
		case Value(functionType):
		case Value(objectType):
		case Value(errorType):
		case Value(stringType):
		case Value(numberType):
		case Value(dateType):
		case Value(booleanType):
			break;
	}
	assert(0);
	abort();
}

int isObject (struct Value value)
{
	return value.type >= Value(objectType);
}

struct Value toType (struct Value value)
{
	switch ((enum Value(Type))value.type)
	{
		case Value(trueType):
		case Value(falseType):
			return text(&Text(boolean));
		
		case Value(undefinedType):
			return text(&Text(undefined));
		
		case Value(integerType):
		case Value(binaryType):
			return text(&Text(number));
		
		case Value(keyType):
		case Value(textType):
		case Value(charsType):
			return text(&Text(string));
		
		case Value(nullType):
		case Value(objectType):
		case Value(stringType):
		case Value(numberType):
		case Value(booleanType):
		case Value(errorType):
		case Value(dateType):
			return text(&Text(object));
		
		case Value(functionType):
			return text(&Text(function));
		
		case Value(referenceType):
			break;
	}
	assert(0);
	abort();
}

void dumpTo (struct Value value, FILE *file)
{
	switch ((enum Value(Type))value.type)
	{
		case Value(nullType):
			fputs("null", file);
			return;
		
		case Value(undefinedType):
			fputs("undefined", file);
			return;
		
		case Value(falseType):
			fputs("false", file);
			return;
		
		case Value(trueType):
			fputs("true", file);
			return;
		
		case Value(booleanType):
			fputs(value.data.boolean->truth? "true": "false", file);
			return;
		
		case Value(integerType):
			fprintf(file, "%d", (int)value.data.integer);
			return;
		
		case Value(numberType):
			value.data.binary = value.data.number->value;
			/*vvv*/
			
		case Value(binaryType):
			fprintf(file, "%g", value.data.binary);
			return;
		
		case Value(keyType):
		{
			const struct Text *text = Key.textOf(value.data.key);
			fwrite(text->location, sizeof(char), text->length, file);
			return;
		}
		
		case Value(textType):
			fwrite(value.data.text->location, sizeof(char), value.data.text->length, file);
			return;
		
		case Value(charsType):
			fwrite(value.data.chars->chars, sizeof(char), value.data.chars->length, file);
			return;
		
		case Value(stringType):
			fwrite(value.data.string->value->chars, sizeof(char), value.data.string->value->length, file);
			return;
		
		case Value(objectType):
		case Value(dateType):
		case Value(errorType):
			Object.dumpTo(value.data.object, file);
			return;
		
		case Value(functionType):
			fwrite(value.data.function->text.location, sizeof(char), value.data.function->text.length, file);
			return;
		
		case Value(referenceType):
			fputs("-> ", file);
			dumpTo(*value.data.reference, file);
			return;
	}
}

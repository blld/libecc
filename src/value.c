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

static inline uint16_t textToBytes (struct Text text, char *bytes)
{
	memcpy(bytes, text.bytes, text.length);
	return text.length;
}

static inline uint16_t binaryToBytes (double binary, int base, char *bytes)
{
	if (!base || base == 10)
	{
		if (binary <= -1e+21 || binary >= 1e+21)
			return bytes?
				sprintf(bytes, "%g", binary):
				snprintf(NULL, 0, "%g", binary);
		else
		{
			double dblDig10 = pow(10, DBL_DIG);
			int precision = binary >= -dblDig10 && binary <= dblDig10? DBL_DIG: 21;
			
			return bytes?
				sprintf(bytes, "%.*g", precision, binary):
				snprintf(NULL, 0, "%.*g", precision, binary);
		}
	}
	else
	{
		int sign = signbit(binary);
		unsigned long integer = sign? -binary: binary;
		
		if (base == 8 || base == 16)
		{
			const char *format = sign? (base == 8? "-%lo": "-%lx"): (base == 8? "%lo": "%lx");
			
			return bytes?
				sprintf(bytes, format, integer):
				snprintf(NULL, 0, format, integer);
		}
		else
		{
			static char const digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
			char buffer[1 + sizeof(integer) * CHAR_BIT];
			char *p = buffer + sizeof(buffer);
			uint16_t count;
			
			while (integer) {
				*(--p) = digits[integer % base];
				integer /= base;
			}
			if (sign)
				*(--p) = '-';
			
			count = buffer + sizeof(buffer) - p;
			if (bytes)
				memcpy(bytes, p, count);
			
			return count;
		}
	}
}

// MARK: - Static Members

// MARK: - Methods


// make

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

struct Value host (struct Object *object)
{
	assert(object);
	
	return (struct Value){
		.data = { .object = object },
		.type = Value(hostType),
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


// check

int isPrimitive (struct Value value)
{
	return !(value.type & Value(objectMask));
}

int isBoolean (struct Value value)
{
	return value.type & Value(booleanMask);
}

int isNumber (struct Value value)
{
	return value.type & Value(numberMask);
}

int isString (struct Value value)
{
	return value.type & Value(stringMask);
}

int isObject (struct Value value)
{
	return value.type & Value(objectMask);
}

int isDynamic (struct Value value)
{
	return value.type & Value(dynamicMask);
}

int isTrue (struct Value value)
{
	if (value.type <= Value(undefinedType))
		return 0;
	if (value.type >= Value(trueType))
		return 1;
	else if (value.type == Value(integerType))
		return value.data.integer != 0;
	else if (value.type == Value(binaryType))
		return value.data.binary != 0;
	else if (isString(value))
		return stringLength(value) > 0;
	
	assert(0);
	abort();
}


// convert

struct Value toPrimitive (struct Native(Context) * const context, struct Value value, const struct Text *text, enum Value(hintPrimitive) hint)
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
		struct Value result = Op.callFunctionVA(context, 0, aFunction.data.function, value, 0);
		if (isPrimitive(result))
			return result;
	}
	
	bFunction = Object.get(object, bKey);
	if (bFunction.type == Value(functionType))
	{
		result = Op.callFunctionVA(context, 0, bFunction.data.function, value, 0);
		if (isPrimitive(result))
			return result;
	}
	
	Ecc.jmpEnv(context->ecc, error(Error.typeError(text? *text: Text(empty), "cannot convert %.*s%sto primitive", text? text->length: 0, text? text->bytes: "", text? " ": "")));
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
			return Lexer.parseBinary(Text.make(stringBytes(value), stringLength(value)));
		
		case Value(objectType):
		case Value(errorType):
		case Value(dateType):
		case Value(functionType):
		case Value(hostType):
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
		case Value(errorType):
		case Value(hostType):
		{
			struct Object *object = value.data.object;
			if (object->type->toLength)
			{
				assert(object->type->toBytes);
				
				uint16_t length = object->type->toLength(value);
				struct Chars *chars = Chars.createSized(length);
				object->type->toBytes(value, chars->bytes);
				return Value.chars(chars);
			}
			else
				return text(value.data.object->type->text);
		}
		
		case Value(referenceType):
			break;
	}
	assert(0);
	abort();
}

uint16_t toLength (struct Value value)
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
			return binaryToBytes(value.data.integer, 10, NULL);
		
		case Value(numberType):
			return binaryToBytes(value.data.number->value, 10, NULL);
		
		case Value(binaryType):
			return binaryToBytes(value.data.binary, 10, NULL);
		
		case Value(objectType):
		case Value(dateType):
		case Value(errorType):
		case Value(functionType):
		case Value(hostType):
		{
			struct Object *object = value.data.object;
			if (object->type->toLength)
			{
				assert(object->type->toBytes);
				return object->type->toLength(value);
			}
			else
				return value.data.object->type->text->length;
		}
		
		case Value(referenceType):
			break;
	}
	
	assert(0);
	abort();
}

uint16_t toBytes (struct Value value, char *bytes)
{
	switch ((enum Value(Type))value.type)
	{
		case Value(keyType):
			return textToBytes(*Key.textOf(value.data.key), bytes);
		
		case Value(textType):
			return textToBytes(*value.data.text, bytes);
		
		case Value(stringType):
			return textToBytes((struct Text){ value.data.string->value->bytes, value.data.string->value->length }, bytes);
		
		case Value(charsType):
			return textToBytes((struct Text){ value.data.chars->bytes, value.data.chars->length }, bytes);
		
		case Value(nullType):
			return textToBytes(Text(null), bytes);
		
		case Value(undefinedType):
			return textToBytes(Text(undefined), bytes);
		
		case Value(falseType):
			return textToBytes(Text(false), bytes);
		
		case Value(trueType):
			return textToBytes(Text(true), bytes);
		
		case Value(booleanType):
			return textToBytes(value.data.boolean->truth? Text(true): Text(false), bytes);
		
		case Value(integerType):
			return binaryToBytes(value.data.integer, 10, bytes);
		
		case Value(numberType):
			return binaryToBytes(value.data.number->value, 10, bytes);
		
		case Value(binaryType):
			return binaryToBytes(value.data.binary, 10, bytes);
		
		case Value(objectType):
		case Value(errorType):
		case Value(dateType):
		case Value(functionType):
		case Value(hostType):
		{
			struct Object *object = value.data.object;
			if (object->type->toBytes)
				return object->type->toBytes(value, bytes);
			else
				return textToBytes(*value.data.object->type->text, bytes);
		}
		
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
	
	length = binaryToBytes(binary, base, NULL);
	chars = Chars.createSized(length);
	binaryToBytes(binary, base, chars->bytes);
	return Value.chars(chars);
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

const char * stringBytes (struct Value value)
{
	if (value.type == Value(charsType))
		return value.data.chars->bytes;
	else if (value.type == Value(textType))
		return value.data.text->bytes;
	else if (value.type == Value(stringType))
		return value.data.string->value->bytes;
	else
		return NULL;
}

struct Value toObject (struct Native(Context) * const context, struct Value value, enum Native(Index) argumentIndex)
{
	if (value.type >= Value(objectType))
		return value;
	
	switch ((enum Value(Type))value.type)
	{
		case Value(binaryType):
			return number(Number.create(value.data.binary));
		
		case Value(integerType):
			return number(Number.create(value.data.integer));
		
		case Value(textType):
		case Value(charsType):
			return string(String.create(Chars.create("%.*s", stringLength(value), stringBytes(value))));
		
		case Value(falseType):
		case Value(trueType):
			return boolean(Boolean.create(value.type == Value(trueType)));
		
		case Value(nullType):
			Ecc.jmpEnv(context->ecc, error(Error.typeError(Native.textSeek(context, argumentIndex), "can't convert null to object")));
		
		case Value(undefinedType):
			Ecc.jmpEnv(context->ecc, error(Error.typeError(Native.textSeek(context, argumentIndex), "can't convert undefined to object")));
		
		case Value(keyType):
		case Value(referenceType):
		case Value(functionType):
		case Value(objectType):
		case Value(errorType):
		case Value(stringType):
		case Value(numberType):
		case Value(dateType):
		case Value(booleanType):
		case Value(hostType):
			break;
	}
	assert(0);
	abort();
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
		case Value(hostType):
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
			fwrite(text->bytes, sizeof(char), text->length, file);
			return;
		}
		
		case Value(textType):
			fwrite(value.data.text->bytes, sizeof(char), value.data.text->length, file);
			return;
		
		case Value(charsType):
			fwrite(value.data.chars->bytes, sizeof(char), value.data.chars->length, file);
			return;
		
		case Value(stringType):
			fwrite(value.data.string->value->bytes, sizeof(char), value.data.string->value->length, file);
			return;
		
		case Value(objectType):
		case Value(dateType):
		case Value(errorType):
		case Value(hostType):
			Object.dumpTo(value.data.object, file);
			return;
		
		case Value(functionType):
			fwrite(value.data.function->text.bytes, sizeof(char), value.data.function->text.length, file);
			return;
		
		case Value(referenceType):
			fputs("-> ", file);
			dumpTo(*value.data.reference, file);
			return;
	}
}

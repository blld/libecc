//
//  chars.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#include "chars.h"

// MARK: - Private

static inline struct Chars * appendText (struct Chars * chars, struct Text text)
{
	return append(chars, "%.*s", text.length, text.bytes);
}

// MARK: - Static Members

// MARK: - Methods

struct Chars * createVA (uint16_t length, const char *format, va_list ap)
{
	struct Chars *self;
	
	self = createSized(length);
	vsprintf(self->bytes, format, ap);
	
	return self;
}

struct Chars * create (const char *format, ...)
{
	uint16_t length;
	va_list ap;
	struct Chars *self;
	
	va_start(ap, format);
	length = vsnprintf(NULL, 0, format, ap);
	va_end(ap);
	
	va_start(ap, format);
	self = createVA(length, format, ap);
	va_end(ap);
	
	return self;
}

struct Chars * createSized (uint16_t length)
{
	struct Chars *self = malloc(sizeof(*self) + length);
	Pool.addChars(self);
	*self = Chars.identity;
	
	self->length = length;
	self->bytes[length] = '\0';
	
	return self;
}

struct Chars * createWithBytes (uint16_t length, const char *bytes)
{
	struct Chars *self = malloc(sizeof(*self) + length);
	Pool.addChars(self);
	*self = Chars.identity;
	
	self->length = length;
	memcpy(self->bytes, bytes, length);
	self->bytes[length] = '\0';
	
	return self;
}


struct Chars * beginAppend (void)
{
	struct Chars *self = malloc(sizeof(*self));
	*self = Chars.identity;
	self->flags |= Chars(inAppend);
	return self;
}

struct Chars * append (struct Chars *self, const char *format, ...)
{
	assert(self);
	
	uint16_t length;
	va_list ap;
	
	va_start(ap, format);
	length = vsnprintf(NULL, 0, format, ap);
	va_end(ap);
	
	va_start(ap, format);
	self = realloc(self, sizeof(*self) + self->length + length);
	vsprintf(self->bytes + self->length, format, ap);
	self->length += length;
	va_end(ap);
	
	return self;
}

struct Chars * appendValue (struct Chars *self, struct Native(Context) * const context, struct Value value)
{
	switch ((enum Value(Type))value.type)
	{
		case Value(keyType):
			return appendText(self, *Key.textOf(value.data.key));
		
		case Value(textType):
			return appendText(self, *value.data.text);
		
		case Value(stringType):
			return appendText(self, (struct Text){ value.data.string->value->bytes, value.data.string->value->length });
		
		case Value(charsType):
			return appendText(self, (struct Text){ value.data.chars->bytes, value.data.chars->length });
		
		case Value(nullType):
			return appendText(self, Text(null));
		
		case Value(undefinedType):
			return appendText(self, Text(undefined));
		
		case Value(falseType):
			return appendText(self, Text(false));
		
		case Value(trueType):
			return appendText(self, Text(true));
		
		case Value(booleanType):
			return appendText(self, value.data.boolean->truth? Text(true): Text(false));
		
		case Value(integerType):
			return appendBinary(self, value.data.integer, 10);
		
		case Value(numberType):
			return appendBinary(self, value.data.number->value, 10);
		
		case Value(binaryType):
			return appendBinary(self, value.data.binary, 10);
		
		case Value(functionType):
		case Value(objectType):
		case Value(errorType):
		case Value(dateType):
		case Value(hostType):
			value = Value.toString(context, value);
			return appendText(self, (struct Text){ Value.stringBytes(value), Value.stringLength(value) });
		
		case Value(referenceType):
			break;
	}
	
	assert(0);
	abort();
}

struct Chars * appendBinary (struct Chars * chars, double binary, int base)
{
	if (!base || base == 10)
	{
		if (binary <= -1e+21 || binary >= 1e+21)
			return append(chars, "%g", binary);
		else
		{
			double dblDig10 = pow(10, DBL_DIG);
			int precision = binary >= -dblDig10 && binary <= dblDig10? DBL_DIG: 21;
			
			return append(chars, "%.*g", precision, binary);
		}
	}
	else
	{
		int sign = signbit(binary);
		unsigned long integer = sign? -binary: binary;
		
		if (base == 8 || base == 16)
		{
			const char *format = sign? (base == 8? "-%lo": "-%lx"): (base == 8? "%lo": "%lx");
			
			return append(chars, format, integer);
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
			return append(chars, "%.*s", count, p);
		}
	}
}

struct Chars * endAppend (struct Chars *self)
{
	Pool.addChars(self);
	self->bytes[self->length] = '\0';
	self->flags &= ~Chars(inAppend);
	return self;
}

void destroy (struct Chars *self)
{
	assert(self);
	
	free(self), self = NULL;
}

//
//  chars.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#define Implementation
#include "chars.h"

#include "ecc.h"
#include "pool.h"

// MARK: - Private

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


void beginAppend (struct Chars **chars)
{
	beginAppendSized(chars, 0);
}

void beginAppendSized (struct Chars **chars, uint16_t sized)
{
	struct Chars *self = malloc(sizeof(*self) + sized);
	*self = Chars.identity;
	self->flags |= Chars(inAppend);
	*chars = self;
	Pool.addChars(self);
}

struct Chars * append (struct Chars **chars, const char *format, ...)
{
	uint16_t length;
	va_list ap;
	struct Chars *self = *chars;
	
	assert(self->flags & Chars(inAppend));
	
	va_start(ap, format);
	length = vsnprintf(NULL, 0, format, ap);
	va_end(ap);
	
	va_start(ap, format);
	self = realloc(self, sizeof(*self) + self->length + length);
	vsprintf(self->bytes + self->length, format, ap);
	self->length += length;
	va_end(ap);
	
	if (self != *chars)
	{
		Pool.reindexChars(self, *chars);
		*chars = self;
	}
	return self;
}

static
inline struct Chars * appendText (struct Chars ** chars, struct Text text)
{
	struct Chars *self = *chars;
	uint32_t lo = Text.codepoint(text, NULL), hi;
	
	if (lo >= 0xDC00 && lo <= 0xDFFF)
	{
		struct Text prev = Text.make(self->bytes, self->length);
		prev.bytes += prev.length;
		hi = Text.prevCodepoint(&prev);
		if (hi >= 0xD800 && hi <= 0xDBFF)
		{
			/* merge 16-bit surrogate */
			self->length = prev.length;
			return appendCodepoint(chars, 0x10000 | ((hi & 0x03FF) << 10) | (lo & 0x03FF));
		}
	}

	self = realloc(self, sizeof(*self) + self->length + text.length);
	memcpy(self->bytes + self->length, text.bytes, text.length);
	self->length += text.length;
	self->bytes[self->length] = '\0';
	
	if (self != *chars)
	{
		Pool.reindexChars(self, *chars);
		*chars = self;
	}
	
	return self;
}

struct Chars * appendCodepoint (struct Chars **chars, uint32_t cp)
{
	char buffer[5] = { 0 };
	struct Text text = Text.make(buffer, 0);
	
	if (cp < 0x80)
	{
		buffer[0] = cp;
		text.length = 1;
	}
	else if (cp < 0x800)
	{
		buffer[0] = 0xC0 | (cp >> 6);
		buffer[1] = 0x80 | (cp & 0x3F);
		text.length = 2;
	}
	else if (cp < 0x10000)
	{
		buffer[0] = 0xE0 | (cp >> 12);
		buffer[1] = 0x80 | (cp >> 6 & 0x3F);
		buffer[2] = 0x80 | (cp & 0x3F);
		text.length = 3;
	}
	else
	{
		buffer[0] = 0xF0 | (cp >> 18);
		buffer[1] = 0x80 | (cp >> 12 & 0x3F);
		buffer[2] = 0x80 | (cp >> 6 & 0x3F);
		buffer[3] = 0x80 | (cp & 0x3F);
		text.length = 4;
	}
	
	return appendText(chars, text);
}

struct Chars * appendValue (struct Chars **chars, struct Context * const context, struct Value value)
{
	switch ((enum Value(Type))value.type)
	{
		case Value(keyType):
			return appendText(chars, *Key.textOf(value.data.key));
		
		case Value(textType):
			return appendText(chars, *value.data.text);
		
		case Value(stringType):
			return appendText(chars, Text.make(value.data.string->value->bytes, value.data.string->value->length));
		
		case Value(charsType):
			return appendText(chars, Text.make(value.data.chars->bytes, value.data.chars->length));
		
		case Value(nullType):
			return appendText(chars, Text(null));
		
		case Value(undefinedType):
			return appendText(chars, Text(undefined));
		
		case Value(falseType):
			return appendText(chars, Text(false));
		
		case Value(trueType):
			return appendText(chars, Text(true));
		
		case Value(booleanType):
			return appendText(chars, value.data.boolean->truth? Text(true): Text(false));
		
		case Value(integerType):
			return appendBinary(chars, value.data.integer, 10);
		
		case Value(numberType):
			return appendBinary(chars, value.data.number->value, 10);
		
		case Value(binaryType):
			return appendBinary(chars, value.data.binary, 10);
		
		case Value(regexpType):
		case Value(functionType):
		case Value(objectType):
		case Value(errorType):
		case Value(dateType):
		case Value(hostType):
			return appendValue(chars, context, Value.toString(context, value));
		
		case Value(referenceType):
			break;
	}
	Ecc.fatal("Invalid Value(Type) : %u", value.type);
}

static
struct Chars * stripBinary (struct Chars * chars)
{
	while (chars->bytes[chars->length - 1] == '0')
		chars->bytes[--chars->length] = '\0';
	
	if (chars->bytes[chars->length - 1] == '.')
		chars->bytes[--chars->length] = '\0';
	
	return chars;
}

struct Chars * normalizeBinary (struct Chars * chars)
{
	if (chars->length > 5 && chars->bytes[chars->length - 5] == 'e' && chars->bytes[chars->length - 3] == '0')
	{
		chars->bytes[chars->length - 3] = chars->bytes[chars->length - 2];
		chars->bytes[chars->length - 2] = chars->bytes[chars->length - 1];
		chars->bytes[chars->length - 1] = 0;
		--chars->length;
	}
	else if (chars->length > 4 && chars->bytes[chars->length - 4] == 'e' && chars->bytes[chars->length - 2] == '0')
	{
		chars->bytes[chars->length - 2] = chars->bytes[chars->length - 1];
		chars->bytes[chars->length - 1] = 0;
		--chars->length;
	}
	return chars;
}

struct Chars * appendBinary (struct Chars **chars, double binary, int base)
{
	if (isnan(binary))
		return appendText(chars, Text(nan));
	else if (!isfinite(binary))
	{
		if (binary < 0)
			return appendText(chars, Text(negativeInfinity));
		else
			return appendText(chars, Text(infinity));
	}
	
	if (!base || base == 10)
	{
		if (binary <= -1e+21 || binary >= 1e+21)
			return normalizeBinary(append(chars, "%g", binary));
		if ((binary < 1 && binary >= 0.000001) || (binary > -1 && binary <= -0.000001))
			return stripBinary(append(chars, "%.10f", binary));
		else
		{
			double dblDig10 = pow(10, DBL_DIG);
			int precision = binary >= -dblDig10 && binary <= dblDig10? DBL_DIG: 21;
			
			return normalizeBinary(append(chars, "%.*g", precision, binary));
		}
	}
	else
	{
		int sign = binary < 0;
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
			char *p = buffer + sizeof(buffer) - 1;
			uint16_t count;
			
			while (integer) {
				*(--p) = digits[integer % base];
				integer /= base;
			}
			if (sign)
				*(--p) = '-';
			
			count = buffer + sizeof(buffer) - 1 - p;
			return append(chars, "%.*s", count, p);
		}
	}
}

struct Chars * endAppend (struct Chars **chars)
{
	struct Chars *self = *chars;
	
	assert(self->flags & Chars(inAppend));
	
	self->bytes[self->length] = '\0';
	self->flags &= ~Chars(inAppend);
	
	return self;
}

void destroy (struct Chars *self)
{
	assert(self);
	
	free(self), self = NULL;
}

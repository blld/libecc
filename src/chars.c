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

static inline
uint32_t nextPowerOfTwo(uint32_t v)
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}

static inline
uint32_t sizeForLength(uint16_t length)
{
	uint32_t size = sizeof(struct Chars) + length;
	
	if (size < 8)
	{
		/* 8-bytes mini */
		return 8;
	}
	else if (size < 1024)
	{
		/* power of two steps between */
		return nextPowerOfTwo(size);
	}
	else
	{
		/* 1024-bytes chunk */
		--size;
		size |= 0x3ff;
		return size + 1;
	}
}

static
struct Chars *reuseOrCreate (struct Chars **chars, uint16_t length)
{
	struct Chars *self = NULL, *reuse = chars? *chars: NULL;
	
	if (reuse && sizeForLength(reuse->length) >= sizeForLength(length))
		return reuse;
//	else
//		chars = Pool.reusableChars(length);
	
	if (!self)
	{
		self = malloc(sizeForLength(length));
		Pool.addChars(self);
	}
	
	if (reuse)
	{
		memcpy(self, reuse, sizeof(*self) + reuse->length);
		reuse->flags &= ~Chars(inAppend);
		*chars = self;
	}
	else
		*self = Chars.identity;
	
	return self;
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
	struct Chars *self = reuseOrCreate(NULL, length);
	
	self->length = length;
	self->bytes[length] = '\0';
	
	return self;
}

struct Chars * createWithBytes (uint16_t length, const char *bytes)
{
	struct Chars *self = reuseOrCreate(NULL, length);
	
	self->length = length;
	memcpy(self->bytes, bytes, length);
	self->bytes[length] = '\0';
	
	return self;
}


void beginAppend (struct Chars **chars)
{
	struct Chars *self = reuseOrCreate(NULL, 0);
	
	self->flags |= Chars(inAppend);
	*chars = self;
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
	
	self = reuseOrCreate(chars, self->length + length);
	
	va_start(ap, format);
	vsprintf(self->bytes + self->length, format, ap);
	self->length += length;
	va_end(ap);
	
	return self;
}

static
inline struct Chars * appendText (struct Chars ** chars, struct Text text)
{
	struct Chars *self = *chars;
	struct Text(Char) lo = Text.character(text), hi;
	struct Text prev;
	
	assert(self->flags & Chars(inAppend));
	
	if (!text.length)
		return self;
	
	self = reuseOrCreate(chars, self->length + text.length);
	prev = Text.make(self->bytes + self->length, self->length);
	
	if (lo.units == 3 && lo.codepoint >= 0xDC00 && lo.codepoint <= 0xDFFF)
	{
		hi = Text.prevCharacter(&prev);
		if (hi.units == 3 && hi.codepoint >= 0xD800 && hi.codepoint <= 0xDBFF)
		{
			/* merge 16-bit surrogates */
			self->length = prev.length + writeCodepoint(self->bytes + prev.length, 0x10000 + (((hi.codepoint - 0xD800) << 10) | ((lo.codepoint - 0xDC00) & 0x03FF)));
			Text.nextCharacter(&text);
		}
	}
	
	memcpy(self->bytes + self->length, text.bytes, text.length);
	self->length += text.length;
	self->bytes[self->length] = '\0';
	
	return self;
}

struct Chars * appendCodepoint (struct Chars **chars, uint32_t cp)
{
	char buffer[5] = { 0 };
	struct Text text = Text.make(buffer, writeCodepoint(buffer, cp));
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

uint8_t codepointLength (uint32_t cp)
{
	if (cp < 0x80)
		return 1;
	else if (cp < 0x800)
		return 2;
	else if (cp < 0x10000)
		return 3;
	else
		return 4;
	
	return 0;
}

uint8_t writeCodepoint (char *bytes, uint32_t cp)
{
	if (cp < 0x80)
	{
		bytes[0] = cp;
		return 1;
	}
	else if (cp < 0x800)
	{
		bytes[0] = 0xC0 | (cp >> 6);
		bytes[1] = 0x80 | (cp & 0x3F);
		return 2;
	}
	else if (cp < 0x10000)
	{
		bytes[0] = 0xE0 | (cp >> 12);
		bytes[1] = 0x80 | (cp >> 6 & 0x3F);
		bytes[2] = 0x80 | (cp & 0x3F);
		return 3;
	}
	else
	{
		bytes[0] = 0xF0 | (cp >> 18);
		bytes[1] = 0x80 | (cp >> 12 & 0x3F);
		bytes[2] = 0x80 | (cp >> 6 & 0x3F);
		bytes[3] = 0x80 | (cp & 0x3F);
		return 4;
	}
	
	return 0;
}

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
	
	if (size < 16)
	{
		/* 8-bytes mini */
		return 16;
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
struct Chars *reuseOrCreate (struct Chars(Append) *chars, uint16_t length)
{
	struct Chars *self = NULL, *reuse = chars? chars->value: NULL;
	
	if (length <= 8)
		return NULL;
	
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
		memcpy(self, reuse, sizeof(*self) + reuse->length);
	else
	{
		*self = Chars.identity;
		self->length = chars->units;
		memcpy(self->bytes, chars->buffer, chars->units);
	}
	
	chars->value = self;
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
	struct Chars *self = malloc(sizeForLength(length));
	Pool.addChars(self);
	*self = Chars.identity;
	
	self->length = length;
	self->bytes[length] = '\0';
	
	return self;
}

struct Chars * createWithBytes (uint16_t length, const char *bytes)
{
	struct Chars *self = malloc(sizeForLength(length));
	Pool.addChars(self);
	*self = Chars.identity;
	
	self->length = length;
	memcpy(self->bytes, bytes, length);
	self->bytes[length] = '\0';
	
	return self;
}


void beginAppend (struct Chars(Append) *chars)
{
	chars->value = NULL;
	chars->units = 0;
}

void append (struct Chars(Append) *chars, const char *format, ...)
{
	uint16_t length;
	va_list ap;
	struct Chars *self = chars->value;
	
	va_start(ap, format);
	length = vsnprintf(NULL, 0, format, ap);
	va_end(ap);
	
	self = reuseOrCreate(chars, (self? self->length: chars->units) + length);
	
	va_start(ap, format);
	vsprintf(self? (self->bytes + self->length): (chars->buffer + chars->units), format, ap);
	va_end(ap);
	
	if (self)
		self->length += length;
	else
		chars->units += length;
}

static
void appendText (struct Chars(Append) * chars, struct Text text)
{
	struct Chars *self = chars->value;
	struct Text(Char) lo = Text.character(text), hi;
	struct Text prev;
	
	if (!text.length)
		return;
	
	self = reuseOrCreate(chars, (self? self->length: chars->units) + text.length);
	
	if (self)
		prev = Text.make(self->bytes + self->length, self->length);
	else
		prev = Text.make(chars->buffer + chars->units, chars->units);
	
	if (lo.units == 3 && lo.codepoint >= 0xDC00 && lo.codepoint <= 0xDFFF)
	{
		hi = Text.prevCharacter(&prev);
		if (hi.units == 3 && hi.codepoint >= 0xD800 && hi.codepoint <= 0xDBFF)
		{
			/* merge 16-bit surrogates */
			uint32_t cp = 0x10000 + (((hi.codepoint - 0xD800) << 10) | ((lo.codepoint - 0xDC00) & 0x03FF));
			
			if (self)
				self->length = prev.length + writeCodepoint(self->bytes + prev.length, cp);
			else
				chars->units = prev.length + writeCodepoint(chars->buffer + prev.length, cp);
			
			Text.nextCharacter(&text);
		}
	}
	
	memcpy(self? (self->bytes + self->length): (chars->buffer + chars->units), text.bytes, text.length);
	if (self)
		self->length += text.length;
	else
		chars->units += text.length;
}

void appendCodepoint (struct Chars(Append) *chars, uint32_t cp)
{
	char buffer[5] = { 0 };
	struct Text text = Text.make(buffer, writeCodepoint(buffer, cp));
	appendText(chars, text);
}

void appendValue (struct Chars(Append) *chars, struct Context * const context, struct Value value)
{
	switch ((enum Value(Type))value.type)
	{
		case Value(keyType):
		case Value(textType):
		case Value(stringType):
		case Value(charsType):
		case Value(bufferType):
			appendText(chars, Value.textOf(&value));
			return;
			
		case Value(nullType):
			appendText(chars, Text(null));
			return;
			
		case Value(undefinedType):
			appendText(chars, Text(undefined));
			return;
			
		case Value(falseType):
			appendText(chars, Text(false));
			return;
			
		case Value(trueType):
			appendText(chars, Text(true));
			return;
			
		case Value(booleanType):
			appendText(chars, value.data.boolean->truth? Text(true): Text(false));
			return;
			
		case Value(integerType):
			appendBinary(chars, value.data.integer, 10);
			return;
			
		case Value(numberType):
			appendBinary(chars, value.data.number->value, 10);
			return;
			
		case Value(binaryType):
			appendBinary(chars, value.data.binary, 10);
			return;
			
		case Value(regexpType):
		case Value(functionType):
		case Value(objectType):
		case Value(errorType):
		case Value(dateType):
		case Value(hostType):
			appendValue(chars, context, Value.toString(context, value));
			return;
			
		case Value(referenceType):
			break;
	}
	Ecc.fatal("Invalid Value(Type) : %u", value.type);
}

static
uint16_t stripBinaryOfBytes (char *bytes, uint16_t length)
{
	while (bytes[length - 1] == '0')
		bytes[--length] = '\0';
	
	if (bytes[length - 1] == '.')
		bytes[--length] = '\0';
	
	return length;
}

static
uint16_t normalizeBinaryOfBytes (char *bytes, uint16_t length)
{
	if (length > 5 && bytes[length - 5] == 'e' && bytes[length - 3] == '0')
	{
		bytes[length - 3] = bytes[length - 2];
		bytes[length - 2] = bytes[length - 1];
		bytes[length - 1] = 0;
		--length;
	}
	else if (length > 4 && bytes[length - 4] == 'e' && bytes[length - 2] == '0')
	{
		bytes[length - 2] = bytes[length - 1];
		bytes[length - 1] = 0;
		--length;
	}
	return length;
}

void appendBinary (struct Chars(Append) *chars, double binary, int base)
{
	if (isnan(binary))
	{
		appendText(chars, Text(nan));
		return;
	}
	else if (!isfinite(binary))
	{
		if (binary < 0)
			appendText(chars, Text(negativeInfinity));
		else
			appendText(chars, Text(infinity));
		
		return;
	}
	
	if (!base || base == 10)
	{
		if (binary <= -1e+21 || binary >= 1e+21)
			append(chars, "%g", binary);
		else if ((binary < 1 && binary >= 0.000001) || (binary > -1 && binary <= -0.000001))
		{
			append(chars, "%.10f", binary);
			if (chars->value)
				chars->value->length = stripBinaryOfBytes(chars->value->bytes, chars->value->length);
			else
				chars->units = stripBinaryOfBytes(chars->buffer, chars->units);
			
			return;
		}
		else
		{
			double dblDig10 = pow(10, DBL_DIG);
			int precision = binary >= -dblDig10 && binary <= dblDig10? DBL_DIG: 21;
			
			append(chars, "%.*g", precision, binary);
		}
		
		normalizeBinary(chars);
		return;
	}
	else
	{
		int sign = binary < 0;
		unsigned long integer = sign? -binary: binary;
		
		if (base == 8 || base == 16)
		{
			const char *format = sign? (base == 8? "-%lo": "-%lx"): (base == 8? "%lo": "%lx");
			
			append(chars, format, integer);
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
			append(chars, "%.*s", count, p);
		}
	}
}

void normalizeBinary (struct Chars(Append) *chars)
{
	if (chars->value)
		chars->value->length = normalizeBinaryOfBytes(chars->value->bytes, chars->value->length);
	else
		chars->units = normalizeBinaryOfBytes(chars->buffer, chars->units);
}

struct Value endAppend (struct Chars(Append) *chars)
{
	struct Chars *self = chars->value;
	
	if (chars->value)
	{
		self->bytes[self->length] = '\0';
		return Value.chars(self);
	}
	else
		return Value.buffer(chars->buffer, chars->units);
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

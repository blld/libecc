//
//  text.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#include "text.h"

// MARK: - Private

#define textMake(N, T) \
	static const char N ## Literal[] = T; \
	const struct Text Text(N) = { N ## Literal, sizeof N ## Literal - 1 }

textMake(undefined, "undefined");
textMake(null, "null");
textMake(true, "true");
textMake(false, "false");
textMake(boolean, "boolean");
textMake(number, "number");
textMake(string, "string");
textMake(object, "object");
textMake(function, "function");
textMake(zero, "0");
textMake(one, "1");
textMake(nan, "NaN");
textMake(infinity, "Infinity");
textMake(negativeInfinity, "-Infinity");
textMake(nativeCode, "[native code]");
textMake(empty, "");

textMake(nullType, "[object Null]");
textMake(undefinedType, "[object Undefined]");
textMake(objectType, "[object Object]");
textMake(errorType, "[object Error]");
textMake(arrayType, "[object Array]");
textMake(stringType, "[object String]");
textMake(numberType, "[object Number]");
textMake(booleanType, "[object Boolean]");
textMake(dateType, "[object Date]");
textMake(functionType, "[object Function]");
textMake(argumentsType, "[object Arguments]");
textMake(mathType, "[object Math]");
textMake(globalType, "[object Global]");

textMake(errorName, "Error");
textMake(rangeErrorName, "RangeError");
textMake(referenceErrorName, "ReferenceError");
textMake(syntaxErrorName, "SyntaxError");
textMake(typeErrorName, "TypeError");
textMake(uriErrorName, "URIError");
textMake(inputErrorName, "InputError");

// MARK: - Static Members

// MARK: - Methods

struct Text make (const char *bytes, uint16_t length)
{
	return (struct Text){
		bytes,
		length,
	};
}

struct Text join (struct Text from, struct Text to)
{
	return make(from.bytes, to.bytes - from.bytes + to.length);
}

uint16_t nextCodepointBytes (struct Text text)
{
	switch (text.length)
	{
		default:
		case 4:
			if ((text.bytes[0] & 0xf8) == 0xf0 && (text.bytes[1] & 0xc0) == 0x80 && (text.bytes[2] & 0xc0) == 0x80 && (text.bytes[3] & 0xc0) == 0x80)
				return 4;
		
		case 3:
			if ((text.bytes[0] & 0xf0) == 0xe0 && (text.bytes[1] & 0xc0) == 0x80 && (text.bytes[2] & 0xc0) == 0x80 )
				return 3;
		
		case 2:
			if ((text.bytes[0] & 0xe0) == 0xc0 && (text.bytes[1] & 0xc0) == 0x80)
				return 2;
		
		case 1:
			return 1;
		
		case 0:
			return 0;
	}
}

uint32_t nextCodepoint (struct Text *text)
{
	uint32_t cp;
	
	switch (text->length)
	{
		default:
		case 4:
			if ((text->bytes[0] & 0xf8) == 0xf0 && (text->bytes[1] & 0xc0) == 0x80 && (text->bytes[2] & 0xc0) == 0x80 && (text->bytes[3] & 0xc0) == 0x80)
			{
				cp  = (*text->bytes++ & 0x07) << 18;
				cp |= (*text->bytes++ & 0x3f) << 12;
				cp |= (*text->bytes++ & 0x3f) << 6;
				cp |= (*text->bytes++ & 0x3f);
				text->length -= 4;
				return cp;
			}
		
		case 3:
			if ((text->bytes[0] & 0xf0) == 0xe0 && (text->bytes[1] & 0xc0) == 0x80 && (text->bytes[2] & 0xc0) == 0x80 )
			{
				cp  = (*text->bytes++ & 0x0f) << 12;
				cp |= (*text->bytes++ & 0x3f) << 6;
				cp |= (*text->bytes++ & 0x3f);
				text->length -= 3;
				return cp;
			}
		
		case 2:
			if ((text->bytes[0] & 0xe0) == 0xc0 && (text->bytes[1] & 0xc0) == 0x80)
			{
				cp  = (*text->bytes++ & 0x1f) << 6;
				cp |= (*text->bytes++ & 0x3f);
				text->length -= 2;
				return cp;
			}
		
		case 1:
			--text->length;
			return *text->bytes++;
		
		case 0:
			return 0;
	}
}

uint16_t toUTF16Bytes (struct Text text)
{
	uint16_t windex = 0;
	
	while (text.length)
		windex += nextCodepoint(&text) > 0x010000? 2: 1;
	
	return windex;
}

uint16_t toUTF16 (struct Text text, uint16_t *wbuffer)
{
	uint16_t windex = 0;
	uint32_t cp;
	
	while (text.length)
	{
		cp = nextCodepoint(&text);
		
		if (cp > 0x010000)
		{
			cp -= 0x010000;
			wbuffer[windex++] = ((cp >> 10) & 0x3ff) + 0xd800;
			wbuffer[windex++] = ((cp >>  0) & 0x3ff) + 0xdc00;
		}
		else
			wbuffer[windex++] = cp;
	}
	
	return windex;
}

//
//  text.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#include "text.h"

// MARK: - Private

#define textMake(T) { T, sizeof(T) - 1 }

const struct Text Text(undefined) = textMake("undefined");
const struct Text Text(null) = textMake("null");
const struct Text Text(true) = textMake("true");
const struct Text Text(false) = textMake("false");
const struct Text Text(boolean) = textMake("boolean");
const struct Text Text(number) = textMake("number");
const struct Text Text(string) = textMake("string");
const struct Text Text(object) = textMake("object");
const struct Text Text(function) = textMake("function");
const struct Text Text(zero) = textMake("0");
const struct Text Text(one) = textMake("1");
const struct Text Text(nan) = textMake("NaN");
const struct Text Text(infinity) = textMake("Infinity");
const struct Text Text(negativeInfinity) = textMake("-Infinity");
const struct Text Text(nativeCode) = textMake("[native code]");

const struct Text Text(nullType) = textMake("[object Null]");
const struct Text Text(undefinedType) =  textMake("[object Undefined]");
const struct Text Text(objectType) = textMake("[object Object]");
const struct Text Text(errorType) = textMake("[object Error]");
const struct Text Text(arrayType) = textMake("[object Array]");
const struct Text Text(stringType) = textMake("[object String]");
const struct Text Text(dateType) = textMake("[object Date]");
const struct Text Text(functionType) = textMake("[object Function]");
const struct Text Text(argumentsType) = textMake("[object Arguments]");

const struct Text Text(errorName) = textMake("Error");
const struct Text Text(rangeErrorName) = textMake("RangeError");
const struct Text Text(referenceErrorName) = textMake("ReferenceError");
const struct Text Text(syntaxErrorName) = textMake("SyntaxError");
const struct Text Text(typeErrorName) = textMake("TypeError");
const struct Text Text(uriErrorName) = textMake("URIError");
const struct Text Text(inputErrorName) = textMake("InputError");

// MARK: - Static Members

// MARK: - Methods

struct Text make (const char *location, uint16_t length)
{
	return (struct Text){
		location,
		length,
	};
}

struct Text join (struct Text from, struct Text to)
{
	return make(from.location, to.location - from.location + to.length);
}

uint16_t nextCodepointBytes (struct Text text)
{
	switch (text.length)
	{
		default:
		case 4:
			if ((text.location[0] & 0xf8) == 0xf0 && (text.location[1] & 0xc0) == 0x80 && (text.location[2] & 0xc0) == 0x80 && (text.location[3] & 0xc0) == 0x80)
				return 4;
		
		case 3:
			if ((text.location[0] & 0xf0) == 0xe0 && (text.location[1] & 0xc0) == 0x80 && (text.location[2] & 0xc0) == 0x80 )
				return 3;
		
		case 2:
			if ((text.location[0] & 0xe0) == 0xc0 && (text.location[1] & 0xc0) == 0x80)
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
			if ((text->location[0] & 0xf8) == 0xf0 && (text->location[1] & 0xc0) == 0x80 && (text->location[2] & 0xc0) == 0x80 && (text->location[3] & 0xc0) == 0x80)
			{
				cp  = (*text->location++ & 0x07) << 18;
				cp |= (*text->location++ & 0x3f) << 12;
				cp |= (*text->location++ & 0x3f) << 6;
				cp |= (*text->location++ & 0x3f);
				text->length -= 4;
				return cp;
			}
		
		case 3:
			if ((text->location[0] & 0xf0) == 0xe0 && (text->location[1] & 0xc0) == 0x80 && (text->location[2] & 0xc0) == 0x80 )
			{
				cp  = (*text->location++ & 0x0f) << 12;
				cp |= (*text->location++ & 0x3f) << 6;
				cp |= (*text->location++ & 0x3f);
				text->length -= 3;
				return cp;
			}
		
		case 2:
			if ((text->location[0] & 0xe0) == 0xc0 && (text->location[1] & 0xc0) == 0x80)
			{
				cp  = (*text->location++ & 0x1f) << 6;
				cp |= (*text->location++ & 0x3f);
				text->length -= 2;
				return cp;
			}
		
		case 1:
			--text->length;
			return *text->location++;
		
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

//
//  text.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#include "text.h"

// MARK: - Private

#define textMake(T) { sizeof(T) - 1, T }

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
		length,
		location,
	};
}

struct Text join (struct Text from, struct Text to)
{
	return make(from.location, to.location - from.location + to.length);
}

uint16_t toUTF16 (struct Text text, uint16_t *wbuffer)
{
	const char *buffer = text.location;
	uint16_t index, windex;
	uint32_t u;
	
	for (index = 0, windex = 0; index < text.length;)
	{
		u = buffer[index++];
		
		switch (text.length - index)
		{
			default:
			case 3:
				if ((u & 0xf8) == 0xf0 && (buffer[index] & 0xc0) == 0x80 && (buffer[index + 1] & 0xc0) == 0x80 && (buffer[index + 2] & 0xc0) == 0x80)
				{
					u  =               (u & 0x07) << 18;
					u |= (buffer[index++] & 0x3f) << 12;
					u |= (buffer[index++] & 0x3f) << 6;
					u |= (buffer[index++] & 0x3f);
					break;
				}
			
			case 2:
				if ((u & 0xf0) == 0xe0 && (buffer[index] & 0xc0) == 0x80 && (buffer[index + 1] & 0xc0) == 0x80 )
				{
					u  =               (u & 0x0f) << 12;
					u |= (buffer[index++] & 0x3f) << 6;
					u |= (buffer[index++] & 0x3f);
					break;
				}
			
			case 1:
				if ((u & 0xe0) == 0xc0 && (buffer[index] & 0xc0) == 0x80)
				{
					u  =               (u & 0x1f) << 6;
					u |= (buffer[index++] & 0x3f);
					break;
				}
			
			case 0:
				break;
		}
		
		if (u > 0x010000)
		{
			u -= 0x010000;
			wbuffer[windex++] = ((u >> 10) & 0x3ff) + 0xd800;
			wbuffer[windex++] = ((u >>  0) & 0x3ff) + 0xdc00;
		}
		else
			wbuffer[windex++] = u;
	}
	
	return windex;
}

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

static const struct Text nullText = textMake("null");
static const struct Text undefinedText = textMake("undefined");
static const struct Text trueText = textMake("true");
static const struct Text falseText = textMake("false");
static const struct Text booleanText = textMake("boolean");
static const struct Text numberText = textMake("number");
static const struct Text stringText = textMake("string");
static const struct Text objectText = textMake("object");
static const struct Text functionText = textMake("function");
static const struct Text zeroText = textMake("0");
static const struct Text oneText = textMake("1");
static const struct Text nanText = textMake("NaN");
static const struct Text infinityText = textMake("Infinity");
static const struct Text negativeInfinityText = textMake("-Infinity");

static const struct Text nullTypeText = textMake("[object Null]");
static const struct Text undefinedTypeText =  textMake("[object Undefined]");
static const struct Text objectTypeText = textMake("[object Object]");
static const struct Text errorTypeText = textMake("[object Error]");
static const struct Text arrayTypeText = textMake("[object Array]");
static const struct Text stringTypeText = textMake("[object String]");
static const struct Text dateTypeText = textMake("[object Date]");
static const struct Text functionTypeText = textMake("[object Function]");
static const struct Text argumentsTypeText = textMake("[object Arguments]");

static const struct Text errorNameText = textMake("Error");
static const struct Text rangeErrorNameText = textMake("RangeError");
static const struct Text referenceErrorNameText = textMake("ReferenceError");
static const struct Text syntaxErrorNameText = textMake("SyntaxError");
static const struct Text typeErrorNameText = textMake("TypeError");
static const struct Text uriErrorNameText = textMake("URIError");

static const struct Text inputErrorNameText = textMake("InputError");

static const struct Text nativeCodeText = textMake("[native code]");

// MARK: - Static Members

// MARK: - Methods

Structure make (const char *location, uint16_t length)
{
	return (Structure){
		length,
		location,
	};
}

Structure join (Structure from, Structure to)
{
	return make(from.location, to.location - from.location + to.length);
}

uint16_t toUTF16 (Structure text, uint16_t *wbuffer)
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

// MARK: Texts

const Instance undefined (void)
{
	return &undefinedText;
}

const Instance null (void)
{
	return &nullText;
}

const Instance false (void)
{
	return &falseText;
}

const Instance true (void)
{
	return &trueText;
}

const Instance boolean (void)
{
	return &booleanText;
}

const Instance number (void)
{
	return &numberText;
}

const Instance string (void)
{
	return &stringText;
}

const Instance object (void)
{
	return &objectText;
}

const Instance function (void)
{
	return &functionText;
}

const Instance zero (void)
{
	return &zeroText;
}

const Instance one (void)
{
	return &oneText;
}

const Instance NaN (void)
{
	return &nanText;
}

const Instance Infinity (void)
{
	return &infinityText;
}

const Instance negativeInfinity (void)
{
	return &negativeInfinityText;
}

const Instance nativeCode (void)
{
	return &nativeCodeText;
}


// MARK: Type

const Instance nullType (void)
{
	return &nullTypeText;
}

const Instance undefinedType (void)
{
	return &undefinedTypeText;
}

const Instance objectType (void)
{
	return &objectTypeText;
}

const Instance errorType (void)
{
	return &errorTypeText;
}

const Instance arrayType (void)
{
	return &arrayTypeText;
}

const Instance stringType (void)
{
	return &stringTypeText;
}

const Instance dateType (void)
{
	return &dateTypeText;
}

const Instance functionType (void)
{
	return &functionTypeText;
}

const Instance argumentsType (void)
{
	return &argumentsTypeText;
}


// MARK: Name

const Instance errorName (void)
{
	return &errorNameText;
}

const Instance rangeErrorName (void)
{
	return &rangeErrorNameText;
}

const Instance referenceErrorName (void)
{
	return &referenceErrorNameText;
}

const Instance syntaxErrorName (void)
{
	return &syntaxErrorNameText;
}

const Instance typeErrorName (void)
{
	return &typeErrorNameText;
}

const Instance uriErrorName (void)
{
	return &uriErrorNameText;
}

const Instance inputErrorName (void)
{
	return &inputErrorNameText;
}


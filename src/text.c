//
//  text.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#include "text.h"

// MARK: - Private

static const struct Text nullText = { 4, "null" };
static const struct Text undefinedText = { 9, "undefined" };
static const struct Text trueText = { 4, "true" };
static const struct Text falseText = { 5, "false" };
static const struct Text booleanText = { 7, "boolean" };
static const struct Text numberText = { 6, "number" };
static const struct Text stringText = { 6, "string" };
static const struct Text objectText = { 6, "object" };
static const struct Text functionText = { 8, "function" };
static const struct Text zeroText = { 1, "0" };
static const struct Text oneText = { 1, "1" };
static const struct Text nanText = { 3, "NaN" };
static const struct Text infinityText = { 8, "Infinity" };
static const struct Text negativeInfinityText = { 9, "-Infinity" };

static const struct Text nullTypeText = { 13, "[object Null]" };
static const struct Text undefinedTypeText =  { 18, "[object Undefined]" };
static const struct Text objectTypeText = { 15, "[object Object]" };
static const struct Text errorTypeText = { 14, "[object Error]" };
static const struct Text arrayTypeText = { 14, "[object Array]" };
static const struct Text stringTypeText = { 15, "[object String]" };
static const struct Text dateTypeText = { 14, "[object Date]" };

static const struct Text errorNameText = { 5, "Error" };
static const struct Text rangeErrorNameText = { 10, "RangeError" };
static const struct Text referenceErrorNameText = { 14, "ReferenceError" };
static const struct Text syntaxErrorNameText = { 11, "SyntaxError" };
static const struct Text typeErrorNameText = { 9, "TypeError" };
static const struct Text uriErrorNameText = { 8, "URIError" };

static const struct Text nativeCodeText = { 13, "[native code]" };

// MARK: - Static Members

// MARK: - Methods

Structure make (const char *location, uint16_t length)
{
	return (Structure){
		.length = length,
		.location = location,
	};
}

Structure join (Structure from, Structure to)
{
	return make(from.location, to.location - from.location + to.length);
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


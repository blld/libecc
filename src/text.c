//
//  text.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#define Implementation
#include "text.h"

// MARK: - Private

#define textMake(N, T) \
	static const char N ## Literal[] = T; \
	const struct Text Text(N) = { .bytes = N ## Literal, .length = sizeof N ## Literal - 1 }

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
textMake(emptyRegExp, "/(?:)/");

textMake(nullType, "[object Null]");
textMake(undefinedType, "[object Undefined]");
textMake(objectType, "[object Object]");
textMake(errorType, "[object Error]");
textMake(arrayType, "[object Array]");
textMake(stringType, "[object String]");
textMake(regexpType, "[object RegExp]");
textMake(numberType, "[object Number]");
textMake(booleanType, "[object Boolean]");
textMake(dateType, "[object Date]");
textMake(functionType, "[object Function]");
textMake(argumentsType, "[object Arguments]");
textMake(mathType, "[object Math]");
textMake(jsonType, "[object JSON]");
textMake(globalType, "[object Global]");

textMake(errorName, "Error");
textMake(rangeErrorName, "RangeError");
textMake(referenceErrorName, "ReferenceError");
textMake(syntaxErrorName, "SyntaxError");
textMake(typeErrorName, "TypeError");
textMake(uriErrorName, "URIError");
textMake(inputErrorName, "InputError");
textMake(evalErrorName, "EvalError");

// MARK: - Static Members

static const char lowers[] = {
	0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
	0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0xC2,0xB5,0xC3,0xA0,
	0xC3,0xA1,0xC3,0xA2,0xC3,0xA3,0xC3,0xA4,0xC3,0xA5,0xC3,0xA6,0xC3,0xA7,0xC3,
	0xA8,0xC3,0xA9,0xC3,0xAA,0xC3,0xAB,0xC3,0xAC,0xC3,0xAD,0xC3,0xAE,0xC3,0xAF,
	0xC3,0xB0,0xC3,0xB1,0xC3,0xB2,0xC3,0xB3,0xC3,0xB4,0xC3,0xB5,0xC3,0xB6,0xC3,
	0xB8,0xC3,0xB9,0xC3,0xBA,0xC3,0xBB,0xC3,0xBC,0xC3,0xBD,0xC3,0xBE,0xC3,0x9F,
	0xC3,0xBF,0xC4,0x81,0xC4,0x83,0xC4,0x85,0xC4,0x87,0xC4,0x89,0xC4,0x8B,0xC4,
	0x8D,0xC4,0x8F,0xC4,0x91,0xC4,0x93,0xC4,0x95,0xC4,0x97,0xC4,0x99,0xC4,0x9B,
	0xC4,0x9D,0xC4,0x9F,0xC4,0xA1,0xC4,0xA3,0xC4,0xA5,0xC4,0xA7,0xC4,0xA9,0xC4,
	0xAB,0xC4,0xAD,0xC4,0xAF,0x69,0xCC,0x87,0xC4,0xB1,0xC4,0xB3,0xC4,0xB5,0xC4,
	0xB7,0xC4,0xBA,0xC4,0xBC,0xC4,0xBE,0xC5,0x80,0xC5,0x82,0xC5,0x84,0xC5,0x86,
	0xC5,0x88,0xC5,0x89,255 ,0xC5,0x8B,0xC5,0x8D,0xC5,0x8F,0xC5,0x91,0xC5,0x93,
	0xC5,0x95,0xC5,0x97,0xC5,0x99,0xC5,0x9B,0xC5,0x9D,0xC5,0x9F,0xC5,0xA1,0xC5,
	0xA3,0xC5,0xA5,0xC5,0xA7,0xC5,0xA9,0xC5,0xAB,0xC5,0xAD,0xC5,0xAF,0xC5,0xB1,
	0xC5,0xB3,0xC5,0xB5,0xC5,0xB7,0xC5,0xBA,0xC5,0xBC,0xC5,0xBE,0xC5,0xBF,0xC6,
	0x80,0xC9,0x93,0xC6,0x83,0xC6,0x85,0xC9,0x94,0xC6,0x88,0xC9,0x96,0xC9,0x97,
	0xC6,0x8C,0xC7,0x9D,0xC9,0x99,0xC9,0x9B,0xC6,0x92,0xC9,0xA0,0xC9,0xA3,0xC6,
	0x95,0xC9,0xA9,0xC9,0xA8,0xC6,0x99,0xC6,0x9A,0xC9,0xAF,0xC9,0xB2,0xC6,0x9E,
	0xC9,0xB5,0xC6,0xA1,0xC6,0xA3,0xC6,0xA5,0xCA,0x80,0xC6,0xA8,0xCA,0x83,0xC6,
	0xAD,0xCA,0x88,0xC6,0xB0,0xCA,0x8A,0xCA,0x8B,0xC6,0xB4,0xC6,0xB6,0xCA,0x92,
	0xC6,0xB9,0xC6,0xBD,0xC6,0xBF,0xC7,0x86,0xC7,0x89,0xC7,0x8C,0xC7,0x8E,0xC7,
	0x90,0xC7,0x92,0xC7,0x94,0xC7,0x96,0xC7,0x98,0xC7,0x9A,0xC7,0x9C,0xC7,0x9F,
	0xC7,0xA1,0xC7,0xA3,0xC7,0xA5,0xC7,0xA7,0xC7,0xA9,0xC7,0xAB,0xC7,0xAD,0xC7,
	0xAF,0xC7,0xB0,255 ,0xC7,0xB3,0xC7,0xB5,0xC7,0xB9,0xC7,0xBB,0xC7,0xBD,0xC7,
	0xBF,0xC8,0x81,0xC8,0x83,0xC8,0x85,0xC8,0x87,0xC8,0x89,0xC8,0x8B,0xC8,0x8D,
	0xC8,0x8F,0xC8,0x91,0xC8,0x93,0xC8,0x95,0xC8,0x97,0xC8,0x99,0xC8,0x9B,0xC8,
	0x9D,0xC8,0x9F,0xC8,0xA3,0xC8,0xA5,0xC8,0xA7,0xC8,0xA9,0xC8,0xAB,0xC8,0xAD,
	0xC8,0xAF,0xC8,0xB1,0xC8,0xB3,0xE2,0xB1,0xA5,0xC8,0xBC,0xE2,0xB1,0xA6,0xC8,
	0xBF,255 ,0xC9,0x80,255 ,0xC9,0x82,0xCA,0x89,0xCA,0x8C,0xC9,0x87,0xC9,0x89,
	0xC9,0x8B,0xC9,0x8D,0xC9,0x8F,0xC9,0x90,255 ,0xC9,0x91,255 ,0xC9,0x92,255 ,
	0xC9,0x9C,255 ,0xC9,0xA1,255 ,0xC9,0xA5,255 ,0xC9,0xA6,255 ,0xC9,0xAB,255 ,
	0xC9,0xAC,255 ,0xC9,0xB1,255 ,0xC9,0xBD,255 ,0xCA,0x87,255 ,0xCA,0x9E,255 ,
	0xCD,0x85,0xCD,0xB1,0xCD,0xB3,0xCD,0xB7,0xCD,0xBB,0xCD,0xBC,0xCD,0xBD,0xCF,
	0xB3,0xCE,0xAC,0xCE,0xAD,0xCE,0xAE,0xCE,0xAF,0xCF,0x8C,0xCF,0x8D,0xCF,0x8E,
	0xCE,0x90,255 ,255 ,255 ,255 ,0xCE,0xB1,0xCE,0xB2,0xCE,0xB3,0xCE,0xB4,0xCE,
	0xB5,0xCE,0xB6,0xCE,0xB7,0xCE,0xB8,0xCE,0xB9,0xCE,0xBA,0xCE,0xBB,0xCE,0xBC,
	0xCE,0xBD,0xCE,0xBE,0xCE,0xBF,0xCF,0x80,0xCF,0x81,0xCF,0x83,0xCF,0x84,0xCF,
	0x85,0xCF,0x86,0xCF,0x87,0xCF,0x88,0xCF,0x89,0xCF,0x8A,0xCF,0x8B,0xCE,0xB0,
	255 ,255 ,255 ,255 ,0xCF,0x82,0xCF,0x97,0xCF,0x90,0xCF,0x91,0xCF,0x95,0xCF,
	0x96,0xCF,0x99,0xCF,0x9B,0xCF,0x9D,0xCF,0x9F,0xCF,0xA1,0xCF,0xA3,0xCF,0xA5,
	0xCF,0xA7,0xCF,0xA9,0xCF,0xAB,0xCF,0xAD,0xCF,0xAF,0xCF,0xB0,0xCF,0xB1,0xCF,
	0xB2,0xCE,0xB8,0xCF,0xB5,0xCF,0xB8,0xCF,0xBB,0xD1,0x90,0xD1,0x91,0xD1,0x92,
	0xD1,0x93,0xD1,0x94,0xD1,0x95,0xD1,0x96,0xD1,0x97,0xD1,0x98,0xD1,0x99,0xD1,
	0x9A,0xD1,0x9B,0xD1,0x9C,0xD1,0x9D,0xD1,0x9E,0xD1,0x9F,0xD0,0xB0,0xD0,0xB1,
	0xD0,0xB2,0xD0,0xB3,0xD0,0xB4,0xD0,0xB5,0xD0,0xB6,0xD0,0xB7,0xD0,0xB8,0xD0,
	0xB9,0xD0,0xBA,0xD0,0xBB,0xD0,0xBC,0xD0,0xBD,0xD0,0xBE,0xD0,0xBF,0xD1,0x80,
	0xD1,0x81,0xD1,0x82,0xD1,0x83,0xD1,0x84,0xD1,0x85,0xD1,0x86,0xD1,0x87,0xD1,
	0x88,0xD1,0x89,0xD1,0x8A,0xD1,0x8B,0xD1,0x8C,0xD1,0x8D,0xD1,0x8E,0xD1,0x8F,
	0xD1,0xA1,0xD1,0xA3,0xD1,0xA5,0xD1,0xA7,0xD1,0xA9,0xD1,0xAB,0xD1,0xAD,0xD1,
	0xAF,0xD1,0xB1,0xD1,0xB3,0xD1,0xB5,0xD1,0xB7,0xD1,0xB9,0xD1,0xBB,0xD1,0xBD,
	0xD1,0xBF,0xD2,0x81,0xD2,0x8B,0xD2,0x8D,0xD2,0x8F,0xD2,0x91,0xD2,0x93,0xD2,
	0x95,0xD2,0x97,0xD2,0x99,0xD2,0x9B,0xD2,0x9D,0xD2,0x9F,0xD2,0xA1,0xD2,0xA3,
	0xD2,0xA5,0xD2,0xA7,0xD2,0xA9,0xD2,0xAB,0xD2,0xAD,0xD2,0xAF,0xD2,0xB1,0xD2,
	0xB3,0xD2,0xB5,0xD2,0xB7,0xD2,0xB9,0xD2,0xBB,0xD2,0xBD,0xD2,0xBF,0xD3,0x8F,
	0xD3,0x82,0xD3,0x84,0xD3,0x86,0xD3,0x88,0xD3,0x8A,0xD3,0x8C,0xD3,0x8E,0xD3,
	0x91,0xD3,0x93,0xD3,0x95,0xD3,0x97,0xD3,0x99,0xD3,0x9B,0xD3,0x9D,0xD3,0x9F,
	0xD3,0xA1,0xD3,0xA3,0xD3,0xA5,0xD3,0xA7,0xD3,0xA9,0xD3,0xAB,0xD3,0xAD,0xD3,
	0xAF,0xD3,0xB1,0xD3,0xB3,0xD3,0xB5,0xD3,0xB7,0xD3,0xB9,0xD3,0xBB,0xD3,0xBD,
	0xD3,0xBF,0xD4,0x81,0xD4,0x83,0xD4,0x85,0xD4,0x87,0xD4,0x89,0xD4,0x8B,0xD4,
	0x8D,0xD4,0x8F,0xD4,0x91,0xD4,0x93,0xD4,0x95,0xD4,0x97,0xD4,0x99,0xD4,0x9B,
	0xD4,0x9D,0xD4,0x9F,0xD4,0xA1,0xD4,0xA3,0xD4,0xA5,0xD4,0xA7,0xD4,0xA9,0xD4,
	0xAB,0xD4,0xAD,0xD4,0xAF,0xD5,0xA1,0xD5,0xA2,0xD5,0xA3,0xD5,0xA4,0xD5,0xA5,
	0xD5,0xA6,0xD5,0xA7,0xD5,0xA8,0xD5,0xA9,0xD5,0xAA,0xD5,0xAB,0xD5,0xAC,0xD5,
	0xAD,0xD5,0xAE,0xD5,0xAF,0xD5,0xB0,0xD5,0xB1,0xD5,0xB2,0xD5,0xB3,0xD5,0xB4,
	0xD5,0xB5,0xD5,0xB6,0xD5,0xB7,0xD5,0xB8,0xD5,0xB9,0xD5,0xBA,0xD5,0xBB,0xD5,
	0xBC,0xD5,0xBD,0xD5,0xBE,0xD5,0xBF,0xD6,0x80,0xD6,0x81,0xD6,0x82,0xD6,0x83,
	0xD6,0x84,0xD6,0x85,0xD6,0x86,0xD6,0x87,255 ,255 ,0xE2,0xB4,0x80,0xE2,0xB4,
	0x81,0xE2,0xB4,0x82,0xE2,0xB4,0x83,0xE2,0xB4,0x84,0xE2,0xB4,0x85,0xE2,0xB4,
	0x86,0xE2,0xB4,0x87,0xE2,0xB4,0x88,0xE2,0xB4,0x89,0xE2,0xB4,0x8A,0xE2,0xB4,
	0x8B,0xE2,0xB4,0x8C,0xE2,0xB4,0x8D,0xE2,0xB4,0x8E,0xE2,0xB4,0x8F,0xE2,0xB4,
	0x90,0xE2,0xB4,0x91,0xE2,0xB4,0x92,0xE2,0xB4,0x93,0xE2,0xB4,0x94,0xE2,0xB4,
	0x95,0xE2,0xB4,0x96,0xE2,0xB4,0x97,0xE2,0xB4,0x98,0xE2,0xB4,0x99,0xE2,0xB4,
	0x9A,0xE2,0xB4,0x9B,0xE2,0xB4,0x9C,0xE2,0xB4,0x9D,0xE2,0xB4,0x9E,0xE2,0xB4,
	0x9F,0xE2,0xB4,0xA0,0xE2,0xB4,0xA1,0xE2,0xB4,0xA2,0xE2,0xB4,0xA3,0xE2,0xB4,
	0xA4,0xE2,0xB4,0xA5,0xE2,0xB4,0xA7,0xE2,0xB4,0xAD,0xE1,0xB5,0xB9,0xE1,0xB5,
	0xBD,0xE1,0xB8,0x81,0xE1,0xB8,0x83,0xE1,0xB8,0x85,0xE1,0xB8,0x87,0xE1,0xB8,
	0x89,0xE1,0xB8,0x8B,0xE1,0xB8,0x8D,0xE1,0xB8,0x8F,0xE1,0xB8,0x91,0xE1,0xB8,
	0x93,0xE1,0xB8,0x95,0xE1,0xB8,0x97,0xE1,0xB8,0x99,0xE1,0xB8,0x9B,0xE1,0xB8,
	0x9D,0xE1,0xB8,0x9F,0xE1,0xB8,0xA1,0xE1,0xB8,0xA3,0xE1,0xB8,0xA5,0xE1,0xB8,
	0xA7,0xE1,0xB8,0xA9,0xE1,0xB8,0xAB,0xE1,0xB8,0xAD,0xE1,0xB8,0xAF,0xE1,0xB8,
	0xB1,0xE1,0xB8,0xB3,0xE1,0xB8,0xB5,0xE1,0xB8,0xB7,0xE1,0xB8,0xB9,0xE1,0xB8,
	0xBB,0xE1,0xB8,0xBD,0xE1,0xB8,0xBF,0xE1,0xB9,0x81,0xE1,0xB9,0x83,0xE1,0xB9,
	0x85,0xE1,0xB9,0x87,0xE1,0xB9,0x89,0xE1,0xB9,0x8B,0xE1,0xB9,0x8D,0xE1,0xB9,
	0x8F,0xE1,0xB9,0x91,0xE1,0xB9,0x93,0xE1,0xB9,0x95,0xE1,0xB9,0x97,0xE1,0xB9,
	0x99,0xE1,0xB9,0x9B,0xE1,0xB9,0x9D,0xE1,0xB9,0x9F,0xE1,0xB9,0xA1,0xE1,0xB9,
	0xA3,0xE1,0xB9,0xA5,0xE1,0xB9,0xA7,0xE1,0xB9,0xA9,0xE1,0xB9,0xAB,0xE1,0xB9,
	0xAD,0xE1,0xB9,0xAF,0xE1,0xB9,0xB1,0xE1,0xB9,0xB3,0xE1,0xB9,0xB5,0xE1,0xB9,
	0xB7,0xE1,0xB9,0xB9,0xE1,0xB9,0xBB,0xE1,0xB9,0xBD,0xE1,0xB9,0xBF,0xE1,0xBA,
	0x81,0xE1,0xBA,0x83,0xE1,0xBA,0x85,0xE1,0xBA,0x87,0xE1,0xBA,0x89,0xE1,0xBA,
	0x8B,0xE1,0xBA,0x8D,0xE1,0xBA,0x8F,0xE1,0xBA,0x91,0xE1,0xBA,0x93,0xE1,0xBA,
	0x95,0xE1,0xBA,0x96,0xE1,0xBA,0x97,0xE1,0xBA,0x98,0xE1,0xBA,0x99,0xE1,0xBA,
	0x9A,0xE1,0xBA,0x9B,0xC3,0x9F,255 ,0xE1,0xBA,0xA1,0xE1,0xBA,0xA3,0xE1,0xBA,
	0xA5,0xE1,0xBA,0xA7,0xE1,0xBA,0xA9,0xE1,0xBA,0xAB,0xE1,0xBA,0xAD,0xE1,0xBA,
	0xAF,0xE1,0xBA,0xB1,0xE1,0xBA,0xB3,0xE1,0xBA,0xB5,0xE1,0xBA,0xB7,0xE1,0xBA,
	0xB9,0xE1,0xBA,0xBB,0xE1,0xBA,0xBD,0xE1,0xBA,0xBF,0xE1,0xBB,0x81,0xE1,0xBB,
	0x83,0xE1,0xBB,0x85,0xE1,0xBB,0x87,0xE1,0xBB,0x89,0xE1,0xBB,0x8B,0xE1,0xBB,
	0x8D,0xE1,0xBB,0x8F,0xE1,0xBB,0x91,0xE1,0xBB,0x93,0xE1,0xBB,0x95,0xE1,0xBB,
	0x97,0xE1,0xBB,0x99,0xE1,0xBB,0x9B,0xE1,0xBB,0x9D,0xE1,0xBB,0x9F,0xE1,0xBB,
	0xA1,0xE1,0xBB,0xA3,0xE1,0xBB,0xA5,0xE1,0xBB,0xA7,0xE1,0xBB,0xA9,0xE1,0xBB,
	0xAB,0xE1,0xBB,0xAD,0xE1,0xBB,0xAF,0xE1,0xBB,0xB1,0xE1,0xBB,0xB3,0xE1,0xBB,
	0xB5,0xE1,0xBB,0xB7,0xE1,0xBB,0xB9,0xE1,0xBB,0xBB,0xE1,0xBB,0xBD,0xE1,0xBB,
	0xBF,0xE1,0xBC,0x80,0xE1,0xBC,0x81,0xE1,0xBC,0x82,0xE1,0xBC,0x83,0xE1,0xBC,
	0x84,0xE1,0xBC,0x85,0xE1,0xBC,0x86,0xE1,0xBC,0x87,0xE1,0xBC,0x90,0xE1,0xBC,
	0x91,0xE1,0xBC,0x92,0xE1,0xBC,0x93,0xE1,0xBC,0x94,0xE1,0xBC,0x95,0xE1,0xBC,
	0xA0,0xE1,0xBC,0xA1,0xE1,0xBC,0xA2,0xE1,0xBC,0xA3,0xE1,0xBC,0xA4,0xE1,0xBC,
	0xA5,0xE1,0xBC,0xA6,0xE1,0xBC,0xA7,0xE1,0xBC,0xB0,0xE1,0xBC,0xB1,0xE1,0xBC,
	0xB2,0xE1,0xBC,0xB3,0xE1,0xBC,0xB4,0xE1,0xBC,0xB5,0xE1,0xBC,0xB6,0xE1,0xBC,
	0xB7,0xE1,0xBD,0x80,0xE1,0xBD,0x81,0xE1,0xBD,0x82,0xE1,0xBD,0x83,0xE1,0xBD,
	0x84,0xE1,0xBD,0x85,0xE1,0xBD,0x90,255 ,0xE1,0xBD,0x91,0xE1,0xBD,0x92,255 ,
	255 ,255 ,0xE1,0xBD,0x93,0xE1,0xBD,0x94,255 ,255 ,255 ,0xE1,0xBD,0x95,0xE1,
	0xBD,0x96,255 ,255 ,255 ,0xE1,0xBD,0x97,0xE1,0xBD,0xA0,0xE1,0xBD,0xA1,0xE1,
	0xBD,0xA2,0xE1,0xBD,0xA3,0xE1,0xBD,0xA4,0xE1,0xBD,0xA5,0xE1,0xBD,0xA6,0xE1,
	0xBD,0xA7,0xE1,0xBD,0xB0,0xE1,0xBD,0xB1,0xE1,0xBD,0xB2,0xE1,0xBD,0xB3,0xE1,
	0xBD,0xB4,0xE1,0xBD,0xB5,0xE1,0xBD,0xB6,0xE1,0xBD,0xB7,0xE1,0xBD,0xB8,0xE1,
	0xBD,0xB9,0xE1,0xBD,0xBA,0xE1,0xBD,0xBB,0xE1,0xBD,0xBC,0xE1,0xBD,0xBD,0xE1,
	0xBE,0x80,255 ,255 ,0xE1,0xBE,0x81,255 ,255 ,0xE1,0xBE,0x82,255 ,255 ,0xE1,
	0xBE,0x83,255 ,255 ,0xE1,0xBE,0x84,255 ,255 ,0xE1,0xBE,0x85,255 ,255 ,0xE1,
	0xBE,0x86,255 ,255 ,0xE1,0xBE,0x87,255 ,255 ,0xE1,0xBE,0x90,255 ,255 ,0xE1,
	0xBE,0x91,255 ,255 ,0xE1,0xBE,0x92,255 ,255 ,0xE1,0xBE,0x93,255 ,255 ,0xE1,
	0xBE,0x94,255 ,255 ,0xE1,0xBE,0x95,255 ,255 ,0xE1,0xBE,0x96,255 ,255 ,0xE1,
	0xBE,0x97,255 ,255 ,0xE1,0xBE,0xA0,255 ,255 ,0xE1,0xBE,0xA1,255 ,255 ,0xE1,
	0xBE,0xA2,255 ,255 ,0xE1,0xBE,0xA3,255 ,255 ,0xE1,0xBE,0xA4,255 ,255 ,0xE1,
	0xBE,0xA5,255 ,255 ,0xE1,0xBE,0xA6,255 ,255 ,0xE1,0xBE,0xA7,255 ,255 ,0xE1,
	0xBE,0xB0,0xE1,0xBE,0xB1,0xE1,0xBE,0xB2,255 ,255 ,0xE1,0xBE,0xB3,255 ,0xE1,
	0xBE,0xB4,255 ,0xE1,0xBE,0xB6,255 ,0xE1,0xBE,0xB7,255 ,255 ,255 ,0xE1,0xBE,
	0xBE,0xE1,0xBF,0x82,255 ,255 ,0xE1,0xBF,0x83,255 ,0xE1,0xBF,0x84,255 ,0xE1,
	0xBF,0x86,255 ,0xE1,0xBF,0x87,255 ,255 ,255 ,0xE1,0xBF,0x90,0xE1,0xBF,0x91,
	0xE1,0xBF,0x92,255 ,255 ,255 ,0xE1,0xBF,0x93,255 ,255 ,255 ,0xE1,0xBF,0x96,
	255 ,0xE1,0xBF,0x97,255 ,255 ,255 ,0xE1,0xBF,0xA0,0xE1,0xBF,0xA1,0xE1,0xBF,
	0xA2,255 ,255 ,255 ,0xE1,0xBF,0xA3,255 ,255 ,255 ,0xE1,0xBF,0xA4,255 ,0xE1,
	0xBF,0xA5,0xE1,0xBF,0xA6,255 ,0xE1,0xBF,0xA7,255 ,255 ,255 ,0xE1,0xBF,0xB2,
	255 ,255 ,0xE1,0xBF,0xB3,255 ,0xE1,0xBF,0xB4,255 ,0xE1,0xBF,0xB6,255 ,0xE1,
	0xBF,0xB7,255 ,255 ,255 ,0xCF,0x89,255 ,0x6b,255 ,255 ,0xC3,0xA5,255 ,0xE2,
	0x85,0x8E,0xE2,0x85,0xB0,0xE2,0x85,0xB1,0xE2,0x85,0xB2,0xE2,0x85,0xB3,0xE2,
	0x85,0xB4,0xE2,0x85,0xB5,0xE2,0x85,0xB6,0xE2,0x85,0xB7,0xE2,0x85,0xB8,0xE2,
	0x85,0xB9,0xE2,0x85,0xBA,0xE2,0x85,0xBB,0xE2,0x85,0xBC,0xE2,0x85,0xBD,0xE2,
	0x85,0xBE,0xE2,0x85,0xBF,0xE2,0x86,0x84,0xE2,0x93,0x90,0xE2,0x93,0x91,0xE2,
	0x93,0x92,0xE2,0x93,0x93,0xE2,0x93,0x94,0xE2,0x93,0x95,0xE2,0x93,0x96,0xE2,
	0x93,0x97,0xE2,0x93,0x98,0xE2,0x93,0x99,0xE2,0x93,0x9A,0xE2,0x93,0x9B,0xE2,
	0x93,0x9C,0xE2,0x93,0x9D,0xE2,0x93,0x9E,0xE2,0x93,0x9F,0xE2,0x93,0xA0,0xE2,
	0x93,0xA1,0xE2,0x93,0xA2,0xE2,0x93,0xA3,0xE2,0x93,0xA4,0xE2,0x93,0xA5,0xE2,
	0x93,0xA6,0xE2,0x93,0xA7,0xE2,0x93,0xA8,0xE2,0x93,0xA9,0xE2,0xB0,0xB0,0xE2,
	0xB0,0xB1,0xE2,0xB0,0xB2,0xE2,0xB0,0xB3,0xE2,0xB0,0xB4,0xE2,0xB0,0xB5,0xE2,
	0xB0,0xB6,0xE2,0xB0,0xB7,0xE2,0xB0,0xB8,0xE2,0xB0,0xB9,0xE2,0xB0,0xBA,0xE2,
	0xB0,0xBB,0xE2,0xB0,0xBC,0xE2,0xB0,0xBD,0xE2,0xB0,0xBE,0xE2,0xB0,0xBF,0xE2,
	0xB1,0x80,0xE2,0xB1,0x81,0xE2,0xB1,0x82,0xE2,0xB1,0x83,0xE2,0xB1,0x84,0xE2,
	0xB1,0x85,0xE2,0xB1,0x86,0xE2,0xB1,0x87,0xE2,0xB1,0x88,0xE2,0xB1,0x89,0xE2,
	0xB1,0x8A,0xE2,0xB1,0x8B,0xE2,0xB1,0x8C,0xE2,0xB1,0x8D,0xE2,0xB1,0x8E,0xE2,
	0xB1,0x8F,0xE2,0xB1,0x90,0xE2,0xB1,0x91,0xE2,0xB1,0x92,0xE2,0xB1,0x93,0xE2,
	0xB1,0x94,0xE2,0xB1,0x95,0xE2,0xB1,0x96,0xE2,0xB1,0x97,0xE2,0xB1,0x98,0xE2,
	0xB1,0x99,0xE2,0xB1,0x9A,0xE2,0xB1,0x9B,0xE2,0xB1,0x9C,0xE2,0xB1,0x9D,0xE2,
	0xB1,0x9E,0xE2,0xB1,0xA1,0xE2,0xB1,0xA8,0xE2,0xB1,0xAA,0xE2,0xB1,0xAC,0xE2,
	0xB1,0xB3,0xE2,0xB1,0xB6,0xE2,0xB2,0x81,0xE2,0xB2,0x83,0xE2,0xB2,0x85,0xE2,
	0xB2,0x87,0xE2,0xB2,0x89,0xE2,0xB2,0x8B,0xE2,0xB2,0x8D,0xE2,0xB2,0x8F,0xE2,
	0xB2,0x91,0xE2,0xB2,0x93,0xE2,0xB2,0x95,0xE2,0xB2,0x97,0xE2,0xB2,0x99,0xE2,
	0xB2,0x9B,0xE2,0xB2,0x9D,0xE2,0xB2,0x9F,0xE2,0xB2,0xA1,0xE2,0xB2,0xA3,0xE2,
	0xB2,0xA5,0xE2,0xB2,0xA7,0xE2,0xB2,0xA9,0xE2,0xB2,0xAB,0xE2,0xB2,0xAD,0xE2,
	0xB2,0xAF,0xE2,0xB2,0xB1,0xE2,0xB2,0xB3,0xE2,0xB2,0xB5,0xE2,0xB2,0xB7,0xE2,
	0xB2,0xB9,0xE2,0xB2,0xBB,0xE2,0xB2,0xBD,0xE2,0xB2,0xBF,0xE2,0xB3,0x81,0xE2,
	0xB3,0x83,0xE2,0xB3,0x85,0xE2,0xB3,0x87,0xE2,0xB3,0x89,0xE2,0xB3,0x8B,0xE2,
	0xB3,0x8D,0xE2,0xB3,0x8F,0xE2,0xB3,0x91,0xE2,0xB3,0x93,0xE2,0xB3,0x95,0xE2,
	0xB3,0x97,0xE2,0xB3,0x99,0xE2,0xB3,0x9B,0xE2,0xB3,0x9D,0xE2,0xB3,0x9F,0xE2,
	0xB3,0xA1,0xE2,0xB3,0xA3,0xE2,0xB3,0xAC,0xE2,0xB3,0xAE,0xE2,0xB3,0xB3,0xEA,
	0x99,0x81,0xEA,0x99,0x83,0xEA,0x99,0x85,0xEA,0x99,0x87,0xEA,0x99,0x89,0xEA,
	0x99,0x8B,0xEA,0x99,0x8D,0xEA,0x99,0x8F,0xEA,0x99,0x91,0xEA,0x99,0x93,0xEA,
	0x99,0x95,0xEA,0x99,0x97,0xEA,0x99,0x99,0xEA,0x99,0x9B,0xEA,0x99,0x9D,0xEA,
	0x99,0x9F,0xEA,0x99,0xA1,0xEA,0x99,0xA3,0xEA,0x99,0xA5,0xEA,0x99,0xA7,0xEA,
	0x99,0xA9,0xEA,0x99,0xAB,0xEA,0x99,0xAD,0xEA,0x9A,0x81,0xEA,0x9A,0x83,0xEA,
	0x9A,0x85,0xEA,0x9A,0x87,0xEA,0x9A,0x89,0xEA,0x9A,0x8B,0xEA,0x9A,0x8D,0xEA,
	0x9A,0x8F,0xEA,0x9A,0x91,0xEA,0x9A,0x93,0xEA,0x9A,0x95,0xEA,0x9A,0x97,0xEA,
	0x9A,0x99,0xEA,0x9A,0x9B,0xEA,0x9C,0xA3,0xEA,0x9C,0xA5,0xEA,0x9C,0xA7,0xEA,
	0x9C,0xA9,0xEA,0x9C,0xAB,0xEA,0x9C,0xAD,0xEA,0x9C,0xAF,0xEA,0x9C,0xB3,0xEA,
	0x9C,0xB5,0xEA,0x9C,0xB7,0xEA,0x9C,0xB9,0xEA,0x9C,0xBB,0xEA,0x9C,0xBD,0xEA,
	0x9C,0xBF,0xEA,0x9D,0x81,0xEA,0x9D,0x83,0xEA,0x9D,0x85,0xEA,0x9D,0x87,0xEA,
	0x9D,0x89,0xEA,0x9D,0x8B,0xEA,0x9D,0x8D,0xEA,0x9D,0x8F,0xEA,0x9D,0x91,0xEA,
	0x9D,0x93,0xEA,0x9D,0x95,0xEA,0x9D,0x97,0xEA,0x9D,0x99,0xEA,0x9D,0x9B,0xEA,
	0x9D,0x9D,0xEA,0x9D,0x9F,0xEA,0x9D,0xA1,0xEA,0x9D,0xA3,0xEA,0x9D,0xA5,0xEA,
	0x9D,0xA7,0xEA,0x9D,0xA9,0xEA,0x9D,0xAB,0xEA,0x9D,0xAD,0xEA,0x9D,0xAF,0xEA,
	0x9D,0xBA,0xEA,0x9D,0xBC,0xEA,0x9D,0xBF,0xEA,0x9E,0x81,0xEA,0x9E,0x83,0xEA,
	0x9E,0x85,0xEA,0x9E,0x87,0xEA,0x9E,0x8C,0xEA,0x9E,0x91,0xEA,0x9E,0x93,0xEA,
	0x9E,0x97,0xEA,0x9E,0x99,0xEA,0x9E,0x9B,0xEA,0x9E,0x9D,0xEA,0x9E,0x9F,0xEA,
	0x9E,0xA1,0xEA,0x9E,0xA3,0xEA,0x9E,0xA5,0xEA,0x9E,0xA7,0xEA,0x9E,0xA9,0xEF,
	0xAC,0x80,0xEF,0xAC,0x81,0xEF,0xAC,0x82,0xEF,0xAC,0x83,0xEF,0xAC,0x84,0xEF,
	0xAC,0x85,0xEF,0xAC,0x86,0xEF,0xAC,0x93,255 ,0xEF,0xAC,0x94,255 ,0xEF,0xAC,
	0x95,255 ,0xEF,0xAC,0x96,255 ,0xEF,0xAC,0x97,255 ,0xEF,0xBD,0x81,0xEF,0xBD,
	0x82,0xEF,0xBD,0x83,0xEF,0xBD,0x84,0xEF,0xBD,0x85,0xEF,0xBD,0x86,0xEF,0xBD,
	0x87,0xEF,0xBD,0x88,0xEF,0xBD,0x89,0xEF,0xBD,0x8A,0xEF,0xBD,0x8B,0xEF,0xBD,
	0x8C,0xEF,0xBD,0x8D,0xEF,0xBD,0x8E,0xEF,0xBD,0x8F,0xEF,0xBD,0x90,0xEF,0xBD,
	0x91,0xEF,0xBD,0x92,0xEF,0xBD,0x93,0xEF,0xBD,0x94,0xEF,0xBD,0x95,0xEF,0xBD,
	0x96,0xEF,0xBD,0x97,0xEF,0xBD,0x98,0xEF,0xBD,0x99,0xEF,0xBD,0x9A,
	0
};

static const char uppers[] = {
	0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
	0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0xCE,0x9C,0xC3,0x80,
	0xC3,0x81,0xC3,0x82,0xC3,0x83,0xC3,0x84,0xC3,0x85,0xC3,0x86,0xC3,0x87,0xC3,
	0x88,0xC3,0x89,0xC3,0x8A,0xC3,0x8B,0xC3,0x8C,0xC3,0x8D,0xC3,0x8E,0xC3,0x8F,
	0xC3,0x90,0xC3,0x91,0xC3,0x92,0xC3,0x93,0xC3,0x94,0xC3,0x95,0xC3,0x96,0xC3,
	0x98,0xC3,0x99,0xC3,0x9A,0xC3,0x9B,0xC3,0x9C,0xC3,0x9D,0xC3,0x9E,0x53,0x53,
	0xC5,0xB8,0xC4,0x80,0xC4,0x82,0xC4,0x84,0xC4,0x86,0xC4,0x88,0xC4,0x8A,0xC4,
	0x8C,0xC4,0x8E,0xC4,0x90,0xC4,0x92,0xC4,0x94,0xC4,0x96,0xC4,0x98,0xC4,0x9A,
	0xC4,0x9C,0xC4,0x9E,0xC4,0xA0,0xC4,0xA2,0xC4,0xA4,0xC4,0xA6,0xC4,0xA8,0xC4,
	0xAA,0xC4,0xAC,0xC4,0xAE,0xC4,0xB0,255 ,0x49,255 ,0xC4,0xB2,0xC4,0xB4,0xC4,
	0xB6,0xC4,0xB9,0xC4,0xBB,0xC4,0xBD,0xC4,0xBF,0xC5,0x81,0xC5,0x83,0xC5,0x85,
	0xC5,0x87,0xCA,0xBC,0x4e,0xC5,0x8A,0xC5,0x8C,0xC5,0x8E,0xC5,0x90,0xC5,0x92,
	0xC5,0x94,0xC5,0x96,0xC5,0x98,0xC5,0x9A,0xC5,0x9C,0xC5,0x9E,0xC5,0xA0,0xC5,
	0xA2,0xC5,0xA4,0xC5,0xA6,0xC5,0xA8,0xC5,0xAA,0xC5,0xAC,0xC5,0xAE,0xC5,0xB0,
	0xC5,0xB2,0xC5,0xB4,0xC5,0xB6,0xC5,0xB9,0xC5,0xBB,0xC5,0xBD,0x53,255 ,0xC9,
	0x83,0xC6,0x81,0xC6,0x82,0xC6,0x84,0xC6,0x86,0xC6,0x87,0xC6,0x89,0xC6,0x8A,
	0xC6,0x8B,0xC6,0x8E,0xC6,0x8F,0xC6,0x90,0xC6,0x91,0xC6,0x93,0xC6,0x94,0xC7,
	0xB6,0xC6,0x96,0xC6,0x97,0xC6,0x98,0xC8,0xBD,0xC6,0x9C,0xC6,0x9D,0xC8,0xA0,
	0xC6,0x9F,0xC6,0xA0,0xC6,0xA2,0xC6,0xA4,0xC6,0xA6,0xC6,0xA7,0xC6,0xA9,0xC6,
	0xAC,0xC6,0xAE,0xC6,0xAF,0xC6,0xB1,0xC6,0xB2,0xC6,0xB3,0xC6,0xB5,0xC6,0xB7,
	0xC6,0xB8,0xC6,0xBC,0xC7,0xB7,0xC7,0x84,0xC7,0x87,0xC7,0x8A,0xC7,0x8D,0xC7,
	0x8F,0xC7,0x91,0xC7,0x93,0xC7,0x95,0xC7,0x97,0xC7,0x99,0xC7,0x9B,0xC7,0x9E,
	0xC7,0xA0,0xC7,0xA2,0xC7,0xA4,0xC7,0xA6,0xC7,0xA8,0xC7,0xAA,0xC7,0xAC,0xC7,
	0xAE,0x4a,0xCC,0x8C,0xC7,0xB1,0xC7,0xB4,0xC7,0xB8,0xC7,0xBA,0xC7,0xBC,0xC7,
	0xBE,0xC8,0x80,0xC8,0x82,0xC8,0x84,0xC8,0x86,0xC8,0x88,0xC8,0x8A,0xC8,0x8C,
	0xC8,0x8E,0xC8,0x90,0xC8,0x92,0xC8,0x94,0xC8,0x96,0xC8,0x98,0xC8,0x9A,0xC8,
	0x9C,0xC8,0x9E,0xC8,0xA2,0xC8,0xA4,0xC8,0xA6,0xC8,0xA8,0xC8,0xAA,0xC8,0xAC,
	0xC8,0xAE,0xC8,0xB0,0xC8,0xB2,0xC8,0xBA,255 ,0xC8,0xBB,0xC8,0xBE,255 ,0xE2,
	0xB1,0xBE,0xE2,0xB1,0xBF,0xC9,0x81,0xC9,0x84,0xC9,0x85,0xC9,0x86,0xC9,0x88,
	0xC9,0x8A,0xC9,0x8C,0xC9,0x8E,0xE2,0xB1,0xAF,0xE2,0xB1,0xAD,0xE2,0xB1,0xB0,
	0xEA,0x9E,0xAB,0xEA,0x9E,0xAC,0xEA,0x9E,0x8D,0xEA,0x9E,0xAA,0xE2,0xB1,0xA2,
	0xEA,0x9E,0xAD,0xE2,0xB1,0xAE,0xE2,0xB1,0xA4,0xEA,0x9E,0xB1,0xEA,0x9E,0xB0,
	0xCE,0x99,0xCD,0xB0,0xCD,0xB2,0xCD,0xB6,0xCF,0xBD,0xCF,0xBE,0xCF,0xBF,0xCD,
	0xBF,0xCE,0x86,0xCE,0x88,0xCE,0x89,0xCE,0x8A,0xCE,0x8C,0xCE,0x8E,0xCE,0x8F,
	0xCE,0x99,0xCC,0x88,0xCC,0x81,0xCE,0x91,0xCE,0x92,0xCE,0x93,0xCE,0x94,0xCE,
	0x95,0xCE,0x96,0xCE,0x97,0xCE,0x98,0xCE,0x99,0xCE,0x9A,0xCE,0x9B,0xCE,0x9C,
	0xCE,0x9D,0xCE,0x9E,0xCE,0x9F,0xCE,0xA0,0xCE,0xA1,0xCE,0xA3,0xCE,0xA4,0xCE,
	0xA5,0xCE,0xA6,0xCE,0xA7,0xCE,0xA8,0xCE,0xA9,0xCE,0xAA,0xCE,0xAB,0xCE,0xA5,
	0xCC,0x88,0xCC,0x81,0xCE,0xA3,0xCF,0x8F,0xCE,0x92,0xCE,0x98,0xCE,0xA6,0xCE,
	0xA0,0xCF,0x98,0xCF,0x9A,0xCF,0x9C,0xCF,0x9E,0xCF,0xA0,0xCF,0xA2,0xCF,0xA4,
	0xCF,0xA6,0xCF,0xA8,0xCF,0xAA,0xCF,0xAC,0xCF,0xAE,0xCE,0x9A,0xCE,0xA1,0xCF,
	0xB9,0xCF,0xB4,0xCE,0x95,0xCF,0xB7,0xCF,0xBA,0xD0,0x80,0xD0,0x81,0xD0,0x82,
	0xD0,0x83,0xD0,0x84,0xD0,0x85,0xD0,0x86,0xD0,0x87,0xD0,0x88,0xD0,0x89,0xD0,
	0x8A,0xD0,0x8B,0xD0,0x8C,0xD0,0x8D,0xD0,0x8E,0xD0,0x8F,0xD0,0x90,0xD0,0x91,
	0xD0,0x92,0xD0,0x93,0xD0,0x94,0xD0,0x95,0xD0,0x96,0xD0,0x97,0xD0,0x98,0xD0,
	0x99,0xD0,0x9A,0xD0,0x9B,0xD0,0x9C,0xD0,0x9D,0xD0,0x9E,0xD0,0x9F,0xD0,0xA0,
	0xD0,0xA1,0xD0,0xA2,0xD0,0xA3,0xD0,0xA4,0xD0,0xA5,0xD0,0xA6,0xD0,0xA7,0xD0,
	0xA8,0xD0,0xA9,0xD0,0xAA,0xD0,0xAB,0xD0,0xAC,0xD0,0xAD,0xD0,0xAE,0xD0,0xAF,
	0xD1,0xA0,0xD1,0xA2,0xD1,0xA4,0xD1,0xA6,0xD1,0xA8,0xD1,0xAA,0xD1,0xAC,0xD1,
	0xAE,0xD1,0xB0,0xD1,0xB2,0xD1,0xB4,0xD1,0xB6,0xD1,0xB8,0xD1,0xBA,0xD1,0xBC,
	0xD1,0xBE,0xD2,0x80,0xD2,0x8A,0xD2,0x8C,0xD2,0x8E,0xD2,0x90,0xD2,0x92,0xD2,
	0x94,0xD2,0x96,0xD2,0x98,0xD2,0x9A,0xD2,0x9C,0xD2,0x9E,0xD2,0xA0,0xD2,0xA2,
	0xD2,0xA4,0xD2,0xA6,0xD2,0xA8,0xD2,0xAA,0xD2,0xAC,0xD2,0xAE,0xD2,0xB0,0xD2,
	0xB2,0xD2,0xB4,0xD2,0xB6,0xD2,0xB8,0xD2,0xBA,0xD2,0xBC,0xD2,0xBE,0xD3,0x80,
	0xD3,0x81,0xD3,0x83,0xD3,0x85,0xD3,0x87,0xD3,0x89,0xD3,0x8B,0xD3,0x8D,0xD3,
	0x90,0xD3,0x92,0xD3,0x94,0xD3,0x96,0xD3,0x98,0xD3,0x9A,0xD3,0x9C,0xD3,0x9E,
	0xD3,0xA0,0xD3,0xA2,0xD3,0xA4,0xD3,0xA6,0xD3,0xA8,0xD3,0xAA,0xD3,0xAC,0xD3,
	0xAE,0xD3,0xB0,0xD3,0xB2,0xD3,0xB4,0xD3,0xB6,0xD3,0xB8,0xD3,0xBA,0xD3,0xBC,
	0xD3,0xBE,0xD4,0x80,0xD4,0x82,0xD4,0x84,0xD4,0x86,0xD4,0x88,0xD4,0x8A,0xD4,
	0x8C,0xD4,0x8E,0xD4,0x90,0xD4,0x92,0xD4,0x94,0xD4,0x96,0xD4,0x98,0xD4,0x9A,
	0xD4,0x9C,0xD4,0x9E,0xD4,0xA0,0xD4,0xA2,0xD4,0xA4,0xD4,0xA6,0xD4,0xA8,0xD4,
	0xAA,0xD4,0xAC,0xD4,0xAE,0xD4,0xB1,0xD4,0xB2,0xD4,0xB3,0xD4,0xB4,0xD4,0xB5,
	0xD4,0xB6,0xD4,0xB7,0xD4,0xB8,0xD4,0xB9,0xD4,0xBA,0xD4,0xBB,0xD4,0xBC,0xD4,
	0xBD,0xD4,0xBE,0xD4,0xBF,0xD5,0x80,0xD5,0x81,0xD5,0x82,0xD5,0x83,0xD5,0x84,
	0xD5,0x85,0xD5,0x86,0xD5,0x87,0xD5,0x88,0xD5,0x89,0xD5,0x8A,0xD5,0x8B,0xD5,
	0x8C,0xD5,0x8D,0xD5,0x8E,0xD5,0x8F,0xD5,0x90,0xD5,0x91,0xD5,0x92,0xD5,0x93,
	0xD5,0x94,0xD5,0x95,0xD5,0x96,0xD4,0xB5,0xD5,0x92,0xE1,0x82,0xA0,0xE1,0x82,
	0xA1,0xE1,0x82,0xA2,0xE1,0x82,0xA3,0xE1,0x82,0xA4,0xE1,0x82,0xA5,0xE1,0x82,
	0xA6,0xE1,0x82,0xA7,0xE1,0x82,0xA8,0xE1,0x82,0xA9,0xE1,0x82,0xAA,0xE1,0x82,
	0xAB,0xE1,0x82,0xAC,0xE1,0x82,0xAD,0xE1,0x82,0xAE,0xE1,0x82,0xAF,0xE1,0x82,
	0xB0,0xE1,0x82,0xB1,0xE1,0x82,0xB2,0xE1,0x82,0xB3,0xE1,0x82,0xB4,0xE1,0x82,
	0xB5,0xE1,0x82,0xB6,0xE1,0x82,0xB7,0xE1,0x82,0xB8,0xE1,0x82,0xB9,0xE1,0x82,
	0xBA,0xE1,0x82,0xBB,0xE1,0x82,0xBC,0xE1,0x82,0xBD,0xE1,0x82,0xBE,0xE1,0x82,
	0xBF,0xE1,0x83,0x80,0xE1,0x83,0x81,0xE1,0x83,0x82,0xE1,0x83,0x83,0xE1,0x83,
	0x84,0xE1,0x83,0x85,0xE1,0x83,0x87,0xE1,0x83,0x8D,0xEA,0x9D,0xBD,0xE2,0xB1,
	0xA3,0xE1,0xB8,0x80,0xE1,0xB8,0x82,0xE1,0xB8,0x84,0xE1,0xB8,0x86,0xE1,0xB8,
	0x88,0xE1,0xB8,0x8A,0xE1,0xB8,0x8C,0xE1,0xB8,0x8E,0xE1,0xB8,0x90,0xE1,0xB8,
	0x92,0xE1,0xB8,0x94,0xE1,0xB8,0x96,0xE1,0xB8,0x98,0xE1,0xB8,0x9A,0xE1,0xB8,
	0x9C,0xE1,0xB8,0x9E,0xE1,0xB8,0xA0,0xE1,0xB8,0xA2,0xE1,0xB8,0xA4,0xE1,0xB8,
	0xA6,0xE1,0xB8,0xA8,0xE1,0xB8,0xAA,0xE1,0xB8,0xAC,0xE1,0xB8,0xAE,0xE1,0xB8,
	0xB0,0xE1,0xB8,0xB2,0xE1,0xB8,0xB4,0xE1,0xB8,0xB6,0xE1,0xB8,0xB8,0xE1,0xB8,
	0xBA,0xE1,0xB8,0xBC,0xE1,0xB8,0xBE,0xE1,0xB9,0x80,0xE1,0xB9,0x82,0xE1,0xB9,
	0x84,0xE1,0xB9,0x86,0xE1,0xB9,0x88,0xE1,0xB9,0x8A,0xE1,0xB9,0x8C,0xE1,0xB9,
	0x8E,0xE1,0xB9,0x90,0xE1,0xB9,0x92,0xE1,0xB9,0x94,0xE1,0xB9,0x96,0xE1,0xB9,
	0x98,0xE1,0xB9,0x9A,0xE1,0xB9,0x9C,0xE1,0xB9,0x9E,0xE1,0xB9,0xA0,0xE1,0xB9,
	0xA2,0xE1,0xB9,0xA4,0xE1,0xB9,0xA6,0xE1,0xB9,0xA8,0xE1,0xB9,0xAA,0xE1,0xB9,
	0xAC,0xE1,0xB9,0xAE,0xE1,0xB9,0xB0,0xE1,0xB9,0xB2,0xE1,0xB9,0xB4,0xE1,0xB9,
	0xB6,0xE1,0xB9,0xB8,0xE1,0xB9,0xBA,0xE1,0xB9,0xBC,0xE1,0xB9,0xBE,0xE1,0xBA,
	0x80,0xE1,0xBA,0x82,0xE1,0xBA,0x84,0xE1,0xBA,0x86,0xE1,0xBA,0x88,0xE1,0xBA,
	0x8A,0xE1,0xBA,0x8C,0xE1,0xBA,0x8E,0xE1,0xBA,0x90,0xE1,0xBA,0x92,0xE1,0xBA,
	0x94,0x48,0xCC,0xB1,0x54,0xCC,0x88,0x57,0xCC,0x8A,0x59,0xCC,0x8A,0x41,0xCA,
	0xBE,0xE1,0xB9,0xA0,0xE1,0xBA,0x9E,0xE1,0xBA,0xA0,0xE1,0xBA,0xA2,0xE1,0xBA,
	0xA4,0xE1,0xBA,0xA6,0xE1,0xBA,0xA8,0xE1,0xBA,0xAA,0xE1,0xBA,0xAC,0xE1,0xBA,
	0xAE,0xE1,0xBA,0xB0,0xE1,0xBA,0xB2,0xE1,0xBA,0xB4,0xE1,0xBA,0xB6,0xE1,0xBA,
	0xB8,0xE1,0xBA,0xBA,0xE1,0xBA,0xBC,0xE1,0xBA,0xBE,0xE1,0xBB,0x80,0xE1,0xBB,
	0x82,0xE1,0xBB,0x84,0xE1,0xBB,0x86,0xE1,0xBB,0x88,0xE1,0xBB,0x8A,0xE1,0xBB,
	0x8C,0xE1,0xBB,0x8E,0xE1,0xBB,0x90,0xE1,0xBB,0x92,0xE1,0xBB,0x94,0xE1,0xBB,
	0x96,0xE1,0xBB,0x98,0xE1,0xBB,0x9A,0xE1,0xBB,0x9C,0xE1,0xBB,0x9E,0xE1,0xBB,
	0xA0,0xE1,0xBB,0xA2,0xE1,0xBB,0xA4,0xE1,0xBB,0xA6,0xE1,0xBB,0xA8,0xE1,0xBB,
	0xAA,0xE1,0xBB,0xAC,0xE1,0xBB,0xAE,0xE1,0xBB,0xB0,0xE1,0xBB,0xB2,0xE1,0xBB,
	0xB4,0xE1,0xBB,0xB6,0xE1,0xBB,0xB8,0xE1,0xBB,0xBA,0xE1,0xBB,0xBC,0xE1,0xBB,
	0xBE,0xE1,0xBC,0x88,0xE1,0xBC,0x89,0xE1,0xBC,0x8A,0xE1,0xBC,0x8B,0xE1,0xBC,
	0x8C,0xE1,0xBC,0x8D,0xE1,0xBC,0x8E,0xE1,0xBC,0x8F,0xE1,0xBC,0x98,0xE1,0xBC,
	0x99,0xE1,0xBC,0x9A,0xE1,0xBC,0x9B,0xE1,0xBC,0x9C,0xE1,0xBC,0x9D,0xE1,0xBC,
	0xA8,0xE1,0xBC,0xA9,0xE1,0xBC,0xAA,0xE1,0xBC,0xAB,0xE1,0xBC,0xAC,0xE1,0xBC,
	0xAD,0xE1,0xBC,0xAE,0xE1,0xBC,0xAF,0xE1,0xBC,0xB8,0xE1,0xBC,0xB9,0xE1,0xBC,
	0xBA,0xE1,0xBC,0xBB,0xE1,0xBC,0xBC,0xE1,0xBC,0xBD,0xE1,0xBC,0xBE,0xE1,0xBC,
	0xBF,0xE1,0xBD,0x88,0xE1,0xBD,0x89,0xE1,0xBD,0x8A,0xE1,0xBD,0x8B,0xE1,0xBD,
	0x8C,0xE1,0xBD,0x8D,0xCE,0xA5,0xCC,0x93,0xE1,0xBD,0x99,0xCE,0xA5,0xCC,0x93,
	0xCC,0x80,0xE1,0xBD,0x9B,0xCE,0xA5,0xCC,0x93,0xCC,0x81,0xE1,0xBD,0x9D,0xCE,
	0xA5,0xCC,0x93,0xCD,0x82,0xE1,0xBD,0x9F,0xE1,0xBD,0xA8,0xE1,0xBD,0xA9,0xE1,
	0xBD,0xAA,0xE1,0xBD,0xAB,0xE1,0xBD,0xAC,0xE1,0xBD,0xAD,0xE1,0xBD,0xAE,0xE1,
	0xBD,0xAF,0xE1,0xBE,0xBA,0xE1,0xBE,0xBB,0xE1,0xBF,0x88,0xE1,0xBF,0x89,0xE1,
	0xBF,0x8A,0xE1,0xBF,0x8B,0xE1,0xBF,0x9A,0xE1,0xBF,0x9B,0xE1,0xBF,0xB8,0xE1,
	0xBF,0xB9,0xE1,0xBF,0xAA,0xE1,0xBF,0xAB,0xE1,0xBF,0xBA,0xE1,0xBF,0xBB,0xE1,
	0xBC,0x88,0xCE,0x99,0xE1,0xBC,0x89,0xCE,0x99,0xE1,0xBC,0x8A,0xCE,0x99,0xE1,
	0xBC,0x8B,0xCE,0x99,0xE1,0xBC,0x8C,0xCE,0x99,0xE1,0xBC,0x8D,0xCE,0x99,0xE1,
	0xBC,0x8E,0xCE,0x99,0xE1,0xBC,0x8F,0xCE,0x99,0xE1,0xBC,0xA8,0xCE,0x99,0xE1,
	0xBC,0xA9,0xCE,0x99,0xE1,0xBC,0xAA,0xCE,0x99,0xE1,0xBC,0xAB,0xCE,0x99,0xE1,
	0xBC,0xAC,0xCE,0x99,0xE1,0xBC,0xAD,0xCE,0x99,0xE1,0xBC,0xAE,0xCE,0x99,0xE1,
	0xBC,0xAF,0xCE,0x99,0xE1,0xBD,0xA8,0xCE,0x99,0xE1,0xBD,0xA9,0xCE,0x99,0xE1,
	0xBD,0xAA,0xCE,0x99,0xE1,0xBD,0xAB,0xCE,0x99,0xE1,0xBD,0xAC,0xCE,0x99,0xE1,
	0xBD,0xAD,0xCE,0x99,0xE1,0xBD,0xAE,0xCE,0x99,0xE1,0xBD,0xAF,0xCE,0x99,0xE1,
	0xBE,0xB8,0xE1,0xBE,0xB9,0xE1,0xBE,0xBA,0xCE,0x99,0xCE,0x91,0xCE,0x99,0xCE,
	0x86,0xCE,0x99,0xCE,0x91,0xCD,0x82,0xCE,0x91,0xCD,0x82,0xCE,0x99,0xCE,0x99,
	255 ,0xE1,0xBF,0x8A,0xCE,0x99,0xCE,0x97,0xCE,0x99,0xCE,0x89,0xCE,0x99,0xCE,
	0x97,0xCD,0x82,0xCE,0x97,0xCD,0x82,0xCE,0x99,0xE1,0xBF,0x98,0xE1,0xBF,0x99,
	0xCE,0x99,0xCC,0x88,0xCC,0x80,0xCE,0x99,0xCC,0x88,0xCC,0x81,0xCE,0x99,0xCD,
	0x82,0xCE,0x99,0xCC,0x88,0xCD,0x82,0xE1,0xBF,0xA8,0xE1,0xBF,0xA9,0xCE,0xA5,
	0xCC,0x88,0xCC,0x80,0xCE,0xA5,0xCC,0x88,0xCC,0x81,0xCE,0xA1,0xCC,0x93,0xE1,
	0xBF,0xAC,0xCE,0xA5,0xCD,0x82,0xCE,0xA5,0xCC,0x88,0xCD,0x82,0xE1,0xBF,0xBA,
	0xCE,0x99,0xCE,0xA9,0xCE,0x99,0xCE,0x8F,0xCE,0x99,0xCE,0xA9,0xCD,0x82,0xCE,
	0xA9,0xCD,0x82,0xCE,0x99,0xE2,0x84,0xA6,0xE2,0x84,0xAA,0xE2,0x84,0xAB,0xE2,
	0x84,0xB2,0xE2,0x85,0xA0,0xE2,0x85,0xA1,0xE2,0x85,0xA2,0xE2,0x85,0xA3,0xE2,
	0x85,0xA4,0xE2,0x85,0xA5,0xE2,0x85,0xA6,0xE2,0x85,0xA7,0xE2,0x85,0xA8,0xE2,
	0x85,0xA9,0xE2,0x85,0xAA,0xE2,0x85,0xAB,0xE2,0x85,0xAC,0xE2,0x85,0xAD,0xE2,
	0x85,0xAE,0xE2,0x85,0xAF,0xE2,0x86,0x83,0xE2,0x92,0xB6,0xE2,0x92,0xB7,0xE2,
	0x92,0xB8,0xE2,0x92,0xB9,0xE2,0x92,0xBA,0xE2,0x92,0xBB,0xE2,0x92,0xBC,0xE2,
	0x92,0xBD,0xE2,0x92,0xBE,0xE2,0x92,0xBF,0xE2,0x93,0x80,0xE2,0x93,0x81,0xE2,
	0x93,0x82,0xE2,0x93,0x83,0xE2,0x93,0x84,0xE2,0x93,0x85,0xE2,0x93,0x86,0xE2,
	0x93,0x87,0xE2,0x93,0x88,0xE2,0x93,0x89,0xE2,0x93,0x8A,0xE2,0x93,0x8B,0xE2,
	0x93,0x8C,0xE2,0x93,0x8D,0xE2,0x93,0x8E,0xE2,0x93,0x8F,0xE2,0xB0,0x80,0xE2,
	0xB0,0x81,0xE2,0xB0,0x82,0xE2,0xB0,0x83,0xE2,0xB0,0x84,0xE2,0xB0,0x85,0xE2,
	0xB0,0x86,0xE2,0xB0,0x87,0xE2,0xB0,0x88,0xE2,0xB0,0x89,0xE2,0xB0,0x8A,0xE2,
	0xB0,0x8B,0xE2,0xB0,0x8C,0xE2,0xB0,0x8D,0xE2,0xB0,0x8E,0xE2,0xB0,0x8F,0xE2,
	0xB0,0x90,0xE2,0xB0,0x91,0xE2,0xB0,0x92,0xE2,0xB0,0x93,0xE2,0xB0,0x94,0xE2,
	0xB0,0x95,0xE2,0xB0,0x96,0xE2,0xB0,0x97,0xE2,0xB0,0x98,0xE2,0xB0,0x99,0xE2,
	0xB0,0x9A,0xE2,0xB0,0x9B,0xE2,0xB0,0x9C,0xE2,0xB0,0x9D,0xE2,0xB0,0x9E,0xE2,
	0xB0,0x9F,0xE2,0xB0,0xA0,0xE2,0xB0,0xA1,0xE2,0xB0,0xA2,0xE2,0xB0,0xA3,0xE2,
	0xB0,0xA4,0xE2,0xB0,0xA5,0xE2,0xB0,0xA6,0xE2,0xB0,0xA7,0xE2,0xB0,0xA8,0xE2,
	0xB0,0xA9,0xE2,0xB0,0xAA,0xE2,0xB0,0xAB,0xE2,0xB0,0xAC,0xE2,0xB0,0xAD,0xE2,
	0xB0,0xAE,0xE2,0xB1,0xA0,0xE2,0xB1,0xA7,0xE2,0xB1,0xA9,0xE2,0xB1,0xAB,0xE2,
	0xB1,0xB2,0xE2,0xB1,0xB5,0xE2,0xB2,0x80,0xE2,0xB2,0x82,0xE2,0xB2,0x84,0xE2,
	0xB2,0x86,0xE2,0xB2,0x88,0xE2,0xB2,0x8A,0xE2,0xB2,0x8C,0xE2,0xB2,0x8E,0xE2,
	0xB2,0x90,0xE2,0xB2,0x92,0xE2,0xB2,0x94,0xE2,0xB2,0x96,0xE2,0xB2,0x98,0xE2,
	0xB2,0x9A,0xE2,0xB2,0x9C,0xE2,0xB2,0x9E,0xE2,0xB2,0xA0,0xE2,0xB2,0xA2,0xE2,
	0xB2,0xA4,0xE2,0xB2,0xA6,0xE2,0xB2,0xA8,0xE2,0xB2,0xAA,0xE2,0xB2,0xAC,0xE2,
	0xB2,0xAE,0xE2,0xB2,0xB0,0xE2,0xB2,0xB2,0xE2,0xB2,0xB4,0xE2,0xB2,0xB6,0xE2,
	0xB2,0xB8,0xE2,0xB2,0xBA,0xE2,0xB2,0xBC,0xE2,0xB2,0xBE,0xE2,0xB3,0x80,0xE2,
	0xB3,0x82,0xE2,0xB3,0x84,0xE2,0xB3,0x86,0xE2,0xB3,0x88,0xE2,0xB3,0x8A,0xE2,
	0xB3,0x8C,0xE2,0xB3,0x8E,0xE2,0xB3,0x90,0xE2,0xB3,0x92,0xE2,0xB3,0x94,0xE2,
	0xB3,0x96,0xE2,0xB3,0x98,0xE2,0xB3,0x9A,0xE2,0xB3,0x9C,0xE2,0xB3,0x9E,0xE2,
	0xB3,0xA0,0xE2,0xB3,0xA2,0xE2,0xB3,0xAB,0xE2,0xB3,0xAD,0xE2,0xB3,0xB2,0xEA,
	0x99,0x80,0xEA,0x99,0x82,0xEA,0x99,0x84,0xEA,0x99,0x86,0xEA,0x99,0x88,0xEA,
	0x99,0x8A,0xEA,0x99,0x8C,0xEA,0x99,0x8E,0xEA,0x99,0x90,0xEA,0x99,0x92,0xEA,
	0x99,0x94,0xEA,0x99,0x96,0xEA,0x99,0x98,0xEA,0x99,0x9A,0xEA,0x99,0x9C,0xEA,
	0x99,0x9E,0xEA,0x99,0xA0,0xEA,0x99,0xA2,0xEA,0x99,0xA4,0xEA,0x99,0xA6,0xEA,
	0x99,0xA8,0xEA,0x99,0xAA,0xEA,0x99,0xAC,0xEA,0x9A,0x80,0xEA,0x9A,0x82,0xEA,
	0x9A,0x84,0xEA,0x9A,0x86,0xEA,0x9A,0x88,0xEA,0x9A,0x8A,0xEA,0x9A,0x8C,0xEA,
	0x9A,0x8E,0xEA,0x9A,0x90,0xEA,0x9A,0x92,0xEA,0x9A,0x94,0xEA,0x9A,0x96,0xEA,
	0x9A,0x98,0xEA,0x9A,0x9A,0xEA,0x9C,0xA2,0xEA,0x9C,0xA4,0xEA,0x9C,0xA6,0xEA,
	0x9C,0xA8,0xEA,0x9C,0xAA,0xEA,0x9C,0xAC,0xEA,0x9C,0xAE,0xEA,0x9C,0xB2,0xEA,
	0x9C,0xB4,0xEA,0x9C,0xB6,0xEA,0x9C,0xB8,0xEA,0x9C,0xBA,0xEA,0x9C,0xBC,0xEA,
	0x9C,0xBE,0xEA,0x9D,0x80,0xEA,0x9D,0x82,0xEA,0x9D,0x84,0xEA,0x9D,0x86,0xEA,
	0x9D,0x88,0xEA,0x9D,0x8A,0xEA,0x9D,0x8C,0xEA,0x9D,0x8E,0xEA,0x9D,0x90,0xEA,
	0x9D,0x92,0xEA,0x9D,0x94,0xEA,0x9D,0x96,0xEA,0x9D,0x98,0xEA,0x9D,0x9A,0xEA,
	0x9D,0x9C,0xEA,0x9D,0x9E,0xEA,0x9D,0xA0,0xEA,0x9D,0xA2,0xEA,0x9D,0xA4,0xEA,
	0x9D,0xA6,0xEA,0x9D,0xA8,0xEA,0x9D,0xAA,0xEA,0x9D,0xAC,0xEA,0x9D,0xAE,0xEA,
	0x9D,0xB9,0xEA,0x9D,0xBB,0xEA,0x9D,0xBE,0xEA,0x9E,0x80,0xEA,0x9E,0x82,0xEA,
	0x9E,0x84,0xEA,0x9E,0x86,0xEA,0x9E,0x8B,0xEA,0x9E,0x90,0xEA,0x9E,0x92,0xEA,
	0x9E,0x96,0xEA,0x9E,0x98,0xEA,0x9E,0x9A,0xEA,0x9E,0x9C,0xEA,0x9E,0x9E,0xEA,
	0x9E,0xA0,0xEA,0x9E,0xA2,0xEA,0x9E,0xA4,0xEA,0x9E,0xA6,0xEA,0x9E,0xA8,0x46,
	0x46,255 ,0x46,0x49,255 ,0x46,0x4c,255 ,0x46,0x46,0x49,0x46,0x46,0x4c,0x53,
	0x54,255 ,0x53,0x54,255 ,0xD5,0x84,0xD5,0x86,0xD5,0x84,0xD4,0xB5,0xD5,0x84,
	0xD4,0xBB,0xD5,0x8E,0xD5,0x86,0xD5,0x84,0xD4,0xBD,0xEF,0xBC,0xA1,0xEF,0xBC,
	0xA2,0xEF,0xBC,0xA3,0xEF,0xBC,0xA4,0xEF,0xBC,0xA5,0xEF,0xBC,0xA6,0xEF,0xBC,
	0xA7,0xEF,0xBC,0xA8,0xEF,0xBC,0xA9,0xEF,0xBC,0xAA,0xEF,0xBC,0xAB,0xEF,0xBC,
	0xAC,0xEF,0xBC,0xAD,0xEF,0xBC,0xAE,0xEF,0xBC,0xAF,0xEF,0xBC,0xB0,0xEF,0xBC,
	0xB1,0xEF,0xBC,0xB2,0xEF,0xBC,0xB3,0xEF,0xBC,0xB4,0xEF,0xBC,0xB5,0xEF,0xBC,
	0xB6,0xEF,0xBC,0xB7,0xEF,0xBC,0xB8,0xEF,0xBC,0xB9,0xEF,0xBC,0xBA,
	0
};

// MARK: - Methods

struct Text make (const char *bytes, int32_t length)
{
	return (struct Text){
		.bytes = bytes,
		.length = length,
	};
}

struct Text join (struct Text from, struct Text to)
{
	return make(from.bytes, (int32_t)(to.bytes - from.bytes) + to.length);
}

struct Text(Char) character (struct Text text)
{
	struct Text(Char) c = { 0 };
	
	switch (text.length)
	{
		default:
		case 4:
			if ((text.bytes[0] & 0xf8) == 0xf0
				&& (text.bytes[1] & 0xc0) == 0x80
				&& (text.bytes[2] & 0xc0) == 0x80
				&& (text.bytes[3] & 0xc0) == 0x80)
			{
				c.codepoint  = (text.bytes[0] & 0x07) << 18;
				c.codepoint |= (text.bytes[1] & 0x3f) << 12;
				c.codepoint |= (text.bytes[2] & 0x3f) << 6;
				c.codepoint |= (text.bytes[3] & 0x3f);
				c.units = 4;
				break;
			}
			
		case 3:
			if ((text.bytes[0] & 0xf0) == 0xe0
				&& (text.bytes[1] & 0xc0) == 0x80
				&& (text.bytes[2] & 0xc0) == 0x80 )
			{
				c.codepoint  = (text.bytes[0] & 0x0f) << 12;
				c.codepoint |= (text.bytes[1] & 0x3f) << 6;
				c.codepoint |= (text.bytes[2] & 0x3f);
				c.units = 3;
				break;
			}
			
		case 2:
			if ((text.bytes[0] & 0xe0) == 0xc0
				&& (text.bytes[1] & 0xc0) == 0x80)
			{
				c.codepoint  = (text.bytes[0] & 0x1f) << 6;
				c.codepoint |= (text.bytes[1] & 0x3f);
				c.units = 2;
				break;
			}
			
		case 1:
			c.codepoint = *(uint8_t *)text.bytes;
			c.units = 1;
			break;
			
		case 0:
			break;
	}
	
	return c;
}

struct Text(Char) nextCharacter (struct Text *text)
{
	struct Text(Char) c = character(*text);
	advance(text, c.units);
	return c;
}

struct Text(Char) prevCharacter (struct Text *text)
{
	struct Text(Char) c = { 0 };
	
	switch (text->length)
	{
		default:
		case 4:
			if ((text->bytes[-4] & 0xf8) == 0xf0
				&& (text->bytes[-3] & 0xc0) == 0x80
				&& (text->bytes[-2] & 0xc0) == 0x80
				&& (text->bytes[-1] & 0xc0) == 0x80)
			{
				c.codepoint  = (*(--text->bytes) & 0x3f);
				c.codepoint |= (*(--text->bytes) & 0x3f) << 6;
				c.codepoint |= (*(--text->bytes) & 0x3f) << 12;
				c.codepoint |= (*(--text->bytes) & 0x07) << 18;
				c.units = 4;
				break;
			}
			
		case 3:
			if ((text->bytes[-3] & 0xf0) == 0xe0
				&& (text->bytes[-2] & 0xc0) == 0x80
				&& (text->bytes[-1] & 0xc0) == 0x80)
			{
				c.codepoint  = (*(--text->bytes) & 0x3f);
				c.codepoint |= (*(--text->bytes) & 0x3f) << 6;
				c.codepoint |= (*(--text->bytes) & 0x0f) << 12;
				c.units = 3;
				break;
			}
			
		case 2:
			if ((text->bytes[-2] & 0xe0) == 0xc0
				&& (text->bytes[-1] & 0xc0) == 0x80)
			{
				c.codepoint  = (*(--text->bytes) & 0x3f);
				c.codepoint |= (*(--text->bytes) & 0x1f) << 6;
				c.units = 2;
				break;
			}
			
		case 1:
			c.codepoint = *(uint8_t *)(--text->bytes);
			c.units = 1;
			break;
			
		case 0:
			break;
	}
	text->length -= c.units;
	return c;
}

void advance (struct Text *text, int32_t units)
{
	if (units >= text->length)
	{
		text->bytes += text->length;
		text->length = 0;
	}
	else
	{
		text->bytes += units;
		text->length -= units;
	}
}

uint16_t toUTF16Length (struct Text text)
{
	uint16_t windex = 0;
	
	while (text.length)
		windex += nextCharacter(&text).codepoint >= 0x010000? 2: 1;
	
	return windex;
}

uint16_t toUTF16 (struct Text text, uint16_t *wbuffer)
{
	uint16_t windex = 0;
	uint32_t cp;
	
	while (text.length)
	{
		cp = nextCharacter(&text).codepoint;
		
		if (cp >= 0x010000)
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

char * toLower (struct Text i, char *o /* length x 2 */)
{
	char buffer[5];
	const char *p;
	struct Text(Char) c;
	
	while (i.length)
	{
		c = character(i);
		memcpy(buffer, i.bytes, c.units);
		buffer[c.units] = '\0';
		advance(&i, c.units);
		p = buffer;
		
		if (c.units > 1 || isupper(*p))
		{
			p = strstr(uppers, p);
			if (p)
			{
				while (p[c.units] == (char)255) ++c.units;
				p = lowers + (p - uppers);
				while (p[c.units - 1] == (char)255) --c.units;
			}
			else
				p = buffer;
		}
		
		memcpy(o, p, c.units);
		o += c.units;
	}
	
	return o;
}

char * toUpper (struct Text i, char *o /* length x 3 */)
{
	char buffer[5];
	const char *p;
	struct Text(Char) c;
	
	while (i.length)
	{
		c = character(i);
		memcpy(buffer, i.bytes, c.units);
		buffer[c.units] = '\0';
		advance(&i, c.units);
		p = buffer;
		
		if (c.units > 1 || islower(*p))
		{
			p = strstr(lowers, p);
			if (p)
			{
				while (p[c.units] == (char)255) ++c.units;
				p = uppers + (p - lowers);
				while (p[c.units - 1] == (char)255) --c.units;
			}
			else
				p = buffer;
		}
		
		memcpy(o, p, c.units);
		o += c.units;
	}
	
	return o;
}

int isSpace (struct Text(Char) c)
{
	return
		isspace(c.codepoint)
		|| c.codepoint == 0xA0
		|| c.codepoint == 0xFEFF
		|| c.codepoint == 0x1680
		|| c.codepoint == 0x180E
		|| (c.codepoint >= 0x2000 && c.codepoint <= 0x200A)
		|| c.codepoint == 0x202F
		|| c.codepoint == 0x205F
		|| c.codepoint == 0x3000
		|| isLineFeed(c)
		;
}

int isDigit (struct Text(Char) c)
{
	return isdigit(c.codepoint) != 0;
}

int isWord (struct Text(Char) c)
{
	return isalnum(c.codepoint) || c.codepoint == '_';
}

int isLineFeed (struct Text(Char) c)
{
	return
		c.codepoint == '\n'
		|| c.codepoint == '\r'
		|| c.codepoint == 0x2028
		|| c.codepoint == 0x2029
		;
}

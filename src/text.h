//
//  text.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef io_libecc_text_h
#ifdef Implementation
#undef Implementation
#include __FILE__
#include "implementation.h"
#else
#include "interface.h"
#define io_libecc_text_h

	extern const struct Text Text(undefined);
	extern const struct Text Text(null);
	extern const struct Text Text(false);
	extern const struct Text Text(true);
	extern const struct Text Text(boolean);
	extern const struct Text Text(number);
	extern const struct Text Text(string);
	extern const struct Text Text(object);
	extern const struct Text Text(function);
	extern const struct Text Text(zero);
	extern const struct Text Text(one);
	extern const struct Text Text(nan);
	extern const struct Text Text(infinity);
	extern const struct Text Text(negativeInfinity);
	extern const struct Text Text(nativeCode);
	extern const struct Text Text(empty);
	extern const struct Text Text(emptyRegExp);

	extern const struct Text Text(nullType);
	extern const struct Text Text(undefinedType);
	extern const struct Text Text(objectType);
	extern const struct Text Text(errorType);
	extern const struct Text Text(arrayType);
	extern const struct Text Text(stringType);
	extern const struct Text Text(regexpType);
	extern const struct Text Text(numberType);
	extern const struct Text Text(booleanType);
	extern const struct Text Text(dateType);
	extern const struct Text Text(functionType);
	extern const struct Text Text(argumentsType);
	extern const struct Text Text(mathType);
	extern const struct Text Text(jsonType);
	extern const struct Text Text(globalType);

	extern const struct Text Text(errorName);
	extern const struct Text Text(rangeErrorName);
	extern const struct Text Text(referenceErrorName);
	extern const struct Text Text(syntaxErrorName);
	extern const struct Text Text(typeErrorName);
	extern const struct Text Text(uriErrorName);
	extern const struct Text Text(inputErrorName);
	extern const struct Text Text(evalErrorName);

	struct Text(Char) {
		uint32_t codepoint;
		uint8_t units;
	};

	enum Text(Flags) {
		Text(breakFlag) = 1 << 0,
	};

#endif


Interface(Text,
	
	(struct Text, make ,(const char *bytes, int32_t length))
	(struct Text, join ,(struct Text from, struct Text to))
	
	(struct Text(Char), character ,(struct Text))
	(struct Text(Char), nextCharacter ,(struct Text *text))
	(struct Text(Char), prevCharacter ,(struct Text *text))
	(void, advance ,(struct Text *text, int32_t units))
	
	(uint16_t, toUTF16Length ,(struct Text))
	(uint16_t, toUTF16 ,(struct Text, uint16_t *wbuffer))
	
	(char *, toLower ,(struct Text, char *x2buffer))
	(char *, toUpper ,(struct Text, char *x3buffer))
	
	(int, isSpace ,(struct Text(Char)))
	(int, isDigit ,(struct Text(Char)))
	(int, isWord ,(struct Text(Char)))
	(int, isLineFeed ,(struct Text(Char)))
	,
	{
		const char *bytes;
		int32_t length;
		uint8_t flags;
	}
)

#endif

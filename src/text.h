//
//  text.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef io_libecc_text_h
#define io_libecc_text_h

#include "namespace_io_libecc.h"

#include "interface.h"


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

extern const struct Text Text(nullType);
extern const struct Text Text(undefinedType);
extern const struct Text Text(objectType);
extern const struct Text Text(errorType);
extern const struct Text Text(arrayType);
extern const struct Text Text(stringType);
extern const struct Text Text(numberType);
extern const struct Text Text(dateType);
extern const struct Text Text(functionType);
extern const struct Text Text(argumentsType);

extern const struct Text Text(errorName);
extern const struct Text Text(rangeErrorName);
extern const struct Text Text(referenceErrorName);
extern const struct Text Text(syntaxErrorName);
extern const struct Text Text(typeErrorName);
extern const struct Text Text(uriErrorName);
extern const struct Text Text(inputErrorName);


Interface(Text,
	
	(struct Text, make ,(const char *location, uint16_t length))
	(struct Text, join ,(struct Text from, struct Text to))
	
	(uint16_t, nextCodepointBytes ,(struct Text))
	(uint32_t, nextCodepoint ,(struct Text *text))
	
	(uint16_t, toUTF16Bytes ,(struct Text))
	(uint16_t, toUTF16 ,(struct Text, uint16_t *wbuffer))
	,
	{
		const char *location;
		uint16_t length;
	}
)

#endif

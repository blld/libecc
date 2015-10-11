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

#define Module \
	io_libecc_Text

Interface(
	(Structure, make ,(const char *location, uint16_t length))
	(Structure, join ,(Structure from, Structure to))
	
	(uint16_t, toUTF16 ,(Structure, uint16_t *wbuffer))
	
	(const Instance, undefined ,(void))
	(const Instance, null ,(void))
	(const Instance, false ,(void))
	(const Instance, true ,(void))
	(const Instance, boolean ,(void))
	(const Instance, number ,(void))
	(const Instance, string ,(void))
	(const Instance, object ,(void))
	(const Instance, function ,(void))
	(const Instance, zero ,(void))
	(const Instance, one ,(void))
	(const Instance, NaN ,(void))
	(const Instance, Infinity ,(void))
	(const Instance, negativeInfinity ,(void))
	(const Instance, nativeCode ,(void))
	
	(const Instance, nullType ,(void))
	(const Instance, undefinedType ,(void))
	(const Instance, objectType ,(void))
	(const Instance, errorType ,(void))
	(const Instance, arrayType ,(void))
	(const Instance, stringType ,(void))
	(const Instance, dateType ,(void))
	(const Instance, functionType ,(void))
	(const Instance, argumentsType ,(void))
	
	(const Instance, errorName ,(void))
	(const Instance, rangeErrorName ,(void))
	(const Instance, referenceErrorName ,(void))
	(const Instance, syntaxErrorName ,(void))
	(const Instance, typeErrorName ,(void))
	(const Instance, uriErrorName ,(void))
	
	(const Instance, inputErrorName ,(void))
	,
	{
		uint16_t length;
		const char *location;
	}
)

#endif

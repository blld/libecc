//
//  chars.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef io_libecc_chars_h
#define io_libecc_chars_h

#include "namespace_io_libecc.h"

#include "pool.h"

#include "interface.h"


enum Chars(Flags)
{
	Chars(mark) = 1 << 0,
};


Interface(Chars,
	
	(struct Chars *, createVA ,(uint16_t length, const char *format, va_list ap))
	(struct Chars *, create ,(const char *format, ...))
	(struct Chars *, createSized ,(uint16_t size))
	(struct Chars *, createWithBuffer ,(uint16_t length, const char *buffer))
	
	(void, destroy ,(struct Chars *))
	,
	{
		uint16_t length;
		int16_t referenceCount;
		uint8_t flags;
		char chars[1];
	}
)

#endif

//
//  chars.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef io_libecc_chars_h
#ifdef Implementation
#undef Implementation
#include __FILE__
#include "implementation.h"
#else
#include "interface.h"
#define io_libecc_chars_h

	#include "builtin/string.h"

	enum Chars(Flags)
	{
		Chars(mark) = 1 << 0,
	};

	struct Chars(Append) {
		struct Chars *value;
		char buffer[9];
		uint8_t units;
	};
	
#endif


Interface(Chars,
	
	(struct Chars *, createVA ,(uint16_t length, const char *format, va_list ap))
	(struct Chars *, create ,(const char *format, ...))
	(struct Chars *, createSized ,(uint16_t length))
	(struct Chars *, createWithBytes ,(uint16_t length, const char *bytes))
	
	(void, beginAppend ,(struct Chars(Append) *))
	(void, append ,(struct Chars(Append) *, const char *format, ...))
	(void, appendCodepoint ,(struct Chars(Append) *, uint32_t cp))
	(void, appendValue ,(struct Chars(Append) *, struct Context * const context, struct Value value))
	(void, appendBinary ,(struct Chars(Append) *, double binary, int base))
	(void, normalizeBinary ,(struct Chars(Append) *))
	(struct Value, endAppend ,(struct Chars(Append) *))
	
	(void, destroy ,(struct Chars *))
	
	(uint8_t, codepointLength ,(uint32_t cp))
	(uint8_t, writeCodepoint ,(char *, uint32_t cp))
	,
	{
		uint16_t length;
		int16_t referenceCount;
		uint8_t flags;
		char bytes[1];
	}
)

#endif

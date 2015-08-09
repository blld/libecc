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


#define io_libecc_interface_noIdentity
#include "interface.h"

#define Module \
	io_libecc_Chars

Interface(
	(Instance, createVA ,(const char *format, va_list ap))
	(Instance, create ,(const char *format, ...))
	(Instance, createSized ,(uint16_t size))
//	(Instance, copy ,(Instance))
	
	(void, destroy ,(Instance))
	,
	{
		uint16_t length;
//		uint8_t flags;
		char chars[];
	}
)

#endif

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
	,
	{
		uint16_t length;
		const char *location;
	}
)

#endif

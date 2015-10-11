//
//  string.h
//  libecc
//
//  Created by Bouilland Aurélien on 04/07/2015.
//  Copyright (c) 2015 Libeccio. All rights reserved.
//

#ifndef io_libecc_string_h
#define io_libecc_string_h

#include "namespace_io_libecc.h"

#include "object.h"


#include "interface.h"

Interface(String,
	
	(struct String *, create ,(const char *format, ...))
	(void, destroy ,(struct String *))
	,
	{
		struct Object object;
		uint32_t length;
		char *chars;
	}
)

#endif

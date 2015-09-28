//
//  entry.h
//  libecc
//
//  Created by Bouilland Aurélien on 27/09/2015.
//  Copyright (c) 2015 Libeccio. All rights reserved.
//

#ifndef io_libecc_entry
#define io_libecc_entry

#include "namespace_io_libecc.h"


#include "interface.h"

#define Module \
	io_libecc_Entry

#define io_libecc_Entry(X) io_libecc_entry_ ## X

enum Module(Flags)
{
	Module(writable) = 1 << 2,
	Module(enumerable) = 1 << 3,
	Module(configurable) = 1 << 4,
	
	Module(isValue) = 0x80,
};

struct Module
{
	struct Value *value;
	uint8_t *flags;
};

#endif

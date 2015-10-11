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


#define io_libecc_Entry(X) io_libecc_entry_ ## X

enum Entry(Flags)
{
	Entry(writable) = 1 << 2,
	Entry(enumerable) = 1 << 3,
	Entry(configurable) = 1 << 4,
	
	Entry(isValue) = 0x80,
};

struct Entry
{
	struct Value *value;
	uint8_t *flags;
};

#endif

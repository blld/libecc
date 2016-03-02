//
//  array.h
//  libecc
//
//  Created by Bouilland Aurélien on 20/07/2015.
//  Copyright (c) 2015 Libeccio. All rights reserved.
//

#ifndef io_libecc_array_h
#define io_libecc_array_h

#include "namespace_io_libecc.h"

#include "value.h"

#include "interface.h"

extern struct Object * Array(prototype);
extern struct Function * Array(constructor);
extern const struct Object(Type) Array(type);


Interface(Array,
	
	(void, setup ,(void))
	(void, teardown ,(void))
	
	(struct Object *, create ,(void))
	(struct Object *, createSized ,(uint32_t size))
	
	(uint16_t, toLength ,(struct Native(Context) * const, struct Value))
	(uint16_t, toBytes ,(struct Native(Context) * const, struct Value, char *bytes))
	,
	{
		struct Object object;
	}
)

#endif

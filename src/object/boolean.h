//
//  boolean.h
//  libecc
//
//  Created by Bouilland Aurélien on 25/10/2015.
//  Copyright (c) 2015 Libeccio. All rights reserved.
//

#ifndef io_libecc_boolean_h
#define io_libecc_boolean_h

#include "namespace_io_libecc.h"

#include "object.h"

#include "interface.h"


extern struct Object * Boolean(prototype);
extern struct Function * Boolean(constructor);


Interface(Boolean,
	
	(void, setup ,(void))
	(void, teardown ,(void))
	
	(struct Boolean *, create ,(int))
	,
	{
		struct Object object;
		int truth;
	}
)

#endif

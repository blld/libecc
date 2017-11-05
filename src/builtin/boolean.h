//
//  boolean.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef io_libecc_boolean_h
#ifdef Implementation
#undef Implementation
#include __FILE__
#include "../implementation.h"
#else
#include "../interface.h"
#define io_libecc_boolean_h

	#include "global.h"

	extern struct Object * Boolean(prototype);
	extern struct Function * Boolean(constructor);
	extern const struct Object(Type) Boolean(type);

#endif


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

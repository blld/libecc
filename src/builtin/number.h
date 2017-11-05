//
//  number.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef io_libecc_number_h
#ifdef Implementation
#undef Implementation
#include __FILE__
#include "../implementation.h"
#else
#include "../interface.h"
#define io_libecc_number_h

	#include "global.h"

	extern struct Object * Number(prototype);
	extern struct Function * Number(constructor);
	extern const struct Object(Type) Number(type);

#endif


Interface(Number,
	
	(void, setup ,(void))
	(void, teardown ,(void))
	
	(struct Number *, create ,(double))
	,
	{
		struct Object object;
		double value;
	}
)

#endif

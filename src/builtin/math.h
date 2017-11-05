//
//  math.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef io_libecc_math_h
#ifdef Implementation
#undef Implementation
#include __FILE__
#include "../implementation.h"
#else
#include "../interface.h"
#define io_libecc_math_h

	#include "global.h"

	extern struct Object * Math(object);
	extern const struct Object(Type) Math(type);

#endif


Interface(Math,
	
	(void, setup ,(void))
	(void, teardown ,(void))
	,
	{
		char empty;
	}
)

#endif

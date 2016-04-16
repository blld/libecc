//
//  date.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef io_libecc_date_h
#ifdef Implementation
#undef Implementation
#include __FILE__
#include "implementation.h"
#else
#include "interface.h"
#define io_libecc_date_h

	#include "global.h"

	extern struct Object * Date(prototype);
	extern struct Function * Date(constructor);
	extern const struct Object(Type) Date(type);

#endif


Interface(Date,
	
	(void, setup ,(void))
	(void, teardown ,(void))
	
	(struct Date *, create ,(double))
	,
	{
		struct Object object;
		double ms;
	}
)

#endif

//
//  number.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef io_libecc_number_h
#define io_libecc_number_h

#include "namespace_io_libecc.h"

#include "object.h"

#include "interface.h"


extern struct Object * Number(prototype);
extern struct Function * Number(constructor);
extern const struct Object(Type) Number(type);


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

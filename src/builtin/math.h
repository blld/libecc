//
//  math.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef io_libecc_math_h
#define io_libecc_math_h

#include "namespace_io_libecc.h"

#include "object.h"

#include "interface.h"


extern struct Object * Math(object);
extern const struct Object(Type) Math(type);


Interface(Math,
	
	(void, setup ,(void))
	(void, teardown ,(void))
	,
	{
		char empty;
	}
)

#endif

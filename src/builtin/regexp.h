//
//  regexp.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef io_libecc_regexp_h
#define io_libecc_regexp_h

#include "namespace_io_libecc.h"

#include "object.h"

#include "interface.h"


extern struct Object * RegExp(prototype);
extern struct Function * RegExp(constructor);


Interface(RegExp,
	
	(void, setup ,(void))
	(void, teardown ,(void))
	,
	{
		struct Object object;
		double value;
	}
)

#endif

//
//  global.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef io_libecc__global_h
#define io_libecc__global_h

#include "namespace_io_libecc.h"

#include "object.h"
#include "function.h"
#include "boolean.h"
#include "date.h"
#include "arguments.h"
#include "math.h"
#include "number.h"
#include "regexp.h"

#include "interface.h"


extern const struct Object(Type) Global(type);


Interface(Global,
	
	(void, setup ,(void))
	(void, teardown ,(void))
	
	(struct Function *, create ,(void))
	,
	{
		char empty;
	}
)

#endif

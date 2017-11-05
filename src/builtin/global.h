//
//  global.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef io_libecc__global_h
#ifdef Implementation
#undef Implementation
#include __FILE__
#include "../implementation.h"
#else
#include "../interface.h"
#define io_libecc__global_h

	#include "object.h"
	#include "array.h"
	#include "function.h"
	#include "boolean.h"
	#include "date.h"
	#include "arguments.h"
	#include "math.h"
	#include "number.h"
	#include "regexp.h"
	#include "error.h"
	#include "json.h"

	extern const struct Object(Type) Global(type);

#endif


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

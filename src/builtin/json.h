//
//  json.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef json_h
#ifdef Implementation
#undef Implementation
#include __FILE__
#include "implementation.h"
#else
#include "interface.h"
#define json_h

	#include "global.h"

	extern struct Object * JSON(object);
	extern const struct Object(Type) JSON(type);

#endif


Interface(JSON,
	
	(void, setup ,(void))
	(void, teardown ,(void))
	,
	{
		char empty;
	}
)

#endif

//
//  math.h
//  libecc
//
//  Created by Bouilland Aurélien on 17/10/2015.
//  Copyright (c) 2015 Libeccio. All rights reserved.
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

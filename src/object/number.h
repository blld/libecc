//
//  number.h
//  libecc
//
//  Created by Bouilland Aurélien on 17/10/2015.
//  Copyright (c) 2015 Libeccio. All rights reserved.
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

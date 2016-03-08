//
//  date.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef io_libecc_date_h
#define io_libecc_date_h

#include "namespace_io_libecc.h"

#include "object.h"

#include "interface.h"


extern struct Object * Date(prototype);
extern struct Function * Date(constructor);
extern const struct Object(Type) Date(type);


Interface(Date,
	
	(void, setup ,(void))
	(void, teardown ,(void))
	
	(struct Object *, prototype ,(void))
//	(struct Function *, constructor ,(void))
	,
	{
		char empty;
	}
)

#endif

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


#define io_libecc_Number(X) io_libecc_number_ ## X

extern struct Object * Number(prototype);
extern struct Function * Number(constructor);


#include "interface.h"

Interface(Number,
	
	(void, setup ,(void))
	(void, teardown ,(void))
	,
	{
		struct Object object;
		double value;
	}
)

#endif

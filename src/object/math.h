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


#define io_libecc_Math(X) io_libecc_math_ ## X

extern struct Object * Math(object);


#include "interface.h"

Interface(Math,
	
	(void, setup ,(void))
	(void, teardown ,(void))
	
	,
	{
		char empty;
	}
)

#endif

//
//  global.h
//  libecc
//
//  Created by Bouilland Aurélien on 10/02/2016.
//  Copyright (c) 2016 Libeccio. All rights reserved.
//

#ifndef io_libecc__global_h
#define io_libecc__global_h

#include "namespace_io_libecc.h"

#include "object.h"
#include "function.h"
#include "boolean.h"
#include "date.h"
#include "arguments.h"

#include "interface.h"


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

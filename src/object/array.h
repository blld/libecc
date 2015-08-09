//
//  array.h
//  libecc
//
//  Created by Bouilland Aurélien on 20/07/2015.
//  Copyright (c) 2015 Libeccio. All rights reserved.
//

#ifndef io_libecc_array_h
#define io_libecc_array_h

#include "namespace_io_libecc.h"

#include "value.h"


#include "interface.h"

#define Module \
	io_libecc_Array

Interface(
	(void, setup ,(void))
	
	(struct Object *, prototype ,(void))
	(struct Object *, constructor ,(void))
	,
	{
		char empty;
	}
)

#endif

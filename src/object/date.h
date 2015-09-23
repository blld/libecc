//
//  date.h
//  libecc
//
//  Created by Bouilland Aurélien on 20/07/2015.
//  Copyright (c) 2015 Libeccio. All rights reserved.
//

#ifndef io_libecc_date_h
#define io_libecc_date_h

#include "namespace_io_libecc.h"

#include "object.h"


#include "interface.h"

#define Module \
	io_libecc_Date

Interface(
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

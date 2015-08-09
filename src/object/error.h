//
//  error.h
//  libecc
//
//  Created by Bouilland Aurélien on 27/06/2015.
//  Copyright (c) 2015 Libeccio. All rights reserved.
//

#ifndef io_libecc_error_h
#define io_libecc_error_h

#include "namespace_io_libecc.h"

#include "text.h"
#include "chars.h"
#include "object.h"


#include "interface.h"

#define Module \
	io_libecc_Error

Interface(
	(void, setup ,(void))
	
	(struct Object *, prototype ,(void))
	(struct Object *, constructor ,(void))
	
	(Instance, error ,(struct Text, const char *format, ...))
	(Instance, rangeError ,(struct Text, const char *format, ...))
	(Instance, referenceError ,(struct Text, const char *format, ...))
	(Instance, syntaxError ,(struct Text, const char *format, ...))
	(Instance, typeError ,(struct Text, const char *format, ...))
	(Instance, uriError ,(struct Text, const char *format, ...))
	(void, destroy ,(Instance))
	,
	{
		struct Object object;
		struct Text text;
	}
)

#endif

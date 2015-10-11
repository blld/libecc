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

Interface(Error,
	
	(void, setup ,(void))
	(void, teardown ,(void))
	
	(struct Object *, prototype ,(void))
	(struct Object *, rangePrototype ,(void))
	(struct Object *, referencePrototype ,(void))
	(struct Object *, syntaxPrototype ,(void))
	(struct Object *, typePrototype ,(void))
	(struct Object *, uriPrototype ,(void))
	
	(struct Error *, error ,(struct Text, const char *format, ...))
	(struct Error *, rangeError ,(struct Text, const char *format, ...))
	(struct Error *, referenceError ,(struct Text, const char *format, ...))
	(struct Error *, syntaxError ,(struct Text, const char *format, ...))
	(struct Error *, typeError ,(struct Text, const char *format, ...))
	(struct Error *, uriError ,(struct Text, const char *format, ...))
	
	(void, destroy ,(struct Error *))
	,
	{
		struct Object object;
		struct Text text;
	}
)

#endif

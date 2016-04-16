//
//  error.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef io_libecc_error_h
#ifdef Implementation
#undef Implementation
#include __FILE__
#include "implementation.h"
#else
#include "interface.h"
#define io_libecc_error_h

	#include "global.h"

	extern struct Object * Error(prototype);
	extern struct Object * Error(rangePrototype);
	extern struct Object * Error(referencePrototype);
	extern struct Object * Error(syntaxPrototype);
	extern struct Object * Error(typePrototype);
	extern struct Object * Error(uriPrototype);

	extern struct Function * Error(constructor);
	extern struct Function * Error(rangeConstructor);
	extern struct Function * Error(referenceConstructor);
	extern struct Function * Error(syntaxConstructor);
	extern struct Function * Error(typeConstructor);
	extern struct Function * Error(uriConstructor);

	extern const struct Object(Type) Error(type);

#endif


Interface(Error,
	
	(void, setup ,(void))
	(void, teardown ,(void))
	
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

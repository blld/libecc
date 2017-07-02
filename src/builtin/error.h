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
	extern struct Object * Error(evalPrototype);

	extern struct Function * Error(constructor);
	extern struct Function * Error(rangeConstructor);
	extern struct Function * Error(referenceConstructor);
	extern struct Function * Error(syntaxConstructor);
	extern struct Function * Error(typeConstructor);
	extern struct Function * Error(uriConstructor);
	extern struct Function * Error(evalConstructor);

	extern const struct Object(Type) Error(type);

#endif


Interface(Error,
	
	(void, setup ,(void))
	(void, teardown ,(void))
	
	(struct Error *, error ,(struct Text, struct Chars *message))
	(struct Error *, rangeError ,(struct Text, struct Chars *message))
	(struct Error *, referenceError ,(struct Text, struct Chars *message))
	(struct Error *, syntaxError ,(struct Text, struct Chars *message))
	(struct Error *, typeError ,(struct Text, struct Chars *message))
	(struct Error *, uriError ,(struct Text, struct Chars *message))
	(void, destroy ,(struct Error *))
	,
	{
		struct Object object;
		struct Text text;
	}
)

#endif

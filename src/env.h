//
//  env.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef io_libecc_env_h
#ifdef Implementation
#undef Implementation
#include __FILE__
#include "implementation.h"
#else
#include "interface.h"
#define io_libecc_env_h

	enum Env(Color) {
		Env(black) = 30,
		Env(red) = 31,
		Env(green) = 32,
		Env(yellow) = 33,
		Env(blue) = 34,
		Env(magenta) = 35,
		Env(cyan) = 36,
		Env(white) = 37,
	};

	enum Env(Attribute) {
		Env(bold) = 1,
		Env(dim) = 2,
		Env(invisible) = 8,
	};

#endif


Interface(Env,
	
	(void, setup ,(void))
	(void, teardown ,(void))
	
	(void, print ,(const char *format, ...))
	(void, printColor ,(enum Env(Color) color, enum Env(Attribute) attribute, const char *format, ...))
	(void, printError ,(int typeLength, const char *type, const char *format, ...))
	(void, printWarning ,(const char *format, ...))
	(void, newline ,())
	,
	{
		struct EnvInternal *internal;
	}
)

#endif

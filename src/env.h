//
//  env.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef io_libecc_env_h
#define io_libecc_env_h

#include "namespace_io_libecc.h"


#include "interface.h"

#define Module \
	io_libecc_Env

#define io_libecc_Env(X) io_libecc_env_ ## X

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

Interface(
	(void, setup ,(void))
	(void, teardown ,(void))
	
	(struct Env *, shared ,(void))
	
	(void, print ,(const char *format, ...))
	(void, printColor ,(enum Env(Color) color, enum Env(Attribute) attribute, const char *format, ...))
	(void, printError ,(int typeLength, const char *type, const char *format, ...))
	(void, printWarning ,(const char *format, ...))
	(void, newline ,())
	,
	{
		int isTerminal;
		struct EnvInternal *internal;
	}
)

#endif

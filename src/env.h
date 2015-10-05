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

enum Module(Color) {
	Module(Black) = 30,
	Module(Red) = 31,
	Module(Green) = 32,
	Module(Yellow) = 33,
	Module(Blue) = 34,
	Module(Magenta) = 35,
	Module(Cyan) = 36,
	Module(White) = 37,
};

enum Module(Attribute) {
	Module(Bold) = 1,
	Module(Dim) = 2,
	Module(Invisible) = 8,
};

Interface(
	(void, setup ,(void))
	(void, teardown ,(void))
	
	(Instance, shared ,(void))
	
	(void, print ,(const char *format, ...))
	(void, printColor ,(enum Module(Color) color, enum Module(Attribute) attribute, const char *format, ...))
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

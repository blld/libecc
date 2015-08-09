//
//  input.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef io_libecc_input_h
#define io_libecc_input_h

#include "namespace_io_libecc.h"

#include "text.h"
#include "env.h"


#include "interface.h"

#define Module \
	io_libecc_Input

Interface(
	(Instance, createFromFile ,(const char *filename))
	(Instance, createFromBytes ,(const char *bytes, uint32_t length, const char *name, ...))
	(void, destroy ,(Instance))
	
	(void, printTextInput, (Instance, struct Text text))
	,
	{
		char name[FILENAME_MAX];
		
		uint32_t length;
		char *bytes;
		
		uint16_t lineCount;
		uint16_t lineCapacity;
		uint32_t *lines;
	}
)

#endif

//
//  input.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef io_libecc_input_h
#ifdef Implementation
#undef Implementation
#include __FILE__
#include "implementation.h"
#else
#include "interface.h"
#define io_libecc_input_h

	#include "value.h"
	#include "env.h"

#endif


Interface(Input,
	
	(struct Input *, createFromFile ,(const char *filename))
	(struct Input *, createFromBytes ,(const char *bytes, uint32_t length, const char *name, ...))
	(void, destroy ,(struct Input *))
	
	(void, printText, (struct Input *, struct Text text, int32_t ofLine, struct Text ofText, const char *ofInput, int fullLine))
	(int32_t, findLine, (struct Input *, struct Text text))
	
	(struct Value, attachValue, (struct Input *, struct Value value))
	,
	{
		char name[FILENAME_MAX];
		
		uint32_t length;
		char *bytes;
		
		uint16_t lineCount;
		uint16_t lineCapacity;
		uint32_t *lines;
		
		struct Value *attached;
		uint16_t attachedCount;
	}
)

#endif

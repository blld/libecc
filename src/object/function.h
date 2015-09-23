//
//  function.h
//  libecc
//
//  Created by Bouilland Aurélien on 15/07/2015.
//  Copyright (c) 2015 Libeccio. All rights reserved.
//

#ifndef io_libecc_closure_h
#define io_libecc_closure_h

#include "namespace_io_libecc.h"

#ifndef io_libecc_op_h
#include "oplist.h"
#endif
#include "object.h"
#include "native.h"


#include "interface.h"

#define Module \
	io_libecc_Function

Interface(
	(Instance, create ,(struct Object *prototype))
	(Instance, createSized ,(struct Object *prototype, uint32_t size))
	(Instance, createWithNative ,(struct Object *prototype, const Native native, int parameterCount))
	(Instance, copy ,(Instance original))
	(void, destroy ,(Instance))
	
	(void, addValue ,(Instance self, const char *name, struct Value value, enum Object(Flags)))
	(Instance, addNative ,(Instance self, const char *name, const Native native, int argumentCount, enum Object(Flags)))
	(Instance, addToObject ,(struct Object *object, const char *name, const Native native, int parameterCount, enum Object(Flags)))
	,
	{
		struct Object object;
		struct Object context;
		struct OpList *oplist;
		struct Text text;
		int parameterCount;
		int needHeap;
		int needArguments;
	}
)

#endif

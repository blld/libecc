//
//  function.h
//  libecc
//
//  Created by Bouilland Aurélien on 15/07/2015.
//  Copyright (c) 2015 Libeccio. All rights reserved.
//

#ifndef io_libecc_function_h
#define io_libecc_function_h

#include "namespace_io_libecc.h"

#include "object.h"
#include "native.h"


#include "interface.h"

Interface(Function,
	
	(void, setup ,(void))
	(void, teardown ,(void))
	
	(struct Object *, prototype ,(void))
	(struct Function *, constructor ,(void))
	
	(struct Function *, create ,(struct Object *prototype))
	(struct Function *, createSized ,(struct Object *prototype, uint32_t size))
	(struct Function *, createWithNative ,(struct Object *prototype, const Native native, int parameterCount))
	(struct Function *, createWithNativeAccessor ,(struct Object *prototype, const Native getter, const Native setter))
	(struct Function *, copy ,(struct Function * original))
	(void, destroy ,(struct Function *))
	
	(void, addValue ,(struct Function *, const char *name, struct Value value, enum Object(Flags)))
	(struct Function *, addNative ,(struct Function *, const char *name, const Native native, int argumentCount, enum Object(Flags)))
	(struct Function *, addToObject ,(struct Object *object, const char *name, const Native native, int parameterCount, enum Object(Flags)))
	,
	{
		struct Object object;
		struct Object context;
		struct OpList *oplist;
		struct Text text;
		int parameterCount;
		int needHeap;
		int needArguments;
		int isAccessor;
		struct Function *pair;
	}
)

#endif

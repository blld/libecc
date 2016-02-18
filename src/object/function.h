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


enum Function(Flags) {
	Function(needHeap)      = 1 << 1,
	Function(needArguments) = 1 << 2,
	Function(isGetter)      = 1 << 3,
	Function(isSetter)      = 1 << 4,
	Function(isAccessor)    = Function(isGetter) | Function(isSetter),
};

extern struct Object * Function(prototype);
extern struct Function * Function(constructor);
extern const struct Object(Type) Function(type);


Interface(Function,
	
	(void, setup ,(void))
	(void, teardown ,(void))
	
	(struct Function *, create ,(struct Object *environment))
	(struct Function *, createSized ,(struct Object *environment, uint32_t size))
	(struct Function *, createWithNative ,(const Native(Function) native, int parameterCount))
	(struct Function *, createWithNativeAccessor ,(const Native(Function) getter, const Native(Function) setter))
	(struct Function *, copy ,(struct Function * original))
	(void, destroy ,(struct Function *))
	
	(void, addValue ,(struct Function *, const char *name, struct Value value, enum Value(Flags)))
	(struct Function *, addNative ,(struct Function *, const char *name, const Native(Function) native, int argumentCount, enum Value(Flags)))
	(struct Function *, addToObject ,(struct Object *object, const char *name, const Native(Function) native, int parameterCount, enum Value(Flags)))
	
	(void, linkPrototype ,(struct Function *, struct Value prototype))
	(void, setupBuiltinObject ,(struct Function **, const Native(Function), int parameterCount, struct Object **, struct Value prototype, const struct Object(Type) *type))
	
	(uint16_t, toLength ,(struct Function *))
	(uint16_t, toBytes ,(struct Function *, char *bytes))
	,
	{
		struct Object object;
		struct Object environment;
		struct OpList *oplist;
		struct Function *pair;
		struct Text text;
		const char *name;
		int parameterCount;
		enum Function(Flags) flags;
	}
)

#endif

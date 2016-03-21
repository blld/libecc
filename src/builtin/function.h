//
//  function.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
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
	Function(useBoundThis)  = 1 << 3,
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
	(struct Function *, copy ,(struct Function * original))
	(void, destroy ,(struct Function *))
	
	(void, addValue ,(struct Function *, const char *name, struct Value value, enum Value(Flags)))
	(struct Function *, addNative ,(struct Function *, const char *name, const Native(Function) native, int argumentCount, enum Value(Flags)))
	(struct Function *, addToObject ,(struct Object *object, const char *name, const Native(Function) native, int parameterCount, enum Value(Flags)))
	
	(void, linkPrototype ,(struct Function *, struct Value prototype))
	(void, setupBuiltinObject ,(struct Function **, const Native(Function), int parameterCount, struct Object **, struct Value prototype, const struct Object(Type) *type))
	
	(struct Value, accessor ,(const Native(Function) getter, const Native(Function) setter))
	,
	{
		struct Object object;
		struct Object environment;
		struct OpList *oplist;
		struct Function *pair;
		struct Value boundThis;
		struct Text text;
		const char *name;
		int parameterCount;
		enum Function(Flags) flags;
	}
)

#endif

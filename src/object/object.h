//
//  object.h
//  libecc
//
//  Created by Bouilland Aurélien on 04/07/2015.
//  Copyright (c) 2015 Libeccio. All rights reserved.
//

#ifndef io_libecc_object_h
#define io_libecc_object_h

#include "namespace_io_libecc.h"

#include "value.h"
#include "identifier.h"
#include "pool.h"


#include "interface.h"

#define Module \
	io_libecc_Object

#define io_libecc_Object(X) io_libecc_object_ ## X

enum Object(Flags)
{
	/* flags */
	Object(mark) = 1,
	Object(extensible),
	
	/* hashmap & element flags */
	Object(writable),
	Object(enumerable),
	Object(configurable),
	
	Object(isValue) = 0x80,
};

Interface(
	(void, setup ,(void))
	(void, teardown ,(void))
	
	(struct Object *, prototype ,(void))
	(struct Closure *, constructor ,(void))
	
	(Instance, create ,(Instance prototype))
	(Instance, createSized ,(Instance prototype, uint32_t size))
	(Instance, initialize ,(Instance, Instance prototype))
	(Instance, initializeSized ,(Instance, Instance prototype, uint32_t size))
	(Instance, finalize ,(Instance))
	(Instance, copy ,(const Instance original))
	(void, destroy ,(Instance))
	
	(void, setType ,(Instance, const struct Text *type))
	(void, mark ,(Instance))
	
//	(void, retain ,(Instance, int count))
//	(void, release ,(Instance, int count, int noDestroy))
	
	(struct Value, getOwn ,(Instance, struct Identifier))
	(struct Value, get ,(Instance, struct Identifier))
	(struct Value *, refOwn ,(Instance, struct Identifier, int create))
	(struct Value *, ref ,(Instance, struct Identifier, int create))
	(void, setOwn ,(Instance, struct Identifier, struct Value))
	(void, set ,(Instance, struct Identifier, struct Value))
	(struct Value *, add ,(Instance, struct Identifier, struct Value, enum Object(Flags)))
	(struct Value, delete ,(Instance, struct Identifier))
	(void, packValue ,(Instance))
	
	(void, resizeElement ,(Instance, uint32_t size))
	(void, addElementAtIndex ,(Instance, uint32_t index, struct Value))
	
	(void, dumpTo ,(Instance self, FILE *file))
	,
	{
		Instance prototype;
		
		const struct Text *type;
		
		struct {
			struct {
				struct Value value;
				char flags;
			} data;
		} *element;
		
		union {
			uint32_t slot[16];
			struct {
				struct Value value;
				struct Identifier identifier;
				char unused[43];
				char flags;
			} data;
		} *hashmap;
		
		uint32_t elementCount;
		uint32_t elementCapacity;
		uint32_t hashmapCount;
		uint32_t hashmapCapacity;
		
		uint8_t traceCount;
		uint8_t flags;
	}
)

#ifndef io_libecc_lexer_h
#include "ecc.h"
#include "string.h"
#include "array.h"

#include "interface.h"
#define Module io_libecc_Object
#endif

#endif

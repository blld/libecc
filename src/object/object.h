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
#include "entry.h"


#include "interface.h"

#define Module \
	io_libecc_Object

#define io_libecc_Object(X) io_libecc_object_ ## X

enum Object(Flags)
{
	/* object flags */
	Object(mark) = 1 << 0,
	Object(extensible) = 1 << 1,
	
	/* hashmap & element flags */
	Object(writable) = 1 << 2,
	Object(enumerable) = 1 << 3,
	Object(configurable) = 1 << 4,
	
	Object(isValue) = 0x80,
};

Interface(
	(void, setupPrototype ,(void))
	(void, setup ,(void))
	(void, teardown ,(void))
	
	(struct Object *, prototype ,(void))
	(struct Function *, constructor ,(void))
	
	(Instance, create ,(Instance prototype))
	(Instance, createSized ,(Instance prototype, uint32_t size))
	(Instance, initialize ,(Instance, Instance prototype))
	(Instance, initializeSized ,(Instance, Instance prototype, uint32_t size))
	(Instance, finalize ,(Instance))
	(Instance, copy ,(const Instance original))
	(void, destroy ,(Instance))
	
	(struct Value, get ,(Instance, struct Identifier))
	(struct Entry, getMember ,(Instance, struct Identifier))
	(struct Entry, getOwnProperty ,(Instance, struct Value))
	(struct Entry, getProperty ,(Instance, struct Value))
	(void, setProperty ,(Instance, struct Value, struct Value))
	(void, add ,(Instance, struct Identifier, struct Value, enum Object(Flags)))
	(struct Value, delete ,(Instance, struct Identifier))
	(struct Value, deleteProperty ,(Instance, struct Value))
	(void, packValue ,(Instance))
	
	(void, resizeElement ,(Instance, uint32_t size))
	(void, addElementAtIndex ,(Instance, uint32_t index, struct Value, enum Object(Flags)))
	
	(void, dumpTo ,(Instance self, FILE *file))
	,
	{
		Instance prototype;
		
		const struct Text *type;
		
		struct {
			struct {
				struct Value value;
				uint8_t flags;
			} data;
		} *element;
		
		union {
			uint32_t slot[16];
			struct {
				struct Value value;
				struct Identifier identifier;
				char unused[63 - sizeof(void *) * 2 - sizeof(struct Identifier)];
				uint8_t flags;
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

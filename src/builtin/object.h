//
//  object.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef io_libecc_object_h
#define io_libecc_object_h

#include "namespace_io_libecc.h"

#include "value.h"
#include "key.h"
#include "pool.h"

#include "interface.h"


struct Object(Type)
{
	const struct Text *text;
	
	void (*finalize)(struct Object *);
};

enum Object(Flags)
{
	Object(mark) = 1 << 0,
	Object(sealed) = 1 << 1,
};

extern struct Object * Object(prototype);
extern struct Function * Object(constructor);
extern const struct Object(Type) Object(type);


Interface(Object,
	
	(void, setup ,(void))
	(void, teardown ,(void))
	
	(struct Object *, create ,(struct Object * prototype))
	(struct Object *, createSized ,(struct Object * prototype, uint16_t size))
	(struct Object *, createTyped ,(const struct Object(Type) *type))
	(struct Object *, initialize ,(struct Object * restrict, struct Object * restrict prototype))
	(struct Object *, initializeSized ,(struct Object * restrict, struct Object * restrict prototype, uint16_t size))
	(struct Object *, finalize ,(struct Object *))
	(struct Object *, copy ,(const struct Object * original))
	(void, destroy ,(struct Object *))
	
	(struct Value *, member ,(struct Object *, struct Key))
	(struct Value *, element ,(struct Object *, uint32_t))
	(struct Value *, property ,(struct Object *, struct Value))
	
	(struct Value *, memberOwn ,(struct Object *, struct Key))
	(struct Value *, elementOwn ,(struct Object *, uint32_t))
	(struct Value *, propertyOwn ,(struct Object *, struct Value))
	
	(struct Value, getValue ,(struct Object *, struct Value *, struct Native(Context) * const))
	(struct Value, getMember ,(struct Object *, struct Key, struct Native(Context) * const))
	(struct Value, getElement ,(struct Object *, uint32_t, struct Native(Context) * const))
	(struct Value, getProperty ,(struct Object *, struct Value, struct Native(Context) * const))
	
	(struct Value *, putValue ,(struct Object *, struct Value *, struct Native(Context) * const, struct Value, const struct Text *))
	(struct Value *, putMember ,(struct Object *, struct Key, struct Native(Context) * const, struct Value, const struct Text *))
	(struct Value *, putElement ,(struct Object *, uint32_t, struct Native(Context) * const, struct Value, const struct Text *))
	(struct Value *, putProperty ,(struct Object *, struct Value, struct Native(Context) * const, struct Value, const struct Text *))
	
	(struct Value *, addMember ,(struct Object *, struct Key member, struct Value, enum Value(Flags)))
	(struct Value *, addElement ,(struct Object *, uint32_t element, struct Value, enum Value(Flags)))
	(struct Value *, addProperty ,(struct Object *, struct Value property, struct Value, enum Value(Flags)))
	
	(int, deleteMember ,(struct Object *, struct Key))
	(int, deleteElement ,(struct Object *, uint32_t))
	(int, deleteProperty ,(struct Object *, struct Value))
	
	(void, packValue ,(struct Object *))
	(void, stripMap ,(struct Object *))
	
	(void, resizeElement ,(struct Object *, uint32_t size))
	(void, populateElementWithCList ,(struct Object *, int count, const char * list[]))
	
	(void, dumpTo ,(struct Object *, FILE *file))
	,
	{
		struct Object *prototype;
		
		const struct Object(Type) *type;
		
		struct {
			struct {
				struct Value value;
			} data;
		} *element;
		
		union {
			struct {
				struct Value value;
				struct Key key;
			} data;
			uint16_t slot[16];
		} *hashmap;
		
		uint32_t elementCount;
		uint32_t elementCapacity;
		uint16_t hashmapCount;
		uint16_t hashmapCapacity;
		
		int16_t referenceCount;
		uint8_t flags;
	}
)

#ifndef io_libecc_lexer_h
#include "ecc.h"
#include "string.h"
#include "array.h"
#endif

#endif

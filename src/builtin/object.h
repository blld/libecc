//
//  object.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef io_libecc_object_h
#ifdef Implementation
#undef Implementation
#include __FILE__
#include "implementation.h"
#else
#include "interface.h"
#define io_libecc_object_h

	#include "../value.h"

	struct Object(Type)
	{
		const struct Text *text;
		
		void (*mark)(struct Object *);
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
	
	extern const uint32_t Object(ElementMax);

#endif


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
	(struct Value *, memberOwn ,(struct Object *, struct Key))
	(struct Value *, addMember ,(struct Object *, struct Key, struct Value, enum Value(Flags)))
	(struct Value *, putMember ,(struct Object *, struct Context * const, struct Key, struct Value))
	(struct Value, getMember ,(struct Object *, struct Context * const, struct Key))
	(int, deleteMember ,(struct Object *, struct Key))
	
	(struct Value *, element ,(struct Object *, uint32_t))
	(struct Value *, elementOwn ,(struct Object *, uint32_t))
	(struct Value *, addElement ,(struct Object *, uint32_t, struct Value, enum Value(Flags)))
	(struct Value *, putElement ,(struct Object *, struct Context * const, uint32_t, struct Value))
	(struct Value, getElement ,(struct Object *, struct Context * const, uint32_t))
	(int, deleteElement ,(struct Object *, uint32_t))
	
	(struct Value *, property ,(struct Object *, struct Context * const, struct Value))
	(struct Value *, propertyOwn ,(struct Object *, struct Context * const, struct Value))
	(struct Value *, addProperty ,(struct Object *, struct Context * const, struct Value property, struct Value, enum Value(Flags)))
	(struct Value *, putProperty ,(struct Object *, struct Context * const, struct Value, struct Value))
	(struct Value, getProperty ,(struct Object *, struct Context * const, struct Value))
	(int, deleteProperty ,(struct Object *, struct Context * const, struct Value))
	
	(struct Value *, putValue ,(struct Object *, struct Context * const, struct Value *, struct Value))
	(struct Value, getValue ,(struct Object *, struct Context * const, struct Value *))
	
	(void, packValue ,(struct Object *))
	(void, stripMap ,(struct Object *))
	
	(void, resizeElement ,(struct Object *, uint32_t size))
	(void, populateElementWithCList ,(struct Object *, uint32_t count, const char * list[]))
	
	(struct Value, toString ,(struct Context * const))
	(void, dumpTo ,(struct Object *, FILE *file))
	,
	{
		struct Object *prototype;
		
		const struct Object(Type) *type;
		
		union Object(Element) {
			struct Value value;
		} *element;
		
		union Object(Hashmap) {
			struct Value value;
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

#endif

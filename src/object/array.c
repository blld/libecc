//
//  array.c
//  libecc
//
//  Created by Bouilland Aurélien on 20/07/2015.
//  Copyright (c) 2015 Libeccio. All rights reserved.
//

#include "array.h"

// MARK: - Private

static struct Object *arrayPrototype = NULL;
static struct Function *arrayConstructor = NULL;

//static struct Value isArray (const struct Op ** const ops, struct Ecc * const ecc)
//{
//	Op.assertParameterCount(ecc, 0);
//	
//	Ecc.result(ecc, Value.boolean(ecc->this.type == Value(object) && ecc->this.data.object->type == &arrayType));
//	
//	return Value.undefined();
//}

static struct Value toString (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 0);
	
	if (ecc->this.data.object->elementCount)
	{
		struct Chars *chars = NULL;
		uint32_t length = 0, stringLength;
		int notLast = 0;
		struct Value value;
		uint32_t index;
		
		for (index = 0; ; ++index)
		{
			notLast = index < ecc->this.data.object->elementCount - 1;
			
			if (ecc->this.data.object->element[index].data.flags & Object(isValue))
			{
				value = ecc->this.data.object->element[index].data.value;
				
				value = Value.toString(value);
				
				stringLength = Value.stringLength(value);
				chars = realloc(chars, sizeof(*chars) + length + stringLength + (notLast? 1: 0));
				
				memcpy(chars->chars + length, Value.stringChars(value), stringLength);
				length += stringLength;
			}
		
			if (notLast)
				chars->chars[length++] = ',';
			else
				break;
		}
		
		chars->length = length;
		Pool.addChars(chars);
		ecc->result = Value.chars(chars);
	}
	
	return Value.undefined();
}

static struct Value getLength (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 0);
	
	ecc->result = Value.binary(ecc->this.data.object->elementCount);
	
	return Value.undefined();
}

static struct Value setLength (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 1);
	
	#warning TODO: check if object?
	Object.resizeElement(ecc->this.data.object, Value.toBinary(Op.argument(ecc, 0)).data.binary);
	
	return Value.undefined();
}

static struct Value constructorFunction (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 1);
	
	#warning TODO
	
	return Value.undefined();
}

// MARK: - Static Members

// MARK: - Methods

void setup (void)
{
	enum Object(Flags) flags = Object(writable) | Object(configurable);
	
	arrayPrototype = Object.create(Object.prototype());
	arrayPrototype->type = &Text(arrayType);
	
	Function.addToObject(arrayPrototype, "toString", toString, 0, flags);
//	Function.addToObject(arrayPrototype, "toLocaleString", toString, 0);
//	Function.addToObject(arrayPrototype, "valueOf", valueOf, 0);
//	Function.addToObject(arrayPrototype, "hasOwnProperty", hasOwnProperty, 0);
//	Function.addToObject(arrayPrototype, "isPrototypeOf", isPrototypeOf, 0);
//	Function.addToObject(arrayPrototype, "propertyIsEnumerable", propertyIsEnumerable, 0);
	Object.add(arrayPrototype, Identifier(length), Value.function(Function.createWithNativeAccessor(NULL, getLength, setLength)), Object(writable));
	
	arrayConstructor = Function.createWithNative(arrayPrototype, constructorFunction, 1);
	
	Object.add(arrayPrototype, Identifier(constructor), Value.function(arrayConstructor), 0);
}

void teardown (void)
{
//	Object.destroy(arrayPrototype), arrayPrototype = NULL;
}

struct Object *create (void)
{
	return createSized(0);
}

struct Object *createSized (uint32_t size)
{
	struct Object *self = Object.create(arrayPrototype);
	assert(self);
	Object.resizeElement(self, size);
	self->type = &Text(arrayType);
	
//	Object.add(self, Identifier.length(), )
	
	return self;
}

struct Object *prototype (void)
{
	return arrayPrototype;
}

struct Function *constructor (void)
{
	return arrayConstructor;
}

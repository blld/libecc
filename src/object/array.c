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
static struct Closure *arrayConstructor = NULL;

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
		
		for (uint_fast32_t index = 0; ; ++index)
		{
			notLast = index < ecc->this.data.object->elementCount - 1;
			
			value = ecc->this.data.object->element[index].data.value;
			
			value = Value.toString(value);
			
			stringLength = Value.stringLength(value);
			chars = realloc(chars, sizeof(*chars) + length + stringLength + (notLast? 2: 0));
			
			memcpy(chars->chars + length, Value.stringChars(value), stringLength);
			
			length += stringLength;
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
	arrayPrototype = Object.create(Object.prototype());
	arrayPrototype->type = Text.arrayType();
	
	enum Object(Flags) flags = Object(writable) | Object(configurable);
	
	Closure.addToObject(arrayPrototype, "toString", toString, 0, flags);
//	Closure.addToObject(arrayPrototype, "toLocaleString", toString, 0);
//	Closure.addToObject(arrayPrototype, "valueOf", valueOf, 0);
//	Closure.addToObject(arrayPrototype, "hasOwnProperty", hasOwnProperty, 0);
//	Closure.addToObject(arrayPrototype, "isPrototypeOf", isPrototypeOf, 0);
//	Closure.addToObject(arrayPrototype, "propertyIsEnumerable", propertyIsEnumerable, 0);
	
	arrayConstructor = Closure.createWithNative(arrayPrototype, constructorFunction, 1);
	
	Object.add(arrayPrototype, Identifier.constructor(), Value.closure(arrayConstructor), 0);
}

void teardown (void)
{
//	Object.destroy(arrayPrototype), arrayPrototype = NULL;
}

struct Object *prototype (void)
{
	return arrayPrototype;
}

struct Closure *constructor (void)
{
	return arrayConstructor;
}

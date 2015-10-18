//
//  number.c
//  libecc
//
//  Created by Bouilland Aurélien on 17/10/2015.
//  Copyright (c) 2015 Libeccio. All rights reserved.
//

#include "number.h"

// MARK: - Private

struct Object * Number(prototype) = NULL;
struct Function * Number(constructor) = NULL;

// MARK: - Static Members

static struct Value constructorFunction (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 1);
	
	ecc->result = Value.toBinary(Op.argument(ecc, 0));
	
	return Value.undefined();
}

// MARK: - Methods

void setup ()
{
	const enum Object(Flags) flags = Object(writable) | Object(configurable);
	
	Number(prototype) = Object.create(Object(prototype));
	
	Number(constructor) = Function.createWithNative(NULL, constructorFunction, 1);
//	Function.addToObject(&numberConstructor->object, "fromCharCode", fromCharCode, -1, flags);
	
	Object.add(Number(prototype), Key(constructor), Value.function(Number(constructor)), 0);
	Object.add(&Number(constructor)->object, Key(prototype), Value.object(Number(prototype)), 0);
}

void teardown (void)
{
	Number(prototype) = NULL;
	Number(constructor) = NULL;
}

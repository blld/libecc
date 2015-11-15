//
//  regexp.c
//  libecc
//
//  Created by Bouilland Aurélien on 17/10/2015.
//  Copyright (c) 2015 Libeccio. All rights reserved.
//

#include "regexp.h"

// MARK: - Private

struct Object * RegExp(prototype) = NULL;
struct Function * RegExp(constructor) = NULL;

// MARK: - Static Members

static struct Value constructorFunction (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 1);
	
#warning TODO
	ecc->result = Value(null);
	
	return Value(undefined);
}

// MARK: - Methods

void setup ()
{
	const enum Object(Flags) flags = Object(writable) | Object(configurable);
	
	RegExp(prototype) = Object.create(Object(prototype));
	
	RegExp(constructor) = Function.createWithNative(NULL, constructorFunction, 1);
//	Function.addToObject(&numberConstructor->object, "fromCharCode", fromCharCode, -1, flags);
	
	Object.add(RegExp(prototype), Key(constructor), Value.function(RegExp(constructor)), 0);
	Object.add(&RegExp(constructor)->object, Key(prototype), Value.object(RegExp(prototype)), 0);
}

void teardown (void)
{
	RegExp(prototype) = NULL;
	RegExp(constructor) = NULL;
}

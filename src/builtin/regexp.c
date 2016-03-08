//
//  regexp.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#include "regexp.h"

// MARK: - Private

struct Object * RegExp(prototype) = NULL;
struct Function * RegExp(constructor) = NULL;

// MARK: - Static Members

static struct Value constructorFunction (struct Native(Context) * const context)
{
	Native.assertParameterCount(context, 1);
	
#warning TODO
	return Value(null);
}

// MARK: - Methods

void setup ()
{
	const enum Value(Flags) flags = Value(hidden);
	
	RegExp(prototype) = Object.create(Object(prototype));
	
	RegExp(constructor) = Function.createWithNative(constructorFunction, 1);
//	Function.addToObject(&numberConstructor->object, "fromCharCode", fromCharCode, -1, flags);
	
	Object.add(RegExp(prototype), Key(constructor), Value.function(RegExp(constructor)), 0);
	Object.add(&RegExp(constructor)->object, Key(prototype), Value.object(RegExp(prototype)), 0);
}

void teardown (void)
{
	RegExp(prototype) = NULL;
	RegExp(constructor) = NULL;
}

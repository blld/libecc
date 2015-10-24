//
//  boolean.c
//  libecc
//
//  Created by Bouilland Aurélien on 25/10/2015.
//  Copyright (c) 2015 Libeccio. All rights reserved.
//

#include "boolean.h"

// MARK: - Private

struct Object * Boolean(prototype) = NULL;
struct Function * Boolean(constructor) = NULL;

// MARK: - Static Members

static struct Value constructorFunction (const struct Op ** const ops, struct Ecc * const ecc)
{
	char truth;
	
	Op.assertParameterCount(ecc, 1);
	
	truth = Value.isTrue(Op.argument(ecc, 0));
	if (ecc->construct)
		ecc->result = Value.boolean(Boolean.create(truth));
	else
		ecc->result = Value.truth(truth);
	
	return Value.undefined();
}

// MARK: - Methods

void setup ()
{
	Boolean(prototype) = Object.create(Object(prototype));
	
	Boolean(constructor) = Function.createWithNative(NULL, constructorFunction, 1);
	
	Object.add(Number(prototype), Key(constructor), Value.function(Number(constructor)), 0);
	Object.add(&Number(constructor)->object, Key(prototype), Value.object(Number(prototype)), 0);
}

void teardown (void)
{
	Boolean(prototype) = NULL;
	Boolean(constructor) = NULL;
}

struct Boolean * create (int truth)
{
	struct Boolean *self = malloc(sizeof(*self));
	*self = Boolean.identity;
	Pool.addObject(&self->object);
	Object.initialize(&self->object, NULL);
	
	self->object.type = &Text(booleanType);
	self->truth = truth;
	
	return self;
}

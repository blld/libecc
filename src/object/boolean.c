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

static struct Value toString (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 0);
	if (ecc->this.type != Value(booleanType))
		Ecc.jmpEnv(ecc, Value.error(Error.typeError((*ops)->text, "is not an boolean")));
	
	ecc->result = Value.text(ecc->this.data.boolean->truth? &Text(true): &Text(false));
	return Value(undefined);
}

static struct Value valueOf (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 0);
	if (ecc->this.type != Value(booleanType))
		Ecc.jmpEnv(ecc, Value.error(Error.typeError((*ops)->text, "is not an boolean")));
	
	ecc->result = Value.truth(ecc->this.data.boolean->truth);
	return Value(undefined);
}

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
	
	return Value(undefined);
}

// MARK: - Methods

void setup ()
{
	const enum Value(Flags) flags = Value(writable) | Value(configurable);
	
	Boolean(prototype) = Object.create(Object(prototype));
	Function.addToObject(Boolean(prototype), "toString", toString, 0, flags);
	Function.addToObject(Boolean(prototype), "valueOf", valueOf, 0, flags);
	
	Boolean(constructor) = Function.createWithNative(NULL, constructorFunction, 1);
	
	Object.add(Boolean(prototype), Key(constructor), Value.function(Boolean(constructor)), 0);
	Object.add(&Boolean(constructor)->object, Key(prototype), Value.object(Boolean(prototype)), 0);
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
	Object.initialize(&self->object, Boolean(prototype));
	
	self->object.type = &Text(booleanType);
	self->truth = truth;
	
	return self;
}

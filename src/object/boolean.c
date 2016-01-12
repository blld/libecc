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
	int truth;
	
	Op.assertParameterCount(ecc, 0);
	
	if (Value.isBoolean(ecc->this))
		truth = Value.isObject(ecc->this)? ecc->this.data.boolean->truth: Value.isTrue(ecc->this);
	else
		Ecc.jmpEnv(ecc, Value.error(Error.typeError(Op.textSeek(ops, ecc, Op(textSeekThis)), "not a boolean")));
	
	ecc->result = Value.text(truth? &Text(true): &Text(false));
	return Value(undefined);
}

static struct Value valueOf (const struct Op ** const ops, struct Ecc * const ecc)
{
	int truth;
	
	Op.assertParameterCount(ecc, 0);
	
	if (Value.isBoolean(ecc->this))
		truth = Value.isObject(ecc->this)? ecc->this.data.boolean->truth: Value.isTrue(ecc->this);
	else
		Ecc.jmpEnv(ecc, Value.error(Error.typeError(Op.textSeek(ops, ecc, Op(textSeekThis)), "not a boolean")));
	
	ecc->result = Value.truth(truth);
	return Value(undefined);
}

// MARK: - Static Members

static struct Value booleanConstructor (const struct Op ** const ops, struct Ecc * const ecc)
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
	enum Value(Flags) flags = Value(hidden);
	
	Function.setupBuiltinObject(&Boolean(constructor), booleanConstructor, 1, &Boolean(prototype), Value.boolean(create(0)), &Text(booleanType));
	
	Function.addToObject(Boolean(prototype), "toString", toString, 0, flags);
	Function.addToObject(Boolean(prototype), "valueOf", valueOf, 0, flags);
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
	
	self->truth = truth;
	
	return self;
}

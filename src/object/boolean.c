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

const struct Object(Type) Boolean(type) = {
	.text = &Text(booleanType),
};

static struct Value toString (struct Native(Context) * const context)
{
	int truth;
	
	Native.assertParameterCount(context, 0);
	
	if (Value.isBoolean(context->this))
		truth = Value.isObject(context->this)? context->this.data.boolean->truth: Value.isTrue(context->this);
	else
		Ecc.jmpEnv(context->ecc, Value.error(Error.typeError(Native.textSeek(context, Native(thisIndex)), "not a boolean")));
	
	return Value.text(truth? &Text(true): &Text(false));
}

static struct Value valueOf (struct Native(Context) * const context)
{
	int truth;
	
	Native.assertParameterCount(context, 0);
	
	if (Value.isBoolean(context->this))
		truth = Value.isObject(context->this)? context->this.data.boolean->truth: Value.isTrue(context->this);
	else
		Ecc.jmpEnv(context->ecc, Value.error(Error.typeError(Native.textSeek(context, Native(thisIndex)), "not a boolean")));
	
	return Value.truth(truth);
}

// MARK: - Static Members

static struct Value booleanConstructor (struct Native(Context) * const context)
{
	char truth;
	
	Native.assertParameterCount(context, 1);
	
	truth = Value.isTrue(Native.argument(context, 0));
	if (context->construct)
		return Value.boolean(Boolean.create(truth));
	else
		return Value.truth(truth);
}

// MARK: - Methods

void setup ()
{
	enum Value(Flags) flags = Value(hidden);
	
	Function.setupBuiltinObject(&Boolean(constructor), booleanConstructor, 1, &Boolean(prototype), Value.boolean(create(0)), &Boolean(type));
	
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

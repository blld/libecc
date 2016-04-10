//
//  boolean.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#include "boolean.h"

// MARK: - Private

struct Object * Boolean(prototype) = NULL;
struct Function * Boolean(constructor) = NULL;

const struct Object(Type) Boolean(type) = {
	.text = &Text(booleanType),
};

static struct Value toString (struct Context * const context)
{
	int truth;
	
	Context.assertParameterCount(context, 0);
	Context.assertThisMask(context, Value(booleanMask));
	
	truth = Value.isObject(context->this)? context->this.data.boolean->truth: Value.isTrue(context->this);
	
	return Value.text(truth? &Text(true): &Text(false));
}

static struct Value valueOf (struct Context * const context)
{
	int truth;
	
	Context.assertParameterCount(context, 0);
	Context.assertThisMask(context, Value(booleanMask));
	
	truth = Value.isObject(context->this)? context->this.data.boolean->truth: Value.isTrue(context->this);
	
	return Value.truth(truth);
}

// MARK: - Static Members

static struct Value booleanConstructor (struct Context * const context)
{
	char truth;
	
	Context.assertParameterCount(context, 1);
	
	truth = Value.isTrue(Context.argument(context, 0));
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

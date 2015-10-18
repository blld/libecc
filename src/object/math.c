//
//  math.c
//  libecc
//
//  Created by Bouilland Aurélien on 17/10/2015.
//  Copyright (c) 2015 Libeccio. All rights reserved.
//

#include "math.h"

// MARK: - Private

// MARK: - Static Members

static struct Value mathPow (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 2);
	
	ecc->result = Value.binary(pow(Value.toBinary(Op.argument(ecc, 0)).data.binary, Value.toBinary(Op.argument(ecc, 1)).data.binary));
	
	return Value.undefined();
}

static struct Value mathFloor (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 1);
	
	ecc->result = Value.binary(floor(Value.toBinary(Op.argument(ecc, 0)).data.binary));
	
	return Value.undefined();
}

// MARK: - Public

struct Object * Math(object) = NULL;

void setup ()
{
	const enum Object(Flags) flags = Object(writable) | Object(configurable);
	
	Math(object) = Object.create(Object(prototype));
	Function.addToObject(Math(object), "pow", mathPow, 2, flags);
	Function.addToObject(Math(object), "floor", mathFloor, 1, flags);
}

void teardown (void)
{
	Math(object) = NULL;
}

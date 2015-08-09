//
//  pool.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#include "pool.h"

// MARK: - Private

static Instance self = NULL;

// MARK: - Static Members

// MARK: - Methods

void setup (void)
{
	assert (!self);
	
	self = malloc(sizeof(*self));
	assert(self);
	*self = Module.identity;
}

void teardown (void)
{
	assert (self);
	
	struct Value value;
	
	while (self->valueCount--)
	{
		value = self->values[self->valueCount];
//		fprintf(stderr, "delete [%d] %p\n", self->valueCount, value.data.closure);
		
		if (value.type == Value(object))
			Object.destroy(value.data.object);
		else if (value.type == Value(closure))
			Closure.destroy(value.data.closure);
	}
	
	free(self->values), self->values = NULL;
	free(self), self = NULL;
}

Instance shared (void)
{
	return self;
}

void add (struct Value value)
{
	if (self->valueCount >= self->valueCapacity)
	{
		self->valueCapacity = self->valueCapacity? self->valueCapacity * 2: 1;
		self->values = realloc(self->values, self->valueCapacity * sizeof(*self->values));
		memset(self->values + self->valueCount, 0, sizeof(*self->values) * (self->valueCapacity - self->valueCount));
	}
//	fprintf(stderr, "add [%d] %p\n", self->valueCount, value.data.closure);
	self->values[self->valueCount++] = value;
}

void delete (struct Value value)
{
	for (uint_fast32_t index = 0; index < self->valueCount; ++index)
		if (self->values[index].data.object == value.data.object)
			self->values[index] = Value.undefined();
}

void collect (void)
{
	
}

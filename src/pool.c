//
//  pool.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#include "pool.h"

// MARK: - Private

static void unmarkValue (struct Value value);
static void unmarkObject (struct Object *object);

static struct Pool *self = NULL;

// MARK: - Static Members

static void unmarkFunction (struct Function *function)
{
	if (function->object.flags & Object(mark))
	{
		unmarkObject(&function->object);
		unmarkObject(&function->context);
		
		if (function->pair)
			unmarkFunction(function->pair);
	}
}

static void unmarkObject (struct Object *object)
{
	if (object->flags & Object(mark))
	{
		uint32_t index, count;
		
		object->flags &= ~Object(mark);
		
		if (object->prototype)
			unmarkObject(object->prototype);
		
		for (index = 0, count = object->elementCount; index < count; ++index)
			unmarkValue(object->element[index].data.value);
		
		for (index = 2, count = object->hashmapCount; index < count; ++index)
			if (object->hashmap[index].data.flags & Object(isValue))
			{
//				Key.dumpTo(object->hashmap[index].data.key, stderr);
//				fprintf(stderr, " < unmark\n");
				unmarkValue(object->hashmap[index].data.value);
			}
	}
}

static void unmarkChars (struct Chars *chars)
{
	if (chars->flags & Chars(mark))
		chars->flags &= ~Chars(mark);
}

// MARK: - Methods

void setup (void)
{
	assert (!self);
	
	self = malloc(sizeof(*self));
	assert(self);
	
	*self = Pool.identity;
}

void teardown (void)
{
	assert (self);
	
	markAll();
	collectMarked();
	
	free(self->functions), self->functions = NULL;
	free(self->objects), self->objects = NULL;
	free(self->chars), self->chars = NULL;
	
	free(self), self = NULL;
}

void addFunction (struct Function *function)
{
	assert(function);
	
	if (self->functionsCount >= self->functionsCapacity)
	{
		self->functionsCapacity = self->functionsCapacity? self->functionsCapacity * 2: 8;
		self->functions = realloc(self->functions, self->functionsCapacity * sizeof(*self->functions));
		memset(self->functions + self->functionsCount, 0, sizeof(*self->functions) * (self->functionsCapacity - self->functionsCount));
	}
	
	self->functions[self->functionsCount++] = function;
}

void addObject (struct Object *object)
{
	assert(object);
	
//	Object.dumpTo(object, stderr);
//	fprintf(stderr, " > add %p %u\n", object, self->objectsCount);
	
	if (self->objectsCount >= self->objectsCapacity)
	{
		self->objectsCapacity = self->objectsCapacity? self->objectsCapacity * 2: 8;
		self->objects = realloc(self->objects, self->objectsCapacity * sizeof(*self->objects));
		memset(self->objects + self->objectsCount, 0, sizeof(*self->objects) * (self->objectsCapacity - self->objectsCount));
	}
	
	self->objects[self->objectsCount++] = object;
}

void addChars (struct Chars *chars)
{
	assert(chars);
	
	if (self->charsCount >= self->charsCapacity)
	{
		self->charsCapacity = self->charsCapacity? self->charsCapacity * 2: 8;
		self->chars = realloc(self->chars, self->charsCapacity * sizeof(*self->chars));
		memset(self->chars + self->charsCount, 0, sizeof(*self->chars) * (self->charsCapacity - self->charsCount));
	}
	
	self->chars[self->charsCount++] = chars;
}

void markAll (void)
{
	uint32_t index, count;
	
	for (index = 0, count = self->functionsCount; index < count; ++index)
	{
		self->functions[index]->object.flags |= Object(mark);
		self->functions[index]->context.flags |= Object(mark);
	}
	
	for (index = 0, count = self->objectsCount; index < count; ++index)
		self->objects[index]->flags |= Object(mark);
	
	for (index = 0, count = self->charsCount; index < count; ++index)
		self->chars[index]->flags |= Chars(mark);
}

void unmarkValue (struct Value value)
{
	if (value.type == Value(function))
		unmarkFunction(value.data.function);
	else if (value.type >= Value(object))
		unmarkObject(value.data.object);
	else if (value.type == Value(chars))
		unmarkChars(value.data.chars);
}

void collectMarked (void)
{
	int total = 0;
	
	uint32_t index;
	
	index = self->functionsCount;
	while (index--)
		if (self->functions[index]->object.flags & Object(mark))
		{
			Function.destroy(self->functions[index]);
			self->functions[index] = self->functions[--self->functionsCount];
			++total;
		}
	
	index = self->objectsCount;
	while (index--)
		if (self->objects[index]->flags & Object(mark))
		{
			Object.destroy(self->objects[index]);
			self->objects[index] = self->objects[--self->objectsCount];
			++total;
		}
	
	index = self->charsCount;
	while (index--)
		if (self->chars[index]->flags & Chars(mark))
		{
			Chars.destroy(self->chars[index]);
			self->chars[index] = self->chars[--self->charsCount];
			++total;
		}
	
//	fprintf(stderr, "collected total: %d\n", total);
}

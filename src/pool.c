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
			if (object->element[index].data.flags & Object(isValue))
				unmarkValue(object->element[index].data.value);
		
		for (index = 2, count = object->hashmapCount; index < count; ++index)
			if (object->hashmap[index].data.flags & Object(isValue))
				unmarkValue(object->hashmap[index].data.value);
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
	
	free(self->functionList), self->functionList = NULL;
	free(self->objectList), self->objectList = NULL;
	free(self->charsList), self->charsList = NULL;
	
	free(self), self = NULL;
}

void addFunction (struct Function *function)
{
	assert(function);
	
	if (self->functionCount >= self->functionCapacity)
	{
		self->functionCapacity = self->functionCapacity? self->functionCapacity * 2: 8;
		self->functionList = realloc(self->functionList, self->functionCapacity * sizeof(*self->functionList));
		memset(self->functionList + self->functionCount, 0, sizeof(*self->functionList) * (self->functionCapacity - self->functionCount));
	}
	
	self->functionList[self->functionCount++] = function;
}

void addObject (struct Object *object)
{
	assert(object);
	
//	Object.dumpTo(object, stderr);
//	fprintf(stderr, " > add %p %u\n", object, self->objectsCount);
	
	if (self->objectCount >= self->objectCapacity)
	{
		self->objectCapacity = self->objectCapacity? self->objectCapacity * 2: 8;
		self->objectList = realloc(self->objectList, self->objectCapacity * sizeof(*self->objectList));
		memset(self->objectList + self->objectCount, 0, sizeof(*self->objectList) * (self->objectCapacity - self->objectCount));
	}
	
	self->objectList[self->objectCount++] = object;
}

void addChars (struct Chars *chars)
{
	assert(chars);
	
	if (self->charsCount >= self->charsCapacity)
	{
		self->charsCapacity = self->charsCapacity? self->charsCapacity * 2: 8;
		self->charsList = realloc(self->charsList, self->charsCapacity * sizeof(*self->charsList));
		memset(self->charsList + self->charsCount, 0, sizeof(*self->charsList) * (self->charsCapacity - self->charsCount));
	}
	
	self->charsList[self->charsCount++] = chars;
}

void markAll (void)
{
	uint32_t index, count;
	
	for (index = 0, count = self->functionCount; index < count; ++index)
	{
		self->functionList[index]->object.flags |= Object(mark);
		self->functionList[index]->context.flags |= Object(mark);
	}
	
	for (index = 0, count = self->objectCount; index < count; ++index)
		self->objectList[index]->flags |= Object(mark);
	
	for (index = 0, count = self->charsCount; index < count; ++index)
		self->charsList[index]->flags |= Chars(mark);
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
	uint32_t index;
	
	index = self->functionCount;
	while (index--)
		if (self->functionList[index]->object.flags & Object(mark))
		{
			Function.destroy(self->functionList[index]);
			self->functionList[index] = self->functionList[--self->functionCount];
		}
	
	index = self->objectCount;
	while (index--)
		if (self->objectList[index]->flags & Object(mark))
		{
			Object.destroy(self->objectList[index]);
			self->objectList[index] = self->objectList[--self->objectCount];
		}
	
	index = self->charsCount;
	while (index--)
		if (self->charsList[index]->flags & Chars(mark))
		{
			Chars.destroy(self->charsList[index]);
			self->charsList[index] = self->charsList[--self->charsCount];
		}
}

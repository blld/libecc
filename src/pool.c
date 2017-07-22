//
//  pool.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#define Implementation
#include "pool.h"

// MARK: - Private

static void markValue (struct Value value);
static void cleanupObject(struct Object *object);

static struct Pool *self = NULL;

// MARK: - Static Members

void markObject (struct Object *object)
{
	uint32_t index, count;
	
	if (object->flags & Object(mark))
		return;
	
	object->flags |= Object(mark);
	
	if (object->prototype)
		markObject(object->prototype);
	
	for (index = 0, count = object->elementCount; index < count; ++index)
		if (object->element[index].value.check == 1)
			markValue(object->element[index].value);
	
	for (index = 2, count = object->hashmapCount; index < count; ++index)
		if (object->hashmap[index].value.check == 1)
			markValue(object->hashmap[index].value);
	
	if (object->type->mark)
		object->type->mark(object);
}

static
void markChars (struct Chars *chars)
{
	if (chars->flags & Chars(mark))
		return;
	
	chars->flags |= Chars(mark);
}

// MARK: - Methods

void setup (void)
{
	assert (!self);
	
	self = malloc(sizeof(*self));
	*self = Pool.identity;
}

void teardown (void)
{
	assert (self);
	
	unmarkAll();
	collectUnmarked();
	
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
	
//	fprintf(stderr, " > add %p %u\n", object, self->objectCount);
//	Object.dumpTo(object, stderr);
	
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

void unmarkAll (void)
{
	uint32_t index, count;
	
	for (index = 0, count = self->functionCount; index < count; ++index)
	{
		self->functionList[index]->object.flags &= ~Object(mark);
		self->functionList[index]->environment.flags &= ~Object(mark);
	}
	
	for (index = 0, count = self->objectCount; index < count; ++index)
		self->objectList[index]->flags &= ~Object(mark);
	
	for (index = 0, count = self->charsCount; index < count; ++index)
		self->charsList[index]->flags &= ~Chars(mark);
}

void markValue (struct Value value)
{
	if (value.type >= Value(objectType))
		markObject(value.data.object);
	else if (value.type == Value(charsType))
		markChars(value.data.chars);
}

static
void releaseObject(struct Object *object)
{
	if (object->referenceCount > 0 && !--object->referenceCount)
		cleanupObject(object);
}

static
struct Value releaseValue(struct Value value)
{
	if (value.type == Value(charsType))
		--value.data.chars->referenceCount;
	if (value.type >= Value(objectType))
		releaseObject(value.data.object);
	
	return value;
}

static void captureObject (struct Object *object);

static
struct Value retainValue(struct Value value)
{
	if (value.type == Value(charsType))
		++value.data.chars->referenceCount;
	if (value.type >= Value(objectType))
	{
		++value.data.object->referenceCount;
		if (!(value.data.object->flags & Object(mark)))
		{
			value.data.object->flags |= Object(mark);
			captureObject(value.data.object);
		}
	}
	return value;
}

static
void cleanupObject(struct Object *object)
{
	struct Value value;
	
	if (object->prototype && object->prototype->referenceCount)
		--object->prototype->referenceCount;
	
	if (object->elementCount)
		while (object->elementCount--)
			if ((value = object->element[object->elementCount].value).check == 1)
				releaseValue(value);
	
	if (object->hashmapCount)
		while (object->hashmapCount--)
			if ((value = object->hashmap[object->hashmapCount].value).check == 1)
				releaseValue(value);
}

static
void captureObject (struct Object *object)
{
	uint32_t index, count;
	union Object(Element) *element;
	union Object(Hashmap) *hashmap;
	
	if (object->prototype)
	{
		++object->prototype->referenceCount;
		if (!(object->prototype->flags & Object(mark)))
		{
			object->prototype->flags |= Object(mark);
			captureObject(object->prototype);
		}
	}
	
	count = object->elementCount < object->elementCapacity? object->elementCount : object->elementCapacity;
	for (index = 0; index < count; ++index)
	{
		element = object->element + index;
		if (element->value.check == 1)
			retainValue(element->value);
	}
	
	count = object->hashmapCount;
	for (index = 2; index < count; ++index)
	{
		hashmap = object->hashmap + index;
		if (hashmap->value.check == 1)
			retainValue(hashmap->value);
	}
	
	if (object->type->capture)
		object->type->capture(object);
}

void collectUnmarked (void)
{
	uint32_t index;
	
	// finalize & destroy
	
	index = self->functionCount;
	while (index--)
		if (!(self->functionList[index]->object.flags & Object(mark)) && !(self->functionList[index]->environment.flags & Object(mark)))
		{
			Function.destroy(self->functionList[index]);
			self->functionList[index] = self->functionList[--self->functionCount];
		}
	
	index = self->objectCount;
	while (index--)
		if (!(self->objectList[index]->flags & Object(mark)))
		{
			Object.finalize(self->objectList[index]);
			Object.destroy(self->objectList[index]);
			self->objectList[index] = self->objectList[--self->objectCount];
		}
	
	index = self->charsCount;
	while (index--)
		if (!(self->charsList[index]->flags & Chars(mark)))
		{
			Chars.destroy(self->charsList[index]);
			self->charsList[index] = self->charsList[--self->charsCount];
		}
}

void collectUnreferencedFromIndices (uint32_t indices[3])
{
	uint32_t index;
	
	// prepare
	
	index = self->objectCount;
	while (index-- > indices[1])
		if (self->objectList[index]->referenceCount <= 0)
			cleanupObject(self->objectList[index]);
	
	index = self->objectCount;
	while (index-- > indices[1])
		if (self->objectList[index]->referenceCount > 0 && !(self->objectList[index]->flags & Object(mark)))
		{
			self->objectList[index]->flags |= Object(mark);
			captureObject(self->objectList[index]);
		}
	
	// destroy
	
	index = self->functionCount;
	while (index-- > indices[0])
		if (!self->functionList[index]->object.referenceCount && !self->functionList[index]->environment.referenceCount)
		{
			Function.destroy(self->functionList[index]);
			self->functionList[index] = self->functionList[--self->functionCount];
		}
	
	index = self->objectCount;
	while (index-- > indices[1])
		if (self->objectList[index]->referenceCount <= 0)
		{
			Object.finalize(self->objectList[index]);
			Object.destroy(self->objectList[index]);
			self->objectList[index] = self->objectList[--self->objectCount];
		}
	
	index = self->charsCount;
	while (index-- > indices[2])
		if (self->charsList[index]->referenceCount <= 0)
		{
			Chars.destroy(self->charsList[index]);
			self->charsList[index] = self->charsList[--self->charsCount];
		}
}

void unreferenceFromIndices (uint32_t indices[3])
{
	uint32_t index;
	
	index = self->functionCount;
	while (index-- > indices[0])
	{
		--self->functionList[index]->object.referenceCount;
		--self->functionList[index]->environment.referenceCount;
	}
	
	index = self->objectCount;
	while (index-- > indices[1])
		--self->objectList[index]->referenceCount;
	
	index = self->charsCount;
	while (index-- > indices[2])
		--self->charsList[index]->referenceCount;
}

void getIndices (uint32_t indices[3])
{
	indices[0] = self->functionCount;
	indices[1] = self->objectCount;
	indices[2] = self->charsCount;
}

//
//  context.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#include "context.h"

// MARK: - Private

// MARK: - Static Members

// MARK: - Methods

void assertParameterCount (struct Context * const self, int parameterCount)
{
	assert(self->environment->hashmapCount == parameterCount + 3);
}

int argumentCount (struct Context * const self)
{
	return self->environment->hashmapCount - 3;
}

struct Value argument (struct Context * const self, int argumentIndex)
{
	self->textIndex = argumentIndex + 4;
	return self->environment->hashmap[argumentIndex + 3].data.value;
}

void replaceArgument (struct Context * const self, int argumentIndex, struct Value value)
{
	self->environment->hashmap[argumentIndex + 3].data.value = value;
}

void assertVariableParameter (struct Context * const self)
{
	assert(self->environment->hashmap[2].data.value.type == Value(objectType));
}

int variableArgumentCount (struct Context * const self)
{
	return self->environment->hashmap[2].data.value.data.object->elementCount;
}

struct Value variableArgument (struct Context * const self, int argumentIndex)
{
	self->textIndex = argumentIndex + 4;
	return self->environment->hashmap[2].data.value.data.object->element[argumentIndex].data.value;
}

struct Value this (struct Context * const self)
{
	self->textIndex = Context(thisIndex);
	return self->this;
}

void assertThisType (struct Context * const self, enum Value(Type) type)
{
	if (self->this.type != type)
	{
		setTextIndex(self, Context(thisIndex));
		Ecc.jmpEnv(self->ecc, Value.error(Error.typeError(textSeek(self), "not a %s", Value.typeName(type))));
	}
}

void assertThisMask (struct Context * const self, enum Value(Mask) mask)
{
	if (!(self->this.type & mask))
	{
		setTextIndex(self, Context(thisIndex));
		Ecc.jmpEnv(self->ecc, Value.error(Error.typeError(textSeek(self), "not a %s", Value.maskName(mask))));
	}
}

void setText (struct Context * const self, const struct Text *text)
{
	self->textIndex = Context(savedIndex);
	self->text = text;
}

void setTextIndex (struct Context * const self, enum Context(Index) index)
{
	self->textIndex = index;
}

struct Text textSeek (struct Context * const self)
{
	const char *bytes;
	struct Context seek = *self;
	uint32_t breakArray = 0, argumentCount = 0;
	struct Text callText;
	enum Context(Index) index;
	
	assert(self);
	assert(self->ecc);
	
	index = self->textIndex;
	
	if (index == Context(savedIndex))
		return *self->text;
	
	while (seek.ops->text.bytes == Text(nativeCode).bytes)
	{
		if (!seek.parent)
			return seek.ops->text;
		
		if (seek.argumentOffset && index >= Context(thisIndex))
		{
			++index;
			++argumentCount;
			breakArray <<= 1;
			
			if (seek.argumentOffset == 2)
				breakArray |= 2;
		}
		seek = *seek.parent;
	}
	
	if (seek.ops->native == Op.noop)
		--seek.ops;
	
	if (index > Context(noIndex))
	{
		while (seek.ops->native != Op.call
			&& seek.ops->native != Op.callMember
			&& seek.ops->native != Op.callProperty
			&& seek.ops->native != Op.eval
			&& seek.ops->native != Op.construct)
			--seek.ops;
		
		argumentCount += seek.ops->value.data.integer;
		callText = seek.ops->text;
		
		// func
		if (index-- > Context(callIndex))
			++seek.ops;
		
		// this
		if (index-- > Context(callIndex) && (seek.ops + 1)->text.bytes <= seek.ops->text.bytes)
			++seek.ops;
		
		// arguments
		while (index-- > Context(callIndex))
		{
			if (!argumentCount--)
				return Text.make(callText.bytes + callText.length - 1, 1);
			
			bytes = seek.ops->text.bytes + seek.ops->text.length;
			while (bytes > seek.ops->text.bytes && seek.ops->text.bytes)
				++seek.ops;
			
			if (breakArray & 0x1 && seek.ops->native == Op.array)
				++seek.ops;
			
			breakArray >>= 1;
		}
	}
	
	return seek.ops->text;
}

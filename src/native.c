//
//  native.c
//  libecc
//
//  Created by Bouilland Aurélien on 02/02/2016.
//  Copyright (c) 2016 Libeccio. All rights reserved.
//

#include "native.h"

// MARK: - Private

// MARK: - Static Members

// MARK: - Methods

void assertParameterCount (struct Native(Context) * const context, int parameterCount)
{
	assert(context->environment->hashmapCount == parameterCount + 3);
}

int argumentCount (struct Native(Context) * const context)
{
	return context->environment->hashmapCount - 3;
}

struct Value argument (struct Native(Context) * const context, int argumentIndex)
{
	return context->environment->hashmap[argumentIndex + 3].data.value;
}

void replaceArgument (struct Native(Context) * const context, int argumentIndex, struct Value value)
{
	context->environment->hashmap[argumentIndex + 3].data.value = value;
}

void assertVariableParameter (struct Native(Context) * const context)
{
	assert(context->environment->hashmap[2].data.value.type == Value(objectType));
}

int variableArgumentCount (struct Native(Context) * const context)
{
	return context->environment->hashmap[2].data.value.data.object->elementCount;
}

struct Value variableArgument (struct Native(Context) * const context, int argumentIndex)
{
	return context->environment->hashmap[2].data.value.data.object->element[argumentIndex].data.value;
}

struct Text textSeek (struct Native(Context) * const context, enum Native(Index) argumentIndex)
{
	const char *bytes;
	struct Native(Context) seek = *context;
	uint32_t breakArray = 0, argumentCount = 0;
	struct Text callText;
	
	assert(context);
	assert(context->ecc);
	
	while (seek.ops->text.bytes == Text(nativeCode).bytes)
	{
		if (!seek.parent)
			return seek.ops->text;
		
		if (seek.argumentOffset && argumentIndex >= Native(thisIndex))
		{
			++argumentIndex;
			++argumentCount;
			breakArray <<= 1;
			
			if (seek.argumentOffset == 2)
				breakArray |= 2;
		}
		seek = *seek.parent;
	}
	
	if (argumentIndex > Native(noIndex))
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
		if (argumentIndex-- > Native(callIndex))
			++seek.ops;
		
		// this
		if (argumentIndex-- > Native(callIndex) && (seek.ops + 1)->text.bytes <= seek.ops->text.bytes)
			++seek.ops;
		
		// arguments
		while (argumentIndex-- > Native(callIndex))
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


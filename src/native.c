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
	const char *location;
	struct Native(Context) seek = *context;
	uint32_t breakArray = 0;
	
	assert(context);
	assert(context->ecc);
	
	while (seek.ops->text.location == Text(nativeCode).location)
	{
		if (!seek.parent)
			return seek.ops->text;
		
		if (seek.argumentOffset && argumentIndex >= Native(thisIndex))
		{
			++argumentIndex;
			breakArray <<= 1;
			
			if (seek.argumentOffset == 2)
				breakArray |= 2;
		}
		seek = *seek.parent;
	}
	
	if (argumentIndex > Native(noIndex))
	{
		while (seek.ops->native != Op.call
			&& seek.ops->native != Op.callLocal
			&& seek.ops->native != Op.callMember
			&& seek.ops->native != Op.callProperty
			&& seek.ops->native != Op.eval
			&& seek.ops->native != Op.construct)
			--seek.ops;
		
		if (seek.ops->value.data.integer <= argumentIndex)
			return seek.ops->text;
		
		// func
		if (argumentIndex-- > Native(callIndex))
			++seek.ops;
		
		// this
		if (argumentIndex-- > Native(callIndex) && (seek.ops + 1)->text.location <= seek.ops->text.location)
			++seek.ops;
		
		// arguments
		while (argumentIndex-- > Native(callIndex))
		{
			location = seek.ops->text.location + seek.ops->text.length;
			while (location > seek.ops->text.location && seek.ops->text.location)
				++seek.ops;
			
			if (breakArray & 0x1 && seek.ops->native == Op.array)
				++seek.ops;
			
			breakArray >>= 1;
		}
	}
	
	return seek.ops->text;
}


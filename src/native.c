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

void assertParameterCount (struct Ecc * const ecc, int parameterCount)
{
	assert(ecc->environment->hashmapCount == parameterCount + 3);
}

int argumentCount (struct Ecc * const ecc)
{
	return ecc->environment->hashmapCount - 3;
}

struct Value argument (struct Ecc * const ecc, int argumentIndex)
{
	return ecc->environment->hashmap[argumentIndex + 3].data.value;
}

void assertVariableParameter (struct Ecc * const ecc)
{
	assert(ecc->environment->hashmap[2].data.value.type == Value(objectType));
}

int variableArgumentCount (struct Ecc * const ecc)
{
	return ecc->environment->hashmap[2].data.value.data.object->elementCount;
}

struct Value variableArgument (struct Ecc * const ecc, int argumentIndex)
{
	return ecc->environment->hashmap[2].data.value.data.object->element[argumentIndex].data.value;
}

struct Text textSeek (struct Native(Context) * const sourceContext, struct Ecc * const ecc, enum Native(Index) argumentIndex)
{
	assert(sourceContext);
	assert(ecc);
	
	const char *location;
	struct Native(Context) context = *sourceContext;
	uint32_t breakArray = 0;
	
	while (context.ops->text.location == Text(nativeCode).location)
	{
		if (!context.parent)
			return context.ops->text;
		
		if (context.argumentOffset && argumentIndex >= Native(thisIndex))
		{
			++argumentIndex;
			breakArray <<= 1;
			
			if (context.argumentOffset == 2)
				breakArray |= 2;
		}
		context = *context.parent;
	}
	
	if (argumentIndex > Native(noIndex))
	{
		while (context.ops->native != Op.call && context.ops->native != Op.eval && context.ops->native != Op.construct)
			--context.ops;
		
		// func
		if (argumentIndex-- > Native(callIndex))
			++context.ops;
		
		// this
		if (argumentIndex-- > Native(callIndex) && (context.ops + 1)->text.location <= context.ops->text.location)
			++context.ops;
		
		// arguments
		while (argumentIndex-- > Native(callIndex))
		{
			location = context.ops->text.location + context.ops->text.length;
			while (location > context.ops->text.location && context.ops->text.location)
				++context.ops;
			
			if (breakArray & 0x1 && context.ops->native == Op.array)
				++context.ops;
			
			breakArray >>= 1;
		}
	}
	
	return context.ops->text;
}


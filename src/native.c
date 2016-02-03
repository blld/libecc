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

struct Text textSeek (const struct Op ** ops, struct Ecc * const ecc, enum Native(Index) argumentIndex)
{
	assert(ops);
	assert(ecc);
	
	const char *location;
	struct Native(Frame) *frame = (struct Native(Frame) *)ops;
	uint32_t breakArray = 0;
	
	while (frame->ops->text.location == Text(nativeCode).location)
	{
		if (!frame->parent)
			return frame->ops->text;
		
		if (frame->argumentOffset && argumentIndex >= Native(thisIndex))
		{
			++argumentIndex;
			breakArray <<= 1;
			
			if (frame->argumentOffset == 2)
				breakArray |= 2;
		}
		frame = (struct Native(Frame) *)frame->parent;
	}
	
	if (frame && argumentIndex > Native(noIndex))
	{
		while (frame->ops->native != Op.call && frame->ops->native != Op.eval && frame->ops->native != Op.construct)
			--frame->ops;
		
		// func
		if (argumentIndex-- > Native(callIndex))
			++frame->ops;
		
		// this
		if (argumentIndex-- > Native(callIndex) && (frame->ops + 1)->text.location <= frame->ops->text.location)
			++frame->ops;
		
		// arguments
		while (argumentIndex-- > Native(callIndex))
		{
			location = frame->ops->text.location + frame->ops->text.length;
			while (location > frame->ops->text.location && frame->ops->text.location)
				++frame->ops;
			
			if (breakArray & 0x1 && frame->ops->native == Op.array)
				++frame->ops;
			
			breakArray >>= 1;
		}
	}
	
	return frame->ops->text;
}


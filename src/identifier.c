//
//  identifier.c
//  monade
//
//  Created by Bouilland Aurélien on 20/07/2014.
//  Copyright (c) 2014 Teppen Game. All rights reserved.
//

#include "identifier.h"

// MARK: - Private

static struct Text *identifierPool = NULL;
static uint16_t identifierCount = 0;
static uint16_t identifierCapacity = 0;

static char **charsList = NULL;
static uint16_t charsCount = 0;

Structure internalIdentifier;
Structure prototypeIdentifier;
Structure constructorIdentifier;
Structure lengthIdentifier;
Structure argumentsIdentifier;
Structure nameIdentifier;
Structure messageIdentifier;
Structure toStringIdentifier;
Structure valueOfIdentifier;
Structure evalIdentifier;

// MARK: - Static Members

// MARK: - Methods

void setup (void)
{
	if (!identifierPool)
	{
		prototypeIdentifier = makeWithCString("prototype");
		constructorIdentifier = makeWithCString("constructor");
		lengthIdentifier = makeWithCString("length");
		argumentsIdentifier = makeWithCString("arguments");
		nameIdentifier = makeWithCString("name");
		messageIdentifier = makeWithCString("message");
		toStringIdentifier = makeWithCString("toString");
		valueOfIdentifier = makeWithCString("valueOf");
		evalIdentifier = makeWithCString("eval");
	}
}

void teardown (void)
{
	while (charsCount)
		free(charsList[--charsCount]), charsList[charsCount] = NULL;
	
	free(identifierPool), identifierPool = NULL, identifierCount = 0, identifierCapacity = 0;
}

Structure makeWithCString (const char *cString)
{
	return makeWithText(Text.make(cString, strlen(cString)), 0);
}

Structure makeWithText (const struct Text text, int copyOnCreate)
{
	uint16_t number = 0;
	
	#warning TODO: use binary tree
	for (uint_fast16_t index = 0; index < identifierCount; ++index)
	{
		if (text.length == identifierPool[index].length && memcmp(identifierPool[index].location, text.location, text.length) == 0)
		{
			number = index + 1;
			break;
		}
	}
	
	if (!number)
	{
		if (identifierCount >= identifierCapacity)
		{
			identifierCapacity = identifierCapacity? identifierCapacity * 2: 1;
			identifierPool = realloc(identifierPool, identifierCapacity * sizeof(*identifierPool));
		}
		
		if (copyOnCreate)
		{
			char *chars = malloc(text.length);
			memcpy(chars, text.location, text.length);
			charsList = realloc(charsList, sizeof(*charsList) * (charsCount + 1));
			charsList[charsCount++] = chars;
			identifierPool[identifierCount++] = Text.make(chars, text.length);
		}
		else
			identifierPool[identifierCount++] = text;
		
		number = identifierCount;
	}
	
	return (Structure) {{{
		number >> 12 & 0xf,
		number >> 8 & 0xf,
		number >> 4 & 0xf,
		number >> 0 & 0xf,
	}}};
}

Structure none (void)
{
	return (Structure) {{{ 0 }}};
}

Structure prototype (void)
{
	return prototypeIdentifier;
}

Structure constructor (void)
{
	return constructorIdentifier;
}

Structure length (void)
{
	return lengthIdentifier;
}

Structure arguments (void)
{
	return argumentsIdentifier;
}

Structure name (void)
{
	return nameIdentifier;
}

Structure message (void)
{
	return messageIdentifier;
}

Structure toString (void)
{
	return toStringIdentifier;
}

Structure valueOf (void)
{
	return valueOfIdentifier;
}

Structure eval (void)
{
	return evalIdentifier;
}

int isEqual (Structure self, Structure to)
{
	return self.data.integer == to.data.integer;
}

struct Text *textOf (Structure self)
{
	uint16_t number = self.data.depth[0] << 12 | self.data.depth[1] << 8 | self.data.depth[2] << 4 | self.data.depth[3];
	if (number)
		return &identifierPool[number - 1];
	else
		return NULL;
}

void dumpTo (Structure self, FILE *file)
{
	struct Text *text = textOf(self);
	fprintf(file, "%.*s", text->length, text->location);
}

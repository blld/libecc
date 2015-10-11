//
//  identifier.c
//  libecc
//
//  Created by Bouilland Aurélien on 20/07/2014.
//  Copyright (c) 2014 Libeccio. All rights reserved.
//

#include "identifier.h"

// MARK: - Private

static struct Text *identifierPool = NULL;
static uint16_t identifierCount = 0;
static uint16_t identifierCapacity = 0;

static char **charsList = NULL;
static uint16_t charsCount = 0;

struct Identifier Identifier(none) = {{{ 0 }}};

struct Identifier Identifier(internal);
struct Identifier Identifier(prototype);
struct Identifier Identifier(constructor);
struct Identifier Identifier(length);
struct Identifier Identifier(arguments);
struct Identifier Identifier(name);
struct Identifier Identifier(message);
struct Identifier Identifier(toString);
struct Identifier Identifier(valueOf);
struct Identifier Identifier(eval);
struct Identifier Identifier(value);
struct Identifier Identifier(writable);
struct Identifier Identifier(enumerable);
struct Identifier Identifier(configurable);
struct Identifier Identifier(get);
struct Identifier Identifier(set);

// MARK: - Static Members

// MARK: - Methods

void setup (void)
{
	if (!identifierPool)
	{
		Identifier(prototype) = makeWithCString("prototype");
		Identifier(constructor) = makeWithCString("constructor");
		Identifier(length) = makeWithCString("length");
		Identifier(arguments) = makeWithCString("arguments");
		Identifier(name) = makeWithCString("name");
		Identifier(message) = makeWithCString("message");
		Identifier(toString) = makeWithCString("toString");
		Identifier(valueOf) = makeWithCString("valueOf");
		Identifier(eval) = makeWithCString("eval");
		Identifier(value) = makeWithCString("value");
		Identifier(writable) = makeWithCString("writable");
		Identifier(enumerable) = makeWithCString("enumerable");
		Identifier(configurable) = makeWithCString("configurable");
		Identifier(get) = makeWithCString("get");
		Identifier(set) = makeWithCString("set");
	}
}

void teardown (void)
{
	while (charsCount)
		free(charsList[--charsCount]), charsList[charsCount] = NULL;
	
	free(identifierPool), identifierPool = NULL, identifierCount = 0, identifierCapacity = 0;
}

struct Identifier makeWithCString (const char *cString)
{
	return makeWithText(Text.make(cString, strlen(cString)), 0);
}

struct Identifier makeWithText (const struct Text text, int copyOnCreate)
{
	uint16_t number = 0, index = 0;
	
	#warning TODO: use binary tree
	for (index = 0; index < identifierCount; ++index)
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
	
	return (struct Identifier) {{{
		number >> 12 & 0xf,
		number >> 8 & 0xf,
		number >> 4 & 0xf,
		number >> 0 & 0xf,
	}}};
}

int isEqual (struct Identifier self, struct Identifier to)
{
	return self.data.integer == to.data.integer;
}

struct Text *textOf (struct Identifier identifier)
{
	uint16_t number = identifier.data.depth[0] << 12 | identifier.data.depth[1] << 8 | identifier.data.depth[2] << 4 | identifier.data.depth[3];
	if (number)
		return &identifierPool[number - 1];
	else
		return NULL;
}

void dumpTo (struct Identifier identifier, FILE *file)
{
	struct Text *text = textOf(identifier);
	fprintf(file, "%.*s", text->length, text->location);
}

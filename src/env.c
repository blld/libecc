//
//  env.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#include "env.h"

// MARK: - Private

static Instance self = NULL;

static void setColor(enum Module(Color) color)
{
	if (self->isTerminal)
		fprintf(stderr, "\x1B[%d;1m", color);
}

static void resetColor()
{
	if (self->isTerminal)
		fprintf(stderr, "\x1B[0m");
}

// MARK: - Static Members

// MARK: - Methods

void setup (void)
{
	assert (!self);
	
	self = malloc(sizeof(*self));
	assert(self);
	*self = Module.identity;
	
	self->isTerminal = getenv("TERM") != NULL;
}

void teardown (void)
{
	assert(self);
	
	free(self), self = NULL;
}

Instance shared (void)
{
	return self;
}

void print (const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
}

void printColor (enum Module(Color) color, const char *format, ...)
{
	setColor(color);
	
	va_list ap;
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
	
	resetColor();
}

void printError (int typeLength, const char *type, const char *format, ...)
{
	setColor(Module(Red));
	fprintf(stderr, "%.*s", typeLength, type);
	resetColor();
	
	fprintf(stderr, ": ");
	
	setColor(Module(Black));
	va_list ap;
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
	resetColor();
	
	putc('\n', stderr);
}

void printWarning (const char *format, ...)
{
	setColor(Module(Yellow));
	fprintf(stderr, "Warning");
	resetColor();
	
	fprintf(stderr, ": ");
	
	setColor(Module(Black));
	va_list ap;
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
	resetColor();
	
	putc('\n', stderr);
}

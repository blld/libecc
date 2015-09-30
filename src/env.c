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

static void printBoldVA (const char *format, va_list ap)
{
	if (self->isTerminal)
		fprintf(stderr, "\x1B[1m");
	
	vfprintf(stderr, format, ap);
	
	if (self->isTerminal)
		fprintf(stderr, "\x1B[0m");
}

// MARK: - Static Members

// MARK: - Methods

void setup (void)
{
	assert(!self);
	
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
	printColor(Env(Red), "%.*s", typeLength, type);
	
	putc(':', stderr);
	putc(' ', stderr);
	
	va_list ap;
	va_start(ap, format);
	printBoldVA(format, ap);
	va_end(ap);
	
	putc('\n', stderr);
}

void printWarning (const char *format, ...)
{
	printColor(Env(Yellow), "Warning");
	
	putc(':', stderr);
	putc(' ', stderr);
	
	va_list ap;
	va_start(ap, format);
	printBoldVA(format, ap);
	va_end(ap);
	
	putc('\n', stderr);
}

void printBold (const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	printBoldVA(format, ap);
	va_end(ap);
}

void printDim (const char *format, ...)
{
	if (self->isTerminal)
		fprintf(stderr, "\x1B[2m");
	
	va_list ap;
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
	
	if (self->isTerminal)
		fprintf(stderr, "\x1B[0m");
}

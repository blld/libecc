//
//  chars.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#include "chars.h"

// MARK: - Private

// MARK: - Static Members

// MARK: - Methods

Instance createVA (int16_t length, const char *format, va_list ap)
{
	Instance self;
	
	self = createSized(length);
	vsprintf(self->chars, format, ap);
	
	return self;
}

Instance create (const char *format, ...)
{
	int16_t length;
	va_list ap;
	Instance self;
	
	va_start(ap, format);
	length = vsnprintf(NULL, 0, format, ap);
	va_end(ap);
	
	va_start(ap, format);
	self = createVA(length, format, ap);
	va_end(ap);
	
	return self;
}

Instance createSized (uint16_t length)
{
	Instance self = malloc(sizeof(*self) + length);
	assert(self);
	Pool.addChars(self);
	self->length = length;
	self->flags = 0;
	return self;
}

void destroy (Instance self)
{
	assert(self);
	
	free(self), self = NULL;
}

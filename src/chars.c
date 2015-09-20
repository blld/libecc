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

Instance createVA (const char *format, va_list ap)
{
	va_list apcpy;
	va_copy(apcpy, ap);
	uint32_t length = vsnprintf(NULL, 0, format, apcpy);
	va_end(apcpy);
	
	Instance self = createSized(length);
	vsprintf(self->chars, format, ap);
	
	return self;
}

Instance create (const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	Instance self = createVA(format, ap);
	va_end(ap);
	
	return self;
}

Instance createSized (uint16_t length)
{
	Instance self = malloc(sizeof(*self) + length + 1);
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

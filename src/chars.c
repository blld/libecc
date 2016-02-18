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

struct Chars * createVA (uint16_t length, const char *format, va_list ap)
{
	struct Chars *self;
	
	self = createSized(length);
	vsprintf(self->bytes, format, ap);
	
	return self;
}

struct Chars * create (const char *format, ...)
{
	uint16_t length;
	va_list ap;
	struct Chars *self;
	
	va_start(ap, format);
	length = vsnprintf(NULL, 0, format, ap);
	va_end(ap);
	
	va_start(ap, format);
	self = createVA(length, format, ap);
	va_end(ap);
	
	return self;
}

struct Chars * createSized (uint16_t length)
{
	struct Chars *self = malloc(sizeof(*self) + length);
	Pool.addChars(self);
	*self = Chars.identity;
	
	self->length = length;
	self->bytes[length] = '\0';
	
	return self;
}

struct Chars * createWithBytes (uint16_t length, const char *bytes)
{
	struct Chars *self = malloc(sizeof(*self) + length);
	Pool.addChars(self);
	*self = Chars.identity;
	
	self->length = length;
	memcpy(self->bytes, bytes, length);
	self->bytes[length] = '\0';
	
	return self;
}

void destroy (struct Chars *self)
{
	assert(self);
	
	free(self), self = NULL;
}

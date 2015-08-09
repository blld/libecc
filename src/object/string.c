//
//  string.c
//  libecc
//
//  Created by Bouilland Aurélien on 04/07/2015.
//  Copyright (c) 2015 Libeccio. All rights reserved.
//

#include "string.h"

// MARK: - Private

static const struct Text stringType = { 15, "[object String]" };

// MARK: - Static Members

// MARK: - Methods

Instance create (const char *format, ...)
{
	Instance self = malloc(sizeof(*self));
	assert(self);
	*self = Module.identity;
	
	Object.initialize(&self->object, NULL);
	
	self->object.type = &stringType;
	
	va_list ap, apsize;
	va_start(ap, format);
	va_copy(apsize, ap);
	self->length = vsnprintf(NULL, 0, format, apsize);
	va_end(apsize);
	
	self->chars = malloc(self->length + 1);
	vsprintf(self->chars, format, ap);
	va_end(ap);
	
	return self;
}

void destroy (Instance self)
{
	assert(self);
	
	free(self->chars), self->chars = NULL;
	free(self), self = NULL;
}

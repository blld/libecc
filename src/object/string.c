//
//  string.c
//  libecc
//
//  Created by Bouilland Aurélien on 04/07/2015.
//  Copyright (c) 2015 Libeccio. All rights reserved.
//

#include "string.h"

// MARK: - Private

// MARK: - Static Members

// MARK: - Methods

Instance create (const char *format, ...)
{
	va_list ap;
	
	Instance self = malloc(sizeof(*self));
	assert(self);
	Pool.addObject(&self->object);
	*self = Module.identity;
	
	Object.initialize(&self->object, NULL);
	
	self->object.type = &Text(stringType);
	
	va_start(ap, format);
	self->length = vsnprintf(NULL, 0, format, ap);
	va_end(ap);
	
	self->chars = malloc(self->length + 1);
	
	va_start(ap, format);
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

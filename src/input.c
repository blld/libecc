//
//  input.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#include "input.h"

// MARK: - Private

// MARK: - Static Members

static Instance create()
{
	Instance self = malloc(sizeof(*self));
	assert(self);
	*self = Module.identity;
	
	self->lineCapacity = 8;
	self->lineCount = 1;
	
	size_t linesBytes = sizeof(*self->lines) * self->lineCapacity;
	self->lines = malloc(linesBytes);
	memset(self->lines, 0, linesBytes);
	
	return self;
}

// MARK: - Methods

Instance createFromFile (const char *filename)
{
	assert(filename);
	
	FILE *file = fopen(filename, "rb");
	if (!file)
	{
		fprintf(stderr, "error opening file '%s'\n", filename);
		return NULL;
	}
	
	// <!> SEEK_END undefined on binary stream, assume correctness
	long size;
	if (fseek(file, 0, SEEK_END) || (size = ftell(file)) < 0 || fseek(file, 0, SEEK_SET))
	{
		fprintf(stderr, "error handling file '%s'\n", filename);
		fclose(file);
		return NULL;
	}
	
	Instance self = create();
	
	strncat(self->name, filename, sizeof(self->name) - 1);
	self->length = (uint32_t)size;
	self->bytes = malloc(size);
	assert(self->bytes);
	fread(self->bytes, sizeof(char), size, file);
	fclose(file), file = NULL;
	
	return self;
}

Instance createFromBytes (const char *bytes, uint32_t length, const char *name, ...)
{
	assert(bytes);
	
	Instance self = create();
	
	if (name) {
		va_list ap;
		va_start(ap, name);
		vsnprintf(self->name, sizeof(self->name), name, ap);
		va_end(ap);
	}
	self->length = length;
	self->bytes = malloc(length);
	assert(self->bytes);
	memcpy(self->bytes, bytes, length);
	
	return self;
}

void destroy (Instance self)
{
	assert(self);
	
	while (self->escapedTextCount--)
		free((char *)self->escapedTextList[self->escapedTextCount].location), self->escapedTextList[self->escapedTextCount].location = NULL;
	
	free(self->escapedTextList), self->escapedTextList = NULL;
	free(self->bytes), self->bytes = NULL;
	free(self->lines), self->lines = NULL;
	free(self), self = NULL;
}

void printText (Instance self, struct Text text)
{
	assert(self);
	
	int32_t line = findLine(self, text);
	
	if (line >= 0)
	{
		Env.printColor(Env(Black), "%s:%d\n", self->name, line);
		
		size_t start = self->lines[line], length = 0;
		const char *location = self->bytes + start;
		
		do
		{
			if (!isblank(location[length]) && !isgraph(location[length]) && location[length] >= 0)
				break;
			
			++length;
		} while (start + length < self->length);
		
		Env.print("%.*s\n", length, location);
		
		char mark[length + 1];
		for (int b = 0; b <= length; ++b)
			if (b >= length || isgraph(location[b]))
				mark[b] = ' ';
			else
				mark[b] = location[b];
		
		long index = text.location - location;
		while (++index < text.location - location + text.length)
			mark[index] = '~';
		
		mark[text.location - location] = '^';
		mark[length + 1] = '\0';
		
		if ((text.location - location) > 0)
			Env.printDim("%.*s", (text.location - location), mark);
		
		Env.printColor(Env(Green), "%s\n", mark + (text.location - location));
	}
	else
		Env.printColor(Env(Black), "%s\n", self->name);
}

int32_t findLine (Instance self, struct Text text)
{
	uint_fast16_t line = self->lineCount + 1;
	while (line--)
		if ((self->bytes + self->lines[line] <= text.location) && (self->bytes + self->lines[line] < self->bytes + self->length))
			return line;
	
	return -1;
}

void addEscapedText (Instance self, struct Text escapedText)
{
	self->escapedTextList = realloc(self->escapedTextList, sizeof(*self->escapedTextList) * (self->escapedTextCount + 1));
	self->escapedTextList[self->escapedTextCount++] = escapedText;
}

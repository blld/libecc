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
	size_t linesBytes;
	Instance self = malloc(sizeof(*self));
	
	assert(self);
	*self = Module.identity;
	
	self->lineCapacity = 8;
	self->lineCount = 1;
	
	linesBytes = sizeof(*self->lines) * self->lineCapacity;
	self->lines = malloc(linesBytes);
	memset(self->lines, 0, linesBytes);
	
	return self;
}

// MARK: - Methods

Instance createFromFile (const char *filename)
{
	struct Text inputError;
	FILE *file;
	long size;
	Instance self;
	
	assert(filename);
	
	inputError = *Text.inputErrorName();
	
	file = fopen(filename, "rb");
	if (!file)
	{
		Env.printError(inputError.length, inputError.location, "cannot open file '%s'", filename);
		return NULL;
	}
	
	if (fseek(file, 0, SEEK_END) || (size = ftell(file)) < 0 || fseek(file, 0, SEEK_SET))
	{
		Env.printError(inputError.length, inputError.location, "cannot handle file '%s'", filename);
		fclose(file);
		return NULL;
	}
	
	self = create();
	
	strncat(self->name, filename, sizeof(self->name) - 1);
	self->bytes = malloc(size);
	self->length = (uint32_t)fread(self->bytes, sizeof(char), size, file);
	fclose(file), file = NULL;
	
	return self;
}

Instance createFromBytes (const char *bytes, uint32_t length, const char *name, ...)
{
	Instance self;
	
	assert(bytes);
	
	self = create();
	
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
	int32_t line;
	
	assert(self);
	
	line = findLine(self, text);
	
	if (self->name[0] == '(')
		Env.printColor(0, Env(Dim), "%s", self->name);
	else
		Env.printColor(0, Env(Bold), "%s", self->name, line);
	
	if (line >= 0)
	{
		const char *location;
		size_t start, length = 0;
		
		Env.printColor(0, Env(Bold), " line:%d", line);
		Env.newline();
		
		start = self->lines[line];
		location = self->bytes + start;
		
		do
		{
			if (!isblank(location[length]) && !isgraph(location[length]) && location[length] >= 0)
				break;
			
			++length;
		} while (start + length < self->length);
		
		Env.print("%.*s", length, location);
		Env.newline();
		
		{
			char mark[length + 1];
			int b;
			long index;
			
			for (b = 0; b <= length; ++b)
				if (b >= length || isgraph(location[b]))
					mark[b] = ' ';
				else
					mark[b] = location[b];
			
			index = text.location - location;
			while (++index < text.location - location + text.length)
				mark[index] = '~';
			
			mark[text.location - location] = '^';
			
			if ((text.location - location) > 0)
				Env.printColor(0, Env(Invisible), "%.*s", (text.location - location), mark);
			
			Env.printColor(Env(Green), Env(Bold), "%.*s", text.length? text.length: 1, mark + (text.location - location));
		}
	}
	
	Env.newline();
}

int32_t findLine (Instance self, struct Text text)
{
	uint16_t line = self->lineCount + 1;
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

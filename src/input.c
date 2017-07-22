//
//  input.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#define Implementation
#include "input.h"

#include "chars.h"

// MARK: - Private

// MARK: - Static Members

static
struct Input * create()
{
	size_t linesBytes;
	struct Input *self = malloc(sizeof(*self));
	*self = Input.identity;
	
	self->lineCapacity = 8;
	self->lineCount = 1;
	
	linesBytes = sizeof(*self->lines) * self->lineCapacity;
	self->lines = malloc(linesBytes);
	memset(self->lines, 0, linesBytes);
	
	return self;
}

static
void printInput (const char *name, uint16_t line)
{
	if (name[0] == '(')
		Env.printColor(0, Env(dim), "%s", name);
	else
		Env.printColor(0, Env(bold), "%s", name);
	
	Env.printColor(0, Env(bold), " line:%d", line);
}

// MARK: - Methods

struct Input * createFromFile (const char *filename)
{
	struct Text inputError = Text(inputErrorName);
	FILE *file;
	long size;
	struct Input *self;
	
	assert(filename);
	
	file = fopen(filename, "rb");
	if (!file)
	{
		Env.printError(inputError.length, inputError.bytes, "cannot open file '%s'", filename);
		return NULL;
	}
	
	if (fseek(file, 0, SEEK_END) || (size = ftell(file)) < 0 || fseek(file, 0, SEEK_SET))
	{
		Env.printError(inputError.length, inputError.bytes, "cannot handle file '%s'", filename);
		fclose(file);
		return NULL;
	}
	
	self = create();
	
	strncat(self->name, filename, sizeof(self->name) - 1);
	self->bytes = malloc(size + 1);
	self->length = (uint32_t)fread(self->bytes, sizeof(char), size, file);
	fclose(file), file = NULL;
	self->bytes[size] = '\0';
	
//	FILE *f = fopen("error.txt", "w");
//	fprintf(f, "%.*s", self->length, self->bytes);
//	fclose(f);
	
	return self;
}

struct Input * createFromBytes (const char *bytes, uint32_t length, const char *name, ...)
{
	struct Input *self;
	
	assert(bytes);
	
	self = create();
	
	if (name)
	{
		va_list ap;
		va_start(ap, name);
		vsnprintf(self->name, sizeof(self->name), name, ap);
		va_end(ap);
	}
	self->length = length;
	self->bytes = malloc(length + 1);
	memcpy(self->bytes, bytes, length);
	self->bytes[length] = '\0';
	
	return self;
}

void destroy (struct Input *self)
{
	assert(self);
	
	free(self->attached), self->attached = NULL;
	free(self->bytes), self->bytes = NULL;
	free(self->lines), self->lines = NULL;
	free(self), self = NULL;
}

void printText (struct Input *self, struct Text text, struct Text ofLine, const char *ofInput, int fullLine)
{
	int32_t line = -1;
	const char *bytes = NULL;
	uint16_t length = 0;
	
	if (ofLine.length)
	{
		bytes = ofLine.bytes;
		length = ofLine.length;
		printInput(ofInput? ofInput: "native code", 1);
	}
	else if (!self)
		printInput(ofInput? ofInput: "(unknown input)", 0);
	else
	{
		line = findLine(self, text);
		printInput(ofInput? ofInput: self->name, line > 0? line: 0);
		
		if (line > 0)
		{
			uint32_t start = self->lines[line];
			
			bytes = self->bytes + start;
			do
			{
				if (!isblank(bytes[length]) && !isgraph(bytes[length]) && bytes[length] >= 0)
					break;
				
				++length;
			} while (start + length < self->length);
		}
	}
	
	if (!fullLine)
	{
		if (text.length)
			Env.printColor(0, 0, " `%.*s`", text.length, text.bytes);
	}
	else if (!length)
	{
		Env.newline();
		Env.printColor(0, 0, "%.*s", text.length, text.bytes);
	}
	else
	{
		Env.newline();
		Env.print("%.*s", length, bytes);
		Env.newline();
		
		if (length >= text.bytes - bytes)
		{
			char mark[length + 2];
			long index = 0, marked = 0;
			
			for (; index < text.bytes - bytes; ++index)
				if (isprint(bytes[index]))
					mark[index] = ' ';
				else
					mark[index] = bytes[index];
			
			if ((signed char)bytes[index] >= 0)
			{
				mark[index] = '^';
				marked = 1;
			}
			else
			{
				mark[index] = bytes[index];
				if (index > 0)
				{
					mark[index - 1] = '>';
					marked = 1;
				}
			}
			
			while (++index < text.bytes - bytes + text.length && index <= length)
				if (isprint(bytes[index]) || isspace(bytes[index]))
					mark[index] = '~';
				else
					mark[index] = bytes[index];
			
			if (!marked)
				mark[index++] = '<';
			
			mark[index] = '\0';
			
			if ((text.bytes - bytes) > 0)
				Env.printColor(0, Env(invisible), "%.*s", (text.bytes - bytes), mark);
			
			Env.printColor(Env(green), Env(bold), "%s", mark + (text.bytes - bytes));
		}
	}
	
	Env.newline();
}

int32_t findLine (struct Input *self, struct Text text)
{
	uint16_t line = self->lineCount + 1;
	while (line--)
		if ((self->bytes + self->lines[line] <= text.bytes) && (self->bytes + self->lines[line] < self->bytes + self->length))
			return line;
	
	return -1;
}

struct Value attachValue (struct Input *self, struct Value value)
{
	if (value.type == Value(charsType))
		value.data.chars->referenceCount++;
	
	self->attached = realloc(self->attached, sizeof(*self->attached) * (self->attachedCount + 1));
	self->attached[self->attachedCount] = value;
	return value;
}

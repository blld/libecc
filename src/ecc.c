//
//  ecc.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#define Implementation
#include "ecc.h"

#include "parser.h"
#include "oplist.h"
#include "pool.h"

// MARK: - Private

static int instanceCount = 0;

// MARK: - Static Members

static void addInput(struct Ecc *self, struct Input *input)
{
	self->inputs = realloc(self->inputs, sizeof(*self->inputs) * (self->inputCount + 1));
	self->inputs[self->inputCount++] = input;
}

// MARK: - Methods

struct Ecc *create (void)
{
	struct Ecc *self;
	
	if (!instanceCount++)
	{
		Env.setup();
		Pool.setup();
		Key.setup();
		Global.setup();
	}
	
	self = malloc(sizeof(*self));
	*self = Ecc.identity;
	
	self->global = Global.create();
	self->maximumCallDepth = 512;
	
	return self;
}

void destroy (struct Ecc *self)
{
	assert(self);
	
	while (self->inputCount--)
		Input.destroy(self->inputs[self->inputCount]), self->inputs[self->inputCount] = NULL;
	
	free(self->inputs), self->inputs = NULL;
	free(self->envList), self->envList = NULL;
	free(self), self = NULL;
	
	if (!--instanceCount)
	{
		Global.teardown();
		Key.teardown();
		Pool.teardown();
		Env.teardown();
	}
}

void addFunction (struct Ecc *self, const char *name, const Native(Function) native, int argumentCount, enum Value(Flags) flags)
{
	assert(self);
	
	Function.addFunction(self->global, name, native, argumentCount, flags);
}

void addValue (struct Ecc *self, const char *name, struct Value value, enum Value(Flags) flags)
{
	assert(self);
	
	Function.addValue(self->global, name, value, flags);
}

useframe
int evalInput (struct Ecc *self, struct Input *input, enum Ecc(EvalFlags) flags)
{
	volatile int result = EXIT_SUCCESS, defaultTrap = !self->envCount, trap = defaultTrap || flags & Ecc(primitiveResult), catch = 0;
	struct Context context = {
		.environment = &self->global->environment,
		.ecc = self,
	};
	
	if (!input)
		return EXIT_FAILURE;
	
	if (trap)
		catch = setjmp(*pushEnv(self));
	
	if (catch)
	{
		if (defaultTrap)
		{
			struct Value value;
			struct Value name;
			struct Value message;
			
			result = EXIT_FAILURE;
			
			value = self->result;
			name = Value(undefined);
			
			if (value.type == Value(errorType))
			{
				name = Value.toString(&context, Object.getMember(value.data.object, &context, Key(name)));
				message = Value.toString(&context, Object.getMember(value.data.object, &context, Key(message)));
			}
			else
				message = Value.toString(&context, value);
			
			if (name.type == Value(undefinedType))
				name = Value.text(&Text(errorName));
			
#warning delete me
//		FILE *blah = fopen("blah.txt", "w");
//		fwrite(self->bytes, 1, self->length, blah);
			fprintf(stderr, "--- src:\n%s", input->bytes);
			
			Env.newline();
			Env.printError(Value.stringLength(name), Value.stringBytes(name), "%.*s" , Value.stringLength(message), Value.stringBytes(message));
			
			printTextInput(self, self->text, 1);
		}
	}
	else
	{
		if (flags & Ecc(globalThis))
			context.this = Value.object(&self->global->environment);
		
		evalInputWithContext(self, input, &context);
	}
	
	if (flags & Ecc(primitiveResult))
	{
		Context.setTextIndex(&context, Context(noIndex));
		
		if ((flags & Ecc(stringResult)) == Ecc(stringResult))
			self->result = Value.toString(&context, self->result);
		else
			self->result = Value.toPrimitive(&context, self->result, Value(hintAuto));
	}
	
	if (trap)
		popEnv(self);
	
	if (catch && !defaultTrap)
		Ecc.jmpEnv(self, self->result);
	
	return result;
}

void evalInputWithContext (struct Ecc *self, struct Input *input, struct Context *context)
{
	struct Lexer *lexer;
	struct Parser *parser;
	struct Function *function;
	
	assert(self);
	assert(self->envCount);
	
	addInput(self, input);
	
	lexer = Lexer.createWithInput(input);
	parser = Parser.createWithLexer(lexer);
	function = Parser.parseWithEnvironment(parser, context->environment);
	context->ops = function->oplist->ops;
	context->environment = &function->environment;
	
	Parser.destroy(parser), parser = NULL;
	
//	fprintf(stderr, "--- source:\n%.*s\n", input->length, input->bytes);
//	OpList.dumpTo(function->oplist, stderr);
	
	self->result = Value(undefined);
	
	context->ops->native(context);
}

jmp_buf * pushEnv(struct Ecc *self)
{
	if (self->envCount >= self->envCapacity)
	{
		uint16_t capacity = self->envCapacity? self->envCapacity * 2: 8;
		self->envList = realloc(self->envList, sizeof(*self->envList) * capacity);
		memset(self->envList + self->envCapacity, 0, sizeof(*self->envList) * (capacity - self->envCapacity));
		self->envCapacity = capacity;
	}
	return &self->envList[self->envCount++];
}

void popEnv(struct Ecc *self)
{
	assert(self->envCount);
	
	--self->envCount;
}

void jmpEnv (struct Ecc *self, struct Value value)
{
	assert(self);
	assert(self->envCount);
	
	self->result = value;
	
	if (value.type == Value(errorType))
		self->text = value.data.error->text;
	
	longjmp(self->envList[self->envCount - 1], 1);
}

void fatal (const char *format, ...)
{
	int16_t length;
	va_list ap;
	
	va_start(ap, format);
	length = vsnprintf(NULL, 0, format, ap);
	va_end(ap);
	{
		const char type[] = "Fatal";
		char buffer[length + 1];
		
		va_start(ap, format);
		vsprintf(buffer, format, ap);
		va_end(ap);
		
		Env.printError(sizeof(type)-1, type, "%s", buffer);
	}
	
	exit(EXIT_FAILURE);
}

struct Input * findInput (struct Ecc *self, struct Text text)
{
	uint16_t i;
	
	for (i = 0; i < self->inputCount; ++i)
		if (text.bytes >= self->inputs[i]->bytes && text.bytes <= self->inputs[i]->bytes + self->inputs[i]->length)
			return self->inputs[i];
	
	return NULL;
}

void printTextInput (struct Ecc *self, struct Text text, int fullLine)
{
	assert(self);
	
	Input.printText(findInput(self, text), text, fullLine);
}

void garbageCollect(struct Ecc *self)
{
	Pool.unmarkAll();
	Pool.markValue(Value.object(Arguments(prototype)));
	Pool.markValue(Value.function(self->global));
	Pool.collectUnmarked();
}

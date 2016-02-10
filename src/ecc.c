//
//  ecc.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#include "ecc.h"

// MARK: - Private

static int instanceCount = 0;

// MARK: - Static Members

static struct Input * findInput (struct Ecc *self, struct Text text)
{
	uint16_t i;
	
	for (i = 0; i < self->inputCount; ++i)
		if (text.location >= self->inputs[i]->bytes && text.location <= self->inputs[i]->bytes + self->inputs[i]->length)
			return self->inputs[i];
	
	return NULL;
}

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

void addNative (struct Ecc *self, const char *name, const Native(Function) native, int argumentCount, enum Value(Flags) flags)
{
	assert(self);
	
	Function.addNative(self->global, name, native, argumentCount, flags);
}

void addValue (struct Ecc *self, const char *name, struct Value value, enum Value(Flags) flags)
{
	assert(self);
	
	Function.addValue(self->global, name, value, flags);
}

int evalInput (struct Ecc *self, struct Input *input, enum Ecc(EvalFlags) flags)
{
	volatile int result = EXIT_SUCCESS, try = !self->envCount, catch = 0;
	struct Native(Context) context = {
		.environment = &self->global->environment,
		.ecc = self,
	};
	
	if (!input)
		return EXIT_FAILURE;
	
	if (try)
		catch = setjmp(*pushEnv(self));
	
	if (catch)
	{
		struct Value value;
		struct Value name;
		struct Value message;
		
		result = EXIT_FAILURE;
		
		value = self->result;
		name = Value(undefined);
		
		if (Value.isObject(value))
		{
			name = Value.toString(Object.get(value.data.object, Key(name)));
			message = Value.toString(Object.get(value.data.object, Key(message)));
		}
		else
			message = Value.toString(value);
		
		if (name.type == Value(undefinedType))
			name = Value.text(&Text(errorName));
		
		Env.newline();
		Env.printError(Value.stringLength(name), Value.stringChars(name), "%.*s" , Value.stringLength(message), Value.stringChars(message));
		
		printTextInput(self, self->text);
	}
	else
	{
		if (flags & Ecc(globalThis))
			context.this = Value.object(&self->global->environment);
		
		evalInputWithContext(self, input, &context);
		
		if (flags & Ecc(primitiveResult))
			self->result = Value.toPrimitive(&context, self->result, NULL, Value(hintAuto));
	}
	
	if (try)
		popEnv(self);
	
	return result;
}

void evalInputWithContext (struct Ecc *self, struct Input *input, struct Native(Context) *context)
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
		self->envCapacity = self->envCapacity? self->envCapacity * 2: 8;
		self->envList = realloc(self->envList, sizeof(*self->envList) * self->envCapacity);
	}
	
	return &self->envList[self->envCount++].buf;
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
	
	longjmp(self->envList[self->envCount - 1].buf, 1);
}

void printTextInput (struct Ecc *self, struct Text text)
{
	struct Input *input;
	
	assert(self);
	
	input = findInput(self, text);
	if (input)
		Input.printText(input, text);
	else
	{
		Env.printColor(0, Env(dim), "(unknown input)\n");
		Env.print("%.*s", text.length, text.location);
		Env.newline();
		Env.newline();
	}
}

void garbageCollect(struct Ecc *self)
{
	Pool.unmarkAll();
	Pool.markValue(Value.object(Arguments(prototype)));
	Pool.markValue(Value.function(self->global));
	Pool.collectUnmarked();
}

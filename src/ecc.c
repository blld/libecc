//
//  ecc.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#include "ecc.h"

static struct Input * findInput (Instance ecc, struct Text text);

// MARK: - Private

static int instanceCount = 0;

// MARK: - Static Members

static struct Input * findInput (Instance ecc, struct Text text)
{
	for (uint_fast16_t i = 0; i < ecc->inputCount; ++i)
		if (text.location >= ecc->inputs[i]->bytes && text.location < ecc->inputs[i]->bytes + ecc->inputs[i]->length)
			return ecc->inputs[i];
	
	return NULL;
}

static void addInput(Instance self, struct Input *input)
{
	self->inputs = realloc(self->inputs, sizeof(*self->inputs) * (self->inputCount + 1));
	self->inputs[self->inputCount++] = input;
}

// MARK: - Methods

Instance create (void)
{
	if (!instanceCount++)
	{
		Env.setup();
		Identifier.setup();
		Pool.setup();
		
		Object.setup();
		Error.setup();
		Array.setup();
		Date.setup();
	}
	
	Instance self = malloc(sizeof(*self));
	*self = Module.identity;
	
	self->global = Closure.create(NULL);
	
	Closure.addValue(self->global, "Infinity", Value.binary(INFINITY), 0);
	Closure.addValue(self->global, "NaN", Value.binary(NAN), 0);
	Closure.addValue(self->global, "null", Value.null(), 0);
	Closure.addValue(self->global, "undefined", Value.undefined(), 0);
	
	self->context = &self->global->context;
	
	return self;
}

void destroy (Instance self)
{
	assert(self);
	
//	Value.finalize(&self->result);
//	Closure.destroy(self->global), self->global = NULL;
	
	while (self->inputCount--)
		Input.destroy(self->inputs[self->inputCount]), self->inputs[self->inputCount] = NULL;
	
	free(self->inputs), self->inputs = NULL;
	free(self->envList), self->envList = NULL;
	free(self), self = NULL;
	
	if (!--instanceCount)
	{
		Pool.teardown();
		Identifier.teardown();
		Env.teardown();
	}
}

void eval (Instance self, struct Input *input)
{
	assert(self);
	assert(input);
	
	addInput(self, input);
	
	struct Lexer *lexer = Lexer.createWithInput(input);
	struct Parser *parser = Parser.createWithLexer(lexer);
	struct Closure *closure = Parser.parseWithContext(parser, self->context);
	const struct Op *ops = closure->oplist->ops;
	
	Parser.destroy(parser), parser = NULL;
	
	if (!self->envCapacity)
	{
		self->envCapacity = 1;
		self->envList = malloc(sizeof(*self->envList) * self->envCapacity);
		
		if (setjmp(self->envList[self->envIndex]))
		{
			struct Value value = self->result;
			struct Value name = Value.undefined();
			struct Value message = Value.undefined();
			
			if (Value.isObject(value))
			{
				name = Value.toString(Object.get(value.data.object, Identifier.name()));
				message = Value.toString(Object.get(value.data.object, Identifier.message()));
			}
			else
				message = Value.toString(value);
			
			if (name.type == Value(undefined))
				name = Value.text(Text.errorName());
			
			printTextInput(self, value.type == Value(error)? value.data.error->text: ops->text);
			Env.printError(Value.stringLength(name), Value.stringChars(name), "%.*s" , Value.stringLength(message), Value.stringChars(message));
			return;
		}
	}
	
	self->context = &closure->context;
	self->result = ops->function(&ops, self);
	self->context = self->context->prototype;
}

void throw (Instance self, struct Value value)
{
	self->result = value;
	longjmp(self->envList[self->envIndex], 1);
}

void printTextInput (Instance self, struct Text text)
{
	struct Input *input = findInput(self, text);
	if (input)
		Input.printTextInput(input, text);
	else
		Env.printColor(Env(Black), "(unknown input)\n\n");
}

// MARK: Memory

//void * allocate (Instance self, size_t size)
//{
//	void *pointer = c_malloc(size);
//	if (!pointer)
//	{
//		static const struct Bytes type = { "Out of memory", 13 };
//		struct Error *error = Error.create(&type, "cannot allocate %ld bytes", size);
//		if (error && self && self->exceptionCount)
//			throw(self, error);
//		
//		Env.printError(type.location, type.length, "cannot allocate %ld bytes", size);
//		exit(EXIT_FAILURE);
//	}
//	return pointer;
//}
//
//void * reallocate (Instance self, void *pointer, size_t size)
//{
//	pointer = c_realloc(pointer, size);
//	if (!pointer)
//	{
//		static const struct io_libecc_Bytes type = { "Out of memory", 13 };
//		struct Error *error = Error.create(&type, "cannot re-allocate %ld bytes", size);
//		if (error && self && self->exceptionCount)
//			throw(self, error);
//		
//		Env.printError(type.location, type.length, "cannot re-allocate %ld bytes", size);
//		exit(EXIT_FAILURE);
//	}
//	return pointer;
//}
//
//void deallocate (Instance self, void *pointer)
//{
//	c_free(pointer), pointer = NULL;
//}

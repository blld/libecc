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

static struct Value eval (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value value;
	struct Input *input;
	struct Object *context;
	struct Value this;
	
	Op.assertParameterCount(ecc, 1);
	
	value = Value.toString(Op.argument(ecc, 0));
	input = Input.createFromBytes(Value.stringChars(value), Value.stringLength(value), "(eval)");
	
	context = ecc->context;
	this = ecc->this;
	ecc->context = &ecc->global->context;
	ecc->this = Value.object(&ecc->global->context);
	evalInput(ecc, input);
	ecc->context = context;
	ecc->this = this;
	
	return Value.undefined();
}

static struct Value parseInt (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value value;
	struct Text text;
	int32_t radix;
	
	Op.assertParameterCount(ecc, 2);
	
	value = Value.toString(Op.argument(ecc, 0));
	radix = Value.toInteger(Op.argument(ecc, 1)).data.integer;
	
	text = Text.make(Value.stringChars(value), Value.stringLength(value));
	ecc->result = Lexer.parseInteger(text, radix);
	
	return Value.undefined();
}

static struct Value parseFloat (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value value;
	struct Text text;
	
	Op.assertParameterCount(ecc, 1);
	
	value = Value.toString(Op.argument(ecc, 0));
	
	text = Text.make(Value.stringChars(value), Value.stringLength(value));
	ecc->result = Lexer.parseBinary(text);
	
	return Value.undefined();
}

static struct Value isFinite (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value value;
	
	Op.assertParameterCount(ecc, 1);
	
	value = Value.toBinary(Op.argument(ecc, 0));
	ecc->result = Value.boolean(!isnan(value.data.binary) && !isinf(value.data.binary));
	
	return Value.undefined();
}

static struct Value isNaN (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value value;
	
	Op.assertParameterCount(ecc, 1);
	
	value = Value.toBinary(Op.argument(ecc, 0));
	ecc->result = Value.boolean(isnan(value.data.binary));
	
	return Value.undefined();
}

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
		
		Object.setupPrototype();
		Function.setup();
		Object.setup();
		Error.setup();
		Array.setup();
		Date.setup();
	}
	
	self = malloc(sizeof(*self));
	*self = Ecc.identity;
	
	self->global = Function.create(NULL);
	
	Function.addValue(self->global, "NaN", Value.binary(NAN), 0);
	Function.addValue(self->global, "Infinity", Value.binary(INFINITY), 0);
	Function.addValue(self->global, "undefined", Value.undefined(), 0);
	Function.addNative(self->global, "eval", eval, 1, 0);
	Function.addNative(self->global, "parseInt", parseInt, 2, 0);
	Function.addNative(self->global, "parseFloat", parseFloat, 1, 0);
	Function.addNative(self->global, "isNaN", isNaN, 1, 0);
	Function.addNative(self->global, "isFinite", isFinite, 1, 0);
	
	Function.addValue(self->global, "Object", Value.function(Object.constructor()), 0);
	Function.addValue(self->global, "Array", Value.function(Array.constructor()), 0);
	Function.addValue(self->global, "Function", Value.function(Function.constructor()), 0);
	
	Function.addValue(self->global, "Error", Value.object(Error.prototype()), 0);
	Function.addValue(self->global, "RangeError", Value.object(Error.rangePrototype()), 0);
	Function.addValue(self->global, "ReferenceError", Value.object(Error.referencePrototype()), 0);
	Function.addValue(self->global, "SyntaxError", Value.object(Error.syntaxPrototype()), 0);
	Function.addValue(self->global, "TypeError", Value.object(Error.typePrototype()), 0);
	Function.addValue(self->global, "URIError", Value.object(Error.uriPrototype()), 0);
	
	Function.addValue(self->global, "Array", Value.object(Array.prototype()), 0);
	Function.addValue(self->global, "Date", Value.object(Date.prototype()), 0);
	
	self->context = &self->global->context;
	
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
		Function.teardown();
		Object.teardown();
//		Error.teardown();
//		Array.teardown();
//		Date.teardown();
		
		Key.teardown();
		Env.teardown();
		
		Pool.teardown();
	}
}

void directGlobalAccess (struct Ecc *self, int allow)
{
	if (allow)
		self->this = Value.object(&self->global->context);
	else
		self->this = Value.undefined();
}

void addNative (struct Ecc *self, const char *name, const Native native, int argumentCount, enum Object(Flags) flags)
{
	assert(self);
	
	Function.addNative(self->global, name, native, argumentCount, flags);
}

void addValue (struct Ecc *self, const char *name, struct Value value, enum Object(Flags) flags)
{
	assert(self);
	
	Function.addValue(self->global, name, value, flags);
}

int evalInput (struct Ecc *self, struct Input *input)
{
	int result;
	struct Object *parentContext;
	struct Lexer *lexer;
	struct Parser *parser;
	struct Function *function;
	const struct Op *ops;
	
	assert(self);
	
	if (!input)
		return EXIT_FAILURE;
	
	addInput(self, input);
	
	result = EXIT_SUCCESS;
	
	parentContext = self->context;
	
	lexer = Lexer.createWithInput(input);
	parser = Parser.createWithLexer(lexer);
	function = Parser.parseWithContext(parser, parentContext);
	ops = function->oplist->ops;
	
	Parser.destroy(parser), parser = NULL;
	
	self->context = &function->context;
	self->result = Value.undefined();
	
//	fprintf(stderr, "source:\n%.*s\n", input->length, input->bytes);
//	OpList.dumpTo(function->oplist, stderr);
	
	if (!self->envCount)
	{
		if (!setjmp(*pushEnv(self)))
			ops->native(&ops, self);
		else
		{
			struct Value value;
			struct Value name;
			struct Value message;
			
			result = EXIT_FAILURE;
			
			value = self->result;
			name = Value.undefined();
			
			if (Value.isObject(value))
			{
				name = Value.toString(Object.get(value.data.object, Key(name)));
				message = Value.toString(Object.get(value.data.object, Key(message)));
			}
			else
				message = Value.toString(value);
			
			if (name.type == Value(undefined))
				name = Value.text(&Text(errorName));
			
			Env.newline();
			Env.printError(Value.stringLength(name), Value.stringChars(name), "%.*s" , Value.stringLength(message), Value.stringChars(message));
			printTextInput(self, value.type == Value(error)? value.data.error->text: ops->text);
		}
		
		popEnv(self);
	}
	else
		ops->native(&ops, self);
	
	self->context = parentContext;
	
	return result;
}

jmp_buf * pushEnv(struct Ecc *self)
{
	if (self->envCount >= self->envCapacity)
	{
		self->envCapacity = self->envCapacity? self->envCapacity * 2: 8;
		self->envList = realloc(self->envList, sizeof(*self->envList) * self->envCapacity);
	}
	
	self->envList[self->envCount].context = self->context;
	self->envList[self->envCount].this = self->this;
	
	return &self->envList[self->envCount++].buf;
}

void popEnv(struct Ecc *self)
{
	assert(self->envCount);
	
	--self->envCount;
	self->context = self->envList[self->envCount].context;
	self->this = self->envList[self->envCount].this;
}

void jmpEnv (struct Ecc *self, struct Value value)
{
	assert(self);
	
	self->result = value;
	
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
	Pool.markAll();
	Pool.unmarkValue(Value.function(self->global));
	Pool.unmarkValue(self->this);
	Pool.collectMarked();
}

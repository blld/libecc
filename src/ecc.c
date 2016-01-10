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
	
	if (ecc->construct)
		Ecc.jmpEnv(ecc, Value.error(Error.typeError((*ops)->text, "eval is not a constructor")));
	
	value = Value.toString(Op.argument(ecc, 0));
	input = Input.createFromBytes(Value.stringChars(value), Value.stringLength(value), "(eval)");
	
	context = ecc->context;
	this = ecc->this;
	ecc->context = &ecc->global->context;
	ecc->this = Value.object(&ecc->global->context);
	evalInput(ecc, input);
	ecc->context = context;
	ecc->this = this;
	
	return Value(undefined);
}

static struct Value parseInt (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value value;
	struct Text text;
	int32_t base;
	
	Op.assertParameterCount(ecc, 2);
	
	value = Value.toString(Op.argument(ecc, 0));
	base = Value.toInteger(Op.argument(ecc, 1)).data.integer;
	
	text = Text.make(Value.stringChars(value), Value.stringLength(value));
	
	if (!base)
	{
		// prevent octal auto-detection
		
		if (text.length > 2 && text.location[0] == '-')
		{
			if (text.location[1] == '0' && tolower(text.location[2]) != 'x')
				base = 10;
		}
		else if (text.length > 1 && text.location[0] == '0' && tolower(text.location[1]) != 'x')
			base = 10;
	}
	
	ecc->result = Lexer.parseInteger(text, base);
	
	return Value(undefined);
}

static struct Value parseFloat (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value value;
	struct Text text;
	
	Op.assertParameterCount(ecc, 1);
	
	value = Value.toString(Op.argument(ecc, 0));
	
	text = Text.make(Value.stringChars(value), Value.stringLength(value));
	ecc->result = Lexer.parseBinary(text);
	
	return Value(undefined);
}

static struct Value isFinite (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value value;
	
	Op.assertParameterCount(ecc, 1);
	
	value = Value.toBinary(Op.argument(ecc, 0));
	ecc->result = Value.truth(!isnan(value.data.binary) && !isinf(value.data.binary));
	
	return Value(undefined);
}

static struct Value isNaN (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value value;
	
	Op.assertParameterCount(ecc, 1);
	
	value = Value.toBinary(Op.argument(ecc, 0));
	ecc->result = Value.truth(isnan(value.data.binary));
	
	return Value(undefined);
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
		
		Function(prototype) = Object(prototype) = Object.create(NULL);
		
		Function.setup();
		Object.setup();
		String.setup();
		Error.setup();
		Array.setup();
		Date.setup();
		Math.setup();
		Number.setup();
		Boolean.setup();
		RegExp.setup();
	}
	
	self = malloc(sizeof(*self));
	*self = Ecc.identity;
	
	self->global = Function.create(NULL, NULL);
	
	Function.addValue(self->global, "NaN", Value.binary(NAN), 0);
	Function.addValue(self->global, "Infinity", Value.binary(INFINITY), 0);
	Function.addValue(self->global, "undefined", Value(undefined), 0);
	Function.addNative(self->global, "eval", eval, 1, 0);
	Function.addNative(self->global, "parseInt", parseInt, 2, 0);
	Function.addNative(self->global, "parseFloat", parseFloat, 1, 0);
	Function.addNative(self->global, "isNaN", isNaN, 1, 0);
	Function.addNative(self->global, "isFinite", isFinite, 1, 0);
	Function.addValue(self->global, "Object", Value.function(Object(constructor)), 0);
	Function.addValue(self->global, "Array", Value.function(Array(constructor)), 0);
	Function.addValue(self->global, "Function", Value.function(Function(constructor)), 0);
	Function.addValue(self->global, "String", Value.function(String(constructor)), 0);
	Function.addValue(self->global, "Number", Value.function(Number(constructor)), 0);
	Function.addValue(self->global, "Boolean", Value.function(Boolean(constructor)), 0);
	Function.addValue(self->global, "RegExp", Value.function(RegExp(constructor)), 0);
	Function.addValue(self->global, "Math", Value.object(Math(object)), 0);
	Function.addValue(self->global, "Error", Value.function(Error(constructor)), 0);
	Function.addValue(self->global, "RangeError", Value.function(Error(rangeConstructor)), 0);
	Function.addValue(self->global, "ReferenceError", Value.function(Error(referenceConstructor)), 0);
	Function.addValue(self->global, "SyntaxError", Value.function(Error(syntaxConstructor)), 0);
	Function.addValue(self->global, "TypeError", Value.function(Error(typeConstructor)), 0);
	Function.addValue(self->global, "URIError", Value.function(Error(uriConstructor)), 0);
	
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
		String.teardown();
		Error.teardown();
		Array.teardown();
		Date.teardown();
		Math.teardown();
		Number.teardown();
		Boolean.teardown();
		RegExp.teardown();
		
		Key.teardown();
		Pool.teardown();
		Env.teardown();
	}
}

void addNative (struct Ecc *self, const char *name, const Native native, int argumentCount, enum Value(Flags) flags)
{
	assert(self);
	
	Function.addNative(self->global, name, native, argumentCount, flags);
}

void addValue (struct Ecc *self, const char *name, struct Value value, enum Value(Flags) flags)
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
	self->result = Value(undefined);
	
//	fprintf(stderr, "--- source:\n%.*s\n", input->length, input->bytes);
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
			struct Text text;
			
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
			
			text = value.type == Value(errorType)? value.data.error->text: ops->text;
			if (text.location == Text(nativeCode).location)
			{
				// show caller's ops for [native code] error
				while (ops->native != Op.call && ops->native != Op.eval  && ops->native != Op.construct && ops > function->oplist->ops)
					--ops;
				
				text = ops->text;
			}
			printTextInput(self, text);
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

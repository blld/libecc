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

static struct Value eval (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value value;
	struct Input *input;
	struct Native(Context) subContext = {
		.parent = context,
		.this = Value.object(&ecc->global->environment),
		.environment =&ecc->global->environment,
	};
	
	Native.assertParameterCount(context, 1);
	
	if (context->construct)
		jmpEnv(ecc, Value.error(Error.typeError(Native.textSeek(context, ecc, Native(thisIndex)), "eval is not a constructor")));
	
	value = Value.toString(Native.argument(context, 0));
	input = Input.createFromBytes(Value.stringChars(value), Value.stringLength(value), "(eval)");
	
	evalInputWithContext(ecc, input, &subContext);
	
	return Value(undefined);
}

static struct Value parseInt (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value value;
	struct Text text;
	int32_t base;
	
	Native.assertParameterCount(context, 2);
	
	value = Value.toString(Native.argument(context, 0));
	base = Value.toInteger(Native.argument(context, 1)).data.integer;
	
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

static struct Value parseFloat (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value value;
	struct Text text;
	
	Native.assertParameterCount(context, 1);
	
	value = Value.toString(Native.argument(context, 0));
	
	text = Text.make(Value.stringChars(value), Value.stringLength(value));
	ecc->result = Lexer.parseBinary(text);
	
	return Value(undefined);
}

static struct Value isFinite (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value value;
	
	Native.assertParameterCount(context, 1);
	
	value = Value.toBinary(Native.argument(context, 0));
	ecc->result = Value.truth(!isnan(value.data.binary) && !isinf(value.data.binary));
	
	return Value(undefined);
}

static struct Value isNaN (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value value;
	
	Native.assertParameterCount(context, 1);
	
	value = Value.toBinary(Native.argument(context, 0));
	ecc->result = Value.truth(isnan(value.data.binary));
	
	return Value(undefined);
}

static struct Value decodeURI (struct Native(Context) * const context, struct Ecc * const ecc)
{
	struct Value value;
	const char *bytes;
	uint16_t index = 0, offset = 0, count;
	struct Chars *chars;
	int c;
	
	Native.assertParameterCount(context, 1);
	
	value = Value.toString(Native.argument(context, 0));
	bytes = Value.stringChars(value);
	count = Value.stringLength(value);
	chars = Chars.createSized(count);
	
	while (index < count)
	{
		c = bytes[index++];
		if (c != '%')
			chars->chars[offset++] = c;
		else if (index + 2 > count || !isxdigit(bytes[index]) || !isxdigit(bytes[index + 1]))
			goto error;
		else
		{
			c = Lexer.uint8Hex(bytes[index], bytes[index + 1]);
			index += 2;
			
			if (c >= 0x80)
			{
				int continuation = (c & 0xf8) == 0xf0? 3: (c & 0xf0) == 0xe0? 2: (c & 0xe0) == 0xc0? 1: 0;
				if (!continuation || index + continuation * 3 > count)
					goto error;
				
				chars->chars[offset++] = c;
				while (continuation--)
				{
					if (bytes[index++] != '%' || !isxdigit(bytes[index]) || !isxdigit(bytes[index + 1]))
						goto error;
					
					chars->chars[offset++] = Lexer.uint8Hex(bytes[index], bytes[index + 1]);
					index += 2;
				}
			}
			else if (strchr(";/?:@&=+$,#", c))
				chars->chars[offset++] = '%', chars->chars[offset++] = bytes[index - 2], chars->chars[offset++] = bytes[index - 1];
			else
				chars->chars[offset++] = c;
		}
	}
	
	chars->length = offset;
	ecc->result = Value.chars(chars);
	return Value(undefined);
	
	error:
	Ecc.jmpEnv(ecc, Value.error(Error.uriError(Native.textSeek(context, ecc, 0), "malformed URI")));
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
	enum Value(Flags) flags = Value(hidden);
	
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
		Arguments.setup();
	}
	
	self = malloc(sizeof(*self));
	*self = Ecc.identity;
	
	self->global = Function.create(Object(prototype));
	self->global->environment.type = &Text(globalType);
	
	Function.addValue(self->global, "NaN", Value.binary(NAN), flags | Value(frozen));
	Function.addValue(self->global, "Infinity", Value.binary(INFINITY), flags | Value(frozen));
	Function.addValue(self->global, "undefined", Value(undefined), flags | Value(frozen));
	Function.addNative(self->global, "eval", eval, 1, flags);
	Function.addNative(self->global, "parseInt", parseInt, 2, flags);
	Function.addNative(self->global, "parseFloat", parseFloat, 1, flags);
	Function.addNative(self->global, "isNaN", isNaN, 1, flags);
	Function.addNative(self->global, "isFinite", isFinite, 1, flags);
	Function.addNative(self->global, "decodeURI", decodeURI, 1, flags);
	#warning decodeURIComponent
	#warning encodeURI
	#warning encodeURIComponent
	Function.addValue(self->global, "Object", Value.function(Object(constructor)), flags);
	Function.addValue(self->global, "Function", Value.function(Function(constructor)), flags);
	Function.addValue(self->global, "Array", Value.function(Array(constructor)), flags);
	Function.addValue(self->global, "String", Value.function(String(constructor)), flags);
	Function.addValue(self->global, "Boolean", Value.function(Boolean(constructor)), flags);
	Function.addValue(self->global, "Number", Value.function(Number(constructor)), flags);
	#warning Date
	Function.addValue(self->global, "RegExp", Value.function(RegExp(constructor)), flags);
	Function.addValue(self->global, "Error", Value.function(Error(constructor)), flags);
	Function.addValue(self->global, "RangeError", Value.function(Error(rangeConstructor)), flags);
	Function.addValue(self->global, "ReferenceError", Value.function(Error(referenceConstructor)), flags);
	Function.addValue(self->global, "SyntaxError", Value.function(Error(syntaxConstructor)), flags);
	Function.addValue(self->global, "TypeError", Value.function(Error(typeConstructor)), flags);
	Function.addValue(self->global, "URIError", Value.function(Error(uriConstructor)), flags);
	Function.addValue(self->global, "Math", Value.object(Math(object)), flags);
	#warning JSON
	
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
		Arguments.teardown();
		
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
		.environment = &self->global->environment
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
			self->result = Value.toPrimitive(NULL, self, self->result, NULL, Value(hintAuto));
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
	
	context->ops->native(context, self);
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

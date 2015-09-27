//
//  function.c
//  libecc
//
//  Created by Bouilland Aurélien on 15/07/2015.
//  Copyright (c) 2015 Libeccio. All rights reserved.
//

#include "function.h"

// MARK: - Private

static struct Object *functionPrototype = NULL;
static struct Function *functionConstructor = NULL;

static struct Value toString (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 0);
	
	if (ecc->this.type != Value(function))
		Ecc.jmpEnv(ecc, Value.error(Error.typeError((*ops)->text, "not a function")));
	
	ecc->result = Value.text(&ecc->this.data.function->text);
	
	return Value.undefined();
}

static struct Value apply (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 2);
	
	if (ecc->this.type != Value(function))
		Ecc.jmpEnv(ecc, Value.error(Error.typeError((*ops)->text, "not a function")));
	
	struct Value this = Op.argument(ecc, 0);
	struct Value arguments = Op.argument(ecc, 1);
	
	if (arguments.type == Value(undefined) || arguments.type == Value(null))
		Op.callFunctionVA(ecc->this.data.function, ecc, this, 0);
	else
	{
		if (!Value.isObject(arguments))
			Ecc.jmpEnv(ecc, Value.error(Error.typeError((*ops)->text, "arguments is not an object")));
		
		Op.callFunctionArguments(ecc->this.data.function, ecc, this, Object.copy(arguments.data.object));
	}
	
	return Value.undefined();
}

static struct Value call (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertVariableParameter(ecc);
	
	if (ecc->this.type != Value(function))
		Ecc.jmpEnv(ecc, Value.error(Error.typeError((*ops)->text, "not a function")));
	
	struct Object *arguments = ecc->context->hashmap[2].data.value.data.object;
	
	if (arguments->elementCount)
	{
		struct Value this = arguments->element[0].data.value;
		
		++arguments->element;
		Op.callFunctionArguments(ecc->this.data.function, ecc, this, arguments);
		--arguments->element;
	}
	else
		Op.callFunctionVA(ecc->this.data.function, ecc, Value.undefined(), 0);
	
	return Value.undefined();
}

static struct Value prototypeFunction (const struct Op ** const ops, struct Ecc * const ecc)
{
	ecc->result = Value.undefined();
	
	return Value.undefined();
}

static struct Value constructorFunction (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertVariableParameter(ecc);
	
	int argumentCount = Op.variableArgumentCount(ecc);
	
	if (argumentCount)
	{
		static const char prefix[] = "(function(";
		
		uint32_t length = 0;
		char *chars = malloc(sizeof(prefix)-1 + (argumentCount == 1? 2: 0));
		memcpy(chars, prefix, sizeof(prefix)-1);
		length += sizeof(prefix)-1;
		
		struct Value value;
		uint32_t valueLength;
		int last;
		
		for (int_fast32_t index = 0; index < argumentCount; ++index)
		{
			last = index == argumentCount - 1;
			if (last)
			{
				chars[length++] = ')';
				chars[length++] = '{';
			}
			
			value = Value.toString(Op.variableArgument(ecc, index));
			valueLength = Value.stringLength(value);
			chars = realloc(chars, sizeof(*chars) + length + valueLength + (index >= argumentCount - 2? 2: 1));
			memcpy(chars + length, Value.stringChars(value), valueLength);
			length += valueLength;
			
			if (index < argumentCount - 2)
				chars[length++] = ',';
		}
		chars[length++] = '}';
		chars[length++] = ')';
		
		Ecc.evalInput(ecc, Input.createFromBytes(chars, length, "(Function)"));
		free(chars), chars = NULL;
	}
	else
	{
		static const char emptyFunction[] = "(function(){})";
		Ecc.evalInput(ecc, Input.createFromBytes(emptyFunction, sizeof(emptyFunction)-1, "(Function)"));
	}
	
	return Value.undefined();
}

// MARK: - Static Members

// MARK: - Methods

void setup ()
{
	functionPrototype = Object.prototype();
	struct Function *functionPrototypeFunction = createWithNative(NULL, prototypeFunction, 0);
	functionPrototype = &functionPrototypeFunction->object;
	Function.addToObject(functionPrototype, "toString", toString, 0, 0);
	Function.addToObject(functionPrototype, "apply", apply, 2, 0);
	Function.addToObject(functionPrototype, "call", call, -1, 0);
	
	functionConstructor = Function.createWithNative(functionPrototype, constructorFunction, -1);
	
	Object.add(functionPrototype, Identifier.constructor(), Value.function(functionConstructor), 0);
	Object.add(&functionConstructor->object, Identifier.prototype(), Value.function(functionPrototypeFunction), 0);
}

void teardown (void)
{
	functionPrototype = NULL;
	functionConstructor = NULL;
}

struct Object *prototype (void)
{
	return functionPrototype;
}

struct Function *constructor (void)
{
	return functionConstructor;
}

Instance create (struct Object *prototype)
{
	return createSized(prototype, 8);
}

Instance createSized (struct Object *prototype, uint32_t size)
{
	Instance self = malloc(sizeof(*self));
	assert(self);
	Pool.addFunction(self);
	
	*self = Module.identity;
	
	Object.initialize(&self->object, functionPrototype);
	Object.initializeSized(&self->context, prototype, size);
	
	self->object.type = Text.functionType();
	
	struct Object *proto = Object.create(Object.prototype());
	Object.add(proto, Identifier.constructor(), Value.function(self), Object(writable) | Object(configurable));
	Object.add(&self->object, Identifier.prototype(), Value.object(proto), Object(writable));
	
	return self;
}

Instance createWithNative (struct Object *prototype, const Native native, int parameterCount)
{
	Instance self = NULL;
	
	if (parameterCount < 0)
	{
		self = createSized(prototype, 3);
		self->needHeap = 1;
		self->needArguments = 1;
		self->object.hashmap[2].data.flags = Object(writable) | Object(isValue);
		self->object.hashmap[2].data.identifier = Identifier.arguments();
	}
	else
	{
		self = createSized(prototype, 3 + parameterCount);
		self->parameterCount = parameterCount;
		
		for (uint_fast16_t index = 0; index < parameterCount; ++index)
			self->object.hashmap[3 + index].data.flags = Object(writable) | Object(isValue);
	}
	self->context.hashmapCount = self->context.hashmapCapacity;
	self->oplist = OpList.create(native, Value.undefined(), *Text.nativeCode());
	self->text = *Text.nativeCode();
	
	Object.add(&self->object, Identifier.length(), Value.integer(abs(parameterCount)), 0);
	
	return self;
}

Instance createWithNativeAccessor (struct Object *prototype, const Native getter, const Native setter)
{
	Instance self = createWithNative(prototype, getter, 0);
	
	self->isAccessor = 1;
	self->pair = createWithNative(prototype, setter, 1);
	
	return self;
}

Instance copy (Instance original)
{
	assert(original);
	
	Instance self = malloc(sizeof(*self));
	assert(self);
	Pool.addObject(&self->object);
	
	*self = *original;
	
	size_t byteSize;
	
	byteSize = sizeof(*self->object.hashmap) * self->object.hashmapCapacity;
	self->object.hashmap = malloc(byteSize);
	memcpy(self->object.hashmap, original->object.hashmap, byteSize);
	
	return self;
}

void destroy (Instance self)
{
	assert(self);
	
	Object.finalize(&self->object);
	Object.finalize(&self->context);
	
	if (self->oplist)
		OpList.destroy(self->oplist), self->oplist = NULL;
	
	free(self), self = NULL;
}

void addValue(Instance self, const char *name, struct Value value, enum Object(Flags) flags)
{
	assert(self);
	
	Object.add(&self->context, Identifier.makeWithCString(name), value, flags);
}

Instance addNative(Instance self, const char *name, const Native native, int parameterCount, enum Object(Flags) flags)
{
	assert(self);
	
	return addToObject(&self->context, name, native, parameterCount, flags);
}

Instance addToObject(struct Object *object, const char *name, const Native native, int parameterCount, enum Object(Flags) flags)
{
	assert(object);
	
	Instance Function = createWithNative(object, native, parameterCount);
	
	Object.add(object, Identifier.makeWithCString(name), Value.function(Function), flags);
	
	return Function;
}

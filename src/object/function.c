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
	struct Value this, arguments;
	
	Op.assertParameterCount(ecc, 2);
	
	if (ecc->this.type != Value(function))
		Ecc.jmpEnv(ecc, Value.error(Error.typeError((*ops)->text, "not a function")));
	
	this = Op.argument(ecc, 0);
	arguments = Op.argument(ecc, 1);
	
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
	struct Object *arguments;
	
	Op.assertVariableParameter(ecc);
	
	if (ecc->this.type != Value(function))
		Ecc.jmpEnv(ecc, Value.error(Error.typeError((*ops)->text, "not a function")));
	
	arguments = ecc->context->hashmap[2].data.value.data.object;
	
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
	int argumentCount;
	
	Op.assertVariableParameter(ecc);
	
	argumentCount = Op.variableArgumentCount(ecc);
	
	if (argumentCount)
	{
		static const char prefix[] = "(function(";
		uint32_t length = 0;
		char *chars = malloc(sizeof(prefix)-1 + (argumentCount == 1? 2: 0));
		struct Value value;
		uint32_t valueLength;
		int last;
		int_fast32_t index;
		
		memcpy(chars, prefix, sizeof(prefix)-1);
		length += sizeof(prefix)-1;
		
		for (index = 0; index < argumentCount; ++index)
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
	struct Function *functionPrototypeFunction;
	
	functionPrototype = Object.prototype();
	functionPrototypeFunction = createWithNative(NULL, prototypeFunction, 0);
	functionPrototype = &functionPrototypeFunction->object;
	Function.addToObject(functionPrototype, "toString", toString, 0, 0);
	Function.addToObject(functionPrototype, "apply", apply, 2, 0);
	Function.addToObject(functionPrototype, "call", call, -1, 0);
	
	functionConstructor = Function.createWithNative(functionPrototype, constructorFunction, -1);
	
	Object.add(functionPrototype, Key(constructor), Value.function(functionConstructor), 0);
	Object.add(&functionConstructor->object, Key(prototype), Value.function(functionPrototypeFunction), 0);
}

void teardown (void)
{
	functionPrototype = NULL;
	functionConstructor = NULL;
}

struct Object * prototype (void)
{
	return functionPrototype;
}

struct Function * constructor (void)
{
	return functionConstructor;
}

struct Function * create (struct Object *prototype)
{
	return createSized(prototype, 8);
}

struct Function * createSized (struct Object *prototype, uint32_t size)
{
	struct Object *proto;
	struct Function *self = malloc(sizeof(*self));
	assert(self);
	Pool.addFunction(self);
	
	*self = Function.identity;
	
	Object.initialize(&self->object, functionPrototype);
	Object.initializeSized(&self->context, prototype, size);
	
	self->object.type = &Text(functionType);
	
	proto = Object.create(Object.prototype());
	Object.add(proto, Key(constructor), Value.function(self), Object(writable) | Object(configurable));
	Object.add(&self->object, Key(prototype), Value.object(proto), Object(writable));
	
	return self;
}

struct Function * createWithNative (struct Object *prototype, const Native native, int parameterCount)
{
	struct Function *self = NULL;
	
	if (parameterCount < 0)
	{
		self = createSized(prototype, 3);
		self->flags |= Function(needHeap) | Function(needArguments);
		self->object.hashmap[2].data.flags = Object(writable) | Object(isValue);
		self->object.hashmap[2].data.key = Key(arguments);
	}
	else
	{
		uint16_t index;
		
		self = createSized(prototype, 3 + parameterCount);
		self->parameterCount = parameterCount;
		
		for (index = 0; index < parameterCount; ++index)
			self->object.hashmap[3 + index].data.flags = Object(writable) | Object(isValue);
	}
	self->context.hashmapCount = self->context.hashmapCapacity;
	self->oplist = OpList.create(native, Value.undefined(), Text(nativeCode));
	self->text = Text(nativeCode);
	
	Object.add(&self->object, Key(length), Value.integer(abs(parameterCount)), 0);
	
	return self;
}

struct Function * createWithNativeAccessor (struct Object *prototype, const Native getter, const Native setter)
{
	struct Function *self = createWithNative(prototype, getter, 0);
	
	self->flags |= Function(isGetter);
	self->pair = createWithNative(prototype, setter, 1);
	
	return self;
}

struct Function * copy (struct Function *original)
{
	struct Function *self = malloc(sizeof(*self));
	size_t byteSize;
	
	assert(self);
	assert(original);
	Pool.addObject(&self->object);
	
	*self = *original;
	
	byteSize = sizeof(*self->object.hashmap) * self->object.hashmapCapacity;
	self->object.hashmap = malloc(byteSize);
	memcpy(self->object.hashmap, original->object.hashmap, byteSize);
	
	return self;
}

void destroy (struct Function *self)
{
	assert(self);
	
	Object.finalize(&self->object);
	Object.finalize(&self->context);
	
	if (self->oplist)
		OpList.destroy(self->oplist), self->oplist = NULL;
	
	free(self), self = NULL;
}

void addValue(struct Function *self, const char *name, struct Value value, enum Object(Flags) flags)
{
	assert(self);
	
	Object.add(&self->context, Key.makeWithCString(name), value, flags);
}

struct Function * addNative(struct Function *self, const char *name, const Native native, int parameterCount, enum Object(Flags) flags)
{
	assert(self);
	
	return addToObject(&self->context, name, native, parameterCount, flags);
}

struct Function * addToObject(struct Object *object, const char *name, const Native native, int parameterCount, enum Object(Flags) flags)
{
	struct Function *function;
	
	assert(object);
	
	function = createWithNative(object, native, parameterCount);
	
	Object.add(object, Key.makeWithCString(name), Value.function(function), flags);
	
	return function;
}

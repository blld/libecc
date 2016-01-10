//
//  function.c
//  libecc
//
//  Created by Bouilland Aurélien on 15/07/2015.
//  Copyright (c) 2015 Libeccio. All rights reserved.
//

#include "function.h"

// MARK: - Private

struct Object * Function(prototype) = NULL;
struct Function * Function(constructor) = NULL;

static struct Value toString (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 0);
	
	if (ecc->this.type != Value(functionType))
		Ecc.jmpEnv(ecc, Value.error(Error.typeError((*ops)->text, "not a function")));
	
	ecc->result = Value.text(&ecc->this.data.function->text);
	
	return Value(undefined);
}

static struct Value apply (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Value this, arguments;
	
	Op.assertParameterCount(ecc, 2);
	
	if (ecc->this.type != Value(functionType))
		Ecc.jmpEnv(ecc, Value.error(Error.typeError((*ops)->text, "not a function")));
	
	this = Op.argument(ecc, 0);
	arguments = Op.argument(ecc, 1);
	
	if (arguments.type == Value(undefinedType) || arguments.type == Value(nullType))
		Op.callFunctionVA(ecc->this.data.function, ecc, this, 0);
	else
	{
		if (!Value.isObject(arguments))
			Ecc.jmpEnv(ecc, Value.error(Error.typeError((*ops)->text, "arguments is not an object")));
		
		Op.callFunctionArguments(ecc->this.data.function, ecc, this, Object.copy(arguments.data.object));
	}
	
	return Value(undefined);
}

static struct Value call (const struct Op ** const ops, struct Ecc * const ecc)
{
	struct Object *arguments;
	
	Op.assertVariableParameter(ecc);
	
	if (ecc->this.type != Value(functionType))
		Ecc.jmpEnv(ecc, Value.error(Error.typeError((*ops)->text, "not a function")));
	
	arguments = ecc->context->hashmap[2].data.value.data.object;
	
	if (arguments->elementCount)
	{
		struct Value this = arguments->element[0].data.value;
		
		--arguments->elementCount;
		++arguments->element;
		Op.callFunctionArguments(ecc->this.data.function, ecc, this, arguments);
		--arguments->element;
		++arguments->elementCount;
	}
	else
		Op.callFunctionVA(ecc->this.data.function, ecc, Value(undefined), 0);
	
	return Value(undefined);
}

static struct Value prototypeFunction (const struct Op ** const ops, struct Ecc * const ecc)
{
	ecc->result = Value(undefined);
	
	return Value(undefined);
}

static struct Value constructorFunction (const struct Op ** const ops, struct Ecc * const ecc)
{
	int argumentCount;
	
	Op.assertVariableParameter(ecc);
	
	argumentCount = Op.variableArgumentCount(ecc);
	
	if (argumentCount)
	{
		static const char prefix[] = "(function(";
		uint16_t length = sizeof(prefix)-1;
		int_fast32_t index;
		
		for (index = 0; index < argumentCount; ++index)
		{
			if (index == argumentCount - 1)
				length++, length++;
			
			length += Value.toBufferLength(Op.variableArgument(ecc, index));
			
			if (index < argumentCount - 2)
				length++;
		}
		length++, length++;
		
		{
			char chars[length];
			uint16_t offset = 0;
			
			memcpy(chars, prefix, sizeof(prefix)-1);
			offset += sizeof(prefix)-1;
			
			for (index = 0; index < argumentCount; ++index)
			{
				if (index == argumentCount - 1)
					chars[offset++] = ')', chars[offset++] = '{';
				
				offset += Value.toBuffer(Op.variableArgument(ecc, index), chars + offset, length - offset);
				
				if (index < argumentCount - 2)
					chars[offset++] = ',';
			}
			chars[offset++] = '}', chars[offset++] = ')';
			Ecc.evalInput(ecc, Input.createFromBytes(chars, length, "(Function)"));
		}
	}
	else
	{
		static const char emptyFunction[] = "(function(){})";
		Ecc.evalInput(ecc, Input.createFromBytes(emptyFunction, sizeof(emptyFunction)-1, "(Function)"));
	}
	
	return Value(undefined);
}

// MARK: - Static Members

// MARK: - Methods

void setup ()
{
	struct Function *functionPrototypeFunction;
	
	functionPrototypeFunction = createWithNative(NULL, prototypeFunction, 0);
	Function(prototype) = &functionPrototypeFunction->object;
	Function(prototype)->type = &Text(functionType);
	Function.addToObject(Function(prototype), "toString", toString, 0, 0);
	Function.addToObject(Function(prototype), "apply", apply, 2, 0);
	Function.addToObject(Function(prototype), "call", call, -1, 0);
	
	Function(constructor) = Function.createWithNative(Function(prototype), constructorFunction, -1);
	
	Object.add(Function(prototype), Key(constructor), Value.function(Function(constructor)), 0);
	Object.add(&Function(constructor)->object, Key(prototype), Value.function(functionPrototypeFunction), 0);
}

void teardown (void)
{
	Function(prototype) = NULL;
	Function(constructor) = NULL;
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
	
	Object.initialize(&self->object, Function(prototype));
	Object.initializeSized(&self->context, prototype, size);
	
	proto = Object.create(Object(prototype));
	Object.add(proto, Key(constructor), Value.function(self), Value(writable) | Value(configurable));
	Object.add(&self->object, Key(prototype), Value.object(proto), Value(writable));
	
	return self;
}

struct Function * createWithNative (struct Object *prototype, const Native native, int parameterCount)
{
	struct Function *self = NULL;
	
	if (parameterCount < 0)
	{
		self = createSized(prototype, 3);
		self->flags |= Function(needArguments);
		self->object.hashmap[2].data.key = Key(arguments);
	}
	else
	{
		self = createSized(prototype, 3 + parameterCount);
		self->parameterCount = parameterCount;
	}
	
	self->context.hashmapCount = self->context.hashmapCapacity;
	self->oplist = OpList.create(native, Value(undefined), Text(nativeCode));
	self->text = Text(nativeCode);
	
	Object.add(&self->object, Key(length), Value.integer(abs(parameterCount)), 0);
	
	return self;
}

struct Function * createPrototypeContructor (struct Object *prototype, const Native native, int parameterCount)
{
	struct Function *constructor = Function.createWithNative(NULL, native, parameterCount);
	
	Object.add(prototype, Key(constructor), Value.function(constructor), 0);
	Object.add(&constructor->object, Key(prototype), Value.object(prototype), 0);
	
	return constructor;
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

void addValue(struct Function *self, const char *name, struct Value value, enum Value(Flags) flags)
{
	assert(self);
	
	Object.add(&self->context, Key.makeWithCString(name), value, flags);
}

struct Function * addNative(struct Function *self, const char *name, const Native native, int parameterCount, enum Value(Flags) flags)
{
	assert(self);
	
	return addToObject(&self->context, name, native, parameterCount, flags);
}

struct Function * addToObject(struct Object *object, const char *name, const Native native, int parameterCount, enum Value(Flags) flags)
{
	struct Function *function;
	
	assert(object);
	
	function = createWithNative(object, native, parameterCount);
	
	Object.add(object, Key.makeWithCString(name), Value.function(function), flags);
	
	return function;
}

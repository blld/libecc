//
//  function.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#include "function.h"

// MARK: - Private

struct Object * Function(prototype) = NULL;
struct Function * Function(constructor) = NULL;

const struct Object(Type) Function(type) = {
	.text = &Text(functionType),
};

static struct Chars * toChars (struct Native(Context) * const context, struct Value value)
{
	struct Function *self;
	struct Chars *chars;
	
	assert(value.type == Value(functionType));
	assert(value.data.function);
	
	self = value.data.function;
	chars = Chars.beginAppend();
	
	if (self->text.bytes == Text(nativeCode).bytes)
		chars = Chars.append(chars, "function %s() [native code]", self->name? self->name: "");
	else
		chars = Chars.append(chars, "%.*s", self->text.length, self->text.bytes);
	
	return Chars.endAppend(chars);
}

static struct Value toString (struct Native(Context) * const context)
{
	Native.assertParameterCount(context, 0);
	
	if (context->this.type != Value(functionType))
		Ecc.jmpEnv(context->ecc, Value.error(Error.typeError(Native.textSeek(context, Native(thisIndex)), "not a function")));
	
	if (context->this.data.function->text.bytes == Text(nativeCode).bytes)
	{
		return Value.chars(toChars(context, context->this));
	}
	else
		return Value.text(&context->this.data.function->text);
}

static struct Value apply (struct Native(Context) * const context)
{
	struct Value this, arguments;
	
	Native.assertParameterCount(context, 2);
	
	if (context->this.type != Value(functionType))
		Ecc.jmpEnv(context->ecc, Value.error(Error.typeError(Native.textSeek(context, Native(thisIndex)), "not a function")));
	
	this = Native.argument(context, 0);
	arguments = Native.argument(context, 1);
	
	if (arguments.type == Value(undefinedType) || arguments.type == Value(nullType))
		return Op.callFunctionVA(context, 2, context->this.data.function, this, 0);
	else
	{
		if (!Value.isObject(arguments))
			Ecc.jmpEnv(context->ecc, Value.error(Error.typeError(Native.textSeek(context, 1), "arguments is not an object")));
		
		return Op.callFunctionArguments(context, 2, context->this.data.function, this, arguments.data.object);
	}
}

static struct Value call (struct Native(Context) * const context)
{
	struct Object *object;
	
	Native.assertVariableParameter(context);
	
	if (context->this.type != Value(functionType))
		Ecc.jmpEnv(context->ecc, Value.error(Error.typeError(Native.textSeek(context, Native(thisIndex)), "not a function")));
	
	object = context->environment->hashmap[2].data.value.data.object;
	
	if (object->elementCount)
	{
		struct Value this = object->element[0].data.value;
		struct Object arguments = *object;
		
		--arguments.elementCapacity;
		--arguments.elementCount;
		++arguments.element;
		if (!arguments.elementCount)
		{
			arguments.element = NULL;
			arguments.elementCapacity = 0;
		}
		
		return Op.callFunctionArguments(context, 1, context->this.data.function, this, &arguments);
	}
	else
		return Op.callFunctionVA(context, 1, context->this.data.function, Value(undefined), 0);
}

static struct Value prototypeConstructor (struct Native(Context) * const context)
{
	return Value(undefined);
}

static struct Value functionConstructor (struct Native(Context) * const context)
{
	int argumentCount;
	
	Native.assertVariableParameter(context);
	
	argumentCount = Native.variableArgumentCount(context);
	
	if (argumentCount)
	{
		int_fast32_t index;
		struct Value value;
		struct Chars *chars = Chars.beginAppend();
		chars = Chars.append(chars, "(function(");
		for (index = 0; index < argumentCount; ++index)
		{
			if (index == argumentCount - 1)
				chars = Chars.append(chars, ") {");
			
			value = Value.toString(context, Native.variableArgument(context, index));
			chars = Chars.append(chars, "%.*s", Value.stringLength(value), Value.stringBytes(value));
			
			if (index < argumentCount - 2)
				chars = Chars.append(chars, ",");
		}
		chars = Chars.append(chars, "})");
		Chars.endAppend(chars);
		Ecc.evalInput(context->ecc, Input.createFromBytes(chars->bytes, chars->length, "(Function)"), 0);
	}
	else
	{
		static const char emptyFunction[] = "(function() {})";
		Ecc.evalInput(context->ecc, Input.createFromBytes(emptyFunction, sizeof(emptyFunction)-1, "(Function)"), 0);
	}
	
	return context->ecc->result;
}

// MARK: - Static Members

// MARK: - Methods

void setup ()
{
	Function.setupBuiltinObject(&Function(constructor), functionConstructor, -1, &Function(prototype), Value.function(createWithNative(prototypeConstructor, 0)), &Function(type));
	
	Function.addToObject(Function(prototype), "toString", toString, 0, 0);
	Function.addToObject(Function(prototype), "apply", apply, 2, 0);
	Function.addToObject(Function(prototype), "call", call, -1, 0);
}

void teardown (void)
{
	Function(prototype) = NULL;
	Function(constructor) = NULL;
}

struct Function * create (struct Object *environment)
{
	return createSized(environment, 8);
}

struct Function * createSized (struct Object *environment, uint32_t size)
{
	struct Function *self = malloc(sizeof(*self));
	assert(self);
	Pool.addFunction(self);
	
	*self = Function.identity;
	
	Object.initialize(&self->object, Function(prototype));
	Object.initializeSized(&self->environment, environment, size);
	
	return self;
}

struct Function * createWithNative (const Native(Function) native, int parameterCount)
{
	struct Function *self = NULL;
	
	if (parameterCount < 0)
	{
		self = createSized(NULL, 3);
		self->flags |= Function(needArguments);
		self->object.hashmap[2].data.key = Key(arguments);
	}
	else
	{
		self = createSized(NULL, 3 + parameterCount);
		self->parameterCount = parameterCount;
	}
	
	self->environment.hashmapCount = self->environment.hashmapCapacity;
	self->oplist = OpList.create(native, Value(undefined), Text(nativeCode));
	self->text = Text(nativeCode);
	
	Object.add(&self->object, Key(length), Value.integer(abs(parameterCount)), 0);
	
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
	Object.finalize(&self->environment);
	
	if (self->oplist)
		OpList.destroy(self->oplist), self->oplist = NULL;
	
	free(self), self = NULL;
}

void addValue(struct Function *self, const char *name, struct Value value, enum Value(Flags) flags)
{
	assert(self);
	
	if (value.type == Value(functionType))
		value.data.function->name = name;
	
	Object.add(&self->environment, Key.makeWithCString(name), value, flags);
}

struct Function * addNative(struct Function *self, const char *name, const Native(Function) native, int parameterCount, enum Value(Flags) flags)
{
	assert(self);
	
	return addToObject(&self->environment, name, native, parameterCount, flags);
}

struct Function * addToObject(struct Object *object, const char *name, const Native(Function) native, int parameterCount, enum Value(Flags) flags)
{
	struct Function *function;
	
	assert(object);
	
	function = createWithNative(native, parameterCount);
	function->name = name;
	
	Object.add(object, Key.makeWithCString(name), Value.function(function), flags);
	
	return function;
}

void linkPrototype (struct Function *self, struct Value prototype)
{
	assert(self);
	
	Object.add(prototype.data.object, Key(constructor), Value.function(self), Value(hidden));
	Object.add(&self->object, Key(prototype), prototype, Value(hidden) | Value(frozen));
}

void setupBuiltinObject (struct Function **constructor, const Native(Function) native, int parameterCount, struct Object **prototype, struct Value prototypeValue, const struct Object(Type) *type)
{
	struct Function *function = createWithNative(native, parameterCount);
	
	if (prototype)
	{
		struct Object *object = prototypeValue.data.object;
		object->type = type;
		
		if (!object->prototype)
			object->prototype = Object(prototype);
		
		*prototype = object;
	}
	
	linkPrototype(function, prototypeValue);
	*constructor = function;
}

struct Value accessor (const Native(Function) getter, const Native(Function) setter)
{
	struct Value value;
	struct Function *getterFunction = NULL, *setterFunction = NULL;
	if (setter)
		setterFunction = Function.createWithNative(setter, 1);
	
	if (getter)
	{
		getterFunction = Function.createWithNative(getter, 0);
		getterFunction->pair = setterFunction;
		value = Value.function(getterFunction);
		value.flags |= Value(getter);
	}
	else if (setter)
	{
		value = Value.function(setterFunction);
		value.flags |= Value(setter);
	}
	
	return value;
}

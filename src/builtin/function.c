//
//  function.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#define Implementation
#include "function.h"

#include "../ecc.h"
#include "../oplist.h"
#include "../pool.h"

// MARK: - Private

struct Object * Function(prototype) = NULL;
struct Function * Function(constructor) = NULL;

static
void mark (struct Object *object)
{
	struct Function *self = (struct Function *)object;
	
	Pool.markObject(&self->object);
	Pool.markObject(&self->environment);
	
	if (self->pair)
		Pool.markObject(&self->pair->object);
}

const struct Object(Type) Function(type) = {
	.text = &Text(functionType),
	.mark = mark,
	/* XXX: don't finalize */
};

static struct Chars * toChars (struct Context * const context, struct Value value)
{
	struct Function *self;
	struct Chars *chars;
	
	assert(value.type == Value(functionType));
	assert(value.data.function);
	
	self = value.data.function;
	Chars.beginAppend(&chars);
	
	if (self->text.bytes == Text(nativeCode).bytes)
		Chars.append(&chars, "function %s() [native code]", self->name? self->name: "");
	else
		Chars.append(&chars, "%.*s", self->text.length, self->text.bytes);
	
	return Chars.endAppend(&chars);
}

static struct Value toString (struct Context * const context)
{
	Context.assertParameterCount(context, 0);
	Context.assertThisType(context, Value(functionType));
	
	if (context->this.data.function->text.bytes == Text(nativeCode).bytes)
	{
		return Value.chars(toChars(context, context->this));
	}
	else
		return Value.text(&context->this.data.function->text);
}

static struct Value apply (struct Context * const context)
{
	struct Value this, arguments;
	
	Context.assertParameterCount(context, 2);
	Context.assertThisType(context, Value(functionType));
	
	this = Context.argument(context, 0);
	if (this.type != Value(undefinedType) && this.type != Value(nullType))
		this = Value.toObject(context, this);
	
	arguments = Context.argument(context, 1);
	
	if (arguments.type == Value(undefinedType) || arguments.type == Value(nullType))
		return Op.callFunctionVA(context, Context(applyOffset), context->this.data.function, this, 0, NULL);
	else
	{
		if (!Value.isObject(arguments))
			Context.typeError(context, Chars.create("arguments is not an object"));
		
		return Op.callFunctionArguments(context, Context(applyOffset), context->this.data.function, this, arguments.data.object);
	}
}

static struct Value call (struct Context * const context)
{
	struct Object arguments;
	
	Context.assertVariableParameter(context);
	Context.assertThisType(context, Value(functionType));
	
	arguments = *context->environment->hashmap[2].value.data.object;
	
	if (arguments.elementCount)
	{
		struct Value this = Context.variableArgument(context, 0);
		if (this.type != Value(undefinedType) && this.type != Value(nullType))
			this = Value.toObject(context, this);
		
		--arguments.elementCapacity;
		--arguments.elementCount;
		++arguments.element;
		if (!arguments.elementCount)
		{
			arguments.element = NULL;
			arguments.elementCapacity = 0;
		}
		
		return Op.callFunctionArguments(context, Context(callOffset), context->this.data.function, this, &arguments);
	}
	else
		return Op.callFunctionVA(context, Context(callOffset), context->this.data.function, Value(undefined), 0, NULL);
}

static struct Value bindCall (struct Context * const context)
{
	struct Function *function;
	struct Object *arguments;
	uint16_t count, length;
	
	Context.assertVariableParameter(context);
	Context.assertThisType(context, Value(functionType));
	
	function = context->this.data.function;
	
	count = Context.variableArgumentCount(context);
	length = (function->environment.elementCount - 1) + count;
	arguments = Array.createSized(length);
	
	memcpy(arguments->element, function->environment.element + 1, sizeof(*arguments->element) * (function->environment.elementCount - 1));
	memcpy(arguments->element + (function->environment.elementCount - 1), context->environment->hashmap[2].value.data.object->element, sizeof(*arguments->element) * (context->environment->hashmap[2].value.data.object->elementCount));
	
	return Op.callFunctionArguments(context, 0, context->this.data.function->pair, function->environment.element[0].value, arguments);
}

static struct Value bind (struct Context * const context)
{
	struct Function *function;
	uint16_t index, count;
	int parameterCount = 0;
	
	Context.assertVariableParameter(context);
	Context.assertThisType(context, Value(functionType));
	
	count = Context.variableArgumentCount(context);
	parameterCount = context->this.data.function->parameterCount - (count > 1? count - 1: 0);
	function = createWithNative(bindCall, parameterCount > 0? parameterCount: 0);
	
	Object.resizeElement(&function->environment, count? count: 1);
	if (count)
		for (index = 0; index < count; ++index)
			function->environment.element[index].value = Context.variableArgument(context, index);
	else
		function->environment.element[0].value = Value(undefined);
	
	function->pair = context->this.data.function;
	function->boundThis = Value.function(function);
	function->flags |= Function(needArguments) | Function(useBoundThis);
	
	return Value.function(function);
}

static struct Value prototypeConstructor (struct Context * const context)
{
	return Value(undefined);
}

static struct Value constructor (struct Context * const context)
{
	int argumentCount;
	
	Context.assertVariableParameter(context);
	
	argumentCount = Context.variableArgumentCount(context);
	
	if (argumentCount)
	{
		int_fast32_t index;
		struct Value value;
		struct Chars *chars;
		struct Input *input;
		struct Context subContext = {
			.parent = context->parent,
			.this = context->parent->this,
			.environment = context->parent->environment,
			.ecc = context->ecc,
			.depth = context->depth + 1,
		};
		
		Chars.beginAppend(&chars);
		Chars.append(&chars, "(function(");
		for (index = 0; index < argumentCount; ++index)
		{
			if (index == argumentCount - 1)
				Chars.append(&chars, ") {");
			
			value = Value.toString(context, Context.variableArgument(context, index));
			Chars.append(&chars, "%.*s", Value.stringLength(value), Value.stringBytes(value));
			
			if (index < argumentCount - 2)
				Chars.append(&chars, ",");
		}
		Chars.append(&chars, "})");
		
		input = Input.createFromBytes(chars->bytes, chars->length, "(Function)");
		Ecc.evalInputWithContext(context->ecc, input, &subContext);
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
	const enum Value(Flags) h = Value(hidden);
	
	Function.setupBuiltinObject(
		&Function(constructor), constructor, -1,
		&Function(prototype), Value.function(createWithNative(prototypeConstructor, 0)),
		&Function(type));
	
	Function(constructor)->object.prototype = Function(prototype);
	
	Function.addToObject(Function(prototype), "toString", toString, 0, h);
	Function.addToObject(Function(prototype), "apply", apply, 2, h);
	Function.addToObject(Function(prototype), "call", call, -1, h);
	Function.addToObject(Function(prototype), "bind", bind, -1, h);
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
	}
	else
	{
		self = createSized(NULL, 3 + parameterCount);
		self->parameterCount = parameterCount;
	}
	
	self->environment.hashmapCount = self->environment.hashmapCapacity;
	self->oplist = OpList.create(native, Value(undefined), Text(nativeCode));
	self->text = Text(nativeCode);
	
	Object.addMember(&self->object, Key(length), Value.integer(abs(parameterCount)), Value(readonly) | Value(hidden) | Value(sealed));
	
	return self;
}

struct Function * copy (struct Function *original)
{
	struct Function *self = malloc(sizeof(*self));
	size_t byteSize;
	
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

void addMember(struct Function *self, const char *name, struct Value value, enum Value(Flags) flags)
{
	assert(self);
	
	if (value.type == Value(functionType))
		value.data.function->name = name;
	
	Object.addMember(&self->object, Key.makeWithCString(name), value, flags);
}

struct Function * addMethod(struct Function *self, const char *name, const Native(Function) native, int parameterCount, enum Value(Flags) flags)
{
	assert(self);
	
	return addToObject(&self->object, name, native, parameterCount, flags);
}

void addValue(struct Function *self, const char *name, struct Value value, enum Value(Flags) flags)
{
	assert(self);
	
	if (value.type == Value(functionType))
		value.data.function->name = name;
	
	Object.addMember(&self->environment, Key.makeWithCString(name), value, flags);
}

struct Function * addFunction(struct Function *self, const char *name, const Native(Function) native, int parameterCount, enum Value(Flags) flags)
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
	
	Object.addMember(object, Key.makeWithCString(name), Value.function(function), flags);
	
	return function;
}

void linkPrototype (struct Function *self, struct Value prototype, enum Value(Flags) flags)
{
	assert(self);
	
	Object.addMember(prototype.data.object, Key(constructor), Value.function(self), Value(hidden));
	Object.addMember(&self->object, Key(prototype), prototype, flags);
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
	
	linkPrototype(function, prototypeValue, Value(readonly) | Value(hidden) | Value(sealed));
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
	else
		value = Value(undefined);
	
	return value;
}

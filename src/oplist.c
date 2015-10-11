//
//  oplist.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#include "oplist.h"

// MARK: - Private

// MARK: - Static Members

// MARK: - Methods

struct OpList * create (const Native native, struct Value value, struct Text text)
{
	struct OpList *self = malloc(sizeof(*self) + sizeof(*self->ops) * 1);
	assert(self);
	self->ops[0] = Op.make(native, value, text);
	self->opCount = 1;
	return self;
}

void destroy (struct OpList * self)
{
	assert(self);
	
//	while (self->opCount--)
//		Value.finalize(&self->ops[self->opCount].value);
	
	free(self), self = NULL;
}

struct OpList * join (struct OpList *self, struct OpList *with)
{
	if (!self)
		return with;
	else if (!with)
		return self;
	
	self = realloc(self, sizeof(*self) + sizeof(*self->ops) * (self->opCount + with->opCount));
	memcpy(self->ops + self->opCount, with->ops, sizeof(*self->ops) * with->opCount);
	self->opCount += with->opCount;
	free(with), with = NULL;
	
	return self;
}

struct OpList * unshift (struct Op op, struct OpList *self)
{
	if (!self)
		return create(op.native, op.value, op.text);
	
	self = realloc(self, sizeof(*self) + sizeof(*self->ops) * (self->opCount + 1));
	memmove(self->ops + 1, self->ops, sizeof(*self->ops) * self->opCount++);
	self->ops[0] = op;
	return self;
}

struct OpList * append (struct OpList *self, struct Op op)
{
	if (!self)
		return create(op.native, op.value, op.text);
	
	self = realloc(self, sizeof(*self) + sizeof(*self->ops) * (self->opCount + 1));
	self->ops[self->opCount++] = op;
	return self;
}

struct OpList * appendNoop (struct OpList *self)
{
	return append(self, Op.make(Op.noop, Value.undefined(), Text.make(NULL, 0)));
}

struct OpList * createLoop (struct OpList * initial, struct OpList * condition, struct OpList * step, struct OpList * body, int reverseCondition)
{
	if (condition && step && condition->opCount == 3 && !reverseCondition)
	{
		if (condition->ops[1].native == Op.getLocal && (
			condition->ops[0].native == Op.less ||
			condition->ops[0].native == Op.lessOrEqual ))
			if (step->opCount >= 2 && step->ops[1].value.data.identifier.data.integer == condition->ops[1].value.data.identifier.data.integer)
			{
				struct Value stepValue;
				if (step->opCount == 2 && (step->ops[0].native == Op.incrementRef || step->ops[0].native == Op.postIncrementRef))
					stepValue = Value.integer(1);
				else if (step->opCount == 3 && step->ops[0].native == Op.addAssignRef && step->ops[2].native == Op.value && step->ops[2].value.type == Value(integer) && step->ops[2].value.data.integer > 0)
					stepValue = step->ops[2].value;
				else
					goto normal;
				
				if (condition->ops[2].native == Op.getLocal)
					body = OpList.unshift(Op.make(Op.getLocalRef, condition->ops[2].value, condition->ops[2].text), body);
				else if (condition->ops[2].native == Op.value)
					body = OpList.unshift(Op.make(Op.valueConstRef, condition->ops[2].value, condition->ops[2].text), body);
				else
					goto normal;
				
				body = OpList.unshift(Op.make(Op.getLocalRef, condition->ops[1].value, condition->ops[1].text), body);
				body = OpList.unshift(Op.make(Op.value, stepValue, condition->ops[0].text), body);
				body = OpList.unshift(Op.make(condition->ops[0].native == Op.less? Op.iterateLessRef: Op.iterateLessOrEqualRef, Value.integer(body->opCount), condition->ops[0].text), body);
				OpList.destroy(condition), condition = NULL;
				OpList.destroy(step), step = NULL;
				return OpList.join(initial, body);
			}
		
		if (condition->ops[1].native == Op.getLocal && (
			condition->ops[0].native == Op.more ||
			condition->ops[0].native == Op.moreOrEqual ))
			if (step->opCount >= 2 && step->ops[1].value.data.identifier.data.integer == condition->ops[1].value.data.identifier.data.integer)
			{
				struct Value stepValue;
				if (step->opCount == 2 && (step->ops[0].native == Op.decrementRef || step->ops[0].native == Op.postDecrementRef))
					stepValue = Value.integer(1);
				else if (step->opCount == 3 && step->ops[0].native == Op.minusAssignRef && step->ops[2].native == Op.value && step->ops[2].value.type == Value(integer) && step->ops[2].value.data.integer > 0)
					stepValue = step->ops[2].value;
				else
					goto normal;
				
				if (condition->ops[2].native == Op.getLocal)
					body = OpList.unshift(Op.make(Op.getLocalRef, condition->ops[2].value, condition->ops[2].text), body);
				else if (condition->ops[2].native == Op.value)
					body = OpList.unshift(Op.make(Op.valueConstRef, condition->ops[2].value, condition->ops[2].text), body);
				else
					goto normal;
				
				body = OpList.unshift(Op.make(Op.getLocalRef, condition->ops[1].value, condition->ops[1].text), body);
				body = OpList.unshift(Op.make(Op.value, stepValue, condition->ops[0].text), body);
				body = OpList.unshift(Op.make(condition->ops[0].native == Op.more? Op.iterateMoreRef: Op.iterateMoreOrEqualRef, Value.integer(body->opCount), condition->ops[0].text), body);
				OpList.destroy(condition), condition = NULL;
				OpList.destroy(step), step = NULL;
				return OpList.join(initial, body);
			}
	}
	
normal:
	{
		int skipOpCount;
		
		if (step && step->ops[0].native != Op.discard)
			step = OpList.unshift(Op.make(Op.discard, Value.undefined(), text(step)), step);
		
		if (!condition)
		 condition = OpList.create(Op.value, Value.boolean(1), OpList.text(body));
		
		skipOpCount = reverseCondition? condition->opCount - 1: 0;
		body = OpList.appendNoop(OpList.join(condition, body));
		step = OpList.unshift(Op.make(Op.jump, Value.integer(body->opCount), text(step)), step);
		skipOpCount += step->opCount;
		initial = OpList.append(initial, Op.make(Op.iterate, Value.integer(skipOpCount), text(initial)));
		return OpList.join(OpList.join(initial, step), body);
	}
}

void optimizeWithContext (struct OpList *self, struct Object *context)
{
	uint32_t index, count, slotIndex, slotCount;
	
	Object.packValue(context);
	if (!self)
		return;
	
	for (index = 0, count = self->opCount; index < count; ++index)
	{
		if (self->ops[index].native == Op.try)
		{
			// skip try/catch
			index += self->ops[index].value.data.integer;
			index += self->ops[index].value.data.integer;
		}
		else if (self->ops[index].native == Op.getLocal || self->ops[index].native == Op.getLocalRef || self->ops[index].native == Op.setLocal)
		{
			for (slotIndex = 0, slotCount = context->hashmapCount; slotIndex < slotCount; ++slotIndex)
			{
				if (context->hashmap[slotIndex].data.flags & Object(isValue))
				{
					if (Identifier.isEqual(context->hashmap[slotIndex].data.identifier, self->ops[index].value.data.identifier))
					{
						self->ops[index] = Op.make(
							self->ops[index].native == Op.getLocal? Op.getLocalSlot:
							self->ops[index].native == Op.getLocalRef? Op.getLocalSlotRef:
							self->ops[index].native == Op.setLocal? Op.setLocalSlot: NULL
							, Value.integer(slotIndex), self->ops[index].text);
					}
				}
			}
		}
	}
}

void dumpTo (struct OpList *self, FILE *file)
{
	int i;
	
	assert(self);
	
	fprintf(file, "--\n");
	if (!self)
		return;
	
	for (i = 0; i < self->opCount; ++i)
	{
		fprintf(file, "[%03d] %s ", i, Op.toChars(self->ops[i].native));
		
		if (self->ops[i].value.type != Value(undefined) || self->ops[i].native == Op.value || self->ops[i].native == Op.exchange)
			Value.dumpTo(self->ops[i].value, file);
		
		if (self->ops[i].native == Op.text)
			fprintf(file, "'%.*s'\n", self->ops[i].text.length, self->ops[i].text.location);
		else
			fprintf(file, "\n");
	}
}

struct Text text (struct OpList *oplist)
{
	if (!oplist)
		return Text.make(NULL, 0);
	
	return Text.make(
		oplist->ops[0].text.location,
		oplist->ops[oplist->opCount - 1].text.location - oplist->ops[0].text.location + oplist->ops[oplist->opCount - 1].text.length);
}

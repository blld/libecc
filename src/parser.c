//
//  parser.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#include "parser.h"

// MARK: - Private

static void changeNative (struct Op *op, const Native native)
{
	*op = Op.make(native, op->value, op->text);
}


// MARK: - Static Members

static struct OpList * new (struct Parser *);
static struct OpList * assignment (struct Parser *, int noIn);
static struct OpList * expression (struct Parser *, int noIn);
static struct OpList * statement (struct Parser *);
static struct OpList * function (struct Parser *, int isDeclaration, int isGetter, int isSetter);
static struct OpList * sourceElements (struct Parser *, enum Lexer(Token) endToken);


// MARK: Token

static inline enum Lexer(Token) previewToken (struct Parser *self)
{
	return self->previewToken;
}

static void error (struct Parser *self, struct Error *error)
{
	if (!self->error)
	{
		self->error = error;
		self->previewToken = Lexer(errorToken);
	}
//	else
//		Error.destroy(error);
}

static inline enum Lexer(Token) nextToken (struct Parser *self)
{
	if (self->previewToken != Lexer(errorToken))
	{
		self->previewToken = Lexer.nextToken(self->lexer);
		
		if (self->previewToken == Lexer(errorToken))
			self->error = self->lexer->value.data.error;
	}
	return self->previewToken;
}

static inline int acceptToken (struct Parser *self, enum Lexer(Token) token)
{
	if (previewToken(self) != token)
		return 0;
	
	nextToken(self);
	return 1;
}

static inline int expectToken (struct Parser *self, enum Lexer(Token) token)
{
	if (previewToken(self) != token)
	{
		if (token && token < Lexer(errorToken))
			error(self, Error.syntaxError(self->lexer->text, "expected '%c', got %s", token, Lexer.tokenChars(previewToken(self))));
		else
			error(self, Error.syntaxError(self->lexer->text, "expected %s, got %s", Lexer.tokenChars(token), Lexer.tokenChars(previewToken(self))));
		
		return 0;
	}
	
	nextToken(self);
	return 1;
}


// MARK: Depth

static void pushDepth (struct Parser *self, struct Key key, char depth)
{
	self->depths = realloc(self->depths, (self->depthCount + 1) * sizeof(*self->depths));
	self->depths[self->depthCount].key = key;
	self->depths[self->depthCount].depth = depth;
	++self->depthCount;
}

static void popDepth (struct Parser *self)
{
	--self->depthCount;
}


// MARK: Expression

static struct OpList * expressionRef (struct Parser *self, struct OpList *oplist, const char *name)
{
	if (oplist->ops[0].native == Op.getLocal && oplist->opCount == 1)
	{
		if (oplist->ops[0].value.type == Value(key))
		{
			if (Key.isEqual(oplist->ops[0].value.data.key, Key(eval)))
				error(self, Error.syntaxError(OpList.text(oplist), name));
			else if (Key.isEqual(oplist->ops[0].value.data.key, Key(arguments)))
				error(self, Error.syntaxError(OpList.text(oplist), name));
		}
		
		oplist->ops[0].native = Op.getLocalRef;
	}
	else if (oplist->ops[0].native == Op.getMember)
		oplist->ops[0].native = Op.getMemberRef;
	else if (oplist->ops[0].native == Op.getProperty)
		oplist->ops[0].native = Op.getPropertyRef;
	else
		error(self, Error.referenceError(OpList.text(oplist), "%s", name));
	
	return oplist;
}

static void semicolon (struct Parser *self)
{
	if (previewToken(self) == ';')
	{
		nextToken(self);
		return;
	}
	else if (self->lexer->didLineBreak || previewToken(self) == '}' || previewToken(self) == Lexer(noToken))
		return;
	
	error(self, Error.syntaxError(self->lexer->text, "missing ; before statement"));
}

static struct Op identifier (struct Parser *self)
{
	struct Value value = self->lexer->value;
	struct Text text = self->lexer->text;
	if (!expectToken(self, Lexer(identifierToken)))
		return (struct Op){ 0 };
	
	return Op.make(Op.value, value, text);
}

static struct OpList * array (struct Parser *self)
{
	struct OpList *oplist = NULL;
	uint32_t count = 0;
	do
	{
		while (previewToken(self) == ',')
		{
			++count;
			oplist = OpList.append(oplist, Op.make(Op.value, Value.breaker(0), self->lexer->text));
			nextToken(self);
		}
		
		if (previewToken(self) == ']')
			break;
		
		++count;
		oplist = OpList.join(oplist, assignment(self, 0));
	}
	while (acceptToken(self, ','));
	return OpList.unshift(Op.make(Op.array, Value.integer(count), OpList.text(oplist)), oplist);
}

static struct OpList * propertyAssignment (struct Parser *self)
{
	struct OpList *oplist = NULL;
	int isGetter = 0, isSetter = 0;
	
	if (previewToken(self) == Lexer(identifierToken))
	{
		if (Key.isEqual(self->lexer->value.data.key, Key(get)))
		{
			nextToken(self);
			if (previewToken(self) == ':')
			{
				oplist = OpList.create(Op.value, Value.key(Key(get)), self->lexer->text);
				goto skipProperty;
			}
			else
				isGetter = 1;
		}
		else if (Key.isEqual(self->lexer->value.data.key, Key(set)))
		{
			nextToken(self);
			if (previewToken(self) == ':')
			{
				oplist = OpList.create(Op.value, Value.key(Key(set)), self->lexer->text);
				goto skipProperty;
			}
			else
				isSetter = 1;
		}
	}
	
	if (previewToken(self) == Lexer(integerToken))
		oplist = OpList.create(Op.value, self->lexer->value, self->lexer->text);
	else if (previewToken(self) == Lexer(binaryToken))
	{
		Input.printText(self->lexer->input, self->lexer->text);
		Env.printWarning("Using floating-point as property name polute identifier's pool");
		oplist = OpList.create(Op.value, Value.key(Key.makeWithText(self->lexer->text, 0)), self->lexer->text);
	}
	else if (previewToken(self) == Lexer(stringToken))
	{
		int32_t element = Lexer.parseElement(self->lexer->text);
		if (element >= 0)
			oplist = OpList.create(Op.value, Value.integer(element), self->lexer->text);
		else
			oplist = OpList.create(Op.value, Value.key(Key.makeWithText(self->lexer->text, 0)), self->lexer->text);
	}
	else if (previewToken(self) == Lexer(identifierToken))
	{
		if (Key.isEqual(self->lexer->value.data.key, Key(eval)))
			error(self, Error.syntaxError(self->lexer->text, "eval in object identifier"));
		else if (Key.isEqual(self->lexer->value.data.key, Key(arguments)))
			error(self, Error.syntaxError(self->lexer->text, "arguments in object identifier"));
		else
			oplist = OpList.create(Op.value, self->lexer->value, self->lexer->text);
	}
	else
	{
		expectToken(self, Lexer(identifierToken));
		return NULL;
	}
	
	nextToken(self);
	
	if (isGetter)
		return OpList.join(oplist, function(self, 0, 1, 0));
	else if (isSetter)
		return OpList.join(oplist, function(self, 0, 0, 1));
	
	skipProperty:
	expectToken(self, ':');
	return OpList.join(oplist, assignment(self, 0));
}

static struct OpList * object (struct Parser *self)
{
	struct OpList *oplist = NULL;
	uint32_t count = 0;
	do
	{
		if (previewToken(self) == '}')
			break;
		
		++count;
		oplist = OpList.join(oplist, propertyAssignment(self));
	}
	while (acceptToken(self, ','));
	return OpList.unshift(Op.make(Op.object, Value.integer(count), OpList.text(oplist)), oplist);
}

static struct OpList * primary (struct Parser *self)
{
	struct OpList *oplist = NULL;
	
	if (previewToken(self) == Lexer(identifierToken))
	{
		oplist = OpList.create(Op.getLocal, self->lexer->value, self->lexer->text);
		
		if (Key.isEqual(self->lexer->value.data.key, Key(arguments)))
			self->function->flags |= Function(needArguments) | Function(needHeap);
	}
	else if (previewToken(self) == Lexer(stringToken))
		oplist = OpList.create(Op.text, Value.undefined(), self->lexer->text);
	else if (previewToken(self) == Lexer(regexToken)
		|| previewToken(self) == Lexer(integerToken)
		|| previewToken(self) == Lexer(binaryToken)
		)
		oplist = OpList.create(Op.value, self->lexer->value, self->lexer->text);
	else if (previewToken(self) == Lexer(thisToken))
		oplist = OpList.create(Op.this, Value.undefined(), self->lexer->text);
	else if (previewToken(self) == Lexer(nullToken))
		oplist = OpList.create(Op.value, Value.null(), self->lexer->text);
	else if (previewToken(self) == Lexer(trueToken))
		oplist = OpList.create(Op.value, Value.truth(1), self->lexer->text);
	else if (previewToken(self) == Lexer(falseToken))
		oplist = OpList.create(Op.value, Value.truth(0), self->lexer->text);
	else if (acceptToken(self, '{'))
	{
		oplist = object(self);
		expectToken(self, '}');
		return oplist;
	}
	else if (acceptToken(self, '['))
	{
		oplist = array(self);
		expectToken(self, ']');
		return oplist;
	}
	else if (acceptToken(self, '('))
	{
		oplist = expression(self, 0);
		expectToken(self, ')');
		return oplist;
	}
	else
		return NULL;
	
	nextToken(self);
	return oplist;
}

static struct OpList * arguments (struct Parser *self, int *count)
{
	struct OpList *oplist = NULL;
	*count = 0;
	if (previewToken(self) != ')')
		do
		{
			++*count;
			oplist = OpList.join(oplist, assignment(self, 0));
		} while (acceptToken(self, ','));
	
	return oplist;
}

static struct OpList * member (struct Parser *self)
{
	struct OpList *oplist = new(self);
	struct Text text = OpList.text(oplist);
	while (1)
	{
		if (acceptToken(self, '.'))
		{
			struct Value value = self->lexer->value;
			text = Text.join(OpList.text(oplist), self->lexer->text);
			if (!expectToken(self, Lexer(identifierToken)))
				return oplist;
			
			oplist = OpList.unshift(Op.make(Op.getMember, value, text), oplist);
		}
		else if (acceptToken(self, '['))
		{
			oplist = OpList.join(oplist, expression(self, 0));
			text = Text.join(OpList.text(oplist), self->lexer->text);
			if (!expectToken(self, ']'))
				return oplist;
			
			oplist = OpList.unshift(Op.make(Op.getProperty, Value.undefined(), text), oplist);
		}
		else
			break;
	}
	return oplist;
}

static struct OpList * new (struct Parser *self)
{
	struct OpList *oplist = NULL;
	
	if (acceptToken(self, Lexer(newToken)))
	{
		int count = 0;
		oplist = member(self);
		if (acceptToken(self, '('))
		{
			oplist = OpList.join(oplist, arguments(self, &count));
			expectToken(self, ')');
		}
		return OpList.unshift(Op.make(Op.construct, Value.integer(count), OpList.text(oplist)), oplist);
	}
	else if (previewToken(self) == Lexer(functionToken))
		return function(self, 0, 0, 0);
	else
		return primary(self);
}

static struct OpList * leftHandSide (struct Parser *self)
{
	struct OpList *oplist = new(self);
	struct Text text = OpList.text(oplist);
	while (1)
	{
		if (previewToken(self) == '.')
		{
			if (!oplist)
				error(self, Error.syntaxError(self->lexer->text, "expected expression, got '%.*s'", self->lexer->text.length, self->lexer->text.location));
			
			nextToken(self);
			
			struct Value value = self->lexer->value;
			text = Text.join(OpList.text(oplist), self->lexer->text);
			if (!expectToken(self, Lexer(identifierToken)))
				return oplist;
			
			oplist = OpList.unshift(Op.make(Op.getMember, value, text), oplist);
		}
		else if (acceptToken(self, '['))
		{
			oplist = OpList.join(oplist, expression(self, 0));
			text = Text.join(OpList.text(oplist), self->lexer->text);
			if (!expectToken(self, ']'))
				return oplist;
			
			oplist = OpList.unshift(Op.make(Op.getProperty, Value.undefined(), text), oplist);
		}
		else if (acceptToken(self, '('))
		{
			int count = 0;
			
			int isEval = oplist->opCount == 1 && oplist->ops[0].native == Op.getLocal && Key.isEqual(oplist->ops[0].value.data.key, Key(eval));
			if (isEval)
				OpList.destroy(oplist), oplist = NULL;
			
			oplist = OpList.join(oplist, arguments(self, &count));
			text = Text.join(OpList.text(oplist), self->lexer->text);
			
			if (isEval)
				oplist = OpList.unshift(Op.make(Op.eval, Value.integer(count), text), oplist);
			else
				oplist = OpList.unshift(Op.make(Op.call, Value.integer(count), text), oplist);
			
			if (!expectToken(self, ')'))
				return oplist;
		}
		else
			break;
	}
	return oplist;
}

static struct OpList * postfix (struct Parser *self)
{
	struct OpList *oplist = leftHandSide(self);
	
	if (!self->lexer->didLineBreak && acceptToken(self, Lexer(incrementToken)))
		oplist = OpList.unshift(Op.make(Op.postIncrementRef, Value.undefined(), self->lexer->text), expressionRef(self, oplist, "invalid increment operand"));
	if (!self->lexer->didLineBreak && acceptToken(self, Lexer(decrementToken)))
		oplist = OpList.unshift(Op.make(Op.postDecrementRef, Value.undefined(), self->lexer->text), expressionRef(self, oplist, "invalid decrement operand"));
	
	return oplist;
}

static struct OpList * unary (struct Parser *self)
{
	if (acceptToken(self, Lexer(deleteToken)))
	{
		struct OpList *oplist = unary(self);
		
		if (oplist && oplist->ops[0].native == Op.getLocal)
			error(self, Error.syntaxError(OpList.text(oplist), "delete of an unqualified identifier in strict mode"));
		else if (oplist && oplist->ops[0].native == Op.getMember)
			changeNative(oplist->ops, Op.deleteMember);
		else if (oplist && oplist->ops[0].native == Op.getProperty)
			changeNative(oplist->ops, Op.deleteProperty);
		else if (oplist)
			error(self, Error.referenceError(OpList.text(oplist), "invalid delete operand"));
		else
			error(self, Error.syntaxError(self->lexer->text, "expected expression, got '%.*s'", self->lexer->text.length, self->lexer->text.location));
		
		return oplist;
	}
	else if (acceptToken(self, Lexer(voidToken)))
		return OpList.unshift(Op.make(Op.exchange, Value.undefined(), self->lexer->text), unary(self));
	else if (acceptToken(self, Lexer(typeofToken)))
		return OpList.unshift(Op.make(Op.typeOf, Value.undefined(), self->lexer->text), unary(self));
	else if (acceptToken(self, Lexer(incrementToken)))
		return OpList.unshift(Op.make(Op.incrementRef, Value.undefined(), self->lexer->text), expressionRef(self, unary(self), "invalid increment operand"));
	else if (acceptToken(self, Lexer(decrementToken)))
		return OpList.unshift(Op.make(Op.decrementRef, Value.undefined(), self->lexer->text), expressionRef(self, unary(self), "invalid decrement operand"));
	else if (acceptToken(self, '+'))
		return OpList.unshift(Op.make(Op.positive, Value.undefined(), self->lexer->text), unary(self));
	else if (acceptToken(self, '-'))
		return OpList.unshift(Op.make(Op.negative, Value.undefined(), self->lexer->text), unary(self));
	else if (acceptToken(self, '~'))
		return OpList.unshift(Op.make(Op.invert, Value.undefined(), self->lexer->text), unary(self));
	else if (acceptToken(self, '!'))
		return OpList.unshift(Op.make(Op.not, Value.undefined(), self->lexer->text), unary(self));
	else
		return postfix(self);
}

static struct OpList * multiplicative (struct Parser *self)
{
	struct OpList *oplist = unary(self);
	while (1)
	{
		struct Text text = self->lexer->text;
		Native native;
		
		if (acceptToken(self, '*'))
			native = Op.multiply;
		else if (acceptToken(self, '/'))
			native = Op.divide;
		else if (acceptToken(self, '%'))
			native = Op.modulo;
		else
			return oplist;
		
		if (oplist)
		{
			oplist = OpList.join(oplist, unary(self));
			oplist = OpList.unshift(Op.make(native, Value.undefined(), OpList.text(oplist)), oplist);
		}
		else
			error(self, Error.syntaxError(text, "expected expression, got '%.*s'", text.length, text.location));
	}
}

static struct OpList * additive (struct Parser *self)
{
	struct OpList *oplist = multiplicative(self);
	while (1)
	{
		struct Text text = self->lexer->text;
		Native native;
		
		if (acceptToken(self, '+'))
			native = Op.add;
		else if (acceptToken(self, '-'))
			native = Op.minus;
		else
			return oplist;
		
		if (oplist)
		{
			oplist = OpList.join(oplist, multiplicative(self));
			oplist = OpList.unshift(Op.make(native, Value.undefined(), OpList.text(oplist)), oplist);
		}
		else
			error(self, Error.syntaxError(text, "expected expression, got '%.*s'", text.length, text.location));
	}
}

static struct OpList * shift (struct Parser *self)
{
	struct OpList *oplist = additive(self);
	while (1)
	{
		struct Text text = self->lexer->text;
		Native native;
		
		if (acceptToken(self, Lexer(leftShiftToken)))
			native = Op.leftShift;
		else if (acceptToken(self, Lexer(rightShiftToken)))
			native = Op.rightShift;
		else if (acceptToken(self, Lexer(unsignedRightShiftToken)))
			native = Op.unsignedRightShift;
		else
			return oplist;
		
		if (oplist)
		{
			oplist = OpList.join(oplist, additive(self));
			oplist = OpList.unshift(Op.make(native, Value.undefined(), OpList.text(oplist)), oplist);
		}
		else
			error(self, Error.syntaxError(text, "expected expression, got '%.*s'", text.length, text.location));
	}
}

static struct OpList * relational (struct Parser *self, int noIn)
{
	struct OpList *oplist = shift(self);
	while (1)
	{
		struct Text text = self->lexer->text;
		Native native;
		
		if (acceptToken(self, '<'))
			native = Op.less;
		else if (acceptToken(self, '>'))
			native = Op.more;
		else if (acceptToken(self, Lexer(lessOrEqualToken)))
			native = Op.lessOrEqual;
		else if (acceptToken(self, Lexer(moreOrEqualToken)))
			native = Op.moreOrEqual;
		else if (acceptToken(self, Lexer(instanceofToken)))
			native = Op.instanceOf;
		else if (!noIn && acceptToken(self, Lexer(inToken)))
			native = Op.in;
		else
			return oplist;
		
		if (oplist)
		{
			oplist = OpList.join(oplist, shift(self));
			oplist = OpList.unshift(Op.make(native, Value.undefined(), OpList.text(oplist)), oplist);
		}
		else
			error(self, Error.syntaxError(text, "expected expression, got '%.*s'", text.length, text.location));
	}
}

static struct OpList * equality (struct Parser *self, int noIn)
{
	struct OpList *oplist = relational(self, noIn);
	while (1)
	{
		struct Text text = self->lexer->text;
		Native native;
		
		if (acceptToken(self, Lexer(equalToken)))
			native = Op.equal;
		else if (acceptToken(self, Lexer(notEqualToken)))
			native = Op.notEqual;
		else if (acceptToken(self, Lexer(identicalToken)))
			native = Op.identical;
		else if (acceptToken(self, Lexer(notIdenticalToken)))
			native = Op.notIdentical;
		else
			return oplist;
		
		if (oplist)
		{
			oplist = OpList.join(oplist, relational(self, noIn));
			oplist = OpList.unshift(Op.make(native, Value.undefined(), OpList.text(oplist)), oplist);
		}
		else
			error(self, Error.syntaxError(text, "expected expression, got '%.*s'", text.length, text.location));
	}
}

static struct OpList * bitwiseAnd (struct Parser *self, int noIn)
{
	struct OpList *oplist = equality(self, noIn);
	struct Text text = self->lexer->text;
	while (acceptToken(self, '&'))
		if (oplist)
		{
			oplist = OpList.join(oplist, equality(self, noIn));
			oplist = OpList.unshift(Op.make(Op.bitwiseAnd, Value.undefined(), OpList.text(oplist)), oplist);
		}
		else
			error(self, Error.syntaxError(text, "expected expression, got '%.*s'", text.length, text.location));
	
	return oplist;
}

static struct OpList * bitwiseXor (struct Parser *self, int noIn)
{
	struct OpList *oplist = bitwiseAnd(self, noIn);
	struct Text text = self->lexer->text;
	while (acceptToken(self, '^'))
		if (oplist)
		{
			oplist = OpList.join(oplist, bitwiseAnd(self, noIn));
			oplist = OpList.unshift(Op.make(Op.bitwiseXor, Value.undefined(), OpList.text(oplist)), oplist);
		}
		else
			error(self, Error.syntaxError(text, "expected expression, got '%.*s'", text.length, text.location));
	
	return oplist;
}

static struct OpList * bitwiseOr (struct Parser *self, int noIn)
{
	struct OpList *oplist = bitwiseXor(self, noIn);
	struct Text text = self->lexer->text;
	while (acceptToken(self, '|'))
		if (oplist)
		{
			oplist = OpList.join(oplist, bitwiseXor(self, noIn));
			oplist = OpList.unshift(Op.make(Op.bitwiseOr, Value.undefined(), OpList.text(oplist)), oplist);
		}
		else
			error(self, Error.syntaxError(text, "expected expression, got '%.*s'", text.length, text.location));
	
	return oplist;
}

static struct OpList * logicalAnd (struct Parser *self, int noIn)
{
	int32_t opCount;
	struct OpList *oplist = bitwiseOr(self, noIn), *nextOp = NULL;
	struct Text text = self->lexer->text;
	while (acceptToken(self, Lexer(logicalAndToken)))
		if (oplist && (nextOp = bitwiseOr(self, noIn)))
		{
			opCount = nextOp->opCount;
			oplist = OpList.join(oplist, nextOp);
			oplist = OpList.unshift(Op.make(Op.logicalAnd, Value.integer(opCount), OpList.text(oplist)), oplist);
		}
		else
			error(self, Error.syntaxError(text, "expected expression, got '%.*s'", text.length, text.location));
	
	return oplist;
}

static struct OpList * logicalOr (struct Parser *self, int noIn)
{
	int32_t opCount;
	struct OpList *oplist = logicalAnd(self, noIn), *nextOp = NULL;
	struct Text text = self->lexer->text;
	while (acceptToken(self, Lexer(logicalOrToken)))
		if (oplist && (nextOp = logicalAnd(self, noIn)))
		{
			opCount = nextOp->opCount;
			oplist = OpList.join(oplist, nextOp);
			oplist = OpList.unshift(Op.make(Op.logicalOr, Value.integer(opCount), OpList.text(oplist)), oplist);
		}
		else
			error(self, Error.syntaxError(text, "expected expression, got '%.*s'", text.length, text.location));
	
	return oplist;
}

static struct OpList * conditional (struct Parser *self, int noIn)
{
	struct OpList *oplist = logicalOr(self, noIn);
	struct Text text = self->lexer->text;
	if (acceptToken(self, '?'))
	{
		if (oplist)
		{
			struct OpList *trueOps = assignment(self, 0);
			struct OpList *falseOps;
			
			if (!expectToken(self, ':'))
				return oplist;
			
			falseOps = assignment(self, noIn);
			
			trueOps = OpList.append(trueOps, Op.make(Op.jump, Value.integer(falseOps->opCount), OpList.text(trueOps)));
			oplist = OpList.unshift(Op.make(Op.jumpIfNot, Value.integer(trueOps->opCount), OpList.text(oplist)), oplist);
			oplist = OpList.join(oplist, trueOps);
			oplist = OpList.join(oplist, falseOps);
			
			return oplist;
		}
		else
			error(self, Error.syntaxError(text, "expected expression, got '%.*s'", text.length, text.location));
	}
	return oplist;
}

static struct OpList * assignment (struct Parser *self, int noIn)
{
	struct OpList *oplist = conditional(self, noIn), *opassign = NULL;
	struct Text text = self->lexer->text;
	Native native = NULL;
	
	if (acceptToken(self, '='))
	{
		if (!oplist)
			error(self, Error.syntaxError(text, "expected expression, got '='"));
		else if (oplist->ops[0].native == Op.getLocal && oplist->opCount == 1)
		{
			if (Key.isEqual(oplist->ops[0].value.data.key, Key(eval)))
				error(self, Error.syntaxError(text, "can't assign to eval"));
			else if (Key.isEqual(oplist->ops[0].value.data.key, Key(arguments)))
				error(self, Error.syntaxError(text, "can't assign to arguments"));
			
			changeNative(oplist->ops, Op.setLocal);
		}
		else if (oplist->ops[0].native == Op.getMember)
			changeNative(oplist->ops, Op.setMember);
		else if (oplist->ops[0].native == Op.getProperty)
			changeNative(oplist->ops, Op.setProperty);
		else
			error(self, Error.referenceError(OpList.text(oplist), "invalid assignment left-hand side"));
		
		if (!( opassign = assignment(self, noIn) ))
			error(self, Error.syntaxError(self->lexer->text, "expected expression, got '%.*s'", self->lexer->text.length, self->lexer->text.location));
		
		return OpList.join(oplist, opassign);
	}
	else if (acceptToken(self, Lexer(multiplyAssignToken)))
		native = Op.multiplyAssignRef;
	else if (acceptToken(self, Lexer(divideAssignToken)))
		native = Op.divideAssignRef;
	else if (acceptToken(self, Lexer(moduloAssignToken)))
		native = Op.moduloAssignRef;
	else if (acceptToken(self, Lexer(addAssignToken)))
		native = Op.addAssignRef;
	else if (acceptToken(self, Lexer(minusAssignToken)))
		native = Op.minusAssignRef;
	else if (acceptToken(self, Lexer(leftShiftAssignToken)))
		native = Op.leftShiftAssignRef;
	else if (acceptToken(self, Lexer(rightShiftAssignToken)))
		native = Op.rightShiftAssignRef;
	else if (acceptToken(self, Lexer(unsignedRightShiftAssignToken)))
		native = Op.unsignedRightShiftAssignRef;
	else if (acceptToken(self, Lexer(andAssignToken)))
		native = Op.bitAndAssignRef;
	else if (acceptToken(self, Lexer(xorAssignToken)))
		native = Op.bitXorAssignRef;
	else if (acceptToken(self, Lexer(orAssignToken)))
		native = Op.bitOrAssignRef;
	else
		return oplist;
	
	if (oplist)
	{
		if (!( opassign = assignment(self, noIn) ))
			error(self, Error.syntaxError(self->lexer->text, "expected expression, got '%.*s'", self->lexer->text.length, self->lexer->text.location));
		
		return OpList.join(OpList.unshift(Op.make(native, Value.undefined(), text), expressionRef(self, oplist, "invalid assignment left-hand side")), opassign);
	}
	
	error(self, Error.syntaxError(text, "expected expression, got '%.*s'", text.length, text.location));
	
	return NULL;
}

static struct OpList * expression (struct Parser *self, int noIn)
{
	struct OpList *oplist = assignment(self, noIn);
	while (acceptToken(self, ','))
		oplist = OpList.unshift(Op.make(Op.discard, Value.undefined(), self->lexer->text), OpList.join(oplist, assignment(self, noIn)));
	
	return oplist;
}


// MARK: Statements

static struct OpList * statementList (struct Parser *self)
{
	struct OpList *oplist = NULL, *statementOps = NULL;
	
	while (( statementOps = statement(self) ))
		oplist = OpList.join(oplist, statementOps);
	
	return oplist;
}

static struct OpList * block (struct Parser *self)
{
	struct OpList *oplist = NULL;
	expectToken(self, '{');
	if (previewToken(self) == '}')
		oplist = OpList.create(Op.next, Value.undefined(), self->lexer->text);
	else
		oplist = statementList(self);
	
	expectToken(self, '}');
	return oplist;
}

static struct OpList * variableDeclaration (struct Parser *self, int noIn)
{
	struct Value value = self->lexer->value;
	struct Text text = self->lexer->text;
	if (!expectToken(self, Lexer(identifierToken)))
		return NULL;
	
	if (Key.isEqual(value.data.key, Key(eval)))
		error(self, Error.syntaxError(text, "redefining eval is deprecated"));
	else if (Key.isEqual(value.data.key, Key(arguments)))
		error(self, Error.syntaxError(text, "redefining arguments is deprecated"));
	
	Object.add(&self->function->context, value.data.key, Value.undefined(), Object(writable));
	
	if (acceptToken(self, '='))
	{
		struct OpList *opassign = assignment(self, noIn);
		if (!opassign)
			error(self, Error.syntaxError(self->lexer->text, "expected expression, got '%.*s'", self->lexer->text.length, self->lexer->text.location));
		
		return OpList.unshift(Op.make(Op.discard, Value.undefined(), self->lexer->text), OpList.join(OpList.create(Op.setLocal, value, text), opassign));
	}
	else
		return OpList.create(Op.next, value, text);
}

static struct OpList * variableDeclarationList (struct Parser *self, int noIn)
{
	struct OpList *oplist = NULL;
	do
		oplist = OpList.join(oplist, variableDeclaration(self, noIn));
	while (acceptToken(self, ','));
	
	return oplist;
}

static struct OpList * ifStatement (struct Parser *self)
{
	struct OpList *oplist = NULL, *trueOps = NULL, *falseOps = NULL;
	expectToken(self, '(');
	oplist = expression(self, 0);
	expectToken(self, ')');
	trueOps = statement(self);
	if (!trueOps)
		return oplist;
	
	if (acceptToken(self, Lexer(elseToken)))
	{
		falseOps = statement(self);
		trueOps = OpList.append(trueOps, Op.make(Op.jump, Value.integer(falseOps->opCount), OpList.text(trueOps)));
	}
	oplist = OpList.unshift(Op.make(Op.jumpIfNot, Value.integer(trueOps->opCount), OpList.text(oplist)), oplist);
	oplist = OpList.join(oplist, trueOps);
	oplist = OpList.join(oplist, falseOps);
	return oplist;
}

static struct OpList * doStatement (struct Parser *self)
{
	struct OpList *oplist, *condition;
	
	pushDepth(self, Key(none), 2);
	oplist = statement(self);
	popDepth(self);
	
	expectToken(self, Lexer(whileToken));
	expectToken(self, '(');
	condition = expression(self, 0);
	expectToken(self, ')');
	semicolon(self);
	
	return OpList.createLoop(NULL, condition, NULL, oplist, 1);
}

static struct OpList * whileStatement (struct Parser *self)
{
	struct OpList *oplist, *condition;
	
	expectToken(self, '(');
	condition = expression(self, 0);
	expectToken(self, ')');
	
	pushDepth(self, Key(none), 2);
	oplist = statement(self);
	popDepth(self);
	
	return OpList.createLoop(NULL, condition, NULL, oplist, 0);
}

static struct OpList * forStatement (struct Parser *self)
{
	struct OpList *oplist = NULL, *condition = NULL, *increment = NULL, *body = NULL;
	
	expectToken(self, '(');
	
	if (acceptToken(self, Lexer(varToken)))
		oplist = variableDeclarationList(self, 1);
	else if (previewToken(self) != ';')
	{
		oplist = expression(self, 1);
		
		if (oplist)
			oplist = OpList.unshift(Op.make(Op.discard, Value.undefined(), OpList.text(oplist)), oplist);
	}
	
	if (acceptToken(self, Lexer(inToken)))
	{
//		OpList.dumpTo(oplist, stderr);
		
		if (oplist->opCount == 2 && oplist->ops[0].native == Op.discard && oplist->ops[1].native == Op.getLocal)
		{
			changeNative(oplist->ops, Op.iterateInRef);
			changeNative(&oplist->ops[1], Op.getLocalRef);
		}
		else if (oplist->opCount == 1 && oplist->ops[0].native == Op.next)
		{
			changeNative(oplist->ops, Op.getLocalRef);
			oplist = OpList.unshift(Op.make(Op.iterateInRef, Value.undefined(), self->lexer->text), oplist);
		}
		else
			error(self, Error.referenceError(OpList.text(oplist), "invalid for/in left-hand side"));
		
		oplist = OpList.join(oplist, expression(self, 0));
		expectToken(self, ')');
		
		pushDepth(self, Key(none), 2);
		body = statement(self);
		popDepth(self);
		
		if (!body)
			return oplist;
		
		return OpList.join(OpList.append(oplist, Op.make(Op.value, Value.integer(body->opCount), self->lexer->text)), body);
	}
	else
	{
		expectToken(self, ';');
		if (previewToken(self) != ';')
			condition = expression(self, 0);
		
		expectToken(self, ';');
		if (previewToken(self) != ')')
			increment = expression(self, 0);
		
		expectToken(self, ')');
		
		pushDepth(self, Key(none), 2);
		body = statement(self);
		popDepth(self);
		
		return OpList.createLoop(oplist, condition, increment, body, 0);
	}
}

static struct OpList * continueStatement (struct Parser *self, struct Text text)
{
	struct OpList *oplist = NULL;
	struct Key label = Key(none);
	struct Text labelText = self->lexer->text;
	uint16_t depth, lastestDepth, breaker = 0;
	
	if (!self->lexer->didLineBreak && previewToken(self) == Lexer(identifierToken))
	{
		label = self->lexer->value.data.key;
		nextToken(self);
	}
	semicolon(self);
	
	depth = self->depthCount;
	if (!depth)
		error(self, Error.syntaxError(text, "continue must be inside loop"));
	
	lastestDepth = 0;
	while (depth--)
	{
		breaker += self->depths[depth].depth;
		if (self->depths[depth].depth)
			lastestDepth = self->depths[depth].depth;
		
		if (lastestDepth == 2)
			if (Key.isEqual(label, Key(none)) || Key.isEqual(label, self->depths[depth].key))
				return OpList.create(Op.value, Value.breaker(breaker - 1), self->lexer->text);
	}
	error(self, Error.syntaxError(labelText, "label not found"));
	return oplist;
}

static struct OpList * breakStatement (struct Parser *self, struct Text text)
{
	struct OpList *oplist = NULL;
	struct Key label = Key(none);
	struct Text labelText = self->lexer->text;
	uint16_t depth, breaker = 0;
	
	if (!self->lexer->didLineBreak && previewToken(self) == Lexer(identifierToken))
	{
		label = self->lexer->value.data.key;
		nextToken(self);
	}
	semicolon(self);
	
	depth = self->depthCount;
	if (!depth)
		error(self, Error.syntaxError(text, "break must be inside loop or switch"));
	
	while (depth--)
	{
		breaker += self->depths[depth].depth;
		if (Key.isEqual(label, Key(none)) || Key.isEqual(label, self->depths[depth].key))
			return OpList.create(Op.value, Value.breaker(breaker), self->lexer->text);
	}
	error(self, Error.syntaxError(labelText, "label not found"));
	return oplist;
}

static struct OpList * returnStatement (struct Parser *self, struct Text text)
{
	struct OpList *oplist = NULL;
	
	if (!self->lexer->didLineBreak && previewToken(self) != ';' && previewToken(self) != '}' && previewToken(self) != Lexer(noToken))
		oplist = expression(self, 0);
	
	semicolon(self);
	
	if (!oplist)
		oplist = OpList.create(Op.value, Value.undefined(), self->lexer->text);
	
	oplist = OpList.unshift(Op.make(Op.result, Value.undefined(), text), oplist);
	return oplist;
}

static struct OpList * switchStatement (struct Parser *self)
{
	struct OpList *oplist = NULL, *conditionOps = NULL, *defaultOps = NULL;
	struct Text text;
	uint32_t conditionCount = 0;
	
	expectToken(self, '(');
	conditionOps = expression(self, 0);
	expectToken(self, ')');
	expectToken(self, '{');
	pushDepth(self, Key(none), 1);
	
	while (previewToken(self) != '}' && previewToken(self) != Lexer(errorToken) && previewToken(self) != Lexer(noToken))
	{
		text = self->lexer->text;
		
		if (acceptToken(self, Lexer(caseToken)))
		{
			conditionOps = OpList.join(conditionOps, expression(self, 0));
			conditionOps = OpList.append(conditionOps, Op.make(Op.value, Value.integer(oplist? oplist->opCount: 0), text));
			++conditionCount;
			expectToken(self, ':');
			oplist = OpList.join(oplist, statementList(self));
		}
		else if (acceptToken(self, Lexer(defaultToken)))
		{
			if (!defaultOps)
			{
				defaultOps = OpList.create(Op.jump, Value.integer(oplist? oplist->opCount: 0), text);
				expectToken(self, ':');
				oplist = OpList.join(oplist, statementList(self));
			}
			else
				error(self, Error.syntaxError(text, "more than one switch default"));
		}
		else
			error(self, Error.syntaxError(text, "invalid switch statement"));
	}
	
	if (!defaultOps && oplist)
		defaultOps = OpList.create(Op.jump, Value.integer(oplist? oplist->opCount: 0), text);
	
	conditionOps = OpList.unshift(Op.make(Op.switchOp, Value.integer(conditionOps? conditionOps->opCount: 0), OpList.text(conditionOps)), conditionOps);
	conditionOps = OpList.join(conditionOps, defaultOps);
	oplist = OpList.join(conditionOps, oplist);
	
	popDepth(self);
	expectToken(self, '}');
	return oplist;
}

static struct OpList * statement (struct Parser *self)
{
	struct OpList *oplist = NULL;
	struct Text text = self->lexer->text;
	
	if (previewToken(self) == '{')
		return block(self);
	else if (acceptToken(self, Lexer(varToken)))
	{
		oplist = variableDeclarationList(self, 0);
		semicolon(self);
		return oplist;
	}
	else if (acceptToken(self, ';'))
		return OpList.create(Op.next, Value.undefined(), text);
	else if (acceptToken(self, Lexer(ifToken)))
		return ifStatement(self);
	else if (acceptToken(self, Lexer(doToken)))
		return doStatement(self);
	else if (acceptToken(self, Lexer(whileToken)))
		return whileStatement(self);
	else if (acceptToken(self, Lexer(forToken)))
		return forStatement(self);
	else if (acceptToken(self, Lexer(continueToken)))
		return continueStatement(self, text);
	else if (acceptToken(self, Lexer(breakToken)))
		return breakStatement(self, text);
	else if (acceptToken(self, Lexer(returnToken)))
		return returnStatement(self, text);
	else if (acceptToken(self, Lexer(withToken)))
	{
		error(self, Error.syntaxError(text, "strict mode code may not contain 'with' statements"));
		return oplist;
	}
	else if (acceptToken(self, Lexer(switchToken)))
		return switchStatement(self);
	else if (acceptToken(self, Lexer(throwToken)))
	{
		if (!self->lexer->didLineBreak && previewToken(self) != ';' && previewToken(self) != '}' && previewToken(self) != Lexer(noToken))
			oplist = expression(self, 0);
		
		if (!oplist)
			error(self, Error.syntaxError(text, "throw statement is missing an expression"));
		
		semicolon(self);
		return OpList.unshift(Op.make(Op.throw, Value.undefined(), Text.join(text, OpList.text(oplist))), oplist);
	}
	else if (acceptToken(self, Lexer(tryToken)))
	{
		oplist = block(self);
		if (!oplist)
			return NULL;
		
		oplist = OpList.unshift(Op.make(Op.try, Value.integer(oplist->opCount), text), oplist);
		
		if (previewToken(self) != Lexer(catchToken) && previewToken(self) != Lexer(finallyToken))
			error(self, Error.syntaxError(self->lexer->text, "expected catch or finally, got %s", Lexer.tokenChars(previewToken(self))));
		
		if (acceptToken(self, Lexer(catchToken)))
		{
			struct Op identiferOp;
			struct OpList *catchOps;
			
			expectToken(self, '(');
			if (previewToken(self) != Lexer(identifierToken))
				error(self, Error.syntaxError(text, "missing identifier in catch"));
			
			identiferOp = identifier(self);
			expectToken(self, ')');
			
			catchOps = block(self);
			if (catchOps)
			{
				catchOps = OpList.appendNoop(OpList.unshift(identiferOp, catchOps));
				catchOps = OpList.unshift(Op.make(Op.jump, Value.integer(catchOps->opCount), text), catchOps);
				oplist = OpList.join(oplist, catchOps);
			}
		}
		else
			oplist = OpList.appendNoop(oplist);
		
		if (acceptToken(self, Lexer(finallyToken)))
			oplist = OpList.join(oplist, block(self));
		
		return OpList.appendNoop(oplist);
	}
	else if (acceptToken(self, Lexer(debuggerToken)))
	{
		semicolon(self);
		return OpList.create(Op.debug, Value.undefined(), text);
	}
	else
	{
		oplist = expression(self, 0);
		if (!oplist)
			return NULL;
		
		if (oplist->ops[0].native == Op.getLocal && oplist->opCount == 1 && acceptToken(self, ':'))
		{
			if (previewToken(self) == Lexer(doToken)
				|| previewToken(self) == Lexer(whileToken)
				|| previewToken(self) == Lexer(forToken)
				|| previewToken(self) == Lexer(switchToken)
				)
			{
				pushDepth(self, oplist->ops[0].value.data.key, 0);
				free(oplist), oplist = NULL;
				oplist = statement(self);
				popDepth(self);
				return oplist;
			}
			else
				return statement(self);
		}
		
		semicolon(self);
		return OpList.unshift(Op.make(Op.expression, Value.undefined(), OpList.text(oplist)), oplist);
	}
}


// MARK: Function

static struct OpList * parameters (struct Parser *self, int *count)
{
	struct Op op;
	*count = 0;
	if (previewToken(self) != ')')
		do
		{
			++*count;
			op = identifier(self);
			
			if (op.value.data.key.data.integer)
			{
				if (Key.isEqual(op.value.data.key, Key(eval)))
					error(self, Error.syntaxError(op.text, "redefining eval is deprecated"));
				else if (Key.isEqual(op.value.data.key, Key(arguments)))
					error(self, Error.syntaxError(op.text, "redefining arguments is deprecated"));
				
				Object.add(&self->function->context, op.value.data.key, Value.undefined(), Object(writable) | Object(configurable));
			}
		} while (acceptToken(self, ','));
	
	return NULL;
}

static struct OpList * function (struct Parser *self, int isDeclaration, int isGetter, int isSetter)
{
	struct Text text = self->lexer->text;
	
	struct OpList *oplist = NULL;
	int parameterCount = 0;
	
	struct Op identifierOp = { 0, Value.undefined() };
	struct Function *parentFunction;
	struct Function *function;
	
	if (!isGetter && !isSetter)
	{
		expectToken(self, Lexer(functionToken));
		
		if (previewToken(self) == Lexer(identifierToken))
		{
			identifierOp = identifier(self);
			
			if (Key.isEqual(identifierOp.value.data.key, Key(eval)))
				error(self, Error.syntaxError(identifierOp.text, "redefining eval is deprecated"));
			else if (Key.isEqual(identifierOp.value.data.key, Key(arguments)))
				error(self, Error.syntaxError(identifierOp.text, "redefining arguments is deprecated"));
		}
		else if (isDeclaration)
		{
			error(self, Error.syntaxError(self->lexer->text, "function statement requires a name"));
			return NULL;
		}
	}
	
	parentFunction = self->function;
	function = Function.create(NULL);
	Object.add(&function->context, Key(arguments), Value.undefined(), Object(writable));
	
	self->function = function;
	expectToken(self, '(');
	oplist = OpList.join(oplist, parameters(self, &parameterCount));
	
	if (isGetter)
	{
		function->flags |= Function(isGetter);
		if (parameterCount != 0)
			error(self, Error.syntaxError(self->lexer->text, "getter functions must have no arguments"));
	}
	else if (isSetter)
	{
		function->flags |= Function(isSetter);
		if (parameterCount != 1)
			error(self, Error.syntaxError(self->lexer->text, "setter functions must have one argument"));
	}
	
	expectToken(self, ')');
	expectToken(self, '{');
	oplist = OpList.join(oplist, sourceElements(self, '}'));
	text.length = self->lexer->text.location - text.location + 1;
	expectToken(self, '}');
	self->function = parentFunction;
	
	function->oplist = oplist;
	function->text = text;
	function->parameterCount = parameterCount;
	parentFunction->flags |= Function(needHeap);
	
	Object.add(&function->object, Key(length), Value.integer(parameterCount), Object(configurable));
	
	if (isDeclaration)
		Object.add(&parentFunction->context, identifierOp.value.data.key, Value.undefined(), Object(writable) | Object(configurable));
	else if (identifierOp.value.type != Value(undefined) && !isGetter && !isSetter)
		Object.add(&function->context, identifierOp.value.data.key, Value.function(function), Object(writable) | Object(configurable));
	
	if (isDeclaration)
		return OpList.append(OpList.create(Op.setLocal, identifierOp.value, text), Op.make(Op.function, Value.function(function), text));
	else
		return OpList.create(Op.function, Value.function(function), self->lexer->text);
}


// MARK: Source

static struct OpList * sourceElements (struct Parser *self, enum Lexer(Token) endToken)
{
	struct OpList *oplist = NULL, *init = NULL, *last = NULL, *statementOps = NULL;
	
	while (previewToken(self) != endToken && previewToken(self) != Lexer(errorToken) && previewToken(self) != Lexer(noToken))
		if (previewToken(self) == Lexer(functionToken))
			init = OpList.join(init, OpList.unshift(Op.make(Op.discard, Value.undefined(), self->lexer->text), function(self, 1, 0, 0)));
		else
		{
			statementOps = statement(self);
			if (statementOps)
			{
				if (statementOps->opCount == 1 && statementOps->ops[0].native == Op.next)
					OpList.destroy(statementOps), statementOps = NULL;
				else
				{
					oplist = OpList.join(oplist, last);
					last = statementOps;
				}
			}
			else
				error(self, Error.syntaxError(self->lexer->text, "expected statement, got %s", Lexer.tokenChars(previewToken(self))));
		}
	
	last = OpList.appendNoop(last);
	
	oplist = OpList.join(init, oplist);
	oplist = OpList.join(oplist, last);
	
	OpList.optimizeWithContext(oplist, &self->function->context);
	
	return oplist;
}


// MARK: - Methods

struct Parser * createWithLexer (struct Lexer *lexer)
{
	struct Parser *self = malloc(sizeof(*self));
	assert(self);
	*self = Parser.identity;
	
	self->lexer = lexer;
	
	return self;
}

void destroy (struct Parser *self)
{
	assert(self);
	
	Lexer.destroy(self->lexer), self->lexer = NULL;
	free(self->depths), self->depths = NULL;
	free(self), self = NULL;
}

struct Function * parseWithContext (struct Parser * const self, struct Object *context)
{
	struct Function *function;
	struct OpList *oplist;
	
	assert(self);
	
	function = Function.create(context);
	nextToken(self);
	
	self->function = function;
	oplist = sourceElements(self, Lexer(noToken));
	self->function = NULL;
	
	if (self->error)
	{
		struct Op errorOps[] = {
			{ Op.throw, Value.undefined(), self->error->text },
			{ Op.value, Value.error(self->error) },
		};
		
		OpList.destroy(oplist), oplist = NULL;
		oplist = malloc(sizeof(*oplist) + sizeof(errorOps));
		oplist->opCount = sizeof(errorOps) / sizeof(*errorOps);
		memcpy(oplist->ops, errorOps, sizeof(errorOps));
	}
	
	function->oplist = oplist;
	return function;
}

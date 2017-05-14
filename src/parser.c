//
//  parser.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#define Implementation
#include "parser.h"

#include "oplist.h"

// MARK: - Private


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
}

static void syntaxError (struct Parser *self, struct Text text, struct Chars *message)
{
	error(self, Error.syntaxError(text, message));
}

static void referenceError (struct Parser *self, struct Text text, struct Chars *message)
{
	error(self, Error.referenceError(text, message));
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
			syntaxError(self, self->lexer->text, Chars.create("expected '%c', got %s", token, Lexer.tokenChars(previewToken(self))));
		else
			syntaxError(self, self->lexer->text, Chars.create("expected %s, got %s", Lexer.tokenChars(token), Lexer.tokenChars(previewToken(self))));
		
		return 0;
	}
	
	nextToken(self);
	return 1;
}

static inline struct OpList * expectationError (struct Parser *self, const char *type)
{
	syntaxError(self, self->lexer->text, Chars.create("expected %s, got %s", type, Lexer.tokenChars(previewToken(self))));
	return NULL;
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

static struct OpList * foldConstant (struct OpList * ops)
{
	struct Context context = { ops->ops };
	struct Value value = context.ops->native(&context);
	struct Text text = OpList.text(ops);
	OpList.destroy(ops);
	return OpList.create(Op.value, value, text);
}

static struct OpList * expressionRef (struct Parser *self, struct OpList *oplist, const char *name)
{
	if (!oplist)
		return NULL;
	
	if (oplist->ops[0].native == Op.getLocal && oplist->opCount == 1)
	{
		if (oplist->ops[0].value.type == Value(keyType))
		{
			if (Key.isEqual(oplist->ops[0].value.data.key, Key(eval)))
				syntaxError(self, OpList.text(oplist), Chars.create(name));
			else if (Key.isEqual(oplist->ops[0].value.data.key, Key(arguments)))
				syntaxError(self, OpList.text(oplist), Chars.create(name));
		}
		
		oplist->ops[0].native = Op.getLocalRef;
	}
	else if (oplist->ops[0].native == Op.getMember)
		oplist->ops[0].native = Op.getMemberRef;
	else if (oplist->ops[0].native == Op.getProperty)
		oplist->ops[0].native = Op.getPropertyRef;
	else
		referenceError(self, OpList.text(oplist), Chars.create("%s", name));
	
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
	
	syntaxError(self, self->lexer->text, Chars.create("missing ; before statement"));
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
	struct Text text = self->lexer->text;
	
	nextToken(self);
	
	do
	{
		while (previewToken(self) == ',')
		{
			++count;
			oplist = OpList.append(oplist, Op.make(Op.value, Value(none), self->lexer->text));
			nextToken(self);
		}
		
		if (previewToken(self) == ']')
			break;
		
		++count;
		oplist = OpList.join(oplist, assignment(self, 0));
	}
	while (acceptToken(self, ','));
	
	text = Text.join(text, self->lexer->text);
	expectToken(self, ']');
	
	return OpList.unshift(Op.make(Op.array, Value.integer(count), text), oplist);
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
		Input.printText(self->lexer->input, self->lexer->text, 1);
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
		oplist = OpList.create(Op.value, self->lexer->value, self->lexer->text);
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
	struct Text text = self->lexer->text;
	
	do
	{
		self->lexer->disallowKeyword = 1;
		nextToken(self);
		self->lexer->disallowKeyword = 0;
		
		if (previewToken(self) == '}')
			break;
		
		++count;
		oplist = OpList.join(oplist, propertyAssignment(self));
	}
	while (previewToken(self) == ',');
	
	text = Text.join(text, self->lexer->text);
	expectToken(self, '}');
	
	return OpList.unshift(Op.make(Op.object, Value.integer(count), text), oplist);
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
		oplist = OpList.create(Op.text, Value(undefined), self->lexer->text);
	else if (previewToken(self) == Lexer(integerToken) || previewToken(self) == Lexer(binaryToken))
		oplist = OpList.create(Op.value, self->lexer->value, self->lexer->text);
	else if (previewToken(self) == Lexer(thisToken))
		oplist = OpList.create(Op.this, Value(undefined), self->lexer->text);
	else if (previewToken(self) == Lexer(nullToken))
		oplist = OpList.create(Op.value, Value(null), self->lexer->text);
	else if (previewToken(self) == Lexer(trueToken))
		oplist = OpList.create(Op.value, Value.truth(1), self->lexer->text);
	else if (previewToken(self) == Lexer(falseToken))
		oplist = OpList.create(Op.value, Value.truth(0), self->lexer->text);
	else if (previewToken(self) == '{')
		return object(self);
	else if (previewToken(self) == '[')
		return array(self);
	else if (acceptToken(self, '('))
	{
		oplist = expression(self, 0);
		expectToken(self, ')');
		return oplist;
	}
	else
	{
		if (self->lexer->text.bytes[0] == '/')
		{
			self->lexer->allowRegex = 1;
			self->lexer->offset -= self->lexer->text.length;
			nextToken(self);
			self->lexer->allowRegex = 0;
			
			if (previewToken(self) != Lexer(regexpToken))
				syntaxError(self, self->lexer->text, Chars.create("expected RegExp, got '%.*s'", self->lexer->text.length, self->lexer->text.bytes));
		}
		
		if (previewToken(self) == Lexer(regexpToken))
			oplist = OpList.create(Op.regexp, Value(undefined), self->lexer->text);
		else
			return NULL;
	}
	
	nextToken(self);
	
	return oplist;
}

static struct OpList * arguments (struct Parser *self, int *count)
{
	struct OpList *oplist = NULL, *argumentOps;
	*count = 0;
	if (previewToken(self) != ')')
		do
		{
			argumentOps = assignment(self, 0);
			if (!argumentOps)
				syntaxError(self, self->lexer->text, Chars.create("expected expression, got '%.*s'", self->lexer->text.length, self->lexer->text.bytes));
			
			++*count;
			oplist = OpList.join(oplist, argumentOps);
		} while (acceptToken(self, ','));
	
	return oplist;
}

static struct OpList * member (struct Parser *self)
{
	struct OpList *oplist = new(self);
	struct Text text;
	while (1)
	{
		if (previewToken(self) == '.')
		{
			struct Value value;
			
			self->lexer->disallowKeyword = 1;
			nextToken(self);
			self->lexer->disallowKeyword = 0;
			
			value = self->lexer->value;
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
			
			oplist = OpList.unshift(Op.make(Op.getProperty, Value(undefined), text), oplist);
		}
		else
			break;
	}
	return oplist;
}

static struct OpList * new (struct Parser *self)
{
	struct OpList *oplist = NULL;
	struct Text text = self->lexer->text;
	
	if (acceptToken(self, Lexer(newToken)))
	{
		int count = 0;
		oplist = member(self);
		text = Text.join(text, OpList.text(oplist));
		if (acceptToken(self, '('))
		{
			oplist = OpList.join(oplist, arguments(self, &count));
			text = Text.join(text, self->lexer->text);
			expectToken(self, ')');
		}
		return OpList.unshift(Op.make(Op.construct, Value.integer(count), text), oplist);
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
	struct Value value;
	
	while (1)
	{
		if (previewToken(self) == '.')
		{
			if (!oplist)
				syntaxError(self, self->lexer->text, Chars.create("expected expression, got '%.*s'", self->lexer->text.length, self->lexer->text.bytes));
			
			self->lexer->disallowKeyword = 1;
			nextToken(self);
			self->lexer->disallowKeyword = 0;
			
			value = self->lexer->value;
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
			
			oplist = OpList.unshift(Op.make(Op.getProperty, Value(undefined), text), oplist);
		}
		else if (acceptToken(self, '('))
		{
			int count = 0;
			
			int isEval = oplist->opCount == 1 && oplist->ops[0].native == Op.getLocal && Key.isEqual(oplist->ops[0].value.data.key, Key(eval));
			if (isEval)
			{
				text = Text.join(OpList.text(oplist), self->lexer->text);
				OpList.destroy(oplist), oplist = NULL;
			}
			
			oplist = OpList.join(oplist, arguments(self, &count));
			text = Text.join(Text.join(text, OpList.text(oplist)), self->lexer->text);
			
			if (isEval)
				oplist = OpList.unshift(Op.make(Op.eval, Value.integer(count), text), oplist);
			else if (oplist->ops->native == Op.getMember)
				oplist = OpList.unshift(Op.make(Op.callMember, Value.integer(count), text), oplist);
			else if (oplist->ops->native == Op.getProperty)
				oplist = OpList.unshift(Op.make(Op.callProperty, Value.integer(count), text), oplist);
			else
				oplist = OpList.unshift(Op.make(Op.call, Value.integer(count), text), oplist);
			
			if (!expectToken(self, ')'))
				break;
		}
		else
			break;
	}
	return oplist;
}

static struct OpList * postfix (struct Parser *self)
{
	struct OpList *oplist = leftHandSide(self);
	struct Text text = self->lexer->text;
	
	if (!self->lexer->didLineBreak && acceptToken(self, Lexer(incrementToken)))
		oplist = OpList.unshift(Op.make(Op.postIncrementRef, Value(undefined), Text.join(oplist->ops->text, text)), expressionRef(self, oplist, "invalid increment operand"));
	if (!self->lexer->didLineBreak && acceptToken(self, Lexer(decrementToken)))
		oplist = OpList.unshift(Op.make(Op.postDecrementRef, Value(undefined), Text.join(oplist->ops->text, text)), expressionRef(self, oplist, "invalid decrement operand"));
	
	return oplist;
}

static struct OpList * unary (struct Parser *self)
{
	struct OpList *oplist, *alt;
	struct Text text = self->lexer->text;
	Native(Function) native;
	
	if (acceptToken(self, Lexer(deleteToken)))
	{
		oplist = unary(self);
		
		if (oplist && oplist->ops[0].native == Op.getLocal)
			syntaxError(self, OpList.text(oplist), Chars.create("delete of an unqualified identifier"));
		else if (oplist && oplist->ops[0].native == Op.getMember)
			oplist->ops->native = Op.deleteMember;
		else if (oplist && oplist->ops[0].native == Op.getProperty)
			oplist->ops->native = Op.deleteProperty;
		else if (oplist)
			referenceError(self, OpList.text(oplist), Chars.create("invalid delete operand"));
		else
			syntaxError(self, self->lexer->text, Chars.create("expected expression, got '%.*s'", self->lexer->text.length, self->lexer->text.bytes));
		
		return oplist;
	}
	else if (acceptToken(self, Lexer(voidToken)))
		native = Op.exchange, alt = unary(self);
	else if (acceptToken(self, Lexer(typeofToken)))
		native = Op.typeOf, alt = unary(self);
	else if (acceptToken(self, Lexer(incrementToken)))
		native = Op.incrementRef, alt = expressionRef(self, unary(self), "invalid increment operand");
	else if (acceptToken(self, Lexer(decrementToken)))
		native = Op.decrementRef, alt = expressionRef(self, unary(self), "invalid decrement operand");
	else if (acceptToken(self, '+'))
		native = Op.positive, alt = unary(self);
	else if (acceptToken(self, '-'))
		native = Op.negative, alt = unary(self);
	else if (acceptToken(self, '~'))
		native = Op.invert, alt = unary(self);
	else if (acceptToken(self, '!'))
		native = Op.not, alt = unary(self);
	else
		return postfix(self);
	
	if (!alt)
		return expectationError(self, "expression");
	
	oplist = OpList.unshift(Op.make(native, Value(undefined), Text.join(text, alt->ops->text)), alt);
	
	if (oplist->ops[1].native == Op.value)
		return foldConstant(oplist);
	else
		return oplist;
}

static struct OpList * multiplicative (struct Parser *self)
{
	struct OpList *oplist = unary(self), *alt;
	
	while (1)
	{
		Native(Function) native;
		
		if (previewToken(self) == '*')
			native = Op.multiply;
		else if (previewToken(self) == '/')
			native = Op.divide;
		else if (previewToken(self) == '%')
			native = Op.modulo;
		else
			return oplist;
		
		if (oplist)
		{
			nextToken(self);
			if ((alt = unary(self)))
			{
				struct Text text = Text.join(oplist->ops->text, alt->ops->text);
				oplist = OpList.unshiftJoin(Op.make(native, Value(undefined), text), oplist, alt);
				
				if (oplist->ops[1].native == Op.value && oplist->ops[2].native == Op.value)
					oplist = foldConstant(oplist);
				
				continue;
			}
			OpList.destroy(oplist);
		}
		return expectationError(self, "expression");
	}
}

static struct OpList * additive (struct Parser *self)
{
	struct OpList *oplist = multiplicative(self), *alt;
	while (1)
	{
		Native(Function) native;
		
		if (previewToken(self) == '+')
			native = Op.add;
		else if (previewToken(self) == '-')
			native = Op.minus;
		else
			return oplist;
		
		if (oplist)
		{
			nextToken(self);
			if ((alt = multiplicative(self)))
			{
				struct Text text = Text.join(oplist->ops->text, alt->ops->text);
				oplist = OpList.unshiftJoin(Op.make(native, Value(undefined), text), oplist, alt);
				
				if (oplist->ops[1].native == Op.value && oplist->ops[2].native == Op.value)
					oplist = foldConstant(oplist);
				
				continue;
			}
			OpList.destroy(oplist);
		}
		return expectationError(self, "expression");
	}
}

static struct OpList * shift (struct Parser *self)
{
	struct OpList *oplist = additive(self), *alt;
	while (1)
	{
		Native(Function) native;
		
		if (previewToken(self) == Lexer(leftShiftToken))
			native = Op.leftShift;
		else if (previewToken(self) == Lexer(rightShiftToken))
			native = Op.rightShift;
		else if (previewToken(self) == Lexer(unsignedRightShiftToken))
			native = Op.unsignedRightShift;
		else
			return oplist;
		
		if (oplist)
		{
			nextToken(self);
			if ((alt = additive(self)))
			{
				struct Text text = Text.join(oplist->ops->text, alt->ops->text);
				oplist = OpList.unshiftJoin(Op.make(native, Value(undefined), text), oplist, alt);
				
				if (oplist->ops[1].native == Op.value && oplist->ops[2].native == Op.value)
					oplist = foldConstant(oplist);
				
				continue;
			}
			OpList.destroy(oplist);
		}
		return expectationError(self, "expression");
	}
}

static struct OpList * relational (struct Parser *self, int noIn)
{
	struct OpList *oplist = shift(self), *alt;
	while (1)
	{
		Native(Function) native;
		
		if (previewToken(self) == '<')
			native = Op.less;
		else if (previewToken(self) == '>')
			native = Op.more;
		else if (previewToken(self) == Lexer(lessOrEqualToken))
			native = Op.lessOrEqual;
		else if (previewToken(self) == Lexer(moreOrEqualToken))
			native = Op.moreOrEqual;
		else if (previewToken(self) == Lexer(instanceofToken))
			native = Op.instanceOf;
		else if (!noIn && previewToken(self) == Lexer(inToken))
			native = Op.in;
		else
			return oplist;
		
		if (oplist)
		{
			nextToken(self);
			if ((alt = shift(self)))
			{
				struct Text text = Text.join(oplist->ops->text, alt->ops->text);
				oplist = OpList.unshiftJoin(Op.make(native, Value(undefined), text), oplist, alt);
				
				continue;
			}
			OpList.destroy(oplist);
		}
		return expectationError(self, "expression");
	}
}

static struct OpList * equality (struct Parser *self, int noIn)
{
	struct OpList *oplist = relational(self, noIn), *alt;
	while (1)
	{
		Native(Function) native;
		
		if (previewToken(self) == Lexer(equalToken))
			native = Op.equal;
		else if (previewToken(self) == Lexer(notEqualToken))
			native = Op.notEqual;
		else if (previewToken(self) == Lexer(identicalToken))
			native = Op.identical;
		else if (previewToken(self) == Lexer(notIdenticalToken))
			native = Op.notIdentical;
		else
			return oplist;
		
		if (oplist)
		{
			nextToken(self);
			if ((alt = relational(self, noIn)))
			{
				struct Text text = Text.join(oplist->ops->text, alt->ops->text);
				oplist = OpList.unshiftJoin(Op.make(native, Value(undefined), text), oplist, alt);
				
				continue;
			}
			OpList.destroy(oplist);
		}
		return expectationError(self, "expression");
	}
}

static struct OpList * bitwiseAnd (struct Parser *self, int noIn)
{
	struct OpList *oplist = equality(self, noIn), *alt;
	while (previewToken(self) == '&')
	{
		if (oplist)
		{
			nextToken(self);
			if ((alt = equality(self, noIn)))
			{
				struct Text text = Text.join(oplist->ops->text, alt->ops->text);
				oplist = OpList.unshiftJoin(Op.make(Op.bitwiseAnd, Value(undefined), text), oplist, alt);
				
				continue;
			}
			OpList.destroy(oplist);
		}
		return expectationError(self, "expression");
	}
	return oplist;
}

static struct OpList * bitwiseXor (struct Parser *self, int noIn)
{
	struct OpList *oplist = bitwiseAnd(self, noIn), *alt;
	while (previewToken(self) == '^')
	{
		if (oplist)
		{
			nextToken(self);
			if ((alt = bitwiseAnd(self, noIn)))
			{
				struct Text text = Text.join(oplist->ops->text, alt->ops->text);
				oplist = OpList.unshiftJoin(Op.make(Op.bitwiseXor, Value(undefined), text), oplist, alt);
				
				continue;
			}
			OpList.destroy(oplist);
		}
		return expectationError(self, "expression");
	}
	return oplist;
}

static struct OpList * bitwiseOr (struct Parser *self, int noIn)
{
	struct OpList *oplist = bitwiseXor(self, noIn), *alt;
	while (previewToken(self) == '|')
	{
		if (oplist)
		{
			nextToken(self);
			if ((alt = bitwiseXor(self, noIn)))
			{
				struct Text text = Text.join(oplist->ops->text, alt->ops->text);
				oplist = OpList.unshiftJoin(Op.make(Op.bitwiseOr, Value(undefined), text), oplist, alt);
				
				continue;
			}
			OpList.destroy(oplist);
		}
		return expectationError(self, "expression");
	}
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
			oplist = OpList.unshiftJoin(Op.make(Op.logicalAnd, Value.integer(opCount), OpList.text(oplist)), oplist, nextOp);
		}
		else
			syntaxError(self, text, Chars.create("expected expression, got '%.*s'", text.length, text.bytes));
	
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
			oplist = OpList.unshiftJoin(Op.make(Op.logicalOr, Value.integer(opCount), OpList.text(oplist)), oplist, nextOp);
		}
		else
			syntaxError(self, text, Chars.create("expected expression, got '%.*s'", text.length, text.bytes));
	
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
			oplist = OpList.join3(oplist, trueOps, falseOps);
			
			return oplist;
		}
		else
			syntaxError(self, text, Chars.create("expected expression, got '%.*s'", text.length, text.bytes));
	}
	return oplist;
}

static struct OpList * assignment (struct Parser *self, int noIn)
{
	struct OpList *oplist = conditional(self, noIn), *opassign = NULL;
	struct Text text = self->lexer->text;
	Native(Function) native = NULL;
	
	if (acceptToken(self, '='))
	{
		if (!oplist)
			syntaxError(self, text, Chars.create("expected expression, got '='"));
		else if (oplist->ops[0].native == Op.getLocal && oplist->opCount == 1)
		{
			if (Key.isEqual(oplist->ops[0].value.data.key, Key(eval)))
				syntaxError(self, text, Chars.create("can't assign to eval"));
			else if (Key.isEqual(oplist->ops[0].value.data.key, Key(arguments)))
				syntaxError(self, text, Chars.create("can't assign to arguments"));
			
			oplist->ops->native = Op.setLocal;
		}
		else if (oplist->ops[0].native == Op.getMember)
			oplist->ops->native = Op.setMember;
		else if (oplist->ops[0].native == Op.getProperty)
			oplist->ops->native = Op.setProperty;
		else
			referenceError(self, OpList.text(oplist), Chars.create("invalid assignment left-hand side"));
		
		if (( opassign = assignment(self, noIn) ))
		{
			oplist->ops->text = Text.join(oplist->ops->text, opassign->ops->text);
			return OpList.join(oplist, opassign);
		}
		
		syntaxError(self, self->lexer->text, Chars.create("expected expression, got '%.*s'", self->lexer->text.length, self->lexer->text.bytes));
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
		if (( opassign = assignment(self, noIn) ))
			oplist->ops->text = Text.join(oplist->ops->text, opassign->ops->text);
		else
			syntaxError(self, self->lexer->text, Chars.create("expected expression, got '%.*s'", self->lexer->text.length, self->lexer->text.bytes));
		
		return OpList.unshiftJoin(Op.make(native, Value(undefined), oplist->ops->text), expressionRef(self, oplist, "invalid assignment left-hand side"), opassign);
	}
	
	syntaxError(self, text, Chars.create("expected expression, got '%.*s'", text.length, text.bytes));
	
	return NULL;
}

static struct OpList * expression (struct Parser *self, int noIn)
{
	struct OpList *oplist = assignment(self, noIn);
	while (acceptToken(self, ','))
		oplist = OpList.unshiftJoin(Op.make(Op.discard, Value(undefined), Text(empty)), oplist, assignment(self, noIn));
	
	return oplist;
}


// MARK: Statements

static struct OpList * statementList (struct Parser *self)
{
	struct OpList *oplist = NULL, *statementOps = NULL, *discardOps = NULL;
	uint16_t discardCount = 0;
	
	while (( statementOps = statement(self) ))
	{
		while (statementOps->opCount > 1 && statementOps->ops[0].native == Op.next)
			OpList.shift(statementOps);
		
		if (statementOps->opCount == 1 && statementOps->ops[0].native == Op.next)
			OpList.destroy(statementOps), statementOps = NULL;
		else
		{
			if (statementOps->ops[0].native == Op.discard)
			{
				++discardCount;
				discardOps = OpList.join(discardOps, OpList.shift(statementOps));
				statementOps = NULL;
			}
			else if (discardOps)
			{
				oplist = OpList.joinDiscarded(oplist, discardCount, discardOps);
				discardOps = NULL;
				discardCount = 0;
			}
			
			oplist = OpList.join(oplist, statementOps);
		}
	}
	
	if (discardOps)
		oplist = OpList.joinDiscarded(oplist, discardCount, discardOps);
	
	return oplist;
}

static struct OpList * block (struct Parser *self)
{
	struct OpList *oplist = NULL;
	expectToken(self, '{');
	if (previewToken(self) == '}')
		oplist = OpList.create(Op.next, Value(undefined), self->lexer->text);
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
		syntaxError(self, text, Chars.create("redefining eval is deprecated"));
	else if (Key.isEqual(value.data.key, Key(arguments)))
		syntaxError(self, text, Chars.create("redefining arguments is deprecated"));
	
	Object.addMember(&self->function->environment, value.data.key, Value(undefined), 0);
	
	if (acceptToken(self, '='))
	{
		struct OpList *opassign = assignment(self, noIn);
		if (opassign)
			return OpList.unshiftJoin(Op.make(Op.discard, Value(undefined), Text(empty)), OpList.create(Op.setLocal, value, Text.join(text, opassign->ops->text)), opassign);
		
		syntaxError(self, self->lexer->text, Chars.create("expected expression, got '%.*s'", self->lexer->text.length, self->lexer->text.bytes));
		return NULL;
	}
	else
		return OpList.create(Op.next, value, text);
}

static struct OpList * variableDeclarationList (struct Parser *self, int noIn)
{
	struct OpList *oplist = NULL, *varOps;
	do
	{
		varOps = variableDeclaration(self, noIn);
		
		if (oplist && varOps && varOps->opCount == 1 && varOps->ops[0].native == Op.next)
			OpList.destroy(varOps), varOps = NULL;
		else
			oplist = OpList.join(oplist, varOps);
	}
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
	oplist = OpList.unshiftJoin3(Op.make(Op.jumpIfNot, Value.integer(trueOps->opCount), OpList.text(oplist)), oplist, trueOps, falseOps);
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
	acceptToken(self, ';');
	
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
			oplist = OpList.unshift(Op.make(Op.discard, Value(undefined), OpList.text(oplist)), oplist);
	}
	
	if (oplist && acceptToken(self, Lexer(inToken)))
	{
		if (oplist && oplist->opCount == 2 && oplist->ops[0].native == Op.discard && oplist->ops[1].native == Op.getLocal)
		{
			oplist->ops[0].native = Op.iterateInRef;
			oplist->ops[1].native = Op.getLocalRef;
		}
		else if (oplist->opCount == 1 && oplist->ops[0].native == Op.next)
		{
			oplist->ops->native = Op.getLocalRef;
			oplist = OpList.unshift(Op.make(Op.iterateInRef, Value(undefined), self->lexer->text), oplist);
		}
		else
			referenceError(self, OpList.text(oplist), Chars.create("invalid for/in left-hand side"));
		
		oplist = OpList.join(oplist, expression(self, 0));
		expectToken(self, ')');
		
		pushDepth(self, Key(none), 2);
		body = statement(self);
		popDepth(self);
		
		body = OpList.appendNoop(body);
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
		syntaxError(self, text, Chars.create("continue must be inside loop"));
	
	lastestDepth = 0;
	while (depth--)
	{
		breaker += self->depths[depth].depth;
		if (self->depths[depth].depth)
			lastestDepth = self->depths[depth].depth;
		
		if (lastestDepth == 2)
			if (Key.isEqual(label, Key(none)) || Key.isEqual(label, self->depths[depth].key))
				return OpList.create(Op.breaker, Value.integer(breaker - 1), self->lexer->text);
	}
	syntaxError(self, labelText, Chars.create("label not found"));
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
		syntaxError(self, text, Chars.create("break must be inside loop or switch"));
	
	while (depth--)
	{
		breaker += self->depths[depth].depth;
		if (Key.isEqual(label, Key(none)) || Key.isEqual(label, self->depths[depth].key))
			return OpList.create(Op.breaker, Value.integer(breaker), self->lexer->text);
	}
	syntaxError(self, labelText, Chars.create("label not found"));
	return oplist;
}

static struct OpList * returnStatement (struct Parser *self, struct Text text)
{
	struct OpList *oplist = NULL;
	
	if (self->sourceDepth <= 1)
		syntaxError(self, text, Chars.create("return not in function"));
	
	if (!self->lexer->didLineBreak && previewToken(self) != ';' && previewToken(self) != '}' && previewToken(self) != Lexer(noToken))
		oplist = expression(self, 0);
	
	semicolon(self);
	
	if (!oplist)
		oplist = OpList.create(Op.value, Value(undefined), Text.join(text, self->lexer->text));
	
	oplist = OpList.unshift(Op.make(Op.result, Value(undefined), Text.join(text, oplist->ops->text)), oplist);
	return oplist;
}

static struct OpList * switchStatement (struct Parser *self)
{
	struct OpList *oplist = NULL, *conditionOps = NULL, *defaultOps = NULL;
	struct Text text = Text(empty);
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
			conditionOps = OpList.append(conditionOps, Op.make(Op.value, Value.integer(oplist? oplist->opCount: 0), Text(empty)));
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
				syntaxError(self, text, Chars.create("more than one switch default"));
		}
		else
			syntaxError(self, text, Chars.create("invalid switch statement"));
	}
	
	if (!defaultOps && oplist)
		defaultOps = OpList.create(Op.jump, Value.integer(oplist? oplist->opCount: 0), text);
	
	conditionOps = OpList.unshiftJoin(Op.make(Op.switchOp, Value.integer(conditionOps? conditionOps->opCount: 0), Text(empty)), conditionOps, defaultOps);
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
		return OpList.create(Op.next, Value(undefined), text);
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
		syntaxError(self, text, Chars.create("strict mode code may not contain 'with' statements"));
		return oplist;
	}
	else if (acceptToken(self, Lexer(switchToken)))
		return switchStatement(self);
	else if (acceptToken(self, Lexer(throwToken)))
	{
		if (!self->lexer->didLineBreak && previewToken(self) != ';' && previewToken(self) != '}' && previewToken(self) != Lexer(noToken))
			oplist = expression(self, 0);
		
		if (!oplist)
			syntaxError(self, text, Chars.create("throw statement is missing an expression"));
		
		semicolon(self);
		return OpList.unshift(Op.make(Op.throw, Value(undefined), Text.join(text, OpList.text(oplist))), oplist);
	}
	else if (acceptToken(self, Lexer(tryToken)))
	{
		oplist = block(self);
		if (!oplist)
			return NULL;
		
		oplist = OpList.unshift(Op.make(Op.try, Value.integer(oplist->opCount), text), oplist);
		
		if (previewToken(self) != Lexer(catchToken) && previewToken(self) != Lexer(finallyToken))
			syntaxError(self, self->lexer->text, Chars.create("expected catch or finally, got %s", Lexer.tokenChars(previewToken(self))));
		
		if (acceptToken(self, Lexer(catchToken)))
		{
			struct Op identiferOp;
			struct OpList *catchOps;
			
			expectToken(self, '(');
			if (previewToken(self) != Lexer(identifierToken))
				syntaxError(self, text, Chars.create("missing identifier in catch"));
			
			identiferOp = identifier(self);
			expectToken(self, ')');
			
			catchOps = block(self);
			if (catchOps)
			{
				catchOps = OpList.unshift(Op.make(Op.pushEnvironment, Value.key(identiferOp.value.data.key), text), catchOps);
				catchOps = OpList.append(catchOps, Op.make(Op.popEnvironment, Value(undefined), text));
				catchOps = OpList.unshift(Op.make(Op.jump, Value.integer(catchOps->opCount), text), catchOps);
				oplist = OpList.join(oplist, catchOps);
			}
		}
		else
			oplist = OpList.append(OpList.append(oplist, Op.make(Op.jump, Value.integer(1), text)), Op.make(Op.noop, Value(undefined), text));
		
		if (acceptToken(self, Lexer(finallyToken)))
			oplist = OpList.join(oplist, block(self));
		
		return OpList.appendNoop(oplist);
	}
	else if (acceptToken(self, Lexer(debuggerToken)))
	{
		semicolon(self);
		return OpList.create(Op.debugger, Value(undefined), text);
	}
	else
	{
		uint32_t index;
		
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
		
		index = oplist->opCount;
		while (index--)
			if (oplist->ops[index].native == Op.call)
				return OpList.unshift(Op.make(self->sourceDepth <=1 ? Op.autoreleaseExpression: Op.autoreleaseDiscard, Value(undefined), Text(empty)), oplist);
		
		return OpList.unshift(Op.make(self->sourceDepth <=1 ? Op.expression: Op.discard, Value(undefined), Text(empty)), oplist);
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
					syntaxError(self, op.text, Chars.create("redefining eval is deprecated"));
				else if (Key.isEqual(op.value.data.key, Key(arguments)))
					syntaxError(self, op.text, Chars.create("redefining arguments is deprecated"));
				
				Object.addMember(&self->function->environment, op.value.data.key, Value(undefined), Value(hidden));
			}
		} while (acceptToken(self, ','));
	
	return NULL;
}

static struct OpList * function (struct Parser *self, int isDeclaration, int isGetter, int isSetter)
{
	struct Value value;
	struct Text text = self->lexer->text, textParameter;
	
	struct OpList *oplist = NULL;
	int parameterCount = 0;
	
	struct Op identifierOp = { 0, Value(undefined) };
	struct Function *parentFunction;
	struct Function *function;
	
	if (!isGetter && !isSetter)
	{
		expectToken(self, Lexer(functionToken));
		
		if (previewToken(self) == Lexer(identifierToken))
		{
			identifierOp = identifier(self);
			
			if (Key.isEqual(identifierOp.value.data.key, Key(eval)))
				syntaxError(self, identifierOp.text, Chars.create("redefining eval is deprecated"));
			else if (Key.isEqual(identifierOp.value.data.key, Key(arguments)))
				syntaxError(self, identifierOp.text, Chars.create("redefining arguments is deprecated"));
		}
		else if (isDeclaration)
		{
			syntaxError(self, self->lexer->text, Chars.create("function statement requires a name"));
			return NULL;
		}
	}
	
	parentFunction = self->function;
	parentFunction->flags |= Function(needHeap);
	
	function = Function.create(&self->function->environment);
	Function.linkPrototype(function, Value.object(Object.create(Object(prototype))));
	
	Object.addMember(&function->environment, Key(arguments), Value(undefined), Value(hidden) | Value(sealed));
	
	self->function = function;
	expectToken(self, '(');
	textParameter = self->lexer->text;
	oplist = OpList.join(oplist, parameters(self, &parameterCount));
	
	if (isGetter && parameterCount != 0)
		syntaxError(self, Text.make(textParameter.bytes, self->lexer->text.bytes - textParameter.bytes), Chars.create("getter functions must have no arguments"));
	else if (isSetter && parameterCount != 1)
		syntaxError(self, Text.make(self->lexer->text.bytes, 0), Chars.create("setter functions must have one argument"));
	
	expectToken(self, ')');
	expectToken(self, '{');
	oplist = OpList.join(oplist, sourceElements(self, '}'));
	text.length = self->lexer->text.bytes - text.bytes + 1;
	expectToken(self, '}');
	self->function = parentFunction;
	
	function->oplist = oplist;
	function->text = text;
	function->parameterCount = parameterCount;
	
	Object.addMember(&function->object, Key(length), Value.integer(parameterCount), Value(readonly) | Value(hidden));
	
	value = Value.function(function);
	
	if (isDeclaration)
		Object.addMember(&parentFunction->environment, identifierOp.value.data.key, Value(undefined), Value(hidden));
	else if (identifierOp.value.type != Value(undefinedType) && !isGetter && !isSetter)
		Object.addMember(&function->environment, identifierOp.value.data.key, value, Value(hidden));
	
	if (isGetter)
		value.flags |= Value(getter);
	else if (isSetter)
		value.flags |= Value(setter);
	
	if (isDeclaration)
		return OpList.append(OpList.create(Op.setLocal, identifierOp.value, Text(empty)), Op.make(Op.function, value, text));
	else
		return OpList.create(Op.function, value, text);
}


// MARK: Source

static struct OpList * sourceElements (struct Parser *self, enum Lexer(Token) endToken)
{
	struct OpList *oplist = NULL, *init = NULL, *last = NULL, *statementOps = NULL, *discardOps = NULL;
	uint16_t discardCount = 0, functionDiscard = 0;
	
	++self->sourceDepth;
	
	while (previewToken(self) != endToken && previewToken(self) != Lexer(errorToken) && previewToken(self) != Lexer(noToken))
		if (previewToken(self) == Lexer(functionToken))
		{
			++functionDiscard;
			init = OpList.join(init, function(self, 1, 0, 0));
		}
		else
		{
			statementOps = statement(self);
			if (statementOps)
			{
				while (statementOps->opCount > 1 && statementOps->ops[0].native == Op.next)
					OpList.shift(statementOps);
				
				if (statementOps->opCount == 1 && statementOps->ops[0].native == Op.next)
					OpList.destroy(statementOps), statementOps = NULL;
				else
				{
					if (statementOps->ops[0].native == Op.discard)
					{
						++discardCount;
						discardOps = OpList.join(discardOps, OpList.shift(statementOps));
						statementOps = NULL;
					}
					else if (discardOps)
					{
						oplist = OpList.joinDiscarded(oplist, discardCount, discardOps);
						discardOps = NULL;
						discardCount = 0;
					}
					
					oplist = OpList.join(oplist, last);
					last = statementOps;
				}
			}
			else
				syntaxError(self, self->lexer->text, Chars.create("expected statement, got %s", Lexer.tokenChars(previewToken(self))));
		}
	
	if (init)
		init = OpList.joinDiscarded(NULL, functionDiscard, init);
	
	if (discardOps)
		oplist = OpList.joinDiscarded(oplist, discardCount, discardOps);
	
	if (!last || last->ops[last->opCount - 1].native != Op.result)
	{
		if (self->sourceDepth <= 1)
			last = OpList.appendNoop(last);
		else
			last = OpList.append(last, Op.make(Op.resultVoid, Value(undefined), Text(empty)));
	}
	
	oplist = OpList.join(init, oplist);
	oplist = OpList.join(oplist, last);
	
	Object.packValue(&self->function->environment);
	
	--self->sourceDepth;
	
	return oplist;
}


// MARK: - Methods

struct Parser * createWithLexer (struct Lexer *lexer)
{
	struct Parser *self = malloc(sizeof(*self));
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

struct Function * parseWithEnvironment (struct Parser * const self, struct Object *environment)
{
	struct Function *function;
	struct OpList *oplist;
	
	assert(self);
	
	function = Function.create(environment);
	nextToken(self);
	
	self->function = function;
	oplist = sourceElements(self, Lexer(noToken));
	self->function = NULL;
	
	OpList.optimizeWithEnvironment(oplist, &function->environment);
	
	if (self->error)
	{
		struct Op errorOps[] = {
			{ Op.throw, Value(undefined), self->error->text },
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

//
//  regexp.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#define Implementation
#include "regexp.h"

#include "../pool.h"
#include "../ecc.h"
#include "../lexer.h"

enum Opcode {
	opOver = 0,
	opNLookahead = 1,
	opLookahead = 2,
	opStart,
	opEnd,
	opBoundary,
	
	opSplit,
	opReference,
	opRedo,
	opSave,
	opAny,
	opOneOf,
	opNeitherOf,
	opDigit,
	opSpace,
	opWord,
	opBytes,
	opJump,
	opMatch,
};

struct RegExp(Node) {
	char *bytes;
	int16_t offset;
	uint8_t opcode;
	uint8_t depth;
};

struct Parse {
	const char *c;
	const char *end;
	uint16_t count;
};

enum Flags {
	infiniteLoop = 1 << 1,
};

// MARK: - Private

struct Object * RegExp(prototype) = NULL;
struct Function * RegExp(constructor) = NULL;

void finalize (struct Object *object)
{
	struct RegExp *self = (struct RegExp *)object;
	struct RegExp(Node) *n = self->program;
	while (n->opcode != opOver)
	{
		if (n->bytes)
			free(n->bytes), n->bytes = NULL;
		
		++n;
	}
	
	free(self->program), self->program = NULL;
}

const struct Object(Type) RegExp(type) = {
	.text = &Text(regexpType),
	
	.finalize = finalize,
};

#if 0
static
void printNode (struct RegExp(Node) *n)
{
	switch (n->opcode)
	{
		case opNLookahead: fprintf(stderr, "!lookahead "); break;
		case opLookahead: fprintf(stderr, "lookahead "); break;
		case opReference: fprintf(stderr, "reference "); break;
		case opStart: fprintf(stderr, "start "); break;
		case opEnd: fprintf(stderr, "end "); break;
		case opBoundary: fprintf(stderr, "boundary "); break;
		case opSplit: fprintf(stderr, "split "); break;
		case opRedo: fprintf(stderr, "redo "); break;
		case opSave: fprintf(stderr, "save "); break;
		case opAny: fprintf(stderr, "any "); break;
		case opOneOf: fprintf(stderr, "oneof "); break;
		case opDigit: fprintf(stderr, "digit "); break;
		case opSpace: fprintf(stderr, "space "); break;
		case opWord: fprintf(stderr, "word "); break;
		case opBytes: fprintf(stderr, "bytes "); break;
		case opJump: fprintf(stderr, "jump "); break;
		case opMatch: fprintf(stderr, "match "); break;
		case opOver: fprintf(stderr, "over "); break;
	}
	fprintf(stderr, "%d", n->offset);
	if (n->bytes)
	{
		if (n->opcode == opRedo)
		{
			char *c = n->bytes + 1;
			fprintf(stderr, " {%u-%u} (:", n->bytes[0], n->bytes[1]);
			while (*(++c))
				fprintf(stderr, "%u,", *c);
			
			fprintf(stderr, ")");
		}
		else if (n->opcode != opRedo)
			fprintf(stderr, " `%s`", n->bytes);
	}
	putc('\n', stderr);
}
#endif

//MARK: parsing

static
struct RegExp(Node) * node (enum Opcode opcode, long offset, const char *bytes)
{
	struct RegExp(Node) *n = calloc(2, sizeof(*n));
	
	n[0].bytes = bytes && strlen(bytes)? strdup(bytes): NULL;
	n[0].offset = offset;
	n[0].opcode = opcode;
	
	return n;
}

static
void toss (struct RegExp(Node) *node)
{
	struct RegExp(Node) *n = node;
	
	if (!node)
		return;
	
	while (n->opcode != opOver)
	{
		free(n->bytes), n->bytes = NULL;
		++n;
	}
	
	free(node), node = NULL;
}

static
uint16_t nlen (struct RegExp(Node) *n)
{
	uint16_t len = 0;
	if (n)
		while (n[++len].opcode != opOver);
	
	return len;
}

static
struct RegExp(Node) *join (struct RegExp(Node) *a, struct RegExp(Node) *b)
{
	uint16_t lena = 0, lenb = 0;
	
	if (!a)
		return b;
	else if (!b)
		return a;
	
	while (a[++lena].opcode != opOver);
	while (b[++lenb].opcode != opOver);
	
	if (lenb == 1 && a[lena - 1].opcode == opBytes && b->opcode == opBytes)
	{
		struct RegExp(Node) *c = a + lena - 1;
		
		c->bytes = realloc(c->bytes, c->offset + b->offset + 1);
		memcpy(c->bytes + c->offset, b->bytes, b->offset + 1);
		c->offset += b->offset;
		free(b->bytes), b->bytes = NULL;
	}
	else
	{
		a = realloc(a, sizeof(*a) * (lena + lenb + 1));
		memcpy(a + lena, b, sizeof(*a) * (lenb + 1));
	}
	free(b), b = NULL;
	return a;
}

static
int accept(struct Parse *p, char c)
{
	if (*p->c == c)
	{
		++p->c;
		return 1;
	}
	return 0;
}

static
int charLength(const char *c)
{
	if ((c[0] & 0xf8) == 0xf0 && (c[1] & 0xc0) == 0x80 && (c[2] & 0xc0) == 0x80 && (c[3] & 0xc0) == 0x80)
		return 4;
	else if ((c[0] & 0xf0) == 0xe0 && (c[1] & 0xc0) == 0x80 && (c[2] & 0xc0) == 0x80)
		return 3;
	else if ((c[0] & 0xe0) == 0xc0 && (c[1] & 0xc0) == 0x80)
		return 2;
	else
		return 1;
}

static
int getU(struct Parse *p, char buffer[5])
{
	int i = 0, l = charLength(p->c);
	do
		buffer[i] = p->c[i];
	while (++i < l);
	buffer[l] = '\0';
	p->c += l;
	return l;
}

static struct RegExp(Node) * disjunction (struct Parse *p, struct Error **error);

static
struct RegExp(Node) * term (struct Parse *p, struct Error **error)
{
	char buffer[5] = { 0 };
	
	if (*p->c == '/')
		return NULL;
	else if (accept(p, '^'))
		return node(opStart, 0, NULL);
	else if (accept(p, '$'))
		return node(opEnd, 0, NULL);
	else if (accept(p, '\\'))
	{
		buffer[0] = *(p->c++);
		
		switch (buffer[0])
		{
			case 'b': return node(opBoundary, 1, NULL);
			case 'B': return node(opBoundary, 0, NULL);
			case 'f': return node(opBytes, 1, "\f");
			case 'n': return node(opBytes, 1, "\n");
			case 'r': return node(opBytes, 1, "\r");
			case 't': return node(opBytes, 1, "\t");
			case 'v': return node(opBytes, 1, "\v");
			case 'd': return node(opDigit, 1, NULL);
			case 'D': return node(opDigit, 0, NULL);
			case 's': return node(opSpace, 1, NULL);
			case 'S': return node(opSpace, 0, NULL);
			case 'w': return node(opWord, 1, NULL);
			case 'W': return node(opWord, 0, NULL);
			case 'c':
				buffer[0] &= 31;
				break;
			
			case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
			{
				int c = buffer[0] - '0';
				while (isdigit(*(p->c + 1)))
					c = c * 10 + *(++p->c);
				
				return node(opReference, c, NULL);
			}
			case '0':
				buffer[0] = *p->c - '0';
				if (buffer[0] >= 0 && buffer[0] <= 7)
				{
					if (*p->c >= '0' && *p->c <= '7')
					{
						buffer[0] = buffer[0] * 8 + *(++p->c) - '0';
						if (*p->c >= '0' && *p->c <= '7')
							buffer[0] = buffer[0] * 8 + *(++p->c) - '0';
					}
				}
				break;
			
			case 'x':
				if (isxdigit(p->c[0]) && isxdigit(p->c[1]))
				{
					buffer[0] = Lexer.uint8Hex(p->c[0], p->c[1]);
					p->c += 2;
				}
				break;
				
			case 'u':
				if (isxdigit(p->c[0]) && isxdigit(p->c[1]) && isxdigit(p->c[2]) && isxdigit(p->c[3]))
				{
					uint16_t c = Lexer.uint16Hex(p->c[0], p->c[1], p->c[2], p->c[3]);
					char *b = buffer;
					
					if (c < 0x80) *b++ = c;
					else if (c < 0x800) *b++ = 192 + c / 64, *b++ = 128 + c % 64;
					else *b++ = 224 + c / 4096, *b++ = 128 + c / 64 % 64, *b++ = 128 + c % 64;
					
					p->c += 4;
					return node(opBytes, (int16_t)(b - buffer) - 1, buffer);
				}
				break;
				
			default:
				break;
		}
		return node(opBytes, 1, buffer);
	}
	else if (accept(p, '('))
	{
		struct RegExp(Node) *n;
		unsigned char count = 0;
		char modifier = '\0';
		
		if (accept(p, '?'))
		{
			if (*p->c == '=' || *p->c == '!' || *p->c == ':')
				modifier = *(p->c++);
		}
		else
		{
			count = ++p->count;
			if ((int)count * 2 + 1 > 0xff)
			{
				*error = Error.syntaxError(Text.make(p->c, 1), Chars.create("too many captures"));
				return NULL;
			}
		}
		
		n = disjunction(p, error);
		if (!accept(p, ')'))
		{
			*error = Error.syntaxError(Text.make(p->c, 1), Chars.create("expect ')'"));
			return NULL;
		}
		
		switch (modifier) {
			case '\0': return join(node(opSave, count * 2, NULL), join(n, node(opSave, count * 2 + 1, NULL)));
			case '=': return join(node(opLookahead, nlen(n) + 1, NULL), n);
			case '!': return join(node(opNLookahead, nlen(n) + 1, NULL), n);
			case ':': return n;
		}
	}
	else if (accept(p, '.'))
		return node(opAny, 0, NULL);
	else if (accept(p, '['))
	{
		int not = accept(p, '^');
		const char *start = p->c;
		
		while (*(p->c++) != ']')
		{
			if (p->c >= p->end)
			{
				*error = Error.syntaxError(Text.make(p->c - 1, 1), Chars.create("expect ']'"));
				return NULL;
			}
		}
		
		{
			int16_t len = p->c - start - 1;
			char buffer[len + 1];
			memcpy(buffer, start, len);
			buffer[len] = '\0';
			accept(p, ']');
			return node(not? opNeitherOf: opOneOf, 0, buffer);
		}
	}
	else if (!*p->c)
	{
		char *bytes = calloc(2, 1);
		struct RegExp(Node) *n = node(opBytes, 1, NULL);
		n->bytes = bytes;
		p->c += 1;
		return n;
	}
	else if (strchr("*+?)]}|", *p->c))
		return NULL;
	
	return node(opBytes, getU(p, buffer), buffer);
}

static
struct RegExp(Node) * alternative (struct Parse *p, struct Error **error)
{
	struct RegExp(Node) *n = NULL, *t = NULL;
	
	for (;;)
	{
		if (!(t = term(p, error)))
			break;
		
		if (t->opcode > opBoundary)
		{
			int noop = 0, lazy = 0;
			
			uint8_t quantifier =
				accept(p, '?')? '?':
				accept(p, '*')? '*':
				accept(p, '+')? '+':
				accept(p, '{')? '{':
				'\0',
				min = 1,
				max = 1;
			
			switch (quantifier)
			{
				case '?':
					min = 0, max = 1;
					break;
					
				case '*':
					min = 0, max = 0;
					break;
					
				case '+':
					min = 1, max = 0;
					break;
					
				case '{':
				{
					
					if (isdigit(*p->c))
					{
						min = *(p->c++) - '0';
						while (isdigit(*p->c))
							min = min * 10 + *(p->c++) - '0';
					}
					else
					{
						*error = Error.syntaxError(Text.make(p->c, 1), Chars.create("expect number"));
						goto error;
					}
					
					if (accept(p, ','))
					{
						if (isdigit(*p->c))
						{
							max = *(p->c++) - '0';
							while (isdigit(*p->c))
								max = max * 10 + *(p->c++) - '0';
							
							if (!max)
								noop = 1;
						}
						else
							max = 0;
					}
					else if (!min)
						noop = 1;
					else
						max = min;
					
					if (!accept(p, '}'))
					{
						*error = Error.syntaxError(Text.make(p->c, 1), Chars.create("expect '}'"));
						goto error;
					}
					break;
				}
			}
			
			lazy = accept(p, '?');
			if (noop)
			{
				toss(t);
				continue;
			}
			
			if (max != 1)
			{
				struct RegExp(Node) *redo;
				uint16_t index, count = nlen(t), length = 2;
				char buffer[count + 2];
				
				for (index = 0; index < count; ++index)
					if (t[index].opcode == opSave)
						buffer[length++] = (uint8_t)t[index].offset;
				
				buffer[0] = min;
				buffer[1] = max;
				buffer[length] = '\0';
				
				if (lazy)
					redo = join(node(opRedo, 2, NULL), node(opJump, -nlen(t) - 1, NULL));
				else
					redo = node(opRedo, -nlen(t), NULL);
				
				redo->bytes = malloc(length + 1);
				memcpy(redo->bytes, buffer, length + 1);
				
				t = join(t, redo);
			}
			
			if (min == 0)
			{
				if (lazy)
					t = join(node(opSplit, 2, NULL), join(node(opJump, nlen(t) + 1, NULL), t));
				else
					t = join(node(opSplit, nlen(t) + 1, NULL), t);
			}
		}
		n = join(n, t);
	}
	return n;
	
error:
	toss(t);
	return n;
}

static
struct RegExp(Node) * disjunction (struct Parse *p, struct Error **error)
{
	struct RegExp(Node) *n = alternative(p, error), *d;
	if (accept(p, '|'))
	{
		uint16_t len;
		d = disjunction(p, error);
		n = join(n, node(opJump, nlen(d) + 1, NULL));
		len = nlen(n);
		n = join(n, d);
		n = join(node(opSplit, len + 1, NULL), n);
	}
	return n;
}

static
struct RegExp(Node) * pattern (struct Parse *p, struct Error **error)
{
	assert(*p->c == '/');
	++p->c;
	
	return join(disjunction(p, error), node(opMatch, 0, NULL));
}


//MARK: matching

int isword (const char *c)
{
	return isalnum(*c) || *c == '_';
}

static
int match (struct RegExp(State) * const s, struct RegExp(Node) *n, const char *c);

static
void clear (struct RegExp(State) * const s, const char *c, uint8_t *bytes)
{
	uint8_t index;
	
	if (!bytes)
		return;
	
	while (*bytes)
	{
		index = *bytes++;
		s->index[index] = index % 2? 0: c;
	}
}

static
int forkMatch (struct RegExp(State) * const s, struct RegExp(Node) *n, const char *c, int16_t offset)
{
	int result;
	
	if (n->depth == 0xff)
		return 0; //!\\ too many recursion
	else
		++n->depth;
	
	result = match(s, n + offset, c);
	--n->depth;
	return result;
}

int match (struct RegExp(State) * const s, struct RegExp(Node) *n, const char *c)
{
	goto start;
next:
	++n;
start:
	;
	
	switch((enum Opcode)n->opcode)
	{
		case opNLookahead:
		case opLookahead:
			if (forkMatch(s, n, c, 1) == (n->opcode - 1))
				goto jump;
			
			return 0;
			
		case opStart:
			if (c != s->start)
				return 0;
			
			goto next;
			
		case opEnd:
			if (c != s->end)
				return 0;
			
			goto next;
			
		case opBoundary:
			if (c == s->start || c == s->end)
			{
				if (isword(c) != n->offset)
					return 0;
			}
			else if ((isword(c - 1) != isword(c)) != n->offset)
				return 0;
			
			goto next;
			
		case opSplit:
			if (c == n->bytes)
			{
				s->flags |= infiniteLoop;
				return 0;
			}
			else
				n->bytes = (char *)c;
			
			if (forkMatch(s, n, c, 1))
				return 1;
			
			goto jump;
			
		case opReference:
		{
			ptrdiff_t len;
			
			if (c == n->bytes)
			{
				s->flags |= infiniteLoop;
				return 0;
			}
			else
				n->bytes = (char *)c;
			
			len = s->capture[n->offset * 2 + 1]? s->capture[n->offset * 2 + 1] - s->capture[n->offset * 2]: 0;
			if (len && memcmp(c, s->capture[n->offset * 2], len))
				return 0;
			
			c += len;
			if (c > s->end)
				return 0;
			
			goto next;
		}
			
		case opRedo:
			if (n->bytes[1] && n->depth >= n->bytes[1])
				return 0;
			
			s->flags &= ~infiniteLoop;
			
			if (forkMatch(s, n, c, n->offset))
			{
				clear(s, c, (uint8_t *)n->bytes + 2);
				return 1;
			}
			
			if (n->depth + 1 < n->bytes[0])
				return 0;
			
			if (s->flags & infiniteLoop)
				clear(s, c, (uint8_t *)n->bytes + 2);
			
			goto next;
			
		case opSave:
			if (forkMatch(s, n, c, 1)) {
				if (s->capture[n->offset] < c && c > s->index[n->offset]) {
					s->capture[n->offset] = c;
				}
				return 1;
			}
			return 0;
			
		case opDigit:
			if ((!isdigit(*c)) == n->offset)
				return 0;
			
			while ((!isdigit(*(++c))) != n->offset);
			goto next;
			
		case opSpace:
			if ((!isspace(*c)) == n->offset)
				return 0;
			
			while ((!isspace(*(++c))) != n->offset);
			goto next;
			
		case opWord:
			if (isword(c) != n->offset)
				return 0;
			
			do
				++c;
			while (isword(c));
			goto next;
			
		case opBytes:
			if (memcmp(n->bytes, c, n->offset))
				return 0;
			
			c += n->offset;
			if (c > s->end)
				return 0;
			
			goto next;
			
		case opOneOf:
		{
			const char *set = n->bytes;
			int len;
			
			while (*set)
			{
				len = charLength(set);
				if (!memcmp(c, set, len))
				{
					c += len;
					if (c > s->end)
						return 0;
					
					goto next;
				}
				set += len;
			}
			return 0;
		}
			
		case opNeitherOf:
		{
			const char *set = n->bytes;
			int len;
			
			while (*set)
			{
				len = charLength(set);
				if (!memcmp(c, set, len))
					return 0;
				
				set += len;
			}
			c += charLength(c);
			if (c > s->end)
				return 0;
			
			goto next;
		}
			
		case opAny:
			if (*c != '\r' && *c != '\n')
				goto next;
			
			return 0;
			
		case opJump:
			goto jump;
			
		case opMatch:
			s->capture[1] = c;
			return 1;
			
		case opOver:
			break;
	}
	abort();
	
jump:
	n += n->offset;
	goto start;
}


// MARK: - Static Members

static struct Value constructor (struct Context * const context)
{
	struct Error *error = NULL;
	struct Value pattern, flags;
	struct Chars *chars;
	struct RegExp *regexp;
	
	Context.assertParameterCount(context, 2);
	
	pattern = Context.argument(context, 0);
	flags = Context.argument(context, 1);
	
	if (pattern.type == Value(regexpType) && flags.type == Value(undefinedType))
	{
		if (context->construct)
			chars = pattern.data.regexp->pattern;
		else
			return pattern;
	}
	else
	{
		Chars.beginAppend(&chars);
		Chars.append(&chars, "/");
		
		if (pattern.type == Value(regexpType))
			Chars.appendValue(&chars, context, Value.chars(pattern.data.regexp->source));
		else
			Chars.appendValue(&chars, context, pattern);
		
		Chars.append(&chars, "/");
		Chars.appendValue(&chars, context, flags);
		Chars.endAppend(&chars);
	}
	
	regexp = create(chars, &error);
	if (error)
	{
		struct Context *c = context;
		while (!c->text && c->parent)
			c = c->parent;
		
		if (c->text)
			error->text = *c->text;
		
		Context.throw(context, Value.error(error));
	}
	return Value.regexp(regexp);
}

static struct Value toString (struct Context * const context)
{
	struct RegExp *self = context->this.data.regexp;
	
	Context.assertParameterCount(context, 0);
	Context.assertThisType(context, Value(regexpType));
	
	if (self->program[0].opcode == opMatch)
		return Value.text(&Text(emptyRegExp));
	else
		return Value.chars(self->pattern);
}

static struct Value exec (struct Context * const context)
{
	struct RegExp *self = context->this.data.regexp;
	struct Value value;
	
	Context.assertParameterCount(context, 2);
	Context.assertThisType(context, Value(regexpType));
	
	value = Value.toString(context, Context.argument(context, 0));
	{
		uint16_t length = Value.stringLength(value);
		const char *bytes = Value.stringBytes(value);
		const char *capture[2 + self->count * 2];
		const char *index[2 + self->count * 2];
		struct RegExp(State) state = { bytes, bytes + length, capture, index };
		struct Chars *element;
		
		if (matchWithState(self, &state))
		{
			struct Object *array = Array.createSized(self->count);
			int index, count;
			
			for (index = 0, count = self->count; index < count; ++index)
			{
				if (capture[index * 2])
				{
					element = Chars.createWithBytes(capture[index * 2 + 1] - capture[index * 2], capture[index * 2]);
#warning TODO: referenceCount
					++element->referenceCount;
					array->element[index].value = Value.chars(element);
				}
				else
					array->element[index].value = Value(undefined);
			}
			return Value.object(array);
		}
	}
	return Value(null);
}

// MARK: - Methods

void setup ()
{
	const enum Value(Flags) flags = Value(hidden);
	
	Function.setupBuiltinObject(
		&RegExp(constructor), constructor, 2,
		&RegExp(prototype), Value.regexp(create(Chars.create("//"), NULL)),
		&RegExp(type));
	
	Function.addToObject(RegExp(prototype), "toString", toString, 0, flags);
	Function.addToObject(RegExp(prototype), "exec", exec, 2, flags);
}

void teardown (void)
{
	RegExp(prototype) = NULL;
	RegExp(constructor) = NULL;
}

struct RegExp * create (struct Chars *s, struct Error **error)
{
	struct Parse p = { 0 };
	
	struct RegExp *self = malloc(sizeof(*self));
	*self = RegExp.identity;
	Pool.addObject(&self->object);
	
	Object.initialize(&self->object, RegExp(prototype));
	
	p.c = s->bytes;
	p.end = s->bytes + s->length;
	
	self->pattern = s;
	self->program = pattern(&p, error);
	self->count = p.count + 1;
	self->source = Chars.createWithBytes(p.c - self->pattern->bytes - 1, self->pattern->bytes + 1);
	
	++self->pattern->referenceCount;
	++self->source->referenceCount;
	
	if (*p.c == '/')
		for (;;)
		{
			switch (*(++p.c)) {
				case 'g':
					if (self->global == 1)
						*error = Error.syntaxError(Text.make(p.c, 1), Chars.create("invalid flags"));
					
					self->global = 1;
					continue;
					
				case 'i':
					if (self->ignoreCase == 1)
						*error = Error.syntaxError(Text.make(p.c, 1), Chars.create("invalid flags"));
					
					self->ignoreCase = 1;
					continue;
					
				case 'm':
					if (self->multiline == 1)
						*error = Error.syntaxError(Text.make(p.c, 1), Chars.create("invalid flags"));
					
					self->multiline = 1;
					continue;
			}
			break;
		}
	else if (!*error)
		*error = Error.syntaxError(Text.make(p.c, 1), Chars.create("invalid character"));
	
	Object.addMember(&self->object, Key(source), Value.chars(self->source), Value(readonly) | Value(hidden) | Value(sealed));
	Object.addMember(&self->object, Key(global), Value.truth(self->global), Value(readonly) | Value(hidden) | Value(sealed));
	Object.addMember(&self->object, Key(ignoreCase), Value.truth(self->ignoreCase), Value(readonly) | Value(hidden) | Value(sealed));
	Object.addMember(&self->object, Key(multiline), Value.truth(self->multiline), Value(readonly) | Value(hidden) | Value(sealed));
	Object.addMember(&self->object, Key(lastIndex), Value.integer(0), Value(hidden) | Value(sealed));
	
	return self;
}

int matchWithState (struct RegExp *self, struct RegExp(State) *state)
{
	const char *chars = state->start;
	int result = 0;
	uint16_t index, count;
	
#if 0
	fprintf(stderr, "\n%.*s\n", self->pattern->length, self->pattern->bytes);
	struct RegExp(Node) *n = self->program;
	while (n->opcode != opOver)
		printNode(n++);
#endif
	
	while (!result && chars < state->end)
	{
		memset(state->capture, 0, sizeof(*state->capture) * (2 + self->count * 2));
		memset(state->index, 0, sizeof(*state->index) * (2 + self->count * 2));
		result = match(state, self->program, state->capture[0] = state->index[0] = chars);
		chars += charLength(chars);
	}
	
	/* XXX: cleanup */
	
	for (index = 0, count = nlen(self->program); index < count; ++index)
	{
		if (self->program[index].opcode == opSplit || self->program[index].opcode == opReference)
			self->program[index].bytes = NULL;
		
		self->program[index].depth = 0;
	}
	
	return result;
}

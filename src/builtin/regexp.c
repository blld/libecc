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

// MARK: - Private

static void mark (struct Object *object);
static void capture (struct Object *object);
static void finalize (struct Object *object);

struct Object * RegExp(prototype) = NULL;
struct Function * RegExp(constructor) = NULL;

const struct Object(Type) RegExp(type) = {
	.text = &Text(regexpType),
	.mark = mark,
	.capture = capture,
	.finalize = finalize,
};

#define DUMP_REGEXP 0

enum Opcode {
	opOver = 0,
	opNLookahead = 1,
	opLookahead = 2,
	opStart,
	opEnd,
	opLineStart,
	opLineEnd,
	opBoundary,
	
	opSplit,
	opReference,
	opRedo,
	opSave,
	opNSave,
	opAny,
	opOneOf,
	opNeitherOf,
	opInRange,
	opInRangeCase,
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
	int ignoreCase;
	int multiline;
	int disallowQuantifier;
};

static struct RegExp(Node) * disjunction (struct Parse *p, struct Error **error);

static
void mark (struct Object *object)
{
	struct RegExp *self = (struct RegExp *)object;
	
	Pool.markValue(Value.chars(self->pattern));
	Pool.markValue(Value.chars(self->source));
}

static
void capture (struct Object *object)
{
	struct RegExp *self = (struct RegExp *)object;
	
	++self->pattern->referenceCount;
	++self->source->referenceCount;
}

static
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
	
	--self->pattern->referenceCount;
	--self->source->referenceCount;
}

#if DUMP_REGEXP
static
void printNode (struct RegExp(Node) *n)
{
	switch (n->opcode)
	{
		case opOver: fprintf(stderr, "over "); break;
		case opNLookahead: fprintf(stderr, "!lookahead "); break;
		case opLookahead: fprintf(stderr, "lookahead "); break;
		case opStart: fprintf(stderr, "start "); break;
		case opEnd: fprintf(stderr, "end "); break;
		case opLineStart: fprintf(stderr, "line start "); break;
		case opLineEnd: fprintf(stderr, "line end "); break;
		case opBoundary: fprintf(stderr, "boundary "); break;
		
		case opSplit: fprintf(stderr, "split "); break;
		case opReference: fprintf(stderr, "reference "); break;
		case opRedo: fprintf(stderr, "redo "); break;
		case opSave: fprintf(stderr, "save "); break;
		case opNSave: fprintf(stderr, "!save "); break;
		case opAny: fprintf(stderr, "any "); break;
		case opOneOf: fprintf(stderr, "one of "); break;
		case opNeitherOf: fprintf(stderr, "neither of "); break;
		case opInRange: fprintf(stderr, "in range "); break;
		case opInRangeCase: fprintf(stderr, "in range (ignore case) "); break;
		case opDigit: fprintf(stderr, "digit "); break;
		case opSpace: fprintf(stderr, "space "); break;
		case opWord: fprintf(stderr, "word "); break;
		case opBytes: fprintf(stderr, "bytes "); break;
		case opJump: fprintf(stderr, "jump "); break;
		case opMatch: fprintf(stderr, "match "); break;
	}
	fprintf(stderr, "%d", n->offset);
	if (n->bytes)
	{
		if (n->opcode == opRedo)
		{
			char *c = n->bytes + 2;
			fprintf(stderr, " {%u-%u}", n->bytes[0], n->bytes[1]);
			if (n->bytes[2])
				fprintf(stderr, " lazy");
			
			if (*(c + 1))
				fprintf(stderr, " clear:");
			
			while (*(++c))
				fprintf(stderr, "%u,", *c);
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
	
	if (offset && bytes)
	{
		n[0].bytes = calloc(offset + 1, 1);
		memcpy(n[0].bytes, bytes, offset);
	}
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
	
	if (lena == 1 && lenb == 1 && a[lena - 1].opcode == opBytes && b->opcode == opBytes)
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
enum Opcode escape (struct Parse *p, int16_t *offset, char buffer[5])
{
	*offset = 1;
	buffer[0] = *(p->c++);
	
	switch (buffer[0])
	{
		case 'D':
			*offset = 0;
		case 'd':
			return opDigit;
			
		case 'S':
			*offset = 0;
		case 's':
			return opSpace;
			
		case 'W':
			*offset = 0;
		case 'w':
			return opWord;
			
		case 'b': buffer[0] = '\b'; break;
		case 'f': buffer[0] = '\f'; break;
		case 'n': buffer[0] = '\n'; break;
		case 'r': buffer[0] = '\r'; break;
		case 't': buffer[0] = '\t'; break;
		case 'v': buffer[0] = '\v'; break;
		case 'c':
			if (tolower(*p->c) >= 'a' && tolower(*p->c) <= 'z')
				buffer[0] = *(p->c++) % 32;
			
			break;
			
		case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7':
			buffer[0] -= '0';
			if (*p->c >= '0' && *p->c <= '7')
			{
				buffer[0] = buffer[0] * 8 + *(p->c++) - '0';
				if (*p->c >= '0' && *p->c <= '7')
				{
					buffer[0] = buffer[0] * 8 + *(p->c++) - '0';
					if (*p->c >= '0' && *p->c <= '7')
					{
						if ((int)buffer[0] * 8 + *p->c - '0' <= 0xFF)
							buffer[0] = buffer[0] * 8 + *(p->c++) - '0';
					}
				}
				
				if (buffer[0])
					*offset = Chars.writeCodepoint(buffer, ((uint8_t *)buffer)[0]);
			}
			break;
			
		case 'x':
			if (isxdigit(p->c[0]) && isxdigit(p->c[1]))
			{
				*offset = Chars.writeCodepoint(buffer, Lexer.uint8Hex(p->c[0], p->c[1]));
				p->c += 2;
			}
			break;
			
		case 'u':
			if (isxdigit(p->c[0]) && isxdigit(p->c[1]) && isxdigit(p->c[2]) && isxdigit(p->c[3]))
			{
				*offset = Chars.writeCodepoint(buffer, Lexer.uint16Hex(p->c[0], p->c[1], p->c[2], p->c[3]));
				p->c += 4;
			}
			break;
			
		default:
			break;
	}
	
	return opBytes;
}

static
enum Opcode character (struct Text text, int16_t *offset, char buffer[12], int ignoreCase)
{
	if (ignoreCase)
	{
		char *split = Text.toLower(text, buffer);
		char *check = Text.toUpper(text, split);
		ptrdiff_t length = check - buffer;
		int codepoints = 0;
		
		*check = '\0';
		while (check-- > buffer)
			if ((*check & 0xc0) != 0x80)
				codepoints++;
		
		if (codepoints == 2 && memcmp(buffer, split, split - buffer))
		{
			*offset = length;
			return opOneOf;
		}
	}
	*offset = text.length;
	memcpy(buffer, text.bytes, text.length);
	return opBytes;
}

static
struct RegExp(Node) * characterNode (struct Text text, int ignoreCase)
{
	char buffer[12];
	int16_t offset;
	enum Opcode opcode = character(text, &offset, buffer, ignoreCase);
	
	return node(opcode, offset, buffer);
}

static
struct RegExp(Node) * term (struct Parse *p, struct Error **error)
{
	struct RegExp(Node) *n;
	struct Text text;
	
	p->disallowQuantifier = 0;
	
	if (p->c >= p->end - 1)
		return NULL;
	else if (accept(p, '^'))
	{
		p->disallowQuantifier = 1;
		return node(p->multiline? opLineStart: opStart, 0, NULL);
	}
	else if (accept(p, '$'))
	{
		p->disallowQuantifier = 1;
		return node(p->multiline? opLineEnd: opEnd, 0, NULL);
	}
	else if (accept(p, '\\'))
	{
		switch (*p->c)
		{
			case 'b':
				++p->c;
				p->disallowQuantifier = 1;
				return node(opBoundary, 1, NULL);
				
			case 'B':
				++p->c;
				p->disallowQuantifier = 1;
				return node(opBoundary, 0, NULL);
				
			case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
			{
				int c = *(p->c++) - '0';
				while (isdigit(*(p->c)))
					c = c * 10 + *(p->c++) - '0';
				
				return node(opReference, c, NULL);
			}
				
			default:
			{
				enum Opcode opcode;
				int16_t offset;
				char buffer[5];
				
				opcode = escape(p, &offset, buffer);
				if (opcode == opBytes)
				{
					text = Text.make(buffer, offset);
					return characterNode(text, p->ignoreCase);
				}
				else
					return node(opcode, offset, NULL);
			}
		}
	}
	else if (accept(p, '('))
	{
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
			if (!*error)
				*error = Error.syntaxError(Text.make(p->c, 1), Chars.create("expect ')'"));
			
			return NULL;
		}
		
		switch (modifier) {
			case '\0':
				return join(node(opSave, count * 2, NULL), join(n, node(opSave, count * 2 + 1, NULL)));
				
			case '=':
				p->disallowQuantifier = 1;
				return join(node(opLookahead, nlen(n) + 2, NULL), join(n, node(opMatch, 0, NULL)));
				
			case '!':
			{
				int i = 0, c = nlen(n);
				p->disallowQuantifier = 1;
				for (; i < c; ++i)
					if (n[i].opcode == opSave)
						n[i].opcode = opNSave;
				
				return join(node(opNLookahead, nlen(n) + 2, NULL), join(n, node(opMatch, 0, NULL)));
			}
			case ':':
				return n;
		}
	}
	else if (accept(p, '.'))
		return node(opAny, 0, NULL);
	else if (accept(p, '['))
	{
		enum Opcode opcode;
		int16_t offset;
		
		int not = accept(p, '^'), length = 0, lastLength, range = -1;
		char buffer[255];
		n = NULL;
		
		while (*p->c != ']' || range >= 0)
		{
			lastLength = length;
			
			if (accept(p, '\\'))
			{
				opcode = escape(p, &offset, buffer + length);
				if (opcode == opBytes)
					length += offset;
				else
				{
					if (not)
						n = join(node(opNLookahead, 3, NULL), join(node(opcode, offset, NULL), join(node(opMatch, 0, NULL), n)));
					else
						n = join(node(opSplit, 3, NULL), join(node(opcode, offset, NULL), join(node(opJump, nlen(n)+2, NULL), n)));
				}
			}
			else
			{
				opcode = opBytes;
				buffer[length++] = *(p->c++);
			}
			
			if (range >= 0)
			{
				if (opcode == opBytes)
				{
					struct Text text = Text.make(buffer + range, length - range);
					struct Text(Char) from, to;
					
					from = Text.nextCharacter(&text);
					Text.advance(&text, 1);
					to = Text.nextCharacter(&text);
					
					if (from.codepoint > to.codepoint)
					{
						toss(n);
						*error = Error.syntaxError(Text.make(p->c - length - range, length - range), Chars.create("range out of order in character class"));
						return NULL;
					}
					
					if (not)
						n = join(node(opNLookahead, 3, NULL),
								 join(node(p->ignoreCase? opInRangeCase: opInRange, length - range, buffer + range),
									  join(node(opMatch, 0, NULL),
										   n)));
					else
						n = join(node(opSplit, 3, NULL),
								 join(node(p->ignoreCase? opInRangeCase: opInRange, length - range, buffer + range),
									  join(node(opJump, nlen(n)+2, NULL),
										   n)));
					
					length = range;
				}
				range = -1;
			}
			
			if (opcode == opBytes && *p->c == '-')
			{
				buffer[length++] = *(p->c++);
				range = lastLength;
			}
			
			if (p->c >= p->end || length >= sizeof(buffer))
			{
				*error = Error.syntaxError(Text.make(p->c - 1, 1), Chars.create("expect ']'"));
				return NULL;
			}
		}
		
		if (p->ignoreCase)
		{
			char casebuffer[6];
			struct Text single;
			struct Text(Char) c;
			text = Text.make(buffer + length, length);
			
			while (text.length)
			{
				c = Text.prevCharacter(&text);
				single = text;
				single.length = c.units;
				
				offset = Text.toLower(single, casebuffer) - casebuffer;
				if (memcmp(text.bytes, casebuffer, offset))
				{
					memcpy(buffer + length, casebuffer, offset);
					length += offset;
				}
				
				offset = Text.toUpper(single, casebuffer) - casebuffer;
				if (memcmp(text.bytes, casebuffer, offset))
				{
					memcpy(buffer + length, casebuffer, offset);
					length += offset;
				}
			}
		}
		
		buffer[length] = '\0';
		accept(p, ']');
		return join(n, node(not? opNeitherOf: opOneOf, length, buffer));
	}
	else if (*p->c && strchr("*+?)}|", *p->c))
		return NULL;
	
	text = Text.make(p->c, p->end - p->c);
	text.length = Text.character(text).units;
	p->c += text.length;
	return characterNode(text, p->ignoreCase);
}

static
struct RegExp(Node) * alternative (struct Parse *p, struct Error **error)
{
	struct RegExp(Node) *n = NULL, *t = NULL;
	
	for (;;)
	{
		if (!(t = term(p, error)))
			break;
		
		if (!p->disallowQuantifier)
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
				uint16_t index, count = nlen(t) - (lazy? 1: 0), length = 3;
				char buffer[count + length];
				
				for (index = (lazy? 1: 0); index < count; ++index)
					if (t[index].opcode == opSave)
						buffer[length++] = (uint8_t)t[index].offset;
				
				buffer[0] = min;
				buffer[1] = max;
				buffer[2] = lazy;
				buffer[length] = '\0';
				
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
	assert(*(p->end - 1) == '/');
	
	++p->c;
	return join(disjunction(p, error), node(opMatch, 0, NULL));
}


//MARK: matching

static
int match (struct RegExp(State) * const s, struct RegExp(Node) *n, struct Text text);

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
int forkMatch (struct RegExp(State) * const s, struct RegExp(Node) *n, struct Text text, int16_t offset)
{
	int result;
	
	if (n->depth == 0xff)
		return 0; //!\\ too many recursion
	else
		++n->depth;
	
	result = match(s, n + offset, text);
	--n->depth;
	return result;
}


int match (struct RegExp(State) * const s, struct RegExp(Node) *n, struct Text text)
{
	goto start;
next:
	++n;
start:
	;
	
//	#if DUMP_REGEXP
//		fprintf(stderr, "\n%.*s\n^ ", text.length, text.bytes);
//		printNode(n);
//	#endif
	
	switch((enum Opcode)n->opcode)
	{
		case opNLookahead:
		case opLookahead:
			if (forkMatch(s, n, text, 1) == (n->opcode - 1))
				goto jump;
			
			return 0;
			
		case opStart:
			if (text.bytes != s->start)
				return 0;
			
			goto next;
			
		case opEnd:
			if (text.bytes != s->end)
				return 0;
			
			goto next;
			
		case opLineStart:
		{
			struct Text prev = Text.make(text.bytes, text.bytes - s->start);
			if (text.bytes != s->start && !Text.isLineFeed(Text.prevCharacter(&prev)))
				return 0;
			
			goto next;
		}
			
		case opLineEnd:
			if (text.bytes != s->end && !Text.isLineFeed(Text.character(text)))
				return 0;
			
			goto next;
			
		case opBoundary:
		{
			struct Text prev = Text.make(text.bytes, text.bytes - s->start);
			if (text.bytes == s->start)
			{
				if (Text.isWord(Text.character(text)) != n->offset)
					return 0;
			}
			else if (text.bytes == s->end)
			{
				if (Text.isWord(Text.prevCharacter(&prev)) != n->offset)
					return 0;
			}
			else
			{
				if ((Text.isWord(Text.prevCharacter(&prev))
					!=
					Text.isWord(Text.character(text))) != n->offset
					)
					return 0;
			}
			goto next;
		}
			
		case opSplit:
			if (forkMatch(s, n, text, 1))
				return 1;
			
			goto jump;
			
		case opReference:
		{
			uint16_t len = s->capture[n->offset * 2 + 1]? s->capture[n->offset * 2 + 1] - s->capture[n->offset * 2]: 0;
			
			if (len)
			{
				//#if DUMP_REGEXP
				//				fprintf(stderr, "ref: %.*s", len, s->capture[n->offset * 2]);
				//#endif
				if (text.length < len || memcmp(text.bytes, s->capture[n->offset * 2], len))
					return 0;
				
				Text.advance(&text, len);
			}
			goto next;
		}
			
		case opRedo:
		{
			int hasmin = n->depth + 1 >= n->bytes[0];
			int lazy = n->bytes[2] && hasmin;
			
			if (n->bytes[1] && n->depth >= n->bytes[1])
				return 0;
			
			if (forkMatch(s, n, text, lazy? 1: n->offset))
			{
				clear(s, text.bytes, (uint8_t *)n->bytes + 3);
				return 1;
			}
			
			if (!hasmin)
				return 0;
			
			if (lazy)
				goto jump;
			else
				goto next;
		}
			
		case opSave:
		{
			const char *capture = s->capture[n->offset];
			s->capture[n->offset] = text.bytes;
			if (forkMatch(s, n, text, 1)) {
				if (s->capture[n->offset] <= s->index[n->offset] && text.bytes == s->capture[n->offset]) {
					s->capture[n->offset] = NULL;
				}
				return 1;
			}
			s->capture[n->offset] = capture;
			return 0;
		}
			
		case opNSave:
		{
			const char *capture = s->capture[n->offset];
			s->capture[n->offset] = text.bytes;
			if (!forkMatch(s, n, text, 1)) {
				s->capture[n->offset] = NULL;
				return 0;
			}
			s->capture[n->offset] = capture;
			return 1;
		}
			
		case opDigit:
			if (text.length < 1 || Text.isDigit(Text.nextCharacter(&text)) != n->offset)
				return 0;
			
			goto next;
			
		case opSpace:
			if (text.length < 1 || Text.isSpace(Text.nextCharacter(&text)) != n->offset)
				return 0;
			
			goto next;
			
		case opWord:
			if (text.length < 1 || Text.isWord(Text.nextCharacter(&text)) != n->offset)
				return 0;
			
			goto next;
			
		case opBytes:
			if (text.length < n->offset || memcmp(n->bytes, text.bytes, n->offset))
				return 0;
			
			Text.advance(&text, n->offset);
			goto next;
			
		case opOneOf:
		{
			char buffer[5];
			struct Text(Char) c;
			
			if (!text.length)
				return 0;
			
			c = Text.character(text);
			memcpy(buffer, text.bytes, c.units);
			buffer[c.units] = '\0';
			
			if (n->bytes && strstr(n->bytes, buffer))
			{
				Text.nextCharacter(&text);
				goto next;
			}
			return 0;
		}
			
		case opNeitherOf:
		{
			char buffer[5];
			struct Text(Char) c;
			
			if (!text.length)
				return 0;
			
			c = Text.character(text);
			memcpy(buffer, text.bytes, c.units);
			buffer[c.units] = '\0';
			
			if (n->bytes && strstr(n->bytes, buffer))
				return 0;
			
			Text.nextCharacter(&text);
			goto next;
		}
			
		case opInRange:
		case opInRangeCase:
		{
			struct Text range = Text.make(n->bytes, n->offset);
			struct Text(Char) from, to, c;
			
			if (!text.length)
				return 0;
			
			from = Text.nextCharacter(&range);
			Text.advance(&range, 1);
			to = Text.nextCharacter(&range);
			c = Text.character(text);
			
			if (n->opcode == opInRangeCase)
			{
				char buffer[c.units];
				struct Text casetext = Text.make(buffer, 0);
				
				casetext.length = Text.toLower(Text.make(text.bytes, sizeof(buffer)), buffer) - buffer;
				c = Text.character(casetext);
				if (c.units == casetext.length && (c.codepoint >= from.codepoint && c.codepoint <= to.codepoint))
				{
					Text.nextCharacter(&text);
					goto next;
				}
				
				casetext.length = Text.toUpper(Text.make(text.bytes, sizeof(buffer)), buffer) - buffer;
				c = Text.character(casetext);
				if (c.units == casetext.length && (c.codepoint >= from.codepoint && c.codepoint <= to.codepoint))
				{
					Text.nextCharacter(&text);
					goto next;
				}
			}
			else
			{
				if ((c.codepoint >= from.codepoint && c.codepoint <= to.codepoint))
				{
					Text.nextCharacter(&text);
					goto next;
				}
			}
			
			return 0;
		}
			
		case opAny:
			if (text.length >= 1 && !Text.isLineFeed(Text.nextCharacter(&text)))
				goto next;
			
			return 0;
			
		case opJump:
			goto jump;
			
		case opMatch:
			s->capture[1] = text.bytes;
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

static
struct Value constructor (struct Context * const context)
{
	struct Value pattern, flags;
	
	Context.assertParameterCount(context, 2);
	
	pattern = Context.argument(context, 0);
	flags = Context.argument(context, 1);
	
	return Value.regexp(createWith(context, pattern, flags));
}

static
struct Value toString (struct Context * const context)
{
	struct RegExp *self = context->this.data.regexp;
	
	Context.assertParameterCount(context, 0);
	Context.assertThisType(context, Value(regexpType));
	
	return Value.chars(self->pattern);
}

static
struct Value exec (struct Context * const context)
{
	struct RegExp *self = context->this.data.regexp;
	struct Value value, lastIndex;
	
	Context.assertParameterCount(context, 1);
	Context.assertThisType(context, Value(regexpType));
	
	value = Value.toString(context, Context.argument(context, 0));
	lastIndex = self->global? Value.toInteger(context, Object.getMember(context, &self->object, Key(lastIndex))): Value.integer(0);
	
	Object.putMember(context, &self->object, Key(lastIndex), Value.integer(0));
	
	if (lastIndex.data.integer >= 0)
	{
		uint16_t length = Value.stringLength(&value);
		const char *bytes = Value.stringBytes(&value);
		const char *capture[self->count * 2];
		const char *index[self->count * 2];
		struct Chars *element;
		
		struct RegExp(State) state = {
			String.textAtIndex(bytes, length, lastIndex.data.integer, 0).bytes,
			bytes + length,
			capture,
			index
		};
		
		if (state.start >= bytes && state.start <= state.end && matchWithState(self, &state))
		{
			struct Object *array = Array.createSized(self->count);
			int32_t index, count;
			
			for (index = 0, count = self->count; index < count; ++index)
			{
				if (capture[index * 2])
				{
					element = Chars.createWithBytes(capture[index * 2 + 1] - capture[index * 2], capture[index * 2]);
					array->element[index].value = Value.chars(element);
				}
				else
					array->element[index].value = Value(undefined);
			}
			
			if (self->global)
				Object.putMember(context, &self->object, Key(lastIndex), Value.integer(String.unitIndex(bytes, length, (int32_t)(capture[1] - bytes))));
			
			Object.addMember(array, Key(index), Value.integer(String.unitIndex(bytes, length, (int32_t)(capture[0] - bytes))), 0);
			Object.addMember(array, Key(input), value, 0);
			
			return Value.object(array);
		}
	}
	return Value(null);
}

static
struct Value test (struct Context * const context)
{
	struct RegExp *self = context->this.data.regexp;
	struct Value value, lastIndex;
	
	Context.assertParameterCount(context, 1);
	Context.assertThisType(context, Value(regexpType));
	
	value = Value.toString(context, Context.argument(context, 0));
	lastIndex = Value.toInteger(context, Object.getMember(context, &self->object, Key(lastIndex)));
	
	Object.putMember(context, &self->object, Key(lastIndex), Value.integer(0));
	
	if (lastIndex.data.integer >= 0)
	{
		uint16_t length = Value.stringLength(&value);
		const char *bytes = Value.stringBytes(&value);
		const char *capture[self->count * 2];
		const char *index[self->count * 2];
		
		struct RegExp(State) state = {
			String.textAtIndex(bytes, length, lastIndex.data.integer, 0).bytes,
			bytes + length,
			capture,
			index
		};
		
		if (state.start >= bytes && state.start <= state.end && matchWithState(self, &state))
		{
			if (self->global)
				Object.putMember(context, &self->object, Key(lastIndex), Value.integer(String.unitIndex(bytes, length, (int32_t)(capture[1] - bytes))));
			
			return Value(true);
		}
	}
	return Value(false);
}

// MARK: - Methods

void setup ()
{
	const enum Value(Flags) h = Value(hidden);
	
	Function.setupBuiltinObject(
		&RegExp(constructor), constructor, 2,
		&RegExp(prototype), Value.regexp(create(Chars.create("/(?:)/"), NULL, 0)),
		&RegExp(type));
	
	Function.addToObject(RegExp(prototype), "toString", toString, 0, h);
	Function.addToObject(RegExp(prototype), "exec", exec, 1, h);
	Function.addToObject(RegExp(prototype), "test", test, 1, h);
}

void teardown (void)
{
	RegExp(prototype) = NULL;
	RegExp(constructor) = NULL;
}

struct RegExp * create (struct Chars *s, struct Error **error, enum RegExp(Options) options)
{
	struct Parse p = { 0 };
	
	struct RegExp *self = malloc(sizeof(*self));
	*self = RegExp.identity;
	Pool.addObject(&self->object);
	
	Object.initialize(&self->object, RegExp(prototype));
	
	p.c = s->bytes;
	p.end = s->bytes + s->length;
	while (p.end > p.c && *(p.end - 1) != '/')
	{
		switch (*(--p.end)) {
			case 'i':
				p.ignoreCase = 1;
				continue;
				
			case 'm':
				p.multiline = 1;
				continue;
		}
	}
	
#if DUMP_REGEXP
	fprintf(stderr, "\n%.*s\n", s->length, s->bytes);
#endif
	
	self->pattern = s;
	self->program = pattern(&p, error);
	self->count = p.count + 1;
	self->source = Chars.createWithBytes(p.c - self->pattern->bytes - 1, self->pattern->bytes + 1);
	
//	++self->pattern->referenceCount;
//	++self->source->referenceCount;
	
	if (*p.c == '/')
		for (;;)
		{
			switch (*(++p.c))
			{
				case 'g': g:
					if (self->global == 1)
						*error = Error.syntaxError(Text.make(p.c, 1), Chars.create("invalid flag"));
					
					self->global = 1;
					continue;
					
				case 'i': i:
					if (self->ignoreCase == 1)
						*error = Error.syntaxError(Text.make(p.c, 1), Chars.create("invalid flag"));
					
					self->ignoreCase = 1;
					continue;
					
				case 'm': m:
					if (self->multiline == 1)
						*error = Error.syntaxError(Text.make(p.c, 1), Chars.create("invalid flag"));
					
					self->multiline = 1;
					continue;
					
				case '\0':
					break;
					
				case '\\':
					if (options & RegExp(allowUnicodeFlags))
					{
						if (!memcmp(p.c + 1, "u0067", 5))
						{
							p.c += 5;
							goto g;
						}
						else if (!memcmp(p.c + 1, "u0069", 5))
						{
							p.c += 5;
							goto i;
						}
						else if (!memcmp(p.c + 1, "u006d", 5) || !memcmp(p.c + 1, "u006D", 5))
						{
							p.c += 5;
							goto m;
						}
					}
					
				default:
					*error = Error.syntaxError(Text.make(p.c, 1), Chars.create("invalid flag"));
			}
			break;
		}
	else if (!*error)
		*error = Error.syntaxError(Text.make(p.c, 1), Chars.create("invalid character '%c'", isgraph(*p.c)? *p.c: '?'));
	
	Object.addMember(&self->object, Key(source), Value.chars(self->source), Value(readonly) | Value(hidden) | Value(sealed));
	Object.addMember(&self->object, Key(global), Value.truth(self->global), Value(readonly) | Value(hidden) | Value(sealed));
	Object.addMember(&self->object, Key(ignoreCase), Value.truth(self->ignoreCase), Value(readonly) | Value(hidden) | Value(sealed));
	Object.addMember(&self->object, Key(multiline), Value.truth(self->multiline), Value(readonly) | Value(hidden) | Value(sealed));
	Object.addMember(&self->object, Key(lastIndex), Value.integer(0), Value(hidden) | Value(sealed));
	
	return self;
}

struct RegExp * createWith (struct Context *context, struct Value pattern, struct Value flags)
{
	struct Error *error = NULL;
	struct Chars(Append) chars;
	struct RegExp *regexp;
	struct Value value;
	
	if (pattern.type == Value(regexpType) && flags.type == Value(undefinedType))
	{
		if (context->construct)
			value = Value.chars(pattern.data.regexp->pattern);
		else
			return pattern.data.regexp;
	}
	else
	{
		Chars.beginAppend(&chars);
		
		Chars.append(&chars, "/");
		
		if (pattern.type == Value(regexpType))
			Chars.appendValue(&chars, context, Value.chars(pattern.data.regexp->source));
		else
		{
			if (pattern.type == Value(undefinedType) || (Value.isString(pattern) && !Value.stringLength(&pattern)))
			{
				if (!context->ecc->sloppyMode)
					Chars.append(&chars, "(?:)");
			}
			else
				Chars.appendValue(&chars, context, pattern);
		}
		
		Chars.append(&chars, "/");
		
		if (flags.type != Value(undefinedType))
			Chars.appendValue(&chars, context, flags);
		
		value = Chars.endAppend(&chars);
		if (value.type != Value(charsType))
			value = Value.chars(Chars.createWithBytes(Value.stringLength(&value), Value.stringBytes(&value)));
	}
	
	assert(value.type == Value(charsType));
	regexp = create(value.data.chars, &error, context->ecc->sloppyMode? RegExp(allowUnicodeFlags): 0);
	if (error)
	{
		context->ecc->ofLine = Value.textOf(&value);
		context->ecc->ofInput = "(RegExp)";
		Context.setTextIndex(context, Context(noIndex));
		Context.throw(context, Value.error(error));
	}
	return regexp;
}

int matchWithState (struct RegExp *self, struct RegExp(State) *state)
{
	int result = 0;
	uint16_t index, count;
	struct Text text = Text.make(state->start, state->end - state->start);
	
#if DUMP_REGEXP
	struct RegExp(Node) *n = self->program;
	while (n->opcode != opOver)
		printNode(n++);
#endif
	
	do
	{
		memset(state->capture, 0, sizeof(*state->capture) * (self->count * 2));
		memset(state->index, 0, sizeof(*state->index) * (self->count * 2));
		state->capture[0] = state->index[0] = text.bytes;
		result = match(state, self->program, text);
		Text.nextCharacter(&text);
	}
	while (!result && text.length);
	
	/* XXX: cleanup */
	
	for (index = 0, count = nlen(self->program); index < count; ++index)
	{
		if (self->program[index].opcode == opSplit || self->program[index].opcode == opReference)
			self->program[index].bytes = NULL;
		
		self->program[index].depth = 0;
	}
	
	return result;
}

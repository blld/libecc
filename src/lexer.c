//
//  lexer.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#include "lexer.h"

// MARK: - Private

static inline int8_t hexhigit(int c)
{
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	else if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	else
		return c - '0';
}

static inline uint8_t uint8Hex(char a, char b)
{
	return hexhigit(a) << 4 | hexhigit(b);
}

static inline uint16_t uint16Hex(char a, char b, char c, char d)
{
	return hexhigit(a) << 12 | hexhigit(b) << 8 | hexhigit(c) << 4 | hexhigit(d);
}

// MARK: - Static Members

static inline void addLine(struct Lexer *self, uint32_t offset)
{
	if (self->input->lineCount + 1 >= self->input->lineCapacity)
	{
		self->input->lineCapacity *= 2;
		self->input->lines = realloc(self->input->lines, sizeof(*self->input->lines) * self->input->lineCapacity);
	}
	self->input->lines[++self->input->lineCount] = offset;
}

static inline char previewChar(struct Lexer *self)
{
	if (self->offset < self->input->length)
		return self->input->bytes[self->offset];
	else
		return 0;
}

static inline char nextChar(struct Lexer *self)
{
	if (self->offset < self->input->length)
	{
		int c = self->input->bytes[self->offset++];
		
		if ((c == '\r' && previewChar(self) != '\n') || c == '\n')
		{
			self->didLineBreak = 1;
			addLine(self, self->offset);
		}
		
		++self->text.length;
		return c;
	}
	else
		return 0;
}

static inline int acceptChar(struct Lexer *self, char c)
{
	if (previewChar(self) == c)
	{
		nextChar(self);
		return 1;
	}
	else
		return 0;
}

static inline enum Lexer(Token) error(struct Lexer *self, struct Error *error)
{
	self->value = Value.error(error);
	return Lexer(errorToken);
}

// MARK: - Methods

struct Lexer * createWithInput(struct Input *input)
{
	struct Lexer *self = malloc(sizeof(*self));
	assert(self);
	*self = Lexer.identity;
	
	assert(input);
	self->input = input;
	
	return self;
}

void destroy (struct Lexer *self)
{
	assert(self);
	
	self->input = NULL;
	free(self), self = NULL;
}

enum Lexer(Token) nextToken (struct Lexer *self)
{
	int c, disallowRegex, disallowKeyword;
	assert(self);
	
	disallowRegex = self->disallowRegex;
	disallowKeyword = self->disallowKeyword;
	
	self->value = Value(undefined);
	self->didLineBreak = 0;
	self->disallowRegex = 0;
	self->disallowKeyword = 0;
	
	retry:
	self->text.location = self->input->bytes + self->offset;
	self->text.length = 0;
	
	while (( c = nextChar(self) ))
	{
		switch (c)
		{
			case '\0':
				return error(self, Error.syntaxError(self->text, "NUL character before end of script"));
			
			case '\n':
			case '\r':
			case '\t':
			case '\v':
			case '\f':
			case ' ':
				goto retry;
			
			case '/':
			{
				if (acceptChar(self, '*'))
				{
					while (( c = nextChar(self) ))
						if (c == '*' && acceptChar(self, '/'))
							goto retry;
					
					return error(self, Error.syntaxError(self->text, "unterminated comment"));
				}
				else if (previewChar(self) == '/')
				{
					while (( c = nextChar(self) ))
						if (c == '\r' || c == '\n')
							goto retry;
					
					return 0;
				}
				else if (!disallowRegex)
				{
					#warning TODO
					Env.printWarning("TODO: regex or division");
					return error(self, Error.syntaxError(self->text, "TODO: regex"));
				}
				else if (acceptChar(self, '='))
					return Lexer(divideAssignToken);
				else
					return '/';
			}
			
			case '\'':
			case '"':
			{
				char end = c;
				int haveEscape = 0;
				int didLineBreak = self->didLineBreak;
				
				self->disallowRegex = 1;
				
				while (( c = nextChar(self) ))
				{
					if (c == '\\')
					{
						haveEscape = 1;
						nextChar(self);
						self->didLineBreak = didLineBreak;
					}
					else if (c == end)
					{
						const char *location = self->text.location++;
						uint32_t length = self->text.length -= 2;
						
						if (haveEscape)
						{
							char buffer[length];
							uint32_t index = 0, bufferIndex = 0;
							
							++location;
							
							for (; index <= length; ++index, ++bufferIndex)
								if (location[index] == '\\' && location[++index] != '\\')
								{
									switch (location[index])
									{
										case '0': buffer[bufferIndex] = '\0'; break;
										case 'a': buffer[bufferIndex] = '\a'; break;
										case 'b': buffer[bufferIndex] = '\b'; break;
										case 'f': buffer[bufferIndex] = '\f'; break;
										case 'n': buffer[bufferIndex] = '\n'; break;
										case 'r': buffer[bufferIndex] = '\r'; break;
										case 't': buffer[bufferIndex] = '\t'; break;
										case 'v': buffer[bufferIndex] = '\v'; break;
										case 'x':
											if (isxdigit(location[index + 1]) && isxdigit(location[index + 2]))
											{
												buffer[bufferIndex] = uint8Hex(location[index + 1], location[index + 2]);
												index += 2;
												break;
											}
											return error(self, Error.syntaxError(Text.make(self->text.location + bufferIndex, 4), "malformed hexadecimal character escape sequence"));
										
										case 'u':
											if (isxdigit(location[index + 1]) && isxdigit(location[index + 2]) && isxdigit(location[index + 3]) && isxdigit(location[index + 4]))
											{
												uint16_t c = uint16Hex(location[index+ 1], location[index + 2], location[index + 3], location[index + 4]);
												char *b = buffer + bufferIndex;
												
												if (c < 0x80) *b++=c;
												else if (c < 0x800) *b++ = 192 + c / 64, *b++ = 128 + c % 64;
												else if (c - 0xd800u < 0x800) goto error;
												else if (c <= 0xffff) *b++ = 224 + c / 4096, *b++ = 128 + c / 64 % 64, *b++ = 128 + c % 64;
												else goto error;
												
												bufferIndex = (unsigned int)(b - buffer) - 1;
												index += 4;
												break;
											}
											error:
											return error(self, Error.syntaxError(Text.make(self->text.location + bufferIndex, 6), "malformed Unicode character escape sequence"));
										
										default:
											buffer[bufferIndex] = location[index];
									}
								}
								else
									buffer[bufferIndex] = location[index];
							
							--bufferIndex;
							
							self->text = Text.make(malloc(length), bufferIndex);
							memcpy((char *)self->text.location, buffer, bufferIndex);
							Input.addEscapedText(self->input, self->text);
						}
						
						return Lexer(stringToken);
					}
					else if (c == '\r' || c == '\n')
						break;
				}
				
				return error(self, Error.syntaxError(self->text, "unterminated string literal"));
			}
			
			case '.':
				if (!isdigit(previewChar(self)))
				{
					self->disallowKeyword = 1;
					return c;
				}
				/* fallthrough */
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			{
				int binary = 0;
				
				self->disallowRegex = 1;
				
				if (c == '0' && (acceptChar(self, 'x') || acceptChar(self, 'X')))
				{
					while (( c = previewChar(self) ))
						if (isxdigit(c))
							nextChar(self);
						else
							break;
					
					if (self->text.length <= 2)
						return error(self, Error.syntaxError(self->text, "missing hexadecimal digits after '0x'"));
				}
				else
				{
					while (isdigit(previewChar(self)))
						nextChar(self);
					
					if (acceptChar(self, '.'))
						binary = 1;
					
					while (isdigit(previewChar(self)))
						nextChar(self);
					
					if (acceptChar(self, 'e') || acceptChar(self, 'E'))
					{
						binary = 1;
						
						if (!acceptChar(self, '+'))
							acceptChar(self, '-');
						
						if (!isdigit(previewChar(self)))
							return error(self, Error.syntaxError(self->text, "missing exponent"));
						
						while (isdigit(previewChar(self)))
							nextChar(self);
					}
				}
				
				if (isalpha(previewChar(self)))
				{
					self->text.location += self->text.length;
					self->text.length = 1;
					return error(self, Error.syntaxError(self->text, "identifier starts immediately after numeric literal"));
				}
				
				if (binary)
				{
					self->value = parseBinary(self->text);
					return Lexer(binaryToken);
				}
				else
				{
					self->value = parseInteger(self->text, 0);
					
					if (self->value.type == Value(integerType))
						return Lexer(integerToken);
					else
						return Lexer(binaryToken);
				}
			}
			
			case '}':
			case ')':
			case ']':
				self->disallowRegex = 1;
				/* fallthrought */
			case '{':
			case '(':
			case '[':
			case ';':
			case ',':
			case '~':
			case '?':
			case ':':
				return c;
			
			case '^':
				if (acceptChar(self, '='))
					return Lexer(xorAssignToken);
				else
					return c;
			
			case '%':
				if (acceptChar(self, '='))
					return Lexer(moduloAssignToken);
				else
					return c;
			
			case '*':
				if (acceptChar(self, '='))
					return Lexer(multiplyAssignToken);
				else
					return c;
			
			case '=':
				if (acceptChar(self, '='))
				{
					if (acceptChar(self, '='))
						return Lexer(identicalToken);
					else
						return Lexer(equalToken);
				}
				else
					return c;
			
			case '!':
				if (acceptChar(self, '='))
				{
					if (acceptChar(self, '='))
						return Lexer(notIdenticalToken);
					else
						return Lexer(notEqualToken);
				}
				else
					return c;
			
			case '+':
				if (acceptChar(self, '+'))
					return Lexer(incrementToken);
				else if (acceptChar(self, '='))
					return Lexer(addAssignToken);
				else
					return c;
			
			case '-':
				if (acceptChar(self, '-'))
					return Lexer(decrementToken);
				else if (acceptChar(self, '='))
					return Lexer(minusAssignToken);
				else
					return c;
			
			case '&':
				if (acceptChar(self, '&'))
					return Lexer(logicalAndToken);
				else if (acceptChar(self, '='))
					return Lexer(andAssignToken);
				else
					return c;
			
			case '|':
				if (acceptChar(self, '|'))
					return Lexer(logicalOrToken);
				else if (acceptChar(self, '='))
					return Lexer(orAssignToken);
				else
					return c;
			
			case '<':
				if (acceptChar(self, '<'))
				{
					if (acceptChar(self, '='))
						return Lexer(leftShiftAssignToken);
					else
						return Lexer(leftShiftToken);
				}
				else if (acceptChar(self, '='))
					return Lexer(lessOrEqualToken);
				else
					return c;
			
			case '>':
				if (acceptChar(self, '>'))
				{
					if (acceptChar(self, '>'))
					{
						if (acceptChar(self, '='))
							return Lexer(unsignedRightShiftAssignToken);
						else
							return Lexer(unsignedRightShiftToken);
					}
					else if (acceptChar(self, '='))
						return Lexer(rightShiftAssignToken);
					else
						return Lexer(rightShiftToken);
				}
				else if (acceptChar(self, '='))
					return Lexer(moreOrEqualToken);
				else
					return c;
			
			default:
			{
				if (isalpha(c) || c == '$' || c == '_')
				{
					static const struct {
						const char *name;
						size_t length;
						enum Lexer(Token) token;
						int disallowRegex;
					} keywords[] = {
						#define _(X, Y) { #X, sizeof(#X) - 1, Lexer(X##Token), 0 ## Y },
						_(break,)
						_(case,)
						_(catch,)
						_(continue,)
						_(debugger,)
						_(default,)
						_(delete,)
						_(do,)
						_(else,)
						_(finally,)
						_(for,)
						_(function,)
						_(if,)
						_(in,)
						_(instanceof,)
						_(new,)
						_(return,)
						_(switch,)
						_(typeof,)
						_(throw,)
						_(try,)
						_(var,)
						_(void,)
						_(while,)
						_(with,)
						
						_(void,)
						_(typeof,)
						
						_(null, 1)
						_(true, 1)
						_(false, 1)
						_(this, 1)
						#undef _
					};
					
					static const struct {
						const char *name;
						size_t length;
					} reservedKeywords[] = {
						#define _(X) { #X, sizeof(#X) - 1 },
						_(class)
						_(enum)
						_(extends)
						_(super)
						_(const)
						_(export)
						_(import)
						_(implements)
						_(let)
						_(private)
						_(public)
						_(interface)
						_(package)
						_(protected)
						_(static)
						_(yield)
						#undef _
					};
					
					int k;
					
					while (isalnum(previewChar(self)) || previewChar(self) == '$' || previewChar(self) == '_')
						nextChar(self);
					
					if (!disallowKeyword)
					{
						for (k = 0; k < sizeof(keywords) / sizeof(*keywords); ++k)
							if (self->text.length == keywords[k].length && memcmp(self->text.location, keywords[k].name, keywords[k].length) == 0)
							{
								self->disallowRegex |= keywords[k].disallowRegex;
								return keywords[k].token;
							}
						
						for (k = 0; k < sizeof(reservedKeywords) / sizeof(*reservedKeywords); ++k)
							if (self->text.length == reservedKeywords[k].length && memcmp(self->text.location, reservedKeywords[k].name, reservedKeywords[k].length) == 0)
								return error(self, Error.syntaxError(self->text, "'%s' is a reserved identifier", reservedKeywords[k]));
					}
					
					self->disallowRegex = 1;
					self->value = Value.key(Key.makeWithText(self->text, 0));
					return Lexer(identifierToken);
				}
				else
					if (isprint(c))
						return error(self, Error.syntaxError(self->text, "invalid character '%c'", c));
					else
						return error(self, Error.syntaxError(self->text, "invalid character '\\%d'", c & 0xff));
			}
		}
	}
	
	addLine(self, self->offset);
	return Lexer(noToken);
}

const char * tokenChars (enum Lexer(Token) token)
{
	// <!> non reentrant
	
	static char buffer[4] = { '\'', 0, '\'', 0 };
	int index;
	
	struct {
		const char *name;
		const enum Lexer(Token) token;
	} static const tokenList[] = {
		#define _(X, S, V) { sizeof(S "") > sizeof("")? S "": #X, Lexer(X ## Token), },
		io_libecc_lexer_Tokens
		#undef _
	};
	
	if (token > Lexer(noToken) && token < Lexer(errorToken))
	{
		buffer[1] = token;
		return buffer;
	}
	
	for (index = 0; index < sizeof(tokenList); ++index)
		if (tokenList[index].token == token)
			return tokenList[index].name;
	
	assert(0);
	return "unknow";
}

struct Value parseBinary (struct Text text)
{
	double binary;
	char buffer[text.length + 1];
	char *end;
	memcpy(buffer, text.location, text.length);
	buffer[text.length] = '\0';
	
	binary = strtod(buffer, &end);
	
	if (end - buffer != text.length)
		return Value.binary(NAN);
	
	return Value.binary(binary);
}

struct Value parseInteger (struct Text text, int base)
{
	long integer;
	char buffer[text.length + 1];
	
	if (!text.length)
		return Value.binary(NAN);
	else if (!base && text.length > 1 && text.location[0] == '0' && tolower(text.location[1]) != 'x')
		base = 10; // avoid auto-detect octal
	
	memcpy(buffer, text.location, text.length);
	buffer[text.length] = '\0';
	
	errno = 0;
	integer = strtol(buffer, NULL, base);
	
	if (errno == ERANGE)
	{
		if (!base || base == 10)
			return Value.binary(strtod(buffer, NULL));
		
		Env.printWarning("parseInt('%.*s', %d) out of bounds; only long int are supported by radices other than 10", text.length, text.location, base);
		return Value.binary(NAN);
	}
	else if (integer < INT32_MIN || integer > INT32_MAX)
		return Value.binary(integer);
	else
		return Value.integer((int32_t)integer);
}

int32_t parseElement (struct Text text)
{
	char c;
	int hadDigit = 0;
	uint32_t index;
	struct Value value;
	
	for (index = 0; index < text.length; ++index)
	{
		c = text.location[index];
		if (isdigit(c))
			hadDigit = 1;
		else if (isgraph(c))
			return -1;
	}
	if (!hadDigit)
		return -1;
	
	value = parseInteger(text, 0);
	if (value.type != Value(integerType))
		return -1;
	
	return value.data.integer;
}

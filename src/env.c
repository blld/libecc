//
//  env.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#if __MSDOS__
#include <conio.h>
#elif _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#endif

#if _WIN32
#include <sys/timeb.h>
#elif _DEFAULT_SOURCE || __APPLE__
#include <sys/time.h>
#endif


#define Implementation
#include "env.h"

// MARK: - Private

struct Env(Internal) {
#if __MSDOS__
	int defaultAttribute;
#elif _WIN32
	HANDLE console;
	int defaultAttribute;
	int defaultOutputCP;
	unsigned int outputFormat;
#else
	int isTerminal;
#endif
} static const EnvInternalIdentity;

static struct Env *self = NULL;

static void setTextColor(enum Env(Color) color, enum Env(Attribute) attribute)
{
	#if __MSDOS__ || _WIN32
	int c = color - 30;
	
	if (attribute == Env(invisible))
		c = (self->internal->defaultAttribute >> 4) & 0x7;
	else if (color == Env(black))
		c = attribute == Env(bold)? 0x7: 0x8;
	else if (color == Env(white) || !color)
		c = attribute == Env(bold)? 0xf: attribute == Env(dim)? 0x8: 0x7;
	else
		c = (attribute == Env(bold)? 0x8: 0) | ((c << 2) & 0x4) | (c & 0x2) | ((c >> 2) & 0x1);
	
	#if __MSDOS__
	textcolor(c);
	#elif _WIN32
	SetConsoleTextAttribute(self->internal->console, c | (self->internal->defaultAttribute & 0xf0));
	#endif
	
	#else
	if (self->internal->isTerminal)
	{
		if (color && attribute)
			fprintf(stderr, "\x1B[%d;%dm", color, attribute);
		else if (color)
			fprintf(stderr, "\x1B[%dm", color);
		else if (attribute)
			fprintf(stderr, "\x1B[%dm", attribute);
		else
			fprintf(stderr, "\x1B[0m");
	}
	#endif
}

#if __MSDOS__
static inline void printVA(uint16_t length, const char *format, va_list ap)
{
	char buffer[length + 1];
	int16_t offset = 0;
	
	const unsigned char csize = 80;
	char cbuffer[csize + 1];
	cbuffer[csize] = '\0';
	
	vsnprintf(buffer, length + 1, format, ap);
	while (offset + csize < length)
	{
		memcpy(cbuffer, buffer + offset, csize);
		cprintf("%s", cbuffer);
		offset += csize;
	}
	cprintf("%s", buffer + offset);
}
#elif _WIN32
static inline void printVA(uint16_t length, const char *format, va_list ap)
{
	char buffer[length + 1];
	vsnprintf(buffer, length + 1, format, ap);
	DWORD numberOfCharsWritten;
	WriteConsoleA(self->internal->console, buffer, length, &numberOfCharsWritten, NULL);
}
#endif

// MARK: - Static Members

// MARK: - Methods

void setup (void)
{
	assert(!self);
	
	self = malloc(sizeof(*self));
	*self = Env.identity;
	
	self->internal = malloc(sizeof(*self->internal));
	*self->internal = EnvInternalIdentity;
	
	#if __MSDOS__
	struct text_info textInfo;
	gettextinfo(&textInfo);
	self->internal->defaultAttribute = textInfo.normattr;
	#elif _WIN32
	self->internal->console = GetStdHandle(STD_ERROR_HANDLE);
	self->internal->defaultOutputCP = GetConsoleOutputCP();
	SetConsoleOutputCP(CP_UTF8);
	
	CONSOLE_SCREEN_BUFFER_INFO consoleScreenBufferInfo;
	GetConsoleScreenBufferInfo(self->internal->console, &consoleScreenBufferInfo);
	self->internal->defaultAttribute = consoleScreenBufferInfo.wAttributes;
	
	#else
	self->internal->isTerminal = getenv("TERM") != NULL && isatty(fileno(stderr));
	#endif
}

void teardown (void)
{
	assert(self);
	
	#if __MSDOS__
	textcolor(self->internal->defaultAttribute);
	#elif _WIN32
	SetConsoleOutputCP(self->internal->defaultOutputCP);
	SetConsoleTextAttribute(self->internal->console, self->internal->defaultAttribute);
	
	#if _MSC_VER && _MSC_VER < 1900
	_set_output_format(self->internal->outputFormat);
	#endif
	
	#endif
	
	print("\n");
	
	free(self->internal), self->internal = NULL;
	free(self), self = NULL;
}

void print (const char *format, ...)
{
	va_list ap;
	
	#if __MSDOS__ || _WIN32
	{
		int16_t length;
		va_start(ap, format);
		length = vsnprintf(NULL, 0, format, ap);
		va_end(ap);
		va_start(ap, format);
		printVA(length, format, ap);
		va_end(ap);
	}
	#else
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
	#endif
}

void printColor (enum Env(Color) color, enum Env(Attribute) attribute, const char *format, ...)
{
	va_list ap;
	
	setTextColor(color, attribute);
	#if __MSDOS__ || _WIN32
	{
		int16_t length;
		va_start(ap, format);
		length = vsnprintf(NULL, 0, format, ap);
		va_end(ap);
		va_start(ap, format);
		printVA(length, format, ap);
		va_end(ap);
	}
	#else
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
	#endif
	setTextColor(0, 0);
}

void printError (int typeLength, const char *type, const char *format, ...)
{
	va_list ap;
	
	printColor(Env(red), Env(bold), "%.*s", typeLength, type);
	print(": ");
	
	setTextColor(0, Env(bold));
	#if __MSDOS__ || _WIN32
	{
		int16_t length;
		va_start(ap, format);
		length = vsnprintf(NULL, 0, format, ap);
		va_end(ap);
		va_start(ap, format);
		printVA(length, format, ap);
		va_end(ap);
	}
	#else
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
	#endif
	setTextColor(0, 0);
	
	newline();
}

void printWarning (const char *format, ...)
{
	va_list ap;
	
	printColor(Env(yellow), Env(bold), "Warning");
	print(": ");
	
	setTextColor(0, Env(bold));
	#if __MSDOS__ || _WIN32
	{
		int16_t length;
		va_start(ap, format);
		length = vsnprintf(NULL, 0, format, ap);
		va_end(ap);
		va_start(ap, format);
		printVA(length, format, ap);
		va_end(ap);
	}
	#else
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
	#endif
	setTextColor(0, 0);
	
	newline();
}

void newline ()
{
	putc('\n', stderr);
}

double currentTime ()
{
#if _WIN32
	struct _timeb timebuffer;
	_ftime (&timebuffer);
	return timebuffer.time * 1000 + timebuffer.millitm;
#elif _DEFAULT_SOURCE || __APPLE__
	struct timeval time;
	gettimeofday(&time, NULL);
	return time.tv_sec * 1000 + time.tv_usec / 1000;
#else
	return time(NULL) * 1000;
#endif
}

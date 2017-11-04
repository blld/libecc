//
//  env.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#define Implementation
#include "env.h"

#if __MSDOS__
	#include <conio.h>
#elif _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#endif

#if _WIN32
	#include <sys/timeb.h>
#elif _DEFAULT_SOURCE || __APPLE__
	#include <sys/time.h>
#endif

// MARK: - Private

#define PRINT_MAX 2048

const int Env(print_max) = PRINT_MAX;

struct {
#if __MSDOS__
	int attribute;
#elif _WIN32
	HANDLE console;
	int attribute;
	int cp;
#else
	int terminal;
#endif
} static env;

void setup(void)
{
	srand((unsigned)time(NULL));
	
#if __MSDOS__
	struct text_info textInfo;
	gettextinfo(&textInfo);
	env.attribute = textInfo.normattr;
#elif _WIN32
	CONSOLE_SCREEN_BUFFER_INFO consoleScreenBufferInfo;
	env.console = GetStdHandle(STD_ERROR_HANDLE);
	env.cp = GetConsoleOutputCP();
	SetConsoleOutputCP(CP_UTF8);
	GetConsoleScreenBufferInfo(env.console, &consoleScreenBufferInfo);
	env.attribute = consoleScreenBufferInfo.wAttributes;
#else
	env.terminal = getenv("TERM") != NULL;
#endif
}

void teardown(void)
{
#if __MSDOS__
	textattr(env.attribute);
#elif _WIN32
	SetConsoleOutputCP(env.cp);
	SetConsoleTextAttribute(env.console, env.attribute);
#endif
	
	newline();
}

static
void textc(enum Env(Color) c, enum Env(Attribute) a)
{
#if __MSDOS__ || _WIN32
	if (a == Env(invisible))
		c = (env.attribute >> 4) & 0x7;
	else if (c == Env(white) || !c)
		c = a == Env(bold)? 0xf: 0x7;
	else if (c == Env(black))
		c = a == Env(bold)? 0x7: 0x8;
	else
	{
		c -= 30;
		c = (a == Env(bold)? 0x8: 0)
			| ((c << 2) & 0x4)
			| (c & 0x2)
			| ((c >> 2) & 0x1)
			;
	}
	c |= env.attribute & 0xf0;
	
	#if __MSDOS__
	textattr(c);
	#elif _WIN32
	SetConsoleTextAttribute(env.console, c);
	#endif
#else
	if (env.terminal)
	{
		if (c && a)
			fprintf(stderr, "\x1B[%d;%dm", c, a);
		else if (c | a)
			fprintf(stderr, "\x1B[%dm", c | a);
		else
			fprintf(stderr, "\x1B[0m");
	}
#endif
}

static
void vprintc(const char *format, va_list ap)
{
	char buffer[PRINT_MAX];
	size_t size = sizeof(buffer);
	
	if (vsnprintf(buffer, size, format, ap) >= size)
	{
		buffer[size - 3] = '.';
		buffer[size - 2] = '.';
	}
#if __MSDOS__
	cputs(buffer);
#elif _WIN32
	{
		DWORD count;
		WriteConsoleA(env.console, buffer, strlen(buffer), &count, NULL);
	}
#else
	fputs(buffer, stderr);
#endif
}

void print(const char *format, ...)
{
	va_list ap;
	
	va_start(ap, format);
	vprintc(format, ap);
	va_end(ap);
}

void printColor(enum Env(Color) color, enum Env(Attribute) attribute, const char *format, ...)
{
	va_list ap;
	
	va_start(ap, format);
	textc(color, attribute);
	vprintc(format, ap);
	textc(0, 0);
	va_end(ap);
}

void printError (int typeLength, const char *type, const char *format, ...)
{
	va_list ap;
	
	printColor(Env(red), Env(bold), "%.*s", typeLength, type);
	print(": ");
	
	va_start(ap, format);
	textc(0, Env(bold));
	vprintc(format, ap);
	textc(0, 0);
	va_end(ap);
	
	newline();
}

void printWarning (const char *format, ...)
{
	va_list ap;
	
	printColor(Env(yellow), Env(bold), "Warning");
	print(": ");
	
	va_start(ap, format);
	textc(0, Env(bold));
	vprintc(format, ap);
	textc(0, 0);
	va_end(ap);
	
	newline();
}

void newline()
{
#if __MSDOS__ || _WIN32
	putc('\r', stderr);
#endif
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

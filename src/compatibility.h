//
//  compatibility.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef libecc_compatibility_h
#define libecc_compatibility_h

#if __STDC_HOSTED__
	#include <stdio.h>
	#include <stdlib.h>
	#include <stddef.h>
	#include <stdarg.h>
	#include <stdint.h>
	#include <string.h>
	#include <signal.h>
	#include <assert.h>
	#include <ctype.h>
	#include <setjmp.h>
	#include <math.h>
	#include <errno.h>
#endif

#ifdef __GNUC__
#define dead __attribute__((noreturn))
#else
#define dead
#endif

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
	#undef setjmp
	#define setjmp _setjmp
	
	#undef longjmp
	#define longjmp _longjmp
#endif

#endif

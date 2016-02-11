//
//  compatibility.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef libecc_compatibility_h
#define libecc_compatibility_h

#if __STDC__

	#include <assert.h>
	#include <ctype.h>
	#include <errno.h>
	#include <float.h>
	#include <limits.h>
	#include <math.h>
	#include <setjmp.h>
	#include <signal.h>
	#include <stdarg.h>
	#include <stddef.h>
	#include <stdint.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <time.h>

	#if __GNUC__
		#define noreturn __attribute__((noreturn))
		#define typeof __typeof__
	#elif _MSC_VER
		#define noreturn __declspec(noreturn)
		#define typeof __typeof
	#else
		#define noreturn
		#define typeof __typeof__
	#endif

	#if (__STDC_VERSION__ < 199901L)
		#ifdef __GNUC__
			#define inline __inline__
			#define restrict __restrict__
		#elif _MSC_VER
			#define inline __inline
			#define restrict __restrict
		#else
			#define inline static
			#define restrict
		#endif
	#endif

	#if (__unix__ && !__MSDOS__) || (defined(__APPLE__) && defined(__MACH__))
		/* don't use signal version of long jump */
		#undef setjmp
		#define setjmp _setjmp
		
		#undef longjmp
		#define longjmp _longjmp
	#endif

	#if __MSDOS__
		#include <conio.h>
	#endif

	#if _WIN32 || _WIN64
		#define WIN32_LEAN_AND_MEAN
		#include <windows.h>
		#define strtod strtold /* supports hex */
	#endif

#endif

#endif

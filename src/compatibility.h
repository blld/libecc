//
//  compatibility.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef libecc_compatibility_h
#define libecc_compatibility_h

#if __STDC__ || _MSC_EXTENSIONS

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
		#define unreachable
		#define typeof __typeof__
	#else
		#define noreturn
		#define unreachable exit(1);
		#define typeof __typeof__
	#endif

	#if (__STDC_VERSION__ < 199901L)
		#ifdef __GNUC__
			#define inline __inline__
			#define restrict __restrict__
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

	#if _WIN32
		/* supports hex */
		#define strtod strtold
	#endif

#endif

#endif

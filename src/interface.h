//
//  interface.h
//  module
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#undef Interface
#undef Module

#undef io_libecc_interface_identity
#ifdef io_libecc_interface_noIdentity
	#define io_libecc_interface_identity
	#undef io_libecc_interface_noIdentity
#else
	#define io_libecc_interface_identity const struct Module identity;
#endif


#if __INCLUDE_LEVEL__ > 2

	#define Interface(methods, ...) \
		struct Module __VA_ARGS__; \
		io_libecc_interface_External(methods) \

#else

	#define Interface(methods, ...) \
		struct Module __VA_ARGS__; \
		io_libecc_interface_Declaration(methods) \
		io_libecc_interface_External(methods) \
		io_libecc_interface_Initialization(methods) \

#endif


#ifndef io_libecc_interface_h
#define io_libecc_interface_h

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

	#define Structure struct Module
	#define Instance struct Module *

	#define io_libecc_interface_External(methods) struct { io_libecc_interface_CAT(_end, io_libecc_interface_E_even methods) io_libecc_interface_identity } extern const Module;
	#define io_libecc_interface_E_even(...) io_libecc_interface_E(__VA_ARGS__) io_libecc_interface_E_odd
	#define io_libecc_interface_E_odd(...) io_libecc_interface_E(__VA_ARGS__) io_libecc_interface_E_even
	#define io_libecc_interface_E_even_end
	#define io_libecc_interface_E_odd_end
	#define io_libecc_interface_E(R, N, P) __typeof__(R) (*N) P;

	#define io_libecc_interface_Declaration(methods) io_libecc_interface_CAT(_end, io_libecc_interface_D_even methods)
	#define io_libecc_interface_D_even(...) io_libecc_interface_D(__VA_ARGS__) io_libecc_interface_D_odd
	#define io_libecc_interface_D_odd(...) io_libecc_interface_D(__VA_ARGS__) io_libecc_interface_D_even
	#define io_libecc_interface_D_even_end
	#define io_libecc_interface_D_odd_end
	#define io_libecc_interface_D(R, N, P) static R N P;

	#define io_libecc_interface_Initialization(methods) __typeof__(Module) Module = { io_libecc_interface_CAT(_end, io_libecc_interface_I_even methods) };
	#define io_libecc_interface_I_even(...) io_libecc_interface_I(__VA_ARGS__) io_libecc_interface_I_odd
	#define io_libecc_interface_I_odd(...) io_libecc_interface_I(__VA_ARGS__) io_libecc_interface_I_even
	#define io_libecc_interface_I_even_end
	#define io_libecc_interface_I_odd_end
	#define io_libecc_interface_I(R, N, P) N,

	#define io_libecc_interface__CAT(last, ...) __VA_ARGS__ ## last
	#define io_libecc_interface_CAT(last, ...) io_libecc_interface__CAT(last, __VA_ARGS__)

#endif

//
//  interface.h
//  module
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#undef Interface
#undef Module

#if __INCLUDE_LEVEL__ > 2

	#define Interface(methods, object) \
		struct Module object; \
		io_libecc_interface_External(methods) \

#else

	#define Interface(methods, object) \
		struct Module object; \
		io_libecc_interface_Declaration(methods) \
		io_libecc_interface_External(methods) \
		io_libecc_interface_Initialization(methods) \

#endif


#ifndef io_libecc_interface_h
#define io_libecc_interface_h

	#define Structure struct Module
	#define Instance struct Module *

	#define io_libecc_interface_External(methods) struct { io_libecc_interface_CAT(_end, io_libecc_interface_E_even methods) const struct Module identity; } extern const Module;
	#define io_libecc_interface_E_even(R, N, P) io_libecc_interface_E(R, N, P) io_libecc_interface_E_odd
	#define io_libecc_interface_E_odd(R, N, P) io_libecc_interface_E(R, N, P) io_libecc_interface_E_even
	#define io_libecc_interface_E_even_end
	#define io_libecc_interface_E_odd_end
	#define io_libecc_interface_E(R, N, P) __typeof__(R) (*N) P;

	#define io_libecc_interface_Declaration(methods) io_libecc_interface_CAT(_end, io_libecc_interface_D_even methods)
	#define io_libecc_interface_D_even(R, N, P) io_libecc_interface_D(R, N, P) io_libecc_interface_D_odd
	#define io_libecc_interface_D_odd(R, N, P) io_libecc_interface_D(R, N, P) io_libecc_interface_D_even
	#define io_libecc_interface_D_even_end
	#define io_libecc_interface_D_odd_end
	#define io_libecc_interface_D(R, N, P) static R N P;

	#define io_libecc_interface_Initialization(methods) __typeof__(Module) Module = { io_libecc_interface_CAT(_end, io_libecc_interface_I_even methods) };
	#define io_libecc_interface_I_even(R, N, P) io_libecc_interface_Unbox io_libecc_interface_I(R, N, P) io_libecc_interface_I_odd
	#define io_libecc_interface_I_odd(R, N, P) io_libecc_interface_Unbox io_libecc_interface_I(R, N, P) io_libecc_interface_I_even
	#define io_libecc_interface_I_even_end
	#define io_libecc_interface_I_odd_end
	#define io_libecc_interface_I(R, N, P) (N,)

	#define io_libecc_interface__CAT(last, entries) entries ## last
	#define io_libecc_interface_CAT(last, entries) io_libecc_interface__CAT(last, entries)

	#define io_libecc_interface_Unbox(x,y) x,

#endif

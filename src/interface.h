//
//  interface.h
//  module
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#undef Interface

#if __INCLUDE_LEVEL__ > 2

	#define Interface(module, methods, object) \
		struct module object; \
		io_libecc_interface_External(module, methods) \

#else

	#define Interface(module, methods, object) \
		struct module object; \
		io_libecc_interface_Declaration(methods) \
		io_libecc_interface_External(module, methods) \
		io_libecc_interface_Initialization(module, methods) \

#endif


#ifndef io_libecc_interface_h
#define io_libecc_interface_h

	#define io_libecc_interface_External(N, M) struct { io_libecc_interface_CAT(_end, io_libecc_interface_E_even M) const struct N identity; } extern const N;
	#define io_libecc_interface_E_even(R, N, P) io_libecc_interface_E(R, N, P) io_libecc_interface_E_odd
	#define io_libecc_interface_E_odd(R, N, P) io_libecc_interface_E(R, N, P) io_libecc_interface_E_even
	#define io_libecc_interface_E_even_end
	#define io_libecc_interface_E_odd_end
	#define io_libecc_interface_E(R, N, P) __typeof__(R) (*N) P;

	#define io_libecc_interface_Declaration(M) io_libecc_interface_CAT(_end, io_libecc_interface_D_even M)
	#define io_libecc_interface_D_even(R, N, P) io_libecc_interface_D(R, N, P) io_libecc_interface_D_odd
	#define io_libecc_interface_D_odd(R, N, P) io_libecc_interface_D(R, N, P) io_libecc_interface_D_even
	#define io_libecc_interface_D_even_end
	#define io_libecc_interface_D_odd_end
	#define io_libecc_interface_D(R, N, P) static R N P;

	#define io_libecc_interface_Initialization(N, M) __typeof__(N) N = { io_libecc_interface_CAT(_end, io_libecc_interface_I_even M) };
	#define io_libecc_interface_I_even(R, N, P) io_libecc_interface_Unwrap io_libecc_interface_I(R, N, P) io_libecc_interface_I_odd
	#define io_libecc_interface_I_odd(R, N, P) io_libecc_interface_Unwrap io_libecc_interface_I(R, N, P) io_libecc_interface_I_even
	#define io_libecc_interface_I_even_end
	#define io_libecc_interface_I_odd_end
	#define io_libecc_interface_I(R, N, P) (N,)

	#define io_libecc_interface__CAT(last, entries) entries ## last
	#define io_libecc_interface_CAT(last, entries) io_libecc_interface__CAT(last, entries)

	#define io_libecc_interface_Unwrap(x,y) x,

#endif

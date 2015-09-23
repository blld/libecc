//
//  native.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef io_libecc_native_h
#define io_libecc_native_h

#include "namespace_io_libecc.h"
struct Op;
struct Ecc;
struct Value;

typedef __typeof__(struct Value) (* io_libecc_Native) (const struct Op ** const ops, struct Ecc * const ecc);

#endif

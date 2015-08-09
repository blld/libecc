//
//  Function.h
//  libecc
//
//  Created by Bouilland Aurélien on 25/07/2015.
//  Copyright (c) 2015 Libeccio. All rights reserved.
//

#ifndef io_libecc_Function_h
#define io_libecc_Function_h

#include "namespace_io_libecc.h"

struct Op;
struct Ecc;
struct Value;

typedef __typeof__(struct Value) (* io_libecc_Function) (const struct Op ** const ops, struct Ecc * const ecc);

#endif

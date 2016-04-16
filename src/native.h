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

struct Value;
struct Context;

typedef struct Value io_libecc_interface_Unwrap ((* Native(Function))) (struct Context * const context);

#endif

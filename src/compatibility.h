//
//  compatibility.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef libecc_compatibility_h
#define libecc_compatibility_h

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#define setjmp _setjmp
#define longjmp _longjmp
#endif

#endif

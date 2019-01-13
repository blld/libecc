
libecc
======

Fast, memory-efficient and easily embeddable Ecmascript (5.1) engine for C (99, GNU)

Tested on:

* macOS
* iOS
* Linux
* Haiku
* Raspberry Pi
* Windows (minGW, Visual Studio w/ clang)
* Dos (DJGPP)

Build
-----

	$ git clone https://github.com/blld/libecc.git
	$ cd libecc/build
	$ make test

Usage
-----

sample.c

	#include "ecc.h"
	
	static const char script1[] = "greetings('world')";
	
	static
	struct Value greetings (struct Context * context)
	{
		// retrieve first argument as string
		struct Value to = Value.toString(context, Context.argument(context, 0));
		
		// get C friendly text
		struct Text text = Value.textOf(&to);
		
		// print & return undefined
		printf("Hello, %.*s!\n", text.length, text.bytes);
		return Value(undefined);
	}
	
	int main (int argc, const char * argv[])
	{
		// setup Ecc
		struct Ecc *ecc = Ecc.create();
		
		// add C function to Ecc
		Ecc.addFunction(ecc, "greetings", greetings, 1, 0);
		
		// run script
		Ecc.evalInput(ecc, Input.createFromBytes(script1, sizeof(script1), "script1"), 0);
		
		// clean-up
		Ecc.destroy(ecc), ecc = NULL;
		return EXIT_SUCCESS;
	}

compile

	$ cc -I ../src -L */lib -lecc sample.c
	$ ./a.out
	Hello, world!

License
-------

Licensed under MIT license, see LICENSE.txt file in project root


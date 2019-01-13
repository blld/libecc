
libecc
======

Fast, memory-efficient and easily embeddable Ecmascript (5.1) engine for C (99, GNU)

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
		struct Value to = Value.toString(context, Context.argument(context, 0));
		struct Text text = Value.textOf(&to);
		printf("Hello, %.*s!\n", text.length, text.bytes);
		return Value(undefined);
	}
	
	int main (int argc, const char * argv[])
	{
		struct Ecc *ecc = Ecc.create();
		
		Ecc.addFunction(ecc, "greetings", greetings, 1, 0);
		Ecc.evalInput(ecc, Input.createFromBytes(script1, sizeof(script1), "script1"), 0);
		
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


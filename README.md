
libecc
======

Fast, memory-efficient and easily embeddable Ecmascript (5.1) engine for C (99, GNU)

Build
-----

	$ git clone http://github.com/blld/libecc.git
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
		struct Value to = Context.argument(context, 0);
		to = Value.toString(context, to);
		printf("Hello, %.*s!\n", Value.stringLength(&to), Value.stringBytes(&to));
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
 
	$ gcc -I ../src -L XXX/lib sample.c -lecc
	$ ./a.out
	Hello, world!

Licensing
---------

See LICENSE


//
//  main.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#include "main.h"

struct Value print (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 1);
	
	Value.dumpTo(*Op.argument(ecc, 0), stderr);
	fputc('\n', stderr);
	
	return Value.undefined();
}

int main(int argc, const char * argv[])
{
	struct Ecc *ecc = Ecc.create();
	
	Closure.addFunction(ecc->global, "print", print, 1, 0);
	
//	char test[] = "a = undefined; if (a) b = true; else b = false";
//	char test[] = "var b = 0, a = 10;do {print(a)} while (a--);";
//	char test[] = "var a = [ 'a','b','c' ], b = 0; for (var c in a) print(c);";
//	char test[] = "var a = 5, b = 0; do { ++b } while (--a); print(a + ' ' + b)";
//	char test[] = "var a = 5, b = 0; abc: while (--a) { ++b; continue abc }";
//	char test[] = "var a = 5, b = 10; test: youpi: for (;a;--a) for (;b;) { --b }";
//	char test[] = "var a = 0, b; if (a) { b = true; } else { b = false; }";
//	char test[] = "var a = 10, b = 456; a *= b; function abc(c, d, e) { return 123; }; for (var a = 1000;a;--a) var c = abc()";
//	char test[] = "var a = 10, b = 456; a *= b; function abc(c, d, e) { return 123; }; for (var a = 100000;a;--a) var c = abc();";
//	char test[] = "print(123)";
//	char test[] = "TEST.toString();; function abc(a, b) { print(a, b) } abc(\"abcd\", 123, abc);";
//	char test[] = "function abc(a, b) { function YO(){} print(TEST.toString(), a, b) } abc(\"abcd\", 123, abc);";
//	char test[] = "var a = [ \"abc\", 123, 'hello', 'world !\\t\\t\\t*', 4.5, {}, ]";
//	char test[] = "var a = [1,'youpi',23], b = '456', c; b += a; print(b)";
//	char test[] = "var a = NaN, b = +Infinity; print(a < b)";
//	char test[] = "for (var a = 0; a < 10; ++a) if (0) continue; else for (var b = 0; b < 10; ++b) { print(a + \" \"+b); continue; } ";
//	char test[] = "if (1) print(true); else print(false); print('next'); print('next2');";
//	char test[] = "throw 'hello';";
//	char test[] = "print(1 > 2? 'a': 'b')";
//	char test[] = "if (1) {} print('ok')";
//	char test[] = "try { eval(\"print('ok)\") } catch (e) { print(e) }";
	char test[] = "eval(\"print('ok)\")";
	
#if 1
	Ecc.evalInput(ecc, Input.createFromBytes(test, sizeof(test), "main"));
#else
	if (argc <= 1)
	{
		fprintf(stderr, "need a file\n");
		return EXIT_FAILURE;
	}
	Ecc.evalInput(ecc, Input.createFromFile(argv[1]));
#endif
	
	Ecc.destroy(ecc), ecc = NULL;
	
    return EXIT_SUCCESS;
}




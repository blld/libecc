//
//  main.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#include "main.h"

static int runTest (void);
static struct Value print (const struct Op ** const ops, struct Ecc * const ecc);

static struct Ecc *ecc = NULL;

int main(int argc, const char * argv[])
{
	ecc = Ecc.create();
	
	if (argc <= 1)
	{
		fprintf(stderr, "need a file\n");
		return EXIT_FAILURE;
	}
	else if (!strcmp(argv[1], "--test"))
	{
		if (runTest())
			return EXIT_FAILURE;
	}
	else
	{
		Closure.addFunction(ecc->global, "print", print, 1, 0);
		Ecc.evalInput(ecc, Input.createFromFile(argv[1]));
	}
	
	Ecc.destroy(ecc), ecc = NULL;
	
    return EXIT_SUCCESS;
}

//

static struct Value print (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 1);
	
	Value.dumpTo(Op.argument(ecc, 0), stderr);
	fputc('\n', stderr);
	
	return Value.undefined();
}

//

static int testCount = 0;
static int testErrorCount = 0;

static void test (const char *name, const char *test, const char *expect)
{
	if (!setjmp(*Ecc.pushEnvList(ecc)))
		Ecc.evalInput(ecc, Input.createFromBytes(test, (uint32_t)strlen(test), name));
	
	Ecc.popEnvList(ecc);
	
	ecc->result = Value.toString(ecc->result);
	
	++testCount;
	
	if (memcmp(expect, Value.stringChars(ecc->result), Value.stringLength(ecc->result)))
	{
		++testErrorCount;
		Env.printColor(Env(Red), "[failure]");
		fprintf(stderr, " %s : ", name);
		Env.printColor(Env(Black), "expect '%s' was '%.*s'\n", expect, Value.stringLength(ecc->result), Value.stringChars(ecc->result));
	}
	else
	{
		Env.printColor(Env(Green), "[success]");
		fprintf(stderr, " %s\n", name);
	}
	
	Pool.collect(Value.closure(ecc->global));
}

static int runTest (void)
{
//	test("test", "try { try { throw 'a' } catch (b) { throw b + 2 } } catch (c) { return c }", "");
//	test("test", "'\t\tabc'", "");
//	test("test", "'\t\tabc'; var a = 1", "1");
//	test("object", "Object('test')", "1");
//	test("switch", "switch (2) { case 2: case 1: 'a'; }", "1");
	
	//
	
	test("statement list result 1", "1;;;;;", "1");
	test("statement list result 2", "1;{}", "1");
	test("statement list result 3", "1;var a;", "1");
	
	test("eval 1", "var x = 2; var y = 39; eval('x + y + 1')", "42");
	test("eval 2", "var z = '42'; eval(z)", "42");
	test("eval 3", "var x = 5, z, str = 'if (x == 5) { z = 42; } else z = 0; z'; eval(str)", "42");
	test("eval 4", "var str = 'if ( a ) { 1+1; } else { 1+2; }', a = true; eval(str)", "2");
	test("eval 5", "var str = 'if ( a ) { 1+1; } else { 1+2; }', a = false; eval(str)", "3");
	test("eval 6", "var str = 'function a() {}'; typeof eval(str)", "undefined");
	test("eval 7", "var str = '(function a() {})'; typeof eval(str)", "function");

	test("simple equality 1", "1 == 1", "true");
	test("simple unequality 1", "1 != 2", "true");
	test("simple strict equality 1", "3 === 3", "true");
	test("simple strict unequality 1", "3 !== '3'", "true");
	
	test("equality 1", "undefined == undefined", "true");
	test("equality 2", "null == null", "true");
	test("equality 3", "true == true", "true");
	test("equality 4", "false == false", "true");
	test("equality 5", "'foo' == 'foo'", "true");
	test("equality 6", "var x = { foo: 'bar' }, y = x; x == y", "true");
	test("equality 7", "0 == 0", "true");
	test("equality 8", "+0 == -0", "true");
	test("equality 9", "0 == false", "true");
	test("equality 10", "'' == false", "true");
	test("equality 11", "'' == 0", "true");
	test("equality 12", "'0' == 0", "true");
	test("equality 13", "'17' == 17", "true");
	test("equality 14", "[1,2] == '1,2'", "true");
	test("equality 15", "null == undefined", "true");
	test("equality 16", "null == false", "false");
	test("equality 17", "undefined == false", "false");
	test("equality 18", "({ foo: 'bar' }) == { foo: 'bar' }", "false");
	test("equality 19", "0 == null", "false");
	test("equality 20", "0 == NaN", "false");
	test("equality 21", "'foo' == NaN", "false");
	test("equality 22", "NaN == NaN", "false");
	test("equality 23", "[1,3] == '1,2'", "false");
	
	test("strict equality 1", "undefined === undefined", "true");
	test("strict equality 2", "null === null", "true");
	test("strict equality 3", "true === true", "true");
	test("strict equality 4", "false === false", "true");
	test("strict equality 5", "'foo' === 'foo'", "true");
	test("strict equality 6", "var x = { foo: 'bar' }, y = x; x === y", "true");
	test("strict equality 7", "0 === 0", "true");
	test("strict equality 8", "+0 === -0", "true");
	test("strict equality 9", "0 === false", "false");
	test("strict equality 10", "'' === false", "false");
	test("strict equality 11", "'' === 0", "false");
	test("strict equality 12", "'0' === 0", "false");
	test("strict equality 13", "'17' === 17", "false");
	test("strict equality 14", "[1,2] === '1,2'", "false");
	test("strict equality 15", "null === undefined", "false");
	test("strict equality 16", "null === false", "false");
	test("strict equality 17", "undefined === false", "false");
	test("strict equality 18", "({ foo: 'bar' }) === { foo: 'bar' }", "false");
	test("strict equality 19", "0 === null", "false");
	test("strict equality 20", "0 === NaN", "false");
	test("strict equality 21", "'foo' === NaN", "false");
	test("strict equality 22", "NaN === NaN", "false");
	
	test("branching false", "var a = null, b; if (a) b = true;", "undefined");
	test("branching true", "var a = 1, b; if (a) b = true;", "true");
	test("branching else false", "var a = undefined, b; if (a) b = true; else b = false", "false");
	test("branching else true", "var a = 1, b; if (a) b = true; else b = false", "true");
	test("while loop", "var b = 0, a = 10;do { ++b } while (a--); b;", "11");
	test("throw", "throw 'hello';", "hello");
	
	test("parse 1", "", "undefined");
	
	test("parse error 1", "= 1", "SyntaxError: expected expression, got '='");
	test("parse error 2", "+= 1", "SyntaxError: expected expression, got '+='");
	test("parse error 3", "== 1", "SyntaxError: expected expression, got '=='");
	test("parse error 3", "% 1", "SyntaxError: expected expression, got '%'");
	
	//
	
	fprintf(stderr, "\n");
	
	if (testErrorCount)
		Env.printColor(Env(Black), "test failure: %d\n", testErrorCount);
	else
		Env.printColor(Env(Black), "all success\n");
	
	fprintf(stderr, "\n");
	
	return testErrorCount;
}

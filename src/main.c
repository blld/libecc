//
//  main.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#include "main.h"

static int printUsage (void);
static int runTest (void);
static struct Value print (const struct Op ** const ops, struct Ecc * const ecc);

static struct Ecc *ecc = NULL;

int main (int argc, const char * argv[])
{
	int result = EXIT_SUCCESS;
	
	ecc = Ecc.create();
	
	if (argc <= 1 || !strcmp(argv[1], "--help"))
		result = printUsage();
	else if (!strcmp(argv[1], "--test"))
		result = runTest();
	else
	{
		Closure.addFunction(ecc->global, "print", print, 1, 0);
		result = Ecc.evalInput(ecc, Input.createFromFile(argv[1]));
	}
	
	Ecc.destroy(ecc), ecc = NULL;
	
    return result;
}

//

static int printUsage (void)
{
	fprintf(stderr, "usage: libecc <filename> or libecc --test");
	
	return EXIT_FAILURE;
}

//

static struct Value print (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 1);
	
	Value.dumpTo(Op.argument(ecc, 0), stdout);
	putc('\n', stdout);
	
	return Value.undefined();
}

//

static int testCount = 0;
static int testErrorCount = 0;

static void test (const char *func, int line, const char *test, const char *expect)
{
	++testCount;
	
	if (!setjmp(*Ecc.pushEnv(ecc)))
		Ecc.evalInput(ecc, Input.createFromBytes(test, (uint32_t)strlen(test), "%s:%d", func, line));
	
	Ecc.popEnv(ecc);
	
	struct Value result = Value.toString(ecc->result);
	uint16_t length = Value.stringLength(result);
	
	if (length != strlen(expect) || memcmp(expect, Value.stringChars(result), length))
	{
		++testErrorCount;
		Env.printColor(Env(Red), "[failure]");
		fprintf(stderr, " %s:%d : ", func, line);
		Env.printColor(Env(Black), "expect '%s' was '%.*s'\n", expect, Value.stringLength(result), Value.stringChars(result));
	}
	else
	{
		Env.printColor(Env(Green), "[success]");
		fprintf(stderr, " %s:%d\n", func, line);
	}
	
	Ecc.garbageCollect(ecc);
}
#define test(i, e) test(__func__, __LINE__, i, e)

static void testLexer (void)
{
	test("1.f", "SyntaxError: identifier starts immediately after numeric literal");
}

static void testParser (void)
{
	test("= 1", "SyntaxError: expected expression, got '='");
	test("+= 1", "SyntaxError: expected expression, got '+='");
	test("== 1", "SyntaxError: expected expression, got '=='");
	test("% 1", "SyntaxError: expected expression, got '%'");
	test("var 1.", "SyntaxError: expected identifier, got number");
	
	test("", "undefined");
	test("1;;;;;", "1");
	test("1;{}", "1");
	test("1;var a;", "1");
}

static void testEval (void)
{
	test("var x = 2; var y = 39; eval('x + y + 1')", "42");
	test("var z = '42'; eval(z)", "42");
	test("var x = 5, z, str = 'if (x == 5) { z = 42; } else z = 0; z'; eval(str)", "42");
	test("var str = 'if ( a ) { 1+1; } else { 1+2; }', a = true; eval(str)", "2");
	test("var str = 'if ( a ) { 1+1; } else { 1+2; }', a = false; eval(str)", "3");
	test("var str = 'function a() {}'; typeof eval(str)", "undefined");
	test("var str = '(function a() {})'; typeof eval(str)", "function");
}

static void testEquality (void)
{
	test("1 == 1", "true");
	test("1 != 2", "true");
	test("3 === 3", "true");
	test("3 !== '3'", "true");
	
	test("undefined == undefined", "true");
	test("null == null", "true");
	test("true == true", "true");
	test("false == false", "true");
	test("'foo' == 'foo'", "true");
	test("var x = { foo: 'bar' }, y = x; x == y", "true");
	test("0 == 0", "true");
	test("+0 == -0", "true");
	test("0 == false", "true");
	test("'' == false", "true");
	test("'' == 0", "true");
	test("'0' == 0", "true");
	test("'17' == 17", "true");
	test("[1,2] == '1,2'", "true");
	test("null == undefined", "true");
	test("null == false", "false");
	test("undefined == false", "false");
	test("({ foo: 'bar' }) == { foo: 'bar' }", "false");
	test("0 == null", "false");
	test("0 == NaN", "false");
	test("'foo' == NaN", "false");
	test("NaN == NaN", "false");
	test("[1,3] == '1,2'", "false");
	
	test("undefined === undefined", "true");
	test("null === null", "true");
	test("true === true", "true");
	test("false === false", "true");
	test("'foo' === 'foo'", "true");
	test("var x = { foo: 'bar' }, y = x; x === y", "true");
	test("0 === 0", "true");
	test("+0 === -0", "true");
	test("0 === false", "false");
	test("'' === false", "false");
	test("'' === 0", "false");
	test("'0' === 0", "false");
	test("'17' === 17", "false");
	test("[1,2] === '1,2'", "false");
	test("null === undefined", "false");
	test("null === false", "false");
	test("undefined === false", "false");
	test("({ foo: 'bar' }) === { foo: 'bar' }", "false");
	test("0 === null", "false");
	test("0 === NaN", "false");
	test("'foo' === NaN", "false");
	test("NaN === NaN", "false");
}

static void testConditional (void)
{
	test("var a = null, b; if (a) b = true;", "undefined");
	test("var a = 1, b; if (a) b = true;", "true");
	test("var a = undefined, b; if (a) b = true; else b = false", "false");
	test("var a = 1, b; if (a) b = true; else b = false", "true");
	test("var b = 0, a = 10;do { ++b } while (a--); b;", "11");
	test("throw 'hello';", "hello");
}

static int runTest (void)
{
//	test("test", "try { try { throw 'a' } catch (b) { throw b + 2 } } catch (c) { return c }", "");
//	test("test", "'\t\tabc'", "");
//	test("test", "'\t\tabc'; var a = 1", "1");
//	test("object", "Object('test')", "1");
//	test("switch", "switch (2) { case 2: case 1: 'a'; }", "1");
//	test("function 1", "function a(){ for  }", "");
//	test("for in 1", "var 1", "");
	
	testLexer();
	testParser();
	testEval();
	testEquality();
	testConditional();
	
	putc('\n', stderr);
	
	if (testErrorCount)
		Env.printColor(Env(Black), "test failure: %d\n", testErrorCount);
	else
		Env.printColor(Env(Black), "all success\n");
	
	putc('\n', stderr);
	
	return testErrorCount? EXIT_FAILURE: EXIT_SUCCESS;
}

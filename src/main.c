//
//  main.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#include "main.h"

static int printUsage (void);
static int runTest (int verbosity);
static struct Value print (const struct Op ** const ops, struct Ecc * const ecc);

static struct Ecc *ecc = NULL;

int main (int argc, const char * argv[])
{
	int result = EXIT_SUCCESS;
	
	ecc = Ecc.create();
	
	if (argc <= 1 || !strcmp(argv[1], "--help"))
		result = printUsage();
	else if (!strcmp(argv[1], "--test"))
		result = runTest(0);
	else if (!strcmp(argv[1], "--test-verbose"))
		result = runTest(1);
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
	fprintf(stderr, "usage: libecc <filename> or libecc --test or libecc --test-verbose\n\n");
	
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

static int testVerbosity = 0;
static int testCount = 0;
static int testErrorCount = 0;

static void test (const char *func, int line, const char *test, const char *expect)
{
	++testCount;
	
	if (testVerbosity || !setjmp(*Ecc.pushEnv(ecc)))
		Ecc.evalInput(ecc, Input.createFromBytes(test, (uint32_t)strlen(test), "%s:%d", func, line));
	
	if (!testVerbosity)
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
	test("/*hello", "SyntaxError: unterminated comment");
	test("/*hello\nworld", "SyntaxError: unterminated comment");
	test("'hello", "SyntaxError: unterminated string literal");
	test("'hello\nworld'", "SyntaxError: unterminated string literal");
	test("0x", "SyntaxError: missing hexadecimal digits after '0x'");
	test("0e+", "SyntaxError: missing exponent");
	test("\\", "SyntaxError: invalid character '\\'");
}

static void testParser (void)
{
	test("debugger()", "SyntaxError: missing ; before statement");
	test("delete a = 1", "ReferenceError: invalid assignment left-hand side");
	test("delete throw", "SyntaxError: expected expression, got 'throw'");
	
	test("{", "SyntaxError: expected '}', got end of script");
	test("[", "SyntaxError: expected ']', got end of script");
	
	test("= 1", "SyntaxError: expected expression, got '='");
	test("+= 1", "SyntaxError: expected expression, got '+='");
	test("var 1.", "SyntaxError: expected identifier, got number");
}

static void testEval (void)
{
	test("", "undefined");
	test("1;;;;;", "1");
	test("1;{}", "1");
	test("1;var a;", "1");
	
	test("var x = 2; var y = 39; eval('x + y + 1')", "42");
	test("var z = '42'; eval(z)", "42");
	test("var x = 5, z, str = 'if (x == 5) { z = 42; } else z = 0; z'; eval(str)", "42");
	test("var str = 'if ( a ) { 1+1; } else { 1+2; }', a = true; eval(str)", "2");
	test("var str = 'if ( a ) { 1+1; } else { 1+2; }', a = false; eval(str)", "3");
	test("var str = 'function a() {}'; typeof eval(str)", "undefined");
	test("var str = '(function a() {})'; typeof eval(str)", "function");
	
	test("x", "ReferenceError: x is not defined");
	test("x = 1", "ReferenceError: x is not defined");
}

static void testException (void)
{
	test("throw undefined", "undefined");
	test("throw null", "null");
	test("throw 123", "123");
	test("throw 'hello'", "hello");
	test("try { throw 'a' } finally { 'b' }", "a");
	test("try { throw 'a' } catch(b){} finally { 'c' }", "c");
	test("try { try { throw 'a' } catch (b) { throw b + 'b'; return 'b' } } catch (c) { throw c + 'c'; return 'c' }", "abc");
	test("try { try { throw 'a' } catch (b) { return 'b' } } catch (c) { throw c + 'c'; return 'c' }", "b");
	test("try { try { throw 'a' } catch (b) { throw b + 'b'; return 'b' } } catch (c) { return 'c' }", "c");
	
	test("try { throw 'a' } ", "SyntaxError: missing catch or finally after try");
}

static void testOperation (void)
{
	test("+ 10", "10");
	test("- 10", "-10");
	test("~ 10", "-11");
	test("! 10", "false");
	test("10 * 2", "20");
	test("10 / 2", "5");
	test("10 % 8", "2");
	test("10 + 1", "11");
	test("10 - 1", "9");
	test("10 & 3", "2");
	test("10 ^ 3", "9");
	test("10 | 3", "11");
	
	test("* 1", "SyntaxError: expected expression, got '*'");
	test("% 1", "SyntaxError: expected expression, got '%'");
	test("& 1", "SyntaxError: expected expression, got '&'");
	test("^ 1", "SyntaxError: expected expression, got '^'");
	test("| 1", "SyntaxError: expected expression, got '|'");
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
	
	test("== 1", "SyntaxError: expected expression, got '=='");
}

static void testRelational (void)
{
}

static void testConditional (void)
{
	test("var a = null, b; if (a) b = true;", "undefined");
	test("var a = 1, b; if (a) b = true;", "true");
	test("var a = undefined, b; if (a) b = true; else b = false", "false");
	test("var a = 1, b; if (a) b = true; else b = false", "true");
	test("var b = 0, a = 10;do { ++b } while (a--); b;", "11");
}

static void testSwitch (void)
{
	test("switch (1) { case 1: 123; case 2: 'abc'; }", "abc");
	test("switch (2) { case 1: 123; case 2: 'abc'; }", "abc");
	test("switch (1) { case 1: 123; break; case 2: 'abc'; }", "123");
	test("switch ('abc') { case 'abc': 123; break; case 2: 'abc'; }", "123");
	test("switch (123) { default: case 1: 123; break; case 2: 'abc'; }", "123");
	test("switch (123) { case 1: 123; break; default: case 2: 'abc'; }", "abc");
	test("switch (123) { case 1: 123; break; case 2: 'abc'; break; default: ({}) }", "[object Object]");
}

static void testDelete (void)
{
	test("delete b", "true");
	test("var a = { b: 123, c: 'abc' }; a.b", "123");
	test("var a = { b: 123, c: 'abc' }; delete a.b; a.b", "undefined");
	test("this.x = 42; delete x", "true");
	
	test("delete Object.prototype", "TypeError: property 'prototype' is non-configurable and can't be deleted");
	test("var y = 43; delete y", "TypeError: property 'y' is non-configurable and can't be deleted");
}

static void testGlobal (void)
{
	test("this.x = 42; var x = 123; x", "123");
	test("var x = 123; this.x = 42; x", "42");
}

static int runTest (int verbosity)
{
	testVerbosity = verbosity;
	
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
	testException();
	testOperation();
	testEquality();
	testRelational();
	testConditional();
	testSwitch();
	testDelete();
	testGlobal();
	
	putc('\n', stderr);
	
	if (testErrorCount)
		Env.printColor(Env(Black), "test failure: %d\n", testErrorCount);
	else
		Env.printColor(Env(Black), "all success\n");
	
	putc('\n', stderr);
	
	return testErrorCount? EXIT_FAILURE: EXIT_SUCCESS;
}

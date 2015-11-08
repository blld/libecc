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
	
	Function.addNative(ecc->global, "print", print, 1, 0);
	
	if (argc <= 1 || !strcmp(argv[1], "--help"))
		result = printUsage();
	else if (!strcmp(argv[1], "--test"))
		result = runTest(0);
	else if (!strcmp(argv[1], "--test-verbose"))
		result = runTest(1);
	else
	{
		ecc->this = Value.object(&ecc->global->context);
		result = Ecc.evalInput(ecc, Input.createFromFile(argv[1]));
	}
	
	Ecc.destroy(ecc), ecc = NULL;
	
    return result;
}

//

static int printUsage (void)
{
	fprintf(stderr, "usage: libecc <filename> or libecc --test or libecc --test-verbose\n");
	
	return EXIT_FAILURE;
}

//

static struct Value print (const struct Op ** const ops, struct Ecc * const ecc)
{
	Op.assertParameterCount(ecc, 1);
	
	Value.dumpTo(Op.argument(ecc, 0), stdout);
	putc('\n', stdout);
	
	ecc->result = Value.undefined();
	
	return Value.undefined();
}

//

static int testVerbosity = 0;
static int testErrorCount = 0;

static void test (const char *func, int line, const char *test, const char *expect)
{
	struct Value result;
	uint16_t length;
	
	if (testVerbosity || !setjmp(*Ecc.pushEnv(ecc)))
		Ecc.evalInput(ecc, Input.createFromBytes(test, (uint32_t)strlen(test), "%s:%d", func, line));
	
	if (!testVerbosity)
		Ecc.popEnv(ecc);
	
	result = Value.toString(ecc->result, NULL);
	length = Value.stringLength(result);
	
	if (length != strlen(expect) || memcmp(expect, Value.stringChars(result), length))
	{
		++testErrorCount;
		Env.printColor(Env(red), Env(bold), "[failure]");
		Env.print(" %s:%d - ", func, line);
		Env.printColor(0, Env(bold), "expect '%s' was '%.*s'", expect, Value.stringLength(result), Value.stringChars(result));
	}
	else
	{
		Env.printColor(Env(green), Env(bold), "[success]");
		Env.print(" %s:%d", func, line);
	}
	Env.newline();
	
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
	test("0a", "SyntaxError: identifier starts immediately after numeric literal");
	test(".e+1", "SyntaxError: expected expression, got '.'");
	test("\\", "SyntaxError: invalid character '\\'");
	test("'\\a\\b\\f\\n\\r\\t\\v'", "\a\b\f\n\r\t\v");
	test("'xxx\\xrrabc'", "SyntaxError: malformed hexadecimal character escape sequence");
	test("'\\x44'", "D");
	test("'xxx\\uabxyabc'", "SyntaxError: malformed Unicode character escape sequence");
	test("'\\u4F8B'", "例");
	test("'例'", "例");
}

static void testParser (void)
{
	test("debugger()", "SyntaxError: missing ; before statement");
	test("delete throw", "SyntaxError: expected expression, got 'throw'");
	
	test("{", "SyntaxError: expected '}', got end of script");
	test("[", "SyntaxError: expected ']', got end of script");
	
	test("= 1", "SyntaxError: expected expression, got '='");
	test("+= 1", "SyntaxError: expected expression, got '+='");
	test("var 1.", "SyntaxError: expected identifier, got number");
	test("var h = ;","SyntaxError: expected expression, got ';'");
	
	test("function eval(){}", "SyntaxError: redefining eval is deprecated");
	test("function arguments(){}", "SyntaxError: redefining arguments is deprecated");
	test("function a(eval){}", "SyntaxError: redefining eval is deprecated");
	test("function a(arguments){}", "SyntaxError: redefining arguments is deprecated");
	test("var eval", "SyntaxError: redefining eval is deprecated");
	test("var arguments", "SyntaxError: redefining arguments is deprecated");
	test("eval = 123", "SyntaxError: can't assign to eval");
	test("arguments = 123", "SyntaxError: can't assign to arguments");
	test("eval++", "SyntaxError: invalid increment operand");
	test("arguments++", "SyntaxError: invalid increment operand");
	test("eval += 1", "SyntaxError: invalid assignment left-hand side");
	test("arguments += 1", "SyntaxError: invalid assignment left-hand side");
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
	test("function a() { var n = 456; return eval('n') }; a()", "456");
	test("var e = eval; function a() { var n = 456; return e('n') }; a()", "ReferenceError: n is not defined");
	test("var e = eval; function a() { var n = 456; return e('this.parseInt.length') }; a()", "2");
	
	test("x", "ReferenceError: x is not defined");
	test("x = 1", "ReferenceError: x is not defined");
	test("new eval('123')", "TypeError: eval is not a constructor");
}

static void testException (void)
{
	test("throw undefined", "undefined");
	test("throw null", "null");
	test("throw 123", "123");
	test("throw 'hello'", "hello");
	test("try { throw 'a' } finally { 'b' }", "a");
	test("try { throw 'a' } catch(b){ 'b' }", "b");
	test("try { throw 'a' } catch(b){ 'b' } 'c'", "c");
	test("try { throw 'a' } catch(b){ 'b' } finally { 'c' }", "c");
	test("try { throw 'a' } catch(b){ 'b' } finally { 'c' } 'd'", "d");
	test("try { try { throw 'a' } catch (b) { throw b + 'b'; return 'b' } } catch (c) { throw c + 'c'; return 'c' }", "abc");
	test("try { try { throw 'a' } catch (b) { return 'b' } } catch (c) { throw c + 'c'; return 'c' }", "b");
	test("try { try { throw 'a' } catch (b) { throw b + 'b'; return 'b' } } catch (c) { return 'c' }", "c");
	test("var a = 0; try { for (;;) { if(++a > 100) throw a; } } catch (e) { e } finally { a + 'f' }", "101f");
	
	test("try { throw 'a' }", "SyntaxError: expected catch or finally, got end of script");
}

static void testOperator (void)
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
	test("4 > 3", "true");
	test("4 >= 3", "true");
	test("3 >= 3", "true");
	test("3 < 4", "true");
	test("3 <= 4", "true");
	
	test("'toString' in {}", "true");
	test("'toString' in null", "TypeError: invalid 'in' operand null");
	test("var a; 'toString' in a", "TypeError: invalid 'in' operand a");
	test("var a = { b: 1, c: 2 }; 'b' in a", "true");
	test("var a = { b: 1, c: 2 }; 'd' in a", "false");
	test("var a = [ 'b', 'c' ]; 0 in a", "true");
	test("var a = [ 'b', 'c' ]; 2 in a", "false");
	test("var a = [ 'b', 'c' ]; '0' in a", "true");
	test("var a = [ 'b', 'c' ]; '2' in a", "false");
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
	test("delete b", "SyntaxError: delete of an unqualified identifier in strict mode");
	test("var a = { b: 123, c: 'abc' }; a.b", "123");
	test("var a = { b: 123, c: 'abc' }; delete a.b; a.b", "undefined");
	
	test("delete Object.prototype", "TypeError: property 'prototype' is non-configurable and can't be deleted");
}

static void testGlobal (void)
{
	test("typeof this", "undefined");
	test("typeof global", "object");
	
	test("null", "null");
	test("global.null", "undefined");
	test("global.Infinity", "Infinity");
	test("-global.Infinity", "-Infinity");
	test("global.NaN", "NaN");
	test("global.undefined", "undefined");
	test("typeof global.eval", "function");
	
	// direct this === global mapping (avoid automatic closure)
	ecc->this = Value.object(&ecc->global->context);
	test("this.NaN", "NaN");
	ecc->this = Value.undefined();
}

static void testFunction (void)
{
	test("var a; a.prototype", "TypeError: can't convert undefined to object");
	test("var a = null; a.prototype", "TypeError: can't convert null to object");
	
	test("function a() {} a.prototype.toString.length", "0");
	test("function a() {} a.prototype.toString()", "[object Object]");
	test("function a() {} a.prototype.hasOwnProperty.length", "1");
	test("function a() {} a.length", "0");
	test("function a(b, c) {} a.length", "2");
	test("function a(b, c) { b + c } a(1, 5)", "6");
	test("function a(b, c) { arguments.toString() } a()", "[object Arguments]");
	test("function a(b, c) { arguments.length } a(1, 5, 6)", "3");
	test("function a(b, c) { arguments[0] + arguments[1] } a(1, 5)", "6");
	
	test("var n = 456; function b(c) { return 'c' + c + n } b(123)", "c123456");
	test("function a() { var n = 456; function b(c) { return 'c' + c + n } return b } a()(123)", "c123456");
	
	test("typeof function(){}", "function");
	test("function a(a, b){ return a + b } a.apply(null, 1, 2)", "TypeError: arguments is not an object");
	test("function a(a, b){ return this + a + b } a.apply(10, [1, 2])", "13");
	test("function a(a, b){ return this + a + b } a.call(10, 1, 2)", "13");
	test("function a(){ return arguments }; a().toString()", "[object Arguments]");
	test("function a(){ return arguments }; a.call().toString()", "[object Arguments]");
	test("function a(){ return arguments }; a.apply().toString()", "[object Arguments]");
	
	test("typeof Function", "function");
	test("Function.length", "1");
	test("typeof Function.prototype", "function");
	test("Function.prototype.length", "0");
	test("Function.prototype(1, 2, 3)", "undefined");
	test("typeof Function()", "function");
	test("Function()()", "undefined");
	test("Function('return 123')()", "123");
	test("new Function('a', 'b', 'c', 'return a+b+c')(1, 2, 3)", "6");
	test("new Function('a, b, c', 'return a+b+c')(1, 2, 3)", "6");
	test("new Function('a,b', 'c', 'return a+b+c')(1, 2, 3)", "6");
}

static void testLoop (void)
{
	test("var a = 0; for (;;) if (++a > 100) break; a", "101");
	test("var a; for (a = 0;;) if (++a > 100) break; a", "101");
	test("for (var a = 0;;) if (++a > 100) break; a", "101");
	test("for (var a = 0; a <= 100;) ++a; a", "101");
	test("for (var a = 0; a <= 100; ++a); a", "101");
	test("for (var a = 0, b = 0; a <= 100; ++a) ++b; b", "101");
	test("var a = 123; for (var i = 10; i >= 0; i--) --a; (a + a)", "224");
	test("var a = 100, b = 0; while (a--) ++b;", "100");
	test("var a = 100, b = 0; do ++b; while (a--)", "101");
	
	test("var a = { 'a': 123 }, b; for (b in a) a[b];", "123");
	test("var a = { 'a': 123 }; for (var b in a) a[b];", "123");
	test("var a = { 'a': 123 }; for (b in a) ;", "ReferenceError: b is not defined");
	test("var a = [ 'a', 123 ], b; for (b in a) b + ':' + a[b];", "1:123");
	test("var a = [ 'a', 123 ], b; for (b in a) typeof b;", "string");
}

static void testThis (void)
{
	test("function a() { return typeof this; } a()", "undefined");
	test("var a = { b: function () { return typeof this; } }; a.b()", "object");
	test("var a = { b: function () { return this; } }; a.b().b === a.b", "true");
}

static void testObject (void)
{
	test("var a = { a: 1, 'b': 2, '1': 3 }; a.a", "1");
	test("var a = { a: 1, 'b': 2, '1': 3 }; a['a']", "1");
	test("var a = { a: 1, 'b': 2, '1': 3 }; a['b']", "2");
	test("var a = { a: 1, 'b': 2, '1': 3 }; a.b", "2");
	test("var a = { a: 1, 'b': 2, 1: 3 }; a[1]", "3");
	test("var a = { a: 1, 'b': 2, '1': 3 }; a[1]", "3");
	test("var a = { a: 1, 'b': 2, '1': 3 }, c = 1; a[c]", "3");
	test("var a = { a: 1, 'b': 2, '1': 3 }; delete a['a']; a['a']", "undefined");
	test("var a = { a: 1, 'b': 2, '1': 3 }; delete a['a']; a['a'] = 123; a['a']", "123");
	
	test("var a = { a: 123 }; a.toString()", "[object Object]");
	
	test("var a = { a: 123 }; a.valueOf() === a", "true");
	
	test("var a = { a: 123 }; a.hasOwnProperty('a')", "true");
	test("var a = { a: 123 }; a.hasOwnProperty('toString')", "false");
	
	test("var a = {}, b = {}; a.isPrototypeOf(123)", "false");
	test("var a = {}, b = {}; a.isPrototypeOf(b)", "false");
	test("var a = {}, b = {}; Object.getPrototypeOf(b).isPrototypeOf(a)", "true");
	
	test("var a = { a: 123 }; a.propertyIsEnumerable('a')", "true");
	test("var a = {}; a.propertyIsEnumerable('toString')", "undefined");
	test("var a = {}; Object.getPrototypeOf(a).propertyIsEnumerable('toString')", "false");
	
	test("var a = {}, b = {}; Object.getPrototypeOf(a) === Object.getPrototypeOf(b)", "true");
	test("var a = {}; Object.getPrototypeOf(Object.getPrototypeOf(a))", "undefined");
	
	test("var a = { a: 123 }; Object.getOwnPropertyDescriptor(a, 'a').value", "123");
	test("var a = { a: 123 }; Object.getOwnPropertyDescriptor(a, 'a').writable", "true");
	
	test("Object.getOwnPropertyNames({ a:'!', 2:'@', 'b':'#'}).toString()", "2,a,b");
	
	test("var a = {}, o = ''; a['a'] = 'abc'; a['c'] = 123; a['b'] = undefined; for (var b in a) o += b + a[b]; o", "aabcc123bundefined");
	
	test("var a = { eval: 123 }", "SyntaxError: eval in object identifier");
	test("var a = { arguments: 123 }", "SyntaxError: arguments in object identifier");
	
	test("var a = {}; a.null = 123; a.null", "123");
	test("var a = {}; a.function = 123; a.function", "123");
	
	test("typeof Object", "function");
	test("typeof Object.prototype", "object");
	
	test("Object.prototype.toString.call(true)", "[object Boolean]");
	test("Object.prototype.toString.call('abc')", "[object String]");
	test("Object.prototype.toString.call(123)", "[object Number]");
}

static void testAccessor (void)
{
	test("var a = { get () {} }", "SyntaxError: expected identifier, got '('");
	test("var a = { get a (b) {} }", "SyntaxError: getter functions must have no arguments");
	test("var a = { set () {} }", "SyntaxError: expected identifier, got '('");
	test("var a = { set a () {} }", "SyntaxError: setter functions must have one argument");
	
	test("var a = { get a() { return 123 } }; a.a", "123");
	test("var a = { get a() { return 123 } }; a['a']", "123");
	test("var a = { a: 'abc', set a(b) {}, get a() { return 123 } }; a.a", "123");
	test("var a = { set a(b) {}, a: 'abc', get a() { return 123 } }; a.a", "123");
	test("var a = { set a(b) {}, get a() { return 123 }, a: 'abc' }; a.a", "abc");
	test("var a = { _a: 'u', get a() { return this._a } }; a.a", "u");
	test("var a = { _a: 'u', set a(v) { this._a = v } }; a.a = 123; a._a", "123");
	test("var a = { _a: 'u', set a(v) { this._a = v }, get a() { return this._a } }; a.a = 123; a.a", "123");
	
	test("var a = { get: 123 }; a.get", "123");
	test("var a = { set: 123 }; a.set", "123");
	test("var a = { get get() { return 123 } }; a.get", "123");
	test("var a = { get set() { return 123 } }; a.set", "123");
}

static void testArray (void)
{
	test("var a = [ 'a', 'b', 'c' ]; a['a'] = 123; a[1]", "b");
	test("var a = [ 'a', 'b', 'c' ]; a['1'] = 123; a[1]", "123");
	test("var a = [ 'a', 'b', 'c' ]; a['a'] = 123; a.a", "123");
	test("var a = [ 'a', 'b', 'c' ]; a['a'] = 123; a.toString()", "a,b,c");
	test("var a = [], o = ''; a[0] = 'abc'; a[4] = 123; a[2] = undefined; for (var b in a) o += b + a[b]; o", "0abc2undefined4123");
	test("var a = [1, 2, 3], o = ''; delete a[1]; for (var b in a) o += b; o", "02");
	test("var a = [1, , 3], o = ''; for (var b in a) o += b; o", "02");
	test("var a = [1, , 3], o = ''; a[1] = undefined; for (var b in a) o += b; o", "012");
	test("[1, , 3]", "[object Array]");
	test("[1, , 3] + ''", "1,,3");
	test("[1, , 3].toString()", "1,,3");
	test("typeof []", "object");
	test("Object.prototype.toString.call([])", "[object Array]");
	test("Array.isArray([])", "true");
	test("Array.isArray({})", "false");
	test("Array.isArray(Array.prototype)", "true");
	test("var alpha = ['a', 'b', 'c'], numeric = [1, 2, 3]; alpha.concat(numeric).toString()", "a,b,c,1,2,3");
	test("Array.prototype.concat.call(undefined, 123).toString()", "TypeError: can't convert undefined to object");
	test("Array.prototype.concat.call(123, 'abc', [{}, 456]).toString()", "123,abc,[object Object],456");
	
	test("var a = [1, 2]; a.length", "2");
	test("var a = [1, 2]; a.length = 5; a.length", "5");
	test("var a = [1, 2]; a[5] = 5; a.length", "6");
	
	test("typeof Array", "function");
	test("typeof Array.prototype", "object");
}

static void testBoolean (void)
{
	test("var b = new Boolean('a'); b.valueOf() === true", "true");
	test("var b = Boolean('a'); b.valueOf() === true", "true");
	test("var b = new Boolean(0); b.toString()", "false");
	test("var b = Boolean(); b.toString()", "false");
}

static void testNumber (void)
{
	test("0xf", "15");
	test("0xff", "255");
	test("0xffff", "65535");
	test("0xffffffff", "4294967295");
	test("0xffffffffffffffff", "18446744073709551616");
	test("0x3635c9adc5de9e0000", "999999999999999868928");
	test("0x3635c9adc5de9f0000", "1e+21");
	
	test("-0xf", "-15");
	test("-0xff", "-255");
	test("-0xffff", "-65535");
	test("-0xffffffff", "-4294967295");
	test("-0xffffffffffffffff", "-18446744073709551616");
	test("-0x3635c9adc5de9e0000", "-999999999999999868928");
	test("-0x3635c9adc5de9f0000", "-1e+21");
	
	test("0x7fffffff | 0", "2147483647");
	test("0xffffffff | 0", "-1");
	test("0x1fffffff0 | 0", "-16");
	test("-0x7fffffff | 0", "-2147483647");
	test("-0xffffffff | 0", "1");
	test("-0x1fffffff0 | 0", "16");
	
	// toString
	// radix 10 -> libc %g (double)
	// radix  8 -> libc %lo (long int)
	// radix 16 -> libc %lx (long int)
	// other radices are custom code (long int)
	
	test("123.456.toString(2)", "1111011");
	test("123.456.toString(4)", "1323");
	test("123.456.toString(8)", "173");
	test("123.456.toString()", "123.456");
	test("123.456.toString(10)", "123.456");
	test("123.456.toString(16)", "7b");
	test("123.456.toString(24)", "53");
	test("123.456.toString(32)", "3r");
	test("123.456.toString(36)", "3f");
	
	test("2147483647..toString(2)", "1111111111111111111111111111111");
	test("(-2147483647).toString(2)", "-1111111111111111111111111111111");
	test("2147483647..toString(8)", "17777777777");
	test("(-2147483647).toString(8)", "-17777777777");
	test("Number.MAX_VALUE.toString(10)", "1.79769313486231570815e+308");
	test("Number.MIN_VALUE.toString(10)", "2.2250738585072e-308");
	test("(2147483647).toString(16)", "7fffffff");
	test("(-2147483647).toString(16)", "-7fffffff");
	test("-2147483647..toString(16)", "NaN");
	test("-2147483647..toString(10)", "-2147483647");
	test("-2147483647 .toString()", "-2147483647");
	
	test("123.456.toExponential()", "1.234560e+02");
	test("123.456.toFixed()", "123.456000");
	test("123.456.toPrecision()", "123.456");
	
	test("123.456.toExponential(10)", "1.2345600000e+02");
	test("123.456.toFixed(10)", "123.4560000000");
	test("123.456.toPrecision(10)", "123.456");
}

static int runTest (int verbosity)
{
	Function.addValue(ecc->global, "global", Value.object(&ecc->global->context), 0);
	
	testVerbosity = verbosity;
	
	testLexer();
	testParser();
	testEval();
	testException();
	testOperator();
	testEquality();
	testRelational();
	testConditional();
	testSwitch();
	testDelete();
	testGlobal();
	testFunction();
	testLoop();
	testThis();
	testObject();
	testAccessor();
	testArray();
	testBoolean();
	testNumber();
	
	Env.newline();
	
	if (testErrorCount)
		Env.printColor(0, Env(bold), "test failure: %d", testErrorCount);
	else
		Env.printColor(0, Env(bold), "all success");
	
	Env.newline();
	
	return testErrorCount? EXIT_FAILURE: EXIT_SUCCESS;
}

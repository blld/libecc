//
//  main.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#include "ecc.h"

static int printUsage (void);
static struct Value print (struct Context * const context);
static struct Value alert (struct Context * const context);
static int runTest (int verbosity);

static struct Ecc *ecc;

int main (int argc, const char * argv[])
{
	int result;
	
	ecc = Ecc.create();
	
	Ecc.addFunction(ecc, "print", print, -1, 0);
	Ecc.addFunction(ecc, "alert", alert, -1, 0);
	
	if (argc <= 1 || !strcmp(argv[1], "--help"))
		result = printUsage();
	else if (!strcmp(argv[1], "--test"))
		result = runTest(0);
	else if (!strcmp(argv[1], "--test-verbose"))
		result = runTest(1);
	else if (!strcmp(argv[1], "--test-quiet"))
		result = runTest(-1);
	else
	{
		struct Object *arguments = Arguments.createWithCList(argc - 2, &argv[2]);
		Ecc.addValue(ecc, "arguments", Value.object(arguments), 0);
		result = Ecc.evalInput(ecc, Input.createFromFile(argv[1]), Ecc(globalThis));
	}
	
	Ecc.destroy(ecc), ecc = NULL;
	
    return result;
}

//

static int printUsage (void)
{
	const char error[] = "Usage";
	Env.printError(sizeof(error)-1, error, "libecc [<filename> | --test | --test-verbose | --test-quiet]");
	
	return EXIT_FAILURE;
}

//

static struct Value dumpTo (struct Context * const context, FILE *file)
{
	int index, count;
	struct Value value;
	
	Context.assertVariableParameter(context);
	
	for (index = 0, count = Context.variableArgumentCount(context); index < count; ++index)
	{
		if (index)
			putc(' ', file);
		
		value = Context.variableArgument(context, index);
		Value.dumpTo(Value.toString(context, value), file);
	}
	putc('\n', file);
	
	return Value(undefined);
}

static struct Value print (struct Context * const context)
{
	return dumpTo(context, stdout);
}

static struct Value alert (struct Context * const context)
{
	return dumpTo(context, stderr);
}

//

static int testVerbosity = 0;
static int testErrorCount = 0;
static int testCount = 0;
static double testTime = 0;

useframe
static void test (const char *func, int line, const char *test, const char *expect, const char *text)
{
	const char *bytes;
	uint16_t length;
	clock_t start = clock();
	
	if (testVerbosity > 0 || !setjmp(*Ecc.pushEnv(ecc)))
		Ecc.evalInput(ecc, Input.createFromBytes(test, (uint32_t)strlen(test), "%s:%d", func, line), Ecc(stringResult));
	
	if (testVerbosity <= 0)
		Ecc.popEnv(ecc);
	
	testTime += (double)(clock() - start) / CLOCKS_PER_SEC;
	++testCount;
	
	assert(Value.isString(ecc->result));
	bytes = Value.stringBytes(ecc->result);
	length = Value.stringLength(ecc->result);
	
	if (length != strlen(expect) || memcmp(expect, bytes, length))
	{
		++testErrorCount;
		Env.printColor(Env(red), Env(bold), "[failure]");
		Env.print(" %s:%d - ", func, line);
		Env.printColor(0, Env(bold), "expect \"%s\" was \"%.*s\"", expect, length, bytes);
		Env.newline();
		goto error;
	}
	
	if (text)
	{
		const char *end = strrchr(text, '~');
		ptrdiff_t textStart = strchr(text, '^') - text, textLength = (end? end - text - textStart: 0) + 1;
		struct Input *input = Ecc.findInput(ecc, ecc->text);
		
		assert(textStart >= 0 && textStart <= strlen(test));
		
		if (input && (input->length - textStart == 0 || input->bytes[textStart] == ')') && textLength == 1)
			textLength = 0;
		
		if (!input || ecc->text.bytes - input->bytes != textStart || ecc->text.length != textLength)
		{
			++testErrorCount;
			Env.printColor(Env(red), Env(bold), "[failure]");
			Env.print(" %s:%d - ", func, line);
			Env.printColor(0, Env(bold), "text should highlight `%.*s`", textLength, test + textStart);
			Env.newline();
			goto error;
		}
	}
	
	if (testVerbosity >= 0)
	{
		Env.printColor(Env(green), Env(bold), "[success]");
		Env.print(" %s:%d", func, line);
		Env.newline();
	}
	
	error:
	Ecc.garbageCollect(ecc);
}
#define test(i, e, t) test(__func__, __LINE__, i, e, t)

static void testLexer (void)
{
	test("/*hello", "SyntaxError: unterminated comment"
	,    "^~~~~~~");
	test("/*hello\n""world", "SyntaxError: unterminated comment"
	,    "^~~~~~~~" "~~~~~");
	test("'hello", "SyntaxError: unterminated string literal"
	,    "^~~~~~");
	test("'hello\n""world'", "SyntaxError: unterminated string literal"
	,    "^~~~~~~" "      ");
	test("0x", "SyntaxError: missing hexadecimal digits after '0x'"
	,    "^~");
	test("0e+", "SyntaxError: missing exponent"
	,    "^~~");
	test("0a", "SyntaxError: identifier starts immediately after numeric literal"
	,    " ^");
	test(".e+1", "SyntaxError: expected expression, got '.'"
	,    "^   ");
	test("\\", "SyntaxError: invalid character '\\'"
	,    "^");
	test("'xxx\\""xrrabc'", "SyntaxError: malformed hexadecimal character escape sequence"
	,    "    ^" "~~~    ");
	test("'xxx\\""uabxyabc'", "SyntaxError: malformed Unicode character escape sequence"
	,    "    ^" "~~~~~    ");
	test("'\\b\\f\\n\\r\\t\\v'", "\b\f\n\r\t\v", NULL);
	test("'\\x44'", "D", NULL);
	test("'\\x44' + 2", "D2", NULL);
	test("'\\u4F8B'", "例", NULL);
	test("'例abc'", "例abc", NULL);
	test("例", "SyntaxError: invalid character '\\228'", NULL);
	test("/abc", "SyntaxError: unterminated regexp literal"
	,    "^~~~");
	test("/abc\n""  ", "SyntaxError: unterminated regexp literal"
	,    "^~~~~" "  ");
}

static void testParser (void)
{
	test("debugger()", "SyntaxError: missing ; before statement"
	,    "        ^ ");
	test("delete throw", "SyntaxError: expected expression, got 'throw'"
	,    "       ^~~~~");
	test("{", "SyntaxError: expected '}', got end of script"
	,    " ^");
	test("[", "SyntaxError: expected ']', got end of script"
	,    " ^");
	test("= 1", "SyntaxError: expected expression, got '='"
	,    "^  ");
	test("+= 1", "SyntaxError: expected expression, got '+='"
	,    "^~  ");
	test("var 1.", "SyntaxError: expected identifier, got number"
	,    "    ^~");
	test("var h = ;","SyntaxError: expected expression, got ';'"
	,    "        ^");
	test("function eval(){}", "SyntaxError: redefining eval is deprecated"
	,    "         ^~~~    ");
	test("function arguments(){}", "SyntaxError: redefining arguments is deprecated"
	,    "         ^~~~~~~~~    ");
	test("function a(eval){}", "SyntaxError: redefining eval is deprecated"
	,    "           ^~~~   ");
	test("function a(arguments){}", "SyntaxError: redefining arguments is deprecated"
	,    "           ^~~~~~~~~   ");
	test("var eval", "SyntaxError: redefining eval is deprecated"
	,    "    ^~~~");
	test("var arguments", "SyntaxError: redefining arguments is deprecated"
	,    "    ^~~~~~~~~");
	test("eval = 123", "SyntaxError: can't assign to eval"
	,    "     ^    ");
	test("arguments = 123", "SyntaxError: can't assign to arguments"
	,    "          ^    ");
	test("eval++", "SyntaxError: invalid increment operand"
	,    "^~~~  ");
	test("arguments++", "SyntaxError: invalid increment operand"
	,    "^~~~~~~~~  ");
	test("++eval", "SyntaxError: invalid increment operand"
	,    "  ^~~~");
	test("++arguments", "SyntaxError: invalid increment operand"
	,    "  ^~~~~~~~~");
	test("eval += 1", "SyntaxError: invalid assignment left-hand side"
	,    "^~~~~~~~~");
	test("arguments += 1", "SyntaxError: invalid assignment left-hand side"
	,    "^~~~~~~~~~~~~~");
	test("var a = { eval: 123 }", "undefined", NULL);
	test("var a = { arguments: 123 }", "undefined", NULL);
	test("a(==)", "SyntaxError: expected expression, got '=='"
	,    "  ^~ ");
	test("a(,1)", "SyntaxError: expected expression, got ','"
	,    "  ^  ");
	test("return", "SyntaxError: return not in function"
	,    "^~~~~~");
	test("* 1", "SyntaxError: expected expression, got '*'"
	,    "^  ");
	test("% 1", "SyntaxError: expected expression, got '%'"
	,    "^  ");
	test("& 1", "SyntaxError: expected expression, got '&'"
	,    "^  ");
	test("^ 1", "SyntaxError: expected expression, got '^'"
	,    "^  ");
	test("| 1", "SyntaxError: expected expression, got '|'"
	,    "^  ");
	test("+", "SyntaxError: expected expression, got end of script"
	,    " ^");
	test("-", "SyntaxError: expected expression, got end of script"
	,    " ^");
	test("~", "SyntaxError: expected expression, got end of script"
	,    " ^");
	test("!", "SyntaxError: expected expression, got end of script"
	,    " ^");
	test("1 *", "SyntaxError: expected expression, got end of script"
	,    "   ^");
	test("1 /", "SyntaxError: expected expression, got end of script"
	,    "   ^");
	test("1 %", "SyntaxError: expected expression, got end of script"
	,    "   ^");
	test("1 +", "SyntaxError: expected expression, got end of script"
	,    "   ^");
	test("1 -", "SyntaxError: expected expression, got end of script"
	,    "   ^");
	test("1 &", "SyntaxError: expected expression, got end of script"
	,    "   ^");
	test("1 ^", "SyntaxError: expected expression, got end of script"
	,    "   ^");
	test("1 |", "SyntaxError: expected expression, got end of script"
	,    "   ^");
	test("++", "SyntaxError: expected expression, got end of script"
	,    "  ^");
	test("--", "SyntaxError: expected expression, got end of script"
	,    "  ^");
	test("== 1", "SyntaxError: expected expression, got '=='"
	,    "^~  ");
	test("!= 1", "SyntaxError: expected expression, got '!='"
	,    "^~  ");
	test("=== 1", "SyntaxError: expected expression, got '==='"
	,    "^~~  ");
	test("!== 1", "SyntaxError: expected expression, got '!=='"
	,    "^~~  ");
	test("1 ==", "SyntaxError: expected expression, got end of script"
	,    "    ^");
	test("1 !=", "SyntaxError: expected expression, got end of script"
	,    "    ^");
	test("1 ===", "SyntaxError: expected expression, got end of script"
	,    "     ^");
	test("1 !==", "SyntaxError: expected expression, got end of script"
	,    "     ^");
	test("> 1", "SyntaxError: expected expression, got '>'"
	,    "^  ");
	test(">= 1", "SyntaxError: expected expression, got '>='"
	,    "^~  ");
	test("< 1", "SyntaxError: expected expression, got '<'"
	,    "^   ");
	test("<= 1", "SyntaxError: expected expression, got '<='"
	,    "^~  ");
	test("1 >", "SyntaxError: expected expression, got end of script"
	,    "   ^");
	test("1 >=", "SyntaxError: expected expression, got end of script"
	,    "    ^");
	test("1 <", "SyntaxError: expected expression, got end of script"
	,    "   ^");
	test("1 <=", "SyntaxError: expected expression, got end of script"
	,    "    ^");
}

static void testEval (void)
{
	test("", "undefined", NULL);
	test("1;;;;;", "1", NULL);
	test("1;{}", "1", NULL);
	test("1;var a;", "1", NULL);
	test("var a, b = 2; b", "2", NULL);
	test("var x = 2; var y = 39; eval('x + y + 1')", "42", NULL);
	test("var z = '42'; eval(z)", "42", NULL);
	test("var x = 5, z, str = 'if (x == 5) { z = 42; } else z = 0; z'; eval(str)", "42", NULL);
	test("var str = 'if ( a ) { 1+1; } else { 1+2; }', a = true; eval(str)", "2", NULL);
	test("var str = 'if ( a ) { 1+1; } else { 1+2; }', a = false; eval(str)", "3", NULL);
	test("var str = 'function a() {}'; typeof eval(str)", "undefined", NULL);
	test("var str = '(function a() {})'; typeof eval(str)", "function", NULL);
	test("function a() { var n = 456; return eval('n') }; a()", "456", NULL);
	test("var e = eval; function a() { var n = 456; return e('n') }; a()", "ReferenceError: 'n' is not defined"
	,                                                        "^");
	test("var e = eval; function a() { var n = 456; return e('this.parseInt.length') }; a()", "2", NULL);
	test("var a = { b: 'abc', c: function(){ return eval('this.b') } }; a.c()", "abc", NULL);
	test("var e = eval, a = { b: 'abc', c: function(){ return eval('e.b') } }; a.c()", "undefined", NULL);
	test("x", "ReferenceError: 'x' is not defined"
	,    "^");
	test("x = 1", "ReferenceError: 'x' is not defined"
	,    "^~~~~");
	test("new eval('123')", "TypeError: 'eval' is not a constructor"
	,    "    ^~~~       ");
	test("new eval.call('123')", "TypeError: 'eval.call' is not a constructor"
	,    "    ^~~~~~~~~       ");
	test("new eval.apply('123')", "TypeError: 'eval.apply' is not a constructor"
	,    "    ^~~~~~~~~~       ");
	test("eval.call('123', 'this+456')", "[object Global]456", NULL);
}

static void testConvertion (void)
{
	test("var a = { b: {'toString': undefined }}; a.b", "TypeError: cannot convert 'a.b' to primitive"
	,    "                                        ^~~");
	test("var a = { b:{ toString: function () { return this }}} , b = ''; (b + b) + a.b", "TypeError: cannot convert 'a.b' to primitive"
	,    "                                                                          ^~~");
	test("var a = { b:{ toString: function () { return this }}} , b = ''; (b + a.b) + b", "TypeError: cannot convert 'a.b' to primitive"
	,    "                                                                     ^~~     ");
	test("var a = { b:{ toString: function () { return this }}} , b = ''; b + (b + a.b)", "TypeError: cannot convert 'a.b' to primitive"
	,    "                                                                         ^~~ ");
	test("var a = { b:{ toString: function () { return this }}} , b = ''; b + (a.b + b)", "TypeError: cannot convert 'a.b' to primitive"
	,    "                                                                     ^~~     ");
	test("var a = { b:{ toString: function () { return this }}}; switch(a.b){ case 'b': }", "TypeError: cannot convert 'a.b' to primitive"
	,    "                                                              ^~~              ");
	test("var a = { b:{ toString: function () { return this }}}, b = []; b.join[a.b](b)", "TypeError: cannot convert 'a.b' to primitive"
	,    "                                                                      ^~~    ");
	test("var a = { toString: function () { return this } }, b = [], c = [a]; b.join[c[0]]", "TypeError: cannot convert 'c[0]' to primitive"
	,    "                                                                           ^~~~ ");
	test("var a = { toString: function () { return this } }, b = [], c = [a]; b.join[c]", "TypeError: cannot convert 'c' to primitive"
	,    "                                                                           ^ ");
	test("var a = { toString: function () { return this } }, b = [1,2,a]; b.join()", "TypeError: cannot convert 'b' to primitive"
	,    "                                                                ^       ");
	test("var a = { toString: function () { return this } }, b = [1,2,a]; b.join.call(b)", "TypeError: cannot convert 'b' to primitive"
	,    "                                                                            ^ ");
	test("Array.prototype.concat.call(undefined, 123).toString()", "TypeError: cannot convert 'undefined' to object"
	,    "                            ^~~~~~~~~                 ");
	test("var b = []; b.call.join(b)", "TypeError: cannot convert 'b.call' to object"
	,    "            ^~~~~~        ");
	test("var b = []; b.call[1](b)", "TypeError: cannot convert 'b.call' to object"
	,    "            ^~~~~~      ");
	test("var b = []; b.call(b)", "TypeError: 'b.call' is not a function"
	,    "            ^~~~~~   ");
	test("var a = { toString: function () { return this } }, b = ''; b.join[a](b)", "TypeError: cannot convert 'b.join' to object"
	,    "                                                           ^~~~~~      ");
}

static void testException (void)
{
	test("throw undefined", "undefined"
	,    "      ^~~~~~~~~");
	test("throw null", "null"
	,    "      ^~~~");
	test("throw 123", "123"
	,    "      ^~~");
	test("throw 'hello'", "hello"
	,    "       ^~~~~ ");
	test("throw [123, 456]", "123,456"
	,    "      ^~~~~~~~~~");
	test("throw {123: 456}", "[object Object]"
	,    "      ^~~~~~~~~~");
	test("try { throw 'a' } finally { 'b' }", "a"
	,    "             ^                   ");
	test("try { throw 'a' } catch(b){ 'b' }", "b", NULL);
	test("try { throw 'a' } catch(b){ 'b' } 'c'", "c", NULL);
	test("try { throw 'a' } catch(b){ 'b' } finally { 'c' }", "c", NULL);
	test("try { throw 'a' } catch(b){ 'b' } finally { 'c' } 'd'", "d", NULL);
	test("(function(){ try { try { throw 'a' } catch (b) { throw b + 'b'; return 'b' } } catch (c) { throw c + 'c'; return 'c' }})()", "abc"
	,    "                                                                                                 ^~~~~~                   ");
	test("(function(){ try { throw 'a' } catch (b) { throw 'b' } finally { return 'c' } })()", "c", NULL);
	test("(function(){ try { try { throw 'a' } catch (b) { return 'b' } } catch (c) { throw c + 'c'; return 'c' } })()", "b", NULL);
	test("(function(){ try { try { throw 'a' } catch (b) { throw b + 'b'; return 'b' } } catch (c) { return 'c' }})()", "c", NULL);
	test("var a = 0; try { for (;;) { if(++a > 10) throw a; } } catch (e) { e } finally { a + 'f' }", "11f", NULL);
	test("var a = 123; try { throw a + 'abc' } catch(b) { a + b }", "123123abc", NULL);
	test("(function (){ try { return 'a'; } finally { return 'b'; } })()", "b", NULL);
	test("try { throw 'a' }", "SyntaxError: expected catch or finally, got end of script"
	,    "                 ^");
}

static void testOperator (void)
{
	test("+ 10", "10", NULL);
	test("- 10", "-10", NULL);
	test("~ 10", "-11", NULL);
	test("! 10", "false", NULL);
	test("10 * 2", "20", NULL);
	test("10 / 2", "5", NULL);
	test("10 % 8", "2", NULL);
	test("10 % -8", "2", NULL);
	test("-10 % 8", "-2", NULL);
	test("-10 % -8", "-2", NULL);
	test("10 + 1", "11", NULL);
	test("10 - 1", "9", NULL);
	test("10 & 3", "2", NULL);
	test("10 ^ 3", "9", NULL);
	test("10 | 3", "11", NULL);
	test("var u = undefined; u += 123.;", "NaN", NULL);
}

static void testEquality (void)
{
	test("1 == 1", "true", NULL);
	test("1 != 2", "true", NULL);
	test("3 === 3", "true", NULL);
	test("3 !== '3'", "true", NULL);
	test("undefined == undefined", "true", NULL);
	test("null == null", "true", NULL);
	test("true == true", "true", NULL);
	test("false == false", "true", NULL);
	test("'foo' == 'foo'", "true", NULL);
	test("var x = { foo: 'bar' }, y = x; x == y", "true", NULL);
	test("0 == 0", "true", NULL);
	test("+0 == -0", "true", NULL);
	test("0 == false", "true", NULL);
	test("'' == false", "true", NULL);
	test("'' == 0", "true", NULL);
	test("'0' == 0", "true", NULL);
	test("'17' == 17", "true", NULL);
	test("[1,2] == '1,2'", "true", NULL);
	test("null == undefined", "true", NULL);
	test("null == false", "false", NULL);
	test("undefined == false", "false", NULL);
	test("({ foo: 'bar' }) == { foo: 'bar' }", "false", NULL);
	test("0 == null", "false", NULL);
	test("0 == NaN", "false", NULL);
	test("'foo' == NaN", "false", NULL);
	test("NaN == NaN", "false", NULL);
	test("[1,3] == '1,2'", "false", NULL);
	test("undefined === undefined", "true", NULL);
	test("null === null", "true", NULL);
	test("true === true", "true", NULL);
	test("false === false", "true", NULL);
	test("'foo' === 'foo'", "true", NULL);
	test("var x = { foo: 'bar' }, y = x; x === y", "true", NULL);
	test("0 === 0", "true", NULL);
	test("+0 === -0", "true", NULL);
	test("0 === false", "false", NULL);
	test("'' === false", "false", NULL);
	test("'' === 0", "false", NULL);
	test("'0' === 0", "false", NULL);
	test("'17' === 17", "false", NULL);
	test("[1,2] === '1,2'", "false", NULL);
	test("null === undefined", "false", NULL);
	test("null === false", "false", NULL);
	test("undefined === false", "false", NULL);
	test("({ foo: 'bar' }) === { foo: 'bar' }", "false", NULL);
	test("0 === null", "false", NULL);
	test("0 === NaN", "false", NULL);
	test("'foo' === NaN", "false", NULL);
	test("NaN === NaN", "false", NULL);
}

static void testRelational (void)
{
	test("4 > 3", "true", NULL);
	test("4 >= 3", "true", NULL);
	test("3 >= 3", "true", NULL);
	test("3 < 4", "true", NULL);
	test("3 <= 4", "true", NULL);
	test("'toString' in {}", "true", NULL);
	test("'toString' in null", "TypeError: 'null' not an object"
	,    "              ^~~~");
	test("var a; 'toString' in a", "TypeError: 'a' not an object"
	,    "                     ^");
	test("var a = { b: 1, c: 2 }; 'b' in a", "true", NULL);
	test("var a = { b: 1, c: 2 }; 'd' in a", "false", NULL);
	test("var a = [ 'b', 'c' ]; 0 in a", "true", NULL);
	test("var a = [ 'b', 'c' ]; 2 in a", "false", NULL);
	test("var a = [ 'b', 'c' ]; '0' in a", "true", NULL);
	test("var a = [ 'b', 'c' ]; '2' in a", "false", NULL);
	test("function F(){}; var o = new F(); o instanceof F", "true", NULL);
	test("function F(){}; function G(){}; var o = new F(); o instanceof G", "false", NULL);
	test("function F(){}; var o = new F(); o instanceof Object", "true", NULL);
}

static void testConditional (void)
{
	test("var a = null, b; if (a) b = true;", "undefined", NULL);
	test("var a = 1, b; if (a) b = true;", "true", NULL);
	test("var a = undefined, b; if (a) b = true; else b = false", "false", NULL);
	test("var a = 1, b; if (a) b = true; else b = false", "true", NULL);
	test("var b = 0, a = 10;do { ++b } while (a--); b;", "11", NULL);
}

static void testSwitch (void)
{
	test("switch (1) { case 1: 123; case 2: 'abc'; }", "abc", NULL);
	test("switch (2) { case 1: 123; case 2: 'abc'; }", "abc", NULL);
	test("switch (1) { case 1: 123; break; case 2: 'abc'; }", "123", NULL);
	test("switch ('abc') { case 'abc': 123; break; case 2: 'abc'; }", "123", NULL);
	test("switch (123) { default: case 1: 123; break; case 2: 'abc'; }", "123", NULL);
	test("switch (123) { case 1: 123; break; default: case 2: 'abc'; }", "abc", NULL);
	test("switch (123) { case 1: 123; break; case 2: 'abc'; break; default: ({}) }", "[object Object]", NULL);
	test("switch (123) { default: default: ; }", "SyntaxError: more than one switch default"
	,    "                        ^~~~~~~     ");
	test("switch (123) { abc: ; }", "SyntaxError: invalid switch statement"
	,    "               ^~~     ");
}

static void testDelete (void)
{
	test("delete b", "SyntaxError: delete of an unqualified identifier"
	,    "       ^");
	test("var a = { b: 123, c: 'abc' }; a.b", "123", NULL);
	test("var a = { b: 123, c: 'abc' }; delete a.b; a.b", "undefined", NULL);
	test("delete Object.prototype", "TypeError: property 'prototype' is non-configurable and can't be deleted"
	,    "       ^~~~~~~~~~~~~~~~");
}

static void testGlobal (void)
{
	test("typeof this", "undefined", NULL);
	test("typeof global", "object", NULL);
	test("null", "null", NULL);
	test("global.null", "undefined", NULL);
	test("global.Infinity", "Infinity", NULL);
	test("-global.Infinity", "-Infinity", NULL);
	test("global.NaN", "NaN", NULL);
	test("global.undefined", "undefined", NULL);
	test("typeof global.eval", "function", NULL);
	test("decodeURI('abc/def')", "abc/def", NULL);
	test("decodeURI('abc%2fdef')", "abc%2fdef", NULL);
	test("decodeURI('abc%2edef')", "abc.def", NULL);
	test("decodeURI('%E3%83%8F%E3%83%AD%E3%83')", "URIError: malformed URI"
	,    "           ^~~~~~~~~~~~~~~~~~~~~~~~  ");
	test("decodeURI('%E3%83%8F%E3%83%AD%E3%83%')", "URIError: malformed URI"
	,    "           ^~~~~~~~~~~~~~~~~~~~~~~~~  ");
	test("decodeURI('%E3%83%8F%E3%83%AD%E3%83%B')", "URIError: malformed URI"
	,    "           ^~~~~~~~~~~~~~~~~~~~~~~~~~  ");
	test("decodeURI('%E3%83%8F%E3%83%AD%E3%83%BC')", "ハロー", NULL);
	test("decodeURI('%F0%A9%B8%BD')", "𩸽", NULL);
	test("decodeURI('%C3%A7')", "ç", NULL);
	test("decodeURI('%3B%2F%3F%3A%40%26%3D%2B%24%2C%23')", "%3B%2F%3F%3A%40%26%3D%2B%24%2C%23", NULL);
	test("decodeURIComponent('%3B%2F%3F%3A%40%26%3D%2B%24%2C%23')", ";/?:@&=+$,#", NULL);
	test("encodeURI('abc/def')", "abc/def", NULL);
	test("encodeURI('abc%2fdef')", "abc%252fdef", NULL);
	test("encodeURI('ハロー')", "%E3%83%8F%E3%83%AD%E3%83%BC", NULL);
	test("encodeURI('𩸽')", "%F0%A9%B8%BD", NULL);
	test("encodeURI('ç')", "%C3%A7", NULL);
	test("encodeURI(';/?:@&=+$,#')", ";/?:@&=+$,#", NULL);
	test("encodeURIComponent(';/?:@&=+$,#')", "%3B%2F%3F%3A%40%26%3D%2B%24%2C%23", NULL);
}

static void testFunction (void)
{
	test("var a; a.prototype", "TypeError: cannot convert 'a' to object"
	,    "       ^          ");
	test("var a = null; a.prototype", "TypeError: cannot convert 'a' to object"
	,    "              ^          ");
	test("function a() {} a.prototype.toString.length", "0", NULL);
	test("function a() {} a.prototype.toString()", "[object Object]", NULL);
	test("function a() {} a.prototype.hasOwnProperty.length", "1", NULL);
	test("function a() {} a.length", "0", NULL);
	test("function a(b, c) {} a.length", "2", NULL);
	test("function a(b, c) { b + c } a(1, 5)", "undefined", NULL);
	test("function a(b, c) { return b + c } a(1, 5)", "6", NULL);
	test("function a(b, c) { return arguments.toString() } a()", "[object Arguments]", NULL);
	test("function a(b, c) { return arguments.length } a(1, 5, 6)", "3", NULL);
	test("function a(b, c) { return arguments[0] + arguments[1] } a(1, 5)", "6", NULL);
	test("var n = 456; function b(c) { return 'c' + c + n } b(123)", "c123456", NULL);
	test("function a() { var n = 456; function b(c) { return 'c' + c + n } return b } a()(123)", "c123456", NULL);
	test("var a = { a: function() { var n = this; function b(c) { return n + c + this } return b }}; a.a()(123)", "[object Object]123undefined", NULL);
	test("typeof function(){}", "function", NULL);
	test("function a(a, b){ return a + b } a.apply(null, 1, 2)", "TypeError: arguments is not an object"
	,    "                                               ^    ");
	test("function a(a, b){ return this + a + b } a.apply(10, [1, 2])", "13", NULL);
	test("function a(a, b){ return this + a + b } a.call(10, 1, 2)", "13", NULL);
	test("function a(){ return arguments }; a()", "[object Arguments]", NULL);
	test("function a(){ return arguments }; a.call()", "[object Arguments]", NULL);
	test("function a(){ return arguments }; a.apply()", "[object Arguments]", NULL);
	test("typeof Function", "function", NULL);
	test("Function.length", "1", NULL);
	test("typeof Function.prototype", "function", NULL);
	test("Function.prototype.length", "0", NULL);
	test("Function.prototype(1, 2, 3)", "undefined", NULL);
	test("typeof Function()", "function", NULL);
	test("Function()()", "undefined", NULL);
	test("Function('return 123')()", "123", NULL);
	test("new Function('a', 'b', 'c', 'return a+b+c')(1, 2, 3)", "6", NULL);
	test("new Function('a, b, c', 'return a+b+c')(1, 2, 3)", "6", NULL);
	test("new Function('a,b', 'c', 'return a+b+c')(1, 2, 3)", "6", NULL);
	test("function a(){ var b = { c: 123 }; function d() { return b.c }; return d; } for (var i = 0; !i; ++i){ var b = a(); } b()", "123", NULL);
	test("123 .toFixed.call.apply([ 123 ], [ 'abc', 100 ])", "TypeError: not a function"
	,    "                        ^~~~~~~                 ");
	test("123 .toFixed.call.call([ 123 ], 'abc', 100)", "TypeError: not a function"
	,    "                       ^~~~~~~             ");
	test("123 .toFixed.call.apply(123 .toFixed, [ 'abc', 100 ])", "TypeError: not a number"
	,    "                                         ^~~         ");
	test("123 .toFixed.apply.call(123 .toFixed, 'abc', [ 100 ])", "TypeError: not a number"
	,    "                                       ^~~           ");
	test("123 .toFixed.call.apply(123 .toFixed, [ 456, 100 ])", "RangeError: precision 100 out of range"
	,    "                                             ^~~   ");
	test("123 .toFixed.apply.call(123 .toFixed, 456, [ 100 ])", "RangeError: precision 100 out of range"
	,    "                                             ^~~   ");
	test("123 .toFixed.apply.apply(123 .toFixed, [ 456, 100 ])", "TypeError: arguments is not an object"
	,    "                                              ^~~   ");
	test("123 .toFixed.apply.apply(123 .toFixed, [ 'abc', [ 100 ] ])", "TypeError: not a number"
	,    "                                          ^~~             ");
	test("123 .toFixed.apply.apply(123 .toFixed, [ 456, [ 100 ] ])", "RangeError: precision 100 out of range"
	,    "                                                ^~~     ");
	test("var a = [123,'abc','def']; Object.defineProperty(a, 1, {get: function(){ return this[1]; },set: function(v){}}); a.shift()", "RangeError: maximum depth exceeded"
	,    "                                                                         ^~~~~~~~~~~~~~                                   ");
	test("function F(){}; F.prototype = 123; new F", "[object Object]", NULL);
	test("var f = function(){ return this }.bind(undefined); f()", "undefined", NULL);
	test("var f = function(){ return this }.bind(null); f()", "null", NULL);
	test("var f = function(){ return this }.bind(123); f()", "123", NULL);
	test("var f = function(a, b){ return arguments }.bind(123); [].join.call(f(1, 2))", "1,2", NULL);
	test("var f = function(a, b){ return arguments }.bind(123, 1); [].join.call(f(2, 3))", "1,2,3", NULL);
	test("var f = function(a, b){ return arguments }.bind(123, 1, 2, 3); [].join.call(f(4, 5))", "1,2,3,4,5", NULL);
	test("function f1(x, x) { return x; } f1(1, 2)", "2", NULL);
	test("function f(n,a,b){ if (n > 0) return f(n - 1, b, a + b); else return a }; f(10, 0, 1)", "55", NULL);
	test("function f(n,a,b){ if (arguments[0] > 0) return f(arguments[0] - 1, arguments[2], arguments[1] + arguments[2]); else return arguments[1] }; f(10, 0, 1)", "55", NULL);
}

static void testLoop (void)
{
	test("var a = 0; for (;;) if (++a > 10) break; a", "11", NULL);
	test("var a; for (a = 0;;) if (++a > 10) break; a", "11", NULL);
	test("for (var a = 0;;) if (++a > 10) break; a", "11", NULL);
	test("for (var a = 0; a <= 10;) ++a; a", "11", NULL);
	test("for (var a = 0; a <= 10; ++a); a", "11", NULL);
	test("for (var a = 0, b = 0; a <= 10; ++a) ++b; b", "11", NULL);
	test("var a = 123; for (var i = 10; i >= 0; i--) --a; (a + a)", "224", NULL);
	test("var a = 10, b = 0; while (a--) ++b;", "10", NULL);
	test("var a = 10, b = 0; do ++b; while (a--)", "11", NULL);
	test("var a = [1], r = ''; for (var i = 0; i < a[0]; ++i){ r += i; } r += 'a';", "0a", NULL);
	test("var a = { 'a': 123 }, b; for (b in a) a[b];", "123", NULL);
	test("var a = { 'a': 123 }; for (var b in a) a[b];", "123", NULL);
	test("var a = { 'a': 123 }; for (b in a) ;", "ReferenceError: 'b' is not defined"
	,    "                           ^        ");
	test("var a = [ 'a', 123 ], b; for (b in a) b + ':' + a[b];", "1:123", NULL);
	test("var a = [ 'a', 123 ], b; for (b in a) typeof b;", "string", NULL);
	test("continue abc;", "SyntaxError: continue must be inside loop"
	,    "^~~~~~~~     ");
	test("while (1) continue abc;", "SyntaxError: label not found"
	,    "                   ^~~ ");
	test("break abc;", "SyntaxError: break must be inside loop or switch"
	,    "^~~~~     ");
	test("while (1) break abc;", "SyntaxError: label not found"
	,    "                ^~~ ");
}

static void testThis (void)
{
	test("function a() { return typeof this; } a()", "undefined", NULL);
	test("var a = { b: function () { return typeof this; } }; a.b()", "object", NULL);
	test("var a = { b: function () { return this; } }; a.b().b === a.b", "true", NULL);
}

static void testObject (void)
{
	test("Object", "function Object() [native code]", NULL);
	test("Object.prototype.toString.call(Object.prototype)", "[object Object]", NULL);
	test("Object.prototype.constructor", "function Object() [native code]", NULL);
	test("Object.prototype", "[object Object]", NULL);
	test("var a = { a: 1, 'b': 2, '1': 3 }; a.a", "1", NULL);
	test("var a = { a: 1, 'b': 2, '1': 3 }; a['a']", "1", NULL);
	test("var a = { a: 1, 'b': 2, '1': 3 }; a['b']", "2", NULL);
	test("var a = { a: 1, 'b': 2, '1': 3 }; a.b", "2", NULL);
	test("var a = { a: 1, 'b': 2, 1: 3 }; a[1]", "3", NULL);
	test("var a = { a: 1, 'b': 2, '1': 3 }; a[1]", "3", NULL);
	test("var a = { a: 1, 'b': 2, '1': 3 }, c = 1; a[c]", "3", NULL);
	test("var a = { a: 1, 'b': 2, '1': 3 }; delete a['a']; a['a']", "undefined", NULL);
	test("var a = { a: 1, 'b': 2, '1': 3 }; delete a['a']; a['a'] = 123; a['a']", "123", NULL);
	test("var a = { a: 123 }; a.toString()", "[object Object]", NULL);
	test("var a = { a: 123 }; a.valueOf() === a", "true", NULL);
	test("var a = { a: 123 }; a.hasOwnProperty('a')", "true", NULL);
	test("var a = { a: 123 }; a.hasOwnProperty('toString')", "false", NULL);
	test("var a = {}, b = {}; a.isPrototypeOf(123)", "false", NULL);
	test("var a = {}, b = {}; a.isPrototypeOf(b)", "false", NULL);
	test("var a = {}, b = {}; Object.getPrototypeOf(b).isPrototypeOf(a)", "true", NULL);
	test("var a = { a: 123 }; a.propertyIsEnumerable('a')", "true", NULL);
	test("var a = {}; a.propertyIsEnumerable('toString')", "false", NULL);
	test("var a = {}; Object.getPrototypeOf(a).propertyIsEnumerable('toString')", "false", NULL);
	test("var a = {}, b = {}; Object.getPrototypeOf(a) === Object.getPrototypeOf(b)", "true", NULL);
	test("var a = {}; Object.getPrototypeOf(Object.getPrototypeOf(a))", "undefined", NULL);
	test("var a = { a: 123 }; Object.getOwnPropertyDescriptor(a, 'a').value", "123", NULL);
	test("var a = { a: 123 }; Object.getOwnPropertyDescriptor(a, 'a').writable", "true", NULL);
	test("Object.getOwnPropertyNames({ a:'!', 2:'@', 'b':'#'}).toString()", "2,a,b", NULL);
	test("var a = {}, o = ''; a['a'] = 'abc'; a['c'] = 123; a['b'] = undefined; for (var b in a) o += b + a[b]; o", "aabcc123bundefined", NULL);
	test("var a = {}; a.null = 123; a.null", "123", NULL);
	test("var a = {}; a.function = 123; a.function", "123", NULL);
	test("typeof Object", "function", NULL);
	test("typeof Object.prototype", "object", NULL);
	test("Object.prototype.toString.call(true)", "[object Boolean]", NULL);
	test("Object.prototype.toString.call('abc')", "[object String]", NULL);
	test("Object.prototype.toString.call(123)", "[object Number]", NULL);
	test("Object.prototype.toString.call(function(){})", "[object Function]", NULL);
	test("var o = {}; Object.defineProperty(o, 'p', { get:undefined }); o.p", "undefined", NULL);
	test("var o = {}; Object.defineProperty(o, 'p', { get:function(){ return 123 } }); o.p", "123", NULL);
	test("var o = {}; Object.defineProperty(o, 'p', { get:function(){ return 123 } }); o.toString.call(Object.getOwnPropertyDescriptor(o, 'p').get)", "[object Function]", NULL);
	test("var a = { b:1 }; ++a.b", "2", NULL);
	test("var a = { b:1 }; ++a['b']", "2", NULL);
	test("var a = {}; ++a.b", "NaN", NULL);
	test("var o = {}; Object.defineProperty(o, 'a', { value: 123 }); var b = o.a; b += 123;", "246", NULL);
	test("var o = { get p(){ return this._p }, set p(v){ this._p = v; } }; o.p = 123; String(o.p) + o._p", "123123", NULL);
	test("var o = { get p(){ return this._p }, set p(v){ this._p = v; } }; Object.defineProperty(o, 'p', { value: 123 }); String(o.p) + o._p", "123undefined", NULL);
	test("var p = {}, o; Object.defineProperty(p, 'p', { value:123 }); o = Object.create(p); o.p = 456", "TypeError: 'p' is readonly"
	,    "                                                                                   ^~~~~~~~~");
	test("var p = {}, o; Object.defineProperty(p, 'p', { value:123, writable: true }); o = Object.create(p); Object.seal(o); o.p = 456", "TypeError: object is not extensible"
	,    "                                                                                                                   ^~~~~~~~~");
	test("var a = ['#'], b = Object.create(a); b[0]", "#", NULL);
	test("var a = ['#'], b = Object.create(a, { '0': {value:'!'} }); b[0]", "!", NULL);
	test("var a = {}; Object.freeze(a); ++a.b", "TypeError: object is not extensible"
	,    "                                ^~~");
	test("var a = {}; Object.freeze(a); a.b += 2", "TypeError: object is not extensible"
	,    "                              ^~~~~~~~");
	test("var a = {}; Object.freeze(a); a.b = 2", "TypeError: object is not extensible"
	,    "                              ^~~~~~~");
	test("var a = {}; Object.freeze(a); a['b'] = 2", "TypeError: object is not extensible"
	,    "                              ^~~~~~~~~~");
	test("var a = { b:1 }; ++a.b", "2", NULL);
	test("var a = { b:1 }; Object.freeze(a); ++a.b", "TypeError: 'b' is read-only property"
	,    "                                   ^~~~~");
	test("var a = { b:1 }; Object.freeze(a); a.b += 1", "TypeError: 'b' is read-only property"
	,    "                                   ^~~~~~~~");
	test("var a = { b:1 }; Object.freeze(a); a.b -= 1", "TypeError: 'b' is read-only property"
	,    "                                   ^~~~~~~~");
	test("var a = { b:1 }; Object.freeze(a); a.b += 2", "TypeError: 'b' is read-only property"
	,    "                                   ^~~~~~~~");
	test("var a = { b:1 }; Object.freeze(a); a.b = 2", "TypeError: 'b' is read-only property"
	,    "                                   ^~~~~~~");
	test("var a = { b:1 }; Object.freeze(a); a['b'] = 2", "TypeError: 'b' is read-only property"
	,    "                                   ^~~~~~~~~~");
	test("var a = { v: 1, get b() { return this.v }, set b(v) { this.v = v } }; ++a.b", "2", NULL);
	test("var a = { v: 1, get b() { return this.v } }; ++a.b", "TypeError: 'b' is read-only accessor"
	,    "                                             ^~~~~");
	test("var a = { v: 1, get b() { return this.v } }; a.b += 2", "TypeError: 'b' is read-only accessor"
	,    "                                             ^~~~~~~~");
	test("var a = { v: 1, get b() { return this.v } }; a.b = 2", "TypeError: 'b' is read-only accessor"
	,    "                                             ^~~~~~~");
	test("var a = { v: 1, get b() { return this.v } }; a['b'] = 2", "TypeError: 'b' is read-only accessor"
	,    "                                             ^~~~~~~~~~");
	test("var o = {}; Object.defineProperty(o, 'a', 123); o.a = 1;", "TypeError: not an object"
	,    "                                          ^~~           ");
	test("var o = {}; Object.defineProperty(o, 'a', {}); o.a = 1;", "TypeError: 'a' is read-only property"
	,    "                                               ^~~~~~~ ");
	test("var o = {}; Object.defineProperty(o, 2, {}); o[2] = 1;", "TypeError: '2' is read-only property"
	,    "                                             ^~~~~~~~ ");
	test("var o = {}; Object.defineProperty(o, 'p', { get: 123 });", "TypeError: getter is not a function"
	,    "                                          ^~~~~~~~~~~~  ");
	test("var o = {}; Object.defineProperty(o, 'p', { get: function(){}, value: 2 });", "TypeError: value & writable forbidden when a getter or setter are set"
	,    "                                          ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~  ");
	test("var o = {}; Object.defineProperty(o, 2, { value: 123 }); Object.defineProperty(o, 2, { enumerable: true });", "TypeError: '2' is non-configurable"
	,    "                                                                                  ^                        ");
	test("var o = {}; Object.defineProperty(o, 'p', { set: function(){} }); Object.defineProperty(o, 'p', { value: 1 });", "TypeError: 'p' is non-configurable"
	,    "                                                                                            ^                 ");
	test("var o = {}; Object.defineProperty(o, 'p', { set: function(){} }); Object.defineProperty(o, 'p', { get: function(){} });", "TypeError: 'p' is non-configurable"
	,    "                                                                                            ^                 ");
	test("var o = {}; Object.defineProperty(o, 'p', { set: function(){} }); Object.defineProperty(o, 'p', { set: function(){} });", "TypeError: 'p' is non-configurable"
	,    "                                                                                            ^                          ");
	test("var o = {}; Object.defineProperty(o, 'p', { value: 123, configurable: false, writable: false });Object.defineProperty(o, 'p', { value: 'abc' }); o", "TypeError: 'p' is non-configurable"
	,    "                                                                                                                          ^                       ");
	test("var o = {}; Object.defineProperty(o, 'p', { value: 123, configurable: true, writable: false }); Object.defineProperty(o, 'p', { value: 'abc' }); o", "[object Object]", NULL);
	test("var o = []; Object.defineProperties(o, { 1: { value: 123 }, 3: { value: '!' } }); o", ",123,,!", NULL);
	test("var o = []; Object.defineProperties(o, 123); o", "", NULL);
	test("var o = []; Object.defineProperties(o, null); o", "TypeError: cannot convert 'null' to object"
	,    "                                       ^~~~    ");
	test("var o = []; Object.defineProperties(o); o", "TypeError: cannot convert undefined to object"
	,    "                                     ^   ");
	test("var o = []; Object.defineProperties(); o", "TypeError: not an object"
	,    "                                    ^   ");
	test("var o = []; Object.defineProperties(1); o", "TypeError: not an object"
	,    "                                    ^    ");
}

static void testError (void)
{
	test("Error", "function Error() [native code]", NULL);
	test("Object.prototype.toString.call(Error.prototype)", "[object Error]", NULL);
	test("Error.prototype.constructor", "function Error() [native code]", NULL);
	test("Error.prototype", "Error", NULL);
	test("RangeError.prototype", "RangeError", NULL);
	test("ReferenceError.prototype", "ReferenceError", NULL);
	test("SyntaxError.prototype", "SyntaxError", NULL);
	test("TypeError.prototype", "TypeError", NULL);
	test("URIError.prototype", "URIError", NULL);
	test("Error.prototype.name", "Error", NULL);
	test("RangeError.prototype.name", "RangeError", NULL);
	test("RangeError('test')", "RangeError: test", NULL);
	test("Object.prototype.toString.call(RangeError())", "[object Error]", NULL);
	test("var e = new Error(); Object.prototype.toString.call(e)", "[object Error]", NULL);
	test("function a(){ 123 .toFixed.call('abc', 100) }; a()", "TypeError: not a number"
	,    "                                 ^~~              ");
	test("function a(){ 123 .toFixed.call(456, 100) }; a()", "RangeError: precision 100 out of range"
	,    "                                     ^~~        ");
	test("function a(){ 123 .toFixed.apply(456, [ 100 ]) }; a()", "RangeError: precision 100 out of range"
	,    "                                        ^~~          ");
	test("''.toString.call(123)", "TypeError: not a string"
	,    "                 ^~~ ");
	test("function a(){}; a.toString = function(){ return 'abc'; }; var b = 123; a + b", "abc123", NULL);
	test("function a(){}; a.toString = function(){ throw Error('test'); }; a", "Error: test"
	,    "                                               ^~~~~~~~~~~~~      ");
	test("function a(){}; a.toString = function(){ return {}; }; a", "TypeError: cannot convert 'a' to primitive"
	,    "                                                       ^");
}

static void testAccessor (void)
{
	test("var a = { get () {} }", "SyntaxError: expected identifier, got '('"
	,    "              ^      ");
	test("var a = { get a (b, c) {} }", "SyntaxError: getter functions must have no arguments"
	,    "                 ^~~~      ");
	test("var a = { set () {} }", "SyntaxError: expected identifier, got '('"
	,    "              ^      ");
	test("var a = { set a () {} }", "SyntaxError: setter functions must have one argument"
	,    "                 ^     ");
	test("var a = { get a() { return 123 } }; a.a", "123", NULL);
	test("var a = { get a() { return 123 } }; a['a']", "123", NULL);
	test("var a = { a: 'abc', set a(b) {}, get a() { return 123 } }; a.a", "123", NULL);
	test("var a = { set a(b) {}, a: 'abc', get a() { return 123 } }; a.a", "123", NULL);
	test("var a = { set a(b) {}, get a() { return 123 }, a: 'abc' }; a.a", "abc", NULL);
	test("var a = { _a: 'u', get a() { return this._a } }; a.a", "u", NULL);
	test("var a = { _a: 'u', set a(v) { this._a = v } }; a.a = 123; a._a", "123", NULL);
	test("var a = { _a: 'u', set a(v) { this._a = v }, get a() { return this._a } }; a.a = 123; a.a", "123", NULL);
	test("var a = { _a: 'u', set a(v) { this._a = v }, get a() { return this._a } }; a.a += 123; a._a", "u123", NULL);
	test("var a = { _a: 2, set a(v) { this._a = v }, get a() { return this._a } }; ++a.a", "3", NULL);
	test("var a = { _a: 2, set a(v) { this._a = v }, get a() { return this._a } }; ++a.a; a._a", "3", NULL);
	test("({ a: function(){ return 123 } }).a()", "123", NULL);
	test("({ a: function(){ return this.b }, b: 456 }).a()", "456", NULL);
	test("var a = { get: 123 }; a.get", "123", NULL);
	test("var a = { set: 123 }; a.set", "123", NULL);
	test("var a = { get get() { return 123 } }; a.get", "123", NULL);
	test("var a = { get set() { return 123 } }; a.set", "123", NULL);
}

static void testArray (void)
{
	test("Array", "function Array() [native code]", NULL);
	test("Object.prototype.toString.call(Array.prototype)", "[object Array]", NULL);
	test("Array.prototype.constructor", "function Array() [native code]", NULL);
	test("Array.prototype", "", NULL);
	test("typeof Array", "function", NULL);
	test("typeof Array.prototype", "object", NULL);
	test("var a = [ 'a', 'b', 'c' ]; a['a'] = 123; a[1]", "b", NULL);
	test("var a = [ 'a', 'b', 'c' ]; a['1'] = 123; a[1]", "123", NULL);
	test("var a = [ 'a', 'b', 'c' ]; a['a'] = 123; a.a", "123", NULL);
	test("var a = [ 'a', 'b', 'c' ]; a['a'] = 123; a", "a,b,c", NULL);
	test("var a = [], o = ''; a[0] = 'abc'; a[4] = 123; a[2] = undefined; for (var b in a) o += b + a[b]; o", "0abc2undefined4123", NULL);
	test("var a = [1, 2, 3], o = ''; delete a[1]; for (var b in a) o += b; o", "02", NULL);
	test("var a = [1, , 3], o = ''; for (var b in a) o += b; o", "02", NULL);
	test("var a = [1, , 3], o = ''; a[1] = undefined; for (var b in a) o += b; o", "012", NULL);
	test("[1, , 3]", "1,,3", NULL);
	test("[1, , 3] + ''", "1,,3", NULL);
	test("[1, , 3].toString()", "1,,3", NULL);
	test("typeof []", "object", NULL);
	test("Object.prototype.toString.call([])", "[object Array]", NULL);
	test("Array.isArray([])", "true", NULL);
	test("Array.isArray({})", "false", NULL);
	test("Array.isArray(Array.prototype)", "true", NULL);
	test("var alpha = ['a', 'b', 'c'], numeric = [1, 2, 3]; alpha.concat(numeric).toString()", "a,b,c,1,2,3", NULL);
	test("Array.prototype.concat.call(123, 'abc', [{}, 456]).toString()", "123,abc,[object Object],456", NULL);
	test("var a = [1, 2]; a.length", "2", NULL);
	test("var a = [1, 2]; a.length = 5; a.length", "5", NULL);
	test("var a = [1, 2]; a[5] = 5; a.length", "6", NULL);
	test("var a = [1, 2]; a.join()", "1,2", NULL);
	test("var a = [1, 2]; a.join('abc')", "1abc2", NULL);
	test("var a = [1, 2], b = ''; b += a.pop(); b += a.pop(); b += a.pop()", "21undefined", NULL);
	test("var a = [1, 2]; a.push(); a.toString()", "1,2", NULL);
	test("var a = [1, 2]; a.push('abc', 345)", "4", NULL);
	test("var a = [1, 2]; a.push('abc', 345); a", "1,2,abc,345", NULL);
	test("var a = [1, 2]; a.unshift(); a.toString()", "1,2", NULL);
	test("var a = [1, 2]; a.unshift('abc', 345)", "4", NULL);
	test("var a = [1, 2]; a.unshift('abc', 345); a", "abc,345,1,2", NULL);
	test("var a = [123, 'abc', 'def']; Object.defineProperty(a, 1, {get: function(){ return this[2]; },set: function(v){}}); a.unshift(); a", "123,def,def", NULL);
	test("var a = [1]; a.reverse(); a.toString()", "1", NULL);
	test("var a = [1,2]; a.reverse(); a.toString()", "2,1", NULL);
	test("var a = [1,2,'abc']; a.reverse(); a.toString()", "abc,2,1", NULL);
	test("var a = [1, 2], b = ''; b += a.shift(); b += a.shift(); b += a.shift()", "12undefined", NULL);
	test("var a = ['abc', 'def']; Object.defineProperty(a, 0, {get: function(){ return this[1]; }}); a.shift()", "TypeError: '0' is read-only accessor"
	,    "                                                                                           ^~~~~~~~~");
	test("var a = [123, 'abc', 'def']; Object.defineProperty(a, 1, {get: function(){ return this[2]; },set: function(v){}}); a.shift()", "123", NULL);
	test("var a = [123, 'abc', 'def']; Object.defineProperty(a, 1, {get: function(){ return this[2]; },set: function(v){}}); a.shift(); a", "def,", NULL);
	test("var a = [123, 'abc', 'def']; Object.defineProperty(a, 1, {get: function(){ return this.length },set: function(v){}}); a.shift(); a.push(123, 456); a", "3,4,123,456", NULL);
	test("var a = [1, 2, 'abc', null, 456]; a.slice().toString()", "1,2,abc,,456", NULL);
	test("var a = [1, 2, 'abc', null, 456]; a.slice(2).toString()", "abc,,456", NULL);
	test("var a = [1, 2, 'abc', null, 456]; a.slice(2,4).toString()", "abc,", NULL);
	test("var a = [1, 2, 'abc', null, 456]; a.slice(-2).toString()", ",456", NULL);
	test("var a = [1, 2, 'abc', null, 456]; a.slice(-4,-2).toString()", "2,abc", NULL);
	test("var a = [123, 'abc', 'def']; Object.defineProperty(a, 1, {get: function(){ return this[2]; },set: function(v){}}); a.slice(-2)", "def,def", NULL);
	test("var a = Array(); a.length", "0", NULL);
	test("var a = Array(4); a.length", "4", NULL);
	test("var a = Array(4.5); a.length", "RangeError: invalid array length"
	,    "              ^~~           ");
	test("var a = Array(-4); a.length", "RangeError: invalid array length"
	,    "              ^~           ");
	test("var a = Array('abc'); a.length", "1", NULL);
	test("var a = Array(123, 'abc'); a.length", "2", NULL);
	test("var a = [ 123 ]; function b(){ arguments[0] = 456; }; b.apply(null, a); a;", "123", NULL);
	test("function b(){ return arguments; }; var a = b.call(null, 123); a;", "[object Arguments]", NULL);
	test("var a = [ 'abc', 'def' ], r = ''; Object.defineProperty(a, 1, {get: function(){ return this[0]; }}); for (var p in a) r += p + a[p]; r", "0abc1abc", NULL);
	test("var a = [ 'abc', 'def' ], r = ''; Object.defineProperty(a, 1, {get: function(){ return this[0]; }}); a.join('|')", "abc|abc", NULL);
	test("var a = [ 'abc', 'def' ], r = ''; Object.defineProperty(a, 1, {get: function(){ return this[0]; }}); a + '^'", "abc,abc^", NULL);
	test("var a = ['abc', 'def'], b = [123], r = ''; Object.defineProperty(a, 1, {get : function(){ return this[0]; }}); b.concat(a)", "123,abc,abc", NULL);
	test("var a = [ 'abc', 'def' ]; Object.defineProperty(a, 1, {get: function(){ return this.length; }}); a.pop()", "2", NULL);
	test("var a = [ 'abc', 'def' ]; Object.defineProperty(a, 1, {get: function(){ return this.length; }}); a = a.reverse()", "TypeError: '1' is read-only accessor"
	,    "                                                                                                     ^~~~~~~~~~~");
	test("var a = [ 'abc', 'def' ]; Object.defineProperty(a, 1, {get: function(){ return this.length; }, set: function(v){ }}); a.push(123); a[1]", "3", NULL);
	test("var a = [ 'abc', 'def' ]; Object.defineProperty(a, 1, {get: function(){ return this.length; }, set: function(v){ }}); a = a.reverse(); a.push(123); a[0]", "2", NULL);
	test("var a = [ 'abc', 'def' ]; Object.defineProperty(a, 0, {get: function(){ return this[1]; }, set: function(v){ }}); a = a.shift()", "def", NULL);
	test("var o = { toString: function(){ return ' world!' } }, a = [ 'hello', o ]; a", "hello, world!", NULL);
	test("var a = [ 123, 456 ], a = [ 'hello', a ]; a.toString = function(){ return 'test' }; [ a ]", "test", NULL);
	test("var o = [], c = 0, a = [o, o]; Object.defineProperty(o, '0', { get: function (){ return ++c } }); a", "1,2", NULL);
	test("var o = {}, a = ['abc',123]; o[a] = 'test'; Object.getOwnPropertyNames(o)", "abc,123", NULL);
	test("var a = [1,2,3,4]; Object.defineProperty(a, 'length', {value:2}); a", "1,2", NULL);
	test("[].join.call({1:'@'})", "", NULL);
	test("[].join.call({1:'@', length:2})", ",@", NULL);
	test("[].join.call({1:'@', length:12})", ",@,,,,,,,,,,", NULL);
	test("[].toString.call({1:'@'})", "[object Object]", NULL);
	test("function f(){ return arguments.hasOwnProperty('length') }; f()", "true", NULL);
	test("function f(){ return Object.getOwnPropertyDescriptor(arguments, 'length') }; f()", "[object Object]", 0);
	test("function f(){ return Object.getOwnPropertyDescriptor(arguments, 'callee') }; f()", "[object Object]", 0);
	test("function f(){ return Object.getOwnPropertyNames(arguments) }; f()", "length,callee", NULL);
	test("function f(){ return arguments.callee }; f()", "TypeError: 'callee' cannot be accessed in this context"
	,    "                     ^~~~~~~~~~~~~~~~       ");
}

static void testBoolean (void)
{
	test("Boolean", "function Boolean() [native code]", NULL);
	test("Object.prototype.toString.call(Boolean.prototype)", "[object Boolean]", NULL);
	test("Boolean.prototype.constructor", "function Boolean() [native code]", NULL);
	test("Boolean.prototype", "false", NULL);
	test("var b = new Boolean('a'); b.valueOf() === true", "true", NULL);
	test("var b = Boolean('a'); b.valueOf() === true", "true", NULL);
	test("var b = new Boolean(0); b.toString()", "false", NULL);
	test("var b = Boolean(); b.toString()", "false", NULL);
	test("if (new Boolean(false)) 1; else 2;", "1", NULL);
	test("Boolean.prototype.toString.call(123)", "TypeError: not a boolean"
	,    "                                ^~~ ");
	test("Boolean.prototype.toString.call(false)", "false", NULL);
	test("Boolean.prototype.toString.call(new Boolean(0))", "false", NULL);
	test("Boolean.prototype.toString.call(new Boolean(true))", "true", NULL);
	test("Boolean.prototype.toString(true)", "false", NULL);
}

static void testNumber (void)
{
	test("Number", "function Number() [native code]", NULL);
	test("Object.prototype.toString.call(Number.prototype)", "[object Number]", NULL);
	test("Number.prototype.constructor", "function Number() [native code]", NULL);
	test("Number.prototype", "0", NULL);
	test("Number.toString", "function toString() [native code]", NULL);
	test("Number.toString()", "function Number() [native code]", NULL);
	test("Number.toString.call(null)", "TypeError: not a function", NULL);
	test("0xf", "15", NULL);
	test("0xff", "255", NULL);
	test("0xffff", "65535", NULL);
	test("0xffffffff", "4294967295", NULL);
	test("String(0xffffffffffffffff).slice(0, -4)", "1844674407370955", NULL);
	test("String(0x3635c9adc5de9e0000).slice(0, -6)", "999999999999999", NULL);
	test("0x3635c9adc5de9f0000", "1e+21", NULL);
	test("-0xf", "-15", NULL);
	test("-0xff", "-255", NULL);
	test("-0xffff", "-65535", NULL);
	test("-0xffffffff", "-4294967295", NULL);
	test("String(-0xffffffffffffffff).slice(0, -4)", "-1844674407370955", NULL);
	test("String(-0x3635c9adc5de9e0000).slice(0, -6)", "-999999999999999", NULL);
	test("-0x3635c9adc5de9f0000", "-1e+21", NULL);
	test("0x7fffffff | 0", "2147483647", NULL);
	test("0xffffffff | 0", "-1", NULL);
	test("0x1fffffff0 | 0", "-16", NULL);
	test("-0x7fffffff | 0", "-2147483647", NULL);
	test("-0xffffffff | 0", "1", NULL);
	test("-0x1fffffff0 | 0", "16", NULL);
	test("0123", "83", NULL);
	test("-0123", "-83", NULL);
	test("parseInt('0123')", "123", NULL);
	test("parseInt('-0123')", "-123", NULL);
	test("123.456.toString(2)", "1111011", NULL);
	test("123.456.toString(4)", "1323", NULL);
	test("123.456.toString(8)", "173", NULL);
	test("123.456.toString()", "123.456", NULL);
	test("123.456.toString(10)", "123.456", NULL);
	test("123.456.toString(16)", "7b", NULL);
	test("123.456.toString(24)", "53", NULL);
	test("123.456.toString(32)", "3r", NULL);
	test("123.456.toString(36)", "3f", NULL);
	test("2147483647..toString(2)", "1111111111111111111111111111111", NULL);
	test("(-2147483647).toString(2)", "-1111111111111111111111111111111", NULL);
	test("2147483647..toString(8)", "17777777777", NULL);
	test("(-2147483647).toString(8)", "-17777777777", NULL);
	test("Number.MAX_VALUE.toString(10)", "1.79769e+308", NULL);
	test("Number.MIN_VALUE.toString(10)", "4.94065645841247e-324", NULL);
	test("(2147483647).toString(16)", "7fffffff", NULL);
	test("(-2147483647).toString(16)", "-7fffffff", NULL);
	test("-2147483647..toString(16)", "NaN", NULL);
	test("-2147483647..toString(10)", "-2147483647", NULL);
	test("-2147483647 .toString()", "-2147483647", NULL);
	test("123.456.toExponential()", "1.234560e+2", NULL);
	test("123.456.toFixed()", "123.456000", NULL);
	test("123.456.toPrecision()", "123.456", NULL);
	test("123.456.toExponential(10)", "1.2345600000e+2", NULL);
	test("123.456.toFixed(10)", "123.4560000000", NULL);
	test("123.456.toPrecision(10)", "123.456", NULL);
	test("Math.round(2147483647.4)", "2147483647", NULL);
	test("Math.round(-2147483647.5)", "-2147483647", NULL);
	test("Math.round(2147483647.5)", "2147483648", NULL);
	test("Math.round(-2147483647.6)", "-2147483648", NULL);
	test("var orig = Number.prototype.toString; Number.prototype.toString = function(){ return this + 'abc' }; var r = 123..toString(); Number.prototype.toString = orig; r", "123abc", NULL);
	test("Number()", "0", NULL);
	test("Number(undefined)", "NaN", NULL);
}

static void testString (void)
{
	test("String", "function String() [native code]", NULL);
	test("Object.prototype.toString.call(String.prototype)", "[object String]", NULL);
	test("String.prototype.constructor", "function String() [native code]", NULL);
	test("String.prototype", "", NULL);
	test("''.toString.call(123)", "TypeError: not a string"
	,    "                 ^~~ ");
	test("''.valueOf.call(123)", "TypeError: not a string"
	,    "                ^~~ ");
	test("'aべcaべc'.length", "6", NULL);
	test("var a = new String('aべcaべc'); a.length = 12; a.length", "TypeError: 'length' is read-only property"
	,    "                     べ  べ     ^~~~~~~~~~~~~          ");
	test("'abせd'.slice()", "abせd", NULL);
	test("'abせd'.slice(1)", "bせd", NULL);
	test("'abせd'.slice(undefined,2)", "ab", NULL);
	test("'abせd'.slice(1,2)", "b", NULL);
	test("'abせd'.slice(-2)", "せd", NULL);
	test("'abせd'.slice(undefined,-2)", "ab", NULL);
	test("'abせd'.slice(-2,-1)", "せ", NULL);
	test("'abせd'.slice(2,1)", "", NULL);
	test("'abせd'.slice(2048)", "", NULL);
	test("'abせd'.slice(0,2048)", "abせd", NULL);
	test("'abせd'.slice(-2048)", "abせd", NULL);
	test("'abせd'.slice(0,-2048)", "", NULL);
	test("'abせd'.substring()", "abせd", NULL);
	test("'abせd'.substring(1)", "bせd", NULL);
	test("'abせd'.substring(undefined,2)", "ab", NULL);
	test("'abせd'.substring(1,2)", "b", NULL);
	test("'abせd'.substring(-2)", "abせd", NULL);
	test("'abせd'.substring(undefined,-2)", "", NULL);
	test("'abせd'.substring(-2,-1)", "", NULL);
	test("'abせd'.substring(2,1)", "b", NULL);
	test("'abせd'.substring(2048)", "", NULL);
	test("'abせd'.substring(0,2048)", "abせd", NULL);
	test("'abせd'.substring(-2048)", "abせd", NULL);
	test("'abせd'.substring(0,-2048)", "", NULL);
	test("'abせd'.charAt()", "a", NULL);
	test("'abせd'.charAt(0)", "a", NULL);
	test("'abせd'.charAt(2)", "せ", NULL);
	test("'abせd'.charAt(3)", "d", NULL);
	test("'abせd'.charAt(-1)", "", NULL);
	test("'abせd'.charAt(2048)", "", NULL);
	test("'abせd'.charCodeAt()", "97", NULL);
	test("'abせd'.charCodeAt(0)", "97", NULL);
	test("'abせd'.charCodeAt(2)", "12379", NULL);
	test("'abせd'.charCodeAt(3)", "100", NULL);
	test("'abせd'.charCodeAt(-1)", "NaN", NULL);
	test("'abせd'.charCodeAt(2048)", "NaN", NULL);
	test("'abせd'.concat()", "abせd", NULL);
	test("'abせd'.concat(123, 'あ', null)", "abせd123あnull", NULL);
	test("'aべundefined'.indexOf()", "2", NULL);
	test("'aべcaべc'.indexOf()", "-1", NULL);
	test("'aべcaべc'.indexOf('c')", "2", NULL);
	test("'aべcaべc'.indexOf('aべ')", "0", NULL);
	test("'aべcaべc'.indexOf('べc')", "1", NULL);
	test("'aべcaべc'.indexOf('c')", "2", NULL);
	test("'aべcaべc'.indexOf('c', 2)", "2", NULL);
	test("'aべcaべc'.indexOf('c', 3)", "5", NULL);
	test("''.indexOf.length", "1", NULL);
	test("'aべundefined'.lastIndexOf()", "2", NULL);
	test("'aべcaべc'.lastIndexOf()", "-1", NULL);
	test("'aべcaべc'.lastIndexOf('c')", "5", NULL);
	test("'aべcaべc'.lastIndexOf('aべ')", "3", NULL);
	test("'aべcaべc'.lastIndexOf('べc')", "4", NULL);
	test("'aべcaべc'.lastIndexOf('c')", "5", NULL);
	test("'aべcaべc'.lastIndexOf('c', 2)", "2", NULL);
	test("'aべcaべc'.lastIndexOf('c', 3)", "2", NULL);
	test("''.lastIndexOf.length", "1", NULL);
	test("'123'[2]", "3", NULL);
	test("'123'[3]", "undefined", NULL);
	test("var a = '123'; a[1] = 5; a", "123", NULL);
	test("var s = new String('abc'); s[1]", "b", NULL);
}

static void testDate (void)
{
	test("Date", "function Date() [native code]", NULL);
	test("Object.prototype.toString.call(Date.prototype)", "[object Date]", NULL);
	test("Date.prototype.constructor", "function Date() [native code]", NULL);
	test("Date.prototype", "Invalid Date", NULL);
	test("(new Date(0)).toISOString()", "1970-01-01T00:00:00.000Z", NULL);
	test("(new Date(951782400000)).toISOString()", "2000-02-29T00:00:00.000Z", NULL);
	test("(new Date(951868800000)).toISOString()", "2000-03-01T00:00:00.000Z", NULL);
	test("(new Date(8640000000000000)).toISOString()", "+275760-09-13T00:00:00.000Z", NULL);
	test("(new Date(-8640000000000000)).toISOString()", "-271821-04-20T00:00:00.000Z", NULL);
	test("var x = new Date(0); x.valueOf() == Date.parse(x.toString())", "true", NULL);
	test("var x = new Date(0); x.valueOf() == Date.parse(x.toUTCString())", "true", NULL);
	test("var x = new Date(0); x.valueOf() == Date.parse(x.toISOString())", "true", NULL);
	test("var x = new Date(951782400000); x.valueOf() == Date.parse(x.toString())", "true", NULL);
	test("var x = new Date(951868800000); x.valueOf() == Date.parse(x.toUTCString())", "true", NULL);
	test("var x = new Date(8640000000000000); x.valueOf() == Date.parse(x.toISOString())", "true", NULL);
	test("new Date('1970-01-01').toISOString()", "1970-01-01T00:00:00.000Z", NULL);
	test("new Date('1970/01/01 12:34:56 +0900').toISOString()", "1970-01-01T03:34:56.000Z", NULL);
	test("new Date(1984, 07, 31, 01, 23, 45, 678).valueOf() - new Date().getTimezoneOffset() * 60000", "462763425678", NULL);
	test("Date.UTC(70, 00, 01)", "0", NULL);
	test("Date.UTC(70, 01, 01)", "2678400000", NULL);
	test("Date.UTC(1984, 07, 31, 01, 23, 45, 678)", "462763425678", NULL);
	test("(Date.parse('1984/08/31') - Date.parse('1984-08-31')) / 60000 == new Date().getTimezoneOffset()", "true", NULL);
	test("var date = new Date('1995/12/25 00:00:00'); date.getFullYear()", "1995", NULL);
	test("var date = new Date('1995/12/25 00:00:00'); date.getMonth()", "11", NULL);
	test("var date = new Date('1995/12/25 23:59:00'); date.getDate()", "25", NULL);
	test("var date = new Date('1995/12/25 00:00:00'); date.getDate()", "25", NULL);
	test("var date = new Date('1995/12/25 23:59:00'); date.getDay()", "1", NULL);
	test("var date = new Date('1995/12/24 00:00:00'); date.getDay()", "0", NULL);
	test("var date = new Date('1995/12/23 23:59:00'); date.getDay()", "6", NULL);
	test("var date = new Date('1995-12-25T12:34:56.789Z'); date.getMilliseconds()", "789", NULL);
	test("var date = new Date('1995-12-25T00:00:00Z'); date.getUTCFullYear()", "1995", NULL);
	test("var date = new Date('1995-12-25T00:00:00Z'); date.getUTCMonth()", "11", NULL);
	test("var date = new Date('1995-12-25T23:59:00Z'); date.getUTCDate()", "25", NULL);
	test("var date = new Date('1995-12-25T00:00:00Z'); date.getUTCDate()", "25", NULL);
	test("var date = new Date('1995-12-25T23:59:00Z'); date.getUTCDay()", "1", NULL);
	test("var date = new Date('1995-12-24T00:00:00Z'); date.getUTCDay()", "0", NULL);
	test("var date = new Date('1995-12-23T23:59:00Z'); date.getUTCDay()", "6", NULL);
	test("var date = new Date('1995-12-25T12:34:56.789Z'); date.getUTCMilliseconds()", "789", NULL);
	test("var date = new Date('1995-12-25T12:34:56.789Z'); date.setUTCMilliseconds(11)", "819894896011", NULL);
	test("var date = new Date('1995-12-25T12:34:56.789Z'); date.setUTCSeconds(11)", "819894851789", NULL);
	test("var date = new Date('1995-12-25T12:34:56.789Z'); date.setUTCSeconds(11, 22)", "819894851022", NULL);
	test("var date = new Date('1995-12-25T12:34:56.789Z'); date.setUTCMinutes(11)", "819893516789", NULL);
	test("var date = new Date('1995-12-25T12:34:56.789Z'); date.setUTCMinutes(11, 22)", "819893482789", NULL);
	test("var date = new Date('1995-12-25T12:34:56.789Z'); date.setUTCMinutes(11, 22, 33)", "819893482033", NULL);
	test("var date = new Date('1995-12-25T12:34:56.789Z'); date.setUTCHours(11)", "819891296789", NULL);
	test("var date = new Date('1995-12-25T12:34:56.789Z'); date.setUTCHours(11, 22)", "819890576789", NULL);
	test("var date = new Date('1995-12-25T12:34:56.789Z'); date.setUTCHours(11, 22, 33)", "819890553789", NULL);
	test("var date = new Date('1995-12-25T12:34:56.789Z'); date.setUTCHours(11, 22, 33, 444)", "819890553444", NULL);
	test("var date = new Date('1995-12-25T12:34:56.789Z'); date.setUTCDate(11)", "818685296789", NULL);
	test("var date = new Date('1995-12-25T12:34:56.789Z'); date.setUTCMonth(11)", "819894896789", NULL);
	test("var date = new Date('1995-12-25T12:34:56.789Z'); date.setUTCMonth(11, 22)", "819635696789", NULL);
	test("var date = new Date('1995-12-25T12:34:56.789Z'); date.setUTCFullYear(1)", "-62104620303211", NULL);
	test("var date = new Date('1995-12-25T12:34:56.789Z'); date.setUTCFullYear(1, 2)", "-62128380303211", NULL);
	test("var date = new Date('1995-12-25T12:34:56.789Z'); date.setUTCFullYear(1, 2, 3)", "-62130281103211", NULL);
	test("var date = new Date(NaN); date.setUTCFullYear(1, 2, 3)", "-62130326400000", NULL);
	test("var date = new Date('1995-12-25T12:34:56.789Z'); date.setUTCMilliseconds()", "NaN", NULL);
	test("var date = new Date('1995-12-25T12:34:56.789Z'); date.setUTCSeconds()", "NaN", NULL);
	test("var date = new Date('1995-12-25T12:34:56.789Z'); date.setUTCFullYear()", "NaN", NULL);
	test("var date = new Date('1995-12-25T12:34:56.789Z'); date.toISOString()", "1995-12-25T12:34:56.789Z", NULL);
	test("var date = new Date(); date.toISOString = undefined; date.toJSON()", "TypeError: toISOString is not a function"
	,    "                                                     ^~~~~~~~~~~~~");
	test("var o = {}; Date.prototype.toJSON.call(o)", "TypeError: toISOString is not a function"
	,    "            ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
	
	// iso format
	test("Date.parse('1984')", "441763200000", NULL);
	test("Date.parse('1984-08')", "460166400000", NULL);
	test("Date.parse('1984-08-31')", "462758400000", NULL);
	test("Date.parse('1984-08-31T01:23Z')", "462763380000", NULL);
	test("Date.parse('1984-08-31T01:23:45Z')", "462763425000", NULL);
	test("Date.parse('1984-08-31T01:23:45.678Z')", "462763425678", NULL);
	test("Date.parse('1984-08-31T01:23+12:34')", "462718140000", NULL);
	test("Date.parse('1984-08-31T01:23:45+12:34')", "462718185000", NULL);
	test("Date.parse('1984-08-31T01:23:45.678+12:34')", "462718185678", NULL);
	
	// iso format with time and no offset is not supported: ES5 & ES6 are contradictory and hence not portable
	test("Date.parse('1984-08-31T01:23')", "NaN", NULL);
	test("Date.parse('1984-08-31T01:23:45')", "NaN", NULL);
	test("Date.parse('1984-08-31T01:23:45.678')", "NaN", NULL);
	
	// iso format only support '+hh:mm' time offset
	test("Date.parse('1984-08-31T01:23+1234')", "NaN", NULL);
	test("Date.parse('1984-08-31T01:23 +12:34')", "NaN", NULL);
	
	// implementation format
	test("Date.parse('1984/08/31 01:23 +0000')", "462763380000", NULL);
	test("Date.parse('1984/08/31 01:23:45 +0000')", "462763425000", NULL);
	test("Date.parse('100/08/31 01:23:45 +0000')", "-58990545375000", NULL);
	
	// implementation format years < 100 are not supported
	test("Date.parse('99/08/31 01:23:45 +0000')", "NaN", NULL);
	
	// implementation format only support ' +hhmm' time offset
	test("Date.parse('1984/08')", "NaN", NULL);
	test("Date.parse('1984/08/31 01:23:45+0000')", "NaN", NULL);
	test("Date.parse('1984/08/31 01:23:45 +00:00')", "NaN", NULL);
	test("Date.parse('1984/08/31 01:23:45  +0000')", "NaN", NULL);
}

static void testRegExp (void)
{
	test("RegExp", "function RegExp() [native code]", NULL);
	test("Object.prototype.toString.call(RegExp.prototype)", "[object RegExp]", NULL);
	test("RegExp.prototype.constructor", "function RegExp() [native code]", NULL);
	test("RegExp.prototype", "/(?:)/", NULL);
	test("/1/gg", "SyntaxError: invalid flags"
	,    "    ^");
	test("/(/", "SyntaxError: expect ')'"
	,    "  ^");
	test("/[/", "SyntaxError: expect ']'"
	,    "  ^");
	test("var r = /a/; r instanceof RegExp", "true", NULL);
	test("var r = /a/g; r.global", "true", NULL);
	test("var r = /a/i; r.ignoreCase", "true", NULL);
	test("var r = /a/m; r.multiline", "true", NULL);
	test("/a|ab/.exec('abc')", "a", NULL);
	test("/ab+c/.exec('abbbc')", "abbbc", NULL);
	test("/あべ+せ/.exec('あべべべせ')", "あべべべせ", NULL);
	test("/べ/.exec('あべせ')", "べ", NULL);
	test("/((a)|(ab))((c)|(bc))/.exec('abc')", "abc,a,a,,bc,,bc", NULL);
	test("/(aa|aabaac|ba|b|c)*/.exec('aabaac')", "aaba,ba", NULL);
	test("/(z)((a+)?(b+)?(c))*/.exec('zaacbbbcac')", "zaacbbbcac,z,ac,a,,c", NULL);
	test("/(a*)*/.exec('b')", ",", NULL);
	test("/(?=(a+))/i.exec('baaabac')", ",aaa", NULL);
	test("/[1二3]/i.exec('1')", "1", NULL);
	test("/[1二3]/i.exec('2')", "null", NULL);
	test("/[1二3]/i.exec('二')", "二", NULL);
	test("/([1二3])/.exec('3')", "3,3", NULL);
	test("/[^1二3]/i.exec('1')", "null", NULL);
	test("/[^1二3]/i.exec('2')", "2", NULL);
	test("/[^1二3]/i.exec('二')", "null", NULL);
	test("/([^1二3])/.exec('3')", "null", NULL);
}

static int runTest (int verbosity)
{
	Function.addValue(ecc->global, "global", Value.object(&ecc->global->environment), 0);
	
	testVerbosity = verbosity;
	
//	test("debugger", "undefined", NULL);
	
	testLexer();
	testParser();
	testEval();
	testConvertion();
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
	testError();
	testAccessor();
	testArray();
	testBoolean();
	testNumber();
	testString();
	testDate();
	testRegExp();
	
	Env.newline();
	
	if (testErrorCount)
		Env.printColor(0, Env(bold), "test failure: %d", testErrorCount);
	else
		Env.printColor(0, Env(bold), "all success");
	
	Env.newline();
	Env.newline();
	Env.print("%d tests, %.2f ms", testCount, testTime * 1000);
	
	return testErrorCount? EXIT_FAILURE: EXIT_SUCCESS;
}

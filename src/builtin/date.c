//
//  date.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#if _WIN32
#include <sys/timeb.h>
#elif _DEFAULT_SOURCE || __APPLE__
#include <sys/time.h>
#endif

#define Implementation
#include "date.h"

#include "../ecc.h"
#include "../pool.h"

// MARK: - Private

struct Object * Date(prototype) = NULL;
struct Function * Date(constructor) = NULL;

const struct Object(Type) Date(type) = {
	.text = &Text(dateType),
};

static double localOffset;

static const double msPerSecond = 1000;
static const double msPerMinute = 60000;
static const double msPerHour = 3600000;
static const double msPerDay = 86400000;

static int issign(int c)
{
	return c == '+' || c == '-';
}

static void setupLocalOffset (void)
{
	struct tm tm = {
		.tm_mday = 2,
		.tm_year = 70,
		.tm_wday = 5,
		.tm_isdst = -1,
	};
	time_t time = mktime(&tm);
	if (time < 0)
		localOffset = -0.;
	else
		localOffset = difftime(86400, time) / 3600.;
}

static double msClip (double ms)
{
	if (!isfinite(ms) || fabs(ms) > 8.64 * pow(10, 15))
		return NAN;
	else
		return ms;
}

static double msFromDate (int32_t year, int32_t month, int32_t day)
{
	// Low-Level Date Algorithms, http://howardhinnant.github.io/date_algorithms.html#days_from_civil
	
	int32_t era;
	uint32_t yoe, doy, doe;
	
	year -= month <= 2;
    era = (year >= 0 ? year : year - 399) / 400;
    yoe = year - era * 400;                                       // [0, 399]
    doy = (153 * (month + (month > 2? -3: 9)) + 2) / 5 + day - 1; // [0, 365]
    doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;                  // [0, 146096]
	
    return (double)(era * 146097 + (int32_t)(doe) - 719468) * msPerDay;
}

static double msFromArguments (struct Context * const context)
{
	double time;
	uint16_t count;
	double year, month, day, h, m, s, ms;
	
	Context.assertVariableParameter(context);
	
	count = Context.variableArgumentCount(context);
	
	year = Value.toBinary(Context.variableArgument(context, 0)).data.binary,
	month = Value.toBinary(Context.variableArgument(context, 1)).data.binary,
	day = count > 2? Value.toBinary(Context.variableArgument(context, 2)).data.binary: 1,
	h = count > 3? Value.toBinary(Context.variableArgument(context, 3)).data.binary: 0,
	m = count > 4? Value.toBinary(Context.variableArgument(context, 4)).data.binary: 0,
	s = count > 5? Value.toBinary(Context.variableArgument(context, 5)).data.binary: 0,
	ms = count > 6? Value.toBinary(Context.variableArgument(context, 6)).data.binary: 0;
	
	if (isnan(year) || isnan(month) || isnan(day) || isnan(h) || isnan(m) || isnan(s) || isnan(ms))
		time = NAN;
	else
	{
		if (year >= 0 && year <= 99)
			year += 1900;
		
		time =
			msFromDate(year, month + 1, day)
			+ h * msPerHour
			+ m * msPerMinute
			+ s * msPerSecond
			+ ms
			;
	}
	
	return time;
}

static double msFromBytes (const char *bytes, uint16_t length)
{
	char buffer[length + 1];
	int n = 0, nOffset = 0, i = 0;
	int year, month = 1, day = 1, h = 0, m = 0, s = 0, ms = 0, hOffset = 0, mOffset = 0;
	
	if (!length)
		return NAN;
	
	memcpy(buffer, bytes, length);
	buffer[length] = '\0';
	
	n = 0, i = sscanf(buffer, issign(buffer[0])
	                          ? "%07d""%n""-%02d""%n""-%02d%n""T%02d:%02d%n"":%02d%n"".%03d%n"
	                          : "%04d""%n""-%02d""%n""-%02d%n""T%02d:%02d%n"":%02d%n"".%03d%n",
	                             &year,&n, &month,&n, &day,&n, &h,  &m,  &n, &s,  &n, &ms, &n);
	
	if (i <= 3)
	{
		if (n == length)
			goto done;
		
		/*vvv*/
	}
	else if (i >= 5)
	{
		if (buffer[n] == 'Z' && n + 1 == length)
			goto done;
		else if (issign(buffer[n]) && sscanf(buffer + n, "%03d:%02d%n", &hOffset, &mOffset, &nOffset) == 2 && n + nOffset == length)
			goto done;
		else
			return NAN;
	}
	else
		return NAN;
	
	hOffset = localOffset;
	n = 0, i = sscanf(buffer, "%d"   "/%02d" "/%02d%n"" %02d:%02d%n"":%02d%n",
	                           &year, &month, &day,&n,  &h,  &m, &n, &s,  &n);
	
	if (year < 100)
		return NAN;
	
	if (n == length)
		goto done;
	else if (i >= 5 && buffer[n++] == ' ' && issign(buffer[n]) && sscanf(buffer + n, "%03d%02d%n", &hOffset, &mOffset, &nOffset) == 2 && n + nOffset == length)
		goto done;
	else
		return NAN;
	
done:
	if (month <= 0 || day <= 0 || h < 0 || m < 0 || s < 0 || ms < 0 || hOffset < -12 || mOffset < 0)
		return NAN;
	
	if (month > 12 || day > 31 || h > 23 || m > 59 || s > 59 || ms > 999 || hOffset > 14 || mOffset > 59)
		return NAN;
	
	return
		msFromDate(year, month, day)
		+ h * msPerHour
		+ m * msPerMinute
		+ s * msPerSecond
		+ ms
		- (hOffset * 60 + mOffset) * msPerMinute
		;
}

static double msToDate (double ms, int32_t *year, int32_t *month, int32_t *day)
{
	// Low-Level Date Algorithms, http://howardhinnant.github.io/date_algorithms.html#civil_from_days
	
	int32_t z, era;
	uint32_t doe, yoe, doy, mp;
	
	z = ms / msPerDay + 719468;
	era = (z >= 0 ? z : z - 146096) / 146097;
    doe = (z - era * 146097);                                     // [0, 146096]
    yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;  // [0, 399]
    doy = doe - (365 * yoe + yoe / 4 - yoe / 100);                // [0, 365]
    mp = (5 * doy + 2) / 153;                                     // [0, 11]
    *day = doy - (153 * mp + 2) / 5 + 1;                          // [1, 31]
    *month = mp + (mp < 10? 3: -9);                               // [1, 12]
    *year = (int32_t)(yoe) + era * 400 + (mp >= 10);
	
	return fmod(ms, 24 * msPerHour) + (ms < 0? 24 * msPerHour: 0);
}

static struct Chars *msToChars (double ms, double offset)
{
	int32_t year, month, day;
	const char *format;
	
	if (isnan(ms))
		return Chars.create("Invalid Date");
	
	ms = msToDate(msClip(ms + offset * msPerHour), &year, &month, &day);
	
	if (year >= 100 && year <= 9999)
		format = "%04d/%02d/%02d %02d:%02d:%02d %+03d%02d";
	else
		format = "%+06d-%02d-%02dT%02d:%02d:%02d%+03d:%02d";
	
	return Chars.create(format
		, year
		, month
		, day
		, (int)fmod(floor(ms / msPerHour), 24)
		, (int)fmod(floor(ms / msPerMinute), 60)
		, (int)fmod(floor(ms / msPerSecond), 60)
		, (int)offset
		, (int)fmod(fabs(offset) * 60, 60)
		);
}

static double currentTime ()
{
#if _WIN32
	struct _timeb timebuffer;
	_ftime (&timebuffer);
	return timebuffer.time * 1000 + timebuffer.millitm;
#elif _DEFAULT_SOURCE || __APPLE__
	struct timeval time;
	gettimeofday(&time, NULL);
	return time.tv_sec * 1000 + time.tv_usec / 1000;
#else
	return time(NULL) * 1000;
#endif
}

//

static struct Value toString (struct Context * const context)
{
	Context.assertParameterCount(context, 0);
	Context.assertThisType(context, Value(dateType));
	
	return Value.chars(msToChars(context->this.data.date->ms, localOffset));
}

static struct Value toUTCString (struct Context * const context)
{
	Context.assertParameterCount(context, 0);
	Context.assertThisType(context, Value(dateType));
	
	return Value.chars(msToChars(context->this.data.date->ms, 0));
}

static struct Value toISOString (struct Context * const context)
{
	const char *format;
	double ms;
	int32_t year, month, day;
	
	Context.assertParameterCount(context, 0);
	Context.assertThisType(context, Value(dateType));
	
	if (isnan(context->this.data.date->ms))
		Ecc.jmpEnv(context->ecc, Value.error(Error.rangeError(Context.textSeek(context), "invalid date")));
	
	ms = msToDate(context->this.data.date->ms, &year, &month, &day);
	
	if (year >= 0 && year <= 9999)
		format = "%04d-%02d-%02dT%02d:%02d:%06.3fZ";
	else
		format = "%+06d-%02d-%02dT%02d:%02d:%06.3fZ";
	
	return Value.chars(Chars.create(format
		, year
		, month
		, day
		, (int)fmod(floor(ms / msPerHour), 24)
		, (int)fmod(floor(ms / msPerMinute), 60)
		, fmod(floor(ms / msPerSecond), 60)
		));
}

static struct Value toDateString (struct Context * const context)
{
	int32_t year, month, day;
	
	Context.assertParameterCount(context, 0);
	Context.assertThisType(context, Value(dateType));
	
	if (isnan(context->this.data.date->ms))
		return Value.chars(Chars.create("Invalid Date"));
	
	msToDate(context->this.data.date->ms + localOffset * msPerHour, &year, &month, &day);
	
	return Value.chars(Chars.create("%04d/%02d/%02d"
		, year
		, month
		, day
		));
}

static struct Value toTimeString (struct Context * const context)
{
	double ms;
	int32_t year, month, day;
	
	Context.assertParameterCount(context, 0);
	Context.assertThisType(context, Value(dateType));
	
	if (isnan(context->this.data.date->ms))
		return Value.chars(Chars.create("Invalid Date"));
	
	ms = msToDate(context->this.data.date->ms + localOffset * msPerHour, &year, &month, &day);
	
	return Value.chars(Chars.create("%02d:%02d:%02d %+03d%02d"
		, (int)fmod(floor(ms / msPerHour), 24)
		, (int)fmod(floor(ms / msPerMinute), 60)
		, (int)fmod(floor(ms / msPerSecond), 60)
		, (int)localOffset
		, (int)fmod(fabs(localOffset) * 60, 60)
		));
}

static struct Value valueOf (struct Context * const context)
{
	Context.assertParameterCount(context, 0);
	Context.assertThisType(context, Value(dateType));
	
	return Value.binary(context->this.data.date->ms);
}

static struct Value getTimezoneOffset (struct Context * const context)
{
	Context.assertParameterCount(context, 0);
	
	return Value.binary(-localOffset * 60);
}

static struct Value now (struct Context * const context)
{
	Context.assertParameterCount(context, 0);
	
	return Value.binary(msClip(currentTime()));
}

static struct Value parse (struct Context * const context)
{
	struct Value value;
	
	Context.assertParameterCount(context, 1);
	
	value = Value.toString(context, Context.argument(context, 0));
	
	return Value.binary(msClip(msFromBytes(Value.stringBytes(value), Value.stringLength(value))));
}

static struct Value UTC (struct Context * const context)
{
	Context.assertVariableParameter(context);
	
	if (Context.variableArgumentCount(context) > 1)
		return Value.binary(msClip(msFromArguments(context)));
	
	return Value.binary(NAN);
}

static struct Value dateConstructor (struct Context * const context)
{
	double time;
	uint16_t count;
	
	Context.assertVariableParameter(context);
	
	if (!context->construct)
		return Value.chars(msToChars(currentTime(), localOffset));
	
	count = Context.variableArgumentCount(context);
	
	if (count > 1)
		time = msFromArguments(context) - localOffset * msPerHour;
	else if (count)
	{
		struct Value value = Value.toPrimitive(context, Context.variableArgument(context, 0), Value(hintAuto));
		
		if (Value.isString(value))
			time = msFromBytes(Value.stringBytes(value), Value.stringLength(value));
		else
			time = Value.toBinary(value).data.binary;
	}
	else
		time = currentTime();
	
	return Value.date(create(time));
}

// MARK: - Static Members

// MARK: - Methods

void setup (void)
{
	const enum Value(Flags) flags = Value(hidden);
	
	Function.setupBuiltinObject(&Date(constructor), dateConstructor, -7, &Date(prototype), Value.date(create(NAN)), &Date(type));
	
	Function.addToObject(Date(prototype), "toString", toString, 0, flags);
	Function.addToObject(Date(prototype), "toISOString", toISOString, 0, flags);
	Function.addToObject(Date(prototype), "toUTCString", toUTCString, 0, flags);
	Function.addToObject(Date(prototype), "toDateString", toDateString, 0, flags);
	Function.addToObject(Date(prototype), "toTimeString", toTimeString, 0, flags);
	Function.addToObject(Date(prototype), "valueOf", valueOf, 0, flags);
	Function.addToObject(Date(prototype), "getTimezoneOffset", getTimezoneOffset, 0, flags);
	
	
	Function.addToObject(&Date(constructor)->object, "now", now, 0, flags);
	Function.addToObject(&Date(constructor)->object, "parse", parse, 1, flags);
	Function.addToObject(&Date(constructor)->object, "UTC", UTC, -7, flags);
	
	setupLocalOffset();
}

void teardown (void)
{
	Date(prototype) = NULL;
	Date(constructor) = NULL;
}

struct Date *create (double ms)
{
	struct Date *self = malloc(sizeof(*self));
	*self = Date.identity;
	Pool.addObject(&self->object);
	Object.initialize(&self->object, Date(prototype));
	
	self->ms = msClip(ms);
	
	return self;
}

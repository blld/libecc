//
//  date.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#if __MSDOS__ || _WIN32 || _WIN64
#else
#include <sys/time.h>
#endif

#include "date.h"

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

static double msFromArguments (struct Native(Context) * const context)
{
	double time;
	uint16_t count;
	
	Native.assertVariableParameter(context);
	
	count = Native.variableArgumentCount(context);
	
	double
		year = Value.toBinary(Native.variableArgument(context, 0)).data.binary,
		month = Value.toBinary(Native.variableArgument(context, 1)).data.binary,
		day = count > 2? Value.toBinary(Native.variableArgument(context, 2)).data.binary: 1,
		h = count > 3? Value.toBinary(Native.variableArgument(context, 3)).data.binary: 0,
		m = count > 4? Value.toBinary(Native.variableArgument(context, 4)).data.binary: 0,
		s = count > 5? Value.toBinary(Native.variableArgument(context, 5)).data.binary: 0,
		ms = count > 6? Value.toBinary(Native.variableArgument(context, 6)).data.binary: 0;
	
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
	int iso = 1, s1 = '-', s2 = 'T';
	
	const char *end = bytes + length;
	int32_t year, month = 1, day = 1, h = 0, m = 0, s = 0, ms = 0, offsetHour = 0, offsetMinute = 0;
	
	if (bytes + 7 <= end && (*bytes == '+' || *bytes == '-') && sscanf(bytes,"%07d", &year) == 1)
		bytes += 7;
	else if (bytes + 4 <= end && isdigit(*bytes) && sscanf(bytes,"%04d", &year) == 1)
		bytes += 4;
	else
		goto error;
	
	if (bytes == end)
		goto done;
	else if (*bytes == '/')
	{
		iso = 0;
		s1 = '/';
		s2 = ' ';
		offsetHour = localOffset;
	}
	
	if (bytes + 3 <= end && *bytes == s1 && sscanf(bytes + 1, "%02d", &month) == 1)
		bytes += 3;
	else
		goto error;
	
	if (bytes == end)
		goto done;
	else if (bytes + 3 <= end && *bytes == s1 && sscanf(bytes + 1,"%02d", &day) == 1)
		bytes += 3;
	else
		goto error;
	
	if (bytes == end)
		goto done;
	else if (bytes + 6 <= end && *bytes == s2 && sscanf(bytes + 1,"%02d:%02d", &h, &m) == 2)
		bytes += 6;
	else
		goto error;
	
	if (!iso && bytes == end)
		goto done;
	else if (bytes + 3 <= end && sscanf(bytes,":%02d", &s) == 1)
	{
		bytes += 3;
		
		if (bytes == end)
			goto done;
		else if (iso && bytes + 4 <= end && sscanf(bytes,".%03d", &ms) == 1)
			bytes += 4;
	}
	
	if (iso)
	{
		if (bytes + 1 <= end && *bytes == 'Z')
			bytes += 1;
		else if (bytes + 6 <= end && (*bytes == '+' || *bytes == '-') && sscanf(bytes,"%03d:%02d", &offsetHour, &offsetMinute) == 2)
			bytes += 6;
		
		if (bytes == end)
			goto done;
		else
			goto error;
	}
	else
	{
		if (bytes == end)
			goto done;
		else if (bytes + 1 <= end && *bytes == ' ')
			bytes += 1;
		else
			goto error;
		
		if (bytes + 5 <= end && (*bytes == '+' || *bytes == '-') && sscanf(bytes,"%03d%02d", &offsetHour, &offsetMinute) == 2)
			bytes += 5;
		else
			goto error;
	}
	
done:
	return
		msFromDate(year, month, day)
		+ h * msPerHour
		+ m * msPerMinute
		+ s * msPerSecond
		+ ms / 1000.
		- (offsetHour * 60 + offsetMinute) * msPerMinute
		;
	
error:
	return NAN;
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
	
	if (isnan(ms))
		return Chars.create("Invalid Date");
	
	ms = msToDate(ms + offset * msPerHour, &year, &month, &day);
	
	return Chars.create("%04d/%02d/%02d %02d:%02d:%02d %+03d%02d"
		, year
		, month
		, day
		, (int)fmod(floor(ms / msPerHour), 24)
		, (int)fmod(floor(ms / msPerMinute), 60)
		, (int)fmod(floor(ms / msPerSecond), 60)
		, (int)offset
		, abs(fmod(offset * 60, 60))
		);
}

static double currentTime ()
{
#if __MSDOS__ || _WIN32 || _WIN64
#else
	struct timeval time;
	gettimeofday(&time, NULL);
	return time.tv_sec * 1000 + time.tv_usec / 1000;
#endif
}

//

static struct Value toString (struct Native(Context) * const context)
{
	Native.assertParameterCount(context, 0);
	
	if (context->this.type != Value(dateType))
		Ecc.jmpEnv(context->ecc, Value.error(Error.typeError(Native.textSeek(context, Native(thisIndex)), "not a date")));
	
	return Value.chars(msToChars(context->this.data.date->ms, localOffset));
}

static struct Value toUTCString (struct Native(Context) * const context)
{
	Native.assertParameterCount(context, 0);
	
	if (context->this.type != Value(dateType))
		Ecc.jmpEnv(context->ecc, Value.error(Error.typeError(Native.textSeek(context, Native(thisIndex)), "not a date")));
	
	return Value.chars(msToChars(context->this.data.date->ms, 0));
}

static struct Value toISOString (struct Native(Context) * const context)
{
	const char *format;
	double ms;
	int32_t year, month, day;
	
	Native.assertParameterCount(context, 0);
	
	if (context->this.type != Value(dateType))
		Ecc.jmpEnv(context->ecc, Value.error(Error.typeError(Native.textSeek(context, Native(thisIndex)), "not a date")));
	
	if (isnan(context->this.data.date->ms))
		Ecc.jmpEnv(context->ecc, Value.error(Error.rangeError(Native.textSeek(context, Native(thisIndex)), "invalid date")));
	
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

static struct Value toDateString (struct Native(Context) * const context)
{
	int32_t year, month, day;
	
	Native.assertParameterCount(context, 0);
	
	if (context->this.type != Value(dateType))
		Ecc.jmpEnv(context->ecc, Value.error(Error.typeError(Native.textSeek(context, Native(thisIndex)), "not a date")));
	
	if (isnan(context->this.data.date->ms))
		return Value.chars(Chars.create("Invalid Date"));
	
	msToDate(context->this.data.date->ms + localOffset * msPerHour, &year, &month, &day);
	
	return Value.chars(Chars.create("%04d/%02d/%02d"
		, year
		, month
		, day
		));
}

static struct Value toTimeString (struct Native(Context) * const context)
{
	double ms;
	int32_t year, month, day;
	
	Native.assertParameterCount(context, 0);
	
	if (context->this.type != Value(dateType))
		Ecc.jmpEnv(context->ecc, Value.error(Error.typeError(Native.textSeek(context, Native(thisIndex)), "not a date")));
	
	if (isnan(context->this.data.date->ms))
		return Value.chars(Chars.create("Invalid Date"));
	
	ms = msToDate(context->this.data.date->ms + localOffset * msPerHour, &year, &month, &day);
	
	return Value.chars(Chars.create("%02d:%02d:%02d %+03d%02d"
		, (int)fmod(floor(ms / msPerHour), 24)
		, (int)fmod(floor(ms / msPerMinute), 60)
		, (int)fmod(floor(ms / msPerSecond), 60)
		, (int)localOffset
		, abs(fmod(localOffset * 60, 60))
		));
}

static struct Value valueOf (struct Native(Context) * const context)
{
	Native.assertParameterCount(context, 0);
	
	if (context->this.type != Value(dateType))
		Ecc.jmpEnv(context->ecc, Value.error(Error.typeError(Native.textSeek(context, Native(thisIndex)), "not a date")));
	
	return Value.binary(context->this.data.date->ms);
}

static struct Value now (struct Native(Context) * const context)
{
	Native.assertParameterCount(context, 0);
	
	return Value.binary(msClip(currentTime()));
}

static struct Value parse (struct Native(Context) * const context)
{
	struct Value value;
	
	Native.assertParameterCount(context, 1);
	
	value = Value.toString(context, Native.argument(context, 0));
	
	return Value.binary(msClip(msFromBytes(Value.stringBytes(value), Value.stringLength(value))));
}

static struct Value UTC (struct Native(Context) * const context)
{
	Native.assertVariableParameter(context);
	
	if (Native.variableArgumentCount(context) > 1)
		return Value.binary(msClip(msFromArguments(context)));
	
	return Value.binary(NAN);
}

static struct Value dateConstructor (struct Native(Context) * const context)
{
	double time;
	uint16_t count;
	
	Native.assertVariableParameter(context);
	
	if (!context->construct)
		return Value.chars(msToChars(currentTime(), localOffset));
	
	count = Native.variableArgumentCount(context);
	
	if (count > 1)
		time = msFromArguments(context) - localOffset * msPerHour;
	else if (count)
	{
		struct Value value = Value.toPrimitive(context, Native.variableArgument(context, 0), &Text(empty), Value(hintAuto));
		
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
	
	Function.addToObject(&Date(constructor)->object, "now", now, 0, flags);
	Function.addToObject(&Date(constructor)->object, "parse", parse, 1, flags);
	Function.addToObject(&Date(constructor)->object, "UTC", UTC, -7, flags);
	
	//
	
	struct tm tm = {
		.tm_mday = 1,
		.tm_year = 70,
	};
	time_t time = mktime(&tm);
	localOffset = difftime(0, time) / 3600;
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

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

struct date {
	int32_t year;
	int32_t month;
	int32_t day;
};

struct time {
	int32_t h;
	int32_t m;
	int32_t s;
	int32_t ms;
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

static double toLocal (double ms)
{
	return ms + localOffset * msPerHour;
}

static double toUTC (double ms)
{
	return ms - localOffset * msPerHour;
}

static double binaryArgumentOr (struct Context * const context, int index, double alternative)
{
	struct Value value = Context.argument(context, index);
	if (value.check == 1)
		return Value.toBinary(Context.argument(context, index)).data.binary;
	else
		return alternative;
}

static double msClip (double ms)
{
	if (!isfinite(ms) || fabs(ms) > 8.64 * pow(10, 15))
		return NAN;
	else
		return ms;
}

static double msFromDate (struct date date)
{
	// Low-Level Date Algorithms, http://howardhinnant.github.io/date_algorithms.html#days_from_civil
	
	int32_t era;
	uint32_t yoe, doy, doe;
	
	date.year -= date.month <= 2;
    era = (int32_t)((date.year >= 0 ? date.year : date.year - 399) / 400);
    yoe = (uint32_t)(date.year - era * 400);                                     // [0, 399]
    doy = (153 * (date.month + (date.month > 2? -3: 9)) + 2) / 5 + date.day - 1; // [0, 365]
    doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;                                 // [0, 146096]
	
    return (double)(era * 146097 + (int32_t)(doe) - 719468) * msPerDay;
}

static double msFromDateAndTime (struct date date, struct time time)
{
	return msFromDate(date)
		+ time.h * msPerHour
		+ time.m * msPerMinute
		+ time.s * msPerSecond
		+ time.ms
		;
}

static double msFromArguments (struct Context * const context)
{
	uint16_t count;
	struct date date;
	struct time time;
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
		return NAN;
	else
	{
		if (year >= 0 && year <= 99)
			year += 1900;
		
		date.year = year;
		date.month = month + 1;
		date.day = day;
		time.h = h;
		time.m = m;
		time.s = s;
		time.ms = ms;
		return msFromDateAndTime(date, time);
	}
}

static double msFromBytes (const char *bytes, uint16_t length)
{
	char buffer[length + 1];
	int n = 0, nOffset = 0, i = 0;
	int year, month = 1, day = 1, h = 0, m = 0, s = 0, ms = 0, hOffset = 0, mOffset = 0;
	struct date date;
	struct time time;
	
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
	
	date.year = year;
	date.month = month;
	date.day = day;
	time.h = h;
	time.m = m;
	time.s = s;
	time.ms = ms;
	return msFromDateAndTime(date, time) - (hOffset * 60 + mOffset) * msPerMinute;
}

static double msToDate (double ms, struct date *date)
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
    date->day = doy - (153 * mp + 2) / 5 + 1;                     // [1, 31]
    date->month = mp + (mp < 10? 3: -9);                          // [1, 12]
    date->year = (int32_t)(yoe) + era * 400 + (mp >= 10);
	
	return fmod(ms, 24 * msPerHour) + (ms < 0? 24 * msPerHour: 0);
}

static void msToTime (double ms, struct time *time)
{
	time->h = fmod(floor(ms / msPerHour), 24);
	time->m = fmod(floor(ms / msPerMinute), 60);
	time->s = fmod(floor(ms / msPerSecond), 60);
	time->ms = fmod(ms, 1000);
}

static void msToDateAndTime (double ms, struct date *date, struct time *time)
{
	msToTime(msToDate(ms, date), time);
}

static double msToHours (double ms)
{
	struct date date;
	struct time time;
	msToDateAndTime(ms, &date, &time);
	return time.h;
}

static double msToMinutes (double ms)
{
	struct date date;
	struct time time;
	msToDateAndTime(ms, &date, &time);
	return time.m;
}

static double msToSeconds (double ms)
{
	struct date date;
	struct time time;
	msToDateAndTime(ms, &date, &time);
	return time.s;
}

static double msToMilliseconds (double ms)
{
	struct date date;
	struct time time;
	msToDateAndTime(ms, &date, &time);
	return time.ms;
}

static struct Chars *msToChars (double ms, double offset)
{
	const char *format;
	struct date date;
	struct time time;
	
	if (isnan(ms))
		return Chars.create("Invalid Date");
	
	msToDateAndTime(ms + offset * msPerHour, &date, &time);
	
	if (date.year >= 100 && date.year <= 9999)
		format = "%04d/%02d/%02d %02d:%02d:%02d %+03d%02d";
	else
		format = "%+06d-%02d-%02dT%02d:%02d:%02d%+03d:%02d";
	
	return Chars.create(format
		, date.year
		, date.month
		, date.day
		, time.h
		, time.m
		, time.s
		, (int)offset
		, (int)fmod(fabs(offset) * 60, 60)
		);
}

unsigned int msToWeekday(double ms)
{
	int z = ms / msPerDay;
    return (unsigned)(z >= -4? (z + 4) % 7: (z + 5) % 7 + 6);
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

static struct Value toJSON (struct Context * const context)
{
	struct Value object = Value.toObject(context, Context.this(context));
	struct Value tv = Value.toPrimitive(context, object, Value(hintNumber));
	struct Value *toISO;
	
	Context.assertParameterCount(context, 1);
	
	if (tv.type == Value(binaryType) && !isfinite(tv.data.binary))
		return Value(null);
	
	toISO = Object.member(object.data.object, Key(toISOString));
	if (!toISO || toISO->type != Value(functionType))
		Context.typeError(context, Chars.create("toISOString is not a function"));
	
	return Context.callFunction(context, toISO->data.function, object, 0);
}

static struct Value toISOString (struct Context * const context)
{
	const char *format;
	struct date date;
	struct time time;
	
	Context.assertParameterCount(context, 0);
	Context.assertThisType(context, Value(dateType));
	
	if (isnan(context->this.data.date->ms))
		Context.rangeError(context, Chars.create("invalid date"));
	
	msToDateAndTime(context->this.data.date->ms, &date, &time);
	
	if (date.year >= 0 && date.year <= 9999)
		format = "%04d-%02d-%02dT%02d:%02d:%06.3fZ";
	else
		format = "%+06d-%02d-%02dT%02d:%02d:%06.3fZ";
	
	return Value.chars(Chars.create(format
		, date.year
		, date.month
		, date.day
		, time.h
		, time.m
		, time.s + (time.ms / 1000.)
		));
}

static struct Value toDateString (struct Context * const context)
{
	struct date date;
	
	Context.assertParameterCount(context, 0);
	Context.assertThisType(context, Value(dateType));
	
	if (isnan(context->this.data.date->ms))
		return Value.chars(Chars.create("Invalid Date"));
	
	msToDate(toLocal(context->this.data.date->ms), &date);
	
	return Value.chars(Chars.create("%04d/%02d/%02d"
		, date.year
		, date.month
		, date.day
		));
}

static struct Value toTimeString (struct Context * const context)
{
	struct time time;
	
	Context.assertParameterCount(context, 0);
	Context.assertThisType(context, Value(dateType));
	
	if (isnan(context->this.data.date->ms))
		return Value.chars(Chars.create("Invalid Date"));
	
	msToTime(toLocal(context->this.data.date->ms), &time);
	
	return Value.chars(Chars.create("%02d:%02d:%02d %+03d%02d"
		, time.h
		, time.m
		, time.s
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

static struct Value getFullYear (struct Context * const context)
{
	struct date date;
	
	Context.assertParameterCount(context, 0);
	Context.assertThisType(context, Value(dateType));
	
	msToDate(toLocal(context->this.data.date->ms), &date);
	return Value.binary(date.year);
}

static struct Value getUTCFullYear (struct Context * const context)
{
	struct date date;
	
	Context.assertParameterCount(context, 0);
	Context.assertThisType(context, Value(dateType));
	
	msToDate(context->this.data.date->ms, &date);
	return Value.binary(date.year);
}

static struct Value getMonth (struct Context * const context)
{
	struct date date;
	
	Context.assertParameterCount(context, 0);
	Context.assertThisType(context, Value(dateType));
	
	msToDate(toLocal(context->this.data.date->ms), &date);
	return Value.binary(date.month - 1);
}

static struct Value getUTCMonth (struct Context * const context)
{
	struct date date;
	
	Context.assertParameterCount(context, 0);
	Context.assertThisType(context, Value(dateType));
	
	msToDate(context->this.data.date->ms, &date);
	return Value.binary(date.month - 1);
}

static struct Value getDate (struct Context * const context)
{
	struct date date;
	
	Context.assertParameterCount(context, 0);
	Context.assertThisType(context, Value(dateType));
	
	msToDate(toLocal(context->this.data.date->ms), &date);
	return Value.binary(date.day);
}

static struct Value getUTCDate (struct Context * const context)
{
	struct date date;
	
	Context.assertParameterCount(context, 0);
	Context.assertThisType(context, Value(dateType));
	
	msToDate(context->this.data.date->ms, &date);
	return Value.binary(date.day);
}

static struct Value getDay (struct Context * const context)
{
	Context.assertParameterCount(context, 0);
	Context.assertThisType(context, Value(dateType));
	
	return Value.binary(msToWeekday(toLocal(context->this.data.date->ms)));
}

static struct Value getUTCDay (struct Context * const context)
{
	Context.assertParameterCount(context, 0);
	Context.assertThisType(context, Value(dateType));
	
	return Value.binary(msToWeekday(context->this.data.date->ms));
}

static struct Value getHours (struct Context * const context)
{
	Context.assertParameterCount(context, 0);
	Context.assertThisType(context, Value(dateType));
	
	return Value.binary(msToHours(toLocal(context->this.data.date->ms)));
}

static struct Value getUTCHours (struct Context * const context)
{
	Context.assertParameterCount(context, 0);
	Context.assertThisType(context, Value(dateType));
	
	return Value.binary(msToHours(context->this.data.date->ms));
}

static struct Value getMinutes (struct Context * const context)
{
	Context.assertParameterCount(context, 0);
	Context.assertThisType(context, Value(dateType));
	
	return Value.binary(msToMinutes(toLocal(context->this.data.date->ms)));
}

static struct Value getUTCMinutes (struct Context * const context)
{
	Context.assertParameterCount(context, 0);
	Context.assertThisType(context, Value(dateType));
	
	return Value.binary(msToMinutes(context->this.data.date->ms));
}

static struct Value getSeconds (struct Context * const context)
{
	Context.assertParameterCount(context, 0);
	Context.assertThisType(context, Value(dateType));
	
	return Value.binary(msToSeconds(toLocal(context->this.data.date->ms)));
}

static struct Value getUTCSeconds (struct Context * const context)
{
	Context.assertParameterCount(context, 0);
	Context.assertThisType(context, Value(dateType));
	
	return Value.binary(msToSeconds(context->this.data.date->ms));
}

static struct Value getMilliseconds (struct Context * const context)
{
	Context.assertParameterCount(context, 0);
	Context.assertThisType(context, Value(dateType));
	
	return Value.binary(msToMilliseconds(toLocal(context->this.data.date->ms)));
}

static struct Value getUTCMilliseconds (struct Context * const context)
{
	Context.assertParameterCount(context, 0);
	Context.assertThisType(context, Value(dateType));
	
	return Value.binary(msToMilliseconds(context->this.data.date->ms));
}

static struct Value getTimezoneOffset (struct Context * const context)
{
	Context.assertParameterCount(context, 0);
	
	return Value.binary(-localOffset * 60);
}

static struct Value setTime (struct Context * const context)
{
	double ms;
	
	Context.assertParameterCount(context, 1);
	Context.assertThisType(context, Value(dateType));
	
	ms = Value.toBinary(Context.argument(context, 0)).data.binary;
	
	return Value.binary(context->this.data.date->ms = msClip(ms));
}

static struct Value setMilliseconds (struct Context * const context)
{
	struct date date;
	struct time time;
	double ms;
	
	Context.assertParameterCount(context, 1);
	Context.assertThisType(context, Value(dateType));
	
	msToDateAndTime(toLocal(context->this.data.date->ms), &date, &time);
	ms = binaryArgumentOr(context, 0, NAN);
	if (isnan(ms))
		return Value.binary(context->this.data.date->ms = NAN);
	
	time.ms = ms;
	return Value.binary(context->this.data.date->ms = msClip(toUTC(msFromDateAndTime(date, time))));
}

static struct Value setUTCMilliseconds (struct Context * const context)
{
	struct date date;
	struct time time;
	double ms;
	
	Context.assertParameterCount(context, 1);
	Context.assertThisType(context, Value(dateType));
	
	msToDateAndTime(context->this.data.date->ms, &date, &time);
	ms = binaryArgumentOr(context, 0, NAN);
	if (isnan(ms))
		return Value.binary(context->this.data.date->ms = NAN);
	
	time.ms = ms;
	return Value.binary(context->this.data.date->ms = msClip(msFromDateAndTime(date, time)));
}

static struct Value setSeconds (struct Context * const context)
{
	struct date date;
	struct time time;
	double s, ms;
	
	Context.assertParameterCount(context, 2);
	Context.assertThisType(context, Value(dateType));
	
	msToDateAndTime(toLocal(context->this.data.date->ms), &date, &time);
	s = binaryArgumentOr(context, 0, NAN);
	ms = binaryArgumentOr(context, 1, time.ms);
	if (isnan(s) || isnan(ms))
		return Value.binary(context->this.data.date->ms = NAN);
	
	time.s = s;
	time.ms = ms;
	return Value.binary(context->this.data.date->ms = msClip(toUTC(msFromDateAndTime(date, time))));
}

static struct Value setUTCSeconds (struct Context * const context)
{
	struct date date;
	struct time time;
	double s, ms;
	
	Context.assertParameterCount(context, 2);
	Context.assertThisType(context, Value(dateType));
	
	msToDateAndTime(context->this.data.date->ms, &date, &time);
	s = binaryArgumentOr(context, 0, NAN);
	ms = binaryArgumentOr(context, 1, time.ms);
	if (isnan(s) || isnan(ms))
		return Value.binary(context->this.data.date->ms = NAN);
	
	time.s = s;
	time.ms = ms;
	return Value.binary(context->this.data.date->ms = msClip(msFromDateAndTime(date, time)));
}

static struct Value setMinutes (struct Context * const context)
{
	struct date date;
	struct time time;
	double m, s, ms;
	
	Context.assertParameterCount(context, 3);
	Context.assertThisType(context, Value(dateType));
	
	msToDateAndTime(toLocal(context->this.data.date->ms), &date, &time);
	m = binaryArgumentOr(context, 0, NAN);
	s = binaryArgumentOr(context, 1, time.s);
	ms = binaryArgumentOr(context, 2, time.ms);
	if (isnan(m) || isnan(s) || isnan(ms))
		return Value.binary(context->this.data.date->ms = NAN);
	
	time.m = m;
	time.s = s;
	time.ms = ms;
	return Value.binary(context->this.data.date->ms = msClip(toUTC(msFromDateAndTime(date, time))));
}

static struct Value setUTCMinutes (struct Context * const context)
{
	struct date date;
	struct time time;
	double m, s, ms;
	
	Context.assertParameterCount(context, 3);
	Context.assertThisType(context, Value(dateType));
	
	msToDateAndTime(context->this.data.date->ms, &date, &time);
	m = binaryArgumentOr(context, 0, NAN);
	s = binaryArgumentOr(context, 1, time.s);
	ms = binaryArgumentOr(context, 2, time.ms);
	if (isnan(m) || isnan(s) || isnan(ms))
		return Value.binary(context->this.data.date->ms = NAN);
	
	time.m = m;
	time.s = s;
	time.ms = ms;
	return Value.binary(context->this.data.date->ms = msClip(msFromDateAndTime(date, time)));
}

static struct Value setHours (struct Context * const context)
{
	struct date date;
	struct time time;
	double h, m, s, ms;
	
	Context.assertParameterCount(context, 4);
	Context.assertThisType(context, Value(dateType));
	
	msToDateAndTime(toLocal(context->this.data.date->ms), &date, &time);
	h = binaryArgumentOr(context, 0, NAN);
	m = binaryArgumentOr(context, 1, time.m);
	s = binaryArgumentOr(context, 2, time.s);
	ms = binaryArgumentOr(context, 3, time.ms);
	if (isnan(h) || isnan(m) || isnan(s) || isnan(ms))
		return Value.binary(context->this.data.date->ms = NAN);
	
	time.h = h;
	time.m = m;
	time.s = s;
	time.ms = ms;
	return Value.binary(context->this.data.date->ms = msClip(toUTC(msFromDateAndTime(date, time))));
}

static struct Value setUTCHours (struct Context * const context)
{
	struct date date;
	struct time time;
	double h, m, s, ms;
	
	Context.assertParameterCount(context, 4);
	Context.assertThisType(context, Value(dateType));
	
	msToDateAndTime(context->this.data.date->ms, &date, &time);
	h = binaryArgumentOr(context, 0, NAN);
	m = binaryArgumentOr(context, 1, time.m);
	s = binaryArgumentOr(context, 2, time.s);
	ms = binaryArgumentOr(context, 3, time.ms);
	if (isnan(h) || isnan(m) || isnan(s) || isnan(ms))
		return Value.binary(context->this.data.date->ms = NAN);
	
	time.h = h;
	time.m = m;
	time.s = s;
	time.ms = ms;
	return Value.binary(context->this.data.date->ms = msClip(msFromDateAndTime(date, time)));
}

static struct Value setDate (struct Context * const context)
{
	struct date date;
	double day, ms;
	
	Context.assertParameterCount(context, 1);
	Context.assertThisType(context, Value(dateType));
	
	ms = msToDate(toLocal(context->this.data.date->ms), &date);
	day = binaryArgumentOr(context, 0, NAN);
	if (isnan(day))
		return Value.binary(context->this.data.date->ms = NAN);
	
	date.day = day;
	return Value.binary(context->this.data.date->ms = msClip(toUTC(ms + msFromDate(date))));
}

static struct Value setUTCDate (struct Context * const context)
{
	struct date date;
	double day, ms;
	
	Context.assertParameterCount(context, 1);
	Context.assertThisType(context, Value(dateType));
	
	ms = msToDate(context->this.data.date->ms, &date);
	day = binaryArgumentOr(context, 0, NAN);
	if (isnan(day))
		return Value.binary(context->this.data.date->ms = NAN);
	
	date.day = day;
	return Value.binary(context->this.data.date->ms = msClip(ms + msFromDate(date)));
}

static struct Value setMonth (struct Context * const context)
{
	struct date date;
	double month, day, ms;
	
	Context.assertParameterCount(context, 2);
	Context.assertThisType(context, Value(dateType));
	
	ms = msToDate(toLocal(context->this.data.date->ms), &date);
	month = binaryArgumentOr(context, 0, NAN) + 1;
	day = binaryArgumentOr(context, 1, date.day);
	if (isnan(month) || isnan(day))
		return Value.binary(context->this.data.date->ms = NAN);
	
	date.month = month;
	date.day = day;
	return Value.binary(context->this.data.date->ms = msClip(toUTC(ms + msFromDate(date))));
}

static struct Value setUTCMonth (struct Context * const context)
{
	struct date date;
	double month, day, ms;
	
	Context.assertParameterCount(context, 2);
	Context.assertThisType(context, Value(dateType));
	
	ms = msToDate(context->this.data.date->ms, &date);
	month = binaryArgumentOr(context, 0, NAN) + 1;
	day = binaryArgumentOr(context, 1, date.day);
	if (isnan(month) || isnan(day))
		return Value.binary(context->this.data.date->ms = NAN);
	
	date.month = month;
	date.day = day;
	return Value.binary(context->this.data.date->ms = msClip(ms + msFromDate(date)));
}

static struct Value setFullYear (struct Context * const context)
{
	struct date date;
	double year, month, day, ms;
	
	Context.assertParameterCount(context, 3);
	Context.assertThisType(context, Value(dateType));
	
	if (isnan(context->this.data.date->ms))
		context->this.data.date->ms = 0;
	
	ms = msToDate(toLocal(context->this.data.date->ms), &date);
	year = binaryArgumentOr(context, 0, NAN);
	month = binaryArgumentOr(context, 1, date.month - 1) + 1;
	day = binaryArgumentOr(context, 2, date.day);
	if (isnan(year) || isnan(month) || isnan(day))
		return Value.binary(context->this.data.date->ms = NAN);
	
	date.year = year;
	date.month = month;
	date.day = day;
	return Value.binary(context->this.data.date->ms = msClip(toUTC(ms + msFromDate(date))));
}

static struct Value setUTCFullYear (struct Context * const context)
{
	struct date date;
	double year, month, day, ms;
	
	Context.assertParameterCount(context, 3);
	Context.assertThisType(context, Value(dateType));
	
	if (isnan(context->this.data.date->ms))
		context->this.data.date->ms = 0;
	
	ms = msToDate(context->this.data.date->ms, &date);
	year = binaryArgumentOr(context, 0, NAN);
	month = binaryArgumentOr(context, 1, date.month - 1) + 1;
	day = binaryArgumentOr(context, 2, date.day);
	if (isnan(year) || isnan(month) || isnan(day))
		return Value.binary(context->this.data.date->ms = NAN);
	
	date.year = year;
	date.month = month;
	date.day = day;
	return Value.binary(context->this.data.date->ms = msClip(ms + msFromDate(date)));
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
		time = toUTC(msFromArguments(context));
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
	
	setupLocalOffset();
	
	Function.setupBuiltinObject(&Date(constructor), dateConstructor, -7, &Date(prototype), Value.date(create(NAN)), &Date(type));
	
	Function.addMethod(Date(constructor), "parse", parse, 1, flags);
	Function.addMethod(Date(constructor), "UTC", UTC, -7, flags);
	Function.addMethod(Date(constructor), "now", now, 0, flags);
	
	Function.addToObject(Date(prototype), "toString", toString, 0, flags);
	Function.addToObject(Date(prototype), "toDateString", toDateString, 0, flags);
	Function.addToObject(Date(prototype), "toTimeString", toTimeString, 0, flags);
	Function.addToObject(Date(prototype), "toLocaleString", toString, 0, flags);
	Function.addToObject(Date(prototype), "toLocaleDateString", toDateString, 0, flags);
	Function.addToObject(Date(prototype), "toLocaleTimeString", toTimeString, 0, flags);
	Function.addToObject(Date(prototype), "valueOf", valueOf, 0, flags);
	Function.addToObject(Date(prototype), "getTime", valueOf, 0, flags);
	Function.addToObject(Date(prototype), "getFullYear", getFullYear, 0, flags);
	Function.addToObject(Date(prototype), "getUTCFullYear", getUTCFullYear, 0, flags);
	Function.addToObject(Date(prototype), "getMonth", getMonth, 0, flags);
	Function.addToObject(Date(prototype), "getUTCMonth", getUTCMonth, 0, flags);
	Function.addToObject(Date(prototype), "getDate", getDate, 0, flags);
	Function.addToObject(Date(prototype), "getUTCDate", getUTCDate, 0, flags);
	Function.addToObject(Date(prototype), "getDay", getDay, 0, flags);
	Function.addToObject(Date(prototype), "getUTCDay", getUTCDay, 0, flags);
	Function.addToObject(Date(prototype), "getHours", getHours, 0, flags);
	Function.addToObject(Date(prototype), "getUTCHours", getUTCHours, 0, flags);
	Function.addToObject(Date(prototype), "getMinutes", getMinutes, 0, flags);
	Function.addToObject(Date(prototype), "getUTCMinutes", getUTCMinutes, 0, flags);
	Function.addToObject(Date(prototype), "getSeconds", getSeconds, 0, flags);
	Function.addToObject(Date(prototype), "getUTCSeconds", getUTCSeconds, 0, flags);
	Function.addToObject(Date(prototype), "getMilliseconds", getMilliseconds, 0, flags);
	Function.addToObject(Date(prototype), "getUTCMilliseconds", getUTCMilliseconds, 0, flags);
	Function.addToObject(Date(prototype), "getTimezoneOffset", getTimezoneOffset, 0, flags);
	Function.addToObject(Date(prototype), "setTime", setTime, 1, flags);
	Function.addToObject(Date(prototype), "setMilliseconds", setMilliseconds, 1, flags);
	Function.addToObject(Date(prototype), "setUTCMilliseconds", setUTCMilliseconds, 1, flags);
	Function.addToObject(Date(prototype), "setSeconds", setSeconds, 2, flags);
	Function.addToObject(Date(prototype), "setUTCSeconds", setUTCSeconds, 2, flags);
	Function.addToObject(Date(prototype), "setMinutes", setMinutes, 3, flags);
	Function.addToObject(Date(prototype), "setUTCMinutes", setUTCMinutes, 3, flags);
	Function.addToObject(Date(prototype), "setHours", setHours, 4, flags);
	Function.addToObject(Date(prototype), "setUTCHours", setUTCHours, 4, flags);
	Function.addToObject(Date(prototype), "setDate", setDate, 1, flags);
	Function.addToObject(Date(prototype), "setUTCDate", setUTCDate, 1, flags);
	Function.addToObject(Date(prototype), "setMonth", setMonth, 2, flags);
	Function.addToObject(Date(prototype), "setUTCMonth", setUTCMonth, 2, flags);
	Function.addToObject(Date(prototype), "setFullYear", setFullYear, 3, flags);
	Function.addToObject(Date(prototype), "setUTCFullYear", setUTCFullYear, 3, flags);
	Function.addToObject(Date(prototype), "toUTCString", toUTCString, 0, flags);
	Function.addToObject(Date(prototype), "toISOString", toISOString, 0, flags);
	Function.addToObject(Date(prototype), "toJSON", toJSON, 1, flags);
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

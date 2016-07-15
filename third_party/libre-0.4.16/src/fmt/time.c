/**
 * @file time.c  Time formatting
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <time.h>
#include <re_types.h>
#include <re_fmt.h>


static const char *dayv[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

static const char *monv[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
			     "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};


/**
 * Print Greenwich Mean Time
 *
 * @param pf Print function for output
 * @param ts Time in seconds since the Epoch or NULL for current time
 *
 * @return 0 if success, otherwise errorcode
 */
int fmt_gmtime(struct re_printf *pf, void *ts)
{
	const struct tm *tm;
	time_t t;

	if (!ts) {
		t  = time(NULL);
		ts = &t;
	}

	tm = gmtime(ts);
	if (!tm)
		return EINVAL;

	return re_hprintf(pf, "%s, %02u %s %u %02u:%02u:%02u GMT",
			  dayv[min((unsigned)tm->tm_wday, ARRAY_SIZE(dayv)-1)],
			  tm->tm_mday,
			  monv[min((unsigned)tm->tm_mon, ARRAY_SIZE(monv)-1)],
			  tm->tm_year + 1900,
			  tm->tm_hour, tm->tm_min, tm->tm_sec);
}


/**
 * Print the human readable time
 *
 * @param pf       Print function for output
 * @param seconds  Pointer to number of seconds
 *
 * @return 0 if success, otherwise errorcode
 */
int fmt_human_time(struct re_printf *pf, const uint32_t *seconds)
{
	/* max 136 years */
	const uint32_t sec  = *seconds%60;
	const uint32_t min  = *seconds/60%60;
	const uint32_t hrs  = *seconds/60/60%24;
	const uint32_t days = *seconds/60/60/24;
	int err = 0;

	if (days)
		err |= re_hprintf(pf, "%u day%s ", days, 1==days?"":"s");

	if (hrs) {
		err |= re_hprintf(pf, "%u hour%s ", hrs, 1==hrs?"":"s");
	}

	if (min) {
		err |= re_hprintf(pf, "%u min%s ", min, 1==min?"":"s");
	}

	if (sec) {
		err |= re_hprintf(pf, "%u sec%s", sec, 1==sec?"":"s");
	}

	return err;
}

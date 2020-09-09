/**************************************************************************//**
 * @file     libc_wrap.c
 * @brief    The wraper functions of ROM code to replace some of utility
 *           functions in Compiler's Library.
 * @version  V1.00
 * @date     2018-08-15
 *
 * @note
 *
 ******************************************************************************
 *
 * Copyright(c) 2007 - 2018 Realtek Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************/
#if defined(CONFIG_PLATFORM_8195BHP) || defined(CONFIG_PLATFORM_8195BLP) || defined(CONFIG_PLATFORM_8710C)
#include "platform_conf.h"
#endif
#include "basic_types.h"
#include "ctype.h"

/* for GNU C++ */
#if defined(__GNUC__)
void* __dso_handle = 0;
#endif

#if defined(CONFIG_CMSIS_FREERTOS_EN) && (CONFIG_CMSIS_FREERTOS_EN != 0)
/**************************************************
 * FreeRTOS memory management functions's wrapper to replace 
 * malloc/free/realloc of GCC Lib.
 **************************************************/
//#include "FreeRTOS.h"
// pvPortReAlloc currently not defined in portalbe.h
extern void* pvPortReAlloc( void *pv,  size_t xWantedSize );
extern void *pvPortMalloc( size_t xWantedSize );
extern void *pvPortCalloc(size_t xWantedCnt, size_t xWantedSize);
extern void vPortFree( void *pv );

void* __wrap_malloc( size_t size )
{
    return pvPortMalloc(size);
}

void *__wrap__malloc_r(void *reent, size_t size)
{
    return pvPortMalloc(size);
}

void* __wrap_realloc( void *p, size_t size )
{
    return (void*)pvPortReAlloc(p, size);
}

void* __wrap_calloc( size_t cnt, size_t size )
{
    return (void*)pvPortCalloc(cnt, size);
}

void* __wrap__calloc_r( void *reent, size_t cnt, size_t size )
{
    return (void*)pvPortCalloc(cnt, size);
}

void __wrap_free( void *p )
{
    vPortFree(p);
}
#endif  //  #if defined(CONFIG_CMSIS_FREERTOS_EN) && (CONFIG_CMSIS_FREERTOS_EN != 0)

/**************************************************
 * string and memory api wrap for compiler
 *
 **************************************************/

#if defined(CONFIG_PLATFORM_8195BHP) || defined(CONFIG_PLATFORM_8195BLP) || defined(CONFIG_PLATFORM_8710C)
#include "stdio_port.h"
#include "rt_printf.h"

int __wrap_printf(const char * fmt,...)
{
	int count;
    va_list list;
    va_start(list, fmt);	
#if defined(CONFIG_BUILD_SECURE)	
	count = stdio_printf_stubs.printf_corel(stdio_printf_stubs.stdio_port_sputc, (void*)NULL, fmt, list);
#else
	count = stdio_printf_stubs.printf_core(stdio_printf_stubs.stdio_port_sputc, (void*)NULL, fmt, list);
#endif
	va_end(list);
	return count;
}

int __wrap_puts(const char *str)
{
	return __wrap_printf("%s\n", str);
}

int __wrap_vprintf(const char *fmt, va_list args)
{
        int count;
#if defined(CONFIG_BUILD_SECURE)     
        count = stdio_printf_stubs.printf_corel(stdio_printf_stubs.stdio_port_sputc, (void*)NULL, fmt, args);
#else
        count = stdio_printf_stubs.printf_core(stdio_printf_stubs.stdio_port_sputc, (void*)NULL, fmt, args);
#endif
        return count;
}

int __wrap_sprintf(char *buf, const char * fmt,...)
{
    int count;
    va_list list;
    stdio_buf_t pnt_buf;

    pnt_buf.pbuf = buf;
    pnt_buf.pbuf_lim = 0;

    va_start(list, fmt);
#if defined(CONFIG_BUILD_SECURE)		
    count = stdio_printf_stubs.printf_corel(stdio_printf_stubs.stdio_port_bufputc, (void *)&pnt_buf, fmt, list);
#else
	count = stdio_printf_stubs.printf_core(stdio_printf_stubs.stdio_port_bufputc, (void *)&pnt_buf, fmt, list);
#endif
    *(pnt_buf.pbuf) = 0;
    va_end(list);
	(void)list;

    return count;	
}

int __wrap_snprintf(char *buf, size_t size, const char *fmt,...)
{
    int count;
    va_list list;
    stdio_buf_t pnt_buf;

    pnt_buf.pbuf = buf;
    pnt_buf.pbuf_lim = buf + size - 1;  // reserve 1 byte for 'end of string'

    va_start(list,fmt);
#if defined(CONFIG_BUILD_SECURE)
    count = stdio_printf_stubs.printf_corel(stdio_printf_stubs.stdio_port_bufputc,(void *)&pnt_buf, fmt, list);
#else
	count = stdio_printf_stubs.printf_core(stdio_printf_stubs.stdio_port_bufputc,(void *)&pnt_buf, fmt, list);
#endif
    *(pnt_buf.pbuf) = 0;
    va_end(list);
	(void)list;

    return count;	
}

int __wrap_vsnprintf(char *buf, size_t size, const char *fmt, va_list args)
{
    int count;
    stdio_buf_t pnt_buf;

    pnt_buf.pbuf = buf;
    pnt_buf.pbuf_lim = buf + size - 1;  // reserve 1 byte for 'end of string'
#if defined(CONFIG_BUILD_SECURE)
    count = stdio_printf_stubs.printf_corel(stdio_printf_stubs.stdio_port_bufputc,(void *)&pnt_buf, fmt, args);
#else
	count = stdio_printf_stubs.printf_core(stdio_printf_stubs.stdio_port_bufputc,(void *)&pnt_buf, fmt, args);
#endif
    *(pnt_buf.pbuf) = 0;

    return count;	
}


// define in AmebaPro utilites/include/memory.h
#include "memory.h"
int __wrap_memcmp(const void *av, const void *bv, size_t len)
{
	return rt_memcmp(av, bv, len);
}

void *__wrap_memcpy( void *s1, const void *s2, size_t n )
{
	return rt_memcpy(s1, s2, n);
}

void *__wrap_memmove (void *destaddr, const void *sourceaddr, unsigned length)
{
	return rt_memmove (destaddr, sourceaddr, length);
}

void *__wrap_memset(void *dst0, int val, size_t length)
{
	return rt_memset(dst0, val, length);
}
// define in AmebaPro utilites/include/strporc.h
// replace by linking command
#include "strproc.h"
char *__wrap_strcat(char *dest,  char const *src)
{
	return strcat(dest, src);
}

char *__wrap_strchr(const char *s, int c)
{
	return strchr(s, c);
}

int __wrap_strcmp(char const *cs, char const *ct)
{
	return strcmp(cs, ct);
}

int __wrap_strncmp(char const *cs, char const *ct, size_t count)
{
	return strncmp(cs, ct, count);
}

int __wrap_strnicmp(char const *s1, char const *s2, size_t len)
{
	return strnicmp(s1, s2, len);
}


char *__wrap_strcpy(char *dest, char const *src)
{
	return strcpy(dest, src);
}


char *__wrap_strncpy(char *dest, char const *src, size_t count)
{
	return strncpy(dest, src, count);
}


size_t __wrap_strlcpy(char *dst, char const *src, size_t s)
{
	return strlcpy(dst, src, s);
}


size_t __wrap_strlen(char const *s)
{
	return strlen(s);
}


size_t __wrap_strnlen(char const *s, size_t count)
{
	return strnlen(s, count);
}


char *__wrap_strncat(char *dest, char const *src, size_t count)
{
	return strncat(dest, src, count);
}

char *__wrap_strpbrk(char const *cs, char const *ct)
{
	return strpbrk(cs, ct);
}


size_t __wrap_strspn(char const *s, char const *accept)
{
	return strspn(s, accept);
}


char *__wrap_strstr(char const *s1, char const *s2)
{
	return strstr(s1, s2);
}


char *__wrap_strtok(char *s, char const *ct)
{
	return strtok(s, ct);
}


size_t __wrap_strxfrm(char *dest, const char *src, size_t n)
{
	return strxfrm(dest, src, n);
}

char *__wrap_strsep(char **s, const char *ct)
{
	return strsep(s, ct);
}

double __wrap_strtod(const char *str, char **endptr)
{
	return strtod(str, endptr);
}

float __wrap_strtof(const char *str, char **endptr)
{
	return strtof(str, endptr);
}


long double __wrap_strtold(const char *str, char **endptr)
{
	return strtold(str, endptr);
}

long __wrap_strtol(const char *nptr, char **endptr, int base)
{
	return strtol(nptr, endptr, base);
}


long long __wrap_strtoll(const char *nptr, char **endptr, int base)
{
	return strtoll(nptr, endptr, base);
}


unsigned long __wrap_strtoul(const char *nptr, char **endptr, int base)
{
	return strtoul(nptr, endptr, base);
}


unsigned long long __wrap_strtoull(const char *nptr, char **endptr, int base)
{
	return strtoull(nptr, endptr, base);
}

int __wrap_atoi(const char *num)
{
#if 0
	return atoi(num);
#else
	int c;
	long total;
	int sign; 
	char *nptr = (char *)num;

	/* skip whitespace */
	while ( isspace((int)(unsigned char)*nptr) )
		++nptr;

	c = (int)(unsigned char)*nptr++;
	sign = c;
	if (c == '-' || c == '+')
		c = (int)(unsigned char)*nptr++;

	total = 0;
	while (isdigit(c)) {
		total = 10 * total + (c - '0');
		c = (int)(unsigned char)*nptr++;
	}

	if (sign == '-')
		return -total;
	else
		return total;
#endif	
}

unsigned int __wrap_atoui(const char *num)
{
	return atoui(num);
}

long __wrap_atol(const char *num)
{
	return atol(num);
}

unsigned long __wrap_atoul(const char *num)
{
	return atoul(num);
}


unsigned long long __wrap_atoull(const char *num)
{
	return atoull(num);
}


double __wrap_atof(const char *str)
{
	return atof(str);
}	

void __wrap_abort(void)
{
	__wrap_printf("\n\rabort execution\n\r");
	while(1);
}
  
#if defined(__GNUC__)
#include <errno.h>

static int gnu_errno;
volatile int * __aeabi_errno_addr (void)
{
  return &gnu_errno;
}
#endif

#endif // #if defined(CONFIG_PLATFORM_8195BHP) || defined(CONFIG_PLATFORM_8195BLP) || defined(CONFIG_PLATFORM_8710C)

/**************************************************
 * mktime/localtime wrap for gcc compiler
 * port from newlib v3.2.0
 * for 64 bit time
 **************************************************/

#define SECSPERMIN	60L
#define MINSPERHOUR	60L
#define HOURSPERDAY	24L
#define SECSPERHOUR	(SECSPERMIN * MINSPERHOUR)
#define SECSPERDAY	(SECSPERHOUR * HOURSPERDAY)
#define DAYSPERWEEK	7
#define MONSPERYEAR	12

#define YEAR_BASE	1900
#define EPOCH_YEAR      1970
#define EPOCH_WDAY      4
#define EPOCH_YEARS_SINCE_LEAP 2
#define EPOCH_YEARS_SINCE_CENTURY 70
#define EPOCH_YEARS_SINCE_LEAP_CENTURY 370

#define isleap(y) ((((y) % 4) == 0 && ((y) % 100) != 0) || ((y) % 400) == 0)

#define EPOCH_ADJUSTMENT_DAYS	719468L
/* year to which the adjustment was made */
#define ADJUSTED_EPOCH_YEAR	0
/* 1st March of year 0 is Wednesday */
#define ADJUSTED_EPOCH_WDAY	3
/* there are 97 leap years in 400-year periods. ((400 - 97) * 365 + 97 * 366) */
#define DAYS_PER_ERA		146097L
/* there are 24 leap years in 100-year periods. ((100 - 24) * 365 + 24 * 366) */
#define DAYS_PER_CENTURY	36524L
/* there is one leap year every 4 years */
#define DAYS_PER_4_YEARS	(3 * 365 + 366)
/* number of days in a non-leap year */
#define DAYS_PER_YEAR		365
/* number of days in January */
#define DAYS_IN_JANUARY		31
/* number of days in non-leap February */
#define DAYS_IN_FEBRUARY	28
/* number of years per era */
#define YEARS_PER_ERA		400

#define _SEC_IN_MINUTE 60L
#define _SEC_IN_HOUR 3600L
#define _SEC_IN_DAY 86400L

static const int DAYS_IN_MONTH[12] =
{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

#define _DAYS_IN_MONTH(x) ((x == 1) ? days_in_feb : DAYS_IN_MONTH[x])

static const int _DAYS_BEFORE_MONTH[12] =
{0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

#define _ISLEAP(y) (((y) % 4) == 0 && (((y) % 100) != 0 || (((y)+1900) % 400) == 0))
#define _DAYS_IN_YEAR(year) (_ISLEAP(year) ? 366 : 365)

#include "time64.h"
#include <platform_stdlib.h>

#if defined (__ICCARM__)
extern long long  _Tzoff();

typedef struct __tzrule_struct
{
  char ch;
  int m;
  int n;
  int d;
  int s;
  time_t change;
  long offset; /* Match type of _timezone. */
} __tzrule_type;

typedef struct __tzinfo_struct
{
  int __tznorth;
  int __tzyear;
  __tzrule_type __tzrule[2];
} __tzinfo_type;

/* Shared timezone information for libc/time functions.  */
static __tzinfo_type tzinfo = {1, 0,
{ {'J', 0, 0, 0, 0, (time_t)0, 0L },
  {'J', 0, 0, 0, 0, (time_t)0, 0L } 
  } 
};

__tzinfo_type *
  __gettzinfo (void)
  {
    return &tzinfo;
  }

#endif

#if defined(__GNUC__)
extern int __tzcalc_limits (int);
#endif

struct tm *
gmtime64_r (const time_t *__restrict tim_p,
	struct tm *__restrict res)
{
  long days, rem;
  const time_t lcltime = *tim_p;
  int era, weekday, year;
  unsigned erayear, yearday, month, day;
  unsigned long eraday;

  days = lcltime / SECSPERDAY + EPOCH_ADJUSTMENT_DAYS;
  rem = lcltime % SECSPERDAY;
  if (rem < 0)
    {
      rem += SECSPERDAY;
      --days;
    }

  /* compute hour, min, and sec */
  res->tm_hour = (int) (rem / SECSPERHOUR);
  rem %= SECSPERHOUR;
  res->tm_min = (int) (rem / SECSPERMIN);
  res->tm_sec = (int) (rem % SECSPERMIN);

  /* compute day of week */
  if ((weekday = ((ADJUSTED_EPOCH_WDAY + days) % DAYSPERWEEK)) < 0)
    weekday += DAYSPERWEEK;
  res->tm_wday = weekday;

  /* compute year, month, day & day of year */
  /* for description of this algorithm see
   * http://howardhinnant.github.io/date_algorithms.html#civil_from_days */
  era = (days >= 0 ? days : days - (DAYS_PER_ERA - 1)) / DAYS_PER_ERA;
  eraday = days - era * DAYS_PER_ERA;	/* [0, 146096] */
  erayear = (eraday - eraday / (DAYS_PER_4_YEARS - 1) + eraday / DAYS_PER_CENTURY -
      eraday / (DAYS_PER_ERA - 1)) / 365;	/* [0, 399] */
  yearday = eraday - (DAYS_PER_YEAR * erayear + erayear / 4 - erayear / 100);	/* [0, 365] */
  month = (5 * yearday + 2) / 153;	/* [0, 11] */
  day = yearday - (153 * month + 2) / 5 + 1;	/* [1, 31] */
  month += month < 10 ? 2 : -10;
  year = ADJUSTED_EPOCH_YEAR + erayear + era * YEARS_PER_ERA + (month <= 1);

  res->tm_yday = yearday >= DAYS_PER_YEAR - DAYS_IN_JANUARY - DAYS_IN_FEBRUARY ?
      yearday - (DAYS_PER_YEAR - DAYS_IN_JANUARY - DAYS_IN_FEBRUARY) :
      yearday + DAYS_IN_JANUARY + DAYS_IN_FEBRUARY + isleap(erayear);
  res->tm_year = year - YEAR_BASE;
  res->tm_mon = month;
  res->tm_mday = day;

  res->tm_isdst = 0;

  return (res);
}

struct tm * __wrap_localtime (const time_t * tim_p)
{
#if defined (__GNUC__)
  struct _reent *reent = _REENT;

  _REENT_CHECK_TM(reent);
  return gmtime64_r (tim_p, (struct tm *)_REENT_TM(reent));

#elif defined (__ICCARM__) //because IAR does not have _reent structure
  __ATTRIBUTES struct tm * __iar_Ttotm64(struct tm *, __time64_t, int);
  struct tm* tm = __iar_Ttotm64(0, *tim_p + (__time64_t) _Tzoff(), -1);  
  return gmtime64_r (tim_p, tm);
#endif
}

static void
validate_structure (struct tm *tim_p)
{
  div_t res;
  int days_in_feb = 28;

  /* calculate time & date to account for out of range values */
  if (tim_p->tm_sec < 0 || tim_p->tm_sec > 59)
    {
      res = div (tim_p->tm_sec, 60);
      tim_p->tm_min += res.quot;
      if ((tim_p->tm_sec = res.rem) < 0)
	{
	  tim_p->tm_sec += 60;
	  --tim_p->tm_min;
	}
    }

  if (tim_p->tm_min < 0 || tim_p->tm_min > 59)
    {
      res = div (tim_p->tm_min, 60);
      tim_p->tm_hour += res.quot;
      if ((tim_p->tm_min = res.rem) < 0)
	{
	  tim_p->tm_min += 60;
	  --tim_p->tm_hour;
        }
    }

  if (tim_p->tm_hour < 0 || tim_p->tm_hour > 23)
    {
      res = div (tim_p->tm_hour, 24);
      tim_p->tm_mday += res.quot;
      if ((tim_p->tm_hour = res.rem) < 0)
	{
	  tim_p->tm_hour += 24;
	  --tim_p->tm_mday;
        }
    }

  if (tim_p->tm_mon < 0 || tim_p->tm_mon > 11)
    {
      res = div (tim_p->tm_mon, 12);
      tim_p->tm_year += res.quot;
      if ((tim_p->tm_mon = res.rem) < 0)
        {
	  tim_p->tm_mon += 12;
	  --tim_p->tm_year;
        }
    }

  if (_DAYS_IN_YEAR (tim_p->tm_year) == 366)
    days_in_feb = 29;

  if (tim_p->tm_mday <= 0)
    {
      while (tim_p->tm_mday <= 0)
	{
	  if (--tim_p->tm_mon == -1)
	    {
	      tim_p->tm_year--;
	      tim_p->tm_mon = 11;
	      days_in_feb =
		((_DAYS_IN_YEAR (tim_p->tm_year) == 366) ?
		 29 : 28);
	    }
	  tim_p->tm_mday += _DAYS_IN_MONTH (tim_p->tm_mon);
	}
    }
  else
    {
      while (tim_p->tm_mday > _DAYS_IN_MONTH (tim_p->tm_mon))
	{
	  tim_p->tm_mday -= _DAYS_IN_MONTH (tim_p->tm_mon);
	  if (++tim_p->tm_mon == 12)
	    {
	      tim_p->tm_year++;
	      tim_p->tm_mon = 0;
	      days_in_feb =
		((_DAYS_IN_YEAR (tim_p->tm_year) == 366) ?
		 29 : 28);
	    }
	}
    }
}

time_t __wrap_mktime (struct tm *tim_p)
{
  time_t tim = 0;
  long days = 0;
  int year, isdst=0;
  __tzinfo_type *tz = __gettzinfo ();

  /* validate structure */
  validate_structure (tim_p);

  /* compute hours, minutes, seconds */
  tim += tim_p->tm_sec + (tim_p->tm_min * _SEC_IN_MINUTE) +
    (tim_p->tm_hour * _SEC_IN_HOUR);

  /* compute days in year */
  days += tim_p->tm_mday - 1;
  days += _DAYS_BEFORE_MONTH[tim_p->tm_mon];
  if (tim_p->tm_mon > 1 && _DAYS_IN_YEAR (tim_p->tm_year) == 366)
    days++;

  /* compute day of the year */
  tim_p->tm_yday = days;

  if (tim_p->tm_year > 10000 || tim_p->tm_year < -10000)
      return (time_t) -1;

  /* compute days in other years */
  if ((year = tim_p->tm_year) > 70)
    {
      for (year = 70; year < tim_p->tm_year; year++)
	days += _DAYS_IN_YEAR (year);
    }
  else if (year < 70)
    {
      for (year = 69; year > tim_p->tm_year; year--)
	days -= _DAYS_IN_YEAR (year);
      days -= _DAYS_IN_YEAR (year);
    }

  /* compute total seconds */
  tim += (time_t)days * _SEC_IN_DAY;

  /* add appropriate offset to put time in gmt format */
  if (isdst == 1)
    tim += (time_t) tz->__tzrule[1].offset;
  else /* otherwise assume std time */
    tim += (time_t) tz->__tzrule[0].offset;

  /* reset isdst flag to what we have calculated */
  tim_p->tm_isdst = isdst;

  /* compute day of the week */
  if ((tim_p->tm_wday = (days + 4) % 7) < 0)
    tim_p->tm_wday += 7;

  return tim;
}

char * __wrap_ctime (long long *t)
{
  /* The C Standard macrsays ctime (t) is equivalent to asctime (localtime (t)).
     In particular, ctime and asctime must yield the same pointer.  */
  return asctime (__wrap_localtime (t));
}


/**********************************************************************************************************************
 *                                                                                                                    *
 *  G A U G E  M O O N . C                                                                                            *
 *  ======================                                                                                            *
 *                                                                                                                    *
 *  Copyright (c) 2023 Chris Knight                                                                                   *
 *                                                                                                                    *
 *  File GaugeMoon.c part of Gauge is free software: you can redistribute it and/or modify it under the terms of the  *
 *  GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at  *
 *  your option) any later version.                                                                                   *
 *                                                                                                                    *
 *  Gauge is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied       *
 *  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more     *
 *  details.                                                                                                          *
 *                                                                                                                    *
 *  You should have received a copy of the GNU General Public License along with this program. If not, see:           *
 *  <http://www.gnu.org/licenses/>                                                                                    *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \file
 *  \brief Handle a gauge that shows battery.
 */
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#define		PI	3.1415926535897932384626433832795
#define		RAD (PI/180.0)
#define			SMALL_FLOAT (1e-12)

typedef struct
{
	int year, month, day;
	double hour;
}
TimePlace;

#include "GaugeDisp.h"

extern FACE_SETTINGS *faceSettings[];
extern GAUGE_ENABLED gaugeEnabled[];
extern MENU_DESC gaugeMenuDesc[];
extern int sysUpdateID;

static int myUpdateID = 100;
static double lastRead = -1;
static int change = 0;

/**********************************************************************************************************************
 *                                                                                                                    *
 *  R E A D  M O O N  P H A S E  I N I T                                                                              *
 *  ====================================                                                                              *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Called on program start to init the gauge.
 *  \result None.
 */
void readMoonPhaseInit (void)
{
	if (gaugeEnabled[FACE_TYPE_MOONPHASE].enabled)
	{
		gaugeMenuDesc[MENU_GAUGE_MOONPHASE].disable = 0;
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  J U L I A N  T O  D A T E                                                                                         *
 *  =========================                                                                                         *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Convert a Julian date back to a day month year.
 *  \param now Save the result here.
 *  \param jd Date to convert.
 *  \result None.
 */
void JulianToDate(TimePlace * now, double jd)
{
	long jdi, b;
	long c, d, e, g, g1;

	jd += 0.5;
	jdi = jd;
	if (jdi > 2299160)
	{
		long a = (jdi - 1867216.25) / 36524.25;
		b = jdi + 1 + a - a / 4;
	}
	else
		b = jdi;

	c = b + 1524;
	d = (c - 122.1) / 365.25;
	e = 365.25 * d;
	g = (c - e) / 30.6001;
	g1 = 30.6001 * g;
	now->day = c - e - g1;
	now->hour = (jd - jdi) * 24.0;
	if (g <= 13)
		now->month = g - 1;
	else
		now->month = g - 13;
	if (now->month > 2)
		now->year = d - 4716;
	else
		now->year = d - 4715;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  J U L I A N                                                                                                       *
 *  ===========                                                                                                       *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Calculate a Julian date.
 *  \param year Year to use.
 *  \param month Month to use.
 *  \param day Daye to use.
 *  \result Julian date.
 */
double Julian(int year, int month, double day)
{
	/*
     * Returns the number of julian days for the specified day.
     */

	int a, b = 0, c, e;
	if (month < 3)
	{
		year--;
		month += 12;
	}
	if (year > 1582 || (year == 1582 && month > 10) || (year == 1582 && month == 10 && day > 15))
	{
		a = year / 100;
		b = 2 - a + a / 4;
	}
	c = 365.25 * year;
	e = 30.6001 * (month + 1);
	return b + c + e + day + 1720994.5;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S U N _ P O S I T I O N                                                                                           *
 *  =======================                                                                                           *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Calculate the suns position.
 *  \param j Don't know copied this code.
 *  \result Don't know copied this code.
 */
double sun_position(double j)
{
	double n, x, e, l, dl, v;
	int i;

	n = 360 / 365.2422 * j;
	i = n / 360;
	n = n - i * 360.0;
	x = n - 3.762863;
	if (x < 0)
		x += 360;
	x *= RAD;
	e = x;
	do
	{
		dl = e - .016718 * sin(e) - x;
		e = e - dl / (1 - .016718 * cos(e));
	}
	while (fabs(dl) >= SMALL_FLOAT);
	v = 360 / PI * atan(1.01686011182 * tan(e / 2));
	l = v + 282.596403;
	i = l / 360;
	l = l - i * 360.0;
	return l;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  M O O N _ P O S I T I O N                                                                                         *
 *  =========================                                                                                         *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Calculate the moon position.
 *  \param j Don't know, copied this code.
 *  \param ls Don't know copied this code.
 *  \result Don't know copied this code.
 */
double moon_position(double j, double ls)
{

	double ms, l, mm, n, ev, sms, ae, ec;
	int i;

	/*
	 * ls = sun_position(j) 
	 */
	ms = 0.985647332099 * j - 3.762863;
	if (ms < 0)
		ms += 360.0;
	l = 13.176396 * j + 64.975464;
	i = l / 360;
	l = l - i * 360.0;
	if (l < 0)
		l += 360.0;
	mm = l - 0.1114041 * j - 349.383063;
	i = mm / 360;
	mm -= i * 360.0;
	n = 151.950429 - 0.0529539 * j;
	i = n / 360;
	n -= i * 360.0;
	ev = 1.2739 * sin((2 * (l - ls) - mm) * RAD);
	sms = sin(ms * RAD);
	ae = 0.1858 * sms;
	mm += ev - ae - 0.37 * sms;
	ec = 6.2886 * sin(mm * RAD);
	l += ev + ec - ae + 0.214 * sin(2 * mm * RAD);
	l = 0.6583 * sin(2 * (l - ls) * RAD) + l;
	return l;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  M O O N _ P H A S E                                                                                               *
 *  ===================                                                                                               *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Calculate the moon phase.
 *  \param year The year to calculate for.
 *  \param month The month to calculate for.
 *  \param day The day to calculate for.
 *  \param hour The hour to calculate for.
 *  \param ip Returned number for the phase.
 *  \result Moon phase 0 to 1.
 */
double moon_phase(int year, int month, int day, double hour, int *ip)
{
	/*
	 * Calculates more accurately than Moon_phase , the phase of the moon at
	 * the given epoch. returns the moon phase as a real number (0-1) 
	 */

	double j = Julian(year, month, (double)day + hour / 24.0) - 2444238.5;
	double ls = sun_position(j);
	double lm = moon_position(j, ls);

	double t = lm - ls;
	if (t < 0)
		t += 360;
	*ip = (int)((t + 22.5) / 45) & 0x7;
	return (1.0 - cos((lm - ls) * RAD)) / 2;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  R E A D  M O O N  P H A S E  V A L U E S                                                                          *
 *  ========================================                                                                          *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Read the state of the moon.
 *  \param face Which face is this for.
 *  \result None.
 */
void readMoonPhaseValues (int face)
{
	if (gaugeEnabled[FACE_TYPE_MOONPHASE].enabled)
	{
		FACE_SETTINGS *faceSetting = faceSettings[face];

		if (faceSetting -> faceFlags & FACE_REDRAW)
		{
			;
		}
		else if (sysUpdateID % 25 != 0)
		{
			return;
		}
		if (myUpdateID != sysUpdateID)
		{
			myUpdateID = sysUpdateID;
		}
		{
			char buff[81];
			struct tm result;
			time_t now = time(NULL);
			double p;
			int ip;

			localtime_r(&now, &result);
			p = moon_phase(result.tm_year + 1900, result.tm_mon + 1, result.tm_mday, result.tm_hour, &ip);

			if (lastRead == -1)
			{
				lastRead = p;
				change = 0;
			}
			else
			{
				if (p < lastRead)
				{
					change = -1;
					lastRead = p;
				}
				else if (p > lastRead)
				{
					change = 1;
					lastRead = p;
				}
			}

			p = floor(p * 1000 + 0.5) / 10;
			strncpy (buff, ip == 0 ? _("New") : ip == 4 ? _("Full") : ip < 4  ? _("Waxing") : _("Waning"), 80);

			faceSetting -> firstValue = p;
			setFaceString (faceSetting, FACESTR_TOP, 0, _("Moon\nPhase"));
			setFaceString (faceSetting, FACESTR_BOT, 0, _("%s\n(%0.0f%%)"), buff, p);
			if (change != 0 && (ip == 0 || ip == 4))
			{
				setFaceString (faceSetting, FACESTR_TIP, 0, _("<b>Moon Phase</b>: %s (%0.1f%%)\n<b>Last Change</b>: %s"),
						buff, p, (change == -1 ? _("Waning") : change == 1 ? _("Waxing") : _("None")));
			}
			else
			{
				setFaceString (faceSetting, FACESTR_TIP, 0, _("<b>Moon Phase</b>: %s (%0.1f%%)"), buff, p);
			}
			setFaceString (faceSetting, FACESTR_WIN, 0, _("Moon Phase: %s (%0.0f%%) - Gauge"), buff, p);
		}
	}
}


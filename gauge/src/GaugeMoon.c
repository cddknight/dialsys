/**********************************************************************************************************************
 *                                                                                                                    *
 *  G A U G E  B A T T E R Y . C                                                                                      *
 *  ============================                                                                                      *
 *                                                                                                                    *
 *  This is free software; you can redistribute it and/or modify it under the terms of the GNU General Public         *
 *  License version 2 as published by the Free Software Foundation.  Note that I am not granting permission to        *
 *  redistribute or modify this under the terms of any later version of the General Public License.                   *
 *                                                                                                                    *
 *  This is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the                *
 *  impliedwarranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for   *
 *  more details.                                                                                                     *
 *                                                                                                                    *
 *  You should have received a copy of the GNU General Public License along with this program (in the file            *
 *  "COPYING"); if not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111,   *
 *  USA.                                                                                                              *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  @file
 *  @brief Handle a gauge that shows battery.
 *  @version $Id: GaugeBattery.c 1856 2014-01-14 15:10:08Z ukchkn $
 */
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#define		PI	3.1415926535897932384626433832795
#define		RAD	(PI/180.0)
#define         SMALL_FLOAT	(1e-12)

typedef struct 
{
	int year, month, day;
	double hour;
} 
TimePlace;

#include "GaugeDisp.h"

extern FACE_SETTINGS *faceSettings[];
extern MENU_DESC gaugeMenuDesc[];
extern int sysUpdateID;

static int myUpdateID = 100;

/**********************************************************************************************************************
 *                                                                                                                    *
 *  R E A D  B A T T E R Y  I N I T                                                                                   *
 *  ===============================                                                                                   *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Called on program start to init the gauge.
 *  \result None.
 */
void readMoonPhaseInit (void)
{
	gaugeMenuDesc[MENU_GAUGE_MOONPHASE].disable = 0;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  J U L I A N  T O  D A T E                                                                                         *
 *  =========================                                                                                         *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief .
 *  \param now .
 *  \param jd .
 *  \result .
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
 *  \brief .
 *  \param year .
 *  \param month .
 *  \param day .
 *  \result .
 */
double Julian(int year, int month, double day)
{
	/*
	 * Returns the number of julian days for the specified day. 
	 */

	int a, b, c, e;
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
 *  \brief .
 *  \param j .
 *  \result .
 */
double sun_position(double j)
{
	double n, x, e, l, dl, v;
	double m2;
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
 *  \brief .
 *  \param j .
 *  \param ls .
 *  \result .
 */
double moon_position(double j, double ls)
{

	double ms, l, mm, n, ev, sms, z, x, lm, bm, ae, ec;
	double d;
	double ds, as, dm;
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
 *  \brief .
 *  \param year .
 *  \param month .
 *  \param day .
 *  \param hour .
 *  \param ip .
 *  \result .
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
 *  R E A D  B A T T E R Y  V A L U E S                                                                               *
 *  ===================================                                                                               *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Read the state of the battery.
 *  \param face Which face is this for.
 *  \result None.
 */
void readMoonPhaseValues (int face)
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
		struct tm result;
		time_t now = time(NULL);
		double p;
		int ip;

		localtime_r(&now, &result);
		p = moon_phase(result.tm_year + 1900, result.tm_mon + 1, result.tm_mday, result.tm_hour, &ip);
		p = floor(p * 1000 + 0.5) / 10;

		faceSetting -> firstValue = p;
		setFaceString (faceSetting, FACESTR_TOP, 0, _("Moon\nPhase"));
		setFaceString (faceSetting, FACESTR_BOT, 0, _("%s\n(%0.1f%%)"),
				ip == 0 ? _("New") : ip == 4 ? _("Full") : ip < 4 ? _("Waxing") : _("Waning"), p);
		setFaceString (faceSetting, FACESTR_TIP, 0, _("<b>Moon Phase</b>: %s (%0.1f%%)"),
				ip == 0 ? _("New") : ip == 4 ? _("Full") : ip < 4 ? _("Waxing") : _("Waning"), p);
		setFaceString (faceSetting, FACESTR_WIN, 0, _("Moon Phase: %s (%0.1f%%) - Gauge"),
				ip == 0 ? _("New") : ip == 4 ? _("Full") : ip < 4 ? _("Waxing") : _("Waning"), p);
	}
/*	faceSetting -> firstValue = atoi (batteryValues[2]) * 100;
	faceSetting -> firstValue /= atoi (batteryValues[1]);
	setFaceString (faceSetting, FACESTR_TOP, 0, _("Battery\n(%s)"), gettext (state));
	setFaceString (faceSetting, FACESTR_TIP, 0, _("<b>Battery</b>: %0.1f%% Full, %s"), 
			faceSetting -> firstValue, gettext (state));
	setFaceString (faceSetting, FACESTR_WIN, 0, _("Battery: %0.1f%% Full - Gauge"), 
			faceSetting -> firstValue);
	setFaceString (faceSetting, FACESTR_BOT, 0, _("%0.1f%%"), 
			faceSetting -> firstValue); */
}


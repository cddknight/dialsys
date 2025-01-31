/**********************************************************************************************************************
 *                                                                                                                    *
 *  T Z  C L O C K . C                                                                                                *
 *  ==================                                                                                                *
 *                                                                                                                    *
 *  Copyright (c) 2023 Chris Knight                                                                                   *
 *                                                                                                                    *
 *  File TzClock.c part of TzClock is free software: you can redistribute it and/or modify it under the terms of the  *
 *  GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at  *
 *  your option) any later version.                                                                                   *
 *                                                                                                                    *
 *  TzClock is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied     *
 *  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more     *
 *  details.                                                                                                          *
 *                                                                                                                    *
 *  You should have received a copy of the GNU General Public License along with this program. If not, see:           *
 *  <http://www.gnu.org/licenses/>                                                                                    *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \file
 *  \brief Main clock sources.
 */
#include "config.h"
#include "buildDate.h"
#include "TzClockDisp.h"
#include "ParseZone.h"

COLOUR_DETAILS colourNames[MAX__COLOURS + 1] =
{
	{	"blk", __("Black"),					"#000000"	},	/* Z:00 */
	{	"wht", __("White"),					"#FFFFFF"	},	/* Z:01 */

	{	"fce", __("Main face colour"),		"#141414"	},	/* Z:02 */
	{	"bri", __("Border inner circle"),	"#000000"	},	/* Z:03 */
	{	"brf", __("Border when focused"),	"#505050"	},	/* Z:04 */
	{	"brn", __("Border not focused"),	"#6E6E6E"	},	/* Z:05 */
	{	"txt", __("Information text"),		"#858585"	},	/* Z:06 */
	{	"qum", __("Quarter markers"),		"#AAAAAA"	},	/* Z:07 */
	{	"quf", __("Quarter markers fill"),	"#646464"	},	/* Z:08 */
	{	"hom", __("Hour markers"),			"#646464"	},	/* Z:09 */

	{	"fsw", __("Stopwatch dial"),		"#000000"	},	/* Z:10 */
	{	"hoh", __("Hour hand outer"),		"#E1E1E1"	},	/* Z:11 */
	{	"hof", __("Hour hand fill"),		"#202020"	},	/* Z:12 */
	{	"mih", __("Minute hand outer"),		"#E1E1E1"	},	/* Z:13 */
	{	"mif", __("Minute hand fill"),		"#202020"	},	/* Z:14 */
	{	"seh", __("Second hand outer"),		"#FF0000"	},	/* Z:15 */
	{	"sef", __("Second hand fill"),		"#800000"	},	/* Z:16 */
	{	"alh", __("Alarm hand outer"),		"#008000"	},	/* Z:17 */
	{	"alf", __("Alarm hand fill"),		"#004000"	},	/* Z:18 */
	{	"sth", __("Stopwatch hand outer"),	"#FF0000"	},	/* Z:19 */
	{	"stf", __("Stopwatch hand fill"),	"#800000"	},	/* Z:20 */
	{	"mim", __("Minute markers"),		"#3C3C3C"	},	/* Z:21 */
	{	"stm", __("Stopwatch markers"),		"#828282"	},	/* Z:22 */
	{	NULL, NULL, ""	}
};

char *handNames[HAND_COUNT] =
{
	"hour", "minute", "second", "sub_second", "alarm",
	"stop_tenths", "stop_secs", "stop_mins"
};

char *nameFormats[TXT_COUNT] =
{
	"ftl", "ftz", "stl", "stz", "fbl", "fbz", "sbl", "sbz",
	"wtl", "wtz", "cbl", "cbz", "cdl", "cdz", "ctl", "ctz",
	"ttl", "ttz", "ssl", "ssr", "tsl", "tsr", "ssb"
};

char displayString[TXT_COUNT][101] =
{
	"%a. %e %b.",					/* Clock top        (localtime) */ /* Y:00 */
	"%#",							/* Clock top        (timezone)  */ /* Y:01 */
	"%e %b.%n%a.",					/* Clock SW top     (localtime) */ /* Y:02 */
	"%#%n",							/* Clock SW top     (timezone)  */ /* Y:03 */
	"- %Z -",						/* Clock bottom     (localtime) */ /* Y:04 */
	"- %p -",						/* Clock bottom     (timezone)  */ /* Y:05 */
	"%Z",							/* Clock SW bottom  (localtime) */ /* Y:06 */
	"%p",							/* Clock SW bottom  (timezone)  */ /* Y:07 */
	"%R, %A %e %B",					/* Window title     (localtime) */ /* Y:08 */
	"%*, %R, %A %e %B",				/* Window title     (timezone)  */ /* Y:09 */
	"%A, %e %B %Y, %X",				/* Copy date time   (localtime) */ /* Y:10 */
	"%A, %e %B %Y, %X, %*",			/* Copy date time   (timezone)  */ /* Y:11 */
	"%A, %e %B %Y",					/* Copy date        (localtime) */ /* Y:12 */
	"%A, %e %B %Y, %*",				/* Copy date        (timezone)  */ /* Y:13 */
	"%X",							/* Copy time        (localtime) */ /* Y:14 */
	"%X, %Z",						/* Copy time        (timezone)  */ /* Y:15 */
	"<b>Time</b>: %X%n"
	"<b>Date</b>: %A, %e %B%n"
	"<b>Timer</b>: %&%n"
	"<b>Alarm</b>: %$",				/* Tool tip         (localtime) */ /* Y:16 */
	"<b>City</b>: %*%n"
	"<b>Time</b>: %X%n"
	"<b>Date</b>: %A, %e %B",		/* Tool tip         (timezone)  */ /* Y:17 */
	"1Sec",							/* Left stopwatch               */ /* Y:18 */
	"30Min",						/* Right stopwatch              */ /* Y:19 */
	"6Hour",						/* Left timer                   */ /* Y:20 */
	"30Min",						/* Right timer                  */ /* Y:21 */
	"1Min"							/* Sub second                   */ /* Y:22 */
};

/*----------------------------------------------------------------------------------------------------*
 * If we cannot find a stock clock icon then use this built in one.                                   *
 *----------------------------------------------------------------------------------------------------*/
#include "TzClockIcon.xpm"

/*----------------------------------------------------------------------------------------------------*
 * Who dunit!                                                                                         *
 *----------------------------------------------------------------------------------------------------*/
const gchar *authors[] =
{
	"Chris Knight <chris@theknight.co.uk>", NULL
};

const gchar *artists[] =
{
	"Annie Knight", "Sonya Knight", NULL
};

int nTimeZones;
TZ_INFO *timeZones;

/*----------------------------------------------------------------------------------------------------*
 * Menu parameters are:                                                                               *
 * menuName, funcCallBack, subMenuDesc, param, stockItem, accelKey, disable, checkbox, checked        *
 *----------------------------------------------------------------------------------------------------*/
MENU_DESC *timeZoneMenu;

MENU_DESC stopWMenuDesc[] =
{
	{	__("Enable"),			stopwatchCallback,		NULL,				0,	NULL,	0,	0,	1	},	/* 1:00 */
	{	__("Start+Stop"),		swStartCallback,		NULL,				0,	NULL,	GDK_KEY_A	},	/* 1:01 */
	{	__("Reset"),			swResetCallback,		NULL,				0,	NULL,	GDK_KEY_Z	},	/* 1:02 */
	{	NULL,					NULL,					NULL,				0	}
};

MENU_DESC timerMenuDesc[] =
{
	{	__("Enable"),			timerCallback,			NULL,				0,	NULL,	0,	0,	1	},	/* 2:00 */
	{	__("Start+Stop"),		tmStartCallback,		NULL,				0,	NULL,	GDK_KEY_D	},	/* 2:01 */
	{	__("Reset"),			tmResetCallback,		NULL,				0,	NULL,	GDK_KEY_X},		/* 2:02 */
	{	__("Set Up Timer..."),	timerSetCallback,		NULL,				0	},
	{	NULL,					NULL,					NULL,				0	}
};

MENU_DESC alarmMenuDesc[] =
{
	{	__("Enable"),			alarmCallback,			NULL,				0,	NULL,	0,	0,	1	},	/* 3:00 */
	{	__("Set Up Alarm..."),	alarmSetCallback,		NULL,				0	},						/* 3:01 */
	{	NULL,					NULL,					NULL,				0	}
};

MENU_DESC editMenuDesc[] =
{
#if GTK_MINOR_VERSION >= 10
	{	__("Copy Date & Time"), copyCallback,			NULL,				0,	NULL,	GDK_KEY_C	},	/* 4:00 */
#else
	{	__("Copy Date & Time"), copyCallback,			NULL,				0,	GTK_STOCK_COPY	},		/* 5:00 */
#endif
	{	__("Copy Date"),		copyCallback,			NULL,				1	},						/* 5:01 */
	{	__("Copy Time"),		copyCallback,			NULL,				2	},						/* 5:02 */
	{	NULL,					NULL,					NULL,				0	}						/* 5:03 */
};

MENU_DESC viewMenuDesc[] =
{
	{	__("Add Column"),		dialAddDelCallback,		NULL,				1	},			/* 6:00 */
	{	__("Remove Column"),	dialAddDelCallback,		NULL,				2	},			/* 6:01 */
	{	__("Add Row"),			dialAddDelCallback,		NULL,				3	},			/* 6:02 */
	{	__("Remove Row"),		dialAddDelCallback,		NULL,				4	},			/* 6:03 */
	{	"-",					NULL,					NULL,				0	},			/* 6:04 */
#if GTK_MINOR_VERSION >= 10
	{	__("Zoom In"),			dialZoomCallback,		NULL,				1,	NULL,	GDK_KEY_I	},	/* 6:05 */
	{	__("Zoom Out"),			dialZoomCallback,		NULL,				2,	NULL,	GDK_KEY_O	},	/* 6:06 */
	{	__("Zoom Max"),			dialZoomCallback,		NULL,				3,	NULL,	GDK_KEY_X	},	/* 6:07 */
#else
	{	__("Zoom In"),			dialZoomCallback,		NULL,				1,	GTK_STOCK_ZOOM_IN	},	/* 7:05 */
	{	__("Zoom Out"),			dialZoomCallback,		NULL,				2,	GTK_STOCK_ZOOM_OUT	},	/* 7:06 */
	{	__("Zoom Max"),			dialZoomCallback,		NULL,				3,	GTK_STOCK_ZOOM_FIT	},	/* 7:07 */
#endif
	{	NULL,					NULL,					NULL,				0	}
};

MENU_DESC markerMenuDesc[] =
{
	{	__("No Markers"),		dialMarkerCallback,		NULL,				0,		NULL,	0,	0,	1	},	/* 8:00 */
	{	__("Triangles"),		dialMarkerCallback,		NULL,				1,		NULL,	0,	0,	1	},	/* 8:01 */
	{	__("Circles"),			dialMarkerCallback,		NULL,				2,		NULL,	0,	0,	1	},	/* 8:02 */
	{	__("Numbers"),			dialMarkerCallback,		NULL,				3,		NULL,	0,	0,	1	},	/* 8:03 */
	{	__("Roman"),			dialMarkerCallback,		NULL,				4,		NULL,	0,	0,	1	},	/* 8:04 */
	{	"-",					NULL,					NULL,				0		},						/* 8:05 */
	{	__("Step 1"),			dialStepCallback,		NULL,				100,	NULL,	0,	0,	1	},	/* 8:06 */
	{	__("Step 2"),			dialStepCallback,		NULL,				200,	NULL,	0,	0,	1	},	/* 8:07 */
	{	__("Step 3"),			dialStepCallback,		NULL,				300,	NULL,	0,	0,	1	},	/* 8:08 */
	{	__("Step 4"),			dialStepCallback,		NULL,				400,	NULL,	0,	0,	1	},	/* 8:09 */
	{	__("Step 6"),			dialStepCallback,		NULL,				600,	NULL,	0,	0,	1	},	/* 8:10 */
	{	__("Step 12"),			dialStepCallback,		NULL,				1200,	NULL,	0,	0,	1	},	/* 8:11 */
	{	NULL,					NULL,					NULL,				0		}
};

MENU_DESC prefMenuDesc[] =
{
	{	__("Always on Top"),	onTopCallback,			NULL,				1,	NULL,	0,	0,	1	},	/* 9:00 */
	{	__("Always Visible"),	stickCallback,			NULL,				1,	NULL,	0,	0,	1	},	/* 9:01 */
	{	__("Lock Position"),	lockCallback,			NULL,				1,	NULL,	0,	1,	1	},	/* 9:02 */
	{	__("Show Time"),		showTimeCallback,		NULL,				1,	NULL,	0,	0,	1	},	/* 9:03 */
	{	__("Show Seconds"),		showSecondsCallback,	NULL,				0,	NULL,	0,	0,	1	},	/* 9:04 */
	{	__("Sub Seconds"),		subSecondCallback,		NULL,				0,	NULL,	0,	0,	1	},	/* 9:05 */
	{	"-",					NULL,					NULL,				0	},			/* 9:06 */
	{	__("Markers"),			NULL,					markerMenuDesc,		0	},			/* 9:07 */
	{	__("View"),				NULL,					viewMenuDesc,		0	},			/* 9:08 */
	{	__("Change Font..."),	dialFontCallback,		NULL,				0	},			/* 9:09 */
	{	__("Change Colour..."),	dialColourCallback,		NULL,				0	},			/* 9:10 */
	{	"-",					NULL,					NULL,				0	},			/* 9:11 */
#if GTK_MINOR_VERSION >= 10
	{	__("Save Preferences"), configSaveCallback,		NULL,				0,	NULL,	GDK_KEY_S	},	/* 9:12 */
#else
	{	__("Save Preferences"), configSaveCallback,		NULL,				0,	GTK_STOCK_SAVE	},		/* A:12 */
#endif
	{	__("Save Display..."),	dialSaveCallback,		NULL,				0,	NULL,	0,	1	},		/* A:13 */
	{	NULL,					NULL,					NULL,				0	}
};

MENU_DESC mainMenuDesc[] =
{
	{	__("Time-zone"),		NULL,					NULL,				0	},						/* B:00 */
	{	__("Alarm"),			NULL,					alarmMenuDesc,		0	},						/* B:01 */
	{	__("Stopwatch"),		NULL,					stopWMenuDesc,		0	},						/* B:02 */
	{	__("Timer"),			NULL,					timerMenuDesc,		0	},						/* B:03 */
	{	__("Edit"),				NULL,					editMenuDesc,		0	},						/* B:04 */
	{	__("Calendar"),			calendarCallback,		NULL,				0	},						/* B:05 */
	{	"-",					NULL,					NULL,				0	},						/* B:06 */
	{	__("Preferences"),		NULL,					prefMenuDesc,		0	},						/* B:07 */
#if GTK_MINOR_VERSION >= 10
	{	__("About"),			aboutCallback,			NULL,				0	},						/* B:08 */
	{	__("Quit"),				quitCallback,			NULL,				0,	NULL,	GDK_KEY_Q},		/* B:09 */
#else
	{	__("About"),			aboutCallback,			NULL,				0,	GTK_STOCK_ABOUT },		/* C:08 */
	{	__("Quit"),				quitCallback,			NULL,				0,	GTK_STOCK_QUIT	},		/* C:09 */
#endif
	{	NULL,					NULL,					NULL,				0	}
};

/*----------------------------------------------------------------------------------------------------*
 *                                                                                                    *
 *----------------------------------------------------------------------------------------------------*/
static GdkPixbuf *defaultIcon;
void updateClock (void);

/*----------------------------------------------------------------------------------------------------*
 *                                                                                                    *
 *----------------------------------------------------------------------------------------------------*/
static bool alwaysOnTop			=  FALSE;			/* Saved in the config file */
static bool stuckOnAll			=  FALSE;			/* Saved in the config file */
static bool lockMove			=  FALSE;			/* Saved in the config file */
static int lastKeyPressTime		=  0;
static int keyPressFaceNum		= -1;
static int stopwatchActive		=  0;
static time_t lastTime			= -1;
static int bounceSec			=  0;

/*----------------------------------------------------------------------------------------------------*
 *                                                                                                    *
 *----------------------------------------------------------------------------------------------------*/
CLOCK_INST clockInst =
{
	NULL,							/* accelGroup */
	FALSE,							/* fastSetting */
	FALSE,							/* showBounceSec */
	FALSE,							/* clockDecorated */
	FALSE,							/* showSubText */
	0,								/* weHaveFocus */
	0,								/* currentFace */
	0,								/* toolTipFace */
	0,								/* timeSetting */
	0,								/* allowSaveDisp */
	0,								/* removeTaskbar */
	-1,								/* forceTime */
	0,								/* reConfigTime */
	"Sans",							// fontName
	".tzclockrc",					// configFile
	"",								// windowTitle
	"",								// windowToolTip
	{								/* -- Used by dial library -- */
		NULL,						/* mainWindow */
		NULL,						/* drawingArea */
		3 * 64,						/* faceSize */
		1,							/* faceWidth */
		1,							/* faceHeight */
		3,							/* markerType */
		300,						/* markerStep */
		99,							/* faceOpacity */
		0,							/* faceGradient */
		0,							/* startPoint */
		8,							/* markerScale */
		&clockInst.fontName[0],		/* Font name pointer */
		updateClock,				/* Update func. */
		dialSave,					/* Save func. */
		&colourNames[0]				/* Colour details */
	}
};

HAND_STYLE handStyle[HAND_COUNT] =					/* Saved in the config file */
{
	{ 0, 19, 5, HOUR__COLOUR, HFILL_COLOUR, 1, 0 }, /* Hour hand */
	{ 0, 28, 7, MIN___COLOUR, MFILL_COLOUR, 1, 0 }, /* Minute hand */
	{ 9, 28, 9, SEC___COLOUR, SFILL_COLOUR, 1, 0 }, /* Second hand */
	{ 9,  7, 0, SEC___COLOUR, SFILL_COLOUR, 1, 0 }, /* Sub-second hand */
	{ 9, 19, 0, ALARM_COLOUR, AFILL_COLOUR, 1, 0 }, /* Alarm hand */
	{ 9,  7, 0, WATCH_COLOUR, WFILL_COLOUR, 1, 0 }, /* Stopwatch hands */
	{ 9,  7, 0, WATCH_COLOUR, WFILL_COLOUR, 1, 0 }, /* Stopwatch hands */
	{ 9,  7, 0, WATCH_COLOUR, WFILL_COLOUR, 1, 0 }	/* Stopwatch hands */
};

/*----------------------------------------------------------------------------------------------------*
 * Prototypes for functions in the tables that are defined later.                                     *
 *----------------------------------------------------------------------------------------------------*/
static void splitTimeZone			(char *timeZone, char *area, char *city, char *display, int doUpper);
static void processCommandLine		(int argc, char *argv[], int *posX, int *posY);
static void howTo					(FILE * outFile, char *format, ...);
static void checkForAlarm			(FACE_SETTINGS *faceSetting, struct tm *tm);
static void checkForTimer			(FACE_SETTINGS *faceSetting);
static void prepareForPopup 		(void);

static gboolean clockTickCallback	(gpointer data);
static gboolean windowClickCallback (GtkWidget * widget, GdkEventButton * event);
static gboolean windowKeyCallback	(GtkWidget * widget, GdkEventKey * event);
static gboolean focusInEvent		(GtkWidget *widget, GdkEventFocus *event, gpointer data);
static gboolean focusOutEvent		(GtkWidget *widget, GdkEventFocus *event, gpointer data);
static gboolean drawCallback		(GtkWidget *widget, cairo_t *cr, gpointer data);

/**********************************************************************************************************************
 *                                                                                                                    *
 *  H O W  T O                                                                                                        *
 *  ==========                                                                                                        *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief How to use the clock.
 *  \param outFile Where to send the output.
 *  \param format Used to decode the application name.
 *  \param ... Extra parameters.
 *  \result None.
 */
void
howTo (FILE * outFile, char *format, ...)
{
	int i;
	va_list ap;

	fprintf (outFile, "----------------------------------------------------------------------------\n");
	fprintf (outFile, _("The Clock %s\n"), VERSION);
	fprintf (outFile, "----------------------------------------------------------------------------\n");
	fprintf (outFile, _("How to use: TzClock [options...]\n\n"));
	fprintf (outFile, _("   -a              :  Toggle always on top\n"));
	fprintf (outFile, _("   -Ahh:mm:msg     :* Alarm message shown at a specified time\n"));
	fprintf (outFile, _("   -b              :* Toggle showing the small second hand\n"));
	fprintf (outFile, _("   -B              :  Toggle showing the second hand bounce\n"));
	fprintf (outFile, _("   -cnnn#RRGGBB    :  Change one of the clock colours\n"));
	fprintf (outFile, _("   -C<file>        :  Specify the configuration file to use\n"));
	fprintf (outFile, _("   -dnnn:format    :  Change one of the date formats\n"));
	fprintf (outFile, _("   -f<face>        :  Select the face, for setting timezone\n"));
	fprintf (outFile, _("   -F<font>        :  Select the font to use on the clock face\n"));
	fprintf (outFile, _("   -g<gradient>    :  Select the amount of gradient on the face\n"));
	fprintf (outFile, _("   -h              :* Toggle showing the second hand\n"));
	fprintf (outFile, _("   -H<name>:s:l:t  :  Set the hands style, length and tail size\n"));
	fprintf (outFile, _("   -l              :  Toggle locking the screen position\n"));
	fprintf (outFile, _("   -m<type><step>  :  Set the marker type and step, default -m13\n"));
	fprintf (outFile, _("                   :  0=none, 1=Triangle, 2=Circle, 3=Latin, 4=Roman\n"));
	fprintf (outFile, _("   -n<c|r><num>    :  Set the number of columns and rows\n"));
	fprintf (outFile, _("                   :  Max clocks %d, no more than 10 in a line\n"), MAX_FACES);
	fprintf (outFile, _("   -o<city>        :  Specify you own city name for a timezone\n"));
	fprintf (outFile, _("   -O<opacity>     :  Change the opacity, 0 clear to 100 solid\n"));
	fprintf (outFile, _("   -q              :  Toggle quick time setting, no smooth scroll\n"));
	fprintf (outFile, _("   -s<size>        :  Set the size of each clock\n"));
	fprintf (outFile, _("   -S              :* Toggle enabling the stopwatch\n"));
	fprintf (outFile, _("   -Thh:mm:ss:msg  :* Toggle enabling the timer\n"));
	fprintf (outFile, _("   -t              :  Toggle removing the clock from the taskbar\n"));
	fprintf (outFile, _("   -u              :* Toggle upper-casing the city name\n"));
	fprintf (outFile, _("   -U              :  Toggle text on the sub dials\n"));
	fprintf (outFile, _("   -w              :  Toggle showing on all the desktops\n"));
	fprintf (outFile, _("   -x<posn>        :  Set the X screen position\n"));
	fprintf (outFile, _("   -y<posn>        :  Set the Y screen position\n"));
	fprintf (outFile, _("                   :  Both X and Y must be set\n"));
	fprintf (outFile, _("   -z<zone>        :* Select the timezone to display\n"));
	fprintf (outFile, _("   -24             :* Toggle showing the 24 hour clock\n"));
	fprintf (outFile, _("   -?              :  This how to information\n\n"));
	fprintf (outFile, _("Options marked with '*' only effect the current face. Use\n"));
	fprintf (outFile, _("the -f<num> option to select the current face.\n"));
	fprintf (outFile, "----------------------------------------------------------------------------\n");
	fprintf (outFile, _("Colour codes: -cnnn#RRGGBB  (nnn Colour name)\n\n"));

	for (i = 2; i < MAX__COLOURS; i++)
		fprintf (outFile, "   %s : %s\n", colourNames[i].shortName, gettext (colourNames[i].longName));
	fprintf (outFile, "----------------------------------------------------------------------------\n");
	fprintf (outFile, _("Date format strings: -dnnn:format (nnn Format name)\n\n"));

	fprintf (outFile, _("   %s : Clock top in local time\n"),			nameFormats[0]);
	fprintf (outFile, _("   %s : Clock top for time zone\n"),			nameFormats[1]);
	fprintf (outFile, _("   %s : Stopwatch top in local time\n"),		nameFormats[2]);
	fprintf (outFile, _("   %s : Stopwatch top for time zone\n"),		nameFormats[3]);
	fprintf (outFile, _("   %s : Clock bottom in local time\n"),		nameFormats[4]);
	fprintf (outFile, _("   %s : Clock bottom for time zone\n"),		nameFormats[5]);
	fprintf (outFile, _("   %s : Stopwatch bottom in local time\n"),	nameFormats[6]);
	fprintf (outFile, _("   %s : Stopwatch bottom for time zone\n"),	nameFormats[7]);
	fprintf (outFile, _("   %s : Window title in local time\n"),		nameFormats[8]);
	fprintf (outFile, _("   %s : Window title for time zone\n"),		nameFormats[9]);
	fprintf (outFile, _("   %s : Copy date time in local time\n"),		nameFormats[10]);
	fprintf (outFile, _("   %s : Copy date time for time zone\n"),		nameFormats[11]);
	fprintf (outFile, _("   %s : Copy date in local time\n"),			nameFormats[12]);
	fprintf (outFile, _("   %s : Copy date for time zone\n"),			nameFormats[13]);
	fprintf (outFile, _("   %s : Copy time in local time\n"),			nameFormats[14]);
	fprintf (outFile, _("   %s : Copy time for time zone\n"),			nameFormats[15]);
	fprintf (outFile, _("   %s : Tool tip text for local time\n"),		nameFormats[16]);
	fprintf (outFile, _("   %s : Tool tip text for time zone\n"),		nameFormats[17]);
	fprintf (outFile, _("   %s : Stopwatch left face scale\n"),			nameFormats[18]);
	fprintf (outFile, _("   %s : Stopwatch right face scale\n"),		nameFormats[19]);
	fprintf (outFile, _("   %s : Timer left face scale\n"),				nameFormats[20]);
	fprintf (outFile, _("   %s : Timer right face scale\n"),			nameFormats[21]);
	fprintf (outFile, _("   %s : Subsecond face scale\n"),				nameFormats[22]);

	fprintf (outFile, "----------------------------------------------------------------------------\n");
	fprintf (outFile, _("Date format options:\n\n"));

	fprintf (outFile, _("   %%#   : Time zone city, upper-cased and wrapped\n"));
	fprintf (outFile, _("   %%*   : Time zone city\n"));
	fprintf (outFile, _("   %%@   : Time zone area\n"));
	fprintf (outFile, _("   %%&   : Stopwatch: h:mm:ss.hh\n"));
	fprintf (outFile, _("   %%&   : Timer left: h:mm:ss\n"));
	fprintf (outFile, _("   %%... : See man page for the date command\n"));

	fprintf (outFile, "----------------------------------------------------------------------------\n");
	if (format)
	{
		va_start(ap, format);
		vfprintf(outFile, format, ap);
		va_end(ap);
		fprintf (outFile, "----------------------------------------------------------------------------\n");
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S P L I T  T I M E  Z O N E                                                                                       *
 *  ===========================                                                                                       *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Split the time in to it's parts.
 *  \param timeZone Timezone to split.
 *  \param area Area name.
 *  \param city City name.
 *  \param display String to display.
 *  \param doUpper Show display be uppercase.
 *  \result None.
 */
static void
splitTimeZone (char *timeZone, char *area, char *city, char *display, int doUpper)
{
	int i = 0, j = 0, w = 0, newLine = 0, wordSize = 0, addCR = 0;
	char inChar;

	if (area)
		area[j] = 0;

	while (timeZone[i])
	{
		inChar = timeZone[i++];

		if (inChar == '_')
		{
			if (j >= 3 && newLine == 0 && w == 1 && wordSize >= 10)
			{
				addCR = 1;
				newLine = 1;
			}
			inChar = ' ';
		}
		if (inChar == '/')
		{
			w = 1;
			j = 0;

			wordSize = strlen (&timeZone[i]);
			if (city)
				city [j] = 0;
			if (display)
				display[j] = 0;
		}
		else if (w == 0)
		{
			if (area)
			{
				area[j++] = inChar;
				area[j] = 0;
			}
		}
		else
		{
			if (city)
			{
				city[j] = inChar;
				city[j + 1] = 0;
			}
			if (display)
			{
				char s = addCR ? '\n' : inChar;
				display[j] = doUpper ? toupper (s) : s;
				display[j + 1] = 0;
			}
			j++;
		}
		addCR = 0;
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  Q U I T  C A L L B A C K                                                                                          *
 *  ========================                                                                                          *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Called when the app is asked to quit.
 *  \param data Not used.
 *  \result None.
 */
void
quitCallback (guint data)
{
/*	g_application_quit (G_APPLICATION (app)); */
	gtk_main_quit ();
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  C O P Y  C A L L B A C K                                                                                          *
 *  ========================                                                                                          *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Copy to the clipboard.
 *  \param data Not used.
 *  \result None.
 */
void
copyCallback (guint data)
{
	time_t t;
	char stringDate[201];
	GtkClipboard* clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
	int timeZone = clockInst.faceSettings[clockInst.currentFace] -> currentTZ;

	t = time (NULL);
	if (timeZones[timeZone].value == 0)
	{
		unsetenv ("TZ");
	}
	else
	{
		setenv ("TZ", timeZones[timeZone].envName, 1);
	}
	localtime (&t);

	switch (data)
	{
	case 0:
		getStringValue (stringDate, 200, timeZone ? TXT_COPY_DT_Z : TXT_COPY_DT_L, clockInst.currentFace, t);
		break;

	case 1:
		getStringValue (stringDate, 200, timeZone ? TXT_COPY_D_Z : TXT_COPY_D_L, clockInst.currentFace, t);
		break;

	case 2:
		getStringValue (stringDate, 200, timeZone ? TXT_COPY_T_Z : TXT_COPY_T_L, clockInst.currentFace, t);
		break;
	}
	gtk_clipboard_set_text (clipboard, stringDate, -1);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S E T  T I M E  Z O N E  C A L L B A C K                                                                          *
 *  ========================================                                                                          *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Set the timezone to use.
 *  \param data Which timezone to set.
 *  \result None.
 */
void
setTimeZoneCallback (guint data)
{
	char value[81];
	int timeZone = (int) data;

	clockInst.faceSettings[clockInst.currentFace] -> currentTZ = timeZones[timeZone].value;
	splitTimeZone (timeZones[timeZone].envName, clockInst.faceSettings[clockInst.currentFace] -> currentTZArea,
			clockInst.faceSettings[clockInst.currentFace] -> currentTZCity, clockInst.faceSettings[clockInst.currentFace] -> currentTZDisp,
			clockInst.faceSettings[clockInst.currentFace] -> upperCity);

	sprintf (value, "timezone_city_%d", clockInst.currentFace + 1);
	configSetValue (value, clockInst.faceSettings[clockInst.currentFace] -> currentTZCity);
	lastTime = -1;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  O N  T O P  C A L L B A C K                                                                                       *
 *  ===========================                                                                                       *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Set the on top flag.
 *  \param data Is this from the menu.
 *  \result None.
 */
void
onTopCallback (guint data)
{
	if (data)
	{
		alwaysOnTop = !alwaysOnTop;
		configSetBoolValue ("always_on_top", alwaysOnTop);
	}
	gtk_window_set_keep_above (GTK_WINDOW (clockInst.dialConfig.mainWindow), alwaysOnTop);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S T I C K  C A L L B A C K                                                                                        *
 *  ==========================                                                                                        *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Set the stick flage.
 *  \param data Is this from the menu.
 *  \result None.
 */
void
stickCallback (guint data)
{
	if (data)
	{
		stuckOnAll = !stuckOnAll;
		configSetBoolValue ("on_all_desktops", stuckOnAll);
	}
	if (stuckOnAll)
		gtk_window_stick (GTK_WINDOW (clockInst.dialConfig.mainWindow));
	else
		gtk_window_unstick (GTK_WINDOW (clockInst.dialConfig.mainWindow));
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  L O C K  C A L L B A C K                                                                                          *
 *  ========================                                                                                          *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Set the lock flag.
 *  \param data Is this from the menu.
 *  \result None.
 */
void
lockCallback (guint data)
{
	prefMenuDesc[MENU_PREF_LOCK].disable = 0;
	if (data)
	{
		lockMove = !lockMove;
		configSetBoolValue ("locked_position", lockMove);
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  C A L E N D A R  C A L L B A C K                                                                                  *
 *  ================================                                                                                  *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Display a calendar.
 *  \param data Not used.
 *  \result None.
 */
void
calendarCallback (guint data)
{
	time_t t;
	GtkWidget *dialog;
	GtkWidget *contentArea;
	GtkWidget *vbox;

	int timeZone = clockInst.faceSettings[clockInst.currentFace] -> currentTZ;

	t = time (NULL);
	if (timeZones[timeZone].value == 0)
	{
		unsetenv ("TZ");
	}
	else
	{
		setenv ("TZ", timeZones[timeZone].envName, 1);
	}
	localtime (&t);

	dialog = gtk_dialog_new_with_buttons (_("Clock Calendar"), GTK_WINDOW(clockInst.dialConfig.mainWindow),
						GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
#if GTK_MINOR_VERSION >= 10
						_("Close"),
#else
						GTK_STOCK_CLOSE,
#endif
						GTK_RESPONSE_NONE, NULL);

	contentArea = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 3);
	gtk_box_pack_start (GTK_BOX (contentArea), vbox, TRUE, TRUE, 0);
	gtk_container_add (GTK_CONTAINER (vbox), gtk_calendar_new ());
	gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
	gtk_widget_show_all (dialog);
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  A L A R M  S E T  A N G L E                                                                                       *
 *  ===========================                                                                                       *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Set the angle of the alarm hand on a given face.
 *  \param face Face the calculate the alarm angle for.
 *  \result None.
 */
void alarmSetAngle (int face)
{
	short angle = clockInst.faceSettings[face] -> show24Hour ?
			(clockInst.faceSettings[face] -> alarmInfo.alarmHour * 50)	+ ((clockInst.faceSettings[face] -> alarmInfo.alarmMin * 60) / 72):
			(clockInst.faceSettings[face] -> alarmInfo.alarmHour * 100) + ((clockInst.faceSettings[face] -> alarmInfo.alarmMin * 60) / 36);
	clockInst.faceSettings[face] -> handPosition[HAND_ALARM] = angle;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  A L A R M  C A L L B A C K                                                                                        *
 *  ==========================                                                                                        *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Called back to enable and disable the alarm..
 *  \param data Not used.
 *  \result None.
 */
void
alarmCallback (guint data)
{
	char value[81];
	int newVal = !clockInst.faceSettings[clockInst.currentFace] -> alarm;

	clockInst.faceSettings[clockInst.currentFace] -> alarm = newVal;
	sprintf (value, "alarm_%d", clockInst.currentFace + 1);
	configSetBoolValue (value, newVal);
	lastTime = -1;
	prepareForPopup ();
	createMenu (mainMenuDesc, clockInst.accelGroup, FALSE);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  A L A R M  S E T  C A L L B A C K                                                                                 *
 *  =================================                                                                                 *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Set the alarm time.
 *  \param data Not used.
 *  \result None.
 */
void
alarmSetCallback (guint data)
{
	char value[81];
	GtkWidget *dialog;
	GtkWidget *entry1, *entry2;
	GtkWidget *label, *check;
	GtkWidget *spinner1, *spinner2;
	GtkWidget *hbox, *vbox1, *vbox2;
	GtkAdjustment *adj;
	GtkWidget *contentArea;

	/*------------------------------------------------------------------------------------------------*
	 * Create the basic dialog box                                                                    *
	 *------------------------------------------------------------------------------------------------*/
	dialog = gtk_dialog_new_with_buttons (_("Set Up Alarm"), GTK_WINDOW(clockInst.dialConfig.mainWindow),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
#if GTK_MINOR_VERSION >= 10
			_("Close"),
#else
			GTK_STOCK_CLOSE,
#endif
			GTK_RESPONSE_ACCEPT, NULL);

	contentArea = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
	vbox1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_pack_start (GTK_BOX (contentArea), vbox1, FALSE, TRUE, 0);
	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (vbox1), hbox, FALSE, TRUE, 0);

	/*------------------------------------------------------------------------------------------------*
	 * Add the hour spinner                                                                           *
	 *------------------------------------------------------------------------------------------------*/
	label = gtk_label_new (_("Hour :"));
	vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_halign (label, GTK_ALIGN_START);
	gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox2), label, FALSE, TRUE, 0);

	adj = (GtkAdjustment *) gtk_adjustment_new
			(clockInst.faceSettings[clockInst.currentFace] -> alarmInfo.alarmHour, 0, 23, 1, 4, 0);
	spinner1 = gtk_spin_button_new (adj, 0, 0);
	gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinner1), TRUE);
	gtk_spin_button_set_snap_to_ticks (GTK_SPIN_BUTTON (spinner1), TRUE);
	gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner1), TRUE);
	gtk_entry_set_max_length (GTK_ENTRY (spinner1), 2);
	gtk_box_pack_start (GTK_BOX (vbox2), spinner1, FALSE, TRUE, 0);

	/*------------------------------------------------------------------------------------------------*
	 * Add the minute spinner                                                                         *
	 *------------------------------------------------------------------------------------------------*/
	label = gtk_label_new (_("Min :"));
	vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_halign (label, GTK_ALIGN_START);
	gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, TRUE, 5);
	gtk_box_pack_start (GTK_BOX (vbox2), label, FALSE, TRUE, 0);

	adj = (GtkAdjustment *) gtk_adjustment_new
			(clockInst.faceSettings[clockInst.currentFace] -> alarmInfo.alarmMin, 0, 59, 1, 5, 0);
	spinner2 = gtk_spin_button_new (adj, 0, 0);
	gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinner2), TRUE);
	gtk_spin_button_set_snap_to_ticks (GTK_SPIN_BUTTON (spinner2), TRUE);
	gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner2), TRUE);
	gtk_entry_set_max_length (GTK_ENTRY (spinner2), 2);
	gtk_box_pack_start (GTK_BOX (vbox2), spinner2, FALSE, TRUE, 0);

	/*------------------------------------------------------------------------------------------------*
	 * Add the message entry                                                                          *
	 *------------------------------------------------------------------------------------------------*/
	label = gtk_label_new (_("Show message :"));
	vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_halign (label, GTK_ALIGN_START);
	gtk_box_pack_start (GTK_BOX (vbox1), vbox2, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX(vbox2), label, FALSE, TRUE, 0);

	entry1 = gtk_entry_new ();
	gtk_entry_set_width_chars (GTK_ENTRY (entry1), 30);
	gtk_entry_set_max_length (GTK_ENTRY (entry1), 40);
	gtk_entry_set_text (GTK_ENTRY (entry1), clockInst.faceSettings[clockInst.currentFace] -> alarmInfo.message);
	gtk_box_pack_start (GTK_BOX(vbox2), entry1, TRUE, TRUE, 0);

	label = gtk_label_new (_("Run command :"));
	gtk_widget_set_halign (label, GTK_ALIGN_START);
	gtk_box_pack_start (GTK_BOX(vbox2), label, FALSE, TRUE, 0);

	entry2 = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY (entry2), 40);
	gtk_entry_set_text (GTK_ENTRY (entry2), clockInst.faceSettings[clockInst.currentFace] -> alarmInfo.command);
	gtk_box_pack_start (GTK_BOX(vbox2), entry2, TRUE, TRUE, 0);

	check = gtk_check_button_new_with_label (_("Weekdays only"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), clockInst.faceSettings[clockInst.currentFace] -> alarmInfo.onlyWeekdays);
	gtk_box_pack_start (GTK_BOX(vbox2), check, FALSE, FALSE, 0);

	/*------------------------------------------------------------------------------------------------*
	 * Display it, if OK pressed the save the new values                                              *
	 *------------------------------------------------------------------------------------------------*/
	gtk_widget_show_all (dialog);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
	{
		strcpy (clockInst.faceSettings[clockInst.currentFace] -> alarmInfo.message, gtk_entry_get_text (GTK_ENTRY(entry1)));
		strcpy (clockInst.faceSettings[clockInst.currentFace] -> alarmInfo.command, gtk_entry_get_text (GTK_ENTRY(entry2)));
		clockInst.faceSettings[clockInst.currentFace] -> alarmInfo.alarmHour = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(spinner1));
		clockInst.faceSettings[clockInst.currentFace] -> alarmInfo.alarmMin	 = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(spinner2));
		clockInst.faceSettings[clockInst.currentFace] -> alarmInfo.onlyWeekdays = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check));

		sprintf (value, "alarm_hour_%d", clockInst.currentFace + 1);
		configSetIntValue (value, clockInst.faceSettings[clockInst.currentFace] -> alarmInfo.alarmHour);
		sprintf (value, "alarm_min_%d", clockInst.currentFace + 1);
		configSetIntValue (value, clockInst.faceSettings[clockInst.currentFace] -> alarmInfo.alarmMin);
		sprintf (value, "alarm_message_%d", clockInst.currentFace + 1);
		configSetValue (value, clockInst.faceSettings[clockInst.currentFace] -> alarmInfo.message);
		sprintf (value, "alarm_command_%d", clockInst.currentFace + 1);
		configSetValue (value, clockInst.faceSettings[clockInst.currentFace] -> alarmInfo.command);
		sprintf (value, "alarm_only_weekdays_%d", clockInst.currentFace + 1);
		configSetBoolValue (value, clockInst.faceSettings[clockInst.currentFace] -> alarmInfo.onlyWeekdays);
		alarmSetAngle (clockInst.currentFace);
		lastTime = -1;
	}
	gtk_widget_destroy (dialog);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  A B O U T  C A L L B A C K                                                                                        *
 *  ==========================                                                                                        *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Display the about dialog.
 *  \param data Not used.
 *  \result None.
 */
void
aboutCallback (guint data)
{
	char comment[256];
	char verString[241];

	sprintf (verString, _("Version: %s, Dial: %s\nBuilt: %s"), VERSION, DIALSYS_VER, buildDate);
	sprintf (comment, _("Timezone Clock is a highly configurable analogue clock, capable\n"
					  "of showing the time in many different countries and cities.\n"
					  "Loaded time zones: %d"), nTimeZones - FIRST_CITY);

	/*------------------------------------------------------------------------------------------------*
	 * Nice dialog that can be used with newer versions of the GTK API.                               *
	 *------------------------------------------------------------------------------------------------*/
	gtk_show_about_dialog (clockInst.dialConfig.mainWindow,
			"title", _("About Timezone Clock"),
			"program-name", _("Timezone Clock"),
			"artists", artists,
			"authors", authors,
			"comments", comment,
			"copyright", "Copyright © 2005 - 2025 Chris Knight <chris@theknight.co.uk>",
			"logo", defaultIcon,
			"version", verString,
			"website", "https://www.TheKnight.co.uk/",
			NULL);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  P R E P A R E  F O R  P O P U P                                                                                   *
 *  ===============================                                                                                   *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Prepare the menus before showing them.
 *  \result None.
 */
void prepareForPopup (void)
{
	int i;

	viewMenuDesc[0].disable = (clockInst.dialConfig.dialWidth >= 10 || (clockInst.dialConfig.dialWidth + 1) * clockInst.dialConfig.dialHeight > MAX_FACES ? 1 : 0);
	viewMenuDesc[1].disable = (clockInst.dialConfig.dialWidth < 2 ? 1 : 0);
	viewMenuDesc[2].disable = (clockInst.dialConfig.dialHeight >= 10 || (clockInst.dialConfig.dialHeight + 1) * clockInst.dialConfig.dialWidth > MAX_FACES ? 1 : 0);
	viewMenuDesc[3].disable = (clockInst.dialConfig.dialHeight < 2 ? 1 : 0);

	prefMenuDesc[MENU_PREF_ONTOP].checked = alwaysOnTop;
	prefMenuDesc[MENU_PREF_STUCK].checked = stuckOnAll;
	prefMenuDesc[MENU_PREF_LOCK].checked = lockMove;

	prefMenuDesc[MENU_PREF_TIME].checked = clockInst.faceSettings[clockInst.currentFace] -> showTime;
	prefMenuDesc[MENU_PREF_SHOWS].checked = clockInst.faceSettings[clockInst.currentFace] -> showSeconds;
	prefMenuDesc[MENU_PREF_SUBS].checked = clockInst.faceSettings[clockInst.currentFace] -> subSecond;
	prefMenuDesc[MENU_PREF_SUBS].disable = !clockInst.faceSettings[clockInst.currentFace] -> showSeconds;
	prefMenuDesc[MENU_PREF_SVG].disable = !clockInst.allowSaveDisp;

	alarmMenuDesc[MENU_ALRM_ENBL].checked = clockInst.faceSettings[clockInst.currentFace] -> alarm;
	stopWMenuDesc[MENU_STPW_ENBL].checked = clockInst.faceSettings[clockInst.currentFace] -> stopwatch;
	stopWMenuDesc[MENU_STPW_START].disable = !clockInst.faceSettings[clockInst.currentFace] -> stopwatch;
	stopWMenuDesc[MENU_STPW_RESET].disable = !clockInst.faceSettings[clockInst.currentFace] -> stopwatch;

	timerMenuDesc[MENU_CNTD_ENBL].checked = clockInst.faceSettings[clockInst.currentFace] -> timer;
	timerMenuDesc[MENU_CNTD_START].disable = !clockInst.faceSettings[clockInst.currentFace] -> timer;
	timerMenuDesc[MENU_CNTD_RESET].disable = !clockInst.faceSettings[clockInst.currentFace] -> timer;

	for (i = MENU_MARK_STRT; i <= MENU_MARK_STOP; ++i)
		markerMenuDesc[i].checked = (markerMenuDesc[i].param == clockInst.dialConfig.markerType ? 1 : 0);
	for (i = MENU_STEP_STRT; i <= MENU_STEP_STOP; ++i)
		markerMenuDesc[i].checked = (markerMenuDesc[i].param == clockInst.dialConfig.markerStep ? 1 : 0);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  W I N D O W  C L I C K  C A L L B A C K                                                                           *
 *  =======================================                                                                           *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Called on button press.
 *  \param widget Window.
 *  \param event Button.
 *  \result Processed or not.
 */
gboolean
windowClickCallback (GtkWidget * widget, GdkEventButton * event)
{
	if (event->type == GDK_BUTTON_PRESS)
	{
		clockInst.currentFace = ((int)event -> x / clockInst.dialConfig.dialSize) + (((int)event -> y / clockInst.dialConfig.dialSize) * clockInst.dialConfig.dialWidth);
		lastTime = -1;

		switch (event->button)
		{
#ifndef CLOCK_IS_DECORATED
#ifdef GDK_BUTTON_PRIMARY
		case GDK_BUTTON_PRIMARY:	/* left button */
#else
		case 1:
#endif
			if (!lockMove && !clockInst.clockDecorated)
			{
				gtk_window_begin_move_drag (GTK_WINDOW (clockInst.dialConfig.mainWindow), event->button, event->x_root,
						event->y_root, event->time);
			}
			return TRUE;
#endif

#ifdef GDK_BUTTON_SECONDARY
		case GDK_BUTTON_SECONDARY:	/* right button */
#else
		case 3:
#endif
			{
				GtkWidget *popupMenu;

				prepareForPopup ();
				popupMenu = createMenu (mainMenuDesc, clockInst.accelGroup, FALSE);
#if GTK_MINOR_VERSION >= 22
				gtk_menu_popup_at_pointer (GTK_MENU(popupMenu), NULL);
#else
				gtk_menu_popup (GTK_MENU(popupMenu), NULL,	/* parent_menu_shell */
						NULL,								/* parent_menu_item */
						NULL,								/* func */
						NULL,								/* data */
						event->button, event->time);
#endif
				return TRUE;
			}
		}
	}
	return FALSE;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  W I N D O W  K E Y  C A L L B A C K                                                                               *
 *  ===================================                                                                               *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Called on key press.
 *  \param widget Window.
 *  \param event Key pressed.
 *  \result Processed or not.
 */
gboolean
windowKeyCallback (GtkWidget * widget, GdkEventKey * event)
{
	/*------------------------------------------------------------------------------------------------*
	 * You can select a face buy pressing and holding the ALT key while typing the number of the      *
	 * clock facce that you want to select.  In order to do this we track the ALT key press and       *
	 * release.  Each key must be pressed with in two seconds of the last key press.                  *
	 *------------------------------------------------------------------------------------------------*/
	if (event->time < lastKeyPressTime + 2000)
	{
		lastKeyPressTime = event->time;
	}
	else
	{
		lastKeyPressTime = 0;
		keyPressFaceNum = -1;
	}

	/*------------------------------------------------------------------------------------------------*
	 * Process key press events                                                                       *
	 *------------------------------------------------------------------------------------------------*/
	if (event->type == GDK_KEY_PRESS)
	{
		if (event->keyval == 0xFFE9) /* Alt key has been pressed */
		{
			lastKeyPressTime = event->time;
			keyPressFaceNum = 0;
		}

		if (event->state & 8) /* Trial and error showed 8 to be the Alt key flag */
		{
			/*----------------------------------------------------------------------------------------*
			 * Press ALT + M to display the Menu at the current mouse position                        *
			 *----------------------------------------------------------------------------------------*/
			if (event->keyval == GDK_KEY_M || event->keyval == GDK_KEY_m)
			{
				GtkWidget *popupMenu;

				prepareForPopup ();
				popupMenu = createMenu (mainMenuDesc, clockInst.accelGroup, FALSE);
#if GTK_MINOR_VERSION >= 22
				gtk_menu_popup_at_pointer (GTK_MENU(popupMenu), NULL);
#else
				gtk_menu_popup (GTK_MENU(popupMenu), NULL,	/* parent_menu_shell */
						NULL,								/* parent_menu_item */
						NULL,								/* func */
						NULL,								/* data */
						0, event->time);
#endif
			}
			/*----------------------------------------------------------------------------------------*
			 * Press ALT + n where n is a number between 1 and 9 to select the face                   *
			 *----------------------------------------------------------------------------------------*/
			if (keyPressFaceNum != -1)
			{
				if (event->keyval >= GDK_KEY_0 && event->keyval <= GDK_KEY_9)
				{
					keyPressFaceNum *= 10;
					keyPressFaceNum += event->keyval - GDK_KEY_0;
					lastKeyPressTime = event->time;
				}
				else
				{
					keyPressFaceNum = -1;
					lastKeyPressTime = 0;
				}
			}
		}
		gtk_window_activate_key (GTK_WINDOW (clockInst.dialConfig.mainWindow), event);
	}
	/*------------------------------------------------------------------------------------------------*
	 * Process key release events                                                                     *
	 *------------------------------------------------------------------------------------------------*/
	else if (event->type == GDK_KEY_RELEASE)
	{
		if (event->keyval == 0xFFE9 && keyPressFaceNum != -1)  /* Alt key released */
		{
			keyPressFaceNum --;
			if (keyPressFaceNum >= 0 && keyPressFaceNum < (clockInst.dialConfig.dialWidth * clockInst.dialConfig.dialHeight))
			{
				clockInst.currentFace = keyPressFaceNum;
				lastTime = -1;
			}
		}
	}
	return TRUE;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  G E T  H A N D  P O S I T I O N S                                                                                 *
 *  =================================                                                                                 *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Calculate the positions of the hands.
 *  \param face Which clock face.
 *  \param faceSetting Settings for the face.
 *  \param tm Current time.
 *  \param t Current second.
 *  \result True if changed.
 */
int getHandPositions (int face, FACE_SETTINGS *faceSetting, struct tm *tm, time_t t)
{
	int i, j, tempTime, update = 0;
	char tempString[211];
	unsigned short angle;

	/*------------------------------------------------------------------------------------------------*
	 * Handle the smooth scroll time change                                                           *
	 *------------------------------------------------------------------------------------------------*/
	tempTime = (tm -> tm_hour * 60) + tm -> tm_min;

	if (!clockInst.fastSetting)
	{
		int gap = 0;

		/*--------------------------------------------------------------------------------------------*
		 * Don't scroll at midnight                                                                   *
         *--------------------------------------------------------------------------------------------*/
		if (tempTime == 0 && faceSetting -> shownTime == (23 * 60) + 59)
			gap = 1;
		else
			gap = tempTime - faceSetting -> shownTime;

		if (abs (gap) > 1)
		{
			/*----------------------------------------------------------------------------------------*
             * The maximum gap is 12 hours unless it is a 24 hour clock                               *
             *----------------------------------------------------------------------------------------*/
			if (abs(gap) > TWELVEHOURS && !faceSetting -> show24Hour)
			{
				if (gap > 0)
				{
					faceSetting -> shownTime += TWELVEHOURS;
					gap -= TWELVEHOURS;
				}
				else
				{
					faceSetting -> shownTime -= TWELVEHOURS;
					gap += TWELVEHOURS;
				}
			}

			j = 1;
			for (i = 1; i <= faceSetting -> stepping; i ++)
			{
				j += (i / 3);
				if (j > abs (gap))
					break;
			}
			i /= 3;
			if (gap < 0)
				i *= -1;

			if (faceSetting -> stepping < 24)
				faceSetting -> stepping ++;

			tempTime = faceSetting -> shownTime + i;

			tm -> tm_min = tempTime % 60;
			tm -> tm_hour = tempTime / 60;

			clockInst.timeSetting = 1;
		}
		else
			faceSetting -> stepping = 0;
	}
	faceSetting -> shownTime = tempTime;

	/*------------------------------------------------------------------------------------------------*
     * Calculate second hand position                                                                 *
     *------------------------------------------------------------------------------------------------*/
	angle = (tm -> tm_sec * 20);
	if (bounceSec) angle += 3;
	if (angle != faceSetting -> handPosition[HAND_SECS])
	{
		faceSetting -> handPosition[HAND_SECS] = angle;
		if (faceSetting -> showSeconds)
			update = 1;

		if (face == clockInst.currentFace)
		{
			getStringValue (tempString, 200, faceSetting -> currentTZ ? TXT_TITLE_Z : TXT_TITLE_L, face, t);
			if (strcmp (clockInst.windowTitle, tempString))
			{
				strcpy (clockInst.windowTitle, tempString);
				gtk_window_set_title (GTK_WINDOW (clockInst.dialConfig.mainWindow), clockInst.windowTitle);
			}
		}
		if (face == clockInst.toolTipFace)
		{
			getStringValue (tempString, 200, faceSetting -> currentTZ ? TXT_TOOLTIP_Z : TXT_TOOLTIP_L, face, t);
			if (strcmp (clockInst.windowToolTip, tempString))
			{
				strcpy (clockInst.windowToolTip, tempString);
				gtk_widget_set_tooltip_markup (GTK_WIDGET (clockInst.dialConfig.mainWindow), clockInst.windowToolTip);
			}
		}
	}
	/*------------------------------------------------------------------------------------------------*
     * Calculate minute hand position                                                                 *
     *------------------------------------------------------------------------------------------------*/
	angle = (tm -> tm_min * 20) + (tm -> tm_sec / 3);
	if (angle != faceSetting -> handPosition[HAND_MINUTE])
	{
		faceSetting -> handPosition[HAND_MINUTE] = angle;
		update = 1;
		/*--------------------------------------------------------------------------------------------*
         * Calculate hour hand position                                                               *
         *--------------------------------------------------------------------------------------------*/
		angle = faceSetting -> show24Hour ?
				((tm -> tm_hour * 50)  + ((tm -> tm_min * 60 + tm -> tm_sec) / 72)):
				((tm -> tm_hour * 100) + ((tm -> tm_min * 60 + tm -> tm_sec) / 36));
		if (angle != faceSetting -> handPosition[HAND_HOUR])
		{
			faceSetting -> handPosition[HAND_HOUR] = angle;
			update = 1;
		}
	}

	if (faceSetting -> stopwatch)
	{
		int swTime = getStopwatchTime(faceSetting);
		/*--------------------------------------------------------------------------------------------*
         * Calculate stopwatch tenths hand position                                                   *
         *--------------------------------------------------------------------------------------------*/
		angle = (swTime % 100) * 12;
		if (angle != faceSetting -> handPosition[HAND_STOPWT])
		{
			faceSetting -> handPosition[HAND_STOPWT] = angle;
			update = 1;
		}
		/*--------------------------------------------------------------------------------------------*
         * Calculate stopwatch seconds hand position                                                   *
         *--------------------------------------------------------------------------------------------*/
		angle = ((swTime / 100) * 20) % 1200;
		if (angle != faceSetting -> handPosition[HAND_STOPWS])
		{
			faceSetting -> handPosition[HAND_STOPWS] = angle;
			update = 1;
		}
		/*--------------------------------------------------------------------------------------------*
         * Calculate stopwatch minute hand position                                                   *
         *--------------------------------------------------------------------------------------------*/
		angle = ((swTime / 600) * 4) % 1200;
		if (angle != faceSetting -> handPosition[HAND_STOPWM])
		{
			faceSetting -> handPosition[HAND_STOPWM] = angle;
			update = 1;
		}
	}
	else if (faceSetting -> timer)
	{
		int cdTime = getTimerTime(faceSetting);
		/*--------------------------------------------------------------------------------------------*
         * Calculate timer second hand position                                                   *
         *--------------------------------------------------------------------------------------------*/
		angle = (cdTime % 60) * 20;
		if (angle != faceSetting -> handPosition[HAND_STOPWS])
		{
			faceSetting -> handPosition[HAND_STOPWS] = angle;
			update = 1;
		}
		/*--------------------------------------------------------------------------------------------*
         * Calculate timer minute hand position                                                   *
         *--------------------------------------------------------------------------------------------*/
		angle = ((cdTime / 6) % 300) * 4;
		if (angle != faceSetting -> handPosition[HAND_STOPWM])
		{
			faceSetting -> handPosition[HAND_STOPWM] = angle;
			update = 1;
		}
		/*--------------------------------------------------------------------------------------------*
         * Calculate timer hours hand position                                                   *
         *--------------------------------------------------------------------------------------------*/
		angle = (cdTime / 90) * 5;
		if (angle != faceSetting -> handPosition[HAND_STOPWT])
		{
			faceSetting -> handPosition[HAND_STOPWT] = angle;
			update = 1;
		}
	}
	if (faceSetting -> updateFace)
	{
		faceSetting -> updateFace = false;
		update = 1;
	}
	return update;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  C L O C K  T I C K  C A L L B A C K                                                                               *
 *  ===================================                                                                               *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Called on the timer to update the face.
 *  \param data Not used.
 *  \result Always true.
 */
gboolean
clockTickCallback (gpointer data)
{
	struct tm tm;
	struct timeval tv;
	time_t t = time (NULL);
	int update = 0, i, faceCount = clockInst.dialConfig.dialHeight * clockInst.dialConfig.dialWidth;

	if (clockInst.forceTime != -1)
		t = clockInst.forceTime;
	if (lastTime == -1)
		update = 1;
	lastTime = t;

	tv.tv_sec = 0;
	for (i = 0; i < faceCount; ++i)
	{
		FACE_SETTINGS *faceSetting = clockInst.faceSettings[i];

		if (faceSetting -> stepping || (faceSetting -> stopwatch && faceSetting -> swStartTime != -1) ||
				(faceSetting -> timer && faceSetting -> swStartTime != -1) ||
				faceSetting -> timeShown != t || faceSetting -> updateFace || bounceSec)
		{
			getTheFaceTime (faceSetting, &t, &tm);
			checkForAlarm (faceSetting, &tm);
			if (clockInst.showBounceSec && faceSetting -> showSeconds)
			{
				if (tv.tv_sec == 0)
					gettimeofday(&tv, NULL);
				bounceSec = tv.tv_usec < 50000 ? 1 : 0;
			}
			update += getHandPositions (i, faceSetting, &tm, t);
			faceSetting -> timeShown = t;
		}
	}
	if (update)
	{
		if (clockInst.dialConfig.drawingArea)
		{
			gtk_widget_queue_draw (clockInst.dialConfig.drawingArea);
		}
	}
	return TRUE;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  F O C U S  I N  E V E N T                                                                                         *
 *  =========================                                                                                         *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Clock gained focus.
 *  \param widget Not used.
 *  \param event not used.
 *  \param data not used.
 *  \result True.
 */
gboolean
focusInEvent (GtkWidget *widget, GdkEventFocus *event, gpointer data)
{
	lastTime = -1;
	clockInst.weHaveFocus = 1;
	return TRUE;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  F O C U S  O U T  E V E N T                                                                                       *
 *  ===========================                                                                                       *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Clock lost focus.
 *  \param widget not used.
 *  \param event not used.
 *  \param data not used.
 *  \result True.
 */
gboolean
focusOutEvent (GtkWidget *widget, GdkEventFocus *event, gpointer data)
{
	lastTime = -1;
	clockInst.weHaveFocus = 0;
	return TRUE;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  U S E R  A C T I V E                                                                                              *
 *  ====================                                                                                              *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Track the mouse so the tooltip matches the face.
 *  \param widget Owner widget.
 *  \param event Move move event.
 *  \param data Not used.
 *  \result TRUE.
 */
gboolean
userActive (GtkWidget *widget, GdkEvent* event, gpointer data)
{
	int newFace = 0;
	gdouble dx, dy;

	gdk_event_get_coords (event, &dx, &dy);
	newFace = ((int)dx / clockInst.dialConfig.dialSize) + (((int)dy / clockInst.dialConfig.dialSize) * clockInst.dialConfig.dialWidth);
	if (newFace != clockInst.toolTipFace)
	{
		clockInst.toolTipFace = newFace;
		lastTime = -1;
	}
	return TRUE;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S H O W  T I M E  C A L L B A C K                                                                                 *
 *  =================================                                                                                 *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Switch on and off the time display.
 *  \param data Not used.
 *  \result None.
 */
void
showTimeCallback (guint data)
{
	char value[81];

	clockInst.faceSettings[clockInst.currentFace] -> showTime = !clockInst.faceSettings[clockInst.currentFace] -> showTime;
	sprintf (value, "show_time_%d", clockInst.currentFace + 1);
	configSetBoolValue (value, clockInst.faceSettings[clockInst.currentFace] -> showTime);
	clockInst.faceSettings[clockInst.currentFace] -> updateFace = true;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S H O W  S E C O N D S  C A L L B A C K                                                                           *
 *  =======================================                                                                           *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Callback for show seconds menu.
 *  \param data Not used.
 *  \result None.
 */
void
showSecondsCallback (guint data)
{
	char value[81];

	clockInst.faceSettings[clockInst.currentFace] -> showSeconds = !clockInst.faceSettings[clockInst.currentFace] -> showSeconds;
	sprintf (value, "show_seconds_%d", clockInst.currentFace + 1);
	configSetBoolValue (value, clockInst.faceSettings[clockInst.currentFace] -> showSeconds);
	clockInst.faceSettings[clockInst.currentFace] -> updateFace = true;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S U B  S E C O N D  C A L L B A C K                                                                               *
 *  ===================================                                                                               *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Callback for sub seconds menu.
 *  \param data Not used.
 *  \result None.
 */
void
subSecondCallback (guint data)
{
	char value[81];

	clockInst.faceSettings[clockInst.currentFace] -> subSecond = !clockInst.faceSettings[clockInst.currentFace] -> subSecond;
	sprintf (value, "sub_second_%d", clockInst.currentFace + 1);
	configSetBoolValue (value, clockInst.faceSettings[clockInst.currentFace] -> subSecond);
	clockInst.faceSettings[clockInst.currentFace] -> updateFace = true;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S T O P W A T C H  C A L L B A C K                                                                                *
 *  ==================================                                                                                *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Callback for stop watch menu.
 *  \param data Not used.
 *  \result None.
 */
void
stopwatchCallback (guint data)
{
	char value[81];
	int newVal = !clockInst.faceSettings[clockInst.currentFace] -> stopwatch;

	clockInst.faceSettings[clockInst.currentFace] -> stopwatch = newVal;
	if (newVal)
	{
		clockInst.faceSettings[clockInst.currentFace] -> timer = false;
		sprintf (value, "timer_%d", clockInst.currentFace + 1);
		configSetBoolValue (value, false);
	}
	sprintf (value, "stopwatch_%d", clockInst.currentFace + 1);
	configSetBoolValue (value, newVal);
	clockInst.faceSettings[clockInst.currentFace] -> swStartTime = -1;
	clockInst.faceSettings[clockInst.currentFace] -> swRunTime = 0;
	clockInst.faceSettings[clockInst.currentFace] -> updateFace = true;
	lastTime = -1;

	prepareForPopup ();
	createMenu (mainMenuDesc, clockInst.accelGroup, FALSE);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S W  S T A R T  C A L L B A C K                                                                                   *
 *  ===============================                                                                                   *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Callback for stop watch start menu.
 *  \param data Not used.
 *  \result None.
 */
void
swStartCallback (guint data)
{
	if (clockInst.faceSettings[clockInst.currentFace] -> stopwatch)
	{
		struct timeval tv;
		if (gettimeofday(&tv, NULL) == 0)
		{
			if (clockInst.faceSettings[clockInst.currentFace] -> swStartTime == -1)
			{
				clockInst.faceSettings[clockInst.currentFace] -> swStartTime = (tv.tv_sec * 100) + (tv.tv_usec / 10000);
				clockInst.faceSettings[clockInst.currentFace] -> swStartTime -= clockInst.faceSettings[clockInst.currentFace] -> swRunTime;
				clockInst.faceSettings[clockInst.currentFace] -> swRunTime = 0;
				clockInst.faceSettings[clockInst.currentFace] -> updateFace = true;
				stopwatchActive ++;
				lastTime = -1;
			}
			else
			{
				long long tempTime = (tv.tv_sec * 100) + (tv.tv_usec / 10000);

				tempTime -= clockInst.faceSettings[clockInst.currentFace] -> swStartTime;
				clockInst.faceSettings[clockInst.currentFace] -> swRunTime = tempTime;
				clockInst.faceSettings[clockInst.currentFace] -> swStartTime = -1;
				clockInst.faceSettings[clockInst.currentFace] -> updateFace = true;
				stopwatchActive --;
				lastTime = -1;
			}
		}
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S W  R E S E T  C A L L B A C K                                                                                   *
 *  ===============================                                                                                   *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Callback for stop watch reset menu.
 *  \param data Not used.
 *  \result None.
 */
void
swResetCallback (guint data)
{
	if (clockInst.faceSettings[clockInst.currentFace] -> stopwatch)
	{
		int oldStartTime = clockInst.faceSettings[clockInst.currentFace] -> swStartTime;

		clockInst.faceSettings[clockInst.currentFace] -> swStartTime = -1;
		clockInst.faceSettings[clockInst.currentFace] -> swRunTime = 0;
		clockInst.faceSettings[clockInst.currentFace] -> updateFace = true;

		if (oldStartTime != -1)
		{
			swStartCallback (data);
		}
		lastTime = -1;
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  T I M E R  C A L L B A C K                                                                                        *
 *  ==========================                                                                                        *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Turn on and off the timer timer.
 *  \param data Not used.
 *  \result None.
 */
void
timerCallback (guint data)
{
	char value[81];
	FACE_SETTINGS *faceSetting = clockInst.faceSettings[clockInst.currentFace];
	bool newVal = !faceSetting -> timer;

	faceSetting -> timer = newVal;
	if (newVal)
	{
		faceSetting -> stopwatch = false;
		sprintf (value, "stopwatch_%d", clockInst.currentFace + 1);
		configSetBoolValue (value, false);
	}
	sprintf (value, "timer_%d", clockInst.currentFace + 1);
	configSetBoolValue (value, newVal);
	faceSetting -> timerInfo.totalTime =
			(faceSetting -> timerInfo.timerHour * 3600) +
			(faceSetting -> timerInfo.timerMin * 60) +
			faceSetting -> timerInfo.timerSec;
	faceSetting -> timerInfo.timerShown = 1;
	faceSetting -> swStartTime = -1;
	faceSetting -> swRunTime = 0;
	faceSetting -> updateFace = true;
	lastTime = -1;

	prepareForPopup ();
	createMenu (mainMenuDesc, clockInst.accelGroup, FALSE);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  T M  S T A R T  C A L L B A C K                                                                                   *
 *  ===============================                                                                                   *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Start or Stop the timer timer.
 *  \param data Not used.
 *  \result None.
 */
void
tmStartCallback (guint data)
{
	FACE_SETTINGS *faceSetting = clockInst.faceSettings[clockInst.currentFace];
	if (faceSetting -> timer)
	{
		struct timeval tv;
		if (gettimeofday(&tv, NULL) == 0)
		{
			if (faceSetting -> swStartTime == -1)
			{
				faceSetting -> swStartTime = tv.tv_sec - faceSetting -> swRunTime;
				faceSetting -> timerInfo.totalTime =
						(faceSetting -> timerInfo.timerHour * 3600) +
						(faceSetting -> timerInfo.timerMin * 60) +
						faceSetting -> timerInfo.timerSec;
				faceSetting -> timerInfo.timerShown = 0;
				faceSetting -> updateFace = true;
				stopwatchActive ++;
				lastTime = -1;
			}
			else
			{
				faceSetting -> swRunTime = (tv.tv_sec - faceSetting -> swStartTime);
				faceSetting -> swStartTime = -1;
				faceSetting -> updateFace = true;
				stopwatchActive --;
				lastTime = -1;
			}
		}
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  T M  R E S E T  C A L L B A C K                                                                                   *
 *  ===============================                                                                                   *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Reset the timer to the start time and stop.
 *  \param data Not used.
 *  \result None.
 */
void
tmResetCallback (guint data)
{
	FACE_SETTINGS *faceSetting = clockInst.faceSettings[clockInst.currentFace];
	if (faceSetting -> timer)
	{
		faceSetting -> swStartTime = -1;
		faceSetting -> timerInfo.totalTime =
				(faceSetting -> timerInfo.timerHour * 3600) +
				(faceSetting -> timerInfo.timerMin * 60) +
				faceSetting -> timerInfo.timerSec;
		faceSetting -> timerInfo.timerShown = 1;
		faceSetting -> swRunTime = 0;
		faceSetting -> updateFace = true;
		lastTime = -1;
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  T I M E R  S E T  C A L L B A C K                                                                                 *
 *  =================================                                                                                 *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Setup the timer start time and what to do when complete.
 *  \param data Not used.
 *  \result None.
 */
void
timerSetCallback (guint data)
{
	char value[81];
	GtkWidget *dialog;
	GtkWidget *entry1, *entry2;
	GtkWidget *label;
	GtkWidget *spinner0, *spinner1, *spinner2;
	GtkWidget *hbox, *vbox1, *vbox2;
	GtkAdjustment *adj;
	GtkWidget *contentArea;
	FACE_SETTINGS *faceSetting = clockInst.faceSettings[clockInst.currentFace];

	/*------------------------------------------------------------------------------------------------*
	 * Create the basic dialog box                                                                    *
	 *------------------------------------------------------------------------------------------------*/
	dialog = gtk_dialog_new_with_buttons (_("Set Up Timer"), GTK_WINDOW(clockInst.dialConfig.mainWindow),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
#if GTK_MINOR_VERSION >= 10
			_("Close"),
#else
			GTK_STOCK_CLOSE,
#endif
			GTK_RESPONSE_ACCEPT, NULL);

	contentArea = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
	vbox1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_pack_start (GTK_BOX (contentArea), vbox1, FALSE, TRUE, 0);
	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (vbox1), hbox, FALSE, TRUE, 0);

	/*------------------------------------------------------------------------------------------------*
	 * Add the hour spinner                                                                           *
	 *------------------------------------------------------------------------------------------------*/
	label = gtk_label_new (_("Hour :"));
	vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_halign (label, GTK_ALIGN_START);
	gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox2), label, FALSE, TRUE, 0);

	adj = (GtkAdjustment *) gtk_adjustment_new (faceSetting -> timerInfo.timerHour, 0, 6, 1, 4, 0);
	spinner0 = gtk_spin_button_new (adj, 0, 0);
	gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinner0), TRUE);
	gtk_spin_button_set_snap_to_ticks (GTK_SPIN_BUTTON (spinner0), TRUE);
	gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner0), TRUE);
	gtk_entry_set_max_length (GTK_ENTRY (spinner0), 2);
	gtk_box_pack_start (GTK_BOX (vbox2), spinner0, FALSE, TRUE, 0);

	/*------------------------------------------------------------------------------------------------*
	 * Add the hour spinner                                                                           *
	 *------------------------------------------------------------------------------------------------*/
	label = gtk_label_new (_("Min :"));
	vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_halign (label, GTK_ALIGN_START);
	gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox2), label, FALSE, TRUE, 0);

	adj = (GtkAdjustment *) gtk_adjustment_new (faceSetting -> timerInfo.timerMin, 0, 59, 1, 4, 0);
	spinner1 = gtk_spin_button_new (adj, 0, 0);
	gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinner1), TRUE);
	gtk_spin_button_set_snap_to_ticks (GTK_SPIN_BUTTON (spinner1), TRUE);
	gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner1), TRUE);
	gtk_entry_set_max_length (GTK_ENTRY (spinner1), 2);
	gtk_box_pack_start (GTK_BOX (vbox2), spinner1, FALSE, TRUE, 0);

	/*------------------------------------------------------------------------------------------------*
	 * Add the minute spinner                                                                         *
	 *------------------------------------------------------------------------------------------------*/
	label = gtk_label_new (_("Sec :"));
	vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_halign (label, GTK_ALIGN_START);
	gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, TRUE, 5);
	gtk_box_pack_start (GTK_BOX (vbox2), label, FALSE, TRUE, 0);

	adj = (GtkAdjustment *) gtk_adjustment_new (faceSetting -> timerInfo.timerSec, 0, 59, 1, 5, 0);
	spinner2 = gtk_spin_button_new (adj, 0, 0);
	gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinner2), TRUE);
	gtk_spin_button_set_snap_to_ticks (GTK_SPIN_BUTTON (spinner2), TRUE);
	gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner2), TRUE);
	gtk_entry_set_max_length (GTK_ENTRY (spinner2), 2);
	gtk_box_pack_start (GTK_BOX (vbox2), spinner2, FALSE, TRUE, 0);

	/*------------------------------------------------------------------------------------------------*
	 * Add the message entry                                                                          *
	 *------------------------------------------------------------------------------------------------*/
	label = gtk_label_new (_("Show message :"));
	vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_halign (label, GTK_ALIGN_START);
	gtk_box_pack_start (GTK_BOX (vbox1), vbox2, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX(vbox2), label, FALSE, TRUE, 0);

	entry1 = gtk_entry_new ();
	gtk_entry_set_width_chars (GTK_ENTRY (entry1), 30);
	gtk_entry_set_max_length (GTK_ENTRY (entry1), 40);
	gtk_entry_set_text (GTK_ENTRY (entry1), faceSetting -> timerInfo.message);
	gtk_box_pack_start (GTK_BOX(vbox2), entry1, TRUE, TRUE, 0);

	label = gtk_label_new (_("Run command :"));
	gtk_widget_set_halign (label, GTK_ALIGN_START);
	gtk_box_pack_start (GTK_BOX(vbox2), label, FALSE, TRUE, 0);

	entry2 = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY (entry2), 40);
	gtk_entry_set_text (GTK_ENTRY (entry2), faceSetting -> timerInfo.command);
	gtk_box_pack_start (GTK_BOX(vbox2), entry2, TRUE, TRUE, 0);

	/*------------------------------------------------------------------------------------------------*
	 * Display it, if OK pressed the save the new values                                              *
	 *------------------------------------------------------------------------------------------------*/
	gtk_widget_show_all (dialog);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
	{
		strcpy (faceSetting -> timerInfo.message, gtk_entry_get_text (GTK_ENTRY(entry1)));
		strcpy (faceSetting -> timerInfo.command, gtk_entry_get_text (GTK_ENTRY(entry2)));
		faceSetting -> timerInfo.timerHour = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(spinner0));
		faceSetting -> timerInfo.timerMin = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(spinner1));
		faceSetting -> timerInfo.timerSec	 = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(spinner2));
		faceSetting -> timerInfo.totalTime =
				(faceSetting -> timerInfo.timerHour * 3600) +
				(faceSetting -> timerInfo.timerMin * 60) +
				faceSetting -> timerInfo.timerSec;

		sprintf (value, "timer_hour_%d", clockInst.currentFace + 1);
		configSetIntValue (value, faceSetting -> timerInfo.timerHour);
		sprintf (value, "timer_min_%d", clockInst.currentFace + 1);
		configSetIntValue (value, faceSetting -> timerInfo.timerMin);
		sprintf (value, "timer_sec_%d", clockInst.currentFace + 1);
		configSetIntValue (value, faceSetting -> timerInfo.timerSec);
		sprintf (value, "timer_message_%d", clockInst.currentFace + 1);
		configSetValue (value, faceSetting -> timerInfo.message);
		sprintf (value, "timer_command_%d", clockInst.currentFace + 1);
		configSetValue (value, faceSetting -> timerInfo.command);
	}
	gtk_widget_destroy (dialog);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  C O N F I G  S A V E  C A L L B A C K                                                                             *
 *  =====================================                                                                             *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Callback for save settings menu.
 *  \param data Not used.
 *  \result None.
 */
void configSaveCallback (guint data)
{
	int posX, posY;
	char *home = getenv ("HOME");
	char configPath[1024];

	gtk_window_get_position (GTK_WINDOW (clockInst.dialConfig.mainWindow), &posX, &posY);
	configSetIntValue ("clock_x_pos", posX);
	configSetIntValue ("clock_y_pos", posY);
	configSetIntValue ("current_face", clockInst.currentFace);

	if (home)
	{
		strcpy (configPath, home);
		strcat (configPath, "/");
		strcat (configPath, clockInst.configFile);
		configSave (configPath);
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  G E T  S T O P W A T C H  T I M E                                                                                 *
 *  =================================                                                                                 *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Get the time on the stopwatch.
 *  \param faceSetting Face settings .
 *  \result time to show.
 */
int
getStopwatchTime (FACE_SETTINGS *faceSetting)
{
	struct timeval tv;

	if (faceSetting -> swStartTime == -1)
	{
		return faceSetting -> swRunTime;
	}
	else
	{
		if (gettimeofday(&tv, NULL) == 0)
		{
			long long tempTime = (tv.tv_sec * 100) + (tv.tv_usec / 10000);
			tempTime -= faceSetting -> swStartTime;
			return (int)(tempTime % (30 * 60 * 100));
		}
	}
	return 0;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  G E T  T I M E R  T I M E                                                                                         *
 *  =========================                                                                                         *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Get the current time to completion of the timer.
 *  \param faceSetting Which face is this for.
 *  \result Time remaining.
 */
int
getTimerTime (FACE_SETTINGS *faceSetting)
{
	struct timeval tv;

	if (faceSetting -> swStartTime == -1)
	{
		int retn = faceSetting -> timerInfo.totalTime - faceSetting -> swRunTime;
		return retn > 0 ? retn % (24 * 3600) : 0;
	}
	else
	{
		if (gettimeofday(&tv, NULL) == 0)
		{
			long long tempTime = tv.tv_sec;

			tempTime -= faceSetting -> swStartTime;
			int retn = faceSetting -> timerInfo.totalTime - tempTime;

			if (retn < 0)
			{
				faceSetting -> swRunTime = faceSetting -> timerInfo.totalTime;
				faceSetting -> swStartTime = -1;
				if (faceSetting -> timerInfo.totalTime > 0)
				{
					checkForTimer (faceSetting);
				}
				return 0;
			}
			return retn % (24 * 3600);
		}
	}
	return 0;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  G E T  S T R I N G  V A L U E                                                                                     *
 *  =============================                                                                                     *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Get one of the strings to display on the face.
 *  \param addBuffer Save to here.
 *  \param maxSize Size of the buffer.
 *  \param stringNumber Which string.
 *  \param face Which face.
 *  \param timeNow Current time.
 *  \result Pointer to add buffer.
 */
char *getStringValue (char *addBuffer, int maxSize, int stringNumber, int face, time_t timeNow)
{
	FACE_SETTINGS *faceSetting = clockInst.faceSettings[face];
	char tempAddStr[101], tempCommand[11], stringFormat[81];
	int i = 0, j = 0;
	struct tm tm;

	localtime_r (&timeNow, &tm);
	tempCommand[0] = tempAddStr[0] = addBuffer[0] = 0;
	strcpy (stringFormat, displayString[stringNumber]);

	while (stringFormat[i])
	{
		if (tempCommand[0] == '%')
		{
			tempCommand[j] = stringFormat[i];
			tempCommand[j + 1] = 0;

			switch (tempCommand[j])
			{
			case '-':
			case '_':
				j = 2;
				break;
			case '*':				/* Timezone's city un-wrapped */
			{
				int k = 0;
				char *text = faceSetting -> overwriteMesg[0] ?
					faceSetting -> overwriteMesg : faceSetting -> currentTZDisp;

				while (text[k])
				{
					tempAddStr[k] = (text[k] == '\n' ? ' ' : text[k]);
					tempAddStr[++k] = 0;
				}
				tempCommand[j = 0] = 0;
				break;
			}
			case '#':				/* Timezone's city wrapped */
				strncpy (tempAddStr, (faceSetting -> overwriteMesg[0] ? faceSetting -> overwriteMesg :
						faceSetting -> currentTZDisp), 100);
				tempCommand[j = 0] = 0;
				break;
			case '@':				/* Timezone's area */
				strcpy (tempAddStr, faceSetting -> currentTZArea);
				tempCommand[j = 0] = 0;
				break;
			case '&':
			{
				if (faceSetting -> stopwatch)
				{
					int swTime = getStopwatchTime (faceSetting);
					sprintf (tempAddStr, "%d:%02d:%02d.%02d",
							swTime / 360000, (swTime / 6000) % 60, (swTime / 100) % 60, swTime % 100);
				}
				else if (faceSetting -> timer)
				{
					int cdTime = getTimerTime (faceSetting);
					sprintf (tempAddStr, "%d:%02d:%02d",
							cdTime / 3600, (cdTime / 60) % 60, cdTime % 60);
				}
				else
				{
					strcpy (tempAddStr, _("not set"));
				}
				tempCommand[j = 0] = 0;
				break;
			}
			case '$':
				if (faceSetting -> alarm)
					sprintf (tempAddStr, "%d:%02d", faceSetting -> alarmInfo.alarmHour, faceSetting -> alarmInfo.alarmMin);
				else
					strcpy (tempAddStr, _("not set"));
				tempCommand[j = 0] = 0;
				break;
			default:
				strftime (tempAddStr, 80, tempCommand, &tm);
				tempCommand[j = 0] = 0;
				break;
			}
		}
		else
		{
			if (stringFormat[i] == '%')
			{
				tempCommand[0] = '%';
				tempCommand[j = 1] = 0;
			}
			else
			{
				tempAddStr[0] = stringFormat[i];
				tempAddStr[1] = 0;
			}
		}
		if (tempAddStr[0])
		{
			if (strlen (tempAddStr) + strlen (addBuffer) < maxSize)
			{
				strcat (addBuffer, tempAddStr);
			}
			tempAddStr[0] = 0;
		}
		i++;
	}
	return addBuffer;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D R A W  C A L L B A C K                                                                                          *
 *  ========================                                                                                          *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Handle a draw call back to display the gauge.
 *  \param widget Which widget.
 *  \param cr Cairo handle to use.
 *  \param data Not used.
 *  \result None.
 */
gboolean
drawCallback (GtkWidget *widget, cairo_t *cr, gpointer data)
{
	clockExpose (cr);
	return TRUE;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  E X E C  C O M M A N D                                                                                            *
 *  ======================                                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief On the alarm run a command.
 *  \param command Command to run.
 *  \param message Extra message passed on command line.
 *  \result None.
 */
void execCommand (char *command, char *message)
{
	char *args[12], cmdParts[81];
	int i = 0, j = 0, k = 0, q = 0;

	args[0] = NULL;

	while (command[i])
	{
		if (command[i] <= ' ' && !q)
		{
			cmdParts[i] = 0;
			k = 0;
		}
		else if (command[i] == '!')
		{
			args[j++] = message;
			args[j] = NULL;
			cmdParts[i] = 0;
			k = 0;
		}
		else
		{
			cmdParts[i] = command[i];
			cmdParts[i + 1] = 0;
			if (k == 0 && j < 10)
			{
				args[j++] = &cmdParts[i];
				args[j] = NULL;
			}
			if (command[i] == '"')
				q = !q;
			k ++;
		}
		i++;
	}
	execv (args[0], args);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  G E T  T H E  F A C E  T I M E                                                                                    *
 *  ==============================                                                                                    *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Calculate the local time for a clock face.
 *  \param faceSetting Face settings.
 *  \param t The time in secs from 1970.
 *  \param tm Put the time here.
 *  \result None.
 */
void getTheFaceTime (FACE_SETTINGS *faceSetting, time_t *t, struct tm *tm)
{
	int timeZone = faceSetting -> currentTZ;

	if (timeZones[timeZone].value == 0)
	{
		unsetenv ("TZ");
	}
	else if (timeZones[timeZone].value < FIRST_CITY)
	{
		setenv ("TZ", "GMT", 1);
		*t += 3600 * (timeZones[timeZone].value - GMT_ZERO);
	}
	else
	{
		setenv ("TZ", timeZones[timeZone].envName, 1);
	}
	tzset ();
	localtime_r (t, tm);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  C H E C K  F O R  A L A R M                                                                                       *
 *  ===========================                                                                                       *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Check to see if alarm should go off.
 *  \param faceSetting Face settings.
 *  \param tm Current time.
 *  \result None.
 */
void checkForAlarm (FACE_SETTINGS *faceSetting, struct tm *tm)
{
	if (faceSetting -> alarm)
	{
		if (faceSetting -> alarmInfo.alarmHour == tm -> tm_hour && faceSetting -> alarmInfo.alarmMin == tm -> tm_min)
		{
			if (!faceSetting -> alarmInfo.alarmShown)
			{
				NotifyNotification *note;
				char name[40] = "TzClock Message", message[40];
				GError *error = NULL;

				faceSetting -> alarmInfo.alarmShown = 1;
				if (faceSetting -> alarmInfo.onlyWeekdays && (tm -> tm_wday == 0 || tm -> tm_wday == 6))
					return;

				if (faceSetting -> alarmInfo.command[0])
				{
					int i = fork();
					if (i == 0)
					{
						/* I am the child */
						execCommand (faceSetting -> alarmInfo.command, faceSetting -> alarmInfo.message);
						exit (1);
					}
				}
				notify_init(name);
				sprintf (message, _("Clock Alarm (%d:%02d) -"), tm -> tm_hour, tm -> tm_min);
				note = notify_notification_new (message, faceSetting -> alarmInfo.message, NULL);
				notify_notification_set_timeout (note, 10000);
				notify_notification_set_category (note, _("Clock alarm"));
				notify_notification_set_urgency (note, NOTIFY_URGENCY_NORMAL);
				notify_notification_set_image_from_pixbuf (note, defaultIcon);
				notify_notification_show (note, &error);
				g_object_unref(G_OBJECT(note));
			}
		}
		else if (faceSetting -> alarmInfo.alarmShown)
		{
			faceSetting -> alarmInfo.alarmShown = 0;
		}
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  C H E C K  F O R  T I M E R                                                                                       *
 *  ===========================                                                                                       *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Check to see what to do when the timer is complete.
 *  \param faceSetting Which face completed.
 *  \result None.
 */
void checkForTimer (FACE_SETTINGS *faceSetting)
{
	if (!faceSetting -> timerInfo.timerShown)
	{
		NotifyNotification *note;
		char name[40] = "TzClock Message";
		GError *error = NULL;

		faceSetting -> timerInfo.timerShown = 1;

		if (faceSetting -> timerInfo.command[0])
		{
			int i = fork();
			if (i == 0)
			{
				/* I am the child */
				execCommand (faceSetting -> timerInfo.command, faceSetting -> timerInfo.message);
				exit (1);
			}
		}
		notify_init(name);
		note = notify_notification_new (_("Clock Timer Complete"), faceSetting -> timerInfo.message, NULL);
		notify_notification_set_timeout (note, 10000);
		notify_notification_set_category (note, _("Clock Timer"));
		notify_notification_set_urgency (note, NOTIFY_URGENCY_NORMAL);
		notify_notification_set_image_from_pixbuf (note, defaultIcon);
		notify_notification_show (note, &error);
		g_object_unref(G_OBJECT(note));
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  L O A D  C O L O U R                                                                                              *
 *  ====================                                                                                              *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Load new colour values.
 *  \param fromColour String colour description.
 *  \result 1 if good.
 */
int loadColour (char *fromColour)
{
	int i, colour = -1;
	char value[81];

	if (strlen (fromColour) > 3)
	{
		for (i = 2; i < MAX__COLOURS; i++)
		{
			if (!strncmp (fromColour, colourNames[i].shortName, 3))
			{
				colour = i;
				break;
			}
		}
		if (colour != -1)
		{
			strncpy (colourNames[colour].defColour, &fromColour[3], 60);
			sprintf (value, "colour_%s", colourNames[colour].shortName);
			configSetValue (value, colourNames[colour].defColour);
			return 1;
		}
	}
	return 0;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  L O A D  D A T E  F O R M A T                                                                                     *
 *  =============================                                                                                     *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Load a new date format.
 *  \param newFormat New format description.
 *  \result None.
 */
void loadDateFormat (char *newFormat)
{
	int i = 0, j = 0, format = -1;
	char foundStr[101], value[101];

	if (strlen (newFormat) >= 4)
	{
		if (newFormat[3] == ':')
		{
			for (i = 0; i < TXT_COUNT; i++)
			{
				if (!strncmp (newFormat, nameFormats[i], 3))
				{
					format = i;
					break;
				}
			}
			if (format != -1)
			{
				i = 4;
				foundStr[j] = 0;

				while (newFormat[i] >= ' ' && j < 100)
				{
					foundStr[j++] = newFormat[i++];
					foundStr[j] = 0;
				}
				strcpy (displayString[format], foundStr);
				sprintf (value, "text_format_%s", nameFormats[format]);
				configSetValue (value, displayString[format]);
				return;
			}
		}
	}
	howTo (stderr, _("ERROR: Invalid date format specified (%s)\n"), newFormat);
	exit (0);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  L O A D  A L A R M  I N F O                                                                                       *
 *  ===========================                                                                                       *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Load a new alarm config.
 *  \param face Which face is this for.
 *  \param buff Config to load.
 *  \result None.
 */
void loadAlarmInfo (int face, char *buff)
{
	int alHour = 0, alMin = 0, i = 0, j = 0, m = 0;
	char msg[41];

	msg[0] = 0;
	while (buff[i] && m < 3)
	{
		if (buff[i] == ':')
		{
			m++;
		}
		else if (m == 0 && buff[i] >= '0' && buff[i] <= '9')
		{
			alHour *= 10;
			alHour += (buff[i] - '0');
		}
		else if (m == 1 && buff[i] >= '0' && buff[i] <= '9')
		{
			alMin *= 10;
			alMin += (buff[i] - '0');
		}
		else if (m == 2 && j < 40)
		{
			msg[j++] = buff[i];
			msg[j] = 0;
		}
		i ++;
	}
	if (msg[0] && alHour < 24 && alMin < 60)
	{
		clockInst.faceSettings[face] -> alarm = TRUE;
		clockInst.faceSettings[face] -> alarmInfo.alarmHour = alHour;
		clockInst.faceSettings[face] -> alarmInfo.alarmMin = alMin;
		strcpy (clockInst.faceSettings[face] -> alarmInfo.message, msg);
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  L O A D  T I M E R  I N F O                                                                                       *
 *  ===========================                                                                                       *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Load the timer info from the command line.
 *  \param face Which face will have the timer.
 *  \param buff Timer config.
 *  \result None.
 */
void loadTimerInfo (int face, char *buff)
{
	char value[81];
	FACE_SETTINGS *faceSetting = clockInst.faceSettings[face];

	faceSetting -> timer = !faceSetting -> timer;
	if (faceSetting -> timer)
	{
		int tmHour = 0, tmMin = 0, tmSec = 0, i = 0, j = 0, m = 0;
		char msg[41] = "";

		while (buff[i] && m < 4)
		{
			if (buff[i] == ':')
			{
				m++;
			}
			else if (m == 0 && buff[i] >= '0' && buff[i] <= '9')
			{
				tmHour *= 10;
				tmHour += (buff[i] - '0');
			}
			else if (m == 1 && buff[i] >= '0' && buff[i] <= '9')
			{
				tmMin *= 10;
				tmMin += (buff[i] - '0');
			}
			else if (m == 2 && buff[i] >= '0' && buff[i] <= '9')
			{
				tmSec *= 10;
				tmSec += (buff[i] - '0');
			}
			else if (m == 3 && j < 40)
			{
				msg[j++] = buff[i];
				msg[j] = 0;
			}
			i ++;
		}
		if (tmHour < 7 && tmMin < 60 && tmSec < 60)
		{
			faceSetting -> stopwatch = false;
			sprintf (value, "stopwatch_%d", face + 1);
			configSetBoolValue (value, false);

			faceSetting -> timerInfo.timerHour = tmHour;
			faceSetting -> timerInfo.timerMin = tmMin;
			faceSetting -> timerInfo.timerSec = tmSec;
			strcpy (faceSetting -> timerInfo.message, msg);

			faceSetting -> swStartTime = -1;
			faceSetting -> timerInfo.totalTime =
					(faceSetting -> timerInfo.timerHour * 3600) +
					(faceSetting -> timerInfo.timerMin * 60) +
					faceSetting -> timerInfo.timerSec;
			faceSetting -> timerInfo.timerShown = 1;
			faceSetting -> swRunTime = 0;
			faceSetting -> updateFace = true;
			lastTime = -1;
		}
	}
	sprintf (value, "timer_%d", face + 1);
	configSetBoolValue (value, faceSetting -> timer);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  L O A D  H A N D  I N F O                                                                                         *
 *  =========================                                                                                         *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Read the setting for the hand from the command line.
 *  \param buff Command line settings.
 *  \result None.
 */
void loadHandInfo (char *buff)
{
	int style = 0, length = 0, tail = 0, i = 0, j = 0, m = 0;
	char hand[41], value[81];

	while (buff[i] && m < 4)
	{
		if (buff[i] == ':')
		{
			m++;
		}
		else if (m == 0 && j < 40)
		{
			hand[j++] = buff[i];
			hand[j] = 0;
		}
		else if (m == 1 && buff[i] >= '0' && buff[i] <= '9')
		{
			style *= 10;
			style += (buff[i] - '0');
		}
		else if (m == 2 && buff[i] >= '0' && buff[i] <= '9')
		{
			length *= 10;
			length += (buff[i] - '0');
		}
		else if (m == 3 && buff[i] >= '0' && buff[i] <= '9')
		{
			tail *= 10;
			tail += (buff[i] - '0');
		}
		i ++;
	}
	for (i = 0; i < HAND_COUNT; i++)
	{
		if (strcmp (hand, handNames[i]) == 0)
		{
			if ((style >= 0 && style < 10) && (length > 0 && length < 40) && (tail >= 0 && tail< 40))
			{
				handStyle[i].style = style;
				handStyle[i].length = length;
				handStyle[i].tail = tail;
				handStyle[i].fillIn = 1;

				sprintf (value, "%s_hand_style", handNames[i]);
				configSetIntValue (value, handStyle[i].style);
				sprintf (value, "%s_hand_length", handNames[i]);
				configSetIntValue (value, handStyle[i].length);
				sprintf (value, "%s_hand_tail", handNames[i]);
				configSetIntValue (value, handStyle[i].tail);
				sprintf (value, "%s_hand_fill", handNames[i]);
				configSetBoolValue (value, handStyle[i].fillIn);
			}
			break;
		}
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  P R O C E S S  C O M M A N D  L I N E                                                                             *
 *  =====================================                                                                             *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Process the options on the command line.
 *  \param argc Arg count.
 *  \param argv Arg values.
 *  \param posX Clock X position.
 *  \param posY Clock Y position.
 *  \result None.
 */
void processCommandLine (int argc, char *argv[], int *posX, int *posY)
{
	int i, j, face = 0, invalidOption = 0;
	char cityName[41], value[81];
	FACE_SETTINGS *faceSetting = clockInst.faceSettings[face];

	for (i = 1; i < argc; i++)
	{
		if (argv[i][0] == '-')
		{
			switch (argv[i][1])
			{
			case 'A':							/* Configure an alarm */
				loadAlarmInfo (face, &argv[i][2]);
				break;
			case 'a':							/* Set the clock always on top */
				alwaysOnTop = !alwaysOnTop;
				configSetBoolValue ("always_on_top", alwaysOnTop);
				break;
			case 'B':
				clockInst.showBounceSec = !clockInst.showBounceSec;
				configSetBoolValue ("bounce_seconds", clockInst.showBounceSec);
				break;
			case 'b':							/* Select sub-second hand */
				faceSetting -> subSecond = !faceSetting -> subSecond;
				sprintf (value, "sub_second_%d", face + 1);
				configSetBoolValue (value, faceSetting -> subSecond);
				break;
			case 'C':							/* Specify config file, done in main */
				break;
			case 'c':							/* Select the colours */
				if (!loadColour (&argv[i][2]))
					invalidOption = 1;
				break;
			case 'd':							/* Set the date format */
				loadDateFormat (&argv[i][2]);
				break;
			case 'D':
				clockInst.clockDecorated = !clockInst.clockDecorated;
				configSetBoolValue ("decorated", clockInst.clockDecorated);
				break;
			case 'f':							/* Select the face for zone changes */
				{
					int f = atoi (&argv[i][2]);
					if (f > 0 && f <= (clockInst.dialConfig.dialWidth * clockInst.dialConfig.dialHeight))
					{
						face = f - 1;
						clockInst.currentFace = face;
						faceSetting = clockInst.faceSettings[face];
					}
					else
						invalidOption = 1;
				}
				break;
			case 'F':							/* Select the font to be used */
				strncpy (clockInst.fontName, &argv[i][2], 99);
				configSetValue ("font_name", clockInst.fontName);
				break;
			case 'g':
				clockInst.dialConfig.dialGradient = atoi (&argv[i][2]);
				if (clockInst.dialConfig.dialGradient < 0) clockInst.dialConfig.dialGradient = 0;
				if (clockInst.dialConfig.dialGradient > 100) clockInst.dialConfig.dialGradient = 100;
				configSetIntValue ("gradient", clockInst.dialConfig.dialGradient);
				break;
			case 'h':							/* Select sub-second hand */
				faceSetting -> showSeconds = !faceSetting -> showSeconds;
				sprintf (value, "show_seconds_%d", face + 1);
				configSetBoolValue (value, faceSetting -> showSeconds);
				break;
			case 'H':							/* Set the hand style, length and tail */
				loadHandInfo (&argv[i][2]);
				break;
			case 'l':							/* Lock the clocks position */
				lockMove = !lockMove;
				configSetBoolValue ("locked_position", lockMove);
				break;
			case 'm':							/* What to show at 12, 3, 6, 9 */
				if (argv[i][2] >= '0' && argv[i][2] <= '9')
				{
					clockInst.dialConfig.markerType = (argv[i][2] - '0');
					if (argv[i][3] >= '1' && argv[i][3] <= '9')
						clockInst.dialConfig.markerStep = (argv[i][3] - '0');
				}
				configSetIntValue ("clock_mark_type", clockInst.dialConfig.markerType);
				configSetIntValue ("clock_mark_step", clockInst.dialConfig.markerStep);
				break;
			case 'n':							/* Set the number of ... */
				switch (argv[i][2])
				{
				case 'c':						/* ... columns */
					{
						int c = atoi (&argv[i][3]);
						if (c > 0 && c <= 10 && c * clockInst.dialConfig.dialHeight <= MAX_FACES)
							configSetIntValue ("clock_num_col", clockInst.dialConfig.dialWidth = c);
						else
							invalidOption = 1;
					}
					break;
				case 'r':						/* ... rows */
					{
						int r = atoi (&argv[i][3]);
						if (r > 0 && r <= 10 && r * clockInst.dialConfig.dialWidth <= MAX_FACES)
							configSetIntValue ("clock_num_row", clockInst.dialConfig.dialHeight = r);
						else
							invalidOption = 1;
					}
					break;
				default:
					invalidOption = 1;
					break;
				}
				for (j = 0; j < (clockInst.dialConfig.dialWidth * clockInst.dialConfig.dialHeight); ++j)
				{
					if (clockInst.faceSettings[j] == NULL)
					{
						clockInst.faceSettings[j] = malloc (sizeof (FACE_SETTINGS));
						memset (clockInst.faceSettings[j], 0, sizeof (FACE_SETTINGS));
					}
				}
				break;
			case 'N':							/* Force the clock to show a fixed time */
				clockInst.forceTime = atoi (&argv[i][2]);
				break;
			case 'o':							/* Overwrite current city name */
				strcpy (cityName, "x/");
				strncat (cityName, &argv[i][2], 25);
				splitTimeZone (cityName, NULL, NULL, faceSetting -> overwriteMesg, 0);
				sprintf (value, "overwrite_city_%d", face + 1);
				configSetValue (value, faceSetting -> overwriteMesg);
				break;
			case 'O':
				clockInst.dialConfig.dialOpacity = atoi (&argv[i][2]);
				if (clockInst.dialConfig.dialOpacity < 0) clockInst.dialConfig.dialOpacity = 0;
				if (clockInst.dialConfig.dialOpacity > 100) clockInst.dialConfig.dialOpacity = 100;
				configSetIntValue ("opacity", clockInst.dialConfig.dialOpacity);
				break;
			case 'q':							/* Quick time setting */
				clockInst.fastSetting = !clockInst.fastSetting;
				configSetBoolValue ("fast_setting", clockInst.fastSetting);
				break;
/**
			case 'r':
				clockInst.reConfigTime = ((time (NULL) / 300) * 300) + 300;
				break;
**/
			case 'S':							/* Enable the stopwatch */
				faceSetting -> stopwatch = !faceSetting -> stopwatch;
				if (faceSetting -> stopwatch)
				{
					faceSetting -> timer = false;
					sprintf (value, "timer_%d", face + 1);
					configSetBoolValue (value, false);
				}
				sprintf (value, "stopwatch_%d", face + 1);
				configSetBoolValue (value, faceSetting);
				break;
			case 's':							/* Select the clockInst.dialConfig.dialSize of the clock */
				clockInst.dialConfig.dialSize = ((atoi (&argv[i][2]) + 63) / 64) * 64;
				if (clockInst.dialConfig.dialSize < 64) clockInst.dialConfig.dialSize = 64;
				if (clockInst.dialConfig.dialSize > 1024) clockInst.dialConfig.dialSize = 1024;
				configSetIntValue ("face_size", clockInst.dialConfig.dialSize);
				break;
			case 't':
				clockInst.removeTaskbar = !clockInst.removeTaskbar;
				configSetBoolValue ("remove_taskbar", clockInst.removeTaskbar);
				break;
			case 'T':							/* Force the clock to show a fixed time */
				loadTimerInfo (face, &argv[i][2]);
				break;
			case 'u':							/* Uppercase the city name */
				faceSetting -> upperCity = !faceSetting -> upperCity;
				sprintf (value, "uppercase_city_%d", face + 1);
				configSetBoolValue (value, faceSetting -> upperCity);
				break;
			case 'U':							/* Show sub text */
				clockInst.showSubText = !clockInst.showSubText;
				configSetBoolValue ("show_sub_text", clockInst.showSubText);
				break;
			case 'V':
				clockInst.allowSaveDisp = !clockInst.allowSaveDisp;
				break;
			case 'w':							/* Show on all workspaces */
				stuckOnAll = !stuckOnAll;
				configSetBoolValue ("on_all_desktops", stuckOnAll);
				break;
			case 'x':							/* Set the x position for the clock */
				if (argv[i][2] == 'c')
					*posX = -2;
				else if (argv[i][2] == 'r')
					*posX = -3;
				else
				{
					int x = atoi (&argv[i][2]);
					if (x >= 0)
						*posX = x;
				}
				break;
			case 'y':							/* Set the y position for the clock */
				if (argv[i][2] == 'c')
					*posY = -2;
				else if (argv[i][2] == 'b')
					*posY = -3;
				else
				{
					int y = atoi (&argv[i][2]);
					if (y >= 0)
						*posY = y;
				}
				break;
			case 'z':							/* Select which timezone to show */
				{
					for (j = 0; j < nTimeZones; j++)
					{
						splitTimeZone (timeZones[j].envName, NULL, cityName, NULL, 0);
						if (strcasecmp (&argv[i][2], cityName) == 0)
						{
							faceSetting -> currentTZ = j;
							break;
						}
					}
					if (j == nTimeZones)
						invalidOption = 1;
				}
				break;
			case '2':							/* Display 24 hour clock */
				if (argv[i][2] == '4')
				{
					faceSetting -> show24Hour = !faceSetting -> show24Hour;
					sprintf (value, "show_24_hour_%d", face + 1);
					configSetBoolValue (value, faceSetting -> show24Hour);
				}
				else
				{
					invalidOption = 1;
				}
				break;
			case '?':							/* Display the help information */
				howTo (stderr, NULL);
				exit (0);
			default:
				howTo (stderr, _("ERROR: Unrecognised option '%s'\n"), argv[i]);
				exit (0);
			}
		}
		else
			invalidOption = 1;

		if (invalidOption)
		{
			howTo (stderr, _("ERROR: Please check option '%s'\n"), argv[i]);
			exit (0);
		}
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  U P D A T E  C L O C K                                                                                            *
 *  ======================                                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Called by dial library when settings change.
 *  \result None.
 */
void updateClock (void)
{
	int i;

	for (i = 0; i < (clockInst.dialConfig.dialHeight * clockInst.dialConfig.dialWidth); i++)
	{
		if (clockInst.faceSettings[i] == NULL)
		{
			clockInst.faceSettings[i] = malloc (sizeof (FACE_SETTINGS));
			memset (clockInst.faceSettings[i], 0, sizeof (FACE_SETTINGS));
		}
	}
	i = 0;
	while (colourNames[i].shortName)
	{
		char value[81];
		sprintf (value, "colour_%s", colourNames[i].shortName);
		configSetValue (value, colourNames[i].defColour);
		++i;
	}
	configSetIntValue ("clock_num_col", clockInst.dialConfig.dialWidth);
	configSetIntValue ("clock_num_row", clockInst.dialConfig.dialHeight);
	configSetIntValue ("clock_mark_type", clockInst.dialConfig.markerType);
	configSetIntValue ("clock_mark_step", clockInst.dialConfig.markerStep);
	configSetIntValue ("face_size", clockInst.dialConfig.dialSize);
	configSetIntValue ("opacity", clockInst.dialConfig.dialOpacity);
	configSetIntValue ("gradient", clockInst.dialConfig.dialGradient);
	configSetIntValue ("marker_scale", clockInst.dialConfig.markerScale);
	configSetValue ("font_name", clockInst.fontName);
	lastTime = -1;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  L O A D  C O N F I G                                                                                              *
 *  ====================                                                                                              *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Load the config file.
 *  \param posX Clock X position.
 *  \param posY Clock Y position.
 *  \result None.
 */
void loadConfig (int *posX, int *posY)
{
	int i, j;
	char *home = getenv ("HOME");
	char configPath[1024], value[81], tempName[81];

	configLoad ("/etc/tzclockrc");
	strcpy (configPath, home);
	strcat (configPath, "/");
	strcat (configPath, clockInst.configFile);
	configLoad (configPath);

	configGetBoolValue ("always_on_top", &alwaysOnTop);
	configGetBoolValue ("on_all_desktops", &stuckOnAll);
	configGetBoolValue ("locked_position", &lockMove);
	configGetBoolValue ("fast_setting", &clockInst.fastSetting);
	configGetBoolValue ("bounce_seconds", &clockInst.showBounceSec);
	configGetBoolValue ("show_sub_text", &clockInst.showSubText);
	configGetBoolValue ("decorated", &clockInst.clockDecorated);
	configGetBoolValue ("remove_taskbar", &clockInst.removeTaskbar);
	configGetIntValue ("face_size", &clockInst.dialConfig.dialSize);
	configGetIntValue ("clock_num_col", &clockInst.dialConfig.dialWidth);
	configGetIntValue ("clock_num_row", &clockInst.dialConfig.dialHeight);
	configGetIntValue ("clock_current", &clockInst.currentFace);
	configGetIntValue ("clock_mark_type", &clockInst.dialConfig.markerType);
	configGetIntValue ("clock_mark_step", &clockInst.dialConfig.markerStep);
	configGetIntValue ("opacity", &clockInst.dialConfig.dialOpacity);
	configGetIntValue ("gradient", &clockInst.dialConfig.dialGradient);
	configGetIntValue ("clock_x_pos", posX);
	configGetIntValue ("clock_y_pos", posY);
	configGetIntValue ("marker_scale", &clockInst.dialConfig.markerScale);
	configGetValue ("font_name", clockInst.fontName, 100);

	for (i = 2; i < MAX__COLOURS; i++)
	{
		sprintf (value, "colour_%s", colourNames[i].shortName);
		strcpy (tempName, colourNames[i].shortName);
		configGetValue (value, &tempName[strlen (tempName)], 60);
		loadColour (tempName);
	}
	for (i = 0; i < TXT_COUNT; i++)
	{
		sprintf (value, "text_format_%s", nameFormats[i]);
		configGetValue (value, displayString[i], 100);
	}
	for (i = 0; i < HAND_COUNT; i++)
	{
		sprintf (value, "%s_hand_style", handNames[i]);
		configGetIntValue (value, &handStyle[i].style);
		sprintf (value, "%s_hand_length", handNames[i]);
		configGetIntValue (value, &handStyle[i].length);
		sprintf (value, "%s_hand_tail", handNames[i]);
		configGetIntValue (value, &handStyle[i].tail);
		sprintf (value, "%s_hand_fill", handNames[i]);
		configGetBoolValue (value, &handStyle[i].fillIn);
	}
	for (i = 0; i < (clockInst.dialConfig.dialWidth * clockInst.dialConfig.dialHeight); i++)
	{
		if (clockInst.faceSettings[i] == NULL)
		{
			clockInst.faceSettings[i] = malloc (sizeof (FACE_SETTINGS));
			memset (clockInst.faceSettings[i], 0, sizeof (FACE_SETTINGS));
		}
		sprintf (value, "alarm_%d", i + 1);
		configGetBoolValue (value, &clockInst.faceSettings[i] -> alarm);
		sprintf (value, "alarm_hour_%d", i + 1);
		configGetIntValue (value, &clockInst.faceSettings[i] -> alarmInfo.alarmHour);
		sprintf (value, "alarm_min_%d", i + 1);
		configGetIntValue (value, &clockInst.faceSettings[i] -> alarmInfo.alarmMin);
		sprintf (value, "alarm_message_%d", i + 1);
		configGetValue (value, clockInst.faceSettings[i] -> alarmInfo.message, 40);
		sprintf (value, "alarm_command_%d", i + 1);
		configGetValue (value, clockInst.faceSettings[i] -> alarmInfo.command, 40);
		sprintf (value, "alarm_only_weekdays_%d", i + 1);
		configGetBoolValue (value, &clockInst.faceSettings[i] -> alarmInfo.onlyWeekdays);

		sprintf (value, "timer_%d", i + 1);
		configGetBoolValue (value, &clockInst.faceSettings[i] -> timer);
		sprintf (value, "timer_hour_%d", i + 1);
		configGetIntValue (value, &clockInst.faceSettings[i] -> timerInfo.timerHour);
		sprintf (value, "timer_min_%d", i + 1);
		configGetIntValue (value, &clockInst.faceSettings[i] -> timerInfo.timerMin);
		sprintf (value, "timer_sec_%d", i + 1);
		configGetIntValue (value, &clockInst.faceSettings[i] -> timerInfo.timerSec);
		sprintf (value, "timer_message_%d", i + 1);
		configGetValue (value, clockInst.faceSettings[i] -> timerInfo.message, 40);
		sprintf (value, "timer_command_%d", i + 1);
		configGetValue (value, clockInst.faceSettings[i] -> timerInfo.command, 40);
		sprintf (value, "stopwatch_%d", i + 1);
		configGetBoolValue (value, &clockInst.faceSettings[i] -> stopwatch);

		sprintf (value, "sub_second_%d", i + 1);
		configGetBoolValue (value, &clockInst.faceSettings[i] -> subSecond);
		sprintf (value, "show_time_%d", i + 1);
		configGetBoolValue (value, &clockInst.faceSettings[i] -> showTime);
		sprintf (value, "show_seconds_%d", i + 1);
		configGetBoolValue (value, &clockInst.faceSettings[i] -> showSeconds);
		sprintf (value, "show_24_hour_%d", i + 1);
		configGetBoolValue (value, &clockInst.faceSettings[i] -> show24Hour);
		sprintf (value, "uppercase_city_%d", i + 1);
		configGetBoolValue (value, &clockInst.faceSettings[i] -> upperCity);
		sprintf (value, "overwrite_city_%d", i + 1);
		configGetValue (value, clockInst.faceSettings[i] -> overwriteMesg, 24);

		sprintf (value, "timezone_city_%d", i + 1);
		configGetValue (value, configPath, 24);

		for (j = 0; j < nTimeZones; j++)
		{
			splitTimeZone (timeZones[j].envName, NULL, tempName, NULL, 0);
			if (strcasecmp (configPath, tempName) == 0)
			{
				clockInst.faceSettings[i] -> currentTZ = j;
				break;
			}
		}
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  M A I N                                                                                                           *
 *  =======                                                                                                           *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Reload the config and use the new colours.
 *  \param argc Arg count.
 *  \param argv Arg values.
 *  \result None.
 */
int
main (int argc, char *argv[])
{
	int posX = -1, posY = -1, saveFace, i;
	GtkWidget *eventBox;

	if (!dialCheckVersion (DIALSYS_VER))
	{
		fprintf (stderr, "Library versions do not match\n");
		return 1;
	}
	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, NULL);
	textdomain (PACKAGE);

	/*------------------------------------------------------------------------------------------------*
	* Initalaise the window.                                                                         *
	*------------------------------------------------------------------------------------------------*/
	gtk_init (&argc, &argv);
	g_set_application_name (PACKAGE_NAME);
	gtk_window_set_default_icon_name ("tzclock");

	clockInst.dialConfig.mainWindow = GTK_WINDOW (gtk_window_new (GTK_WINDOW_TOPLEVEL));
	gtk_window_set_title (clockInst.dialConfig.mainWindow, PACKAGE_NAME);

	for (i = 1; i < argc; i++)
	{
		if (argv[i][0] == '-')
		{
			if (argv[i][1] == 'C')
			{
				strncpy (clockInst.configFile, &argv[i][2], 80);
				clockInst.configFile[80] = 0;
			}
		}
	}

	parseZone ();
	loadConfig (&posX, &posY);
	processCommandLine (argc, argv, &posX, &posY);
	mainMenuDesc[0].subMenuDesc = timeZoneMenu;

	saveFace = clockInst.currentFace;
	for (i = 0; i < (clockInst.dialConfig.dialHeight * clockInst.dialConfig.dialWidth); i++)
	{
		clockInst.currentFace = i;
		if (clockInst.faceSettings[i] == NULL)
		{
			clockInst.faceSettings[i] = malloc (sizeof (FACE_SETTINGS));
			memset (clockInst.faceSettings[i], 0, sizeof (FACE_SETTINGS));
		}
		setTimeZoneCallback (clockInst.faceSettings[i] -> currentTZ);
		tmResetCallback (0);
		clockInst.faceSettings[i] -> showTime = 1;
		clockInst.faceSettings[i] -> swStartTime = -1;
		alarmSetAngle (i);
	}
	clockInst.currentFace = clockInst.toolTipFace = saveFace;

	/*------------------------------------------------------------------------------------------------*
	* Do all the other windows initialisation.                                                       *
	*------------------------------------------------------------------------------------------------*/
	gtk_window_set_resizable (GTK_WINDOW (clockInst.dialConfig.mainWindow), FALSE);

	/*------------------------------------------------------------------------------------------------*
	* Icon stuff.                                                                                    *
	*------------------------------------------------------------------------------------------------*/
	defaultIcon = gdk_pixbuf_new_from_xpm_data ((const char **) &TzClockIcon_xpm);

	/*------------------------------------------------------------------------------------------------*
	* Final windows configuration.                                                                   *
	*------------------------------------------------------------------------------------------------*/
	dialInit (&clockInst.dialConfig);

	/*------------------------------------------------------------------------------------------------*
	* This is the first time we can do this because we check the screen size in this routine.        *
	*------------------------------------------------------------------------------------------------*/
	dialFixFaceSize ();

	g_signal_connect (G_OBJECT (clockInst.dialConfig.drawingArea), "draw", G_CALLBACK (drawCallback), NULL);
	g_signal_connect (G_OBJECT (clockInst.dialConfig.mainWindow), "button_press_event", G_CALLBACK (windowClickCallback), NULL);
	g_signal_connect (G_OBJECT (clockInst.dialConfig.mainWindow), "key_press_event", G_CALLBACK (windowKeyCallback), NULL);
	g_signal_connect (G_OBJECT (clockInst.dialConfig.mainWindow), "key_release_event", G_CALLBACK (windowKeyCallback), NULL);
	g_signal_connect (G_OBJECT (clockInst.dialConfig.mainWindow), "destroy", G_CALLBACK (quitCallback), NULL);
	g_signal_connect (G_OBJECT (clockInst.dialConfig.mainWindow), "motion-notify-event", G_CALLBACK(userActive), NULL);

	g_signal_connect (G_OBJECT (clockInst.dialConfig.mainWindow), "focus-in-event", G_CALLBACK(focusInEvent), NULL);
	g_signal_connect (G_OBJECT (clockInst.dialConfig.mainWindow), "focus-out-event", G_CALLBACK(focusOutEvent), NULL);
	eventBox = gtk_event_box_new ();

	gtk_container_add (GTK_CONTAINER (eventBox), clockInst.dialConfig.drawingArea);
	gtk_container_add (GTK_CONTAINER (clockInst.dialConfig.mainWindow), eventBox);

	if (!clockInst.clockDecorated)
	{
		gtk_window_set_decorated (GTK_WINDOW (clockInst.dialConfig.mainWindow), FALSE);
	}
	gtk_widget_set_tooltip_markup (GTK_WIDGET (clockInst.dialConfig.mainWindow), "TzClock");

	/*------------------------------------------------------------------------------------------------*
	* Called to set any values                                                                       *
	*------------------------------------------------------------------------------------------------*/
	clockInst.accelGroup = gtk_accel_group_new ();
	gtk_window_add_accel_group (GTK_WINDOW (clockInst.dialConfig.mainWindow), clockInst.accelGroup);
	if (clockInst.removeTaskbar)
		gtk_window_set_skip_taskbar_hint (GTK_WINDOW (clockInst.dialConfig.mainWindow), TRUE);

	stickCallback (0);
	onTopCallback (0);
	if (!clockInst.clockDecorated)
	{
		lockCallback (0);
	}

	/*------------------------------------------------------------------------------------------------*
	* OK all ready lets run it!                                                                      *
	*------------------------------------------------------------------------------------------------*/
	gtk_widget_show_all (GTK_WIDGET (clockInst.dialConfig.mainWindow));
	g_timeout_add (50, clockTickCallback, NULL);
	dialSetOpacity ();

	/*------------------------------------------------------------------------------------------------*
	* Complete stuff left over from the command line                                                 *
	*------------------------------------------------------------------------------------------------*/
	if (posX != -1 && posY != -1)
	{
		int width = 1024, height = 768;

		dialGetScreenSize (&width, &height);

		if (posX == -2)
			posX = (width - (clockInst.dialConfig.dialWidth * clockInst.dialConfig.dialSize)) / 2;
		if (posY == -2)
			posY = (height - (clockInst.dialConfig.dialHeight * clockInst.dialConfig.dialSize)) / 2;
		if (posX == -3)
			posX = width - (clockInst.dialConfig.dialWidth * clockInst.dialConfig.dialSize);
		if (posY == -3)
			posY = height - (clockInst.dialConfig.dialHeight * clockInst.dialConfig.dialSize);
		if (posX > width - 64)
			posX = width - 64;
		if (posY > height - 64)
			posY = height - 64;

		gtk_window_move (clockInst.dialConfig.mainWindow, posX, posY);
	}
	prepareForPopup ();
	createMenu (mainMenuDesc, clockInst.accelGroup, FALSE);
	i = nice (5);
	gtk_main ();
	exit (0);
}


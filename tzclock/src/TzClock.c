/*----------------------------------------------------------------------------------------------------*
 *                                                                                                    *
 *  T z C L O C K . C                                                                                 *
 *  =================                                                                                 *
 *                                                                                                    *
 *  TzClock developed by Chris Knight based on glock by Eric L. Smith.                                * 
 *                                                                                                    *
 *----------------------------------------------------------------------------------------------------*
 *                                                                                                    *
 *  TzClock is free software; you can redistribute it and/or modify it under the terms of the GNU     *
 *  General Public License version 2 as published by the Free Software Foundation.  Note that I       *
 *  am not granting permission to redistribute or modify TzClock under the terms of any later         *
 *  version of the General Public License.                                                            *
 *                                                                                                    *
 *  TzClock is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without      *
 *  even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 *  GNU General Public License for more details.                                                      *
 *                                                                                                    *
 *  You should have received a copy of the GNU General Public License along with this program (in     *
 *  the file "COPYING"); if not, write to the Free Software Foundation, Inc.,                         *
 *  59 Temple Place - Suite 330, Boston, MA 02111, USA.                                               *
 *                                                                                                    *
 *----------------------------------------------------------------------------------------------------*/
/**
 *  @file
 *  @brief .
 *  @version $Id: TzClock.c 1815 2013-12-23 08:43:10Z chris $
 */
 
#include "config.h"
#include "TzClockDisp.h"
#include "ParseZone.h"

#ifndef GDK_KEY_M
#define GDK_KEY_A	GDK_A
#define GDK_KEY_M	GDK_M
#define GDK_KEY_m	GDK_m
#define GDK_KEY_Z	GDK_Z
#define GDK_KEY_0	GDK_0
#define GDK_KEY_9	GDK_9
#endif

COLOUR_DETAILS colourNames[MAX__COLOURS + 1] =
{
	{	"blk", __("Black"),							"#000000"	},	//	00
	{	"wht", __("White"),							"#FFFFFF"	},	//	01
	{	"cbf", __("Clock border when focused"),		"#505050"	},	//	02
	{	"cbn", __("Clock border not focused"),		"#6E6E6E"	},	//	03
	{	"cbi", __("Clock border inner circle"),		"#000000"	},	//	04
	{	"fce", __("Clock face colour"),				"#141414"	},	//	05
	{	"fsw", __("Stopwatch dial colour"),			"#000000"	},	//	06
	{	"txt", __("Information text colour"),		"#858585"	},	//	07
	{	"hrh", __("Hour hand outer colour"),		"#E1E1E1"	},	//	08
	{	"hhf", __("Hour hand fill colour"),			"#202020"	},	//	09
	{	"mnh", __("Minute hand outer colour"),		"#E1E1E1"	},	//	10
	{	"mhf", __("Minute hand fill colour"),		"#202020"	},	//	11
	{	"sch", __("Second hand outer colour"),		"#FF0000"	},	//	12
	{	"shf", __("Second hand fill colour"),		"#800000"	},	//	13
	{	"alh", __("Alarm hand outer colour"),		"#008000"	},	//	14
	{	"ahf", __("Alarm hand fill colour"),		"#004000"	},	//	15
	{	"swh", __("Stopwatch hand outer colour"),	"#FF0000"	},	//	16
	{	"swf", __("Stopwatch hand fill colour"),	"#800000"	},	//	17
	{	"hrm", __("Hour marker colour"),			"#646464"	},	//	18
	{	"mnm", __("Minute marker colour"),			"#3C3C3C"	},	//	19
	{	"swm", __("Stopwatch marker colour"),		"#828282"	},	//	20
	{	"qum", __("Quarter marker colour"),			"#AAAAAA"	},	//	21
	{	"qmf", __("Quarter marker fill colour"),	"#646464"	},	//	22
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
	"ttl", "ttz"
};

char displayString[TXT_COUNT][81] =
{
	"%a. %e %b.",					/* Clock top 		(localtime)		00 */
	"%#",							/* Clock top		(timezone) 		01 */
	"%e %b.%n%a.",					/* Clock SW top 	(localtime)		02 */
	"%#%n",							/* Clock SW top		(timezone) 		03 */
	"- %Z -",						/* Clock bottom		(localtime)		04 */
	"- %p -",						/* Clock bottom		(timezone) 		05 */
	"%Z",							/* Clock SW bottom	(localtime)		06 */
	"%p",							/* Clock SW bottom	(timezone) 		07 */	
	"%R, %A %e %B",					/* Window title 	(localtime)		08 */
	"%*, %R, %A %e %B",				/* Window title		(timezone) 		09 */
	"%A, %e %B %Y, %X",				/* Copy date time 	(localtime)		10 */
	"%A, %e %B %Y, %X, %*",			/* Copy date time	(timezone) 		11 */
	"%A, %e %B %Y",					/* Copy date 		(localtime)		12 */
	"%A, %e %B %Y, %*",				/* Copy date		(timezone) 		13 */
	"%X",							/* Copy time 		(localtime)		14 */
	"%X, %Z",						/* Copy time		(timezone) 		15 */
	"<b>Time</b>: %X%n"
	"<b>Date</b>: %A, %e %B%n"
	"<b>Alarm</b>: %$",				/* Tool tip		 	(localtime)		16 */
	"<b>City</b>: %*%n"
	"<b>Time</b>: %X%n"
	"<b>Date</b>: %A, %e %B"		/* Tool tip			(timezone) 		17 */
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
	{	__("Enable"),			stopwatchCallback,		NULL,				0,	NULL,	0,	0,	1	},
	{	__("Start+Stop"),		swStartCallback,		NULL,				0,	NULL,	GDK_KEY_A	},
	{	__("Reset"),			swResetCallback,		NULL,		  		0,	NULL,	GDK_KEY_Z	},
	{	NULL, 					NULL,					NULL,				0	}	
};

MENU_DESC editMenuDesc[] =
{
#if GTK_MAJOR_VERSION == 3 && GTK_MINOR_VERSION >= 10
	{	__("Copy Date & Time"),	copyCallback,			NULL,				0,	NULL,	GDK_KEY_C	},
#else
	{	__("Copy Date & Time"),	copyCallback,			NULL,				0,	GTK_STOCK_COPY	},
#endif
	{	__("Copy Date"),		copyCallback,			NULL,				1	},
	{	__("Copy Time"),		copyCallback,			NULL,				2	},
	{	NULL, 					NULL,					NULL,				0	}	
};

MENU_DESC viewMenuDesc[] =
{
	{	__("Add Column"),		dialAddDelCallback,		NULL,				1	},	/*	00	*/
	{	__("Remove Column"),	dialAddDelCallback,		NULL,				2	},	/*	01	*/
	{	__("Add Row"),			dialAddDelCallback,		NULL,				3	},	/*	02	*/
	{	__("Remove Row"),		dialAddDelCallback,		NULL,				4	},	/*	03	*/
	{	"-",					NULL,					NULL,				0	},	/*	04	*/
#if GTK_MAJOR_VERSION == 3 && GTK_MINOR_VERSION >= 10
	{	__("Zoom In"),			dialZoomCallback,		NULL,				1,	NULL,	GDK_KEY_I	},	/*	05	*/
	{	__("Zoom Out"),			dialZoomCallback,		NULL,				2,	NULL,	GDK_KEY_O	},	/*	06	*/
	{	__("Zoom Max"),			dialZoomCallback,		NULL,				3,	NULL,	GDK_KEY_X	},	/*	07	*/
#else
	{	__("Zoom In"),			dialZoomCallback,		NULL,				1,	GTK_STOCK_ZOOM_IN	},	/*	05	*/
	{	__("Zoom Out"),			dialZoomCallback,		NULL,				2,	GTK_STOCK_ZOOM_OUT	},	/*	06	*/
	{	__("Zoom Max"),			dialZoomCallback,		NULL,				3,	GTK_STOCK_ZOOM_FIT	},	/*	07	*/
#endif
	{	NULL, 					NULL,					NULL,				0	}	/*	08	*/
};

MENU_DESC markerMenuDesc[] =
{
	{	__("No Markers"),		dialMarkerCallback,		NULL,				0,		NULL,	0,	0,	1	},	/*	00	*/
	{	__("Triangles"),		dialMarkerCallback,		NULL,				1,		NULL,	0,	0,	1	},	/*	01	*/
	{	__("Circles"),			dialMarkerCallback,		NULL,				2,		NULL,	0,	0,	1	},	/*	02	*/
	{	__("Numbers"),			dialMarkerCallback,		NULL,				3,		NULL,	0,	0,	1	},	/*	03	*/
	{	__("Roman"),			dialMarkerCallback,		NULL,				4,		NULL,	0,	0,	1	},	/*	04	*/
	{	"-",					NULL,					NULL,				0		},
	{	__("Step 1"),			dialStepCallback,		NULL,				100,	NULL,	0,	0,	1	},	/*	06	*/
	{	__("Step 2"),			dialStepCallback,		NULL,				200,	NULL,	0,	0,	1	},	/*	07	*/
	{	__("Step 3"),			dialStepCallback,		NULL,				300,	NULL,	0,	0,	1	},	/*	08	*/
	{	__("Step 4"),			dialStepCallback,		NULL,				400,	NULL,	0,	0,	1	},	/*	09	*/
	{	__("Step 6"),			dialStepCallback,		NULL,				600,	NULL,	0,	0,	1	},	/*	10	*/
	{	__("Step 12"),			dialStepCallback,		NULL,				1200,	NULL,	0,	0,	1	},	/*	11	*/
	{	NULL, 					NULL,					NULL,				0		}
};

MENU_DESC prefMenuDesc[] =
{
	{	__("Always on Top"),	onTopCallback,			NULL,				1,	NULL,	0,	0,	1	},	/*	00	*/
	{	__("Always Visible"),	stickCallback,			NULL,				1,	NULL,	0,	0,	1	},	/*	01	*/
	{	__("Lock Position"),	lockCallback,			NULL,				1,	NULL,	0,	1,	1	},	/*	02	*/
	{	__("Show Seconds"),		showSecondsCallback,	NULL,				0,	NULL,	0,	0,	1	},	/*	03	*/
	{	__("Sub Seconds"),		subSecondCallback,		NULL,				0,	NULL,	0,	0,	1	},	/*	04	*/
	{	"-",					NULL,					NULL,				0	},			/*	05	*/
	{	__("Markers"),			NULL,					markerMenuDesc,		0	},			/*	06	*/
	{	__("View"),				NULL,					viewMenuDesc,		0	},			/*	07	*/
	{	__("Set-up Alarm"),		alarmCallback,			NULL,				0	},			/*  08	*/	
	{	__("Change Font"),		dialFontCallback,		NULL,				0	},			/*	09	*/
	{	__("Change Colour"),	dialColourCallback,		NULL,				0	},			/*	10	*/
	{	"-",					NULL,					NULL,				0	},			/*	11	*/
#if GTK_MAJOR_VERSION == 3 && GTK_MINOR_VERSION >= 10
	{	__("Save Preferences"),	configSaveCallback,		NULL,				0,	NULL,	GDK_KEY_S	},	/*	12	*/
#else
	{	__("Save Preferences"),	configSaveCallback,		NULL,				0,	GTK_STOCK_SAVE	},	/*	12	*/
#endif
	{	__("Save Display"),		dialSaveCallback,		NULL,				0,	NULL,	0,	1	},	/*  13  */
	{	NULL,					NULL,					NULL,				0	}			/*	14	*/
};

MENU_DESC mainMenuDesc[] =
{
	{	__("Time-zone"),		NULL,					NULL,				0	},
	{	__("Stopwatch"),		NULL,					stopWMenuDesc,		0	},
	{	__("Edit"),				NULL,					editMenuDesc,		0	},
	{	__("Calendar"),			calendarCallback,		NULL,				0	},
	{	"-",					NULL,					NULL,				0	},
	{	__("Preferences"),		NULL,					prefMenuDesc,		0	},
#if GTK_MAJOR_VERSION == 3 && GTK_MINOR_VERSION >= 10
	{	__("About"),			aboutCallback,			NULL,				0	},
	{	__("Quit"),				quitCallback,			NULL,				0,	NULL,	GDK_KEY_Q},
#else
	{	__("About"),			aboutCallback,			NULL,				0,	GTK_STOCK_ABOUT	},
	{	__("Quit"),				quitCallback,			NULL,				0,	GTK_STOCK_QUIT	},
#endif
	{	NULL, 					NULL,					NULL,				0	}
};

/*----------------------------------------------------------------------------------------------------*
 *                                                                                                    *
 *----------------------------------------------------------------------------------------------------*/
static GdkPixbuf *defaultIcon;

/*----------------------------------------------------------------------------------------------------*
 *                                                                                                    *
 *----------------------------------------------------------------------------------------------------*/
static bool alwaysOnTop 		=  FALSE;			// Saved in the config file
static bool stuckOnAll 			=  FALSE;			// Saved in the config file
static bool lockMove 			=  FALSE;			// Saved in the config file
static int lastKeyPressTime		=  0;
static int keyPressFaceNum		= -1;
static int stopwatchActive		=  0;
static time_t lastTime 			= -1;
static int bounceSec			=  0;

/*----------------------------------------------------------------------------------------------------*
 *                                                                                                    *
 *----------------------------------------------------------------------------------------------------*/
GtkWindow *mainWindow;
GtkWidget *drawingArea;
GtkAccelGroup *accelGroup;
bool fastSetting				=  FALSE;		// Saved in the config file
bool showBounceSec				=  FALSE;		// Saved in the config file
int faceSize 					=  3 * 64;		// Saved in the config file
int faceWidth 					=  1;			// Saved in the config file
int faceHeight 					=  1;			// Saved in the config file
int weHaveFocus 				=  0;
int currentFace 				=  0;			// Saved in the config file
int toolTipFace					=  0;
int timeSetting					=  0;
int allowSaveDisp				=  0;
int markerType	 				=  3;			// Saved in the config file
int markerStep	 				=  300;			// Saved in the config file
int faceOpacity					=  100;			// Saved in the config file
int faceGradient				=  0;			// Saved in the config file
time_t forceTime				= -1;
FACE_SETTINGS *faceSettings[MAX_FACES];
char fontName[101]				=  "Sans";		// Saved in the config file
char configFile[81]				=  ".tzclockrc";
char windowTitle[128];
char windowToolTip[128];
HAND_STYLE handStyle[HAND_COUNT] =  				// Saved in the config file
{ 
	{ 0, 19, 5, HOUR__COLOUR, HFILL_COLOUR, 1, 0 },	// Hour hand
	{ 0, 28, 7, MIN___COLOUR, MFILL_COLOUR, 1, 0 }, // Minute hand
	{ 9, 28, 9, SEC___COLOUR, SFILL_COLOUR, 1, 0 },	// Second hand
	{ 9,  7, 0, SEC___COLOUR, SFILL_COLOUR, 1, 0 },	// Sub-second hand
	{ 9, 19, 0, ALARM_COLOUR, AFILL_COLOUR, 1, 0 },	// Alarm hand
	{ 9,  7, 0, WATCH_COLOUR, WFILL_COLOUR, 1, 0 },	// Stopwatch hands
	{ 9,  7, 0, WATCH_COLOUR, WFILL_COLOUR, 1, 0 },	// Stopwatch hands
	{ 9,  7, 0, WATCH_COLOUR, WFILL_COLOUR, 1, 0 }	// Stopwatch hands
};

/*----------------------------------------------------------------------------------------------------*
 * Prototypes for functions in the tables that are defined later.                                     *
 *----------------------------------------------------------------------------------------------------*/
static void splitTimeZone 			(char *timeZone, char *area, char *city, char *display, int doUpper);

static void processCommandLine 		(int argc, char *argv[], int *posX, int *posY);
static void howTo 					(FILE * outFile, char *format, ...);
static void checkForAlarm 			(FACE_SETTINGS *faceSetting, struct tm *tm);

static gboolean clockTickCallback	(gpointer data);
static gboolean windowClickCallback	(GtkWidget * widget, GdkEventButton * event);
static gboolean windowKeyCallback	(GtkWidget * widget, GdkEventKey * event);
static gboolean focusInEvent 		(GtkWidget *widget, GdkEventFocus *event, gpointer data);
static gboolean focusOutEvent 		(GtkWidget *widget, GdkEventFocus *event, gpointer data);
#if GTK_MAJOR_VERSION == 2
static gboolean exposeCallback 		(GtkWidget * widget, GdkEventExpose * event, gpointer data);
static int alarmCounter				=  0;
#else
static gboolean drawCallback 		(GtkWidget *widget, cairo_t *cr, gpointer data);
#endif

/**********************************************************************************************************************
 *                                                                                                                    *
 *  H O W  T O                                                                                                        *
 *  ==========                                                                                                        *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  @brief .
 *  @param outFile .
 *  @param format .
 *  @param ... .
 *  @result .
 */
void
howTo (FILE * outFile, char *format, ...)
{
	int i;
	va_list ap;

	fprintf (outFile, "------------------------------------------------------------\n");
	fprintf (outFile, _("The Clock %s\n"), VERSION);
	fprintf (outFile, "------------------------------------------------------------\n");
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
	fprintf (outFile, _("   -u              :* Toggle upper-casing the city name\n"));
	fprintf (outFile, _("   -w              :  Toggle showing on all the desktops\n"));
	fprintf (outFile, _("   -x<posn>        :  Set the X screen position\n"));
	fprintf (outFile, _("   -y<posn>        :  Set the Y screen position\n"));
	fprintf (outFile, _("                   :  Both X and Y must be set\n"));
	fprintf (outFile, _("   -z<zone>        :* Select the timezone to display\n"));
	fprintf (outFile, _("   -24             :* Toggle showing the 24 hour clock\n"));
	fprintf (outFile, _("   -?              :  This how to information\n\n"));
	fprintf (outFile, _("Options marked with '*' only effect the current face. Use\n"));
	fprintf (outFile, _("the -f<num> option to select the current face.\n"));
	fprintf (outFile, "------------------------------------------------------------\n");
	fprintf (outFile, _("Colour codes: -cnnn#RRGGBB  (nnn Colour name)\n\n"));

	for (i = 2; i < MAX__COLOURS; i++)
		fprintf (outFile, "   %s : %s\n", colourNames[i].shortName, gettext (colourNames[i].longName));
	fprintf (outFile, "------------------------------------------------------------\n");
	fprintf (outFile, _("Date format strings: -dnnn:format (nnn Format name)\n\n"));

	fprintf (outFile, _("   %s : Clock top in local time\n"), 		nameFormats[0]);
	fprintf (outFile, _("   %s : Clock top for time zone\n"), 		nameFormats[1]);
	fprintf (outFile, _("   %s : Stopwatch top in local time\n"), 	nameFormats[2]);
	fprintf (outFile, _("   %s : Stopwatch top for time zone\n"), 	nameFormats[3]);
	fprintf (outFile, _("   %s : Clock bottom in local time\n"), 	nameFormats[4]);
	fprintf (outFile, _("   %s : Clock bottom for time zone\n"), 	nameFormats[5]);
	fprintf (outFile, _("   %s : Stopwatch bottom in local time\n"), nameFormats[6]);
	fprintf (outFile, _("   %s : Stopwatch bottom for time zone\n"), nameFormats[7]);
	fprintf (outFile, _("   %s : Window title in local time\n"), 	nameFormats[8]);
	fprintf (outFile, _("   %s : Window title for time zone\n"), 	nameFormats[9]);
	fprintf (outFile, _("   %s : Copy date time in local time\n"), 	nameFormats[10]);
	fprintf (outFile, _("   %s : Copy date time for time zone\n"), 	nameFormats[11]);
	fprintf (outFile, _("   %s : Copy date in local time\n"), 		nameFormats[12]);
	fprintf (outFile, _("   %s : Copy date for time zone\n"), 		nameFormats[13]);
	fprintf (outFile, _("   %s : Copy time in local time\n"), 		nameFormats[14]);
	fprintf (outFile, _("   %s : Copy time for time zone\n"), 		nameFormats[15]);
	fprintf (outFile, _("   %s : Tool tip text for local time\n"), 	nameFormats[16]);
	fprintf (outFile, _("   %s : Tool tip text for time zone\n"), 	nameFormats[17]);
	fprintf (outFile, "------------------------------------------------------------\n");
	fprintf (outFile, _("Date format options:\n\n"));

	fprintf (outFile, _("   %%#   : Time zone city, upper-cased and wrapped\n"));
	fprintf (outFile, _("   %%*   : Time zone city\n"));
	fprintf (outFile, _("   %%@   : Time zone area\n"));
	fprintf (outFile, _("   %%&   : Stopwatch time: h:mm:ss.hh\n"));
	fprintf (outFile, _("   %%... : See man page for the date command\n"));

	fprintf (outFile, "------------------------------------------------------------\n");
	if (format)
	{
		va_start(ap, format);
		vfprintf(outFile, format, ap);
		va_end(ap);
		fprintf (outFile, "------------------------------------------------------------\n");
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S P L I T  T I M E  Z O N E                                                                                       *
 *  ===========================                                                                                       *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  @brief .
 *  @param timeZone .
 *  @param area .
 *  @param city .
 *  @param display .
 *  @param doUpper .
 *  @result .
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
 *  @brief .
 *  @param data .
 *  @result .
 */
void
quitCallback (guint data)
{
	gtk_main_quit ();
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  C O P Y  C A L L B A C K                                                                                          *
 *  ========================                                                                                          *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  @brief .
 *  @param data .
 *  @result .
 */
void
copyCallback (guint data)
{
	time_t t;
	char stringDate[101];
	GtkClipboard* clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
	int timeZone = faceSettings[currentFace] -> currentTZ;

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
		getStringValue (stringDate, 100, timeZone ? TXT_COPY_DT_Z : TXT_COPY_DT_L, currentFace, t);
		break;

	case 1:
		getStringValue (stringDate, 100, timeZone ? TXT_COPY_D_Z : TXT_COPY_D_L, currentFace, t);
		break;

	case 2:
		getStringValue (stringDate, 100, timeZone ? TXT_COPY_T_Z : TXT_COPY_T_L, currentFace, t);
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
 *  @brief .
 *  @param data .
 *  @result .
 */
void
setTimeZoneCallback (guint data)
{
	int timeZone = (int) data;
	
	faceSettings[currentFace] -> currentTZ = timeZones[timeZone].value;
	splitTimeZone (timeZones[timeZone].envName, faceSettings[currentFace] -> currentTZArea, 
			faceSettings[currentFace] -> currentTZCity, faceSettings[currentFace] -> currentTZDisp,
			faceSettings[currentFace] -> upperCity);

	lastTime = -1;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  O N  T O P  C A L L B A C K                                                                                       *
 *  ===========================                                                                                       *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  @brief .
 *  @param data .
 *  @result .
 */
void
onTopCallback (guint data)
{
	if (data) alwaysOnTop = !alwaysOnTop;
	gtk_window_set_keep_above (GTK_WINDOW (mainWindow), alwaysOnTop);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S T I C K  C A L L B A C K                                                                                        *
 *  ==========================                                                                                        *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  @brief .
 *  @param data .
 *  @result .
 */
void 
stickCallback (guint data)
{
	if (data) stuckOnAll = !stuckOnAll;
	
	if (stuckOnAll)
		gtk_window_stick (GTK_WINDOW (mainWindow));
	else
		gtk_window_unstick (GTK_WINDOW (mainWindow));
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  L O C K  C A L L B A C K                                                                                          *
 *  ========================                                                                                          *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  @brief .
 *  @param data .
 *  @result .
 */
void
lockCallback (guint data)
{
	prefMenuDesc[MENU_PREF_LOCK].disable = 0;
	if (data) lockMove = !lockMove;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  C A L E N D A R  C A L L B A C K                                                                                  *
 *  ================================                                                                                  *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  @brief .
 *  @param data .
 *  @result .
 */
void
calendarCallback (guint data)
{
	time_t t;
	GtkWidget *dialog;
#if GTK_MAJOR_VERSION != 2
	GtkWidget *contentArea;
	GtkWidget *vbox;
#endif

	int timeZone = faceSettings[currentFace] -> currentTZ;

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
	
	dialog = gtk_dialog_new_with_buttons (_("Clock calendar"), mainWindow,
						GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, 
#if GTK_MAJOR_VERSION == 3 && GTK_MINOR_VERSION >= 10
						_("Close"), 
#else
						GTK_STOCK_CLOSE, 
#endif
						GTK_RESPONSE_NONE, NULL);

#if GTK_MAJOR_VERSION == 2
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), gtk_calendar_new ());
#else
	contentArea = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 3);
	gtk_box_pack_start (GTK_BOX (contentArea), vbox, TRUE, TRUE, 0);
	gtk_container_add (GTK_CONTAINER (vbox), gtk_calendar_new ());
#endif

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
 *  @brief Set the angle of the alarm hand on a given face.
 *  @param face Face the calculate the alarm angle for.
 *  @result None.
 */
void alarmSetAngle (int face)
{
	short angle = faceSettings[face] -> show24Hour ? 
			(faceSettings[face] -> alarmInfo.alarmHour * 50)  + ((faceSettings[face] -> alarmInfo.alarmMin * 60) / 72): 
			(faceSettings[face] -> alarmInfo.alarmHour * 100) + ((faceSettings[face] -> alarmInfo.alarmMin * 60) / 36);
	if (angle != faceSettings[face] -> handPosition[HAND_ALARM])
		faceSettings[face] -> handPosition[HAND_ALARM] = angle;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  A L A R M  C A L L B A C K                                                                                        *
 *  ==========================                                                                                        *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  @brief .
 *  @param data .
 *  @result .
 */
void
alarmCallback (guint data)
{
	GtkWidget *dialog;
	GtkWidget *entry1, *entry2;
	GtkWidget *label, *check;
	GtkWidget *spinner1, *spinner2;
	GtkWidget *hbox, *vbox1, *vbox2;
	GtkAdjustment *adj;
#if GTK_MAJOR_VERSION != 2
	GtkWidget *contentArea;
#endif
	
	/*------------------------------------------------------------------------------------------------*
	 * Create the basic dialog box                                                                    *
	 *------------------------------------------------------------------------------------------------*/	
	dialog = gtk_dialog_new_with_buttons (_("Set-up alarm"), mainWindow,
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, 
#if GTK_MAJOR_VERSION == 3 && GTK_MINOR_VERSION >= 10
			_("Close"), 
#else
			GTK_STOCK_CLOSE,
#endif
			GTK_RESPONSE_ACCEPT, NULL);

#if GTK_MAJOR_VERSION == 2
	vbox1 = GTK_DIALOG (dialog)->vbox;;
	hbox = gtk_hbox_new (FALSE, 0);
#else
	contentArea = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
	vbox1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_pack_start (GTK_BOX (contentArea), vbox1, FALSE, TRUE, 0);
	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
#endif
	gtk_box_pack_start (GTK_BOX (vbox1), hbox, FALSE, TRUE, 0);
	
	/*------------------------------------------------------------------------------------------------*
	 * Add the hour spinner                                                                           *
	 *------------------------------------------------------------------------------------------------*/	
#if GTK_MAJOR_VERSION == 2
	vbox2 = gtk_vbox_new (FALSE, 0);
#else
	vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
#endif
	gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, TRUE, 0);

	label = gtk_label_new (_("Hour :"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_box_pack_start (GTK_BOX (vbox2), label, FALSE, TRUE, 0);

	adj = (GtkAdjustment *) gtk_adjustment_new 
			(faceSettings[currentFace] -> alarmInfo.alarmHour, 0, 23, 1, 4, 0);
	spinner1 = gtk_spin_button_new (adj, 0, 0);
	gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinner1), TRUE);
	gtk_spin_button_set_snap_to_ticks (GTK_SPIN_BUTTON (spinner1), TRUE);
	gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner1), TRUE);
	gtk_entry_set_max_length (GTK_ENTRY (spinner1), 2);
	gtk_box_pack_start (GTK_BOX (vbox2), spinner1, FALSE, TRUE, 0);

	/*------------------------------------------------------------------------------------------------*
	 * Add the minute spinner                                                                         *
	 *------------------------------------------------------------------------------------------------*/	
#if GTK_MAJOR_VERSION == 2
	vbox2 = gtk_vbox_new (FALSE, 0);
#else
	vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
#endif
	gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, TRUE, 5);

	label = gtk_label_new (_("Min :"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_box_pack_start (GTK_BOX (vbox2), label, FALSE, TRUE, 0);

	adj = (GtkAdjustment *) gtk_adjustment_new
			(faceSettings[currentFace] -> alarmInfo.alarmMin, 0, 59, 1, 5, 0);
	spinner2 = gtk_spin_button_new (adj, 0, 0);
	gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinner2), TRUE);
	gtk_spin_button_set_snap_to_ticks (GTK_SPIN_BUTTON (spinner2), TRUE);
	gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner2), TRUE);
	gtk_entry_set_max_length (GTK_ENTRY (spinner2), 2);
	gtk_box_pack_start (GTK_BOX (vbox2), spinner2, FALSE, TRUE, 0);

	/*------------------------------------------------------------------------------------------------*
	 * Add the message entry                                                                          *
	 *------------------------------------------------------------------------------------------------*/	
#if GTK_MAJOR_VERSION == 2
	vbox2 = gtk_vbox_new (FALSE, 0);
#else
	vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
#endif
	gtk_box_pack_start (GTK_BOX (vbox1), vbox2, TRUE, TRUE, 0);

	label = gtk_label_new (_("Show message :"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_box_pack_start (GTK_BOX(vbox2), label, FALSE, TRUE, 0);

	entry1 = gtk_entry_new ();
	gtk_entry_set_width_chars (GTK_ENTRY (entry1), 30);
	gtk_entry_set_max_length (GTK_ENTRY (entry1), 40);
	gtk_entry_set_text (GTK_ENTRY (entry1), faceSettings[currentFace] -> alarmInfo.message);
    gtk_box_pack_start (GTK_BOX(vbox2), entry1, TRUE, TRUE, 0);

	label = gtk_label_new (_("Run command :"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_box_pack_start (GTK_BOX(vbox2), label, FALSE, TRUE, 0);

	entry2 = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY (entry2), 40);
	gtk_entry_set_text (GTK_ENTRY (entry2), faceSettings[currentFace] -> alarmInfo.command);
    gtk_box_pack_start (GTK_BOX(vbox2), entry2, TRUE, TRUE, 0);

	check = gtk_check_button_new_with_label (_("Weekdays only"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), faceSettings[currentFace] -> alarmInfo.onlyWeekdays);
	gtk_box_pack_start (GTK_BOX(vbox2), check, FALSE, FALSE, 0);

	/*------------------------------------------------------------------------------------------------*
	 * Display it, if OK pressed the save the new values                                              *
	 *------------------------------------------------------------------------------------------------*/	
	gtk_widget_show_all (dialog);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
	{
		strcpy (faceSettings[currentFace] -> alarmInfo.message, gtk_entry_get_text (GTK_ENTRY(entry1)));
		strcpy (faceSettings[currentFace] -> alarmInfo.command, gtk_entry_get_text (GTK_ENTRY(entry2)));
		faceSettings[currentFace] -> alarmInfo.alarmHour = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(spinner1));
		faceSettings[currentFace] -> alarmInfo.alarmMin  = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(spinner2));
		faceSettings[currentFace] -> alarmInfo.showAlarm = (faceSettings[currentFace] -> alarmInfo.message[0] != 0);
		faceSettings[currentFace] -> alarmInfo.onlyWeekdays = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check));
		alarmSetAngle (currentFace);
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
 *  @brief .
 *  @param data .
 *  @result .
 */
void
aboutCallback (guint data)
{	
	char comment[256];
	char verString[81];

	sprintf (verString, _("Version: %s"), VERSION);
	sprintf (comment, _("Timezone Clock is a highly configurable analogue clock, capable\n"
					  "of showing the time in many different countries and cities.\n"
					  "Loaded time zones: %d"), nTimeZones - FIRST_CITY);

	/*------------------------------------------------------------------------------------------------*
	 * Nice dialog that can be used with newer versions of the GTK API.                               *
	 *------------------------------------------------------------------------------------------------*/
	gtk_show_about_dialog (mainWindow,
			"title", _("About Timezone Clock"),
#if GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION > 11)
			"program-name", _("Timezone Clock"),
#endif
			"artists", artists,
			"authors", authors,
			"comments", comment,
			"copyright", "Copyright © 2005 - 2012 Chris Knight <chris@theknight.co.uk>",
			"logo", defaultIcon,
			"version", verString,
			"website", "http://www.TzClock.org/",
			NULL);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  P R E P A R E  F O R  P O P U P                                                                                   *
 *  ===============================                                                                                   *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  @brief .
 *  @result .
 */
void prepareForPopup (void)
{
	int i;
	
	viewMenuDesc[0].disable = (faceWidth >= 10 || (faceWidth + 1) * faceHeight > MAX_FACES ? 1 : 0);
	viewMenuDesc[1].disable = (faceWidth < 2 ? 1 : 0);
	viewMenuDesc[2].disable = (faceHeight >= 10 || (faceHeight + 1) * faceWidth > MAX_FACES ? 1 : 0);
	viewMenuDesc[3].disable = (faceHeight < 2 ? 1 : 0);

	prefMenuDesc[MENU_PREF_ONTOP].checked = alwaysOnTop;
	prefMenuDesc[MENU_PREF_STUCK].checked = stuckOnAll;
	prefMenuDesc[MENU_PREF_LOCK].checked = lockMove;
	prefMenuDesc[MENU_PREF_SHOWS].checked = faceSettings[currentFace] -> showSeconds;
	prefMenuDesc[MENU_PREF_SUBS].checked = faceSettings[currentFace] -> subSecond;
	prefMenuDesc[MENU_PREF_SUBS].disable = !faceSettings[currentFace] -> showSeconds;
	prefMenuDesc[MENU_PREF_SVG].disable = !allowSaveDisp;
	
	stopWMenuDesc[MENU_STPW_ENBL].checked = faceSettings[currentFace] -> stopwatch;
	stopWMenuDesc[MENU_STPW_START].disable = !faceSettings[currentFace] -> stopwatch;
	stopWMenuDesc[MENU_STPW_RESET].disable = !faceSettings[currentFace] -> stopwatch;
	
	for (i = MENU_MARK_STRT; i <= MENU_MARK_STOP; ++i)
		markerMenuDesc[i].checked = (markerMenuDesc[i].param == markerType ? 1 : 0);
	for (i = MENU_STEP_STRT; i <= MENU_STEP_STOP; ++i)
		markerMenuDesc[i].checked = (markerMenuDesc[i].param == markerStep ? 1 : 0);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  W I N D O W  C L I C K  C A L L B A C K                                                                           *
 *  =======================================                                                                           *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  @brief .
 *  @param widget .
 *  @param event .
 *  @result .
 */
gboolean
windowClickCallback (GtkWidget * widget, GdkEventButton * event)
{
	GtkWidget *popupMenu;

	if (event->type == GDK_BUTTON_PRESS)
	{
		currentFace = ((int)event -> x / faceSize) + (((int)event -> y / faceSize) * faceWidth);
		lastTime = -1;

		switch (event->button)
		{
#ifndef CLOCK_IS_DECORATED
		case 1:	/* left button */
			if (!lockMove)
			{
				gtk_window_begin_move_drag (GTK_WINDOW (mainWindow), event->button, event->x_root,
						event->y_root, event->time);
			}
			return TRUE;
#endif
			
		case 3:	/* right button */
			prepareForPopup ();
			popupMenu = createMenu (mainMenuDesc, accelGroup, FALSE);
			gtk_menu_popup (GTK_MENU (popupMenu), NULL,	/* parent_menu_shell */
					NULL,								/* parent_menu_item */
					NULL,								/* func */
					NULL,								/* data */
					event->button, event->time);
			return TRUE;
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
 *  @brief .
 *  @param widget .
 *  @param event .
 *  @result .
 */
gboolean
windowKeyCallback (GtkWidget * widget, GdkEventKey * event)
{
	GtkWidget *popupMenu;

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
				prepareForPopup ();
				popupMenu = createMenu (mainMenuDesc, accelGroup, FALSE);
				gtk_menu_popup (GTK_MENU (popupMenu), NULL,	/* parent_menu_shell */
						NULL,								/* parent_menu_item */
						NULL,								/* func */
						NULL,								/* data */
						0, event->time);
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
		gtk_window_activate_key (GTK_WINDOW (mainWindow), event);
	}
	/*------------------------------------------------------------------------------------------------*
	 * Process key release events                                                                     *
	 *------------------------------------------------------------------------------------------------*/
	else if (event->type == GDK_KEY_RELEASE)
	{
		if (event->keyval == 0xFFE9 && keyPressFaceNum != -1)  /* Alt key released */
		{
			keyPressFaceNum --;
			if (keyPressFaceNum >= 0 && keyPressFaceNum < (faceWidth * faceHeight))
			{
				currentFace = keyPressFaceNum;
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
 *  @brief .
 *  @param face .
 *  @param faceSetting .
 *  @param tm .
 *  @param t .
 *  @result .
 */
int getHandPositions (int face, FACE_SETTINGS *faceSetting, struct tm *tm, time_t t)
{
	int i, j, tempTime, update = 0;
	char tempString[111];
	short angle;

	/*------------------------------------------------------------------------------------------------*
	 * Handle the smooth scroll time change                                                           *
	 *------------------------------------------------------------------------------------------------*/	
	tempTime = (tm -> tm_hour * 60) + tm -> tm_min;
		
	if (!fastSetting)
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
			
			timeSetting = 1;
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

		if (face == currentFace)
		{
			getStringValue (tempString, 100, faceSetting -> currentTZ ? TXT_TITLE_Z : TXT_TITLE_L, face, t);
			if (strcmp (windowTitle, tempString))
			{
				strcpy (windowTitle, tempString);
				gtk_window_set_title (GTK_WINDOW (mainWindow), windowTitle);
			}
		}
		if (face == toolTipFace)
		{
			getStringValue (tempString, 100, faceSetting -> currentTZ ? TXT_TOOLTIP_Z : TXT_TOOLTIP_L, face, t);
			if (strcmp (windowToolTip, tempString))
			{
				strcpy (windowToolTip, tempString);
#if GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION > 11)
				gtk_widget_set_tooltip_markup (GTK_WIDGET (mainWindow), windowToolTip);
#endif
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
			/*----------------------------------------------------------------------------------------*
			 * Calculate stopwatch secons hand position                                               *
			 *----------------------------------------------------------------------------------------*/	
			angle = (swTime / 100) * 20;
			if (angle != faceSetting -> handPosition[HAND_STOPWS])
			{
				faceSetting -> handPosition[HAND_STOPWS] = angle;
				update = 1;
				/*------------------------------------------------------------------------------------*
				 * Calculate stopwatch minute hand position                                           *
				 *------------------------------------------------------------------------------------*/	
				angle = (swTime / 6000) * 40;
				if (angle != faceSetting -> handPosition[HAND_STOPWM])
				{
					faceSetting -> handPosition[HAND_STOPWM] = angle;
					update = 1;
				}
			}
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
 *  @brief .
 *  @param data .
 *  @result .
 */
gboolean
clockTickCallback (gpointer data)
{
	struct tm tm;
    struct timeval tv;
	time_t t = time (NULL);
	int update = 0, i, faceCount = faceHeight * faceWidth;
	
	if (forceTime != -1)
		t = forceTime;
	if (lastTime == -1)
		update = 1;
	lastTime = t;

	tv.tv_sec = 0;
	for (i = 0; i < faceCount; ++i)
	{	
		FACE_SETTINGS *faceSetting = faceSettings[i];

		if (faceSetting -> stepping || (faceSetting -> stopwatch && faceSetting -> swStartTime != -1) || 
				faceSetting -> timeShown != t || faceSetting -> updateFace || bounceSec)
		{
			getTheFaceTime (faceSetting, &t, &tm);
			checkForAlarm (faceSetting, &tm);
			if (showBounceSec && faceSetting -> showSeconds)
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
		if (drawingArea)
		{
			GdkWindow *window = gtk_widget_get_parent_window (drawingArea);
			gdk_window_invalidate_rect (GDK_WINDOW(window), NULL, TRUE);
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
 *  @brief .
 *  @param widget .
 *  @param event .
 *  @param data .
 *  @result .
 */
gboolean 
focusInEvent (GtkWidget *widget, GdkEventFocus *event, gpointer data)
{
	lastTime = -1;
	weHaveFocus = 1;
	return TRUE;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  F O C U S  O U T  E V E N T                                                                                       *
 *  ===========================                                                                                       *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  @brief .
 *  @param widget .
 *  @param event .
 *  @param data .
 *  @result .
 */
gboolean 
focusOutEvent (GtkWidget *widget, GdkEventFocus *event, gpointer data)
{
	lastTime = -1;
	weHaveFocus = 0;
	return TRUE;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  U S E R  A C T I V E                                                                                              *
 *  ====================                                                                                              *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  @brief Track the mouse so the tooltip matches the face.
 *  @param widget Owner widget.
 *  @param event Move move event.
 *  @param data Not used.
 *  @result TRUE.
 */
gboolean
userActive (GtkWidget *widget, GdkEvent* event, gpointer data)
{
	int newFace = 0;
	gdouble dx, dy;

	gdk_event_get_coords (event, &dx, &dy);
	newFace = ((int)dx / faceSize) + (((int)dy / faceSize) * faceWidth);
	if (newFace != toolTipFace)
	{
		toolTipFace = newFace;
		lastTime = -1;
	}
	return TRUE;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S H O W  S E C O N D S  C A L L B A C K                                                                           *
 *  =======================================                                                                           *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  @brief .
 *  @param data .
 *  @result .
 */
void
showSecondsCallback (guint data)
{
	faceSettings[currentFace] -> showSeconds = !faceSettings[currentFace] -> showSeconds;
	faceSettings[currentFace] -> updateFace = true;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S U B  S E C O N D  C A L L B A C K                                                                               *
 *  ===================================                                                                               *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  @brief .
 *  @param data .
 *  @result .
 */
void
subSecondCallback (guint data)
{
	faceSettings[currentFace] -> subSecond = !faceSettings[currentFace] -> subSecond;
	faceSettings[currentFace] -> updateFace = true;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S T O P W A T C H  C A L L B A C K                                                                                *
 *  ==================================                                                                                *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  @brief .
 *  @param data .
 *  @result .
 */
void
stopwatchCallback (guint data)
{
	faceSettings[currentFace] -> stopwatch = !faceSettings[currentFace] -> stopwatch;
	faceSettings[currentFace] -> swStartTime = -1;
	faceSettings[currentFace] -> swRunTime = 0;
	faceSettings[currentFace] -> updateFace = true;
	lastTime = -1;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S W  S T A R T  C A L L B A C K                                                                                   *
 *  ===============================                                                                                   *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  @brief .
 *  @param data .
 *  @result .
 */
void
swStartCallback (guint data)
{
	if (faceSettings[currentFace] -> stopwatch)
	{
		struct timeval tv;
	
		if (faceSettings[currentFace] -> swStartTime == -1)
		{
			if (gettimeofday(&tv, NULL) == 0)
			{
				faceSettings[currentFace] -> swStartTime = (tv.tv_sec * 100) + (tv.tv_usec / 10000);
				faceSettings[currentFace] -> swStartTime -= faceSettings[currentFace] -> swRunTime;
				faceSettings[currentFace] -> swRunTime = 0;
				faceSettings[currentFace] -> updateFace = true;
				stopwatchActive ++;
				lastTime = -1;
			}
		}
		else
		{
			if (gettimeofday(&tv, NULL) == 0)
			{
				long long tempTime = (tv.tv_sec * 100) + (tv.tv_usec / 10000);
				
				tempTime -= faceSettings[currentFace] -> swStartTime;		
				faceSettings[currentFace] -> swRunTime = tempTime;
				faceSettings[currentFace] -> swStartTime = -1;
				faceSettings[currentFace] -> updateFace = true;
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
 *  @brief .
 *  @param data .
 *  @result .
 */
void
swResetCallback (guint data)
{
	if (faceSettings[currentFace] -> stopwatch)
	{
		int oldStartTime = faceSettings[currentFace] -> swStartTime;
		
		faceSettings[currentFace] -> swStartTime = -1;
		faceSettings[currentFace] -> swRunTime = 0;
		faceSettings[currentFace] -> updateFace = true;
		
		if (oldStartTime != -1)
		{
			swStartCallback (data);
		}
		lastTime = -1;
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  C O N F I G  S A V E  C A L L B A C K                                                                             *
 *  =====================================                                                                             *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  @brief .
 *  @param data .
 *  @result .
 */
void configSaveCallback (guint data)
{
	int i, posX, posY;
	char *home = getenv ("HOME");
	char configPath[1024], value[81];

	gtk_window_get_position (GTK_WINDOW (mainWindow), &posX, &posY);
	
	configSetBoolValue ("always_on_top", alwaysOnTop);
	configSetBoolValue ("on_all_desktops", stuckOnAll);
	configSetBoolValue ("locked_position", lockMove);
	configSetBoolValue ("fast_setting", fastSetting);
	configSetBoolValue ("bounce_seconds", showBounceSec);

	configSetIntValue ("face_size", faceSize);
	configSetIntValue ("number_cols", faceWidth);
	configSetIntValue ("number_rows", faceHeight);
	configSetIntValue ("current_face", currentFace);
	configSetIntValue ("marker_type", markerType);
	configSetIntValue ("marker_step", markerStep);
	configSetIntValue ("opacity", faceOpacity);
	configSetIntValue ("gradient", faceGradient);
	configSetIntValue ("x_pos", posX);
	configSetIntValue ("y_pos", posY);
	configSetValue ("font_name", fontName);

	for (i = 2; i < MAX__COLOURS; i++)
	{
		sprintf (value, "colour_%s", colourNames[i].shortName);
		configSetValue (value, colourNames[i].defColour);
	}
	for (i = 0; i < TXT_COUNT; i++)
	{
		sprintf (value, "text_format_%s", nameFormats[i]);
		configSetValue (value, displayString[i]);
	}
	for (i = 0; i < HAND_COUNT; i++)
	{
		sprintf (value, "%s_hand_style", handNames[i]);
		configSetIntValue (value, handStyle[i].style);
		sprintf (value, "%s_hand_length", handNames[i]);
		configSetIntValue (value, handStyle[i].length);
		sprintf (value, "%s_hand_tail", handNames[i]);
		configSetIntValue (value, handStyle[i].tail);
		sprintf (value, "%s_hand_fill", handNames[i]);
		configSetBoolValue (value, handStyle[i].fillIn);
	}
	for (i = 0; i < (faceWidth * faceHeight); i++)
	{
		sprintf (value, "alarm_hour_%d", i + 1);
		configSetIntValue (value, faceSettings[i] -> alarmInfo.alarmHour);
		sprintf (value, "alarm_min_%d", i + 1);
		configSetIntValue (value, faceSettings[i] -> alarmInfo.alarmMin);
		sprintf (value, "alarm_message_%d", i + 1);
		configSetValue (value, faceSettings[i] -> alarmInfo.message);
		sprintf (value, "alarm_command_%d", i + 1);
		configSetValue (value, faceSettings[i] -> alarmInfo.command);
		sprintf (value, "alarm_only_weekdays_%d", i + 1);
		configSetBoolValue (value, faceSettings[i] -> alarmInfo.onlyWeekdays);
		sprintf (value, "stopwatch_%d", i + 1);
		configSetBoolValue (value, faceSettings[i] -> stopwatch);
		sprintf (value, "sub_second_%d", i + 1);
		configSetBoolValue (value, faceSettings[i] -> subSecond);
		sprintf (value, "show_seconds_%d", i + 1);
		configSetBoolValue (value, faceSettings[i] -> showSeconds);
		sprintf (value, "show_24_hour_%d", i + 1);
		configSetBoolValue (value, faceSettings[i] -> show24Hour);
		sprintf (value, "uppercase_city_%d", i + 1);
		configSetBoolValue (value, faceSettings[i] -> upperCity);
		sprintf (value, "overwrite_city_%d", i + 1);
		configSetValue (value, faceSettings[i] -> overwriteMesg);
		sprintf (value, "timezone_city_%d", i + 1);
		configSetValue (value, faceSettings[i] -> currentTZCity);
	}
	if (home)
	{
		strcpy (configPath, home);
		strcat (configPath, "/");
		strcat (configPath, configFile);
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
 *  @brief .
 *  @param faceSetting .
 *  @result .
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
 *  G E T  S T R I N G  V A L U E                                                                                     *
 *  =============================                                                                                     *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  @brief .
 *  @param addBuffer .
 *  @param maxSize .
 *  @param stringNumber .
 *  @param face .
 *  @param timeNow .
 *  @result .
 */
char *getStringValue (char *addBuffer, int maxSize, int stringNumber, int face, time_t timeNow)
{
	FACE_SETTINGS *faceSetting = faceSettings[face];
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
				int swTime = getStopwatchTime (faceSetting);
				sprintf (tempAddStr, "%d:%02d:%02d.%02d",
						swTime / 360000, (swTime / 6000) % 60, (swTime / 100) % 60, swTime % 100);
				tempCommand[j = 0] = 0;				
				break;
			}
			case '$':
				if (faceSetting -> alarmInfo.showAlarm)
					sprintf (tempAddStr, "%d:%02d", faceSetting -> alarmInfo.alarmHour, faceSetting -> alarmInfo.alarmMin);
				else
					strcpy (tempAddStr, _("not set"));
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

#if GTK_MAJOR_VERSION == 2
/**********************************************************************************************************************
 *                                                                                                                    *
 *  E X P O S E  C A L L B A C K                                                                                      *
 *  ============================                                                                                      *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  @brief The window is exposed.
 *  @param widget Not used.
 *  @param event Not used.
 *  @param data Not used.
 *  @result None.
 */
gboolean
exposeCallback (GtkWidget *widget, GdkEventExpose* event, gpointer data)
{
	clockExpose (widget);
	return TRUE;
}
#else
/**********************************************************************************************************************
 *                                                                                                                    *
 *  D R A W  C A L L B A C K                                                                                          *
 *  ========================                                                                                          *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  @brief Handle a draw call back to display the gauge.
 *  @param widget Which widget.
 *  @param cr Cairo handle to use.
 *  @param data Not used.
 *  @result None.
 */
gboolean
drawCallback (GtkWidget *widget, cairo_t *cr, gpointer data)
{
	clockExpose (cr);
	return TRUE;
}
#endif

/**********************************************************************************************************************
 *                                                                                                                    *
 *  E X E C  C O M M A N D                                                                                            *
 *  ======================                                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  @brief .
 *  @param command .
 *  @param message .
 *  @result .
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
 *  @brief Calculate the local time for a clock face.
 *  @param faceSetting .
 *  @param t The time in secs from 1970.
 *  @param tm Put the time here.
 *  @result None.
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
	localtime (t);
	localtime_r (t, tm);
}			

#if GTK_MAJOR_VERSION == 2
/**********************************************************************************************************************
 *                                                                                                                    *
 *  A L A R M  D I A L O G  D E S T R O Y                                                                             *
 *  =====================================                                                                             *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  @brief Called to destroy the alarm dialog.
 *  @param widget Which dialog.
 *  @result None.
 */
void alarmDialogDestroy (GtkWidget *widget)
{
	if (alarmCounter)
	{
		gtk_widget_destroy (widget);
		--alarmCounter;
#ifdef STATUS_ICON
		if (!alarmCounter)
		{
			gtk_status_icon_set_blinking (statusIcon, 0);				
			gtk_status_icon_set_visible (statusIcon, 0);
		}
#endif
	}
}
#endif

/**********************************************************************************************************************
 *                                                                                                                    *
 *  C H E C K  F O R  A L A R M                                                                                       *
 *  ===========================                                                                                       *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  @brief .
 *  @param faceSetting .
 *  @param tm .
 *  @result .
 */
void checkForAlarm (FACE_SETTINGS *faceSetting, struct tm *tm)
{
	if (faceSetting -> alarmInfo.showAlarm)
	{
		if (faceSetting -> alarmInfo.alarmHour == tm -> tm_hour && faceSetting -> alarmInfo.alarmMin == tm -> tm_min)
		{
			if (!faceSetting -> alarmInfo.alarmShown)
			{
#if GTK_MAJOR_VERSION == 2
				GtkWidget* dialog;
#else
				NotifyNotification *note;
				char name[40] = "Timezone Clock Message", message[40];
				GError *error = NULL;
#endif
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
#if GTK_MAJOR_VERSION == 2
				dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW (mainWindow),
						GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
						"%d:%02d - <b>\"%s\"</b>", tm -> tm_hour, tm -> tm_min,
						faceSetting -> alarmInfo.message);
				gtk_window_set_title (GTK_WINDOW (dialog), "TzClock Alarm");
#ifdef STATUS_ICON
				if (alarmCounter == 0)
				{
					gtk_status_icon_set_visible (statusIcon, 1);
					gtk_status_icon_set_blinking (statusIcon, 1);
					gtk_status_icon_set_tooltip (statusIcon, faceSetting -> alarmInfo.message);
				}
#endif
				gdk_beep();
				alarmCounter ++;			
				gtk_window_set_keep_above (GTK_WINDOW (dialog), true);
				g_signal_connect_swapped (dialog, "response", G_CALLBACK (alarmDialogDestroy), dialog);
				gtk_widget_show_all (dialog);
#else
				notify_init(name);
				sprintf (message, _("Timezone Clock Alarm (%d:%02d) -"), tm -> tm_hour, tm -> tm_min);
				note = notify_notification_new (message, faceSetting -> alarmInfo.message, NULL);
				notify_notification_set_timeout (note, 10000);
				notify_notification_set_category (note, _("Clock alarm"));
				notify_notification_set_urgency (note, NOTIFY_URGENCY_NORMAL);
				notify_notification_set_image_from_pixbuf (note, defaultIcon);
				notify_notification_show (note, &error);
				g_object_unref(G_OBJECT(note));
#endif
		   	}
		}
	   	else if (faceSetting -> alarmInfo.alarmShown)
	   	{
	   		faceSetting -> alarmInfo.alarmShown = 0;
		}
	}
}

void updateClock (void)
{
	int i;
	
	configGetIntValue ("number_cols", &faceWidth);
	configGetIntValue ("number_rows", &faceHeight);
	configGetIntValue ("face_size", &faceSize);
	configGetIntValue ("marker_type", &markerType);
	configGetIntValue ("marker_step", &markerStep);
	configGetIntValue ("opacity", &faceOpacity);
	configGetIntValue ("gradient", &faceGradient);
	configGetValue ("font_name", fontName, 100);

	for (i = 0; i < (faceHeight * faceWidth); i++)
	{
		if (faceSettings[i] == NULL)
		{
			faceSettings[i] = malloc (sizeof (FACE_SETTINGS));
			memset (faceSettings[i], 0, sizeof (FACE_SETTINGS));
		}
	}
	lastTime = -1;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  L O A D  C O L O U R                                                                                              *
 *  ====================                                                                                              *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  @brief .
 *  @param fromColour .
 *  @result .
 */
int loadColour (char *fromColour)
{
	int i, colour = -1;
	
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
 *  @brief .
 *  @param newFormat .
 *  @result .
 */
void loadDateFormat (char *newFormat)
{
	int i = 0, j = 0, format = -1;
	char foundStr[81];

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
				
				while (newFormat[i] >= ' ' && j < 80)
				{
					foundStr[j++] = newFormat[i++];
					foundStr[j] = 0;
				}
				strcpy (displayString[format], foundStr);
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
 *  @brief .
 *  @param face .
 *  @param buff .
 *  @result .
 */
void loadAlarmInfo (int face, char *buff)
{
	int alHour = 0, alMin = 0, i = 0, j = 0, m = 0;
	char msg[41];
		
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
		faceSettings[face] -> alarmInfo.showAlarm = 1;
		faceSettings[face] -> alarmInfo.alarmHour = alHour;
		faceSettings[face] -> alarmInfo.alarmMin = alMin;
		strcpy (faceSettings[face] -> alarmInfo.message, msg);
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  L O A D  H A N D  I N F O                                                                                         *
 *  =========================                                                                                         *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  @brief Read the setting for the hand from the command line.
 *  @param buff Command line settings.
 *  @result None.
 */
void loadHandInfo (char *buff)
{
	int style = 0, length = 0, tail = 0, i = 0, j = 0, m = 0;
	char hand[41];
		
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
 *  @brief .
 *  @param argc .
 *  @param argv .
 *  @param posX .
 *  @param posY .
 *  @result .
 */
void processCommandLine (int argc, char *argv[], int *posX, int *posY)
{
	int i, j, face = 0, invalidOption = 0;
	char cityName[41];

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
				break;
			case 'B':
				showBounceSec = !showBounceSec;
				break;
			case 'b':							/* Select sub-second hand */
				faceSettings[face] -> subSecond = !faceSettings[face] -> subSecond;
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
			case 'f':							/* Select the face for zone changes */
				{
					int f = atoi (&argv[i][2]);
					if (f > 0 && f <= (faceWidth * faceHeight))
					{
						face = f - 1;
						currentFace = face;
					}
					else
						invalidOption = 1;
				}
				break;
			case 'F':							/* Select the font to be used */
				strncpy (fontName, &argv[i][2], 99);
				break;
			case 'g':
				faceGradient = atoi (&argv[i][2]);
				if (faceGradient < 0) faceGradient = 0;
				if (faceGradient > 100) faceGradient = 100;
				configSetIntValue ("gradient", faceGradient);
				break;
			case 'h':							/* Select sub-second hand */
				faceSettings[face] -> showSeconds = !faceSettings[face] -> showSeconds;
				break;
			case 'H':							/* Set the hand style, length and tail */
				loadHandInfo (&argv[i][2]);
				break;
			case 'l':							/* Lock the clocks position */
				lockMove = !lockMove;
				break;
			case 'm':							/* What to show at 12, 3, 6, 9 */
				if (argv[i][2] >= '0' && argv[i][2] <= '9')
				{
					markerType = (argv[i][2] - '0');
					if (argv[i][3] >= '1' && argv[i][3] <= '9')
						markerStep = (argv[i][3] - '0');
				}
				break;
			case 'n':							/* Set the number of ... */
				switch (argv[i][2])
				{
				case 'c':						/* ... columns */
					{
						int c = atoi (&argv[i][3]);
						if (c > 0 && c <= 10 && c * faceHeight <= MAX_FACES)
							configSetIntValue ("number_cols", faceWidth = c);
						else
							invalidOption = 1;
					}
					break;
				case 'r':						/* ... rows */
					{
						int r = atoi (&argv[i][3]);
						if (r > 0 && r <= 10 && r * faceWidth <= MAX_FACES)
							configSetIntValue ("number_rows", faceHeight = r);
						else
							invalidOption = 1;
					}
					break;
				default:
					invalidOption = 1;
					break;
				}
				for (j = 0; j < (faceWidth * faceHeight); ++j)
				{
					if (faceSettings[j] == NULL)
					{
						faceSettings[j] = malloc (sizeof (FACE_SETTINGS));
						memset (faceSettings[j], 0, sizeof (FACE_SETTINGS));
					}			
				}
				break;
			case 'o':							/* Overwrite current city name */
				strcpy (cityName, "x/");
				strncat (cityName, &argv[i][2], 25);
				splitTimeZone (cityName, NULL, NULL, faceSettings[face] -> overwriteMesg, 0);
				break;
			case 'O':
				faceOpacity = atoi (&argv[i][2]);
				break;
			case 'q':							/* Quick time setting */
				fastSetting = !fastSetting;
				break;
			case 'S':							/* Enable the stopwatch */
				faceSettings[face] -> stopwatch = !faceSettings[face] -> stopwatch;
				break;
			case 's':							/* Select the faceSize of the clock */
				faceSize = atoi (&argv[i][2]);
				configSetIntValue ("face_size", faceSize);
				break;
			case 'T':							/* Force the clock to show a fixed time */
				forceTime = atoi (&argv[i][2]);
				break;
			case 'u':							/* Uppercase the city name */
				faceSettings[face] -> upperCity = !faceSettings[face] -> upperCity;
				break;
			case 'V':
				allowSaveDisp = !allowSaveDisp;
				break;
			case 'w':							/* Show on all workspaces */
				stuckOnAll = !stuckOnAll;
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
							faceSettings[face] -> currentTZ = j;
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
					faceSettings[face] -> show24Hour = !faceSettings[face] -> show24Hour;
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
 *  L O A D  C O N F I G                                                                                              *
 *  ====================                                                                                              *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  @brief .
 *  @param posX .
 *  @param posY .
 *  @result .
 */
void loadConfig (int *posX, int *posY)
{
	int i, j;
	char *home = getenv ("HOME");
	char configPath[1024], value[81], tempName[81];
	
	configSetIntValue ("face_size", faceSize);
	configSetIntValue ("number_cols", faceWidth);
	configSetIntValue ("number_rows", faceHeight);
	configSetIntValue ("marker_type", markerType);
	configSetIntValue ("marker_step", markerStep);
	configSetIntValue ("opacity", faceOpacity);	
	configSetIntValue ("gradient", faceGradient);	
	configSetValue ("font_name", fontName);
	configSetIntValue ("x_pos", *posX);
	configSetIntValue ("y_pos", *posY);

	for (i = 2; i < MAX__COLOURS; i++)
	{
		sprintf (value, "colour_%s", colourNames[i].shortName);
		configSetValue (value, colourNames[i].defColour);
	}
	for (i = 0; i < TXT_COUNT; i++)
	{
		sprintf (value, "text_format_%s", nameFormats[i]);
		configSetValue (value, displayString[i]);
	}
	for (i = 0; i < HAND_COUNT; i++)
	{
		sprintf (value, "%s_hand_style", handNames[i]);
		configSetIntValue (value, handStyle[i].style);
		sprintf (value, "%s_hand_length", handNames[i]);
		configSetIntValue (value, handStyle[i].length);
		sprintf (value, "%s_hand_tail", handNames[i]);
		configSetIntValue (value, handStyle[i].tail);
		sprintf (value, "%s_hand_fill", handNames[i]);
		configSetBoolValue (value, handStyle[i].fillIn);
	}
	for (i = 0; i < MAX_FACES; i++)
	{
		sprintf (value, "show_seconds_%d", i + 1);
		configSetIntValue (value, 1);
	}

	configLoad ("/etc/tzclockrc");
	strcpy (configPath, home);
	strcat (configPath, "/");
	strcat (configPath, configFile);
	configLoad (configPath);

	configGetBoolValue ("always_on_top", &alwaysOnTop);
	configGetBoolValue ("on_all_desktops", &stuckOnAll);
	configGetBoolValue ("locked_position", &lockMove);
	configGetBoolValue ("fast_setting", &fastSetting);
	configGetBoolValue ("bounce_seconds", &showBounceSec);
	configGetIntValue ("face_size", &faceSize);
	configGetIntValue ("number_cols", &faceWidth);
	configGetIntValue ("number_rows", &faceHeight);
	configGetIntValue ("current_face", &currentFace);
	configGetIntValue ("marker_type", &markerType);
	configGetIntValue ("marker_step", &markerStep);
	configGetIntValue ("opacity", &faceOpacity);	
	configGetIntValue ("gradient", &faceGradient);	
	configGetIntValue ("x_pos", posX);
	configGetIntValue ("y_pos", posY);
	configGetValue ("font_name", fontName, 100);

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
		configGetValue (value, displayString[i], 80);
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
	for (i = 0; i < (faceWidth * faceHeight); i++)
	{
		if (faceSettings[i] == NULL)
		{
			faceSettings[i] = malloc (sizeof (FACE_SETTINGS));
			memset (faceSettings[i], 0, sizeof (FACE_SETTINGS));
		}
		sprintf (value, "alarm_hour_%d", i + 1);
		configGetIntValue (value, &faceSettings[i] -> alarmInfo.alarmHour);
		sprintf (value, "alarm_min_%d", i + 1);
		configGetIntValue (value, &faceSettings[i] -> alarmInfo.alarmMin);
		sprintf (value, "alarm_message_%d", i + 1);
		configGetValue (value, faceSettings[i] -> alarmInfo.message, 40);
		sprintf (value, "alarm_command_%d", i + 1);
		configGetValue (value, faceSettings[i] -> alarmInfo.command, 40);
		sprintf (value, "alarm_only_weekdays_%d", i + 1);
		configGetBoolValue (value, &faceSettings[i] -> alarmInfo.onlyWeekdays);
		faceSettings[i] -> alarmInfo.showAlarm = (faceSettings[i] -> alarmInfo.message[0] ? 1 : 0);

		sprintf (value, "stopwatch_%d", i + 1);
		configGetBoolValue (value, &faceSettings[i] -> stopwatch);
		sprintf (value, "sub_second_%d", i + 1);
		configGetBoolValue (value, &faceSettings[i] -> subSecond);
		sprintf (value, "show_seconds_%d", i + 1);
		configGetBoolValue (value, &faceSettings[i] -> showSeconds);
		sprintf (value, "show_24_hour_%d", i + 1);
		configGetBoolValue (value, &faceSettings[i] -> show24Hour);
		sprintf (value, "uppercase_city_%d", i + 1);
		configGetBoolValue (value, &faceSettings[i] -> upperCity);
		sprintf (value, "overwrite_city_%d", i + 1);
		configGetValue (value, faceSettings[i] -> overwriteMesg, 24);

		sprintf (value, "timezone_city_%d", i + 1);
		configGetValue (value, configPath, 24);

		for (j = 0; j < nTimeZones; j++)
		{
			splitTimeZone (timeZones[j].envName, NULL, tempName, NULL, 0);
			if (strcasecmp (configPath, tempName) == 0)
			{
				faceSettings[i] -> currentTZ = j;
				break;
			}
		}
	}
//	configFree ();
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  M A I N                                                                                                           *
 *  =======                                                                                                           *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  @brief .
 *  @param argc .
 *  @param argv .
 *  @result .
 */
int
main (int argc, char *argv[])
{
	int posX = -1, posY = -1, saveFace, i;
	GtkWidget *eventBox;

	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, NULL);
	textdomain (PACKAGE);

	/*------------------------------------------------------------------------------------------------*
	 * Initalaise the window.                                                                         *
	 *------------------------------------------------------------------------------------------------*/
	gtk_init (&argc, &argv);
	g_set_application_name (PACKAGE_NAME);
	gtk_window_set_default_icon_name ("tzclock");

	mainWindow = GTK_WINDOW (gtk_window_new (GTK_WINDOW_TOPLEVEL));
	gtk_window_set_title (mainWindow, PACKAGE_NAME);

	for (i = 1; i < argc; i++)
	{
		if (argv[i][0] == '-')
		{
			if (argv[i][1] == 'C')
			{
				strncpy (configFile, &argv[i][2], 80);
				configFile[80] = 0;
			}
		}
	}
	
	parseZone ();
	loadConfig (&posX, &posY);
	processCommandLine (argc, argv, &posX, &posY);
	mainMenuDesc[0].subMenuDesc = timeZoneMenu;

	saveFace = currentFace;
	for (i = 0; i < (faceHeight * faceWidth); i++)
	{
		currentFace = i;
		if (faceSettings[i] == NULL)
		{
			faceSettings[i] = malloc (sizeof (FACE_SETTINGS));
			memset (faceSettings[i], 0, sizeof (FACE_SETTINGS));
		}
		setTimeZoneCallback (faceSettings[i] -> currentTZ);
		faceSettings[i] -> swStartTime = -1;
		alarmSetAngle (i);
	}
	currentFace = toolTipFace = saveFace;
	
	/*------------------------------------------------------------------------------------------------*
	 * Do all the other windows initialisation.                                                       *
	 *------------------------------------------------------------------------------------------------*/
	gtk_window_set_resizable (GTK_WINDOW (mainWindow), FALSE);

	/*------------------------------------------------------------------------------------------------*
	 * Icon stuff.                                                                                    *
	 *------------------------------------------------------------------------------------------------*/
	defaultIcon = gdk_pixbuf_new_from_xpm_data ((const char **) &TzClockIcon_xpm);

	/*------------------------------------------------------------------------------------------------*
	 * Final windows configuration.                                                                   *
	 *------------------------------------------------------------------------------------------------*/
	drawingArea = dialInit (mainWindow, updateClock, &colourNames[0], 0, dialSave);
	
	/*------------------------------------------------------------------------------------------------*
	 * This is the first time we can do this because we check the screen size in this routine.        *
	 *------------------------------------------------------------------------------------------------*/
	dialFixFaceSize ();

#if GTK_MAJOR_VERSION == 2
	g_signal_connect (G_OBJECT (drawingArea), "expose_event", G_CALLBACK (exposeCallback), NULL);
#else
	g_signal_connect (G_OBJECT (drawingArea), "draw", G_CALLBACK (drawCallback), NULL);
#endif
	g_signal_connect (G_OBJECT (mainWindow), "button_press_event", G_CALLBACK (windowClickCallback), NULL);
	g_signal_connect (G_OBJECT (mainWindow), "key_press_event", G_CALLBACK (windowKeyCallback), NULL);
	g_signal_connect (G_OBJECT (mainWindow), "key_release_event", G_CALLBACK (windowKeyCallback), NULL);
	g_signal_connect (G_OBJECT (mainWindow), "destroy", G_CALLBACK (quitCallback), NULL);
	g_signal_connect (G_OBJECT (mainWindow), "motion-notify-event", G_CALLBACK(userActive), NULL); 

	g_signal_connect (G_OBJECT (mainWindow), "focus-in-event", G_CALLBACK(focusInEvent), NULL);
	g_signal_connect (G_OBJECT (mainWindow), "focus-out-event", G_CALLBACK(focusOutEvent), NULL);
 	eventBox = gtk_event_box_new ();

	gtk_container_add (GTK_CONTAINER (eventBox), drawingArea);
	gtk_container_add (GTK_CONTAINER (mainWindow), eventBox);

#ifndef CLOCK_IS_DECORATED
	gtk_window_set_decorated (GTK_WINDOW (mainWindow), FALSE);
#endif

	/*------------------------------------------------------------------------------------------------*
	 * Complete stuff left over from the command line                                                 *
	 *------------------------------------------------------------------------------------------------*/
	if (posX != -1 && posY != -1)
	{
		if (posX == -2)
			posX = (gdk_screen_width() - (faceWidth * faceSize)) / 2;
		if (posY == -2)
			posY = (gdk_screen_height() - (faceHeight * faceSize)) / 2;
		if (posX == -3)
			posX = gdk_screen_width() - (faceWidth * faceSize);
		if (posY == -3)
			posY = gdk_screen_height() - (faceHeight * faceSize);
		if (posX > gdk_screen_width() - 64)
			posX = gdk_screen_width() - 64;
		if (posY > gdk_screen_height() - 64)
			posY = gdk_screen_height() - 64;

		gtk_window_move (mainWindow, posX, posY);
	}
#if GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION > 11)
	gtk_widget_set_tooltip_markup (GTK_WIDGET (mainWindow), "TzClock");
#endif

	/*------------------------------------------------------------------------------------------------*
	 * Called to set any values                                                                       *
	 *------------------------------------------------------------------------------------------------*/
	accelGroup = gtk_accel_group_new ();
	gtk_window_add_accel_group (GTK_WINDOW (mainWindow), accelGroup);
	createMenu (mainMenuDesc, accelGroup, TRUE);

	stickCallback (0);
	onTopCallback (0);
#ifndef CLOCK_IS_DECORATED
	lockCallback (0);
#endif

	/*------------------------------------------------------------------------------------------------*
	 * OK all ready lets run it!                                                                      *
	 *------------------------------------------------------------------------------------------------*/
	gtk_widget_show_all (GTK_WIDGET (mainWindow));
	g_timeout_add (50, clockTickCallback, NULL);
#if GTK_MAJOR_VERSION > 2 && GTK_MINOR_VERSION > 7
	gtk_widget_set_opacity (GTK_WIDGET (mainWindow), ((double)faceOpacity) / 100);
#elif GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION > 11)
	gtk_window_set_opacity (mainWindow, ((double)faceOpacity) / 100);
#endif

	i = nice (5);
	gtk_main ();
	exit (0);
}


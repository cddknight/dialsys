/**********************************************************************************************************************
 *                                                                                                                    *
 *  T Z  C L O C K  D I S P . H                                                                                       *
 *  ===========================                                                                                       *
 *                                                                                                                    *
 *  This is free software; you can redistribute it and/or modify it under the terms of the GNU General Public         *
 *  License version 2 as published by the Free Software Foundation.  Note that I am not granting permission to        *
 *  redistribute or modify this under the terms of any later version of the General Public License.                   *
 *                                                                                                                    *
 *  This is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied        *
 *  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more     *
 *  details.                                                                                                          *
 *                                                                                                                    *
 *  You should have received a copy of the GNU General Public License along with this program (in the file            *
 *  "COPYING"); if not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111,   *
 *  USA.                                                                                                              *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \file
 *  \brief Function and structures for the clock.
 */
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <libnotify/notify.h>
#include <libintl.h>
#include <locale.h>
#include <cairo-svg.h> 
#include <dialsys.h>

#define _(String) gettext (String)
#define __(String) (String)

/*#define MAX_FACES 50 */
/*#define CLOCK_IS_DECORATED */

/*----------------------------------------------------------------------------------------------------*
 * Stuff used for picking the colours on the clock. Add to the command line.                          *
 *----------------------------------------------------------------------------------------------------*/
#define BLACK_COLOUR	0	/* fixed dont change */
#define WHITE_COLOUR	1	/* fixed dont change */
#define FACE1_COLOUR	2
#define FACE2_COLOUR	3
#define FACE3_COLOUR	4
#define FACE4_COLOUR	5
#define TEXT__COLOUR	6
#define QMARK_COLOUR	7
#define QFILL_COLOUR	8
#define HMARK_COLOUR	9

#define FACE5_COLOUR	10
#define HOUR__COLOUR	11
#define HFILL_COLOUR	12
#define MIN___COLOUR	13
#define MFILL_COLOUR	14
#define SEC___COLOUR	15
#define SFILL_COLOUR	16
#define ALARM_COLOUR	17
#define AFILL_COLOUR	18
#define WATCH_COLOUR	19
#define WFILL_COLOUR	20
#define MMARK_COLOUR	21
#define WMARK_COLOUR	22
#define MAX__COLOURS	23

/*----------------------------------------------------------------------------------------------------*
 * Setting the format of the date string                                                              *
 *----------------------------------------------------------------------------------------------------*/
#define TXT_TOP_L			0
#define TXT_TOP_Z			1
#define TXT_TOPSW_L			2
#define TXT_TOPSW_Z			3
#define TXT_BOTTOM_L		4
#define TXT_BOTTOM_Z		5
#define TXT_BOTTOMSW_L		6
#define TXT_BOTTOMSW_Z		7
#define TXT_TITLE_L			8
#define TXT_TITLE_Z			9
#define TXT_COPY_DT_L		10
#define TXT_COPY_DT_Z		11
#define TXT_COPY_D_L		12
#define TXT_COPY_D_Z		13
#define TXT_COPY_T_L		14
#define TXT_COPY_T_Z		15
#define TXT_TOOLTIP_L		16
#define TXT_TOOLTIP_Z		17
#define TXT_COUNT			18

/*----------------------------------------------------------------------------------------------------*
 *                                                                                                    *
 *----------------------------------------------------------------------------------------------------*/
#define FLAG_LOCK			1
#define FLAG_ON_TOP			2
#define FLAG_STICK			4
#define FLAG_TIPS			8
#define FLAG_STOPWATCH		16

#define TWELVEHOURS			12 * 60

#define HAND_COUNT			8
#define HAND_HOUR			0
#define HAND_MINUTE			1
#define HAND_SECS			2
#define HAND_SUBS			3
#define HAND_ALARM			4
#define HAND_STOPWT			5
#define HAND_STOPWS			6
#define HAND_STOPWM			7

#define SCALE_1				300
#define SCALE_2				600
#define SCALE_3				900
#define SCALE_4				1200
#define SCALE_0				0

/*----------------------------------------------------------------------------------------------------*
 * Defines used on the menus                                                                          *
 *----------------------------------------------------------------------------------------------------*/
#define MENU_PREF_ONTOP		0
#define MENU_PREF_STUCK		1
#define MENU_PREF_LOCK		2
#define MENU_PREF_SHOWS		3
#define MENU_PREF_SUBS		4
#define MENU_PREF_SVG		13

#define MENU_STPW_ENBL		0
#define MENU_STPW_START		1
#define MENU_STPW_RESET		2

#define MENU_MARK_STRT		0
#define MENU_MARK_STOP		4
#define MENU_STEP_STRT		6
#define MENU_STEP_STOP		11

/*----------------------------------------------------------------------------------------------------*
 * Structure to store alarm information                                                               *
 *----------------------------------------------------------------------------------------------------*/
typedef struct _alarm
{
	int alarmHour;				/* Setting */
	int alarmMin;				/* Setting */
	int alarmShown;
	int showAlarm;				/* Setting (based on message) */
	bool onlyWeekdays;			/* Setting */
	char message[41];			/* Setting */
	char command[41];			/* Setting */
}
ALARM_TIME;

/*----------------------------------------------------------------------------------------------------*
 * Structure to store face information and settings                                                   *
 *----------------------------------------------------------------------------------------------------*/
typedef struct _faceSettings 
{
	bool stopwatch;				/* Setting */
	bool subSecond;				/* Setting */
	bool showSeconds;			/* Setting */
	bool show24Hour;			/* Setting */
	bool upperCity;				/* Setting */
	bool updateFace;
	int currentTZ;				/* Setting (as zone name) */
	int swRunTime;
	int shownTime;
	int stepping;
	time_t timeShown;
	long long swStartTime;
	char currentTZArea[25];
	char currentTZCity[25];
	char currentTZDisp[25];
	char overwriteMesg[25];		/* Setting */
	short handPosition[HAND_COUNT];
	GtkWidget *drawingArea, *eventBox;
	ALARM_TIME alarmInfo;
}
FACE_SETTINGS;

typedef struct _clockInst
{
	GtkAccelGroup *accelGroup;
	bool fastSetting;			/* Saved in the config file */
	bool showBounceSec;			/* Saved in the config file */
	bool clockDecorated;		/* Saved in the config file */
	int weHaveFocus;
	int currentFace;			/* Saved in the config file */
	int toolTipFace;
	int timeSetting;
	int allowSaveDisp;
	bool removeTaskbar;
	time_t forceTime;
	time_t reConfigTime;
	char fontName[128];			/* Saved in the config file */
	char configFile[128];
	char windowTitle[128];
	char windowToolTip[128];
	DIAL_CONFIG dialConfig;
	FACE_SETTINGS *faceSettings[MAX_FACES];
}
CLOCK_INST;

/*----------------------------------------------------------------------------------------------------*
 * Store for the supported timezones                                                                  *
 *----------------------------------------------------------------------------------------------------*/
typedef struct _tzInfo
{
	char *envName;
	int value;
} 
TZ_INFO;

/*----------------------------------------------------------------------------------------------------*
 * Prototypes                                                                                         *
 *----------------------------------------------------------------------------------------------------*/
void makeWindowMask ();
void getTheFaceTime (FACE_SETTINGS *faceSetting, time_t *t, struct tm *tm);
#if GTK_MAJOR_VERSION == 2
void clockExpose (GtkWidget *widget);
#else
void clockExpose (cairo_t *cr);
#endif
void dialSave(char *fileName);
char *getStringValue (char *addBuffer, int maxSize, int stringNumber, int face, time_t timeNow);
int  xSinCos (int number, int angle, int useCos);
int  getStopwatchTime (FACE_SETTINGS *faceSetting);


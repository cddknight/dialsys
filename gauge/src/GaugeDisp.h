/**********************************************************************************************************************
 *                                                                                                                    *
 *  G A U G E  D I S P . H                                                                                            *
 *  ======================                                                                                            *
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
 *  \brief Main header file.
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
#include <locale.h>
#include <sys/time.h>
#include <time.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <libintl.h>
#include <cairo-svg.h>
#include <dialsys.h>

#define _(String) gettext (String)
#define __(String) (String)

/*#define GAUGE_IS_DECORATED */

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
#define HMARK_COLOUR	8

#define HAND1_COLOUR	9
#define HFIL1_COLOUR	10
#define HAND2_COLOUR	11
#define HFIL2_COLOUR	12
#define MAXH__COLOUR	13
#define MAXF__COLOUR	14
#define MINH__COLOUR	15
#define MINF__COLOUR	16
#define CIRC__COLOUR	17
#define CFILL_COLOUR	18
#define COLD__COLOUR	19
#define HOT___COLOUR	20
#define MAX__COLOURS	21

/*----------------------------------------------------------------------------------------------------*
 *                                                                                                    *
 *----------------------------------------------------------------------------------------------------*/
#define FLAG_LOCK				1
#define FLAG_ON_TOP				2
#define FLAG_STICK				4	

#define HAND_FIRST				0
#define HAND_SECOND				1
#define HAND_MAX				2
#define HAND_MIN				3
#define HAND_COUNT				4

#define MAX_MIN_COUNT			24

#define SCALE_1					300
#define SCALE_2					600
#define SCALE_3					900
#define SCALE_4					1200
#define SCALE_0					750

/*----------------------------------------------------------------------------------------------------*
 *                                                                                                    *
 *----------------------------------------------------------------------------------------------------*/
#define MENU_GAUGE_BATTERY		0
#define MENU_GAUGE_LOAD			1
#define MENU_GAUGE_ENTROPY		2
#define MENU_GAUGE_HARDDISK		3
#define MENU_GAUGE_MEMORY		4
#define MENU_GAUGE_MOONPHASE	5
#define MENU_GAUGE_NETWORK		6
#define MENU_GAUGE_POWER		7
#define MENU_GAUGE_SENSOR		8
#define MENU_GAUGE_THERMO		9
#define MENU_GAUGE_TIDE			10
#define MENU_GAUGE_WEATHER		11
#define MENU_GAUGE_WIFI			12

#define MENU_PREF_ONTOP			0
#define MENU_PREF_STUCK			1
#define MENU_PREF_LOCK			2
#define MENU_PREF_SVG			10

#define MENU_SENSOR_TEMP		0
#define MENU_SENSOR_FAN			1

#define MENU_MARK_STRT			0
#define MENU_MARK_STOP			1
#define MENU_STEP_STRT			3
#define MENU_STEP_STOP			6

#define FACE_REDRAW		0x0001
#define FACE_SHOWHOT	0x0002
#define FACE_SHOWCOLD	0x0004
#define FACE_HOT_COLD	0x0006
#define FACE_HC_REVS	0x0008
#define FACE_SHOW_MAX	0x0010
#define FACE_SHOW_MIN	0x0020
#define FACE_MAX_MIN	0x0030
#define FACE_SHOW_POINT	0x0040
#define DONT_SHOW		0x7FFF

#define FACESTR_TIP		0
#define FACESTR_TOP		1
#define FACESTR_BOT		2
#define FACESTR_WIN		3
#define FACESTR_COUNT	4

typedef struct _savedMaxMin
{
	time_t nextUpdateTime;
	short maxMinCount;
	short updateInterval;
	short maxMinPosn;
	short shownMaxValue;
	short shownMinValue;
	short maxBuffer[MAX_MIN_COUNT];
	short minBuffer[MAX_MIN_COUNT];
}
SAVED_MAX_MIN;

typedef struct _faceSettings 
{
	unsigned int faceFlags;
	unsigned int showFaceType;
	unsigned int faceSubType;
	short int nextUpdate;
	short int updateNum;
	char  *text[FACESTR_COUNT];
	short textSize[FACESTR_COUNT];
	float firstValue;
	float secondValue;
	short int shownFirstValue;
	short int shownSecondValue;
	SAVED_MAX_MIN savedMaxMin;
	float faceScaleMin;
	float faceScaleMax;
}
FACE_SETTINGS;

#define FACE_TYPE_CPU_LOAD		0
#define FACE_TYPE_SENSOR_TEMP	1
#define FACE_TYPE_SENSOR_FAN	2
#define FACE_TYPE_WEATHER		3
#define FACE_TYPE_MEMORY		4
#define FACE_TYPE_BATTERY		5
#define FACE_TYPE_NETWORK		6
#define FACE_TYPE_ENTROPY		7
#define FACE_TYPE_TIDE			8
#define FACE_TYPE_HARDDISK		9
#define FACE_TYPE_THERMO		10
#define FACE_TYPE_POWER			11
#define FACE_TYPE_MOONPHASE		12
#define FACE_TYPE_WIFI			13
#define FACE_TYPE_MAX			14

typedef struct _gaugeEnabled 
{
	char *gaugeName;
	int enabled;
}
GAUGE_ENABLED;

#define LOCATION_COUNT			6

/*----------------------------------------------------------------------------------------------------*
 *                                                                                                    *
 *----------------------------------------------------------------------------------------------------*/
void aboutCallback 			(guint data);
void taskbarCallback 		(guint data);
void onTopCallback 			(guint data);
void stickCallback 			(guint data);
void lockCallback 			(guint data);
void fontCallback	 		(guint data);
void colourCallback			(guint data);
void alarmCallback	 		(guint data);
void zoomCallback 			(guint data);
void faceCallback			(guint data);
void quitCallback 			(guint data);
void stepCallback			(guint data);
void loadCallback			(guint data);
void cpuCallback			(guint data);
void memoryCallback			(guint data);
void networkCallback		(guint data);
void harddiskCallback		(guint data);
void batteryCallback		(guint data);
void moonPhaseCallback		(guint data);
void wifiCallback			(guint data);
void entropyCallback		(guint data);
void tideCallback			(guint data);
void sensorTempCallback		(guint data);
void sensorFanCallback		(guint data);
void weatherCallback		(guint data);
void weatherSettings		(guint data);
void tideSettings			(guint data);
void thermometerSettings	(guint data);
void thermometerCallback	(guint data);
void powerMeterCallback		(guint data);
void powerSettings			(guint data);
void configSaveCallback		(guint data);
void dialSaveCallback		(guint data);
void gaugeReset				(int face, int type, int subType);

char *wrapText (char *inText, char top);
void makeWindowMask ();
void getTheFaceTime (int face, time_t t, struct tm *tm);
void clockExpose (cairo_t *cr);
void dialSave (char *fileName); 
char *getStringValue (char *outString1, char *outString2, int maxSize, int stringNumber, int face, time_t timeNow);
int xSinCos (int number, int angle, int useCos);
void maxMinReset (SAVED_MAX_MIN *savedMaxMin, int count, int interval);
void setFaceString (FACE_SETTINGS *faceSetting, int str, int shorten, char *format, ...);

void readCPUInit (void);
void readCPUValues (int face);
void readMemoryInit (void);
void readMemoryValues (int face);
void readNetworkInit (void);
void readNetworkValues (int face);
void readHarddiskInit (void);
void readHarddiskValues (int face);
void readBatteryInit (void);
void readBatteryValues (int face);
void readMoonPhaseInit (void);
void readMoonPhaseValues (int face);
void readWifiInit (void);
void readWifiValues (int face);
void readEntropyInit (void);
void readEntropyValues (int face);
void readTideInit (void);
void readTideValues (int face);
void readSensorInit (void);
void readSensorValues (int face);
void readWeatherInit (void);
void readWeatherValues (int face);
void readThermometerInit (void);
void readThermometerValues (int face);
void readPowerMeterInit (void);
void readPowerMeterValues (int face);
void weatherGetMaxMin (FACE_SETTINGS *faceSetting);


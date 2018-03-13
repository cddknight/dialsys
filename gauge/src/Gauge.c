/**********************************************************************************************************************
 *                                                                                                                    *
 *  G A U G E . C                                                                                                     *
 *  =============                                                                                                     *
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
 *  \brief Display a gauge.
 */
#include <libintl.h>
#include "config.h"
#include "GaugeDisp.h"

COLOUR_DETAILS colourNames[MAX__COLOURS + 1] =
{
	{	"blk", __("Black"),					"#000000"	},	//	00
	{	"wht", __("White"),					"#FFFFFF"	},	//	01

	{	"fce", __("Main face colour"),		"#141414"	},	//	02	T
	{	"bri", __("Border inner circle"),	"#000000"	},	//	03	T
	{	"brf", __("Border when focused"),	"#505050"	},	//	04	T
	{	"brn", __("Border not focused"),	"#6E6E6E"	},	//	05	T
	{	"txt", __("Information text"),		"#858585"	},	//	06	T
	{	"qum", __("Scale text"),			"#AAAAAA"	},	//	07	T
	{	"hom", __("Scale markers"),			"#646464"	},	//	08	T

	{	"gah", __("Main hand outer"),		"#E0E0E0"	},	//	09
	{	"gaf", __("Main hand fill"),		"#202020"	},	//	10
	{	"oth", __("Other hand outer"),		"#808080"	},	//	11
	{	"otf", __("Other hand fill"),		"#202020"	},	//	12
	{	"mah", __("Max value hand outer"),	"#E00000"	},	//	13
	{	"maf", __("Max value hand fill"),	"#200000"	},	//	14
	{	"mnh", __("Min value hand outer"),	"#0080E0"	},	//	15
	{	"mnf", __("Min value hand fill"),	"#002020"	},	//	16
	{	"cec", __("Centre circle outer"),	"#AA0000"	},	//	17
	{	"cef", __("Centre circle fill"),	"#640000"	},	//	18
	{	"col", __("Cold zone colour"),		"#0060A0"	},	//	19
	{	"hot", __("Hot zone colour"),		"#600000"	},	//	20
	{	NULL, NULL, ""	}
}; 

char *handNames[HAND_COUNT] =
{
	"main", "option", "max", "min"
};

#ifndef COLOUR
#define COLOUR 0
#endif

#ifndef GDK_KEY_M
#define GDK_KEY_M	GDK_M
#define GDK_KEY_m	GDK_m
#define GDK_KEY_0	GDK_0
#define GDK_KEY_9	GDK_9
#endif

/******************************************************************************************************
 * If we cannot find a stock clock icon then use this built in one.                                   *
 ******************************************************************************************************/
#include "GaugeIcon.xpm"

/******************************************************************************************************
 * Who dunit!                                                                                         *
 ******************************************************************************************************/
const gchar *authors[] =
{
	"Chris Knight <chris@theknight.co.uk>", NULL
};

const gchar *artists[] =
{
	"Annie Knight", "Sonya Knight", NULL
};

/******************************************************************************************************
 * Menu parameters are:                                                                               *
 * menuName, funcCallBack, subMenuDesc, param, stockItem, accelKey, disable, checkbox, checked        *
 ******************************************************************************************************/
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
	{	__("Numbers"),			dialMarkerCallback,		NULL,				3,		NULL,	0,	0,	1	},	/*	01	*/
	{	"-",					NULL,					NULL,				0	},							/*	02	*/
	{	__("Show 2"),			dialStepCallback,		NULL,				900,	NULL,	0,	0,	1	},	/*	03	*/
	{	__("Show 3"),			dialStepCallback,		NULL,				450,	NULL,	0,	0,	1	},	/*	04	*/
	{	__("Show 6"),			dialStepCallback,		NULL,				180,	NULL,	0,	0,	1	},	/*	05	*/
	{	__("Show all"),			dialStepCallback,		NULL,				90,		NULL,	0,	0,	1	},	/*	06	*/
	{	NULL, 					NULL,					NULL,				0	}	
};

MENU_DESC spaceMenuDesc[] =
{
	{	NULL,					harddiskCallback,		NULL,				0,	NULL,	0,	1	},
	{	NULL,					harddiskCallback,		NULL,				1,	NULL,	0,	1	},
	{	NULL,					harddiskCallback,		NULL,				2,	NULL,	0,	1	},
	{	NULL,					harddiskCallback,		NULL,				3,	NULL,	0,	1	},
	{	NULL,					harddiskCallback,		NULL,				4,	NULL,	0,	1	},
	{	NULL,					harddiskCallback,		NULL,				5,	NULL,	0,	1	},
	{	NULL,					harddiskCallback,		NULL,				6,	NULL,	0,	1	},
	{	NULL,					harddiskCallback,		NULL,				7,	NULL,	0,	1	},
	{	NULL,					harddiskCallback,		NULL,				8,	NULL,	0,	1	},
	{	NULL,					harddiskCallback,		NULL,				9,	NULL,	0,	1	},
	{	NULL, 					NULL,					NULL,				0	}	
};

MENU_DESC diskMenuDesc[] =
{
	{	__("All"),				harddiskCallback,		NULL,			0x1000,	NULL,	0,	0	},
	{	NULL,					harddiskCallback,		NULL,			0x1001,	NULL,	0,	1	},
	{	NULL,					harddiskCallback,		NULL,			0x1002,	NULL,	0,	1	},
	{	NULL,					harddiskCallback,		NULL,			0x1003,	NULL,	0,	1	},
	{	NULL,					harddiskCallback,		NULL,			0x1004,	NULL,	0,	1	},
	{	NULL,					harddiskCallback,		NULL,			0x1005,	NULL,	0,	1	},
	{	NULL,					harddiskCallback,		NULL,			0x1006,	NULL,	0,	1	},
	{	NULL,					harddiskCallback,		NULL,			0x1007,	NULL,	0,	1	},
	{	NULL,					harddiskCallback,		NULL,			0x1008,	NULL,	0,	1	},
	{	NULL,					harddiskCallback,		NULL,			0x1009,	NULL,	0,	1	},
	{	NULL,					harddiskCallback,		NULL,			0x1010,	NULL,	0,	1	},
	{	NULL, 					NULL,					NULL,			0	}	
};

MENU_DESC harddiskMenuDesc[] =
{
	{	__("Partition Space"),	NULL,					spaceMenuDesc,		0		},
	{	"-",					NULL,					NULL,				0		},
	{	__("Sector Reads"),		harddiskCallback,		NULL,				0x100	},
	{	__("Sector Writes"),	harddiskCallback,		NULL,				0x200	},
	{	"-",					NULL,					NULL,				0		},
	{	__("Which Disk"),		NULL,					diskMenuDesc,		0		},
	{	NULL, 					NULL,					NULL,				0		}	
};

MENU_DESC tideMenuDesc[] =
{
	{	__("Show Tide"),		tideCallback,			NULL,				0,	},
	{	"-",					NULL,					NULL,				0	},
	{	__("Settings"),			tideSettings,			NULL,				0	},
	{	NULL,					NULL,					NULL,				0	}	
};

MENU_DESC weatherMenuDesc[] =
{
	{	__("Temperature"),		weatherCallback,		NULL,				0	},
	{	__("Humidity"),			weatherCallback,		NULL,				1	},
	{	__("Pressure"),			weatherCallback,		NULL,				2	},
	{	__("Wind Speed"),		weatherCallback,		NULL,				3	},
	{	"-",					NULL,					NULL,				0	},
	{	__("Today"),			weatherCallback,		NULL,				4	},
	{	__("Tomorrow"),			weatherCallback,		NULL,				5	},
	{	__("Two days time"),	weatherCallback,		NULL,				6	},
	{	"-",					NULL,					NULL,				0	},
	{	__("Settings"),			weatherSettings,		NULL,				0	},
	{	NULL, 					NULL,					NULL,				0	}	
};

MENU_DESC sTempMenuDesc[] =
{
	{	__("Temp 1"),			sensorTempCallback, 	NULL,				0,	NULL,	0,	1	},
	{	__("Temp 2"),			sensorTempCallback, 	NULL,				1,	NULL,	0,	1	},
	{	__("Temp 3"),			sensorTempCallback, 	NULL,				2,	NULL,	0,	1	},
	{	__("Temp 4"),			sensorTempCallback, 	NULL,				3,	NULL,	0,	1	},
	{	__("Temp 5"),			sensorTempCallback, 	NULL,				4,	NULL,	0,	1	},
	{	__("Temp 6"),			sensorTempCallback, 	NULL,				5,	NULL,	0,	1	},
	{	__("Temp 7"),			sensorTempCallback, 	NULL,				6,	NULL,	0,	1	},
	{	__("Temp 8"),			sensorTempCallback, 	NULL,				7,	NULL,	0,	1	},
	{	NULL, 					NULL,					NULL,				0	}	
};

MENU_DESC sFanMenuDesc[] =
{
	{	__("Fan 1"),			sensorFanCallback, 		NULL,				0,	NULL,	0,	1	},
	{	__("Fan 2"),			sensorFanCallback, 		NULL,				1,	NULL,	0,	1	},
	{	__("Fan 3"),			sensorFanCallback, 		NULL,				2,	NULL,	0,	1	},
	{	__("Fan 4"),			sensorFanCallback, 		NULL,				3,	NULL,	0,	1	},
	{	__("Fan 5"),			sensorFanCallback, 		NULL,				4,	NULL,	0,	1	},
	{	__("Fan 6"),			sensorFanCallback, 		NULL,				5,	NULL,	0,	1	},
	{	__("Fan 7"),			sensorFanCallback, 		NULL,				6,	NULL,	0,	1	},
	{	__("Fan 8"),			sensorFanCallback, 		NULL,				7,	NULL,	0,	1	},
	{	NULL, 					NULL,					NULL,				0	}	
};

MENU_DESC sensorMenuDesc[] =
{
	{	__("Temperature"),		NULL,					sTempMenuDesc,		0,	NULL,	0,	1	},
	{	__("Fan Speed"),		NULL,					sFanMenuDesc,		0,	NULL,	0,	1	},
	{	NULL, 					NULL,					NULL,				0	}	
};

MENU_DESC networkDevDesc[] =
{
	{	__("All"),				networkCallback,		NULL,				0x1000	},
	{	NULL,					networkCallback,		NULL,				0x1001,	NULL,	0,	1	},
	{	NULL,					networkCallback,		NULL,				0x1002,	NULL,	0,	1	},
	{	NULL,					networkCallback,		NULL,				0x1003,	NULL,	0,	1	},
	{	NULL,					networkCallback,		NULL,				0x1004,	NULL,	0,	1	},
	{	NULL,					networkCallback,		NULL,				0x1005,	NULL,	0,	1	},
	{	NULL,					networkCallback,		NULL,				0x1006,	NULL,	0,	1	},
	{	NULL,					networkCallback,		NULL,				0x1007,	NULL,	0,	1	},
	{	NULL,					networkCallback,		NULL,				0x1008,	NULL,	0,	1	},
	{	NULL,					networkCallback,		NULL,				0x1009,	NULL,	0,	1	},
	{	NULL,					networkCallback,		NULL,				0x100A,	NULL,	0,	1	},
	{	NULL, 					NULL,					NULL,				0	}	
};

MENU_DESC networkMenuDesc[] =
{
	{	__("Bytes Sent"),		networkCallback,		NULL,				0x0000	},
	{	__("Bytes Received"),	networkCallback,		NULL,				0x0100	},
	{	"-",					NULL,					NULL,				0		},
	{	__("Which Interface"),	NULL,					networkDevDesc,		0		},
	{	NULL, 					NULL,					NULL,				0		}	
};

MENU_DESC memoryMenuDesc[] =
{
	{	__("Programs"),			memoryCallback,			NULL,				0	},
	{	__("Free"),				memoryCallback,			NULL,				1	},
	{	__("Buffers"),			memoryCallback,			NULL,				2	},
	{	__("Cached"),			memoryCallback,			NULL,				3	},
	{	__("Swap"),				memoryCallback,			NULL,				4	},
	{	NULL, 					NULL,					NULL,				0	}	
};

MENU_DESC pickCPUMenuDesc[] =
{
	{ 	__("All"),				loadCallback,			NULL,				0x1000	},
	{ 	__("CPU1"),				loadCallback,			NULL,				0x1001	},
	{ 	__("CPU2"),				loadCallback,			NULL,				0x1002,	NULL,	0,	1	},
	{ 	__("CPU3"),				loadCallback,			NULL,				0x1003,	NULL,	0,	1	},
	{ 	__("CPU4"),				loadCallback,			NULL,				0x1004,	NULL,	0,	1	},
	{ 	__("CPU5"),				loadCallback,			NULL,				0x1005,	NULL,	0,	1	},
	{ 	__("CPU6"),				loadCallback,			NULL,				0x1006,	NULL,	0,	1	},
	{ 	__("CPU7"),				loadCallback,			NULL,				0x1007,	NULL,	0,	1	},
	{ 	__("CPU8"),				loadCallback,			NULL,				0x1008,	NULL,	0,	1	},
	{	NULL, 					NULL,					NULL,				0x1000	}
};

MENU_DESC cpuMenuDesc[] =
{
	{ 	__("Average"),			loadCallback,	 		NULL,				0x2000	},
	{	"-",					NULL,					NULL,				0		},
	{ 	__("Total"),			loadCallback,	 		NULL,				0x0000	},
	{ 	__("User"),				loadCallback,	 		NULL,				0x0100	},	
	{ 	__("Nice"),				loadCallback,	 		NULL,				0x0200	},
	{ 	__("System"),			loadCallback,	 		NULL,				0x0300	},
	{ 	__("Idle"),				loadCallback,	 		NULL,				0x0400	},
	{ 	__("ioWait"),			loadCallback,	 		NULL,				0x0500	},
	{	"-",					NULL,					NULL,				0		},
	{	__("Which CPU"),		NULL,					pickCPUMenuDesc,	0		},
	{	NULL, 					NULL,					NULL,				0		}
};

MENU_DESC gaugeMenuDesc[] =
{
	{	__("Battery"),			batteryCallback,		NULL,				0,	NULL,	0,	1	},	/*	00	*/
	{	__("CPU Load"),			NULL,					cpuMenuDesc,		0,	NULL,	0,	1	},	/*	01	*/
	{	__("Entropy"),			entropyCallback,		NULL,				0,	NULL,	0,	1	},	/*	02	*/
	{	__("Hard Disk"),		NULL,					harddiskMenuDesc,	0,	NULL,	0,	1	},	/*	03	*/
	{	__("Memory"),			NULL,					memoryMenuDesc,		0,	NULL,	0,	1	},	/*	04	*/
	{	__("Moon Phase"),		moonPhaseCallback,		NULL,				0,	NULL,	0,	1	},	/*	05	*/
	{	__("Network"),			NULL,					networkMenuDesc,	0,	NULL,	0,	1	},	/*	06	*/
	{	__("Power"),			powerMeterCallback,		NULL,				0,	NULL,	0,	1	},	/*	07	*/
	{	__("Sensor"),			NULL,					sensorMenuDesc,		0,	NULL,	0,	1	},	/*	08	*/
	{	__("Thermometer"),		thermometerCallback,	NULL,				0,	NULL,	0,	1	},	/*	09	*/
	{	__("Tide"),				NULL,					tideMenuDesc,		0,	NULL,	0,	1	},	/*	10	*/
	{	__("Weather"),			NULL,					weatherMenuDesc,	0,	NULL,	0,	1	},	/*	11	*/
	{	__("Wifi Quality"),		wifiCallback,			NULL,				0,	NULL,	0,	1	},	/*	12	*/
	{	NULL,					NULL,					NULL,				0	}					/*	13	*/
};

MENU_DESC prefMenuDesc[] =
{
	{	__("Always on Top"),	onTopCallback,			NULL,				1,	NULL,	0,	0,	1	},	/*	00	*/
	{	__("Always Visible"),	stickCallback,			NULL,				1,	NULL,	0,	0,	1	},	/*	01	*/
	{	__("Lock Position"),	lockCallback,			NULL,				1,	NULL,	0,	1,	1	},	/*	02	*/
	{	"-",					NULL,					NULL,				0	},					/*	03	*/
	{	__("Markers"),			NULL,					markerMenuDesc,		0	},					/*	04	*/
	{	__("View"),				NULL,					viewMenuDesc,		0	},					/*	05	*/
	{	__("Change Font"),		dialFontCallback,		NULL,				0	},					/*	06	*/
	{	__("Change Colour"),	dialColourCallback,		NULL,				0	},					/*	07	*/
	{	"-",					NULL,					NULL,				0	},					/*	08	*/
#if GTK_MAJOR_VERSION == 3 && GTK_MINOR_VERSION >= 10
	{	__("Save Preferences"),	configSaveCallback,		NULL,				0,	NULL,	GDK_KEY_S	},	/*	12	*/
#else
	{	__("Save Preferences"),	configSaveCallback,		NULL,				0,	GTK_STOCK_SAVE	},	/*	12	*/
#endif
	{	__("Save Display"),		dialSaveCallback,		NULL,				0,	NULL,	0,	1	},	/*  10  */
	{	NULL,					NULL,					NULL,				0	}
};

MENU_DESC mainMenuDesc[] =
{
	{	__("Gauge"),			NULL,					gaugeMenuDesc,		0	},
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

/******************************************************************************************************
 *                                                                                                    *
 ******************************************************************************************************/
static GdkPixbuf *defaultIcon;

/******************************************************************************************************
 *                                                                                                    *
 ******************************************************************************************************/
static bool alwaysOnTop 		=  0;			/* Saved in the config file */
static bool stuckOnAll 			=  0;			/* Saved in the config file */
static bool lockMove 			=  0;			/* Saved in the config file */
static int lastKeyPressTime		=  0;
static int keyPressFaceNum		= -1;
static time_t lastTime 			= -1;
void updateGauge (void);

/******************************************************************************************************
 *                                                                                                    *
 ******************************************************************************************************/
GtkAccelGroup *accelGroup;
int weHaveFocus 				=  0;
int currentFace 				=  0;			/* Saved in the config file */
int toolTipFace					=  0;
int sysUpdateID					=  100;
int allowSaveDisp				=  0;
FACE_SETTINGS *faceSettings[MAX_FACES];
char fontName[101]				=  "Sans";		// Saved in the config file
char configFile[81]				=  ".gaugerc";
HAND_STYLE handStyle[HAND_COUNT]	=			/* Saved in the config file */
{ 
	{ 1, 28, 0, HAND1_COLOUR, HFIL1_COLOUR, 1, 1 },		/* First hand */
	{ 1, 25, 0, HAND2_COLOUR, HFIL2_COLOUR, 1, 1 },		/* Second hand */
	{ 9, 28, 0, MAXH__COLOUR, MAXF__COLOUR, 1, 1 },		/* Max hand */
	{ 9, 28, 0, MINH__COLOUR, MINF__COLOUR, 1, 1 }		/* Min hand */
};
char tideURL[129] = "http://www.ukho.gov.uk/easytide/EasyTide/ShowPrediction.aspx?PredictionLength=4&PortID=0113";
DIAL_CONFIG dialConfig =
{								/* -- Used by dial library -- */
	NULL, 						/* mainWindow */
	NULL, 						/* drawingArea */
	3 * 64,						/* faceSize */
	1,	 						/* faceWidth */
	1, 							/* faceHeight */
	3,	 						/* markerType */
	300, 						/* markerStep */
	99, 						/* faceOpacity */
	0,	 						/* faceGradient */
	750,							/* startPoint */
	&fontName[0],				/* Font name pointer */
	updateGauge,				/* Update func. */
	dialSave,					/* Save func. */
	&colourNames[0]				/* Colour details */
};

GAUGE_ENABLED gaugeEnabled[FACE_TYPE_MAX + 1] =
{
	{	"cpu_load",		1	},	{	"sensor_temp",	1	},	{	"sensor_fan",	1	},
	{	"weather",		1	},	{	"memory",		1	},	{	"battery",		1	},
	{	"network",		1	},	{	"entropy",		0	},	{	"tide",			1	},
	{	"harddisk",		1	},	{	"thermo",		0	},	{	"power",		0	},
	{	"moonphase",	1	},	{	"wifi",			1,	},	{	NULL, 			0	}
};

/******************************************************************************************************
 * Prototypes for functions in the tables that are defined later.                                     *
 ******************************************************************************************************/
static void processCommandLine 		(int argc, char *argv[], int *posX, int *posY);
static void howTo 					(FILE * outFile, char *format, ...);
static int  updateMaxMinValues 		(FACE_SETTINGS *faceSetting, int firstValue);

static gboolean clockTickCallback	(gpointer data);
static gboolean windowClickCallback	(GtkWidget * widget, GdkEventButton * event);
static gboolean windowKeyCallback	(GtkWidget * widget, GdkEventKey * event);
static gboolean focusInEvent 		(GtkWidget *widget, GdkEventFocus *event, gpointer data);
static gboolean focusOutEvent 		(GtkWidget *widget, GdkEventFocus *event, gpointer data);

unsigned int weatherScales;
char locationKey[41] = "2643743";
char thermoServer[41] = "tinyfour";
int thermoPort = 30302;
char powerServer[41] = "littleone";
int powerPort = 30303;

/**********************************************************************************************************************
 *                                                                                                                    *
 *  H O W  T O                                                                                                        *
 *  ==========                                                                                                        *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Tell them how to run the program.
 *  \param outFile Handle to output to.
 *  \param format Format string.
 *  \param ... Extra parameters.
 *  \result None.
 */
void
howTo (FILE * outFile, char *format, ...)
{
	int i;
	va_list ap;

	fprintf (outFile, "------------------------------------------------------------\n");
	fprintf (outFile, _("The Gauge %s\n"), VERSION);
	fprintf (outFile, "------------------------------------------------------------\n");
	fprintf (outFile, _("How to use: Gauge [options...]\n\n"));
	fprintf (outFile, _("   -a              :  Toggle always on top\n"));
	fprintf (outFile, _("   -cnnn#RRGGBB    :  Change one of the gauge colours\n"));
	fprintf (outFile, _("   -C<file>        :  Specify the configuration file to use\n"));
	fprintf (outFile, _("   -f<face>        :  Select the face, for setting next option\n"));
	fprintf (outFile, _("   -F<font>        :  Select the font to use on the gauge face\n"));
	fprintf (outFile, _("   -H<name>:s:l:t  :  Set the hands style, length and tail size\n"));
	fprintf (outFile, _("   -g<gradient>    :  Select the amount of gradient on the face\n"));
	fprintf (outFile, _("   -l              :  Toggle locking the screen position\n"));
	fprintf (outFile, _("   -m<type><step>  :  Set the marker type and step, default -m13\n"));
	fprintf (outFile, _("                   :  0=none, 1=Triangle, 2=Circle, 3=Latin, 4=Roman\n"));
	fprintf (outFile, _("   -n<c|r><num>    :  Set the number of columns and rows\n"));
	fprintf (outFile, _("                   :  Max clocks %d, no more than 10 in a line\n"), MAX_FACES);
	fprintf (outFile, _("   -O<opacity>     :  Change the opacity, 0 clear to 100 solid\n"));	
	fprintf (outFile, _("   -s<size>        :  Set the size of each clock\n"));
	fprintf (outFile, _("   -w              :  Toggle showing on all the desktops\n"));
	fprintf (outFile, _("   -x<posn>        :  Set the X screen position\n"));
	fprintf (outFile, _("   -y<posn>        :  Set the Y screen position\n"));
	fprintf (outFile, _("                   :  Both X and Y must be set\n"));
	fprintf (outFile, _("   -?              :  This how to information\n\n"));
	fprintf (outFile, _("Options marked with '*' only effect the current face. Use\n"));
	fprintf (outFile, _("the -f<num> option to select the current face.\n"));
	fprintf (outFile, "------------------------------------------------------------\n");
	fprintf (outFile, _("Colour codes: -cnnn#RRGGBB  (nnn Colour name)\n\n"));

	for (i = 2; i < MAX__COLOURS; i++)
		fprintf (outFile, "   %s : %s\n", colourNames[i].shortName, colourNames[i].longName);
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
 *  Q U I T  C A L L B A C K                                                                                          *
 *  ========================                                                                                          *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Called on selecting quit on the menu.
 *  \param data Data from the memnu.
 *  \result None.
 */
void
quitCallback (guint data)
{
	gtk_main_quit ();
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  O N  T O P  C A L L B A C K                                                                                       *
 *  ===========================                                                                                       *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Called to change the always on top flag.
 *  \param data Data from the memnu.
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
	gtk_window_set_keep_above (GTK_WINDOW (dialConfig.mainWindow), alwaysOnTop != 0);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S T I C K  C A L L B A C K                                                                                        *
 *  ==========================                                                                                        *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Called to put the app on all the work spaces.
 *  \param data Data from the menu.
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
		gtk_window_stick (GTK_WINDOW (dialConfig.mainWindow));
	else
		gtk_window_unstick (GTK_WINDOW (dialConfig.mainWindow));
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  L O C K  C A L L B A C K                                                                                          *
 *  ========================                                                                                          *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Called to lock the current position.
 *  \param data Data from the menu.
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
 *  A B O U T  C A L L B A C K                                                                                        *
 *  ==========================                                                                                        *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Called to display the about box.
 *  \param data Data from the menu.
 *  \result None.
 */
void
aboutCallback (guint data)
{
	char verString[81];

	sprintf (verString, _("Version: %s"), VERSION);
	/*------------------------------------------------------------------------------------------------*
	 * Nice dialog that can be used with newer versions of the GTK API.                               *
	 *------------------------------------------------------------------------------------------------*/
	gtk_show_about_dialog (dialConfig.mainWindow,
			"title", _("About Gauge"),
#if GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION > 11)
			"program-name", _("Gauge"),
#endif
			"artists", artists,
			"authors", authors,
			"comments", _("Gauge is a highly configurable analogue gauge,\n"
						  "capable of monitoring many different system\n"
						  "and environmental values."),
			"copyright", "Copyright © 2005 - 2018 Chris Knight <chris@theknight.co.uk>",
			"logo", defaultIcon,
			"version", verString,
			"website", "https://www.TheKnight.co.uk/",
			NULL);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  W R A P  T E X T                                                                                                  *
 *  ================                                                                                                  *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Wrap a line of text.
 *  \param inText Line to wrap.
 *  \param top Is this the top or bottom of the gauge.
 *  \result Pointer to static wrapped line.
 */
char *wrapText (char *inText, char top)
{
	char space = 0;
	int i = 0, j = 0, o = 0, points[10];
	static char outText[121];
	
	outText[i] = 0;
	while (inText[i] && i < 120 && j < 10)
	{
		if (inText[i] == ' ')
		{
			if (o)
				space = 1;
		}
		else
		{
			if (space)
			{
				points[j++] = o;
				outText[o++] = ' ';
				space = 0;
			}
			outText[o++] = inText[i]; 
		}
		++i;
	}
	outText[o] = 0; 
	if (j)
	{
		int diff = strlen (inText);
		int use = 0, half = diff >> 1;
		
		for (i = 0; i < j; ++i)
		{
			int point = points[i], nDiff;
			
			if (top) ++point;
			nDiff = (half > point ? half - point : point - half);
			if (diff > nDiff)
			{
				use = i;
				diff = nDiff;
			}
		}
		outText[points[use]] = '\n'; 
	}
	return outText;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  P R E P A R E  F O R  P O P U P                                                                                   *
 *  ===============================                                                                                   *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief enable and disable some options.
 *  \result None.
 */
void prepareForPopup (void)
{
	int i;

	viewMenuDesc[0].disable = (dialConfig.dialWidth >= 10 || (dialConfig.dialWidth + 1) * dialConfig.dialHeight > MAX_FACES ? 1 : 0);
	viewMenuDesc[1].disable = (dialConfig.dialWidth < 2 ? 1 : 0);
	viewMenuDesc[2].disable = (dialConfig.dialHeight >= 10 || (dialConfig.dialHeight + 1) * dialConfig.dialWidth > MAX_FACES ? 1 : 0);
	viewMenuDesc[3].disable = (dialConfig.dialHeight < 2 ? 1 : 0);
	
	for (i = MENU_MARK_STRT; i <= MENU_MARK_STOP; ++i)
		markerMenuDesc[i].checked = (markerMenuDesc[i].param == dialConfig.markerType ? 1 : 0);
	for (i = MENU_STEP_STRT; i <= MENU_STEP_STOP; ++i)
		markerMenuDesc[i].checked = (markerMenuDesc[i].param == dialConfig.markerStep ? 1 : 0);

	prefMenuDesc[MENU_PREF_ONTOP].checked = alwaysOnTop;
	prefMenuDesc[MENU_PREF_STUCK].checked = stuckOnAll;
	prefMenuDesc[MENU_PREF_LOCK].checked = lockMove;
	
	prefMenuDesc[MENU_PREF_SVG].disable = !allowSaveDisp;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  W I N D O W  C L I C K  C A L L B A C K                                                                           *
 *  =======================================                                                                           *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Someone clicked on the window.
 *  \param widget What was clicked on.
 *  \param event Type of click.
 *  \result None.
 */
gboolean
windowClickCallback (GtkWidget * widget, GdkEventButton * event)
{
	GtkWidget *popupMenu;

	if (event->type == GDK_BUTTON_PRESS)
	{
		currentFace = ((int)event -> x / dialConfig.dialSize) + (((int)event -> y / dialConfig.dialSize) * dialConfig.dialWidth);
		lastTime = -1;

		switch (event->button)
		{
#ifndef GAUGE_IS_DECORATED
#ifdef GDK_BUTTON_PRIMARY
		case GDK_BUTTON_PRIMARY:	/* left button */
#else
		case 1:
#endif
			if (!lockMove)
			{
				gtk_window_begin_move_drag (GTK_WINDOW (dialConfig.mainWindow), event->button, event->x_root,
						event->y_root, event->time);
			}
			return TRUE;
#endif
			
#ifdef GDK_BUTTON_SECONDARY
		case GDK_BUTTON_SECONDARY:	/* right button */
#else
		case 3:
#endif
			prepareForPopup ();
			popupMenu = createMenu (mainMenuDesc, accelGroup, FALSE);
#if GTK_MAJOR_VERSION == 3 && GTK_MINOR_VERSION >= 22
			gtk_menu_popup_at_pointer (GTK_MENU(popupMenu), NULL);
#else
			gtk_menu_popup (GTK_MENU (popupMenu), NULL,	/* parent_menu_shell */
					NULL,								/* parent_menu_item */
					NULL,								/* func */
					NULL,								/* data */
					event->button, event->time);
#endif
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
 *  \brief Someone pressed a key.
 *  \param widget What called.
 *  \param event What key was pressed.
 *  \result None.
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
#if GTK_MAJOR_VERSION == 3 && GTK_MINOR_VERSION >= 22
				gtk_menu_popup_at_pointer (GTK_MENU(popupMenu), NULL);
#else
				gtk_menu_popup (GTK_MENU (popupMenu), NULL,	/* parent_menu_shell */
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
		gtk_window_activate_key (GTK_WINDOW (dialConfig.mainWindow), event);
	}
	/*------------------------------------------------------------------------------------------------*
	 * Process key release events                                                                     *
	 *------------------------------------------------------------------------------------------------*/
	else if (event->type == GDK_KEY_RELEASE)
	{
		if (event->keyval == 0xFFE9 && keyPressFaceNum != -1)  /* Alt key released */
		{
			keyPressFaceNum --;
			if (keyPressFaceNum >= 0 && keyPressFaceNum < (dialConfig.dialWidth * dialConfig.dialHeight))
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
 *  S L I D E  V A L U E S                                                                                            *
 *  ======================                                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Slowly move from one value to another.
 *  \param oldValue Starting here.
 *  \param newValue Move to here.
 *  \result New value.
 */
int slideValues (int oldValue, int newValue)
{
	int step = newValue - oldValue, move;

	if (step && newValue != DONT_SHOW)
	{
		move = step / 2;
		move += (move > 0 ? 1 : -1);

		if (move > 12 || move < -12)
			step = move;

		oldValue += step;
	}
	return oldValue;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  C A L C  S H O W  V A L U E S                                                                                     *
 *  =============================                                                                                     *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Calculate the values to show on the face.
 *  \param faceSetting Which face are we working on.
 *  \result Update if an update is needed.
 */
int calcShowValues (FACE_SETTINGS *faceSetting)
{
	int update = 0, firstValue = DONT_SHOW, secondValue = DONT_SHOW, scale;
	
	scale = SCALE_3 / (faceSetting -> faceScaleMax - faceSetting -> faceScaleMin);

	if (faceSetting -> firstValue != DONT_SHOW)
	{
		firstValue = (faceSetting -> firstValue - faceSetting -> faceScaleMin) * scale;
		if (faceSetting -> shownFirstValue == DONT_SHOW)
			faceSetting -> shownFirstValue = firstValue;
	}
	if (faceSetting -> shownFirstValue != firstValue)
	{
		faceSetting -> shownFirstValue = slideValues (faceSetting -> shownFirstValue, firstValue);
		++update;
	}
	if (faceSetting -> secondValue != DONT_SHOW)
	{
		secondValue = (faceSetting -> secondValue - faceSetting -> faceScaleMin) * scale;
		if (faceSetting -> shownSecondValue == DONT_SHOW)
			faceSetting -> shownSecondValue = secondValue;
	}
	if (faceSetting -> shownSecondValue != secondValue)
	{
		faceSetting -> shownSecondValue = slideValues (faceSetting -> shownSecondValue, secondValue);
		++update;
	}
	if (faceSetting -> faceFlags & FACE_REDRAW)
	{
		faceSetting -> faceFlags &= ~FACE_REDRAW;
		++update;
	}
	if (updateMaxMinValues (faceSetting, firstValue))
		++update;
		
	return update;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  C L O C K  T I C K  C A L L B A C K                                                                               *
 *  ===================================                                                                               *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief The timer when off.
 *  \param data Not used.
 *  \result None.
 */
gboolean
clockTickCallback (gpointer data)
{
	int update = 0, i, j, face = 0;

	for (j = 0; j < dialConfig.dialHeight; j++)
	{
		for (i = 0; i < dialConfig.dialWidth; i++)
		{
			if (!faceSettings[face])
			{
				printf ("No configuration for face %d (%d, %d)\n", face, j, i);
				continue;
			}
			if (faceSettings[face] -> showFaceType >= 0 && faceSettings[face] -> showFaceType < FACE_TYPE_MAX)
			{
				if (!gaugeEnabled[faceSettings[face] -> showFaceType].enabled)
				{
					faceSettings[face] -> showFaceType = FACE_TYPE_MAX;
				}
			}
			switch (faceSettings[face] -> showFaceType)
			{
			case FACE_TYPE_CPU_LOAD:
				readCPUValues (face);
				break;
			case FACE_TYPE_MEMORY:
				readMemoryValues (face);
				break;
			case FACE_TYPE_BATTERY:
				readBatteryValues (face);
				break;
			case FACE_TYPE_ENTROPY:
				readEntropyValues (face);
				break;
			case FACE_TYPE_NETWORK:
				readNetworkValues (face);
				break;
			case FACE_TYPE_HARDDISK:
				readHarddiskValues (face);
				break;
			case FACE_TYPE_SENSOR_TEMP:
			case FACE_TYPE_SENSOR_FAN:
				readSensorValues (face);
				break;
			case FACE_TYPE_TIDE:
				readTideValues (face);
				break;
			case FACE_TYPE_WEATHER:
				readWeatherValues (face);
				break;
			case FACE_TYPE_THERMO:
				readThermometerValues (face);
				break;
			case FACE_TYPE_POWER:
				readPowerMeterValues (face);
				break;
			case FACE_TYPE_MOONPHASE:
				readMoonPhaseValues (face);
				break;

			case FACE_TYPE_WIFI:
				readWifiValues (face);
				break;

			case FACE_TYPE_MAX:
			default:
				/*------------------------------------------------------------------------------------*
				 * Used for drawing the icon on the about box.                                        *
				 *------------------------------------------------------------------------------------*/
				gaugeReset (face, faceSettings[face] -> showFaceType, 0);
				faceSettings[face] -> faceFlags |= FACE_SHOWHOT;
				faceSettings[face] -> faceFlags |= FACE_SHOWCOLD;
				setFaceString (faceSettings[face], FACESTR_TOP, 0, "Gauge");
				setFaceString (faceSettings[face], FACESTR_BOT, 0, VERSION);
				faceSettings[face] -> showFaceType = FACE_TYPE_MAX;
				faceSettings[face] -> firstValue = 0;
				break;
			}
			update += calcShowValues (faceSettings[face]);
			++face;
		}
	}
	if (lastTime == -1)
	{
		++update;
	}
	if (update)
	{
		if (dialConfig.drawingArea)
		{
			gtk_widget_queue_draw (dialConfig.drawingArea);
		}
		lastTime = time (NULL);
	}
	++sysUpdateID;
	return TRUE;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  F O C U S  I N  E V E N T                                                                                         *
 *  =========================                                                                                         *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief The window came in to focus.
 *  \param widget The window.
 *  \param event The event that brought the window into focus.
 *  \param data Not used.
 *  \result None.
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
 *  \brief The window when out of focus.
 *  \param widget The window.
 *  \param event The event that took the window out of focus.
 *  \param data Not used.
 *  \result None.
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
 *  M A X  M I N  R E S E T                                                                                           *
 *  =======================                                                                                           *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Reset the max and Min values.
 *  \param savedMaxMin Pointer to the max and min structure to reset.
 *  \param count Number of values to save.
 *  \param interval Interval between updates.
 *  \result None.
 */
void maxMinReset (SAVED_MAX_MIN *savedMaxMin, int count, int interval)
{
	int i;
	savedMaxMin -> maxMinCount = count > MAX_MIN_COUNT ? MAX_MIN_COUNT : count;
	savedMaxMin -> updateInterval = interval;
	savedMaxMin -> nextUpdateTime = time (NULL);
	savedMaxMin -> maxMinPosn = 0;
	savedMaxMin -> shownMaxValue = -1;
	savedMaxMin -> shownMinValue = -1;
	for (i = 0; i < MAX_MIN_COUNT; ++i)
	{
		savedMaxMin -> maxBuffer[i] = savedMaxMin -> minBuffer[i] = -1;
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  G A U G E  R E S E T                                                                                              *
 *  ====================                                                                                              *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Reset the values on the Gauge.
 *  \param face Which Gauge to reset.
 *  \param type New gauge type.
 *  \param subType New gauge sub-type.
 *  \result None.
 */
void gaugeReset (int face, int type, int subType)
{
	char value[81];

	faceSettings[face] -> showFaceType = type;
	faceSettings[face] -> faceSubType = subType;
	faceSettings[face] -> nextUpdate = 0;
	faceSettings[face] -> updateNum = -1;
	
	setFaceString (faceSettings[face], FACESTR_TIP, 0, "");
	setFaceString (faceSettings[face], FACESTR_TOP, 0, "");
	setFaceString (faceSettings[face], FACESTR_BOT, 0, "");
	setFaceString (faceSettings[face], FACESTR_WIN, 0, "");

	faceSettings[face] -> firstValue = DONT_SHOW;
	faceSettings[face] -> secondValue = DONT_SHOW;
	faceSettings[face] -> shownFirstValue = DONT_SHOW;
	faceSettings[face] -> shownSecondValue = DONT_SHOW;
	maxMinReset (&faceSettings[face] -> savedMaxMin, MAX_MIN_COUNT, 1);
	faceSettings[face] -> faceFlags = FACE_REDRAW;
	faceSettings[face] -> faceScaleMin = 0;
	faceSettings[face] -> faceScaleMax = 100;

	sprintf (value, "show_face_type_%d", face + 1);
	configSetIntValue (value, type);
	sprintf (value, "face_sub_type_%d", face + 1);
	configSetIntValue (value, subType);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  L O A D  C A L L B A C K                                                                                          *
 *  ========================                                                                                          *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Display The CPU load.
 *  \param data What to show.
 *  \result None.
 */
void
loadCallback (guint data)
{
	int faceSubType = 0;

	if (data & 0x2000)
	{
		faceSubType = 0x0F00;
	}
	else if (faceSettings[currentFace] -> showFaceType == FACE_TYPE_CPU_LOAD)
	{
		if (data & 0x1000)
		{
			faceSubType = faceSettings[currentFace] -> faceSubType & 0x0F00;
			faceSubType |= (data & 0x00FF);
		}
		else
		{
			faceSubType = faceSettings[currentFace] -> faceSubType & 0x00FF;
			faceSubType |= (data & 0x0F00);
		}
	}
	faceSubType &= 0x0FFF;

	gaugeReset (currentFace, FACE_TYPE_CPU_LOAD, faceSubType);
	faceSettings[currentFace] -> faceFlags |= FACE_HOT_COLD;
	faceSettings[currentFace] -> savedMaxMin.maxMinCount = 5;

	if (faceSettings[currentFace] -> faceSubType == 0x0F00)
	{
		faceSettings[currentFace] -> faceScaleMin = 0;
		faceSettings[currentFace] -> faceScaleMax = 2.5;
		faceSettings[currentFace] -> faceFlags |= (FACE_SHOW_POINT | FACE_SHOW_MAX);
		faceSettings[currentFace] -> savedMaxMin.maxMinCount = 10;
	}
	else if ((faceSettings[currentFace] -> faceSubType & 0x0F00) == 0x0400)
		faceSettings[currentFace] -> faceFlags |= (FACE_HC_REVS | FACE_SHOW_MIN);
	else
		faceSettings[currentFace] -> faceFlags |= FACE_SHOW_MAX;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  M E M O R Y  C A L L B A C K                                                                                      *
 *  ============================                                                                                      *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Called from the menu to select memory gauge.
 *  \param data Which memory gauge.
 *  \result None.
 */
void
memoryCallback (guint data)
{
	if (data > 4) return;
	
	gaugeReset (currentFace, FACE_TYPE_MEMORY, data);
	faceSettings[currentFace] -> faceFlags |= FACE_HOT_COLD;
	faceSettings[currentFace] -> savedMaxMin.maxMinCount = 10;
	faceSettings[currentFace] -> savedMaxMin.updateInterval = 3;
	if (data == 1) 
		faceSettings[currentFace] -> faceFlags |= (FACE_HC_REVS | FACE_SHOW_MIN);
	else
		faceSettings[currentFace] -> faceFlags |= FACE_SHOW_MAX;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  B A T T E R Y  C A L L B A C K                                                                                    *
 *  ==============================                                                                                    *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Called from the menu to select battery gauge.
 *  \param data Not used.
 *  \result None.
 */
void
batteryCallback (guint data)
{
	gaugeReset (currentFace, FACE_TYPE_BATTERY, data);
	faceSettings[currentFace] -> faceFlags |= (FACE_HOT_COLD | FACE_HC_REVS);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  M O O N  P H A S E  C A L L B A C K                                                                               *
 *  ===================================                                                                               *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Called to setup moon phase dial.
 *  \param data Not used.
 *  \result None.
 */
void
moonPhaseCallback (guint data)
{
	gaugeReset (currentFace, FACE_TYPE_MOONPHASE, data);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  W I F I  C A L L B A C K                                                                                          *
 *  ========================                                                                                          *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Called to setup wifi dial.
 *  \param data Sub-type if needed.
 *  \result None.
 */
void
wifiCallback (guint data)
{
	gaugeReset (currentFace, FACE_TYPE_WIFI, data);
	faceSettings[currentFace] -> faceFlags |= (FACE_MAX_MIN | FACE_SHOWCOLD | FACE_HC_REVS);
	faceSettings[currentFace] -> savedMaxMin.maxMinCount = 10;
	faceSettings[currentFace] -> savedMaxMin.updateInterval = 2;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  E N T R O P Y  C A L L B A C K                                                                                    *
 *  ==============================                                                                                    *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Call to set the gauge to entropy mode.
 *  \param data Not used.
 *  \result None.
 */
void 
entropyCallback (guint data)
{
	gaugeReset (currentFace, FACE_TYPE_ENTROPY, data);
	faceSettings[currentFace] -> faceFlags |= (FACE_MAX_MIN | FACE_HOT_COLD | FACE_HC_REVS);
	faceSettings[currentFace] -> savedMaxMin.maxMinCount = 10;
	faceSettings[currentFace] -> savedMaxMin.updateInterval = 2;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  T I D E  C A L L B A C K                                                                                          *
 *  ========================                                                                                          *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Call to set the gauge to tide clock mode.
 *  \param data Not used.
 *  \result None.
 */
void 
tideCallback (guint data)
{
	gaugeReset (currentFace, FACE_TYPE_TIDE, data);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  N E T W O R K  C A L L B A C K                                                                                    *
 *  ==============================                                                                                    *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Called when network is selected on the menu.
 *  \param data What to monitor.
 *  \result None.
 */
void
networkCallback (guint data)
{
	int faceSubType = data;
	
	if (faceSettings[currentFace] -> showFaceType == FACE_TYPE_NETWORK)
	{
		if (data & 0x1000)
		{
			faceSubType = faceSettings[currentFace] -> faceSubType & 0x0FF0;
			faceSubType |= (data & 0x000F);
		}
		else
		{
			faceSubType = faceSettings[currentFace] -> faceSubType & 0x000F;
			faceSubType |= (data & 0x0FF0);
		}
	}
	faceSubType &= 0x0FFF;

	gaugeReset (currentFace, FACE_TYPE_NETWORK, faceSubType);
	faceSettings[currentFace] -> faceFlags |= (FACE_MAX_MIN | FACE_HOT_COLD | FACE_HC_REVS);
	faceSettings[currentFace] -> savedMaxMin.maxMinCount = 10;
	faceSettings[currentFace] -> savedMaxMin.updateInterval = 2;
	setFaceString (faceSettings[currentFace], FACESTR_WIN, 0, "Network Usage - Gauge");
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  H A R D D I S K  C A L L B A C K                                                                                  *
 *  ================================                                                                                  *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Called to setup the hard drive gauge.
 *  \param data Which gauge to show.
 *  \result None.
 */
void harddiskCallback (guint data)
{
	int faceSubType = data;
	
	if (faceSettings[currentFace] -> showFaceType == FACE_TYPE_HARDDISK)
	{
		if (data & 0x1000)
		{
			if (faceSettings[currentFace] -> faceSubType & 0x0F00)	
			{
				faceSubType = faceSettings[currentFace] -> faceSubType & 0x0F00;
				faceSubType |= (data & 0x00FF);
			}
			else
			{
				faceSubType &= 0x00FF;
				faceSubType |= 0x0100;
			}
		}
		else if (data & 0x0F00)
		{
			if (faceSettings[currentFace] -> faceSubType & 0x0F00)	
			{
				faceSubType = faceSettings[currentFace] -> faceSubType & 0x00FF;
				faceSubType |= (data & 0x0F00);
			}
		}
	}
	else if (data & 0x1000)
	{
		faceSubType |= 0x100;
	}

	faceSubType &= 0x0FFF;
	gaugeReset (currentFace, FACE_TYPE_HARDDISK, faceSubType);
	faceSettings[currentFace] -> faceFlags |= (FACE_MAX_MIN | FACE_SHOWHOT);
	faceSettings[currentFace] -> savedMaxMin.maxMinCount = 10;
	faceSettings[currentFace] -> savedMaxMin.updateInterval = 2;
	setFaceString (faceSettings[currentFace], FACESTR_WIN, 0, "Hard Disk - Gauge");
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S E N S O R  T E M P  C A L L B A C K                                                                             *
 *  =====================================                                                                             *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Tempature sensor option selected from the menu.
 *  \param data Which option was selected.
 *  \result None.
 */
void
sensorTempCallback (guint data)
{
	gaugeReset (currentFace, FACE_TYPE_SENSOR_TEMP, data);
	faceSettings[currentFace] -> faceFlags |= (FACE_MAX_MIN | FACE_HOT_COLD);
	faceSettings[currentFace] -> savedMaxMin.maxMinCount = 10;
	faceSettings[currentFace] -> savedMaxMin.updateInterval = 2;
	faceSettings[currentFace] -> faceScaleMin = 10;
	faceSettings[currentFace] -> faceScaleMax = 35;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S E N S O R  F A N  C A L L B A C K                                                                               *
 *  ===================================                                                                               *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Fan sensor option selected from the menu.
 *  \param data Which option was selected.
 *  \result None.
 */
void
sensorFanCallback (guint data)
{
	gaugeReset (currentFace, FACE_TYPE_SENSOR_FAN, data);
	faceSettings[currentFace] -> faceFlags |= (FACE_MAX_MIN | FACE_SHOWHOT);
	faceSettings[currentFace] -> savedMaxMin.maxMinCount = 10;
	faceSettings[currentFace] -> savedMaxMin.updateInterval = 2;
	faceSettings[currentFace] -> faceScaleMin = 0;
	faceSettings[currentFace] -> faceScaleMax = 25;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  W E A T H E R  C A L L B A C K                                                                                    *
 *  ==============================                                                                                    *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Weather option selected from the menu.
 *  \param data Which option was selected.
 *  \result None.
 */
void
weatherCallback (guint data)
{
	gaugeReset (currentFace, FACE_TYPE_WEATHER, data);
	faceSettings[currentFace] -> savedMaxMin.updateInterval = 3600;
	weatherGetMaxMin (faceSettings[currentFace]);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  T H E R M O M E T E R  C A L L B A C K                                                                            *
 *  ======================================                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Function to turn on the thermometer gauge.
 *  \param data Which gauge to display.
 *  \result None.
 */
void
thermometerCallback (guint data)
{
	gaugeReset (currentFace, FACE_TYPE_THERMO, data);
	faceSettings[currentFace] -> faceFlags |= (FACE_MAX_MIN | FACE_HOT_COLD);
	faceSettings[currentFace] -> savedMaxMin.maxMinCount = 10;
	faceSettings[currentFace] -> savedMaxMin.updateInterval = 2;
	faceSettings[currentFace] -> faceScaleMin = -10;
	faceSettings[currentFace] -> faceScaleMax = 40;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  P O W E R  M E T E R  C A L L B A C K                                                                             *
 *  =====================================                                                                             *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Function to turn on the power meter gauge.
 *  \param data Which gauge to display.
 *  \result None.
 */
void
powerMeterCallback (guint data)
{
	gaugeReset (currentFace, FACE_TYPE_POWER, data);
	faceSettings[currentFace] -> faceFlags |= (FACE_MAX_MIN | FACE_HOT_COLD);
	faceSettings[currentFace] -> savedMaxMin.maxMinCount = 10;
	faceSettings[currentFace] -> savedMaxMin.updateInterval = 2;
	faceSettings[currentFace] -> faceScaleMin = 0;
	faceSettings[currentFace] -> faceScaleMax = 2.5;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  C O N F I G  S A V E  C A L L B A C K                                                                             *
 *  =====================================                                                                             *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Save the configuration.
 *  \param data Not used.
 *  \result None.
 */
void configSaveCallback (guint data)
{
	int posX, posY;
	char *home = getenv ("HOME");
	char configPath[1024];

	gtk_window_get_position (GTK_WINDOW (dialConfig.mainWindow), &posX, &posY);
	configSetIntValue ("gauge_x_pos", posX);
	configSetIntValue ("gauge_y_pos", posY);
	configSetIntValue ("gauge_current", currentFace);
	
	if (home)
	{
		strcpy (configPath, home);
		strcat (configPath, "/");
		strcat (configPath, configFile);
		configSave (configPath);
	}
}

#if GTK_MAJOR_VERSION == 2
/**********************************************************************************************************************
 *                                                                                                                    *
 *  E X P O S E  C A L L B A C K                                                                                      *
 *  ============================                                                                                      *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief The window is exposed.
 *  \param widget Not used.
 *  \param event Not used.
 *  \param data Not used.
 *  \result None.
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
#endif

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
	newFace = ((int)dx / dialConfig.dialSize) + (((int)dy / dialConfig.dialSize) * dialConfig.dialWidth);
	if (newFace != toolTipFace)
	{
		toolTipFace = newFace;
		lastTime = -1;
	}
	return TRUE;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  U P D A T E  M A X  M I N  V A L U E S                                                                            *
 *  ======================================                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Save a new value in the max min buffer.
 *  \param faceSetting Which face is this for.
 *  \param firstValue Get the max and min from weather units settings.
 *  \result 1 if a paint is needed bacuase value changed.
 */
static int updateMaxMinValues (FACE_SETTINGS *faceSetting, int firstValue)
{
	int i, posn, maxVal = -1, minVal = -1;
	SAVED_MAX_MIN *saved = &faceSetting -> savedMaxMin;
	int saveMax = saved -> shownMaxValue, saveMin = saved -> shownMinValue;
	
	if (time (NULL) > saved -> nextUpdateTime)
	{
		++saved -> maxMinPosn;
		if (saved -> maxMinPosn >= MAX_MIN_COUNT || saved -> maxMinPosn >= saved -> maxMinCount)
			saved -> maxMinPosn = 0;
			
		saved -> maxBuffer[saved -> maxMinPosn] = -1;
		saved -> minBuffer[saved -> maxMinPosn] = -1;
		saved -> nextUpdateTime = time (NULL) + saved -> updateInterval;
	}
	posn = saved -> maxMinPosn;
	
	if (faceSetting -> shownFirstValue <= firstValue && faceSetting -> faceFlags & FACE_SHOW_MAX)
	{
		if (faceSetting -> shownFirstValue > saved -> maxBuffer[posn] || saved -> maxBuffer[posn] == -1)
			saved -> maxBuffer[posn] = faceSetting -> shownFirstValue;
	}
	if (faceSetting -> shownFirstValue >= firstValue && faceSetting -> faceFlags & FACE_SHOW_MIN)
	{
		if (faceSetting -> shownFirstValue < saved -> minBuffer[saved -> maxMinPosn] || saved -> minBuffer[saved -> maxMinPosn] == -1)
			saved -> minBuffer[posn] = faceSetting -> shownFirstValue;
	}
	for (i = 0; i < MAX_MIN_COUNT && i < saved -> maxMinCount; ++i)
	{
		if ((saved -> maxBuffer[i] != -1 && saved -> maxBuffer[i] > maxVal) || maxVal == -1)
			maxVal = saved -> maxBuffer[i];
			
		if ((saved -> minBuffer[i] != -1 && saved -> minBuffer[i] < minVal) || minVal == -1)
			minVal = saved -> minBuffer[i];
	}
	if (faceSetting -> faceFlags & FACE_SHOW_MAX && maxVal != -1)
	{
		if (saveMax != -1)
		{
			saved -> shownMaxValue = slideValues (saved -> shownMaxValue, maxVal);
			if (saved -> shownMaxValue < faceSetting -> shownFirstValue)
				saved -> shownMaxValue = faceSetting -> shownFirstValue;
		}
		else
			saved -> shownMaxValue = faceSetting -> shownFirstValue;
	}
	if (faceSetting -> faceFlags & FACE_SHOW_MIN && minVal != -1)
	{
		if (saveMin != -1)
		{
			saved -> shownMinValue = slideValues (saved -> shownMinValue, minVal);
			if (saved -> shownMinValue > faceSetting -> shownFirstValue)
				saved -> shownMinValue = faceSetting -> shownFirstValue;
		}
		else
			saved -> shownMinValue = faceSetting -> shownFirstValue;
	}		
	return (saveMax == saved -> shownMaxValue && saveMin == saved -> shownMinValue) ? 0 : 1;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  L O A D  C O L O U R                                                                                              *
 *  ====================                                                                                              *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Read a clolour from the command line.
 *  \param fromColour Command line.
 *  \result True colour.
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
 *  \brief Process parameters on the command line.
 *  \param argc Number of args.
 *  \param argv The Args themselves.
 *  \param posX Screen position X.
 *  \param posY Screen position Y.
 *  \result None.
 */
void processCommandLine (int argc, char *argv[], int *posX, int *posY)
{
	int i, j, face = 0, invalidOption = 0;

	for (i = 1; i < argc; i++)
	{
		if (argv[i][0] == '-')
		{
			switch (argv[i][1])
			{
			case 'a':							/* Set the clock always on top */
				alwaysOnTop = !alwaysOnTop;
				configSetBoolValue ("always_on_top", alwaysOnTop);
				break;
			case 'C':							/* Specify config file, done in main */
				break;
			case 'c':							/* Select the colours */
				if (!loadColour (&argv[i][2]))
					invalidOption = 1;
				break;
			case 'f':							/* Select the face for zone changes */
				{
					int f = atoi (&argv[i][2]);
					if (f > 0 && f <= (dialConfig.dialWidth * dialConfig.dialHeight))
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
				configSetValue ("font_name", fontName);
				break;
			case 'g':
				dialConfig.dialGradient = atoi (&argv[i][2]);
				if (dialConfig.dialGradient < 0) dialConfig.dialGradient = 0;
				if (dialConfig.dialGradient > 100) dialConfig.dialGradient = 100;
				configSetIntValue ("gradient", dialConfig.dialGradient);
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
					dialConfig.markerType = (argv[i][2] - '0');
					if (argv[i][3] >= '1' && argv[i][3] <= '9')
						dialConfig.markerStep = (argv[i][3] - '0');
				}
				configSetIntValue ("gauge_mark_type", dialConfig.markerType);
				configSetIntValue ("gauge_mark_step", dialConfig.markerStep);
				break;
			case 'n':							/* Set the number of ... */
				switch (argv[i][2])
				{
				case 'c':						/* ... columns */
					{
						int c = atoi (&argv[i][3]);
						if (c > 0 && c <= 10 && c * dialConfig.dialHeight <= MAX_FACES)
							configSetIntValue ("gauge_num_col", dialConfig.dialWidth = c);
						else
							invalidOption = 1;
					}
					break;
				case 'r':						/* ... rows */
					{
						int r = atoi (&argv[i][3]);
						if (r > 0 && r <= 10 && r * dialConfig.dialWidth <= MAX_FACES)
							configSetIntValue ("gauge_num_row", dialConfig.dialHeight = r);
						else
							invalidOption = 1;
					}
					break;
				default:
					invalidOption = 1;
					break;
				}
				for (j = 0; j < (dialConfig.dialWidth * dialConfig.dialHeight); ++j)
				{
					if (faceSettings[j] == NULL)
					{
						faceSettings[j] = malloc (sizeof (FACE_SETTINGS));
						memset (faceSettings[j], 0, sizeof (FACE_SETTINGS));
					}			
				}
				break;
			case 'O':
				dialConfig.dialOpacity = atoi (&argv[i][2]);
				if (dialConfig.dialOpacity < 0) dialConfig.dialOpacity = 0;
				if (dialConfig.dialOpacity > 99) dialConfig.dialOpacity = 99;
				configSetIntValue ("opacity", dialConfig.dialOpacity);
				break;
			case 's':							/* Select the dialConfig.dialSize of the clock */
				dialConfig.dialSize = atoi (&argv[i][2]);
				configSetIntValue ("face_size", dialConfig.dialSize);
				break;
			case 'T':
				{
					int type = atoi (&argv[i][2]);
					if (type >= 0 && type <= FACE_TYPE_MAX)
					{
						gaugeReset (currentFace, type, 0);
					}
				}
				break;
			case 'V':
				allowSaveDisp = !allowSaveDisp;
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
			case '?':							/* Display the help information */
				howTo (stderr, NULL);
				exit (0);
			default:
				howTo (stderr, "ERROR: Unrecognised option '%s'\n", argv[i]);
				exit (0);
			}
		}
		else
			invalidOption = 1;
			
		if (invalidOption)
		{
			howTo (stderr, "ERROR: Please check option '%s'\n", argv[i]);
			exit (0);
		}
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  U P D A T E  G A U G E                                                                                            *
 *  ======================                                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Call back called when a gauge setting is changed by the dial system.
 *  \result None.
 */
void updateGauge (void)
{
	int i;

	for (i = 0; i < (dialConfig.dialHeight * dialConfig.dialWidth); i++)
	{
		if (faceSettings[i] == NULL)
		{
			faceSettings[i] = malloc (sizeof (FACE_SETTINGS));
			memset (faceSettings[i], 0, sizeof (FACE_SETTINGS));
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
	configSetIntValue ("gauge_num_col", dialConfig.dialWidth);
	configSetIntValue ("gauge_num_row", dialConfig.dialHeight);
	configSetIntValue ("gauge_mark_type", dialConfig.markerType);
	configSetIntValue ("gauge_mark_step", dialConfig.markerStep);
	configSetIntValue ("face_size", dialConfig.dialSize);
	configSetIntValue ("opacity", dialConfig.dialOpacity);
	configSetIntValue ("gradient", dialConfig.dialGradient);
	configSetValue ("font_name", fontName);
	lastTime = -1;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  L O A D  C O N F I G                                                                                              *
 *  ====================                                                                                              *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Load the config from file.
 *  \param posX Screen position X.
 *  \param posY Screen position Y.
 *  \result None.
 */
void loadConfig (int *posX, int *posY)
{
	int i;
	char *home = getenv ("HOME");
	char configPath[1024], value[81], tempName[81];
	
	configLoad ("/etc/gaugerc");
	strcpy (configPath, home);
	strcat (configPath, "/");
	strcat (configPath, configFile);
	configLoad (configPath);

	configGetBoolValue ("always_on_top", &alwaysOnTop);
	configGetBoolValue ("on_all_desktops", &stuckOnAll);
	configGetBoolValue ("locked_position", &lockMove);
	configGetIntValue ("face_size", &dialConfig.dialSize);
	configGetIntValue ("gauge_num_col", &dialConfig.dialWidth);
	configGetIntValue ("gauge_num_row", &dialConfig.dialHeight);
	configGetIntValue ("gauge_current", &currentFace);
	configGetIntValue ("gauge_mark_type", &dialConfig.markerType);
	configGetIntValue ("gauge_mark_step", &dialConfig.markerStep);
	configGetIntValue ("opacity", &dialConfig.dialOpacity);
	configGetIntValue ("gradient", &dialConfig.dialGradient);	
	configGetIntValue ("gauge_x_pos", posX);
	configGetIntValue ("gauge_y_pos", posY);
	configGetValue ("font_name", fontName, 100);
	configGetValue ("tide_info_url", tideURL, 100);
	configGetIntValue ("weather_scales", (int *)&weatherScales);
	configGetValue ("location_key", locationKey, 40);
	configGetValue ("thermo_server", thermoServer, 40);
	configGetIntValue ("thermo_port", &thermoPort);
	configGetValue ("power_server", powerServer, 40);
	configGetIntValue ("power_port", &powerPort);

	for (i = 2; i < MAX__COLOURS; i++)
	{
		sprintf (value, "colour_%s", colourNames[i].shortName);
		strcpy (tempName, colourNames[i].shortName);
		configGetValue (value, &tempName[strlen (tempName)], 60);
		loadColour (tempName);
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
	for (i = 0; i < (dialConfig.dialWidth * dialConfig.dialHeight); ++i)
	{
		if (faceSettings[i] == NULL)
		{
			faceSettings[i] = malloc (sizeof (FACE_SETTINGS));
			memset (faceSettings[i], 0, sizeof (FACE_SETTINGS));
		}
		sprintf (value, "show_face_type_%d", i + 1);
		configGetIntValue (value, (int *)&faceSettings[i] -> showFaceType);
		sprintf (value, "face_sub_type_%d", i + 1);
		configGetIntValue (value, (int *)&faceSettings[i] -> faceSubType);
	}
	i = 0;
	while (gaugeEnabled[i].gaugeName != NULL)
	{
		sprintf (value, "%s_enabled", gaugeEnabled[i].gaugeName);
		configGetIntValue (value, &gaugeEnabled[i].enabled);
		++i;
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S E T U P  D I S P L A Y                                                                                          *
 *  ========================                                                                                          *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Set values in the config that will be used by the library.
 *  \result None.
 */
void setupDisplay()
{
	configSetIntValue ("number_cols", dialConfig.dialWidth);
	configSetIntValue ("number_rows", dialConfig.dialHeight);
	configSetIntValue ("marker_type", dialConfig.markerType);
	configSetIntValue ("marker_step", dialConfig.markerStep);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S H O R T E N  W O R D S                                                                                          *
 *  ========================                                                                                          *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Function to make strings fit the display.
 *  \param inString String to convert.
 *  \param outString Output the string here.
 *  \param max Display space.
 *  \result Pointer to the changed string.
 */
char *shortenWords (char *inString, char *outString, int max)
{
	int words, j, k, curWord;
	char lastChar = 0;
	
	if (max > 80) max = 80;
	for (words = 0; words < 10; ++words)
	{
		int i = j = k = curWord = 0;
		outString[0] = 0;
		while (inString[i] && k <= max + 1)
		{
			if (inString[i] <= ' ')
			{
				if (lastChar != ' ')
				{
					outString[k] = lastChar = ' ';
					outString[++k] = 0;
					j = 0;
					++curWord;
				}
			}
			else if (curWord < words)
			{
				if (j == 0)
				{
					outString[k] = lastChar = inString[i];
					outString[++k] = 0;
					++j;
				}
			}
			else
			{
				outString[k] = lastChar = inString[i];
				outString[++k] = 0;
				++j;
			}				
			++i;
		}
		if (strlen (outString) <= max)
			break;
	}
	return outString;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  V  S E T  F A C E  S T R I N G                                                                                    *
 *  ==============================                                                                                    *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Save a string in to one of the positions on the face.
 *  \param faceSetting Which face to save to.
 *  \param str Which string is being set.
 *  \param shorten Should words be made shorter to fit.
 *  \param format Format of the string.
 *  \param arg_ptr Pointer to the arguments.
 *  \result None.
 */
void vSetFaceString (FACE_SETTINGS *faceSetting, int str, int shorten, char *format, va_list arg_ptr)
{
	char buff[1025];

	if (str >= 0 && str < FACESTR_COUNT)
	{
		vsnprintf (buff, 1024, format, arg_ptr);
		if (shorten)
		{
			char buff2[1025];
			shortenWords (buff, buff2, shorten);
			strcpy (buff, wrapText (buff2, 1));
		}
		if (faceSetting -> text[str])
		{
			if (strlen (buff) >= faceSetting -> textSize[str])
			{
				faceSetting -> textSize[str] = strlen (buff) + 5;
				faceSetting -> text[str] = realloc (faceSetting -> text[str], faceSetting -> textSize[str]);
			}
		}
		else
		{
			faceSetting -> textSize[str] = strlen (buff) + 1;
			faceSetting -> text[str] = malloc (faceSetting -> textSize[str]);
		}
		if (faceSetting -> text[str])
		{
			strcpy (faceSetting -> text[str], buff);
		}
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S E T  F A C E  S T R I N G                                                                                       *
 *  ===========================                                                                                       *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Save a string in to one of the positions on the face.
 *  \param faceSetting Which face to save to.
 *  \param str Which string is being set.
 *  \param shorten Should words be made shorter to fit.
 *  \param format Format of the string.
 *  \param ... Varible argument list.
 *  \result None.
 */
void setFaceString (FACE_SETTINGS *faceSetting, int str, int shorten, char *format, ...)
{
	va_list arg_ptr;

	va_start (arg_ptr, format);
	vSetFaceString (faceSetting, str, shorten, format, arg_ptr);
	va_end (arg_ptr);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  M A I N                                                                                                           *
 *  =======                                                                                                           *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief The program starts here.
 *  \param argc The number of arguments passed to the program.
 *  \param argv Pointers to the arguments passed to the program.
 *  \result 0 (zero) if all process OK.
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
	gtk_window_set_default_icon_name (PACKAGE);

	dialConfig.mainWindow = GTK_WINDOW (gtk_window_new (GTK_WINDOW_TOPLEVEL));
	gtk_window_set_title (GTK_WINDOW (dialConfig.mainWindow), PACKAGE_NAME);

	/*------------------------------------------------------------------------------------------------*
	 * There must be at least one face.                                                               *
	 *------------------------------------------------------------------------------------------------*/
	faceSettings[0] = malloc (sizeof (FACE_SETTINGS));
	memset (faceSettings[0], 0, sizeof (FACE_SETTINGS));

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

	loadConfig (&posX, &posY);
	processCommandLine (argc, argv, &posX, &posY);
	setupDisplay();
	
	saveFace = currentFace;
	for (i = 0; i < (dialConfig.dialWidth * dialConfig.dialHeight); i++)
	{
		currentFace = i;
		if (faceSettings[i] == NULL)
		{
			faceSettings[i] = malloc (sizeof (FACE_SETTINGS));
			memset (faceSettings[i], 0, sizeof (FACE_SETTINGS));
		}
		switch (faceSettings[i] -> showFaceType)
		{
		case FACE_TYPE_BATTERY:
			batteryCallback (faceSettings[i] -> faceSubType);
			break;
		case FACE_TYPE_ENTROPY:
			entropyCallback (faceSettings[i] -> faceSubType);
			break;
		case FACE_TYPE_TIDE:
			tideCallback (faceSettings[i] -> faceSubType);
			break;
		case FACE_TYPE_CPU_LOAD:
			loadCallback (faceSettings[i] -> faceSubType);
			break;
		case FACE_TYPE_MEMORY:
			memoryCallback (faceSettings[i] -> faceSubType);
			break;
		case FACE_TYPE_NETWORK:
			networkCallback (faceSettings[i] -> faceSubType);
			break;
		case FACE_TYPE_HARDDISK:
			harddiskCallback (faceSettings[i] -> faceSubType);
			break;
		case FACE_TYPE_SENSOR_TEMP:
			sensorTempCallback (faceSettings[i] -> faceSubType);
			break;
		case FACE_TYPE_SENSOR_FAN:
			sensorFanCallback (faceSettings[i] -> faceSubType);
			break;
		case FACE_TYPE_THERMO:
			thermometerCallback (faceSettings[i] -> faceSubType);
			break;
		case FACE_TYPE_POWER:
			powerMeterCallback (faceSettings[i] -> faceSubType);
			break;
		case FACE_TYPE_WEATHER:
			weatherCallback (faceSettings[i] -> faceSubType);
			break;
		case FACE_TYPE_MOONPHASE:
			moonPhaseCallback (faceSettings[i] -> faceSubType);
			break;

		case FACE_TYPE_WIFI:
			wifiCallback (faceSettings[i] -> faceSubType);
			break;
		}
	}
	currentFace = saveFace;


	/*------------------------------------------------------------------------------------------------*
	 * Do all the other windows initialisation.                                                       *
	 *------------------------------------------------------------------------------------------------*/
	gtk_window_set_resizable (GTK_WINDOW (dialConfig.mainWindow), FALSE);

	/*------------------------------------------------------------------------------------------------*
	 * Icon stuff.                                                                                    *
	 *------------------------------------------------------------------------------------------------*/
	defaultIcon = gdk_pixbuf_new_from_xpm_data ((const char **) &GaugeIcon_xpm);

	/*------------------------------------------------------------------------------------------------*
	 * Final windows configuration.                                                                   *
	 *------------------------------------------------------------------------------------------------*/
    dialInit (&dialConfig);
	
	/*------------------------------------------------------------------------------------------------*
	 * This is the first time we can do this because we check the screen size in this routine.        *
	 *------------------------------------------------------------------------------------------------*/
	dialFixFaceSize ();
	
	/*------------------------------------------------------------------------------------------------*
	 * Final windows configuration.                                                                   *
	 *------------------------------------------------------------------------------------------------*/
#if GTK_MAJOR_VERSION == 2
	g_signal_connect (G_OBJECT (dialConfig.drawingArea), "expose_event", G_CALLBACK (exposeCallback), NULL);
#else
	g_signal_connect (G_OBJECT (dialConfig.drawingArea), "draw", G_CALLBACK (drawCallback), NULL);
#endif
	g_signal_connect (G_OBJECT (dialConfig.mainWindow), "button_press_event", G_CALLBACK (windowClickCallback), NULL);
	g_signal_connect (G_OBJECT (dialConfig.mainWindow), "key_press_event", G_CALLBACK (windowKeyCallback), NULL);
	g_signal_connect (G_OBJECT (dialConfig.mainWindow), "key_release_event", G_CALLBACK (windowKeyCallback), NULL);
	g_signal_connect (G_OBJECT (dialConfig.mainWindow), "destroy", G_CALLBACK (quitCallback), NULL);
	g_signal_connect (G_OBJECT (dialConfig.mainWindow), "motion-notify-event", G_CALLBACK(userActive), NULL); 

	g_signal_connect (G_OBJECT (dialConfig.mainWindow), "focus-in-event", G_CALLBACK(focusInEvent), NULL);
	g_signal_connect (G_OBJECT (dialConfig.mainWindow), "focus-out-event", G_CALLBACK(focusOutEvent), NULL);
 	eventBox = gtk_event_box_new ();

	gtk_container_add (GTK_CONTAINER (eventBox), dialConfig.drawingArea);
	gtk_container_add (GTK_CONTAINER (dialConfig.mainWindow), eventBox);

#ifndef GAUGE_IS_DECORATED
	gtk_window_set_decorated (GTK_WINDOW (dialConfig.mainWindow), FALSE);
#endif

	/*------------------------------------------------------------------------------------------------*
	 * Complete stuff left over from the command line                                                 *
	 *------------------------------------------------------------------------------------------------*/
	if (posX != -1 && posY != -1)
	{
		int width = 1024, height = 768;

		dialGetScreenSize (&width, &height);

		if (posX == -2)
			posX = (width - (dialConfig.dialWidth * dialConfig.dialSize)) / 2;
		if (posY == -2)
			posY = (height - (dialConfig.dialHeight * dialConfig.dialSize)) / 2;
		if (posX == -3)
			posX = width - (dialConfig.dialWidth * dialConfig.dialSize);
		if (posY == -3)
			posY = height - (dialConfig.dialHeight * dialConfig.dialSize);
		if (posX > width - 64)
			posX = width - 64;
		if (posY > height - 64)
			posY = height - 64;

		gtk_window_move (dialConfig.mainWindow, posX, posY);
	}

#if GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION > 11)
	gtk_widget_set_tooltip_markup (GTK_WIDGET (dialConfig.mainWindow), "Gauge");
#endif

	/*------------------------------------------------------------------------------------------------*
	 * Intitalise all fo the gauges                                                                   *
	 *------------------------------------------------------------------------------------------------*/
	readCPUInit();
	readBatteryInit();
	readEntropyInit();
	readMemoryInit();
	readNetworkInit();
	readHarddiskInit();
	readSensorInit ();
	readTideInit();
	readWeatherInit();
	readThermometerInit();
	readPowerMeterInit();
	readMoonPhaseInit();
	readWifiInit();

	/*------------------------------------------------------------------------------------------------*
	 * Called to set any values                                                                       *
	 *------------------------------------------------------------------------------------------------*/
	accelGroup = gtk_accel_group_new ();
	gtk_window_add_accel_group (dialConfig.mainWindow, accelGroup);
	createMenu (mainMenuDesc, accelGroup, FALSE);

	stickCallback (0);
	onTopCallback (0);
#ifndef GAUGE_IS_DECORATED
	lockCallback (0);
#endif

	/*------------------------------------------------------------------------------------------------*
	 * OK all ready lets run it!                                                                      *
	 *------------------------------------------------------------------------------------------------*/
	gtk_widget_show_all (GTK_WIDGET (dialConfig.mainWindow));
	g_timeout_add (200, clockTickCallback, NULL);
	dialSetOpacity();

	i = nice (5);
	gtk_main ();
	exit (0);
}


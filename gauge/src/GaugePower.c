/**********************************************************************************************************************
 *                                                                                                                    *
 *  G A U G E  P O W E R . C                                                                                          *
 *  ========================                                                                                          *
 *                                                                                                                    *
 *  Copyright (c) 2023 Chris Knight                                                                                   *
 *                                                                                                                    *
 *  File GaugePower.c part of Gauge is free software: you can redistribute it and/or modify it under the terms of     *
 *  the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or  *
 *  (at your option) any later version.                                                                               *
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
 *  \brief Routines to display the powermeter information.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <dirent.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "config.h"
#include "socketC.h"
#include "GaugeDisp.h"

extern FACE_SETTINGS *faceSettings[];
extern GAUGE_ENABLED gaugeEnabled[];
extern MENU_DESC gaugeMenuDesc[];
extern DIAL_CONFIG dialConfig;
extern char powerServer[];
extern int powerPort;

#define POWER_STATE_UPDATED	0
#define POWER_STATE_PENDING	1
#define POWER_STATE_ERROR	2
#define POWER_MAX_READING	18

static time_t lastRead;
static int powerState;
static int powerStart = 1;
static pthread_t threadHandle = 0;

typedef struct _powerReading
{
	char *name;
	char *display;
	char *formatLow;
	char *formatHigh;
	double value;
	char showValue[21];
}
PowerReading;

PowerReading myPowerReading[POWER_MAX_READING] =
{
	{ "now", "Current", "%0.0fW", "%0.1fKW", 0, "NA" },				//  0
	{ "max", "Maximum", "%0.0fW", "%0.1fKW", 0, "NA" },				//  1
	{ "min", "Minimum", "%0.0fW", "%0.1fKW", 0, "NA" },				//  2
	{ "minavg", "Minute", "%0.0fW", "%0.1fKW", 0, "NA" },			//  3	0
	{ "houravg", "Hour", "%0.0fW", "%0.1fKW", 0, "NA" },			//  4	1
	{ "dayavg", "Day", "%0.0fW", "%0.1fKW", 0, "NA" },				//  5	2
	{ "monthavg", "Month", "%0.0fW", "%0.1fKW", 0, "NA" },			//  6	3
	{ "yearavg", "Year", "%0.0fW", "%0.1fKW", 0, "NA" },			//  7	4
	{ "minmin", "Minute Minimum", "%0.0fW", "%0.1fKW", 0, "NA" },	//  8
	{ "hourmin", "Hour Minimum", "%0.0fW", "%0.1fKW", 0, "NA" },	//  9
	{ "daymin", "Day Minimum", "%0.0fW", "%0.1fKW", 0, "NA" },		// 10
	{ "monthmin", "Month Minimum", "%0.0fW", "%0.1fKW", 0, "NA" },	// 11
	{ "yearmin", "Year Minimum", "%0.0fW", "%0.1fKW", 0, "NA" },	// 12
	{ "minmax", "Minute Maximum", "%0.0fW", "%0.1fKW", 0, "NA" },	// 13
	{ "hourmax", "Hour Maximum", "%0.0fW", "%0.1fKW", 0, "NA" },	// 14
	{ "daymax", "Day Maximum", "%0.0fW", "%0.1fKW", 0, "NA" },		// 15
	{ "monthmax", "Month Maximum", "%0.0fW", "%0.1fKW", 0, "NA" },	// 16
	{ "yearmax", "Year Maximum", "%0.0fW", "%0.1fKW", 0, "NA" }		// 17
};

/**********************************************************************************************************************
 *                                                                                                                    *
 *  R E A D  P O W E R  M E T E R  I N I T                                                                            *
 *  ======================================                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Called once at the program start to find the devices.
 *  \result None 0 all is OK.
 */
void readPowerMeterInit (void)
{
	if (gaugeEnabled[FACE_TYPE_POWER].enabled)
	{
		int clientSock = ConnectClientSocket (powerServer, powerPort, 3, USE_ANY, NULL);
		if (SocketValid (clientSock))
		{
			gaugeMenuDesc[MENU_GAUGE_POWER].disable = 0;
			CloseSocket (&clientSock);
		}
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  P R O C E S S  P O W E R  K E Y                                                                                   *
 *  ===============================                                                                                   *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Process each of the fields in the XML.
 *  \param readLevel 0 current, 1 today, 2 tomorrow.
 *  \param name Name of the field.
 *  \param value Value of the field.
 *  \result None.
 */
static void processPowerKey (int readLevel, const char *name, char *value)
{
	int i;
	for (i = 0; i < POWER_MAX_READING; ++i)
	{
		if (readLevel == 1)
		{
			if (strcmp (name, myPowerReading[i].name) == 0)
			{
				myPowerReading[i].value = atof (value);
				if (myPowerReading[i].value == -1)
				{
					strcpy (myPowerReading[i].showValue, "NA");
				}
				else if (myPowerReading[i].value >= 1000)
				{
					sprintf (myPowerReading[i].showValue, myPowerReading[i].formatHigh, myPowerReading[i].value / 1000);
				}
				else
				{
					sprintf (myPowerReading[i].showValue, myPowerReading[i].formatLow, myPowerReading[i].value);
				}
			}
		}
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  P R O C E S S  E L E M E N T  N A M E S                                                                           *
 *  =======================================                                                                           *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Process each of the elements in the file.
 *  \param doc Document to read.
 *  \param aNode Current node.
 *  \param readLevel 0 current, 1 today, 2 tomorrow.
 *  \result None.
 */
static void processElementNames (xmlDoc *doc, xmlNode * aNode, int readLevel)
{
	xmlChar *key;
	xmlNode *curNode = NULL;

	for (curNode = aNode; curNode; curNode = curNode->next)
	{
		if (curNode->type == XML_ELEMENT_NODE)
		{
			if ((!xmlStrcmp (curNode -> name, (const xmlChar *)"power")))
			{
				++readLevel;
				lastRead = time (NULL);
			}
			else
			{
				key = xmlNodeListGetString (doc, curNode -> xmlChildrenNode, 1);
				processPowerKey (readLevel, (const char *)curNode -> name, (char *)key);
				xmlFree (key);
			}
		}
		processElementNames (doc, curNode->children, readLevel);
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  P R O C E S S  B U F F E R                                                                                        *
 *  ==========================                                                                                        *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Process the down loaded buffer.
 *  \param buffer Buffer to process.
 *  \param size Size of the buffer.
 *  \result None.
 */
static void processBuffer (char *buffer, size_t size)
{
	xmlDoc *doc = NULL;
	xmlNode *rootElement = NULL;
	xmlChar *xmlBuffer = NULL;

	xmlBuffer = xmlCharStrndup (buffer, size);
	if (xmlBuffer != NULL)
	{
		doc = xmlParseDoc (xmlBuffer);

		if (doc != NULL)
		{
			rootElement = xmlDocGetRootElement (doc);
			processElementNames (doc, rootElement, 0);
			xmlFreeDoc (doc);
		}
		else
		{
			printf ("error: could not parse memory\n");
			printf ("BUFF[%lu] [[[%s]]]\n", size, buffer);
		}
		xmlFree (xmlBuffer);
	}
	xmlCleanupParser ();
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  R E A D  P O W E R  M E T E R  I N F O                                                                            *
 *  ======================================                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Read the current tempature from the powermeter.
 *  \result None.
 */
void *readPowerMeterInfo (void *notUsed)
{
	char buffer[512] = "";
	int bytesRead = 0;
	
	powerState = POWER_STATE_PENDING;
	int clientSock = ConnectClientSocket (powerServer, powerPort, 3, USE_ANY, NULL);
	if (SocketValid (clientSock))
	{
		if (WaitSocket (clientSock, 2) > 0)
		{
			bytesRead = RecvSocket (clientSock, buffer, 511);
		}
		CloseSocket (&clientSock);
	}
	if (bytesRead)
	{
		processBuffer (buffer, bytesRead);
		powerStart = 0;
		powerState = POWER_STATE_UPDATED;
	}
	else
	{
		powerState = POWER_STATE_ERROR;
	}
	return NULL;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S T A R T  U P D A T E  P O W E R  I N F O                                                                        *
 *  ==========================================                                                                        *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Create a thread to read the tide times.
 *  \result None.
 */
void startUpdatePowerInfo()
{
	if (powerState != POWER_STATE_PENDING)
	{
		if (threadHandle != 0)
		{
			pthread_join(threadHandle, NULL);
		}
		pthread_create (&threadHandle, NULL, readPowerMeterInfo, NULL);
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  R E A D  P O W E R  M E T E R  V A L U E S                                                                        *
 *  ==========================================                                                                        *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Calculate the value to show on the face.
 *  \param face Which face to display on.
 *  \result None zero if the face should be re-shown.
 */
void readPowerMeterValues (int face)
{
	if (gaugeEnabled[FACE_TYPE_POWER].enabled)
	{
		FACE_SETTINGS *faceSetting = faceSettings[face];
		char readTimeStr[81] = "Never";

		if (faceSetting -> faceFlags & FACE_REDRAW)
		{
			;
		}
		else if (faceSetting -> nextUpdate)
		{
			faceSetting -> nextUpdate -= 1;
			return;
		}
		else if (!faceSetting -> nextUpdate)
		{
			startUpdatePowerInfo ();
			faceSetting -> nextUpdate = (powerStart ? 5 : 60);
		}

		if (lastRead != 0)
		{
			struct tm readTime;
			localtime_r (&lastRead, &readTime);
			strftime (readTimeStr, 80, "%e/%b %k:%M:%S", &readTime);
		}

		setFaceString (faceSetting, FACESTR_TOP, 0, "Power\nMeter");
		setFaceString (faceSetting, FACESTR_TIP, 0, _(
				"<b>%s</b>: %s\n"
				"<b>%s</b>: %s\n"
				"<b>%s</b>: %s\n"
				"<b>%s</b>: %s (%s to %s)\n"
				"<b>%s</b>: %s (%s to %s)\n"
				"<b>%s</b>: %s (%s to %s)\n"
				"<b>%s</b>: %s (%s to %s)\n"
				"<b>Read</b>: %s"),
				myPowerReading[0].display, myPowerReading[0].showValue,
				myPowerReading[1].display, myPowerReading[1].showValue,
				myPowerReading[2].display, myPowerReading[2].showValue,
				myPowerReading[4].display, myPowerReading[4].showValue, myPowerReading[9].showValue, myPowerReading[14].showValue,
				myPowerReading[5].display, myPowerReading[5].showValue, myPowerReading[10].showValue, myPowerReading[15].showValue,
				myPowerReading[6].display, myPowerReading[6].showValue, myPowerReading[11].showValue, myPowerReading[16].showValue,
				myPowerReading[7].display, myPowerReading[7].showValue, myPowerReading[12].showValue, myPowerReading[17].showValue,
				readTimeStr);
		setFaceString (faceSetting, FACESTR_WIN, 0, _("%s: %s, %s: %s"),
				myPowerReading[0].display, myPowerReading[0].showValue,
				myPowerReading[5].display, myPowerReading[5].showValue);
		setFaceString (faceSetting, FACESTR_BOT, 0, "%s\n(%s)", 
				myPowerReading[0].showValue, myPowerReading[5].showValue);
		faceSetting -> firstValue = myPowerReading[0].value / 1000;
		faceSetting -> secondValue = myPowerReading[5].value / 1000;

		while (faceSetting -> firstValue > faceSetting -> faceScaleMax)
		{
			faceSetting -> faceScaleMax *= 2;
			maxMinReset (&faceSetting -> savedMaxMin, 10, 2);
		}
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  P O W E R  S E T T I N G S                                                                                        *
 *  ==========================                                                                                        *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Configure the server and port to be used.
 *  \param data 1 if from the menu, 0 on startup.
 *  \result None.
 */
void powerSettings(guint data)
{
	GtkWidget *dialog;
	GtkWidget *vbox;
	GtkWidget *label;
	GtkWidget *entryServer;
	GtkWidget *spinPort;
	GtkWidget *contentArea;
	GtkWidget *grid;

	dialog = gtk_dialog_new_with_buttons("Power Settings", GTK_WINDOW(dialConfig.mainWindow),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			_("_OK"), GTK_RESPONSE_ACCEPT, _("_Cancel"), GTK_RESPONSE_REJECT, NULL);

	contentArea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 3);
	gtk_box_pack_start(GTK_BOX(contentArea), vbox, TRUE, TRUE, 0);
	grid = gtk_grid_new();

	label = gtk_label_new(_("Server: "));
	gtk_widget_set_halign (label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), label, 1, 1, 1, 1);
	entryServer = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entryServer), 30);
	gtk_entry_set_text(GTK_ENTRY(entryServer), powerServer);
	gtk_grid_attach(GTK_GRID(grid), entryServer, 2, 1, 1, 1);

	label = gtk_label_new(_("Port: "));
	gtk_widget_set_halign (label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), label, 1, 2, 1, 1);
	spinPort = gtk_spin_button_new_with_range(1, 64000, 1);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(spinPort), powerPort);
	gtk_grid_attach(GTK_GRID(grid), spinPort, 2, 2, 1, 1);

	gtk_box_pack_start(GTK_BOX(vbox), grid, FALSE, FALSE, 0);
	gtk_widget_show_all(dialog);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
	{
		const char *saveText = gtk_entry_get_text(GTK_ENTRY(entryServer));
		int saveValue = (int)gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinPort));
		
		if (strcmp(saveText, powerServer) != 0)
		{
			strncpy(powerServer, saveText, 40);
			configSetValue ("power_server", powerServer);
		}
		if (powerPort != saveValue)
		{
			powerPort = saveValue;
			configSetIntValue ("power_port", powerPort);
		}
	}
	gtk_widget_destroy(dialog);
}


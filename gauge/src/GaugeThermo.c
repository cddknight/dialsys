/**********************************************************************************************************************
 *                                                                                                                    *
 *  G A U G E  T H E R M O . C                                                                                        *
 *  ==========================                                                                                        *
 *                                                                                                                    *
 *  Copyright (c) 2023 Chris Knight                                                                                   *
 *                                                                                                                    *
 *  File GaugeThermo.c part of Gauge is free software: you can redistribute it and/or modify it under the terms of    *
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
 *  \brief Routines to display the thermometer information.
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
extern char thermoServer[];
extern int thermoPort;

#define THERMO_STATE_UPDATED	0
#define THERMO_STATE_PENDING	1
#define THERMO_STATE_ERROR		2

static int thermoState;
static int thermoStart = 1;
static pthread_t threadHandle = 0;
static time_t lastRead;

double myThermoReading[5] = { 0, 0, 0, 0, 0 };

/**********************************************************************************************************************
 *                                                                                                                    *
 *  R E A D  T H E R M O M E T E R  I N I T                                                                           *
 *  =======================================                                                                           *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Called once at the program start to find the devices.
 *  \result None 0 all is OK.
 */
void readThermometerInit (void)
{
	if (gaugeEnabled[FACE_TYPE_THERMO].enabled)
	{
		int clientSock = ConnectClientSocket (thermoServer, thermoPort, 3, USE_ANY, NULL);
		if (SocketValid (clientSock))
		{
			gaugeMenuDesc[MENU_GAUGE_THERMO].disable = 0;
			CloseSocket (&clientSock);
		}
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  P R O C E S S  T H E R M O  K E Y                                                                                 *
 *  =================================                                                                                 *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Process each of the fields in the XML.
 *  \param readLevel 0 current, 1 today, 2 tomorrow.
 *  \param name Name of the field.
 *  \param value Value of the field.
 *  \result None.
 */
static void processThermoKey (int readLevel, const char *name, char *value)
{
	if (readLevel == 1 && strcmp (name, "outside") == 0)
		myThermoReading[0] = atof (value);
	if (readLevel == 1 && strcmp (name, "inside") == 0)
		myThermoReading[1] = atof (value);
	if (readLevel == 1 && strcmp (name, "pressure") == 0)
		myThermoReading[2] = atof (value);
	if (readLevel == 1 && strcmp (name, "light") == 0)
		myThermoReading[3] = atof (value);
	if (readLevel == 1 && strcmp (name, "humidity") == 0)
		myThermoReading[4] = atof (value);
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
			if ((!xmlStrcmp (curNode -> name, (const xmlChar *)"sensors")))
			{
				++readLevel;
				lastRead = time (NULL);
			}
			else
			{
				key = xmlNodeListGetString (doc, curNode -> xmlChildrenNode, 1);
				processThermoKey (readLevel, (const char *)curNode -> name, (char *)key);
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
 *  R E A D  T H E R M O M E T E R  I N F O                                                                           *
 *  =======================================                                                                           *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Read the current tempature from the thermometer.
 *  \result None.
 */
void *readThermometerInfo ()
{
	char buffer[512] = "";
	int bytesRead = 0;

	thermoState = THERMO_STATE_PENDING;
	int clientSock = ConnectClientSocket (thermoServer, thermoPort, 3, USE_ANY, NULL);
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
		thermoStart = 0;
		thermoState = THERMO_STATE_UPDATED;
	}
	else
	{
		thermoState = THERMO_STATE_ERROR;
	}			
	return NULL;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S T A R T  U P D A T E  T H E R M O  I N F O                                                                      *
 *  ============================================                                                                      *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Create a thread to read the tide times.
 *  \result None.
 */
void startUpdateThermoInfo()
{
	if (thermoState != THERMO_STATE_PENDING)
	{
		if (threadHandle != 0)
		{
			pthread_join(threadHandle, NULL);
		}
		pthread_create (&threadHandle, NULL, readThermometerInfo, NULL);
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  R E A D  T H E R M O M E T E R  V A L U E S                                                                       *
 *  ===========================================                                                                       *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Calculate the value to show on the face.
 *  \param face Which face to display on.
 *  \result None zero if the face should be re-shown.
 */
void readThermometerValues (int face)
{
	if (gaugeEnabled[FACE_TYPE_THERMO].enabled)
	{
		char readTimeStr[81] = "Never";
		FACE_SETTINGS *faceSetting = faceSettings[face];

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
			startUpdateThermoInfo ();
			faceSetting -> nextUpdate = (thermoStart ? 5 : 120);
		}
		if (lastRead != 0)
		{
			struct tm readTime;
			localtime_r (&lastRead, &readTime);
			strftime (readTimeStr, 80, "%e/%b %k:%M:%S", &readTime);
		}

		setFaceString (faceSetting, FACESTR_TOP, 0, "Thermometer");
		setFaceString (faceSetting, FACESTR_TIP, 0, _("<b>Outside</b>: %0.1f\302\260C\n<b>Inside</b>: %0.1f\302\260C\n"
				"<b>Pressure</b>: %0.0fmb\n<b>Brightness</b>: %0.0flux\n<b>Humidity</b>: %0.0f%%\n"
				"<b>Read</b>: %s"),
				myThermoReading[0], myThermoReading[1], myThermoReading[2], myThermoReading[3], myThermoReading[4],
				readTimeStr);
		setFaceString (faceSetting, FACESTR_WIN, 0, _("Outside: %0.1f%s, Inside: %0.1f%s"),
				myThermoReading[0], "\302\260C", myThermoReading[1], "\302\260C");
		setFaceString (faceSetting, FACESTR_BOT, 0, "%0.1f%s\n(%0.1f%s)",
				myThermoReading[0], "\302\260C", myThermoReading[1], "\302\260C");
		faceSetting -> firstValue = myThermoReading[0];
		faceSetting -> secondValue = myThermoReading[1];
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  T H E R M O M E T E R  S E T T I N G S                                                                            *
 *  ======================================                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Configure the server and port to be used.
 *  \param data 1 if from the menu, 0 on startup.
 *  \result None.
 */
void thermometerSettings(guint data)
{
	GtkWidget *dialog;
	GtkWidget *vbox;
	GtkWidget *label;
	GtkWidget *entryServer;
	GtkWidget *spinPort;
	GtkWidget *contentArea;
	GtkWidget *grid;

	dialog = gtk_dialog_new_with_buttons("Thermometer Settings", GTK_WINDOW(dialConfig.mainWindow),
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
	gtk_entry_set_text(GTK_ENTRY(entryServer), thermoServer);
	gtk_grid_attach(GTK_GRID(grid), entryServer, 2, 1, 1, 1);

	label = gtk_label_new(_("Port: "));
	gtk_widget_set_halign (label, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), label, 1, 2, 1, 1);
	spinPort = gtk_spin_button_new_with_range(1, 64000, 1);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(spinPort), thermoPort);
	gtk_grid_attach(GTK_GRID(grid), spinPort, 2, 2, 1, 1);

	gtk_box_pack_start(GTK_BOX(vbox), grid, FALSE, FALSE, 0);
	gtk_widget_show_all(dialog);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
	{
		const char *saveText = gtk_entry_get_text(GTK_ENTRY(entryServer));
		int saveValue = (int)gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinPort));
		
		if (strcmp(saveText, thermoServer) != 0)
		{
			strncpy(thermoServer, saveText, 40);
			configSetValue ("thermo_server", thermoServer);
		}
		if (thermoPort != saveValue)
		{
			thermoPort = saveValue;
			configSetIntValue ("thermo_port", thermoPort);
		}
	}
	gtk_widget_destroy(dialog);
}


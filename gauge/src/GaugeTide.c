/**********************************************************************************************************************
 *                                                                                                                    *
 *  G A U G E  T I D E . C                                                                                            *
 *  ======================                                                                                            *
 *                                                                                                                    *
 *  Copyright (c) 2023 Chris Knight                                                                                   *
 *                                                                                                                    *
 *  File GaugeTide.c part of Gauge is free software: you can redistribute it and/or modify it under the terms of the  *
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
 *  \brief Display a gauge showing the current state of the tide.
 */
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <curl/curl.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/HTMLparser.h>
#include <glib-object.h>
#include <json-glib/json-glib.h>

#include "GaugeDisp.h"

extern FACE_SETTINGS *faceSettings[];
extern GAUGE_ENABLED gaugeEnabled[];
extern MENU_DESC gaugeMenuDesc[];
extern DIAL_CONFIG dialConfig;
extern int sysUpdateID;
extern char tideURL[];
extern char tideAPIKey[];

#define MAX_SAVE_TIDES	21
#define TIDE_STATE_UPDATED	0
#define TIDE_STATE_PENDING	1
#define TIDE_STATE_SHOWN	2

static int tideState;
static pthread_t threadHandle;

struct MemoryStruct
{
  char *memory;
  size_t size;
};

struct TideTime
{
	time_t tideTime;
	double tideHeight;
	char tideType;
	char tideSet;
};

struct TideInfo
{
	char location[41];
	char country[41];
	int locRead;
	struct TideTime tideTimes[MAX_SAVE_TIDES];
	time_t readTime;
};

static struct TideInfo tideInfo;
static int lastReadTide;

static int myUpdateID = -1;
static time_t tideDuration = 22358;
static char *days[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
static char *months[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
static char *urlPrefix = "https://admiraltyapi.azure-api.net/uktidalapi/api/V1/Stations/%04d";
static char *urlSuffix = "/TidalEvents?duration=3";

void jsonArrayForEachFunc (JsonArray *array, guint index_, JsonNode *element_node, gpointer user_data);
void jsonObjectForEachFunc(JsonObject *object, const gchar *member_name, JsonNode *member_node, gpointer user_data);

/**********************************************************************************************************************
 *                                                                                                                    *
 *  P R O P E R  C A S E  W O R D                                                                                     *
 *  =============================                                                                                     *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief We get name in uppercase so convert to propercase.
 *  \param word Word to convert.
 *  \result None.
 */
static void properCaseWord (char *word)
{
	int i = 0, start = 1;

	while (word[i] != 0)
	{
		if (word[i] >= 'A' && word[i] <= 'Z')
		{
			if (!start) word[i] = (word[i] - 'A') + 'a';
			start = 0;
		}
		else if (word[i] >= 'a' && word[i] <= 'z')
		{
			if (start) word[i] = (word[i] - 'a') + 'A';
			start = 0;
		}
		else
		{
			start = 1;
		}
		++i;
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  G E T  L O C A L  N E X T  M I D D A Y                                                                            *
 *  ======================================                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Get the epoch for the next midday.
 *  \result time_t of the next midday for the update.
 */
static time_t getLocalNextMidday ()
{
	struct tm localTime;
	time_t now = time(NULL), retn;
	localtime_r (&now, &localTime);
	localTime.tm_hour = 12;
	localTime.tm_min = 0;
	retn = mktime (&localTime);
	if (retn < now)
	{
		retn += (24 * 60 *60);
	}
	return retn;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  G E T  N E X T  T I D E                                                                                           *
 *  =======================                                                                                           *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Look at the loaded times and see what is next.
 *  \result Number of next tide or -1 if none found.
 */
static int getNextTide ()
{
	int i, retn = -1;
	time_t now = time(NULL);

	for (i = 0; i < lastReadTide && retn == -1; ++i)
	{
		if (tideInfo.tideTimes[i].tideTime > now && tideInfo.tideTimes[i].tideSet)
		{
			retn = i;
		}
	}
	if (retn == -1)
	{
		for (i = 0; i < MAX_SAVE_TIDES; ++i)
		{
			memset (&tideInfo.tideTimes[i], 0, sizeof (struct TideTime));
		}
		if (!tideInfo.locRead) strcpy (tideInfo.location, "Pending");
		tideInfo.tideTimes[0].tideTime = tideInfo.readTime = time (NULL);
		tideInfo.tideTimes[0].tideType = 'L';
		retn = 0;
	}
	return retn;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S C A N  D A T E  T I M E                                                                                         *
 *  =========================                                                                                         *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Read a date out of a string.
 *  \param index Which tide is this for.
 *  \param strValue String to read date from.
 *  \result None.
 */
void scanDateTime (int index, const char *strValue)
{
	int i = 0, c = 0, v = 0;
	struct tm tideTime;
	time_t now = time(NULL);

	gmtime_r (&now, &tideTime);
	while (1)
	{
		if (strValue[i] >= '0' && strValue[i] <= '9')
		{
			c = (c * 10) + (strValue[i] - '0');
		}
		else if (strValue[i] == '-' || strValue[i] == ':' || strValue[i] == 'T' || strValue[i] == 0)
		{
			switch (v)
			{
			case 0:
				tideTime.tm_year = c - 1900;
				break;
			case 1:
				tideTime.tm_mon = c - 1;
				break;
			case 2:
				tideTime.tm_mday = c;
				break;
			case 3:
				tideTime.tm_hour = c;
				break;
			case 4:
				tideTime.tm_min = c;
				tideTime.tm_sec = 0;
				tideTime.tm_isdst = 0;
				break;
			}
			c = 0;
			++v;
		}
		if (strValue[i] == 0)
		{
			tideInfo.tideTimes[index].tideTime = mktime(&tideTime);
			break;
		}
		++i;
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  W R I T E  M E M O R Y  C A L L B A C K                                                                           *
 *  =======================================                                                                           *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Called back by curl to write page to memoy.
 *  \param ptr Pointer to the data.
 *  \param size Size of the data.
 *  \param nmemb Size of the data parts.
 *  \param data Pointer to my data structure.
 *  \result Total size.
 */
static size_t
writeMemoryCallback (void *ptr, size_t size, size_t nmemb, void *data)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)data;

	mem -> memory = realloc (mem -> memory, mem -> size + realsize + 1);
	if (mem -> memory == NULL)
		return realsize;

	memcpy (&(mem -> memory[mem->size]), ptr, realsize);
	mem -> size += realsize;
	mem -> memory[mem->size] = 0;
	return realsize;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I S P L A Y  V A L U E                                                                                          *
 *  ========================                                                                                          *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Display the contents of a value.
 *  \param name Process a json name value.
 *  \param index Which tide we are reading.
 *  \param value Value to display.
 *  \result None.
 */
void displayValue (char *name, int index, GValue *value)
{
	if (index < MAX_SAVE_TIDES)
	{
		if (G_VALUE_HOLDS (value, G_TYPE_STRING))
		{
			const char *strValue = g_value_get_string (value);
			if (strcmp (name, "EventType") == 0)
			{
				tideInfo.tideTimes[index].tideType = strValue[0];
			}
			else if (strcmp (name, "DateTime") == 0)
			{
				scanDateTime (index, strValue);
			}
			else if (strcmp (name, "Date") == 0)
			{
				tideInfo.tideTimes[index].tideSet = 1;
				tideInfo.readTime = getLocalNextMidday();
				lastReadTide = index;
			}
			else if (strcmp (name, "Name") == 0)
			{
				strcpy (tideInfo.location, strValue);
				properCaseWord (tideInfo.location);
				tideInfo.locRead = 1;
			}
			else if (strcmp (name, "Country") == 0)
			{
				strcpy (tideInfo.country, strValue);
				properCaseWord (tideInfo.country);
			}
		}
		else if (G_VALUE_HOLDS (value, G_TYPE_BOOLEAN))
		{
		}
		else
		{
			GValue number = G_VALUE_INIT;
			g_value_init (&number, G_TYPE_DOUBLE);
			if (g_value_transform (value, &number))
			{
				double num = g_value_get_double (&number);
				if (strcmp (name, "Height") == 0)
				{
					tideInfo.tideTimes[index].tideHeight = num;
				}
			}
			g_value_unset (&number);
		}
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  J S O N  A R R A Y  F O R  E A C H  F U N C                                                                       *
 *  ===========================================                                                                       *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Process the elements in an array.
 *  \param array Array we are processing.
 *  \param index_ Index of this element.
 *  \param element_node The elements node.
 *  \param user_data User data (at the moment this is level).
 *  \result None.
 */
void jsonArrayForEachFunc (JsonArray *array, guint index_, JsonNode *element_node, gpointer user_data)
{
	int index = index_;

	if (element_node)
	{
		if (json_node_get_node_type (element_node) == JSON_NODE_OBJECT)
		{
			JsonObject *objectInner = json_node_get_object(element_node);
			if (objectInner != NULL)
			{
				json_object_foreach_member (objectInner, jsonObjectForEachFunc, &index);
			}
		}
		else if (json_node_get_node_type (element_node) == JSON_NODE_VALUE)
		{
			GValue value = G_VALUE_INIT;
			json_node_get_value (element_node, &value);
			displayValue ("Index", index, &value);
			g_value_unset (&value);
		}
		else if (json_node_get_node_type (element_node) == JSON_NODE_ARRAY)
		{
			JsonArray *array = json_node_get_array(element_node);
			if (array != NULL)
			{
				json_array_foreach_element (array, jsonArrayForEachFunc, &index);
			}
		}
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  J S O N  O B J E C T  F O R  E A C H  F U N C                                                                     *
 *  =============================================                                                                     *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Process the objects in a node.
 *  \param object Current object.
 *  \param member_name Name of the object.
 *  \param member_node The node of the object.
 *  \param user_data User data (at the moment this is level).
 *  \result None.
 */
void jsonObjectForEachFunc(JsonObject *object, const gchar *member_name, JsonNode *member_node, gpointer user_data)
{
	int index = -1;

	if (user_data != NULL)
	{
		index = *(int *)user_data;
	}

	if (member_node)
	{
		if (json_node_get_node_type (member_node) == JSON_NODE_OBJECT)
		{
			JsonObject *objectInner = json_node_get_object(member_node);
			if (objectInner != NULL)
			{
				json_object_foreach_member (objectInner, jsonObjectForEachFunc, NULL);
			}
		}
		else if (json_node_get_node_type (member_node) == JSON_NODE_VALUE)
		{
			GValue value = G_VALUE_INIT;
			json_node_get_value (member_node, &value);
			displayValue ((char *)member_name, index, &value);
			g_value_unset (&value);
		}
		else if (json_node_get_node_type (member_node) == JSON_NODE_ARRAY)
		{
			JsonArray *array = json_node_get_array(member_node);
			if (array != NULL)
			{
				json_array_foreach_element (array, jsonArrayForEachFunc, NULL);
			}
		}
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  P R O C E S S  B U F F E R                                                                                        *
 *  ==========================                                                                                        *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Process the xml memory buffer in libxml.
 *  \param buffer Buffer to process.
 *  \param size Size of the buffer.
 *  \result None.
 */
static void processBuffer (char *buffer, size_t size)
{
	JsonParser *parser;
	JsonNode *root;
	GError *error;

	parser = json_parser_new ();
	error = NULL;

	json_parser_load_from_data (parser, buffer, size, &error);
	if (error)
	{
		printf ("ERROR: %s\n", error -> message);
	}
	else
	{
		lastReadTide = 0;
		root = json_parser_get_root (parser);
		if (root != NULL)
		{
			if (json_node_get_node_type (root) == JSON_NODE_OBJECT)
			{
				JsonObject *object = json_node_get_object(root);
				if (object != NULL)
				{
					json_object_foreach_member (object, jsonObjectForEachFunc, NULL);
				}
			}
			else if (json_node_get_node_type (root) == JSON_NODE_ARRAY)
			{
				JsonArray *array = json_node_get_array(root);
				if (array != NULL)
				{
					json_array_foreach_element (array, jsonArrayForEachFunc, NULL);
				}
			}
		}
		g_object_unref (parser);
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  G E T  T I D E  T I M E S                                                                                         *
 *  =========================                                                                                         *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Get the tide times using curl.
 *  \param arg Not used.
 *  \result None.
 */
void *getTideTimes (void *arg)
{
	CURL *curlHandle;
	char longUrl[256], apiKey[256];
	struct MemoryStruct chunk;
	struct curl_slist *list = NULL;

	chunk.memory = malloc(1);
	chunk.size = 0;

	curl_global_init(CURL_GLOBAL_ALL);
	curlHandle = curl_easy_init();
	if (curlHandle)
	{
		strcpy (apiKey, "Ocp-Apim-Subscription-Key: ");
		strcat (apiKey, tideAPIKey);
		if (tideInfo.locRead == 0)
		{
			curl_easy_setopt (curlHandle, CURLOPT_URL, &tideURL[0]);
			curl_easy_setopt (curlHandle, CURLOPT_WRITEFUNCTION, writeMemoryCallback);
			curl_easy_setopt (curlHandle, CURLOPT_WRITEDATA, (void *)&chunk);
			curl_easy_setopt (curlHandle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
			list = curl_slist_append (list, "Accept: application/json");
			list = curl_slist_append (list, apiKey);
			curl_easy_setopt (curlHandle, CURLOPT_HTTPHEADER, list);
			curl_easy_perform (curlHandle);
			curl_easy_cleanup (curlHandle);

			if (chunk.memory)
			{
				if (chunk.size)
				{
					memset (&tideInfo, 0, sizeof (tideInfo));
					processBuffer (chunk.memory, chunk.size);
				}
			}
			tideState = TIDE_STATE_UPDATED;
		}
		else
		{
			strcpy (longUrl, tideURL);
			strcat (longUrl, urlSuffix);
			curl_easy_setopt (curlHandle, CURLOPT_URL, longUrl);
			curl_easy_setopt (curlHandle, CURLOPT_WRITEFUNCTION, writeMemoryCallback);
			curl_easy_setopt (curlHandle, CURLOPT_WRITEDATA, (void *)&chunk);
			curl_easy_setopt (curlHandle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
			list = curl_slist_append (list, "Accept: application/json");
			list = curl_slist_append (list, apiKey);
			curl_easy_setopt (curlHandle, CURLOPT_HTTPHEADER, list);
			curl_easy_perform (curlHandle);
			curl_easy_cleanup (curlHandle);

			if (chunk.memory)
			{
				if (chunk.size)
				{
					processBuffer (chunk.memory, chunk.size);
				}
			}
			tideState = TIDE_STATE_UPDATED;
		}
		curl_global_cleanup();
	}
	free(chunk.memory);
	return NULL;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S T A R T  U P D A T E  T I D E  I N F O                                                                          *
 *  ========================================                                                                          *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Create a thread to read the tide times.
 *  \result None.
 */
void startUpdateTideInfo()
{
	if (tideState != TIDE_STATE_PENDING)
	{
		if (pthread_create (&threadHandle, NULL, getTideTimes, NULL) == 0)
		{
			tideState = TIDE_STATE_PENDING;
		}
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  R E A D  T I D E  I N I T                                                                                         *
 *  =========================                                                                                         *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Init function if needed, not used.
 *  \result None.
 */
void readTideInit (void)
{
	if (gaugeEnabled[FACE_TYPE_TIDE].enabled)
	{
		gaugeMenuDesc[MENU_GAUGE_TIDE].disable = 0;
		memset (&tideInfo, 0, sizeof (tideInfo));
		strcpy (tideInfo.location, "Unknown");
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  R E A D  T I D E  V A L U E S                                                                                     *
 *  =============================                                                                                     *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Calculate the state of the tide at the current location.
 *  \param face Face to display on.
 *  \result Values save to the face.
 */
void readTideValues (int face)
{
	if (gaugeEnabled[FACE_TYPE_TIDE].enabled)
	{
		struct tm *tideTime;
		time_t nextTideTime, lastTideTime;
		char tideDirStr[41], tideHeightStr[41], tideTimeStr[41], toolTip[1024];
		FACE_SETTINGS *faceSetting = faceSettings[face];
		int i, nextTide, loopStart, loopEnd;
		long duration;

		if (faceSetting -> faceFlags & FACE_REDRAW)
		{
			;
		}
		else if (sysUpdateID % 10 != 0 && myUpdateID != -1)
		{
			return;
		}

		if (myUpdateID != sysUpdateID)
		{
			time_t now = time (NULL);

			if (tideInfo.readTime < now || myUpdateID == -1)
			{
				startUpdateTideInfo();
			}
			myUpdateID = sysUpdateID;
		}
		nextTide = getNextTide();
		if (nextTide)
		{
			lastTideTime = tideInfo.tideTimes[nextTide - 1].tideTime;
			nextTideTime = tideInfo.tideTimes[nextTide].tideTime;
		}
		else
		{
			nextTideTime = tideInfo.tideTimes[nextTide].tideTime;
			lastTideTime = nextTideTime - tideDuration;
		}
		tideTime = localtime (&nextTideTime);
		duration = nextTideTime - lastTideTime;

		faceSetting -> firstValue = (double)(time (NULL) - lastTideTime) / duration;
		if (tideInfo.tideTimes[nextTide].tideType == 'L')
		{
			faceSetting -> firstValue = (double)1 - faceSetting -> firstValue;
		}
		faceSetting -> firstValue *= 3.141592654;
		faceSetting -> firstValue = cos (faceSetting -> firstValue);
		faceSetting -> firstValue *= -50;
		faceSetting -> firstValue += 50;

		strcpy (tideDirStr, tideInfo.tideTimes[nextTide].tideType == 'H' ? _("High") : _("Low"));
		sprintf (tideTimeStr, "%d:%02d", tideTime -> tm_hour, tideTime -> tm_min);
		sprintf (tideHeightStr, "%0.1fm", tideInfo.tideTimes[nextTide].tideHeight);

		setFaceString (faceSetting, FACESTR_TOP, 22, "%s", tideInfo.location);
		setFaceString (faceSetting, FACESTR_BOT, 0, _("%s %s\n%s"), tideDirStr, tideTimeStr, tideHeightStr);
		setFaceString (faceSetting, FACESTR_WIN, 0, _("Tide %s: %0.1f%% Gauge"),
				tideInfo.tideTimes[nextTide].tideType == 'H' ? _("coming in") : _("going out"),
				faceSetting -> firstValue);
		sprintf (toolTip, "<b>Port</b>: %s, %s\n<b>Tide Level</b>: %s %0.1f%%",
				tideInfo.location, tideInfo.country,
				tideInfo.tideTimes[nextTide].tideType == 'H' ? _("Coming in") : _("Going out"),
				faceSetting -> firstValue);

		loopStart = nextTide ? nextTide - 1 : nextTide;
		loopEnd = loopStart + 4;
		for (i = loopStart; i < loopEnd; ++i)
		{
			if (tideInfo.tideTimes[i].tideSet)
			{
				tideTime = localtime (&tideInfo.tideTimes[i].tideTime);
				sprintf (tideTimeStr, "%s. %d %s, %d:%02d", days[tideTime -> tm_wday], tideTime -> tm_mday,
						months[tideTime -> tm_mon], tideTime -> tm_hour, tideTime -> tm_min);
				strcpy (tideDirStr, tideInfo.tideTimes[i].tideType == 'H' ? _("High water") : _("Low water"));
				sprintf (tideHeightStr, "%0.1fm", tideInfo.tideTimes[i].tideHeight);
				sprintf (&toolTip[strlen(toolTip)], _("\n<b>%s</b>: %s, %s"), tideDirStr, tideTimeStr, tideHeightStr);
			}
		}
		setFaceString (faceSetting, FACESTR_TIP, 0, toolTip);
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  T I D E  S E T T I N G S                                                                                          *
 *  ========================                                                                                          *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Settings for the tide gauge.
 *  \param data Passed from the menu.
 *  \result None.
 */
void tideSettings (guint data)
{
	GtkWidget *dialog;
	GtkWidget *vbox;
	GtkWidget *label;
	GtkWidget *entryPort, *entryKey;
	const char *savePort, *saveKey;
	char portCode[21];
	int portNum = 0, len;
	GtkWidget *contentArea;
	GtkWidget *grid;

	len = strlen (tideURL);
	if (len > 4)
		portNum = atoi (&tideURL[len - 4]);
	if (portNum == 0)
		strcpy (portCode, "0113");
	else
		sprintf (portCode, "%04d", portNum);

	dialog = gtk_dialog_new_with_buttons ("Tide Settings", GTK_WINDOW(dialConfig.mainWindow),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			_("_OK"), GTK_RESPONSE_ACCEPT, _("_Cancel"), GTK_RESPONSE_REJECT, NULL);

	contentArea = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 3);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 0);
	gtk_box_pack_start (GTK_BOX (contentArea), vbox, TRUE, TRUE, 0);
	grid = gtk_grid_new ();

	label = gtk_label_new (_("Prediction from: "));
	gtk_widget_set_halign (label, GTK_ALIGN_START);
	gtk_grid_attach (GTK_GRID (grid), label, 1, 1, 1, 1);
	label = gtk_label_new (_("Prediction from: https://easytide.admiralty.co.uk/"));
	gtk_widget_set_halign (label, GTK_ALIGN_START);
	gtk_grid_attach (GTK_GRID (grid), label, 2, 1, 1, 1);

	label = gtk_label_new (_("Location port number: "));
	gtk_widget_set_halign (label, GTK_ALIGN_START);
	gtk_grid_attach (GTK_GRID (grid), label, 1, 2, 1, 1);
	entryPort = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY (entryPort), 4);
	gtk_entry_set_input_purpose (GTK_ENTRY (entryPort), GTK_INPUT_PURPOSE_DIGITS);
	gtk_entry_set_text (GTK_ENTRY (entryPort), portCode);
	gtk_grid_attach (GTK_GRID (grid), entryPort, 2, 2, 1, 1);
	
	label = gtk_label_new (_("Admiralty API Key: "));
	gtk_widget_set_halign (label, GTK_ALIGN_START);
	gtk_grid_attach (GTK_GRID (grid), label, 1, 3, 1, 1);
	entryKey = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY (entryKey), 32);
	gtk_entry_set_input_purpose (GTK_ENTRY (entryKey), GTK_INPUT_PURPOSE_FREE_FORM);
	gtk_entry_set_text (GTK_ENTRY (entryKey), tideAPIKey);
	gtk_grid_attach (GTK_GRID (grid), entryKey, 2, 3, 1, 1);	
	
	gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 0);
	gtk_widget_show_all (dialog);
	
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
	{
		int saved = 0;
		savePort = gtk_entry_get_text(GTK_ENTRY (entryPort));
		saveKey = gtk_entry_get_text(GTK_ENTRY (entryKey));
		
		if (strcmp (savePort, portCode) != 0)
		{
			sprintf (tideURL, urlPrefix, atoi (savePort));
			configSetValue ("tide_info_url", tideURL);
			saved = 1;
		}
		if (strcmp (saveKey, tideAPIKey) != 0)
		{
			strcpy (tideAPIKey, saveKey);
			configSetValue ("tide_api_key", tideAPIKey);
			saved = 1;
		}
		if (saved)
		{
			tideInfo.locRead = 0;
			startUpdateTideInfo ();
			myUpdateID = -1;
		}
	}
	gtk_widget_destroy (dialog);
}


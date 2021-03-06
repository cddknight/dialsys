/**********************************************************************************************************************
 *                                                                                                                    *
 *  G A U G E  T I D E . C                                                                                            *
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
 *  \brief Display a gauge showing the current state of the tide.
 */
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <curl/curl.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/HTMLparser.h>

#include "GaugeDisp.h"

extern FACE_SETTINGS *faceSettings[];
extern GAUGE_ENABLED gaugeEnabled[];
extern MENU_DESC gaugeMenuDesc[];
extern DIAL_CONFIG dialConfig;
extern int sysUpdateID;
extern char tideURL[];

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
	double locationOffset;
	struct TideTime tideTimes[MAX_SAVE_TIDES];
	time_t readTime;
};

static struct TideInfo tideInfo;
static char tideReadLine[1025];
static int lastReadTide;

static int myUpdateID = -1;
static time_t tideDuration = 22358;
static char removePrefix[] = "Port predictions (Standard Local Time) are ";
static char *days[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
static char *months[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
static char *urlPrefix = "http://www.ukho.gov.uk/easytide/easytide/ShowPrediction.aspx?PredictionLength=4&PortID=";

/**********************************************************************************************************************
 *                                                                                                                    *
 *  G E T  M O N T H                                                                                                  *
 *  ================                                                                                                  *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Convert a month string into a number.
 *  \param mon Month to find.
 *  \result Number of the month.
 */
static int getMonth (char *mon)
{
	int i;
	for (i = 0; i < 12; ++i)
		if (strncmp (mon, months[i], 3) == 0)
			return i + 1;
	return 0;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  G E T  D A Y                                                                                                      *
 *  ============                                                                                                      *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Convert a simple Sun, Mon format to a number 1, 2.
 *  \param day Day string to convert.
 *  \result 1 for Sun 7 for Sat, 0 if not found.
 */
static int getDay (char *day)
{
	int i;
	for (i = 0; i < 7; ++i)
		if (strncmp (day, days[i], 3) == 0)
			return i + 1;
	return 0;
}

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
		memset (&tideInfo, 0, sizeof (tideInfo));
		strcpy (tideInfo.location, "Pending");
		tideInfo.tideTimes[0].tideTime = tideInfo.readTime = time (NULL);
		tideInfo.tideTimes[0].tideType = 'L';
		retn = 0;
	}
	return retn;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  P R O C E S S  P R O C E S S  A T I D E                                                                           *
 *  =======================================                                                                           *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Process a tide read from HTML.
 *  \param mDay Month day.
 *  \param mon Month.
 *  \param type H or L.
 *  \param hour Hour (local time).
 *  \param min Min (local time).
 *  \param height Height of the tide.
 *  \result None.
 */
static void processProcessATide(int mDay, int mon, char type, int hour, int min, double height)
{
	struct tm tideTime, *timeNow;
	time_t now = time(NULL);

/*  printf ("Tide[%d]: %d/%d %c %d:%02d %f\n", lastReadTide, mDay, mon, type, hour, min, height); */
	timeNow = gmtime (&now);
	memcpy (&tideTime, timeNow, sizeof (tideTime));
	tideTime.tm_mday = mDay;
	tideTime.tm_mon = mon - 1;
	tideTime.tm_hour = hour;
	tideTime.tm_min = min;
	tideTime.tm_sec = 0;
	tideTime.tm_isdst = 0;

	if (lastReadTide < MAX_SAVE_TIDES)
	{
		tideInfo.tideTimes[lastReadTide].tideTime = mktime(&tideTime);
		tideInfo.tideTimes[lastReadTide].tideTime -= (tideInfo.locationOffset * 3600);
		tideInfo.tideTimes[lastReadTide].tideHeight = height;
		tideInfo.tideTimes[lastReadTide].tideType = type;
		tideInfo.tideTimes[lastReadTide].tideSet = 1;
		++lastReadTide;
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  P R O C E S S  C U R R E N T  D A Y                                                                               *
 *  ===================================                                                                               *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Process all the values read for a day.
 *  \result None.
 */
static void processCurrentDay()
{
	char words[41][11];
	int i = 0, j = 0, w = 0, tideCount = 0;

	memset (&words, 0, sizeof (words));
	do
	{
		switch (tideReadLine[i])
		{
		case ' ':
		case ';':
		case ':':
			if (j > 0 && w < 40)
			{
				if (!strcmp (words[w], "HW") || !strcmp (words[w], "LW"))
				{
					++tideCount;
				}
				++w;
				j = 0;
			}
			break;

		default:
			if (j < 10)
			{
				char ch = tideReadLine[i];
				if ((ch >= 'A' && ch <='Z') || (ch >= 'a' && ch <='z') || (ch >= '0' && ch <='9') ||
						ch == '.' || ch == '-')
				{
					words[w][j] = tideReadLine[i];
					words[w][++j] = 0;
				}
			}
			break;
		}
		++i;
	}
	while (tideReadLine[i]);

	if (w == 3 + (tideCount * 4))
	{
		int i;
		for (i = 0; i < tideCount; ++i)
		{
			processProcessATide (atoi (words[1]), getMonth(words[2]), words[3 + i][0],
					atoi ((const char *)&words[3 + tideCount + (i * 2)]),
					atoi ((const char *)&words[4 + tideCount + (i * 2)]),
					atof ((const char *)&words[3 + (tideCount * 3) + i]));
		}
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
writeMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data)
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
 *  P R O C E S S  E L E M E N T  N A M E S                                                                           *
 *  =======================================                                                                           *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Call by libxml for each of the elements.
 *  \param doc xmldoc handle.
 *  \param aNode Xml node pointer.
 *  \param curPath The path to the current element.
 *  \param readLevel How many levels of html are we at.
 *  \result None.
 */
static void
processElementNames (xmlDoc *doc, xmlNode * aNode, char *curPath, int readLevel)
{
	xmlChar *key;
	xmlNode *curNode = NULL;
	char fullPath[1024];
	char *matchPath[3] =
	{
		"/html/body/div/form/div/div/ul/li/span",
		"/html/body/div/form/div/div/div/div/table/tr",
		"/html/body/div/form/div/div/p/span"
	};

	for (curNode = aNode; curNode; curNode = curNode->next)
	{
		int saveLevel = readLevel;

		if (curNode -> name != NULL)
		{
			if ((strlen (curPath) + strlen ((char *)curNode -> name)) > 1022)
			{
				fprintf (stderr, "XML path too long to display\n");
				break;
			}
			strcpy (fullPath, curPath);
			strcat (fullPath, "/");
			strcat (fullPath, (char *)curNode -> name);
		}

		if (curNode->type == XML_ELEMENT_NODE)
		{
			++readLevel;
			if (!strncmp (fullPath, matchPath[0], strlen (matchPath[0])))
			{
				key = xmlNodeListGetString (doc, curNode -> xmlChildrenNode, 1);
				if (key && !strncmp ((char *)(curNode -> name), "span", 4))
				{
					if (tideInfo.location[0] == 0)
					{
						strncpy (tideInfo.location, (char *)key, 40);
						properCaseWord (tideInfo.location);
					}
					else if (tideInfo.country[0] == 0)
					{
						strncpy (tideInfo.country, (char *)key, 40);
						properCaseWord (tideInfo.country);
					}
				}
			}
			if (!strncmp (fullPath, matchPath[1], strlen (matchPath[1])))
			{
				key = xmlNodeListGetString (doc, curNode -> xmlChildrenNode, 1);
				if (key && readLevel == 11)
				{
					if (getDay ((char *)key))
					{
						if (tideReadLine[0])
						{
							processCurrentDay();
							tideReadLine[0] = 0;
						}
					}
					strncat (tideReadLine, (char *)key, 1024);
					strncat (tideReadLine, ";", 1024);
				}
			}
			if (!strncmp (fullPath, matchPath[2], strlen (matchPath[2])))
			{
				key = xmlNodeListGetString (doc, curNode -> xmlChildrenNode, 1);
				if (key)
				{
					if (!strncmp (removePrefix, (char *)key, strlen (removePrefix)))
					{
						tideInfo.locationOffset = atof ((char *)&key[strlen (removePrefix)]);
					}
				}
			}
		}
		processElementNames (doc, curNode->children, fullPath, readLevel);
		readLevel = saveLevel;
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
	htmlDocPtr hDoc = NULL;
	htmlNodePtr rootElement = NULL;
	xmlChar *xmlBuffer = NULL;

	xmlBuffer = xmlCharStrndup (buffer, size);
	if (xmlBuffer != NULL)
	{
		if ((hDoc = htmlParseDoc(xmlBuffer, NULL)) != NULL)
		{
			if ((rootElement = xmlDocGetRootElement (hDoc)) != NULL)
			{
				lastReadTide = 0;
				tideReadLine[0] = 0;
				tideInfo.location[0] = 0;
				processElementNames (hDoc, rootElement, "", 0);
				processCurrentDay();
				if (lastReadTide)
				{
					tideInfo.readTime = getLocalNextMidday();
				}
			}
			xmlFreeDoc(hDoc);
		}
		else
		{
			printf ("error: could not parse memory\n");
			printf ("BUFF[%lu] [[[%s]]]\n", size, buffer);
		}
		xmlFree (xmlBuffer);
	}
	xmlCleanupParser();
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
	struct MemoryStruct chunk;

	chunk.memory = malloc(1);
	chunk.size = 0;

	curl_global_init(CURL_GLOBAL_ALL);
	curlHandle = curl_easy_init();
	curl_easy_setopt(curlHandle, CURLOPT_URL, &tideURL[0]);
	curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, writeMemoryCallback);
	curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, (void *)&chunk);
	curl_easy_setopt(curlHandle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
	curl_easy_perform(curlHandle);
	curl_easy_cleanup(curlHandle);

	if (chunk.memory)
	{
		if (chunk.size)
		{
			memset (&tideInfo, 0, sizeof (tideInfo));
			processBuffer (chunk.memory, chunk.size);
		}
		free(chunk.memory);
	}
	curl_global_cleanup();
	tideState = TIDE_STATE_UPDATED;
	return NULL;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S T A R T  U P D A T E  T I D E  I N F O                                                                          *
 *  ========================================                                                                          *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Create a thread to read the tide times..
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
	GtkWidget *entry;
	const char *saveText;
	char portCode[21];
#if GTK_MAJOR_VERSION == 2
	GtkWidget *hbox;
#else
	GtkWidget *contentArea;
	GtkWidget *grid;
#endif

	if (strlen(tideURL) > strlen(urlPrefix))
	{
		sprintf (portCode, "%d", atoi (&tideURL[strlen(urlPrefix)]));
	}
	else
	{
		strcpy (portCode, "113");
	}

#if GTK_MAJOR_VERSION == 2

	dialog = gtk_dialog_new_with_buttons ("Tide Settings", GTK_WINDOW(dialConfig.mainWindow),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL);

	vbox = GTK_DIALOG (dialog)->vbox;
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 3);

	label = gtk_label_new (_("Prediction from: http://www.ukho.gov.uk/easytide/"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

	hbox = gtk_hbox_new (FALSE, 3);
	label = gtk_label_new ("Location port number: ");
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	entry = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY (entry), 4);
	gtk_entry_set_text (GTK_ENTRY (entry), portCode);
	gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

#else

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
	label = gtk_label_new (_("http://www.ukho.gov.uk/easytide/"));
	gtk_widget_set_halign (label, GTK_ALIGN_START);
	gtk_grid_attach (GTK_GRID (grid), label, 2, 1, 1, 1);

	label = gtk_label_new (_("Location port number: "));
	gtk_widget_set_halign (label, GTK_ALIGN_START);
	gtk_grid_attach (GTK_GRID (grid), label, 1, 2, 1, 1);
	entry = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY (entry), 4);
	gtk_entry_set_input_purpose (GTK_ENTRY (entry), GTK_INPUT_PURPOSE_DIGITS);
	gtk_entry_set_text (GTK_ENTRY (entry), portCode);
	gtk_grid_attach (GTK_GRID (grid), entry, 2, 2, 1, 1);
	gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 0);

#endif

	gtk_widget_show_all (dialog);
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
	{
		saveText = gtk_entry_get_text(GTK_ENTRY (entry));
		if (strcmp (saveText, portCode) != 0)
		{
			strcpy (tideURL, urlPrefix);
			sprintf (&tideURL[strlen(urlPrefix)], "%04d", atoi (saveText));
			configSetValue ("tide_info_url", tideURL);
			startUpdateTideInfo ();
			myUpdateID = -1;
		}
	}
	gtk_widget_destroy (dialog);
}


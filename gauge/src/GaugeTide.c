/**********************************************************************************************************************
 *                                                                                                                    *
 *  G A U G E  T I D E . C                                                                                            *
 *  ======================                                                                                            *
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
 *  @brief Display a gauge showing the current state of the tide.
 *  @version $Id: GaugeTide.c 1856 2014-01-14 15:10:08Z ukchkn $
 */
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <curl/curl.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
 
#include "GaugeDisp.h"

#ifdef GAUGE_HAS_TIDE

extern FACE_SETTINGS *faceSettings[];
extern MENU_DESC gaugeMenuDesc[];
extern int sysUpdateID;
extern time_t hightideTime;
extern GtkWindow *mainWindow;

struct MemoryStruct 
{
  char *memory;
  size_t size;
};

struct TideTime
{
	time_t tideTime;
	int tideHeight;
	char tideType;
	char estimate;
};

struct TideInfo
{
	char location[41];
	time_t baseTime, readTime;
	struct TideTime tideTimes[2];
};

static struct TideInfo tideInfo =
{
	"Unknown"
};

extern char tideURL[];
static char *lineSep = "<br/>";
static int myUpdateID = 100;
static time_t tideDuration = 22357;
static char removePrefix[] = "Tide Times & Heights for ";

/**********************************************************************************************************************
 *                                                                                                                    *
 *  P R O C E S S  T I D E  T I M E                                                                                   *
 *  ===============================                                                                                   *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Extract parts from: (01:23 - Low Tide (1.
 *  \param line Line read from page.
 *  \result 1 if all OK.
 */
int processTideTime (char *line)
{
	char words[5][41];	
	int word = 0, i = 0, j = 0;

	while (line[i] && word < 5)
	{
		if (line[i] <= ' ' || line[i] == ':' || line[i] == '(' || line[i] == ')' || line[i] == '-')
		{
			if (j)
				words[++word][j = 0] = 0;
		}
		else
		{
			words[word][j] = line[i];
			words[word][++j] = 0;
		}
		++i;
	}

	for (i = 0; i <= word; ++i)
	{
		if (strcmp (words[3], "Tide") == 0)
		{
			tideInfo.tideTimes[1].tideTime = tideInfo.baseTime + (atoi (words[0]) * 3600) + (atoi (words[1]) * 60);
			tideInfo.tideTimes[1].tideHeight = (int)((atof (words[4]) * 10) + 0.5);
			tideInfo.tideTimes[1].tideType = words[2][0];
			if (tideInfo.tideTimes[1].tideTime > time(NULL))
				return 1;

			tideInfo.tideTimes[0] = tideInfo.tideTimes[1];
			tideInfo.tideTimes[1].tideTime = 0;
		}
	}
	return 0;
}

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
int getMonth (char *mon)
{
	int i;
	static char *months[12] = 
			{ "Jan", "Feb", "Mar", "Apr", "May", "Jun", 
			  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	for (i = 0; i < 12; ++i)
		if (strncmp (mon, months[i], 3) == 0)
			return i;
	return 0;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  P R O C E S S  L O C A T I O N  D A T E                                                                           *
 *  =======================================                                                                           *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Process the line: (Grovehurst Jetty on 8th July 2011).
 *  \param line Line read from the page.
 *  \result None.
 */
void processLocationDate (char *line)
{
	char words[5][41];
	int word = 0, i = 0, j = 0;
	struct tm tideTime, *timeNow;
	time_t now = time(NULL);

	memset (&tideTime, 0, sizeof (tideTime));
	while (line[i] && word < 5)
	{
		if (line[i] <= ' ')
		{
			if (j)
			{
				if (word == 1)
				{
					if (strcmp (words[1], "on") != 0)
					{
						strcat (words[0], " ");
						strcat (words[0], words[1]);
						--word;
					}
				}
				words[++word][j = 0] = 0;
			}
		}
		else
		{
			words[word][j] = line[i];
			words[word][++j] = 0;
		}
		++i;
	}

	timeNow = localtime (&now);
	memcpy (&tideTime, timeNow, sizeof (tideTime));
	strcpy (tideInfo.location, words[0]);
	tideTime.tm_mday = atoi (words[2]);
	tideTime.tm_mon = getMonth (words[3]);
	tideTime.tm_year = atoi (words[4]) - 1900;
	tideTime.tm_hour = 0;
	tideTime.tm_min = 0;
	tideTime.tm_sec = 1;
	tideInfo.baseTime = mktime(&tideTime);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  P R O C E S S  D E S C R I P T I O N                                                                              *
 *  ====================================                                                                              *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Process a descripsion breaking in to parts at <br>.
 *  \param description Description read fron the page.
 *  \result Number of parts found.
 */
int processDescription (xmlChar *description)
{
	char lines[6][81];
	int outLine = 0, i = 0, j = 0, k = 0, level = 0;

	memset (&lines[0][0], 0, 6 * 81);
	
	while (description[i] && outLine < 5)
	{
		if (description[i] == '<')
			++level;
		if (description[i] == '>' && level)
			--level;
		if (description[i] != lineSep[k])
		{
			k = 0;
		}
		if (description[i] == lineSep[k])
		{
			if (lineSep[++k] == 0)
			{
				if (j)
				{
					++outLine;
					k = 0;
					j = 0;
				}
				lines[outLine][0] = 0;
			}
		}
		else if (j < 100 && level == 0 && description[i] != '>')
		{
			lines[outLine][j] = description[i];
			lines[outLine][++j] = 0;
			if (j == strlen (removePrefix))
			{
				if (strcmp (lines[outLine], removePrefix) == 0)
					lines[outLine][j = 0] = 0;
			}
		}
		++i;
	}
	if (outLine >= 3)
	{
		processLocationDate (lines[0]);
		for (i = 1; i < outLine; ++i)
		{
			if (processTideTime (lines[i]))
				break;
		}
	}
	return outLine;
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
 *  \result None.
 */
static void
processElementNames (xmlDoc *doc, xmlNode * aNode)
{
	xmlChar *key;
    xmlNode *cur_node = NULL;

    for (cur_node = aNode; cur_node; cur_node = cur_node->next) 
    {
        if (cur_node->type == XML_ELEMENT_NODE) 
        {
			if ((!xmlStrcmp (cur_node -> name, (const xmlChar *)"description"))) 
			{
				key = xmlNodeListGetString (doc, cur_node -> xmlChildrenNode, 1);
				processDescription (key);
		    	xmlFree (key);
			}
        }
        processElementNames (doc, cur_node->children);
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
			processElementNames (doc, rootElement);
			xmlFreeDoc (doc);
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
 *  \result None.
 */
void getTideTimes () 
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

	tideInfo.tideTimes[0].tideTime = tideInfo.tideTimes[1].tideTime = 0;
	tideInfo.tideTimes[0].estimate = tideInfo.tideTimes[1].estimate = 0;
	
	processBuffer (chunk.memory, chunk.size);
	
	if (!tideInfo.tideTimes[0].tideTime)
	{
		if (tideInfo.tideTimes[1].tideTime)
		{
			tideInfo.tideTimes[0].tideTime = tideInfo.tideTimes[1].tideTime - tideDuration;
			tideInfo.tideTimes[0].tideType = (tideInfo.tideTimes[1].tideType == 'L' ? 'H' : 'L');
		}
		else
		{
			time_t now = time (NULL);
			tideInfo.tideTimes[0].tideTime = hightideTime;
			tideInfo.tideTimes[0].tideType = 'H';
			while (tideInfo.tideTimes[0].tideTime + tideDuration < now)
			{
				tideInfo.tideTimes[0].tideTime += tideDuration;
				tideInfo.tideTimes[0].tideType = (tideInfo.tideTimes[0].tideType == 'L' ? 'H' : 'L');
			}
		}
		tideInfo.tideTimes[0].estimate = 1;
	}
/*	printf ("Last Tide: %c, Type: %s, At %s", 
			tideInfo.tideTimes[0].tideType, 
			tideInfo.tideTimes[0].estimate ? "Estimate" : "Known",
			ctime (&tideInfo.tideTimes[0].tideTime)); */
	
	if (!tideInfo.tideTimes[1].tideTime)
	{
		tideInfo.tideTimes[1].tideTime = tideInfo.tideTimes[0].tideTime + tideDuration;
		tideInfo.tideTimes[1].tideType = (tideInfo.tideTimes[0].tideType == 'L' ? 'H' : 'L');
		tideInfo.tideTimes[1].estimate = 1;
		tideInfo.readTime = time (NULL) + (60 * 30);
	}
	else
		tideInfo.readTime = tideInfo.tideTimes[1].tideTime + 5;

/*	printf ("Next Tide: %c, Type: %s, At %s", 
			tideInfo.tideTimes[1].tideType, 
			tideInfo.tideTimes[1].estimate ? "Estimate" : "Known",
			ctime (&tideInfo.tideTimes[1].tideTime)); */

	if(chunk.memory)
		free(chunk.memory);

	curl_global_cleanup();
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
	gaugeMenuDesc[MENU_GAUGE_TIDE].disable = 0;
	tideInfo.tideTimes[0].tideTime = tideInfo.tideTimes[1].tideTime = 0;
	tideInfo.tideTimes[0].estimate = tideInfo.tideTimes[1].estimate = 1;
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
	struct tm *tideTime;
	time_t nextTideTime, lastTideTime;
	char tideDirStr[8], tideHeightStr[8], tideTimeStr[8];
	long duration;

	FACE_SETTINGS *faceSetting = faceSettings[face];

	if (faceSetting -> faceFlags & FACE_REDRAW)
	{
		;
	}
	else if (sysUpdateID % 60 != 0)
	{
		return;
	}
	if (myUpdateID != sysUpdateID)
	{
		time_t now = time (NULL);
		
		if (tideInfo.readTime < now || tideInfo.tideTimes[1].tideTime < now)
			getTideTimes();
		myUpdateID = sysUpdateID;
	}

	lastTideTime = tideInfo.tideTimes[0].tideTime;
	nextTideTime = tideInfo.tideTimes[1].tideTime;
	tideTime = localtime (&nextTideTime);
	duration = nextTideTime - lastTideTime;
	
	faceSetting -> firstValue = (double)(time (NULL) - lastTideTime) / duration;
	if (tideInfo.tideTimes[1].tideType == 'L')
		faceSetting -> firstValue = (double)1 - faceSetting -> firstValue;
	faceSetting -> firstValue *= 3.141592654;
	faceSetting -> firstValue = cos (faceSetting -> firstValue);
	faceSetting -> firstValue *= -50;
	faceSetting -> firstValue += 50;

	strcpy (tideDirStr, tideInfo.tideTimes[1].tideType == 'H' ? _("High") : _("Low"));
	sprintf (tideTimeStr, "%d:%02d", tideTime -> tm_hour, tideTime -> tm_min);
	sprintf (tideHeightStr, "%d.%01d", tideInfo.tideTimes[1].tideHeight / 10, 
			tideInfo.tideTimes[1].tideHeight % 10);

	setFaceString (faceSetting, FACESTR_TOP, 22, "%s", tideInfo.location);
	if (tideInfo.tideTimes[1].estimate)
	{
		setFaceString (faceSetting, FACESTR_BOT, 0, _("%s %s\n(est.)"), tideDirStr, tideTimeStr);
		setFaceString (faceSetting, FACESTR_TIP, 0, _("<b>Next</b>: %s tide\n<b>Time</b>: %s (estimate)"), 
				tideDirStr, tideTimeStr);
	}
	else
	{
		setFaceString (faceSetting, FACESTR_BOT, 0, _("%s %s\n%sm"), tideDirStr, tideTimeStr, tideHeightStr);
		setFaceString (faceSetting, FACESTR_TIP, 0, _("<b>Next</b>: %s tide\n<b>Time</b>: %s\n<b>Height</b>: %sm"),
				tideDirStr, tideTimeStr, tideHeightStr);
	}
	setFaceString (faceSetting, FACESTR_WIN, 0, _("Tide %s: %0.1f%% Gauge"), 
			tideInfo.tideTimes[1].tideType == 'H' ? _("coming in") : _("going out"),
			faceSetting -> firstValue);
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
	char textUpdate = 0;
#if GTK_MAJOR_VERSION == 2
	GtkWidget *hbox;
#else
	GtkWidget *contentArea;
	GtkWidget *grid;
#endif

	dialog = gtk_dialog_new_with_buttons ("Tide Settings", GTK_WINDOW(mainWindow),
						GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, 
#if GTK_MAJOR_VERSION == 3 && GTK_MINOR_VERSION >= 10
						_("Close"), 
#else
						GTK_STOCK_CLOSE, 
#endif
						GTK_RESPONSE_NONE, NULL);

#if GTK_MAJOR_VERSION == 2

	vbox = GTK_DIALOG (dialog)->vbox;
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 3);

	label = gtk_label_new (_("RSS feed from: http://www.tidetimes.org.uk/"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

	hbox = gtk_hbox_new (FALSE, 3);
	label = gtk_label_new ("Location feed: ");
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	entry = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY (entry), 128);
	gtk_entry_set_text (GTK_ENTRY (entry), tideURL);
	gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

#else

	contentArea = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 3);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 3);
	gtk_box_pack_start (GTK_BOX (contentArea), vbox, TRUE, TRUE, 0);
	grid = gtk_grid_new ();
	
	label = gtk_label_new (_("RSS feed from: "));
	gtk_widget_set_halign (label, GTK_ALIGN_START);
	gtk_grid_attach (GTK_GRID (grid), label, 1, 1, 1, 1);
	label = gtk_label_new (_("http://www.tidetimes.org.uk/"));
	gtk_widget_set_halign (label, GTK_ALIGN_START);
	gtk_grid_attach (GTK_GRID (grid), label, 2, 1, 1, 1);

	label = gtk_label_new (_("Location feed: "));
	gtk_widget_set_halign (label, GTK_ALIGN_START);
	gtk_grid_attach (GTK_GRID (grid), label, 1, 2, 1, 1);
	entry = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY (entry), 128);
	gtk_entry_set_text (GTK_ENTRY (entry), tideURL);
	gtk_grid_attach (GTK_GRID (grid), entry, 2, 2, 1, 1);

	gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 0);

#endif

	gtk_widget_show_all (dialog);
	gtk_dialog_run (GTK_DIALOG (dialog));
	
	saveText = gtk_entry_get_text(GTK_ENTRY (entry));
	if (strcmp (saveText, tideURL) != 0)
	{
		strncpy (tideURL, saveText, 128);
		configSetValue ("tide_info_url", tideURL);
		textUpdate = 1;
	}
	if (textUpdate)
	{
		getTideTimes ();
		myUpdateID = -1;
	}
	gtk_widget_destroy (dialog);
}

#endif



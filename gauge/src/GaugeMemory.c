/**********************************************************************************************************************
 *                                                                                                                    *
 *  G A U G E  M E M O R Y . C                                                                                        *
 *  ==========================                                                                                        *
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
 *  \brief Handle a gauge that shows memory.
 */
#include <stdio.h>
#include <string.h>
#include "GaugeDisp.h"

extern FACE_SETTINGS *faceSettings[];
extern GAUGE_ENABLED gaugeEnabled[];
extern MENU_DESC gaugeMenuDesc[];
extern int sysUpdateID;

int readMemInfo (void);

#define MAX_MEMINFO 6
static int myUpdateID = 100;
static unsigned long memValues[MAX_MEMINFO];

static char *infoName[MAX_MEMINFO] =
{
	"MemTotal:",	/*  0   */
	"MemFree:",		/*  1   */
	"Buffers:",		/*  2   */
	"Cached:",		/*  3   */
	"SwapTotal:",	/*  4   */
	"SwapFree:"		/*  5   */
};

static char *name[] =
{
	__("Programs"),		/*  0   */
	__("Free"),			/*  1   */
	__("Buffers"),		/*  2   */
	__("Cached"),		/*  3   */
	__("Swap")			/*  4   */
};

/**********************************************************************************************************************
 *                                                                                                                    *
 *  R E A D  M E M O R Y  I N I T                                                                                     *
 *  =============================                                                                                     *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Called on program start to init the gauge.
 *  \result None.
 */
void readMemoryInit (void)
{
	if (gaugeEnabled[FACE_TYPE_MEMORY].enabled)
	{
		gaugeMenuDesc[MENU_GAUGE_MEMORY].disable = 0;
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  R E A D  M E M O R Y  V A L U E S                                                                                 *
 *  =================================                                                                                 *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Read the state of the memory.
 *  \param face Which face is this for.
 *  \result None.
 */
void readMemoryValues (int face)
{
	if (gaugeEnabled[FACE_TYPE_MEMORY].enabled)
	{
		FACE_SETTINGS *faceSetting = faceSettings[face];
		int faceType = faceSetting -> faceSubType;
		float value = 0, totalMem = 0;
		unsigned long total;

		if (faceSetting -> faceFlags & FACE_REDRAW)
		{
			;
		}
		else if (sysUpdateID % 15 != 0)
		{
			return;
		}
		if (myUpdateID != sysUpdateID)
		{
			readMemInfo ();
			myUpdateID = sysUpdateID;
		}

		faceSetting -> firstValue = 0;
		faceSetting -> secondValue = 0;

		if (faceType == 4)
		{
			if ((total = memValues[4]) == 0)
				value = totalMem = 0;
			else
			{
				value = total - memValues[5];

				faceSetting -> firstValue = (value * 100) / total;
				totalMem = (double)total / 1024000;
				value /= 1024000;
			}
		}
		else if (memValues[0])
		{
			total = memValues[0];
			totalMem = (double)total / 1024000;
			if (faceType == 0)			/* Used by applications */
			{
				value = total - (memValues[1] + memValues[2] + memValues[3]);
			}
			else
			{
				value = memValues[faceType];
			}
			if (total)
			{
				faceSetting -> firstValue = (value * 100) / total;
				faceSetting -> secondValue = ((total - memValues[1]) * 100) / total;
			}
			value /= 1024000;
		}

		setFaceString (faceSetting, FACESTR_TOP, 0, _("Memory\n(%s)"), gettext (name[faceType]));
		if (totalMem > 10.0)
		{
			setFaceString (faceSetting, FACESTR_BOT, 0, _("%0.1f%%\n%0.0fGB"), faceSetting -> firstValue, totalMem);
		}
		else
		{
			setFaceString (faceSetting, FACESTR_BOT, 0, _("%0.1f%%\n%0.1fGB"), faceSetting -> firstValue, totalMem);
		}
		setFaceString (faceSetting, FACESTR_TIP, 0, _("<b>%s</b>: %0.1f%% (%0.1fGB)\n<b>Total Size</b>: %0.1fGB"),
				gettext (name[faceType]), faceSetting -> firstValue, value, totalMem);
		setFaceString (faceSetting, FACESTR_WIN, 0, _("Memory %s: %0.1f%% - Gauge"), gettext (name[faceType]),
				faceSetting -> firstValue);
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  R E A D  M E M  I N F O                                                                                           *
 *  =======================                                                                                           *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Read from proc the state of the memory.
 *  \result Number of values read.
 */
int readMemInfo ()
{
	int i, j, k, n, found = 0;
	char readBuff[1025], word[254];
	FILE *inFile = fopen ("/proc/meminfo", "r");

	while (inFile != NULL && found < MAX_MEMINFO)
	{
		word[i = j = n = 0] = 0;
		if (!fgets (readBuff, 1024, inFile))
			break;

		k = -1;
		while (n < 2)
		{
			if (readBuff[i] == ' ' || readBuff[i] == 0)
			{
				if (word[0])
				{
					if (n == 0)
					{
						int p;
						for (p = 0; p < MAX_MEMINFO; ++p)
							if (strcmp (infoName[p], word) == 0)
								k = p;
					}
					else if (n == 1 && k != -1)
					{
						memValues[k] = atol (word);
						++found;
					}
					word[j = 0] = 0;
					n ++;
				}
				if (readBuff[i] == 0)
					break;
			}
			else
			{
				word[j++] = readBuff[i];
				word[j] = 0;
			}
			i ++;
		}
	}
	if (inFile != NULL)
	{
		fclose (inFile);
	}
	return found;
}


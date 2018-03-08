/**********************************************************************************************************************
 *                                                                                                                    *
 *  G A U G E  W I F I . C                                                                                            *
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
 *  \brief Calculate the wifi quality for the gauge.
 */
#include <stdio.h>
#include <string.h>
#include "GaugeDisp.h"

extern FACE_SETTINGS *faceSettings[];
extern GAUGE_ENABLED gaugeEnabled[];
extern MENU_DESC gaugeMenuDesc[];
extern int sysUpdateID;

static int myUpdateID = 100;
static double lastRead = -1;
static char *findQualityStr = "Link Quality=";
static char *findSignalStr = "Signal level=";

/**********************************************************************************************************************
 *                                                                                                                    *
 *  H U N T  F O R  Q U A L I T Y                                                                                     *
 *  =============================                                                                                     *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Search for a matching string in a string.
 *  \param inStr String to search in.
 *  \param findStr String to look for.
 *  \result -1 not found.
 */
int huntForQuality (char *inStr, char *findStr)
{
    int matched = 0, len = strlen (findStr), i = 0;

    while (inStr[i] != 0 && matched < len)
    {
		int retry;
		do
		{
			retry = 0;
			if (inStr[i] == findStr[matched])
    	    {
    	        ++matched;
    	    }
    	    else if (matched)
    	    {
				matched = 0;
				retry = 1;
    	    }
		}
		while (retry);
		++i;
    }
    return (matched == len ? i - len : -1);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  R E A D  L I N K  Q U A L I T Y                                                                                   *
 *  ===============================                                                                                   *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Read the link quality by running iwconfig.
 *  \param qualityValue Link quality value.
 *  \param signalValue Signal level value.
 *  \result 1 if a value was read, 0 if not.
 */
int readLinkQuality(double *qualityValue, double *signalValue)
{
	int retn = 0;
	char readBuff[1025];
	FILE *inFile = popen("iwconfig 2>> /dev/null", "r");

	if (inFile != NULL)
	{
		while (fgets (readBuff, 1024, inFile))
		{
			int found = huntForQuality (readBuff, findQualityStr);
			if (found != -1)
			{
				int val1, val2;
				if (sscanf (&readBuff[found + strlen (findQualityStr)], "%d/%d", &val1, val2) == 2)
				{
					if (qualityValue != NULL)
					{
						*qualityValue = ((double)val1 / (double)val2) * 100;
					}
					retn = 1;
				}
			}
			found = huntForQuality (readBuff, findSignalStr);
			if (found != -1)
			{
				int val1;
				if (sscanf (&readBuff[found + strlen (findSignalStr)], "%d", &val1) == 2)
				{
					if (signalValue != NULL)
					{
						*signalValue = (double)val1;
					}
				}
			}
		}
		fclose (inFile);
	}
	return retn;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  R E A D  W I F I  I N I T                                                                                         *
 *  =========================                                                                                         *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Unlock the menu if we can read the wifi quality.
 *  \result None.
 */
void readWifiInit (void)
{
	if (gaugeEnabled[FACE_TYPE_WIFI].enabled)
	{
		if (readLinkQuality(NULL, NULL))
		{
			gaugeMenuDesc[MENU_GAUGE_WIFI].disable = 0;
		}
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  R E A D  W I F I  V A L U E S                                                                                     *
 *  =============================                                                                                     *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Read the value and save it onto the dial.
 *  \param face None.
 *  \result None.
 */
void readWifiValues (int face)
{
	int update = 0;

	if (gaugeEnabled[FACE_TYPE_MOONPHASE].enabled)
	{
		double linkQuality = 0, signalLevel = 0;
		FACE_SETTINGS *faceSetting = faceSettings[face];

		if (faceSetting -> faceFlags & FACE_REDRAW)
		{
			faceSetting -> nextUpdate = 5;
			update = 1;
		}
		if (!update && sysUpdateID % 2 != 0)
		{
			return;
		}
		readLinkQuality (&linkQuality, &signalLevel);
		setFaceString (faceSetting, FACESTR_TOP, 0, _("Wifi\nQuality"));
		setFaceString (faceSetting, FACESTR_WIN, 0, _("Wifi - Gauge"));
		setFaceString (faceSetting, FACESTR_BOT, 0, _("%0.1f%%"), linkQuality);
		setFaceString (faceSetting, FACESTR_TIP, 0, _("<b>Wifi Quality</b>: %0.1f%%\n<b>Signal Level</b>:%0.0f dBm"),  
				linkQuality, signalLevel);
		faceSetting -> firstValue = linkQuality;
	}
}


/**********************************************************************************************************************
 *                                                                                                                    *
 *  G A U G E  W I F I . C                                                                                            *
 *  ======================                                                                                            *
 *                                                                                                                    *
 *  Copyright (c) 2023 Chris Knight                                                                                   *
 *                                                                                                                    *
 *  File GaugeWifi.c part of Gauge is free software: you can redistribute it and/or modify it under the terms of the  *
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
 *  \brief Calculate the wifi quality for the gauge.
 */
#include <stdio.h>
#include <string.h>
#include "GaugeDisp.h"

extern FACE_SETTINGS *faceSettings[];
extern GAUGE_ENABLED gaugeEnabled[];
extern MENU_DESC gaugeMenuDesc[];
extern int sysUpdateID;

static char *findQualityStr = "Link Quality=";
static char *findSignalStr = "Signal level=";
static char *findBitRateStr = "Bit Rate=";

struct sReadInfo
{
	double quality;
	double level;
	double rate;

	char levelType[11];
	char rateType[11];
};

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
 *  \param readInfo Pointer to buffer or read info.
 *  \result 1 if a value was read, 0 if not.
 */
int readLinkQuality(struct sReadInfo *readInfo)
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
				if (sscanf (&readBuff[found + strlen (findQualityStr)], "%d/%d", &val1, &val2) == 2)
				{
					readInfo -> quality = ((double)val1 / (double)val2) * 100;
					retn = 1;
				}
			}
			found = huntForQuality (readBuff, findSignalStr);
			if (found != -1)
			{
				sscanf (&readBuff[found + strlen (findSignalStr)], "%lf %8s", &readInfo -> level, readInfo -> levelType);
			}
			found = huntForQuality (readBuff, findBitRateStr);
			if (found != -1)
			{
				sscanf (&readBuff[found + strlen (findBitRateStr)], "%lf %8s", &readInfo -> rate, readInfo -> rateType);
			}
		}
		pclose (inFile);
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
		struct sReadInfo readInfo;
		if (readLinkQuality (&readInfo))
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
		struct sReadInfo readInfo;
		FACE_SETTINGS *faceSetting = faceSettings[face];

		if (faceSetting -> faceFlags & FACE_REDRAW)
		{
			faceSetting -> nextUpdate = 5;
			update = 1;
		}
		if (!update && sysUpdateID % 10 != 0)
		{
			return;
		}
		readLinkQuality (&readInfo);
		setFaceString (faceSetting, FACESTR_TOP, 0, _("Wifi\nQuality"));
		setFaceString (faceSetting, FACESTR_WIN, 0, _("Wifi - Gauge"));
		setFaceString (faceSetting, FACESTR_BOT, 0, _("%0.1f%%"), readInfo.quality);
		setFaceString (faceSetting, FACESTR_TIP, 0, _("<b>Wifi Quality</b>: %0.1f%%\n"
				"<b>Signal Level</b>: %0.0f %s\n"
				"<b>Bit Rate</b>: %0.0f %s"),
				readInfo.quality, readInfo.level, readInfo.levelType, readInfo.rate, readInfo.rateType);
		faceSetting -> firstValue = readInfo.quality;
	}
}


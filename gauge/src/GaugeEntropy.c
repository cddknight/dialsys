/**********************************************************************************************************************
 *                                                                                                                    *
 *  G A U G E  E N T R O P Y . C                                                                                      *
 *  ============================                                                                                      *
 *                                                                                                                    *
 *  Copyright (c) 2023 Chris Knight                                                                                   *
 *                                                                                                                    *
 *  File GaugeEntropy.c part of Gauge is free software: you can redistribute it and/or modify it under the terms of   *
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
 *  \brief Handle a gauge that shows entropy.
 */
#include <stdio.h>
#include <string.h>
#include <dirent.h>

#include "GaugeDisp.h"

extern FACE_SETTINGS *faceSettings[];
extern GAUGE_ENABLED gaugeEnabled[];
extern MENU_DESC gaugeMenuDesc[];
extern int sysUpdateID;

static int readEntropyFile (char *fileName, int defValue);
/* static char *entropyPoolFile = "/proc/sys/kernel/random/poolsize"; */
static char *entropyAvailFile = "/proc/sys/kernel/random/entropy_avail";
static int myUpdateID = 100;
static int myPoolsize = 4096;
static int myEntropyAvail = 0;

/**********************************************************************************************************************
 *                                                                                                                    *
 *  R E A D  E N T R O P Y  I N I T                                                                                   *
 *  ===============================                                                                                   *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Initialise the gauge.
 *  \result None.
 */
void readEntropyInit (void)
{
	if (gaugeEnabled[FACE_TYPE_ENTROPY].enabled)
	{
		gaugeMenuDesc[MENU_GAUGE_ENTROPY].disable = 0;
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  R E A D  E N T R O P Y  V A L U E S                                                                               *
 *  ===================================                                                                               *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Read the current entropy value.
 *  \param face Which face is this for.
 *  \result None.
 */
void readEntropyValues (int face)
{
	if (gaugeEnabled[FACE_TYPE_ENTROPY].enabled)
	{
		FACE_SETTINGS *faceSetting = faceSettings[face];

		if (faceSetting -> faceFlags & FACE_REDRAW)
		{
			;
		}
		else if (sysUpdateID % 5 != 0)
		{
			return;
		}
		if (myUpdateID != sysUpdateID)
		{
			myEntropyAvail = readEntropyFile (entropyAvailFile, myEntropyAvail);
			myUpdateID = sysUpdateID;
		}
		faceSetting -> firstValue = myEntropyAvail * 100;
		faceSetting -> firstValue /= myPoolsize;
		setFaceString (faceSetting, FACESTR_TOP, 0, _("Random\nEntropy"));
		setFaceString (faceSetting, FACESTR_TIP, 0, _("<b>Entropy</b>: %0.1f%% Full"), faceSetting -> firstValue);
		setFaceString (faceSetting, FACESTR_WIN, 0, _("Random Entropy: %0.1f%% Full - Gauge"), faceSetting -> firstValue);
		setFaceString (faceSetting, FACESTR_BOT, 0, _("%0.1f%%"), faceSetting -> firstValue);
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  R E A D  E N T R O P Y  F I L E                                                                                   *
 *  ===============================                                                                                   *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Read a value from a file.
 *  \param filename Name of the file to read .
 *  \param defValue Default value if the file could not be read.
 *  \result Value read from the file.
 */
static int readEntropyFile (char *filename, int defValue)
{
	int valRead = defValue;
	FILE *inFile = fopen (filename, "r");

	if (inFile != NULL)
	{
		char readBuff[81];
		if (fgets (readBuff, 80, inFile))
			valRead = atoi (readBuff);
		fclose (inFile);
	}
	return valRead;
}


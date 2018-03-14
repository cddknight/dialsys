/**********************************************************************************************************************
 *                                                                                                                    *
 *  G A U G E  B A T T E R Y . C                                                                                      *
 *  ============================                                                                                      *
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
 *  \brief Handle a gauge that shows battery.
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>

#include "GaugeDisp.h"

extern FACE_SETTINGS *faceSettings[];
extern GAUGE_ENABLED gaugeEnabled[];
extern MENU_DESC gaugeMenuDesc[];
extern int sysUpdateID;

static int readBatteryDir ();
static int readBattery (char *filePath);
static char *batteryRoot = "/sys/class/power_supply/";

/**********************************************************************************************************************
 *                                                                                                                    *
 **********************************************************************************************************************/
static char *matchStrings[] =
{
	"POWER_SUPPLY_STATUS=",
	"POWER_SUPPLY_VOLTAGE_MIN_DESIGN=",
	"POWER_SUPPLY_VOLTAGE_NOW=",
	"POWER_SUPPLY_CURRENT_NOW=",
	"POWER_SUPPLY_CHARGE_FULL_DESIGN=",
	"POWER_SUPPLY_CHARGE_FULL=",
	"POWER_SUPPLY_CHARGE_NOW=",
	NULL
};

typedef struct
{
	int readBat;
	char status[41];
	int voltMinDesign;
	int voltNow;
	int currentNow;
	int chargeDesign;
	int chargeFull;
	int chargeNow;
}
BAT_STATE;

BAT_STATE currentState;
static int myUpdateID = 100;

/**********************************************************************************************************************
 *                                                                                                                    *
 *  R E A D  B A T T E R Y  I N I T                                                                                   *
 *  ===============================                                                                                   *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Called on program start to init the gauge.
 *  \result None.
 */
void readBatteryInit (void)
{
	if (gaugeEnabled[FACE_TYPE_BATTERY].enabled)
	{
		if (readBatteryDir ())
			gaugeMenuDesc[MENU_GAUGE_BATTERY].disable = 0;
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  R E A D  B A T T E R Y  V A L U E S                                                                               *
 *  ===================================                                                                               *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Read the state of the battery.
 *  \param face Which face is this for.
 *  \result None.
 */
void readBatteryValues (int face)
{
	if (gaugeEnabled[FACE_TYPE_BATTERY].enabled)
	{
		FACE_SETTINGS *faceSetting = faceSettings[face];

		if (faceSetting -> faceFlags & FACE_REDRAW)
		{
			;
		}
		else if (sysUpdateID % 25 != 0)
		{
			return;
		}
		if (myUpdateID != sysUpdateID)
		{
			readBatteryDir ();
			myUpdateID = sysUpdateID;
		}
		if (currentState.readBat && currentState.chargeFull)
		{
			faceSetting -> firstValue = currentState.chargeNow * 100;
			faceSetting -> firstValue /= currentState.chargeFull;
			setFaceString (faceSetting, FACESTR_TOP, 0, _("Battery\n%s"), currentState.status);
			setFaceString (faceSetting, FACESTR_BOT, 0, _("%0.0f%%"), faceSetting -> firstValue);
			setFaceString (faceSetting, FACESTR_TIP, 0, _("<b>Status</b>: %s\n"
					"<b>Charge Now</b>: %d mAh\n<b>Charge Full</b>: %d mAh\n<b>Charge Design</b>: %d mAh"), 
					currentState.status, currentState.chargeNow / 1000, 
					currentState.chargeFull / 1000, currentState.chargeDesign / 1000);
			setFaceString (faceSetting, FACESTR_WIN, 0, _("Battery: %0.0f%% Full - Gauge"), 
					faceSetting -> firstValue);
		}	
		else
		{
			faceSetting -> firstValue = 0;
			setFaceString (faceSetting, FACESTR_TOP, 0, _("Battery\n(Missing)"));
			setFaceString (faceSetting, FACESTR_TIP, 0, _("<b>Battery</b>: Not Installed"));
			setFaceString (faceSetting, FACESTR_WIN, 0, _("Battery: Not Installed - Gauge"));
			setFaceString (faceSetting, FACESTR_BOT, 0, _("0%%"));
		}
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  R E A D  B A T T E R Y  D I R                                                                                     *
 *  =============================                                                                                     *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Search battery directory for batteries.
 *  \result Number of lines read, it should be 3.
 */
static int readBatteryDir ()
{
	char fullPath[256];
	struct dirent *dirEntry;
	DIR *dir = opendir(batteryRoot);
	
	currentState.readBat = 0;
	if (dir)
	{
		while ((dirEntry = readdir(dir)) != NULL)
		{
			if (strncmp (dirEntry -> d_name, "BAT", 3) == 0)
			{
				strcpy (fullPath, batteryRoot);
				strcat (fullPath, dirEntry -> d_name);
				strcat (fullPath, "/");
				if (readBattery (fullPath))
				{
					break;
				}
			}
		}
		closedir (dir);
	}
	return currentState.readBat;
}


/**********************************************************************************************************************
 *                                                                                                                    *
 *  C O P Y  N O  C T R L                                                                                             *
 *  =====================                                                                                             *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Copy a string without control characters.
 *  \param outStr Output the string here.
 *  \param inStr Input the string from here.
 *  \param maxSize Max size of the output string.
 *  \result Pointer to the output string.
 */
char *copyNoCtrl (char *outStr, char *inStr, int maxSize)
{
	int i = 0, j = 0;

	while (inStr[i] && j < maxSize)
	{
		if (inStr[i] >= ' ')
		{
			outStr[j++] = inStr[i];
			outStr[j] = 0;
		}
		++i;
	}
	return outStr;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  R E A D  B A T T E R Y                                                                                            *
 *  ======================                                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Read from proc the state of the battery.
 *  \param filePath Path of the battery state directory.
 *  \result Number of values read.
 */
static int readBattery (char *filePath)
{
	FILE *inFile;
	char fullName[256];

	strcpy (fullName, filePath);
	strcat (fullName, "uevent");
	if ((inFile = fopen (fullName, "r")) != NULL)
	{
		while (fgets (fullName, 255, inFile))
		{
			int i = 0;
			while (matchStrings[i] != NULL)
			{
				if (strncmp (matchStrings[i], fullName, strlen (matchStrings[i])) == 0)
				{
					switch (i)
					{
					case 0:
						copyNoCtrl (currentState.status, &fullName[strlen (matchStrings[i])], 40);
						break;
					case 1:
						currentState.voltMinDesign = atoi (&fullName[strlen (matchStrings[i])]);
						break;
					case 2:
						currentState.voltNow = atoi (&fullName[strlen (matchStrings[i])]);
						break;
					case 3:
						currentState.currentNow = atoi (&fullName[strlen (matchStrings[i])]);
						break;
					case 4:
						currentState.chargeDesign = atoi (&fullName[strlen (matchStrings[i])]);
						break;
					case 5:
						currentState.chargeFull = atoi (&fullName[strlen (matchStrings[i])]);
						break;
					case 6:
						currentState.chargeNow = atoi (&fullName[strlen (matchStrings[i])]);
						currentState.readBat = 1;
						break;
					}
					break;
				}
				++i;
			}
		}
		fclose (inFile);
	}
	return currentState.readBat;
}


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

typedef struct
{
	int readBat;
	int chargeDesign;
	int chargeNow;
	int chargeFull;
	char status[41];
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
			setFaceString (faceSetting, FACESTR_TIP, 0, _("<b>Battery</b>: %0.1f%% Full, %s"), 
					faceSetting -> firstValue, currentState.status);
			setFaceString (faceSetting, FACESTR_WIN, 0, _("Battery: %0.1f%% Full - Gauge"), 
					faceSetting -> firstValue);
			setFaceString (faceSetting, FACESTR_BOT, 0, _("%0.1f%%"), 
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
	int readBat = 0;
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
					readBat = 1;
					break;
				}
			}
		}
		closedir (dir);
	}
	return readBat;
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
	int chargeDesign = -1, chargeNow = -1, chargeFull = -1;
	char fullName[256], status[41];

	strcpy (fullName, filePath);
	strcat (fullName, "charge_full_design");
	if ((inFile = fopen (fullName, "r")) != NULL)
	{
		if (fscanf (inFile, "%d", &chargeDesign) == 1)
		{
			fclose (inFile);
			strcpy (fullName, filePath);
			strcat (fullName, "charge_now");
			if ((inFile = fopen (fullName, "r")) != NULL)
			{
				if (fscanf (inFile, "%d", &chargeNow) == 1)
				{
					fclose (inFile);
					strcpy (fullName, filePath);
					strcat (fullName, "charge_full");
					if ((inFile = fopen (fullName, "r")) != NULL)
					{
						if (fscanf (inFile, "%d", &chargeFull) == 1)
						{
							fclose (inFile);
							strcpy (fullName, filePath);
							strcat (fullName, "status");
							status[0] = 0;
							if ((inFile = fopen (fullName, "r")) != NULL)
							{
								int size = fread (currentState.status, 1, 40, inFile);
								if (size)
								{
									if (currentState.status[size - 1] == '\n')
									{
										--size;
									}
									currentState.status[size] = 0;
									currentState.readBat = 1;
									currentState.chargeDesign = chargeDesign;
									currentState.chargeNow = chargeNow;
									currentState.chargeFull = chargeFull;
									fclose (inFile);
									inFile = NULL;
								}
							}
						}
					}
				}
			}
		}
		if (inFile != NULL)
		{
			printf ("errno: %d\n", errno);
			fclose (inFile);
		}
	}
	return currentState.readBat;
}


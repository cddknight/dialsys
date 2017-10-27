/**********************************************************************************************************************
 *                                                                                                                    *
 *  G A U G E  B A T T E R Y . C                                                                                      *
 *  ============================                                                                                      *
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
 *  @brief Handle a gauge that shows battery.
 *  @version $Id: GaugeBattery.c 1856 2014-01-14 15:10:08Z ukchkn $
 */
#include <stdio.h>
#include <string.h>
#include <dirent.h>

#include "GaugeDisp.h"

extern FACE_SETTINGS *faceSettings[];
extern GAUGE_ENABLED gaugeEnabled[];
extern MENU_DESC gaugeMenuDesc[];
extern int sysUpdateID;

static int readBatteryDir ();
static int readBattery (char *fileName);
static int valsRead = 0;
static char batteryValues[3][21];
static char *batteryRoot = "/proc/acpi/battery/";
static char *name[3] = 
{
	"chargingstate:",				/*	charged		*/
	"designcapacity:",				/*	3221 mAh	*/
	"remainingcapacity:",			/*	3221 mAh	*/
};

static char *batState[6] =
{
	"charged",		__("Charged"),
	"charging",		__("Charging"),
	"discharging",	__("Active")
};

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
		readBatteryDir ();
		if (valsRead == 3)
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
		if (valsRead == 3)
		{
			int i;
			char *state = batteryValues[0];
		
			for (i = 0; i < 6; i += 2)
			{
				if (strcmp (state, batState[i]) == 0)
				{
					state = batState[i + 1];
					break;
				}
			}
			faceSetting -> firstValue = atoi (batteryValues[2]) * 100;
			faceSetting -> firstValue /= atoi (batteryValues[1]);
			setFaceString (faceSetting, FACESTR_TOP, 0, _("Battery\n(%s)"), gettext (state));
			setFaceString (faceSetting, FACESTR_TIP, 0, _("<b>Battery</b>: %0.1f%% Full, %s"), 
					faceSetting -> firstValue, gettext (state));
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
	char fullPath[80];
	struct dirent *dirEntry;
	DIR *dir = opendir(batteryRoot);
	
	valsRead = 0;
	if (dir)
	{
		while ((dirEntry = readdir(dir)) != NULL && valsRead != 3)
		{
			if (dirEntry -> d_name[0] != '.')
			{
				int i;
				
				strcpy (fullPath, batteryRoot);
				strcat (fullPath, dirEntry -> d_name);
				strcat (fullPath, "/");
				i = strlen (fullPath);
				strcat (fullPath, "info");
				if ((valsRead = readBattery (fullPath)) != 0)
				{
					strcpy (&fullPath[i], "state");
					valsRead += readBattery (fullPath);
				}				
			}
		}
		closedir (dir);
	}
	return valsRead;
}		

/**********************************************************************************************************************
 *                                                                                                                    *
 *  R E A D  B A T T E R Y                                                                                            *
 *  ======================                                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Read from proc the state of the battery.
 *  \param fileName Which files to read.
 *  \result Number of values read.
 */
static int readBattery (char *fileName)
{
	int i, j, k, n, p, found = 0;
	char readBuff[1025], word[254];
	FILE *inFile = fopen (fileName, "r");

	while (inFile != NULL && found < 4)
	{
		word[i = j = n = 0] = 0;
		if (!fgets (readBuff, 1024, inFile))
			break;

		k = -1;
		while (n < 2)
		{
			if (readBuff[i] == ':' || readBuff[i] == 0)
			{
				if (word[0])
				{
					if (n == 0)
					{
						word[j] = ':';
						word[++j] = 0;
						
						for (p = 0; p < 3; ++p)
							if (strcmp (name[p], word) == 0)
								k = p;
					}
					else if (n == 1 && k != -1)
					{
						strncpy (batteryValues[k], word, 20);
						++found;
					}
					word[j = 0] = 0;
					n ++;
				}
				if (readBuff[i] == 0)
					break;
			}
			else if (readBuff[i] > ' ')
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


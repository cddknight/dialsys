/**********************************************************************************************************************
 *                                                                                                                    *
 *  G A U G E  N E T W O R K . C                                                                                      *
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
 *  \brief Functions to display a graph of network speed.
 */
#include <stdio.h>
#include <string.h>
#include <dirent.h>

#include "GaugeDisp.h"

#define MAX_DEVICES		10
#define MAX_SCALE_MEM	20

extern FACE_SETTINGS *faceSettings[];
extern GAUGE_ENABLED gaugeEnabled[];
extern MENU_DESC gaugeMenuDesc[];
extern MENU_DESC networkDevDesc[];
extern int sysUpdateID;

struct devValues
{
	float rate;
	unsigned long long value;
	int useScale;
	int oldScales[MAX_SCALE_MEM];
};

typedef struct _devInfo
{
	char name[41];
	struct devValues dataRead;
	struct devValues dataWrite;
}
DEVICE_INFO;

static int myUpdateID = 100;
static long lastTime;
static char *deviceStats = "/proc/net/dev";
static char *typeNames[] = { "Rx", "Tx" };
DEVICE_INFO deviceActivity[MAX_DEVICES + 1] =
{
	{ "All" }
};

static char *speedName[] =
{
	"B/s",			//  0
	"10B/s",		//  1
	"100B/s",		//  2
	"KB/s",			//  3
	"10KB/s",		//  4
	"100KB/s",		//  5
	"MB/s",			//  6
	"10MB/s",		//  7
	"100MB/s",		//  8
	"GB/s",			//  9
	"10GB/s",		//  10
	"100GB/s",		//  11
	"TB/s",			//  12
	"10TB/s",		//  13
	"100TB/s",		//  14
};

/**********************************************************************************************************************
 *                                                                                                                    *
 *  G E T  S C A L E  S H I F T  C O U N T                                                                            *
 *  ======================================                                                                            *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Work out how many shifts have been done.
 *  \param scale scale to work on.
 *  \result Number of shifts in the scale.
 */
int getScaleShiftCount (int scale)
{
	int retn = 0;
	while (scale >= 10)
	{
		scale /= 10;
		++retn;
	}
	return retn;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S E T  D E V I C E  S C A L E                                                                                     *
 *  =============================                                                                                     *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Calculate the best scale for this gauge.
 *  \param values Values to scale.
 *  \result None (saved in structure).
 */
void setDeviceScale (struct devValues *values, int lockScale)
{
	int scale = 1, i;
	while ((values -> rate / scale) > 100)
		scale *= 10;

	if (!lockScale || scale > values -> useScale)
	{
		values -> useScale = scale;
		for (i = 0; i < (MAX_SCALE_MEM - 1); ++i)
		{
			values -> oldScales[i] = values -> oldScales[i + 1];
			if (values -> oldScales[i] > values -> useScale)
				values -> useScale = values -> oldScales[i];
		}
		values -> oldScales[i] = scale;
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  R E A D  D E V I C E  V A L U E S                                                                                 *
 *  =================================                                                                                 *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Read the number of sectors read.
 *  \result None.
 */
void readDeviceValues(int lockScale)
{
	FILE *devstats;
	struct timeval tvTaken;
	char readBuff[1025], readWord[256];
	long thisTime = 0, readTime;
	int device = 1;

	gettimeofday (&tvTaken, NULL);
	thisTime = ((tvTaken.tv_sec % 100000) * 1000) + (tvTaken.tv_usec / 1000);
	readTime = thisTime - lastTime;
	lastTime = thisTime;
	if (!readTime) return;

	deviceActivity[0].dataRead.value = deviceActivity[0].dataRead.rate = 0;
	deviceActivity[0].dataWrite.value = deviceActivity[0].dataWrite.rate = 0;

	if ((devstats = fopen (deviceStats, "r")) != NULL)
	{
		while (fgets (readBuff, 1024, devstats) && device < (MAX_DEVICES + 1))
		{
			int i = 0, j = 0, w = 0;

			if (strncmp (readBuff, "Inter", 5) == 0 || strncmp (readBuff, " face", 5) == 0)
				continue;

			while (readBuff[i])
			{
				if (readBuff[0] == '#' && w == 0)
					break;
				if (readBuff[i] > ' ' && readBuff[i] != ':')
				{
					if (j == 0)
					{
						if (++w == 11)
							break;
					}
					readWord[j] = readBuff[i];
					readWord[++j] = 0;
				}
				else if (j)
				{
					if (w == 1)
					{
						strncpy (deviceActivity[device].name, readWord, 40);
					}
					if (w == 2)
					{
						unsigned long long diff;
						unsigned long long value = atoll (readWord);
						if (value < deviceActivity[device].dataRead.value)
						{
							deviceActivity[device].dataRead.value = value;
							break;
						}
						diff = value - deviceActivity[device].dataRead.value;
						deviceActivity[device].dataRead.rate = (diff * 1000) / readTime;
						deviceActivity[device].dataRead.value = value;
						deviceActivity[0].dataRead.value += diff;
					}
					if (w == 10)
					{
						unsigned long long diff;
						unsigned long long value = atoll (readWord);
						if (value < deviceActivity[device].dataWrite.value)
						{
							deviceActivity[device].dataWrite.value = value;
							break;
						}
						diff = value - deviceActivity[device].dataWrite.value;
						deviceActivity[device].dataWrite.rate = (diff * 1000) / readTime;
						deviceActivity[device].dataWrite.value = value;
						deviceActivity[0].dataWrite.value += diff;

						setDeviceScale (&deviceActivity[device].dataRead, lockScale);
						setDeviceScale (&deviceActivity[device].dataWrite, lockScale);

						networkDevDesc[device].disable = 0;
						networkDevDesc[device].menuName = deviceActivity[device].name;
						gaugeMenuDesc[MENU_GAUGE_NETWORK].disable = 0;
						++device;
					}
					j = 0;
				}
				++i;
			}
		}
		fclose (devstats);
	}
	deviceActivity[0].dataRead.rate = (deviceActivity[0].dataRead.value * 1000) / readTime;
	deviceActivity[0].dataWrite.rate = (deviceActivity[0].dataWrite.value * 1000) / readTime;
	setDeviceScale (&deviceActivity[0].dataRead, lockScale);
	setDeviceScale (&deviceActivity[0].dataWrite, lockScale);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  R E A D  N E T W O R K  I N I T                                                                                   *
 *  ===============================                                                                                   *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Call at prog start, read initial values.
 *  \result None.
 */
void readNetworkInit (void)
{
	if (gaugeEnabled[FACE_TYPE_NETWORK].enabled)
	{
		readDeviceValues(0);
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  R E A D  N E T W O R K  V A L U E S                                                                               *
 *  ===================================                                                                               *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Read values for the face.
 *  \param face Save them on the face.
 *  \result None.
 */
void readNetworkValues (int face)
{
	if (gaugeEnabled[FACE_TYPE_NETWORK].enabled)
	{
		FACE_SETTINGS *faceSetting = faceSettings[face];
		int device = faceSetting -> faceSubType & 0x000F, scale;
		int lockScale = (faceSetting -> faceSubType >> 4) & 1;
		int faceType = (faceSetting -> faceSubType >> 8) & 1;
		unsigned long value = 0;
		unsigned short shift;
		char *nameT, *nameD;

		if (faceSetting -> faceFlags & FACE_REDRAW)
		{
			;
		}
		else if (sysUpdateID % 10 != 0)
		{
			return;
		}
		if (myUpdateID != sysUpdateID)
		{
			readDeviceValues(lockScale);
			myUpdateID = sysUpdateID;
		}

		nameD = deviceActivity[device].name;
		if (faceType)
		{
			nameT = typeNames[0];
			scale = deviceActivity[device].dataRead.useScale;
			value = faceSetting -> firstValue = deviceActivity[device].dataRead.rate;
		}
		else
		{
			nameT = typeNames[1];
			scale = deviceActivity[device].dataWrite.useScale;
			value = faceSetting -> firstValue = deviceActivity[device].dataWrite.rate;
		}
		faceSetting -> firstValue /= scale;
		shift = getScaleShiftCount(scale);
		setFaceString (faceSetting, FACESTR_TOP, 0, _("Network\n%s (%s)"), nameT, nameD);
		setFaceString (faceSetting, FACESTR_TIP, 0, _("<b>Network %s</b>: %lu B/s (%s)"), nameT, value, nameD);
		setFaceString (faceSetting, FACESTR_BOT, 0, _("%s\n(%0.1f)"), speedName[shift], faceSetting -> firstValue);
		setFaceString (faceSetting, FACESTR_WIN, 0, _("Network %s - Gauge"), nameT);

		if (faceSetting -> updateNum != shift)
		{
			maxMinReset (&faceSetting -> savedMaxMin, 10, 2);
			faceSetting -> faceFlags |= FACE_REDRAW;
			faceSetting -> updateNum = shift;
		}
	}
}


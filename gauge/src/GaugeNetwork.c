/**********************************************************************************************************************
 *                                                                                                                    *
 *  G A U G E  N E T W O R K . C                                                                                      *
 *  ============================                                                                                      *
 *                                                                                                                    *
 *  Copyright (c) 2023 Chris Knight                                                                                   *
 *                                                                                                                    *
 *  File GaugeNetwork.c part of Gauge is free software: you can redistribute it and/or modify it under the terms of   *
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
	int useScale;
	unsigned long long value;
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

typedef struct _speedInfo
{
	unsigned long kiloBitsPerSec;
	char *speedName;
}
SPEED_INFO;

SPEED_INFO speedInfo[] =
{
	{	100,		"100 kbps"	},	// 0
	{	1000,		"1 Mbps",	},	// 1
	{	10000,		"10 Mbps"	},	// 2
	{	100000,		"100 Mbps"	},	// 3
	{	1000000,	"1 Gbps"	},	// 4
	{	2500000,	"2.5 Gbps"	},	// 5
	{	5000000,	"5 Gbps"	},	// 6
	{	10000000,	"10 Gbps"	},	// 7
	{	100000000,	"100 Gbps"	},	// 8
	{	1000000000,	"1 Tbps"	},	// 9
	{	0, 			NULL		},	// 10
};
	
/**********************************************************************************************************************
 *                                                                                                                    *
 *  S E T  D E V I C E  S C A L E                                                                                     *
 *  =============================                                                                                     *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Calculate the best scale for this gauge.
 *  \param values Values to scale.
 *  \param lockScale Alter the scale after it is changed.
 *  \result None (saved in structure).
 */
void setDeviceScale (struct devValues *values)
{
	unsigned long kr = (values -> rate * 8) / 1024;
	int scale = 0, i;
	while (kr > speedInfo[scale].kiloBitsPerSec)
	{
		if (speedInfo[scale + 1].kiloBitsPerSec == 0)
			break;
		++scale;
	}

	if (scale > values -> useScale)
	{
/*		printf ("Switch to scale: %d, Up to: %d \n", scale, speedInfo[scale].kiloBitsPerSec, values -> rate); */
		values -> useScale = scale;
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
 *  \param lockScale Alter the scale after it is changed.
 *  \result None.
 */
void readDeviceValues()
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
						readWord[40] = 0;
						strcpy (deviceActivity[device].name, readWord);
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

						setDeviceScale (&deviceActivity[device].dataRead);
						setDeviceScale (&deviceActivity[device].dataWrite);

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
	setDeviceScale (&deviceActivity[0].dataRead);
	setDeviceScale (&deviceActivity[0].dataWrite);
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
			readDeviceValues();
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
		
		faceSetting -> firstValue *= 8;
		faceSetting -> firstValue /= 1024;
		faceSetting -> firstValue /= speedInfo[scale].kiloBitsPerSec;
		faceSetting -> firstValue *= 100;
		
/*		printf ("%d: Face value: %f, Scale: %d, %d, Bytes: %d\n", faceType, faceSetting -> firstValue, scale, speedInfo[scale].kiloBitsPerSec, value); */
		if (faceSetting -> firstValue > 100)
		{
			faceSetting -> firstValue = 100;
		}
		
		setFaceString (faceSetting, FACESTR_TOP, 0, _("Network\n%s (%s)"), nameT, nameD);
		setFaceString (faceSetting, FACESTR_TIP, 0, _("<b>Network %s</b>: %lu B/s (%s)"), nameT, value, nameD);
		setFaceString (faceSetting, FACESTR_BOT, 0, _("%s\n(%0.1f)"), speedInfo[scale].speedName, faceSetting -> firstValue);
		setFaceString (faceSetting, FACESTR_WIN, 0, _("Network %s - Gauge"), nameT);

		if (faceSetting -> updateNum != scale)
		{
			maxMinReset (&faceSetting -> savedMaxMin, 10, 2);
			faceSetting -> faceFlags |= FACE_REDRAW;
			faceSetting -> updateNum = scale;
		}
	}
}


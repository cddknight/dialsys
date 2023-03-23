/**********************************************************************************************************************
 *                                                                                                                    *
 *  G A U G E  H A R D D I S K . C                                                                                    *
 *  ==============================                                                                                    *
 *                                                                                                                    *
 *  Copyright (c) 2023 Chris Knight                                                                                   *
 *                                                                                                                    *
 *  File GaugeHarddisk.c part of Gauge is free software: you can redistribute it and/or modify it under the terms of  *
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
 *  \brief Create gauges for hard disk info.
 */
#include <stdio.h>
#include <string.h>
#include <sys/statvfs.h>
#include <dialsys.h>

#include "GaugeDisp.h"

#define MAX_PARTITIONS	20
#define MAX_DISKS		20
#define MAX_SCALE_MEM	20

extern FACE_SETTINGS *faceSettings[];
extern GAUGE_ENABLED gaugeEnabled[];
extern MENU_DESC gaugeMenuDesc[];
extern MENU_DESC spaceMenuDesc[];
extern MENU_DESC diskMenuDesc[];
extern int sysUpdateID;

struct diskValues
{
	float rate;
	unsigned long long value;
	int useScale;
	int oldScales[MAX_SCALE_MEM];
};

typedef struct _diskInfo
{
	char name[41];
	struct diskValues secRead;
	struct diskValues secWrite;
}
DISK_INFO;

typedef struct _partInfo
{
	char diskName[81];
	char tidyName[41];
}
PARTITION_INFO;

static int myUpdateID = 100;
static long lastTime;
static char *diskTypes[] = { "ext2","ext3","ext4","btrfs","xfs","cifs","nfs","usbfs","vfat","fuseblk",NULL };
static char *diskInfo = "/proc/mounts"; /* /etc/fstab */
static char *diskStats = "/proc/diskstats";
static char *typeNames[] = { "Reads", "Writes" };

void *diskActivity;
void *partitionInfo;

/**********************************************************************************************************************
 *                                                                                                                    *
 *  P A R T  Q U E U E  C O M P                                                                                       *
 *  ===========================                                                                                       *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Compare two components.
 *  \param item1 First item.
 *  \param item2 Second Item.
 *  \result Result of compare.
 */
int partQueueComp (void *item1, void *item2)
{
	PARTITION_INFO *partOne = (PARTITION_INFO *)item1;
	PARTITION_INFO *partTwo = (PARTITION_INFO *)item2;
	return strcmp (partOne -> tidyName, partTwo -> tidyName);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  D I S K  Q U E U E  C O M P                                                                                       *
 *  ===========================                                                                                       *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Compare two disk names.
 *  \param item1 First item.
 *  \param item2 Second Item.
 *  \result Result of compare.
 */
int diskQueueComp (void *item1, void *item2)
{
	DISK_INFO *diskOne = (DISK_INFO *)item1;
	DISK_INFO *diskTwo = (DISK_INFO *)item2;
	return strcmp (diskOne -> name, diskTwo -> name);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  T I D Y  P A R T I T I O N  N A M E                                                                               *
 *  ===================================                                                                               *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Convert a mount point into a short name.
 *  \param partInfo Partition name.
 *  \result None.
 */
void tidyPartitionName (PARTITION_INFO *partInfo)
{
	int i = 0, j = 0;

	while (partInfo -> diskName[i])
	{
		if (partInfo -> diskName[i] == '.' || partInfo -> diskName[i] <= ' ')
			;
		else if (partInfo -> diskName[i] == '/')
			j = 0;
		else if (j < 40)
		{
			partInfo -> tidyName[j] = partInfo -> diskName[i];
			partInfo -> tidyName[++j] = 0;
		}
		++i;
	}
	if (partInfo -> tidyName[0] == 0)
		strcpy (partInfo -> tidyName, "root");
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  R E A D  P A R T I T I O N  N A M E S                                                                             *
 *  =====================================                                                                             *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Read the partition names from the system.
 *  \result None.
 */
void readPartitionNames()
{
	FILE *fstab;
	char readBuff[256], readWord[256];
	int disk = 0, menu = 0;

	if ((fstab = fopen (diskInfo, "r")) != NULL)
	{
		while (fgets (readBuff, 255, fstab) && disk < MAX_PARTITIONS)
		{
			int i = 0, j = 0, w = 0;
			char diskName[81];

			diskName[0] = 0;
			while (readBuff[i])
			{
				if (readBuff[0] == '#' && w == 0)
					break;
				if (readBuff[i] > ' ')
				{
					if (j == 0)
					{
						if (++w == 4)
							break;
					}
					readWord[j] = readBuff[i];
					readWord[++j] = 0;
				}
				else if (j)
				{
					if (w == 2)
					{
						if (strncmp (readWord, "/proc", 5) == 0)
							break;
						readWord[78] = 0;
						strcpy (diskName, readWord);
						if (diskName[j - 1] != '/') strcat (diskName, "/");
						strcat (diskName, ".");
					}
					if (w == 3)
					{
						j = 0;
						while (diskTypes[j])
						{
							if (strcmp (readWord, diskTypes[j]) == 0)
							{
								PARTITION_INFO *partInfo = (PARTITION_INFO *)malloc (sizeof (PARTITION_INFO));
								memset (partInfo, 0, sizeof (PARTITION_INFO));
								strcpy (partInfo -> diskName, diskName);
								tidyPartitionName (partInfo);
								queuePutSort (partitionInfo, partInfo, partQueueComp);
								++disk;
								break;
							}
							++j;
						}
					}
					j = 0;
				}
				++i;
			}
		}
		while (menu < disk)
		{
			PARTITION_INFO *partInfo = queueRead (partitionInfo, menu);
			if (partInfo != NULL)
			{
				spaceMenuDesc[menu].disable = 0;
				spaceMenuDesc[menu].menuName = partInfo -> tidyName;
				gaugeMenuDesc[MENU_GAUGE_HARDDISK].disable = 0;
			}
			++menu;
		}
		fclose (fstab);
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  G E T  P A R T I T I O N  F R E E  S P A C E                                                                      *
 *  ============================================                                                                      *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Calculate the free space for a disk.
 *  \param partInfo Caclulate the free space.
 *  \param partTotal Return the total partition size.
 *  \param partSize Return the used size.
 *  \result Percent of disk used.
 */
float getPartitionFreeSpace (PARTITION_INFO *partInfo, unsigned long long *partTotal, unsigned long long *partSize)
{
	struct statvfs stats;

	if (statvfs (partInfo -> diskName, &stats) == 0)
	{
		unsigned long long total = (unsigned long long)stats.f_blocks * stats.f_frsize / 1024;
		unsigned long long available = (unsigned long long)stats.f_bavail * stats.f_frsize / 1024;
		unsigned long long free = (unsigned long long)stats.f_bfree * stats.f_frsize / 1024;
		unsigned long long used = total - free;
		unsigned long long u100 = 0, nonroot_total = 0;

		u100 = used * 100;
		nonroot_total = used + available;
		*partTotal = total;
		*partSize = used;
		return (float)u100 / nonroot_total;
	}
	*partTotal = *partSize = 0;
	return 0;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S E T  A C T I V I T Y  S C A L E                                                                                 *
 *  =================================                                                                                 *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Calculate the best scale for this gauge.
 *  \param values Values to scale.
 *  \result None (saved in structure).
 */
void setActivityScale (struct diskValues *values)
{
	int scale = 1, i;
	while ((values -> rate / scale) > 100)
		scale *= 10;

	values -> useScale = scale;
	for (i = 0; i < (MAX_SCALE_MEM - 1); ++i)
	{
		values -> oldScales[i] = values -> oldScales[i + 1];
		if (values -> oldScales[i] > values -> useScale)
			values -> useScale = values -> oldScales[i];
	}
	values -> oldScales[i] = scale;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  R E A D  A C T I V I T Y  V A L U E S                                                                             *
 *  =====================================                                                                             *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Read the number of sectors read.
 *  \result None.
 */
void readActivityValues()
{
	FILE *diskstats;
	struct timeval tvTaken;
	DISK_INFO *allDiskInfo = NULL, *thisDiskInfo = NULL;
	char readBuff[256], readWord[256];
	long thisTime = 0, readTime;
	int disk = 1;

	gettimeofday (&tvTaken, NULL);
	thisTime = ((tvTaken.tv_sec % 100000) * 1000) + (tvTaken.tv_usec / 1000);
	readTime = thisTime - lastTime;
	lastTime = thisTime;
	if (!readTime) return;

	allDiskInfo = queueRead (diskActivity, 0);
	allDiskInfo -> secRead.value = allDiskInfo -> secRead.rate = 0;
	allDiskInfo -> secWrite.value = allDiskInfo -> secWrite.rate = 0;

	if ((diskstats = fopen (diskStats, "r")) != NULL)
	{
		while (fgets (readBuff, 255, diskstats) && disk < (MAX_DISKS + 1))
		{
			int i = 0, j = 0, w = 0;
			while (readBuff[i])
			{
				if (readBuff[0] == '#' && w == 0)
					break;
				if (readBuff[i] > ' ')
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
					if (w == 3)
					{
						int k = 0, f = 0;
						DISK_INFO *tmpDiskInfo = NULL;

						if (strncmp (readWord, "ram", 3) == 0 || strncmp (readWord, "loop", 4) == 0)
						{
							/* Ignore ram and loop diska */
							break;
						}
						/* Find this disk, only look for sda ignore sda1 */
						do
						{
							tmpDiskInfo = queueRead (diskActivity, ++k);
							if (tmpDiskInfo != NULL)
							{
								if (strcmp (tmpDiskInfo -> name, readWord) == 0)
								{
									thisDiskInfo = tmpDiskInfo;
									f = 1;
								}
								else if (strncmp (tmpDiskInfo -> name, readWord, strlen (tmpDiskInfo -> name)) == 0)
								{
									f = 2;
								}
							}
						}
						while (tmpDiskInfo != NULL && !f);

						if (f == 2)
						{
							break;
						}
						else if (f == 0)
						{
							/* Create a new entry for this disk */
							thisDiskInfo = (DISK_INFO *)malloc (sizeof (DISK_INFO));
							memset (thisDiskInfo, 0, sizeof (DISK_INFO));
							strncpy (thisDiskInfo -> name, readWord, 40);
							thisDiskInfo -> name[40] = 0;
							queuePut (diskActivity, thisDiskInfo);
						}
					}
					if (w == 6)
					{
						unsigned long long diff;
						unsigned long long value = atoll (readWord);
						if (value < thisDiskInfo -> secRead.value)
						{
							thisDiskInfo -> secRead.value = value;
							break;
						}
						diff = value - thisDiskInfo -> secRead.value;
						thisDiskInfo -> secRead.rate = (diff * 1000) / readTime;
						thisDiskInfo -> secRead.value = value;
						allDiskInfo -> secRead.value += diff;
					}
					if (w == 10)
					{
						unsigned long long diff;
						unsigned long long value = atoll (readWord);

						if (value < thisDiskInfo -> secWrite.value)
						{
							thisDiskInfo -> secWrite.value = value;
							break;
						}
						diff = value - thisDiskInfo -> secWrite.value;
						thisDiskInfo -> secWrite.rate = (diff * 1000) / readTime;
						thisDiskInfo -> secWrite.value = value;
						allDiskInfo -> secWrite.value += diff;

						setActivityScale (&thisDiskInfo -> secRead);
						setActivityScale (&thisDiskInfo -> secWrite);

						diskMenuDesc[disk].disable = 0;
						diskMenuDesc[disk].menuName = thisDiskInfo -> name;
						++disk;
					}
					j = 0;
				}
				++i;
			}
		}
		fclose (diskstats);
	}
	allDiskInfo -> secRead.rate = (allDiskInfo -> secRead.value * 1000) / readTime;
	allDiskInfo -> secWrite.rate = (allDiskInfo -> secWrite.value * 1000) / readTime;
	setActivityScale (&allDiskInfo -> secRead);
	setActivityScale (&allDiskInfo -> secWrite);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  R E A D  H A R D D I S K  I N I T                                                                                 *
 *  =================================                                                                                 *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Call at application start to read the fstab and get disk names.
 *  \result None.
 */
void readHarddiskInit (void)
{
	if (gaugeEnabled[FACE_TYPE_HARDDISK].enabled)
	{
		DISK_INFO *diskInfo = (DISK_INFO *)malloc (sizeof (DISK_INFO));
		memset (diskInfo, 0, sizeof (DISK_INFO));
		strcpy (diskInfo -> name, "All");
		diskActivity = queueCreate();
		queuePut (diskActivity, diskInfo);
		partitionInfo = queueCreate();
		readPartitionNames();
		readActivityValues();
	}
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S I Z E  T O  S T R I N G                                                                                         *
 *  =========================                                                                                         *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Convert a size in to a string.
 *  \param partSize Size to convert.
 *  \param sizeStr String to sve it in.
 *  \result Pointer to the string.
 */
char *sizeToString (unsigned long long partSize, char *sizeStr)
{
	char codes[] = "KMGTPEZY";
	unsigned long long divider = 1;
	int step = 0;

	while (partSize / divider > 1000)
	{
		divider *= 1000;
		++step;
	}
	if (partSize / divider > 100)
		sprintf (sizeStr, "%0.0f%c", (float)partSize / (float)divider, codes[step]);
	else if (partSize / divider > 10)
		sprintf (sizeStr, "%0.1f%c", (float)partSize / (float)divider, codes[step]);
	else
		sprintf (sizeStr, "%0.2f%c", (float)partSize / (float)divider, codes[step]);

	return sizeStr;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  R E A D  H A R D D I S K  V A L U E S                                                                             *
 *  =====================================                                                                             *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Calculate the values to show on the gauge.
 *  \param face Not used.
 *  \result None.
 */
void readHarddiskValues (int face)
{
	if (gaugeEnabled[FACE_TYPE_HARDDISK].enabled)
	{
		FACE_SETTINGS *faceSetting = faceSettings[face];

		if (faceSetting -> faceFlags & FACE_REDRAW)
		{
			;
		}
		else if (sysUpdateID % (faceSetting -> faceSubType & 0x0300 ? 10 : 50) != 0)
		{
			return;
		}
		if (faceSetting -> faceSubType & 0x0300)
		{
			DISK_INFO *thisDiskInfo = NULL;
			int scale, disk = faceSetting -> faceSubType & 0x00FF;
			unsigned long value = 0;
			char *nameT, *nameD;

			if (myUpdateID != sysUpdateID)
			{
				readActivityValues();
				myUpdateID = sysUpdateID;
			}
			thisDiskInfo = queueRead (diskActivity, disk);
			if (thisDiskInfo != NULL)
			{
				nameD = thisDiskInfo -> name;
				if (faceSetting -> faceSubType & 0x0100)
				{
					nameT = typeNames[0];
					scale = thisDiskInfo -> secRead.useScale;
					faceSetting -> firstValue = value = thisDiskInfo -> secRead.rate;
				}
				else
				{
					nameT = typeNames[1];
					scale = thisDiskInfo -> secWrite.useScale;
					faceSetting -> firstValue = value = thisDiskInfo -> secWrite.rate;
				}
				faceSetting -> firstValue /= scale;
				setFaceString (faceSetting, FACESTR_TOP, 0, _("%s\n(%s)"), gettext (nameT), nameD);
				setFaceString (faceSetting, FACESTR_TIP, 0, _("<b>Sector %s</b>: %lu/sec (%s)"), gettext (nameT), value, nameD);
				if (scale > 1)
					setFaceString (faceSetting, FACESTR_BOT, 0, _("%0.1f/sec\nx%d"), faceSetting -> firstValue, scale);
				else
					setFaceString (faceSetting, FACESTR_BOT, 0, _("%0.1f/sec"), faceSetting -> firstValue);
				setFaceString (faceSetting, FACESTR_WIN, 0, _("Sector %s - Gauge"), nameT);

				if (faceSetting -> updateNum != scale)
				{
					maxMinReset (&faceSetting -> savedMaxMin, 10, 2);
					faceSetting -> faceFlags |= FACE_REDRAW;
					faceSetting -> updateNum = scale;
				}
			}
		}
		else
		{
			PARTITION_INFO *partInfo = queueRead (partitionInfo, faceSetting -> faceSubType);
			if (partInfo != NULL)
			{
				char sizeStr[2][41];
				unsigned long long partTotal, partSize;

				faceSetting -> firstValue = getPartitionFreeSpace(partInfo, &partTotal, &partSize);
				sizeToString (partTotal, sizeStr[0]);
				sizeToString (partSize, sizeStr[1]);

				setFaceString (faceSetting, FACESTR_TOP, 0, _("Partition\n%s"), partInfo -> tidyName);
				setFaceString (faceSetting, FACESTR_TIP, 0, _("<b>Used Space</b>: %0.1f%% (%s)\n<b>Total Size</b>: %s"),
						faceSetting -> firstValue, sizeStr[1], sizeStr[0]);
				setFaceString (faceSetting, FACESTR_WIN, 0, _("Partition Space: %0.1f%% Used - Gauge"), faceSetting -> firstValue);
				setFaceString (faceSetting, FACESTR_BOT, 0, _("%0.1f%%\n%s"), faceSetting -> firstValue, sizeStr[0]);
			}
		}
	}
}


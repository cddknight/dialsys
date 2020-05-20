/**********************************************************************************************************************
 *                                                                                                                    *
 *  G A U G E  H A R D D I S K . C                                                                                    *
 *  ==============================                                                                                    *
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
 *  \brief Create gauges for hard disk info.
 */
#include <stdio.h>
#include <string.h>
#include <sys/statvfs.h>

#include "GaugeDisp.h"

#define MAX_PARTITIONS	10
#define MAX_DISKS		10
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

static int myUpdateID = 100;
static long lastTime;
static char diskNames[MAX_PARTITIONS][81];
static char tidyNames[MAX_PARTITIONS][41];
static char *diskTypes[] = { "ext2","ext3","ext4","btrfs","xfs","cifs","nfs","usbfs","vfat","fuseblk",NULL };
static char *diskInfo = "/proc/mounts"; /* /etc/fstab */
static char *diskStats = "/proc/diskstats";
static char *typeNames[] = { "Reads", "Writes" };
DISK_INFO diskActivity[MAX_DISKS + 1] =
{
	{ __("All") }
};

/**********************************************************************************************************************
 *                                                                                                                    *
 *  T I D Y  P A R T I T I O N  N A M E                                                                               *
 *  ===================================                                                                               *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Convert a mount point into a short name.
 *  \param disk The number of the disk drive.
 *  \param fullName Full partition name.
 *  \result None.
 */
void tidyPartitionName (int disk, char *fullName)
{
	int i = 0, j = 0;
	if (disk < 10)
	{
		while (fullName[i])
		{
			if (fullName[i] == '.' || fullName[i] <= ' ')
				;
			else if (fullName[i] == '/')
				j = 0;
			else if (j < 40)
			{
				tidyNames[disk][j] = fullName[i];
				tidyNames[disk][++j] = 0;
			}
			++i;
		}
		if (tidyNames[disk][0] == 0)
			strcpy (tidyNames[disk], "root");
	}
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
	int disk = 0;

	if ((fstab = fopen (diskInfo, "r")) != NULL)
	{
		while (fgets (readBuff, 255, fstab) && disk < MAX_PARTITIONS)
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
						strcpy (diskNames[disk], readWord);
						if (diskNames[disk][j - 1] != '/') strcat (diskNames[disk], "/");
						strcat (diskNames[disk], ".");
					}
					if (w == 3)
					{
						j = 0;
						while (diskTypes[j])
						{
							if (strcmp (readWord, diskTypes[j]) == 0)
							{
								tidyPartitionName (disk, diskNames[disk]);
								spaceMenuDesc[disk].disable = 0;
								spaceMenuDesc[disk].menuName = tidyNames[disk];
								gaugeMenuDesc[MENU_GAUGE_HARDDISK].disable = 0;
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
 *  \param disk Disk to calculate for.
 *  \param partTotal Return the total partition size.
 *  \param partSize Return the used size.
 *  \result Percent of disk used.
 */
float getPartitionFreeSpace (int disk, unsigned long long *partTotal, unsigned long long *partSize)
{
	struct statvfs stats;
	if (diskNames[disk])
	{
		if (statvfs (diskNames[disk], &stats) == 0)
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
	char readBuff[256], readWord[256];
	long thisTime = 0, readTime;
	int disk = 1;

	gettimeofday (&tvTaken, NULL);
	thisTime = ((tvTaken.tv_sec % 100000) * 1000) + (tvTaken.tv_usec / 1000);
	readTime = thisTime - lastTime;
	lastTime = thisTime;
	if (!readTime) return;

	diskActivity[0].secRead.value = diskActivity[0].secRead.rate = 0;
	diskActivity[0].secWrite.value = diskActivity[0].secWrite.rate = 0;

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
						int k, f = 0;
						if (strncmp (readWord, "ram", 3) == 0 || strncmp (readWord, "loop", 4) == 0)
						{
							/* Ignore ram and loop diska */
							break;
						}
						for (k = 1; k < disk; ++k)
						{
							if (strncmp (diskActivity[k].name, readWord, strlen (diskActivity[k].name)) == 0)
							{
								f = 1;
								break;
							}
						}
						if (f)
						{
							break;
						}
						else
						{
							strncpy (diskActivity[disk].name, readWord, 40);
						}
					}
					if (w == 6)
					{
						unsigned long long diff;
						unsigned long long value = atoll (readWord);
						if (value < diskActivity[disk].secRead.value)
						{
							diskActivity[disk].secRead.value = value;
							break;
						}
						diff = value - diskActivity[disk].secRead.value;
						diskActivity[disk].secRead.rate = (diff * 1000) / readTime;
						diskActivity[disk].secRead.value = value;
						diskActivity[0].secRead.value += diff;
					}
					if (w == 10)
					{
						unsigned long long diff;
						unsigned long long value = atoll (readWord);
						if (value < diskActivity[disk].secWrite.value)
						{
							diskActivity[disk].secWrite.value = value;
							break;
						}
						diff = value - diskActivity[disk].secWrite.value;
						diskActivity[disk].secWrite.rate = (diff * 1000) / readTime;
						diskActivity[disk].secWrite.value = value;
						diskActivity[0].secWrite.value += diff;

						setActivityScale (&diskActivity[disk].secRead);
						setActivityScale (&diskActivity[disk].secWrite);

						diskMenuDesc[disk].disable = 0;
						diskMenuDesc[disk].menuName = diskActivity[disk].name;
						++disk;
					}
					j = 0;
				}
				++i;
			}
		}
		fclose (diskstats);
	}
	diskActivity[0].secRead.rate = (diskActivity[0].secRead.value * 1000) / readTime;
	diskActivity[0].secWrite.rate = (diskActivity[0].secWrite.value * 1000) / readTime;
	setActivityScale (&diskActivity[0].secRead);
	setActivityScale (&diskActivity[0].secWrite);
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
		if (myUpdateID != sysUpdateID)
		{
			readActivityValues();
			myUpdateID = sysUpdateID;
		}
		if (faceSetting -> faceSubType & 0x0300)
		{
			int scale, disk = faceSetting -> faceSubType & 0x00FF;
			unsigned long value = 0;
			char *nameT, *nameD;

			nameD = diskActivity[disk].name;
			if (faceSetting -> faceSubType & 0x0100)
			{
				nameT = typeNames[0];
				scale = diskActivity[disk].secRead.useScale;
				faceSetting -> firstValue = value = diskActivity[disk].secRead.rate;
			}
			else
			{
				nameT = typeNames[1];
				scale = diskActivity[disk].secWrite.useScale;
				faceSetting -> firstValue = value = diskActivity[disk].secWrite.rate;
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
		else
		{
			char sizeStr[2][41];
			unsigned long long partTotal, partSize;

			faceSetting -> firstValue = getPartitionFreeSpace(faceSetting -> faceSubType, &partTotal, &partSize);
			sizeToString (partTotal, sizeStr[0]);
			sizeToString (partSize, sizeStr[1]);

			setFaceString (faceSetting, FACESTR_TOP, 0, _("Partition\n%s"), tidyNames[faceSetting -> faceSubType]);
			setFaceString (faceSetting, FACESTR_TIP, 0, _("<b>Used Space</b>: %0.1f%% (%s)\n<b>Total Size</b>: %s"),
					faceSetting -> firstValue, sizeStr[1], sizeStr[0]);
			setFaceString (faceSetting, FACESTR_WIN, 0, _("Partition Space: %0.1f%% Used - Gauge"), faceSetting -> firstValue);
			setFaceString (faceSetting, FACESTR_BOT, 0, _("%0.1f%%\n%s"), faceSetting -> firstValue, sizeStr[0]);
		}
	}
}

